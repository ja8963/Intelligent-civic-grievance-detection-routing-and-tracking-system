#pragma once

#include <QString>
#include <optional>

// Role-based access control roles
enum class UserRole {
    SuperAdmin,
    DeptAdmin,
    Citizen
};

// Represents an authenticated user in the system
struct User {
    long long id = 0;
    QString username;
    UserRole role = UserRole::DeptAdmin;
    std::optional<int> departmentId; // NULL for SuperAdmin & Citizen, set for DeptAdmin
    QString departmentName;          // Human-readable department name

    bool isSuperAdmin() const { return role == UserRole::SuperAdmin; }
    bool isDeptAdmin() const { return role == UserRole::DeptAdmin; }
    bool isCitizen() const { return role == UserRole::Citizen; }

    QString roleName() const {
        if (isSuperAdmin()) return "Super Admin";
        if (isDeptAdmin()) return "Department Admin";
        return "Citizen (Public)";
    }
};

// Structure representing a department in the database
struct DepartmentRecord {
    int id = 0;
    QString name;
};

// Structure representing an entry in status_history
struct StatusHistoryRecord {
    long long historyId = 0;
    long long complaintId = 0;
    QString previousStatus;
    QString newStatus;
    long long changedByUserId = 0;
    QString changedByUsername;
    QString notes;
    QString changedAt;
};
