#include "databasemanager.h"
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QDateTime>
#include <QDebug>
#include <sstream>

DatabaseManager::DatabaseManager() {
}

DatabaseManager::~DatabaseManager() {
    close();
}

DatabaseManager& DatabaseManager::instance() {
    static DatabaseManager mgr;
    return mgr;
}

bool DatabaseManager::connectToDatabase(const QString& host,
                                       int port,
                                       const QString& dbName,
                                       const QString& user,
                                       const QString& password)
{
    close();

    conn = mysql_init(nullptr);
    if (!conn) {
        m_lastError = "Failed to initialize MySQL structure.";
        return false;
    }

    // Set connection timeout
    unsigned int timeout = 5;
    mysql_options(conn, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);

    // Connect to MySQL 8.0 server
    if (!mysql_real_connect(conn,
                            host.toUtf8().constData(),
                            user.toUtf8().constData(),
                            password.toUtf8().constData(),
                            dbName.toUtf8().constData(),
                            port,
                            nullptr,
                            0))
    {
        m_lastError = QString::fromUtf8(mysql_error(conn));
        mysql_close(conn);
        conn = nullptr;
        return false;
    }

    // Set UTF-8 charset
    mysql_set_character_set(conn, "utf8mb4");

    m_lastError.clear();
    return true;
}

bool DatabaseManager::isConnected() const {
    return (conn != nullptr);
}

QString DatabaseManager::lastError() const {
    return m_lastError;
}

void DatabaseManager::close() {
    if (conn) {
        mysql_close(conn);
        conn = nullptr;
    }
}

QString DatabaseManager::escape(const QString& str) const {
    if (!conn || str.isEmpty()) return str;
    QByteArray utf8 = str.toUtf8();
    QByteArray buffer;
    buffer.resize(utf8.size() * 2 + 1);
    unsigned long len = mysql_real_escape_string(conn, buffer.data(), utf8.constData(), utf8.size());
    return QString::fromUtf8(buffer.constData(), len);
}

// ──────────────────────────────────────────────────────────
// Password Hashing with Salt & Iterated SHA-256
// Format: sha256$iterations$saltHex$hashHex
// ──────────────────────────────────────────────────────────
QString DatabaseManager::hashPassword(const QString& plainPassword) {
    const int iterations = 10000;
    const int saltLength = 16;

    QByteArray salt;
    salt.resize(saltLength);
    for (int i = 0; i < saltLength; ++i) {
        salt[i] = static_cast<char>(QRandomGenerator::global()->bounded(256));
    }

    QByteArray hash = plainPassword.toUtf8() + salt;
    for (int i = 0; i < iterations; ++i) {
        hash = QCryptographicHash::hash(hash + salt, QCryptographicHash::Sha256);
    }

    return QString("sha256$%1$%2$%3")
        .arg(iterations)
        .arg(QString::fromLatin1(salt.toHex()))
        .arg(QString::fromLatin1(hash.toHex()));
}

bool DatabaseManager::verifyPassword(const QString& plainPassword, const QString& storedHash) {
    if (storedHash.isEmpty()) return false;

    QStringList parts = storedHash.split('$');
    if (parts.size() != 4 || parts[0] != "sha256") {
        return false;
    }

    int iterations = parts[1].toInt();
    QByteArray salt = QByteArray::fromHex(parts[2].toLatin1());
    QByteArray expectedHash = QByteArray::fromHex(parts[3].toLatin1());

    QByteArray calculatedHash = plainPassword.toUtf8() + salt;
    for (int i = 0; i < iterations; ++i) {
        calculatedHash = QCryptographicHash::hash(calculatedHash + salt, QCryptographicHash::Sha256);
    }

    return (calculatedHash == expectedHash);
}

// ──────────────────────────────────────────────────────────
// Super Admin Management
// ──────────────────────────────────────────────────────────
bool DatabaseManager::hasAnySuperAdmin() {
    if (!isConnected()) return false;
    if (mysql_query(conn, "SELECT COUNT(*) FROM users WHERE role = 'SuperAdmin'")) {
        m_lastError = QString::fromUtf8(mysql_error(conn));
        return false;
    }
    MYSQL_RES* res = mysql_store_result(conn);
    if (!res) return false;
    MYSQL_ROW row = mysql_fetch_row(res);
    bool hasAdmin = false;
    if (row && row[0]) {
        hasAdmin = (std::atoi(row[0]) > 0);
    }
    mysql_free_result(res);
    return hasAdmin;
}

bool DatabaseManager::ensureInitialSuperAdmin(const QString& username, const QString& password) {
    if (!isConnected()) return false;
    if (hasAnySuperAdmin()) return true;

    QString query = QString("INSERT INTO users (username, password_hash, role, department_id) "
                            "VALUES ('%1', '%2', 'SuperAdmin', NULL)")
                        .arg(escape(username.trimmed()))
                        .arg(escape(hashPassword(password)));

    if (mysql_query(conn, query.toUtf8().constData())) {
        m_lastError = QString::fromUtf8(mysql_error(conn));
        return false;
    }
    return true;
}

// ──────────────────────────────────────────────────────────
// Authentication
// ──────────────────────────────────────────────────────────
std::optional<User> DatabaseManager::authenticateUser(const QString& username, const QString& password) {
    if (!isConnected()) {
        m_lastError = "Database not connected.";
        return std::nullopt;
    }

    QString sql = QString("SELECT u.user_id, u.username, u.password_hash, u.role, u.department_id, d.name "
                          "FROM users u "
                          "LEFT JOIN departments d ON u.department_id = d.department_id "
                          "WHERE u.username = '%1' LIMIT 1")
                      .arg(escape(username.trimmed()));

    if (mysql_query(conn, sql.toUtf8().constData())) {
        m_lastError = QString::fromUtf8(mysql_error(conn));
        return std::nullopt;
    }

    MYSQL_RES* res = mysql_store_result(conn);
    if (!res) return std::nullopt;

    MYSQL_ROW row = mysql_fetch_row(res);
    if (!row) {
        mysql_free_result(res);
        m_lastError = "Invalid username or password.";
        return std::nullopt;
    }

    QString storedHash = QString::fromUtf8(row[2]);
    if (!verifyPassword(password, storedHash)) {
        mysql_free_result(res);
        m_lastError = "Invalid username or password.";
        return std::nullopt;
    }

    User user;
    user.id = std::atoll(row[0]);
    user.username = QString::fromUtf8(row[1]);
    QString roleStr = QString::fromUtf8(row[3]);
    user.role = (roleStr == "SuperAdmin") ? UserRole::SuperAdmin : UserRole::DeptAdmin;

    if (row[4] != nullptr) {
        user.departmentId = std::atoi(row[4]);
        if (row[5] != nullptr) {
            user.departmentName = QString::fromUtf8(row[5]);
        }
    }

    mysql_free_result(res);
    return user;
}

// ──────────────────────────────────────────────────────────
// Department Queries
// ──────────────────────────────────────────────────────────
std::vector<DepartmentRecord> DatabaseManager::getDepartments() {
    std::vector<DepartmentRecord> list;
    if (!isConnected()) return list;

    if (mysql_query(conn, "SELECT department_id, name FROM departments ORDER BY department_id ASC")) {
        m_lastError = QString::fromUtf8(mysql_error(conn));
        return list;
    }

    MYSQL_RES* res = mysql_store_result(conn);
    if (!res) return list;

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res))) {
        DepartmentRecord d;
        d.id = std::atoi(row[0]);
        d.name = QString::fromUtf8(row[1]);
        list.push_back(d);
    }
    mysql_free_result(res);
    return list;
}

std::optional<DepartmentRecord> DatabaseManager::getDepartmentById(int departmentId) {
    if (!isConnected()) return std::nullopt;
    QString sql = QString("SELECT department_id, name FROM departments WHERE department_id = %1").arg(departmentId);
    if (mysql_query(conn, sql.toUtf8().constData())) return std::nullopt;

    MYSQL_RES* res = mysql_store_result(conn);
    if (!res) return std::nullopt;
    MYSQL_ROW row = mysql_fetch_row(res);
    if (!row) {
        mysql_free_result(res);
        return std::nullopt;
    }

    DepartmentRecord d{std::atoi(row[0]), QString::fromUtf8(row[1])};
    mysql_free_result(res);
    return d;
}

std::optional<DepartmentRecord> DatabaseManager::getDepartmentByName(const QString& name) {
    if (!isConnected()) return std::nullopt;
    QString sql = QString("SELECT department_id, name FROM departments WHERE name = '%1'").arg(escape(name));
    if (mysql_query(conn, sql.toUtf8().constData())) return std::nullopt;

    MYSQL_RES* res = mysql_store_result(conn);
    if (!res) return std::nullopt;
    MYSQL_ROW row = mysql_fetch_row(res);
    if (!row) {
        mysql_free_result(res);
        return std::nullopt;
    }

    DepartmentRecord d{std::atoi(row[0]), QString::fromUtf8(row[1])};
    mysql_free_result(res);
    return d;
}

// ──────────────────────────────────────────────────────────
// User Management (Super Admin only)
// ──────────────────────────────────────────────────────────
bool DatabaseManager::createUser(const User& caller,
                                const QString& username,
                                const QString& password,
                                UserRole role,
                                std::optional<int> departmentId)
{
    if (!caller.isSuperAdmin()) {
        m_lastError = "Access denied: Only Super Admin can create accounts.";
        return false;
    }

    if (username.trimmed().isEmpty() || password.isEmpty()) {
        m_lastError = "Username and password cannot be empty.";
        return false;
    }

    if (role == UserRole::DeptAdmin && !departmentId.has_value()) {
        m_lastError = "A Department Admin must be assigned to a department.";
        return false;
    }

    QString deptVal = (role == UserRole::DeptAdmin && departmentId.has_value())
                          ? QString::number(departmentId.value())
                          : "NULL";

    QString sql = QString("INSERT INTO users (username, password_hash, role, department_id) "
                          "VALUES ('%1', '%2', '%3', %4)")
                      .arg(escape(username.trimmed()))
                      .arg(escape(hashPassword(password)))
                      .arg((role == UserRole::SuperAdmin) ? "SuperAdmin" : "DeptAdmin")
                      .arg(deptVal);

    if (mysql_query(conn, sql.toUtf8().constData())) {
        m_lastError = QString::fromUtf8(mysql_error(conn));
        return false;
    }
    return true;
}

std::vector<User> DatabaseManager::getAllUsers(const User& caller) {
    std::vector<User> list;
    if (!caller.isSuperAdmin() || !isConnected()) return list;

    const char* sql = "SELECT u.user_id, u.username, u.role, u.department_id, d.name "
                      "FROM users u "
                      "LEFT JOIN departments d ON u.department_id = d.department_id "
                      "ORDER BY u.user_id ASC";

    if (mysql_query(conn, sql)) {
        m_lastError = QString::fromUtf8(mysql_error(conn));
        return list;
    }

    MYSQL_RES* res = mysql_store_result(conn);
    if (!res) return list;

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res))) {
        User u;
        u.id = std::atoll(row[0]);
        u.username = QString::fromUtf8(row[1]);
        QString r = QString::fromUtf8(row[2]);
        u.role = (r == "SuperAdmin") ? UserRole::SuperAdmin : UserRole::DeptAdmin;
        if (row[3] != nullptr) {
            u.departmentId = std::atoi(row[3]);
            if (row[4] != nullptr) {
                u.departmentName = QString::fromUtf8(row[4]);
            }
        }
        list.push_back(u);
    }
    mysql_free_result(res);
    return list;
}

bool DatabaseManager::updateUserDepartment(const User& caller, long long userId, int departmentId) {
    if (!caller.isSuperAdmin()) {
        m_lastError = "Access denied: Only Super Admin can modify department assignments.";
        return false;
    }

    QString sql = QString("UPDATE users SET department_id = %1 WHERE user_id = %2 AND role = 'DeptAdmin'")
                      .arg(departmentId).arg(userId);

    if (mysql_query(conn, sql.toUtf8().constData())) {
        m_lastError = QString::fromUtf8(mysql_error(conn));
        return false;
    }
    return true;
}

bool DatabaseManager::deleteUser(const User& caller, long long userId) {
    if (!caller.isSuperAdmin()) {
        m_lastError = "Access denied: Only Super Admin can delete users.";
        return false;
    }
    if (caller.id == userId) {
        m_lastError = "You cannot delete your own account.";
        return false;
    }

    QString sql = QString("DELETE FROM users WHERE user_id = %1").arg(userId);
    if (mysql_query(conn, sql.toUtf8().constData())) {
        m_lastError = QString::fromUtf8(mysql_error(conn));
        return false;
    }
    return true;
}

bool DatabaseManager::checkDepartmentPermission(const User& caller, int complaintDepartmentId) {
    if (caller.isSuperAdmin() || caller.isCitizen()) return true;
    if (caller.isDeptAdmin() && caller.departmentId.has_value()) {
        return (caller.departmentId.value() == complaintDepartmentId);
    }
    return false;
}

// ──────────────────────────────────────────────────────────
// Complaint Management
// ──────────────────────────────────────────────────────────
bool DatabaseManager::registerComplaint(const Complaint& c, int& outGeneratedId) {
    if (!isConnected()) return false;

    auto deptOpt = getDepartmentByName(QString::fromStdString(c.department));
    int deptId = deptOpt.has_value() ? deptOpt->id : 6;

    QString sql = QString("INSERT INTO complaints "
                          "(citizen_name, citizen_contact, category, description, location, department_id, priority, status) "
                          "VALUES ('%1', '%2', '%3', '%4', '%5', %6, '%7', '%8')")
                      .arg(escape(QString::fromStdString(c.citizen)))
                      .arg(escape(QString::fromStdString(c.contact)))
                      .arg(escape(QString::fromStdString(c.category)))
                      .arg(escape(QString::fromStdString(c.description)))
                      .arg(escape(QString::fromStdString(c.location)))
                      .arg(deptId)
                      .arg(escape(QString::fromStdString(priorityName(c.priority))))
                      .arg(escape(QString::fromStdString(c.status.empty() ? "Registered" : c.status)));

    if (mysql_query(conn, sql.toUtf8().constData())) {
        m_lastError = QString::fromUtf8(mysql_error(conn));
        return false;
    }

    outGeneratedId = static_cast<int>(mysql_insert_id(conn));
    return true;
}

std::optional<Complaint> DatabaseManager::getComplaintById(const User& caller, int id) {
    if (!isConnected()) return std::nullopt;

    QString sql = QString("SELECT c.complaint_id, c.citizen_name, c.citizen_contact, c.category, "
                          "c.description, c.location, c.department_id, d.name AS dept_name, "
                          "c.priority, c.status, c.created_at "
                          "FROM complaints c "
                          "JOIN departments d ON c.department_id = d.department_id "
                          "WHERE c.complaint_id = %1").arg(id);

    if (mysql_query(conn, sql.toUtf8().constData())) return std::nullopt;

    MYSQL_RES* res = mysql_store_result(conn);
    if (!res) return std::nullopt;
    MYSQL_ROW row = mysql_fetch_row(res);
    if (!row) {
        mysql_free_result(res);
        return std::nullopt;
    }

    int deptId = std::atoi(row[6]);
    if (!checkDepartmentPermission(caller, deptId)) {
        mysql_free_result(res);
        m_lastError = "Access denied: You are not authorized to view complaints outside your department.";
        return std::nullopt;
    }

    Complaint comp;
    comp.id = std::atoi(row[0]);
    comp.citizen = row[1] ? row[1] : "";
    comp.contact = row[2] ? row[2] : "";
    comp.category = row[3] ? row[3] : "";
    comp.description = row[4] ? row[4] : "";
    comp.location = row[5] ? row[5] : "";
    comp.department = row[7] ? row[7] : "";
    comp.priority = priorityFromName(row[8] ? row[8] : "Low");
    comp.status = row[9] ? row[9] : "";
    comp.createdAt = row[10] ? row[10] : "";

    mysql_free_result(res);
    return comp;
}

std::vector<Complaint> DatabaseManager::getComplaints(const User& caller,
                                                     const QString& statusFilter,
                                                     const QString& searchKeyword)
{
    std::vector<Complaint> list;
    if (!isConnected()) return list;

    QString sql = "SELECT c.complaint_id, c.citizen_name, c.citizen_contact, c.category, "
                  "c.description, c.location, c.department_id, d.name AS dept_name, "
                  "c.priority, c.status, c.created_at "
                  "FROM complaints c "
                  "JOIN departments d ON c.department_id = d.department_id "
                  "WHERE 1=1 ";

    // Backend role enforcement
    if (caller.isDeptAdmin()) {
        if (!caller.departmentId.has_value()) return list;
        sql += QString(" AND c.department_id = %1 ").arg(caller.departmentId.value());
    }

    if (!statusFilter.isEmpty() && statusFilter != "All") {
        sql += QString(" AND c.status = '%1' ").arg(escape(statusFilter));
    }

    if (!searchKeyword.trimmed().isEmpty()) {
        QString kw = escape(searchKeyword.trimmed());
        sql += QString(" AND (c.complaint_id LIKE '%%1%' OR c.citizen_name LIKE '%%1%' OR c.category LIKE '%%1%' OR c.description LIKE '%%1%' OR c.location LIKE '%%1%') ")
                   .arg(kw);
    }

    sql += " ORDER BY c.complaint_id DESC";

    if (mysql_query(conn, sql.toUtf8().constData())) {
        m_lastError = QString::fromUtf8(mysql_error(conn));
        return list;
    }

    MYSQL_RES* res = mysql_store_result(conn);
    if (!res) return list;

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res))) {
        Complaint comp;
        comp.id = std::atoi(row[0]);
        comp.citizen = row[1] ? row[1] : "";
        comp.contact = row[2] ? row[2] : "";
        comp.category = row[3] ? row[3] : "";
        comp.description = row[4] ? row[4] : "";
        comp.location = row[5] ? row[5] : "";
        comp.department = row[7] ? row[7] : "";
        comp.priority = priorityFromName(row[8] ? row[8] : "Low");
        comp.status = row[9] ? row[9] : "";
        comp.createdAt = row[10] ? row[10] : "";
        list.push_back(comp);
    }
    mysql_free_result(res);
    return list;
}

bool DatabaseManager::updateComplaintStatus(const User& caller,
                                            int complaintId,
                                            const QString& newStatus,
                                            const QString& notes)
{
    if (!isConnected()) return false;

    if (caller.isCitizen()) {
        m_lastError = "Access denied: Citizens are not authorized to update grievance statuses.";
        return false;
    }

    // Verify department access
    QString checkSql = QString("SELECT department_id, status FROM complaints WHERE complaint_id = %1").arg(complaintId);
    if (mysql_query(conn, checkSql.toUtf8().constData())) return false;

    MYSQL_RES* checkRes = mysql_store_result(conn);
    if (!checkRes) return false;
    MYSQL_ROW row = mysql_fetch_row(checkRes);
    if (!row) {
        mysql_free_result(checkRes);
        m_lastError = "Complaint ID not found.";
        return false;
    }

    int deptId = std::atoi(row[0]);
    QString prevStatus = QString::fromUtf8(row[1] ? row[1] : "");
    mysql_free_result(checkRes);

    if (!checkDepartmentPermission(caller, deptId)) {
        m_lastError = "Unauthorized: You cannot update complaints assigned to other departments.";
        return false;
    }

    // Update status
    QString updateSql = QString("UPDATE complaints SET status = '%1' WHERE complaint_id = %2")
                            .arg(escape(newStatus)).arg(complaintId);
    if (mysql_query(conn, updateSql.toUtf8().constData())) {
        m_lastError = QString::fromUtf8(mysql_error(conn));
        return false;
    }

    // Insert into status_history audit table
    QString histSql = QString("INSERT INTO status_history (complaint_id, previous_status, new_status, changed_by, notes) "
                              "VALUES (%1, '%2', '%3', %4, '%5')")
                          .arg(complaintId)
                          .arg(escape(prevStatus))
                          .arg(escape(newStatus))
                          .arg(caller.id)
                          .arg(escape(notes));

    if (mysql_query(conn, histSql.toUtf8().constData())) {
        m_lastError = QString::fromUtf8(mysql_error(conn));
        return false;
    }

    return true;
}

std::vector<StatusHistoryRecord> DatabaseManager::getComplaintHistory(const User& caller, int complaintId) {
    std::vector<StatusHistoryRecord> list;
    if (!isConnected()) return list;

    // Check permission first
    QString checkSql = QString("SELECT department_id FROM complaints WHERE complaint_id = %1").arg(complaintId);
    if (mysql_query(conn, checkSql.toUtf8().constData())) return list;
    MYSQL_RES* cr = mysql_store_result(conn);
    if (!cr) return list;
    MYSQL_ROW crow = mysql_fetch_row(cr);
    if (!crow) {
        mysql_free_result(cr);
        return list;
    }
    int deptId = std::atoi(crow[0]);
    mysql_free_result(cr);

    if (!checkDepartmentPermission(caller, deptId)) return list;

    QString sql = QString("SELECT h.history_id, h.complaint_id, h.previous_status, h.new_status, "
                          "h.changed_by, u.username, h.notes, h.changed_at "
                          "FROM status_history h "
                          "LEFT JOIN users u ON h.changed_by = u.user_id "
                          "WHERE h.complaint_id = %1 "
                          "ORDER BY h.history_id ASC").arg(complaintId);

    if (mysql_query(conn, sql.toUtf8().constData())) return list;

    MYSQL_RES* res = mysql_store_result(conn);
    if (!res) return list;

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res))) {
        StatusHistoryRecord rec;
        rec.historyId = std::atoll(row[0]);
        rec.complaintId = std::atoll(row[1]);
        rec.previousStatus = row[2] ? QString::fromUtf8(row[2]) : "";
        rec.newStatus = row[3] ? QString::fromUtf8(row[3]) : "";
        rec.changedByUserId = std::atoll(row[4]);
        rec.changedByUsername = row[5] ? QString::fromUtf8(row[5]) : "Unknown";
        rec.notes = row[6] ? QString::fromUtf8(row[6]) : "";
        rec.changedAt = row[7] ? QString::fromUtf8(row[7]) : "";
        list.push_back(rec);
    }
    mysql_free_result(res);
    return list;
}

ComplaintStatistics DatabaseManager::getStatistics(const User& caller) {
    ComplaintStatistics stats;
    if (!isConnected()) return stats;

    QString baseWhere = " WHERE 1=1 ";
    if (caller.isDeptAdmin()) {
        if (!caller.departmentId.has_value()) return stats;
        baseWhere += QString(" AND department_id = %1 ").arg(caller.departmentId.value());
    }

    auto runCount = [this](const QString& sql) -> int {
        if (mysql_query(conn, sql.toUtf8().constData())) return 0;
        MYSQL_RES* res = mysql_store_result(conn);
        if (!res) return 0;
        MYSQL_ROW row = mysql_fetch_row(res);
        int c = (row && row[0]) ? std::atoi(row[0]) : 0;
        mysql_free_result(res);
        return c;
    };

    stats.totalComplaints = runCount("SELECT COUNT(*) FROM complaints" + baseWhere);
    stats.pendingComplaints = runCount("SELECT COUNT(*) FROM complaints" + baseWhere + " AND status = 'Registered'");
    stats.inProgressComplaints = runCount("SELECT COUNT(*) FROM complaints" + baseWhere + " AND status IN ('In Progress', 'Urgent Processing')");
    stats.resolvedComplaints = runCount("SELECT COUNT(*) FROM complaints" + baseWhere + " AND status IN ('Resolved', 'Closed')");
    stats.urgentComplaints = runCount("SELECT COUNT(*) FROM complaints" + baseWhere + " AND priority = 'High' AND status NOT IN ('Resolved', 'Closed')");

    if (caller.isSuperAdmin()) {
        const char* dsql = "SELECT d.name, COUNT(c.complaint_id) "
                           "FROM departments d "
                           "LEFT JOIN complaints c ON d.department_id = c.department_id "
                           "GROUP BY d.department_id, d.name "
                           "ORDER BY d.department_id ASC";
        if (!mysql_query(conn, dsql)) {
            MYSQL_RES* res = mysql_store_result(conn);
            if (res) {
                MYSQL_ROW row;
                while ((row = mysql_fetch_row(res))) {
                    stats.departmentCounts.push_back({QString::fromUtf8(row[0]), std::atoi(row[1])});
                }
                mysql_free_result(res);
            }
        }
    } else if (caller.isDeptAdmin() && caller.departmentId.has_value()) {
        stats.departmentCounts.push_back({caller.departmentName, stats.totalComplaints});
    }

    return stats;
}

std::vector<Complaint> DatabaseManager::loadAllComplaintsForSystem() {
    std::vector<Complaint> list;
    if (!isConnected()) return list;

    const char* sql = "SELECT c.complaint_id, c.citizen_name, c.citizen_contact, c.category, "
                      "c.description, c.location, c.department_id, d.name AS dept_name, "
                      "c.priority, c.status, c.created_at "
                      "FROM complaints c "
                      "JOIN departments d ON c.department_id = d.department_id "
                      "ORDER BY c.complaint_id ASC";

    if (mysql_query(conn, sql)) return list;

    MYSQL_RES* res = mysql_store_result(conn);
    if (!res) return list;

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res))) {
        Complaint comp;
        comp.id = std::atoi(row[0]);
        comp.citizen = row[1] ? row[1] : "";
        comp.contact = row[2] ? row[2] : "";
        comp.category = row[3] ? row[3] : "";
        comp.description = row[4] ? row[4] : "";
        comp.location = row[5] ? row[5] : "";
        comp.department = row[7] ? row[7] : "";
        comp.priority = priorityFromName(row[8] ? row[8] : "Low");
        comp.status = row[9] ? row[9] : "";
        comp.createdAt = row[10] ? row[10] : "";
        list.push_back(comp);
    }
    mysql_free_result(res);
    return list;
}
