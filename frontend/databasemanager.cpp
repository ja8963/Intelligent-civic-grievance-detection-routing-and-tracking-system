#include "databasemanager.h"
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QVariant>
#include <QDateTime>
#include <QDebug>

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
    if (db.isOpen()) {
        db.close();
    }

    // Determine available driver
    QString driver = "QMYSQL";
    if (!QSqlDatabase::isDriverAvailable("QMYSQL")) {
        if (QSqlDatabase::isDriverAvailable("QODBC")) {
            driver = "QODBC";
        } else if (QSqlDatabase::isDriverAvailable("QSQLITE")) {
            driver = "QSQLITE";
        }
    }

    if (QSqlDatabase::contains("civic_connection")) {
        db = QSqlDatabase::database("civic_connection");
    } else {
        db = QSqlDatabase::addDatabase(driver, "civic_connection");
    }

    if (driver == "QMYSQL") {
        db.setHostName(host);
        db.setPort(port);
        db.setDatabaseName(dbName);
        db.setUserName(user);
        db.setPassword(password);
    } else if (driver == "QODBC") {
        QString dsn = QString("DRIVER={MySQL ODBC 8.0 Unicode Driver};SERVER=%1;PORT=%2;DATABASE=%3;UID=%4;PWD=%5;OPTION=3;")
                          .arg(host).arg(port).arg(dbName).arg(user).arg(password);
        db.setDatabaseName(dsn);
    } else {
        // Fallback SQLite file for local embedded use
        db.setDatabaseName("civic_grievance_local.db");
    }

    if (!db.open()) {
        m_lastError = db.lastError().text();
        qWarning() << "Database connection failed:" << m_lastError;
        return false;
    }

    m_lastError.clear();
    return true;
}

bool DatabaseManager::isConnected() const {
    return db.isOpen();
}

QString DatabaseManager::lastError() const {
    return m_lastError;
}

void DatabaseManager::close() {
    if (db.isOpen()) {
        db.close();
    }
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
// Initial Super Admin Setup
// ──────────────────────────────────────────────────────────
bool DatabaseManager::hasAnySuperAdmin() {
    if (!isConnected()) return false;
    QSqlQuery q(db);
    q.prepare("SELECT COUNT(*) FROM users WHERE role = 'SuperAdmin'");
    if (q.exec() && q.next()) {
        return q.value(0).toInt() > 0;
    }
    return false;
}

bool DatabaseManager::ensureInitialSuperAdmin(const QString& username, const QString& password) {
    if (!isConnected()) return false;
    if (hasAnySuperAdmin()) return true;

    QSqlQuery q(db);
    q.prepare("INSERT INTO users (username, password_hash, role, department_id) "
              "VALUES (:u, :p, 'SuperAdmin', NULL)");
    q.bindValue(":u", username);
    q.bindValue(":p", hashPassword(password));

    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    return true;
}

// ──────────────────────────────────────────────────────────
// Authentication
// ──────────────────────────────────────────────────────────
std::optional<User> DatabaseManager::authenticateUser(const QString& username, const QString& password) {
    if (!isConnected()) return std::nullopt;

    QSqlQuery q(db);
    q.prepare("SELECT u.user_id, u.username, u.password_hash, u.role, u.department_id, d.name "
              "FROM users u "
              "LEFT JOIN departments d ON u.department_id = d.department_id "
              "WHERE u.username = :u LIMIT 1");
    q.bindValue(":u", username.trimmed());

    if (!q.exec() || !q.next()) {
        m_lastError = "User not found or query failed.";
        return std::nullopt;
    }

    QString storedHash = q.value("password_hash").toString();
    if (!verifyPassword(password, storedHash)) {
        m_lastError = "Invalid credentials.";
        return std::nullopt;
    }

    User user;
    user.id = q.value("user_id").toLongLong();
    user.username = q.value("username").toString();
    QString roleStr = q.value("role").toString();
    user.role = (roleStr == "SuperAdmin") ? UserRole::SuperAdmin : UserRole::DeptAdmin;

    if (!q.value("department_id").isNull()) {
        user.departmentId = q.value("department_id").toInt();
        user.departmentName = q.value("name").toString();
    }

    return user;
}

// ──────────────────────────────────────────────────────────
// Department Queries
// ──────────────────────────────────────────────────────────
std::vector<DepartmentRecord> DatabaseManager::getDepartments() {
    std::vector<DepartmentRecord> list;
    if (!isConnected()) return list;

    QSqlQuery q(db);
    if (q.exec("SELECT department_id, name FROM departments ORDER BY department_id ASC")) {
        while (q.next()) {
            DepartmentRecord d;
            d.id = q.value(0).toInt();
            d.name = q.value(1).toString();
            list.push_back(d);
        }
    }
    return list;
}

std::optional<DepartmentRecord> DatabaseManager::getDepartmentById(int departmentId) {
    if (!isConnected()) return std::nullopt;
    QSqlQuery q(db);
    q.prepare("SELECT department_id, name FROM departments WHERE department_id = :id");
    q.bindValue(":id", departmentId);
    if (q.exec() && q.next()) {
        return DepartmentRecord{q.value(0).toInt(), q.value(1).toString()};
    }
    return std::nullopt;
}

std::optional<DepartmentRecord> DatabaseManager::getDepartmentByName(const QString& name) {
    if (!isConnected()) return std::nullopt;
    QSqlQuery q(db);
    q.prepare("SELECT department_id, name FROM departments WHERE name = :name");
    q.bindValue(":name", name);
    if (q.exec() && q.next()) {
        return DepartmentRecord{q.value(0).toInt(), q.value(1).toString()};
    }
    return std::nullopt;
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

    QSqlQuery q(db);
    q.prepare("INSERT INTO users (username, password_hash, role, department_id) "
              "VALUES (:u, :p, :r, :d)");
    q.bindValue(":u", username.trimmed());
    q.bindValue(":p", hashPassword(password));
    q.bindValue(":r", (role == UserRole::SuperAdmin) ? "SuperAdmin" : "DeptAdmin");
    if (role == UserRole::DeptAdmin && departmentId.has_value()) {
        q.bindValue(":d", departmentId.value());
    } else {
        q.bindValue(":d", QVariant(QMetaType(QMetaType::Int)));
    }

    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    return true;
}

std::vector<User> DatabaseManager::getAllUsers(const User& caller) {
    std::vector<User> list;
    if (!caller.isSuperAdmin() || !isConnected()) return list;

    QSqlQuery q(db);
    if (q.exec("SELECT u.user_id, u.username, u.role, u.department_id, d.name "
               "FROM users u "
               "LEFT JOIN departments d ON u.department_id = d.department_id "
               "ORDER BY u.user_id ASC"))
    {
        while (q.next()) {
            User u;
            u.id = q.value(0).toLongLong();
            u.username = q.value(1).toString();
            QString roleStr = q.value(2).toString();
            u.role = (roleStr == "SuperAdmin") ? UserRole::SuperAdmin : UserRole::DeptAdmin;
            if (!q.value(3).isNull()) {
                u.departmentId = q.value(3).toInt();
                u.departmentName = q.value(4).toString();
            }
            list.push_back(u);
        }
    }
    return list;
}

bool DatabaseManager::updateUserDepartment(const User& caller, long long userId, int departmentId) {
    if (!caller.isSuperAdmin()) {
        m_lastError = "Access denied: Only Super Admin can modify department assignments.";
        return false;
    }

    QSqlQuery q(db);
    q.prepare("UPDATE users SET department_id = :d WHERE user_id = :uid AND role = 'DeptAdmin'");
    q.bindValue(":d", departmentId);
    q.bindValue(":uid", userId);

    if (!q.exec()) {
        m_lastError = q.lastError().text();
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

    QSqlQuery q(db);
    q.prepare("DELETE FROM users WHERE user_id = :uid");
    q.bindValue(":uid", userId);

    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    return true;
}

// ──────────────────────────────────────────────────────────
// Helper: Department permission check
// ──────────────────────────────────────────────────────────
bool DatabaseManager::checkDepartmentPermission(const User& caller, int complaintDepartmentId) {
    if (caller.isSuperAdmin()) return true;
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

    // Resolve department ID from department name
    auto deptOpt = getDepartmentByName(QString::fromStdString(c.department));
    int deptId = deptOpt.has_value() ? deptOpt->id : 6; // fallback General (id=6)

    QSqlQuery q(db);
    q.prepare("INSERT INTO complaints "
              "(citizen_name, citizen_contact, category, description, location, department_id, priority, status) "
              "VALUES (:citizen, :contact, :cat, :descr, :loc, :dept, :prio, :stat)");
    q.bindValue(":citizen", QString::fromStdString(c.citizen));
    q.bindValue(":contact", QString::fromStdString(c.contact));
    q.bindValue(":cat", QString::fromStdString(c.category));
    q.bindValue(":descr", QString::fromStdString(c.description));
    q.bindValue(":loc", QString::fromStdString(c.location));
    q.bindValue(":dept", deptId);
    q.bindValue(":prio", QString::fromStdString(priorityName(c.priority)));
    q.bindValue(":stat", QString::fromStdString(c.status.empty() ? "Registered" : c.status));

    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }

    outGeneratedId = q.lastInsertId().toInt();
    return true;
}

std::optional<Complaint> DatabaseManager::getComplaintById(const User& caller, int id) {
    if (!isConnected()) return std::nullopt;

    QSqlQuery q(db);
    q.prepare("SELECT c.complaint_id, c.citizen_name, c.citizen_contact, c.category, "
              "c.description, c.location, c.department_id, d.name AS dept_name, "
              "c.priority, c.status, c.created_at "
              "FROM complaints c "
              "JOIN departments d ON c.department_id = d.department_id "
              "WHERE c.complaint_id = :id");
    q.bindValue(":id", id);

    if (!q.exec() || !q.next()) {
        return std::nullopt;
    }

    int deptId = q.value("department_id").toInt();
    // Enforce role-based access
    if (!checkDepartmentPermission(caller, deptId)) {
        m_lastError = "Access denied: You are not authorized to view complaints outside your department.";
        return std::nullopt;
    }

    Complaint comp;
    comp.id = q.value("complaint_id").toInt();
    comp.citizen = q.value("citizen_name").toString().toStdString();
    comp.contact = q.value("citizen_contact").toString().toStdString();
    comp.category = q.value("category").toString().toStdString();
    comp.description = q.value("description").toString().toStdString();
    comp.location = q.value("location").toString().toStdString();
    comp.department = q.value("dept_name").toString().toStdString();
    comp.priority = priorityFromName(q.value("priority").toString().toStdString());
    comp.status = q.value("status").toString().toStdString();
    comp.createdAt = q.value("created_at").toDateTime().toString("yyyy-MM-dd HH:mm:ss").toStdString();

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

    // Backend authorization enforcement
    if (caller.isDeptAdmin()) {
        if (!caller.departmentId.has_value()) return list; // DeptAdmin with no dept has no access
        sql += " AND c.department_id = :deptId";
    }

    if (!statusFilter.isEmpty() && statusFilter != "All") {
        sql += " AND c.status = :status";
    }

    if (!searchKeyword.trimmed().isEmpty()) {
        sql += " AND (c.complaint_id LIKE :kw OR c.citizen_name LIKE :kw OR c.category LIKE :kw OR c.description LIKE :kw OR c.location LIKE :kw)";
    }

    sql += " ORDER BY c.complaint_id DESC";

    QSqlQuery q(db);
    q.prepare(sql);

    if (caller.isDeptAdmin()) {
        q.bindValue(":deptId", caller.departmentId.value());
    }
    if (!statusFilter.isEmpty() && statusFilter != "All") {
        q.bindValue(":status", statusFilter);
    }
    if (!searchKeyword.trimmed().isEmpty()) {
        q.bindValue(":kw", "%" + searchKeyword.trimmed() + "%");
    }

    if (q.exec()) {
        while (q.next()) {
            Complaint comp;
            comp.id = q.value("complaint_id").toInt();
            comp.citizen = q.value("citizen_name").toString().toStdString();
            comp.contact = q.value("citizen_contact").toString().toStdString();
            comp.category = q.value("category").toString().toStdString();
            comp.description = q.value("description").toString().toStdString();
            comp.location = q.value("location").toString().toStdString();
            comp.department = q.value("dept_name").toString().toStdString();
            comp.priority = priorityFromName(q.value("priority").toString().toStdString());
            comp.status = q.value("status").toString().toStdString();
            comp.createdAt = q.value("created_at").toDateTime().toString("yyyy-MM-dd HH:mm:ss").toStdString();
            list.push_back(comp);
        }
    }
    return list;
}

bool DatabaseManager::updateComplaintStatus(const User& caller,
                                            int complaintId,
                                            const QString& newStatus,
                                            const QString& notes)
{
    if (!isConnected()) return false;

    // First fetch current record to verify authorization and previous status
    QSqlQuery checkQ(db);
    checkQ.prepare("SELECT department_id, status FROM complaints WHERE complaint_id = :id");
    checkQ.bindValue(":id", complaintId);
    if (!checkQ.exec() || !checkQ.next()) {
        m_lastError = "Complaint ID not found.";
        return false;
    }

    int deptId = checkQ.value("department_id").toInt();
    QString prevStatus = checkQ.value("status").toString();

    // Verify department access
    if (!checkDepartmentPermission(caller, deptId)) {
        m_lastError = "Unauthorized: You cannot update complaints assigned to other departments.";
        return false;
    }

    // Begin transaction for updating status and inserting history
    db.transaction();

    QSqlQuery updateQ(db);
    updateQ.prepare("UPDATE complaints SET status = :status WHERE complaint_id = :id");
    updateQ.bindValue(":status", newStatus);
    updateQ.bindValue(":id", complaintId);

    if (!updateQ.exec()) {
        m_lastError = updateQ.lastError().text();
        db.rollback();
        return false;
    }

    // Insert audit history
    QSqlQuery histQ(db);
    histQ.prepare("INSERT INTO status_history (complaint_id, previous_status, new_status, changed_by, notes) "
                  "VALUES (:cid, :prev, :new, :uid, :notes)");
    histQ.bindValue(":cid", complaintId);
    histQ.bindValue(":prev", prevStatus);
    histQ.bindValue(":new", newStatus);
    histQ.bindValue(":uid", caller.id);
    histQ.bindValue(":notes", notes);

    if (!histQ.exec()) {
        m_lastError = histQ.lastError().text();
        db.rollback();
        return false;
    }

    db.commit();
    return true;
}

std::vector<StatusHistoryRecord> DatabaseManager::getComplaintHistory(const User& caller, int complaintId) {
    std::vector<StatusHistoryRecord> list;
    if (!isConnected()) return list;

    // Check permission first
    QSqlQuery checkQ(db);
    checkQ.prepare("SELECT department_id FROM complaints WHERE complaint_id = :id");
    checkQ.bindValue(":id", complaintId);
    if (!checkQ.exec() || !checkQ.next()) return list;

    int deptId = checkQ.value("department_id").toInt();
    if (!checkDepartmentPermission(caller, deptId)) return list;

    QSqlQuery q(db);
    q.prepare("SELECT h.history_id, h.complaint_id, h.previous_status, h.new_status, "
              "h.changed_by, u.username, h.notes, h.changed_at "
              "FROM status_history h "
              "LEFT JOIN users u ON h.changed_by = u.user_id "
              "WHERE h.complaint_id = :cid "
              "ORDER BY h.history_id ASC");
    q.bindValue(":cid", complaintId);

    if (q.exec()) {
        while (q.next()) {
            StatusHistoryRecord rec;
            rec.historyId = q.value(0).toLongLong();
            rec.complaintId = q.value(1).toLongLong();
            rec.previousStatus = q.value(2).toString();
            rec.newStatus = q.value(3).toString();
            rec.changedByUserId = q.value(4).toLongLong();
            rec.changedByUsername = q.value(5).toString();
            rec.notes = q.value(6).toString();
            rec.changedAt = q.value(7).toDateTime().toString("yyyy-MM-dd HH:mm:ss");
            list.push_back(rec);
        }
    }
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

    QSqlQuery q(db);
    // Total count
    if (q.exec("SELECT COUNT(*) FROM complaints" + baseWhere) && q.next()) {
        stats.totalComplaints = q.value(0).toInt();
    }

    // Pending count
    if (q.exec("SELECT COUNT(*) FROM complaints" + baseWhere + " AND status = 'Registered'") && q.next()) {
        stats.pendingComplaints = q.value(0).toInt();
    }

    // In Progress count
    if (q.exec("SELECT COUNT(*) FROM complaints" + baseWhere + " AND status IN ('In Progress', 'Urgent Processing')") && q.next()) {
        stats.inProgressComplaints = q.value(0).toInt();
    }

    // Resolved count
    if (q.exec("SELECT COUNT(*) FROM complaints" + baseWhere + " AND status IN ('Resolved', 'Closed')") && q.next()) {
        stats.resolvedComplaints = q.value(0).toInt();
    }

    // Urgent count
    if (q.exec("SELECT COUNT(*) FROM complaints" + baseWhere + " AND priority = 'High' AND status != 'Resolved' AND status != 'Closed'") && q.next()) {
        stats.urgentComplaints = q.value(0).toInt();
    }

    // Department breakdown
    if (caller.isSuperAdmin()) {
        if (q.exec("SELECT d.name, COUNT(c.complaint_id) "
                   "FROM departments d "
                   "LEFT JOIN complaints c ON d.department_id = c.department_id "
                   "GROUP BY d.department_id, d.name "
                   "ORDER BY d.department_id ASC"))
        {
            while (q.next()) {
                stats.departmentCounts.push_back({q.value(0).toString(), q.value(1).toInt()});
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

    QSqlQuery q(db);
    if (q.exec("SELECT c.complaint_id, c.citizen_name, c.citizen_contact, c.category, "
               "c.description, c.location, c.department_id, d.name AS dept_name, "
               "c.priority, c.status, c.created_at "
               "FROM complaints c "
               "JOIN departments d ON c.department_id = d.department_id "
               "ORDER BY c.complaint_id ASC"))
    {
        while (q.next()) {
            Complaint comp;
            comp.id = q.value("complaint_id").toInt();
            comp.citizen = q.value("citizen_name").toString().toStdString();
            comp.contact = q.value("citizen_contact").toString().toStdString();
            comp.category = q.value("category").toString().toStdString();
            comp.description = q.value("description").toString().toStdString();
            comp.location = q.value("location").toString().toStdString();
            comp.department = q.value("dept_name").toString().toStdString();
            comp.priority = priorityFromName(q.value("priority").toString().toStdString());
            comp.status = q.value("status").toString().toStdString();
            comp.createdAt = q.value("created_at").toDateTime().toString("yyyy-MM-dd HH:mm:ss").toStdString();
            list.push_back(comp);
        }
    }
    return list;
}
