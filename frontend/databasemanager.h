#pragma once

#include <QString>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <vector>
#include <optional>
#include <memory>
#include "user.h"
#include "complaint.h"

// Statistics summary structure
struct ComplaintStatistics {
    int totalComplaints = 0;
    int pendingComplaints = 0;
    int inProgressComplaints = 0;
    int resolvedComplaints = 0;
    int urgentComplaints = 0;

    // Breakdown per department (Department Name -> Count)
    std::vector<std::pair<QString, int>> departmentCounts;
};

// DatabaseManager handles all MySQL 8.0 interactions using Qt SQL and prepared statements.
// It securely hashes passwords and enforces authorization at the database query level.
class DatabaseManager {
public:
    static DatabaseManager& instance();

    // Database connection
    bool connectToDatabase(const QString& host = "127.0.0.1",
                           int port = 3306,
                           const QString& dbName = "civic_grievance",
                           const QString& user = "root",
                           const QString& password = "");
    bool isConnected() const;
    QString lastError() const;
    void close();

    // Secure Password Hashing & Verification (PBKDF2-HMAC-SHA256 with cryptographically random salt)
    static QString hashPassword(const QString& plainPassword);
    static bool verifyPassword(const QString& plainPassword, const QString& storedHash);

    // Initial setup helper: ensures at least one SuperAdmin account exists
    bool ensureInitialSuperAdmin(const QString& username, const QString& password);
    bool hasAnySuperAdmin();

    // Authentication
    std::optional<User> authenticateUser(const QString& username, const QString& password);

    // Department queries
    std::vector<DepartmentRecord> getDepartments();
    std::optional<DepartmentRecord> getDepartmentById(int departmentId);
    std::optional<DepartmentRecord> getDepartmentByName(const QString& name);

    // User management (Strictly enforced: caller must be SuperAdmin)
    bool createUser(const User& caller,
                    const QString& username,
                    const QString& password,
                    UserRole role,
                    std::optional<int> departmentId);
    std::vector<User> getAllUsers(const User& caller);
    bool updateUserDepartment(const User& caller, long long userId, int departmentId);
    bool deleteUser(const User& caller, long long userId);

    // Complaint management (Permissions enforced based on caller role)
    bool registerComplaint(const Complaint& c, int& outGeneratedId);
    std::optional<Complaint> getComplaintById(const User& caller, int id);
    std::vector<Complaint> getComplaints(const User& caller,
                                        const QString& statusFilter = "",
                                        const QString& searchKeyword = "");
    bool updateComplaintStatus(const User& caller,
                               int complaintId,
                               const QString& newStatus,
                               const QString& notes = "");
    std::vector<StatusHistoryRecord> getComplaintHistory(const User& caller, int complaintId);

    // Statistics calculation (Role-filtered)
    ComplaintStatistics getStatistics(const User& caller);

    // Load complaints for in-memory DSA initialization
    std::vector<Complaint> loadAllComplaintsForSystem();

private:
    DatabaseManager();
    ~DatabaseManager();
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    QSqlDatabase db;
    QString m_lastError;

    // Helper to verify caller has access to a specific complaint's department
    bool checkDepartmentPermission(const User& caller, int complaintDepartmentId);
};
