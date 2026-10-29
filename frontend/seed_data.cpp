#include <iostream>
#include <QCoreApplication>
#include "databasemanager.h"

int main(int argc, char* argv[]) {
    QCoreApplication a(argc, argv);
    std::cout << "[SEED] Connecting to MySQL 8.0...\n";

    if (!DatabaseManager::instance().connectToDatabase("127.0.0.1", 3306, "civic_grievance", "root", "12345678")) {
        std::cerr << "[ERROR] Database connection failed: " << DatabaseManager::instance().lastError().toStdString() << "\n";
        return 1;
    }

    std::cout << "[SEED] Connected! Checking existing users...\n";

    // 1. Create Super Admin if not exists
    if (!DatabaseManager::instance().hasAnySuperAdmin()) {
        bool ok = DatabaseManager::instance().ensureInitialSuperAdmin("superadmin", "Admin@123");
        std::cout << "  [+] Created Super Admin: superadmin / Admin@123 -> " << (ok ? "SUCCESS" : "FAILED") << "\n";
    }

    auto superUserOpt = DatabaseManager::instance().authenticateUser("superadmin", "Admin@123");
    if (!superUserOpt.has_value()) {
        std::cerr << "[ERROR] Could not authenticate superadmin for provisioning.\n";
        return 1;
    }
    User superAdmin = superUserOpt.value();

    // 2. Provision Department Admins
    struct DeptAdminSeed {
        QString username;
        QString password;
        int deptId;
        QString deptName;
    };

    std::vector<DeptAdminSeed> seeds = {
        {"road_admin",   "Road@123",       1, "Road"},
        {"san_admin",    "Sanitation@123", 2, "Sanitation"},
        {"elec_admin",   "Elec@123",       3, "Electrical"},
        {"water_admin",  "Water@123",      4, "Water"},
        {"drain_admin",  "Drain@123",      5, "Drainage"},
        {"gen_admin",    "General@123",    6, "General"}
    };

    for (const auto& s : seeds) {
        bool ok = DatabaseManager::instance().createUser(superAdmin, s.username, s.password, UserRole::DeptAdmin, s.deptId);
        std::cout << "  [+] Created Dept Admin: " << s.username.toStdString() << " (" << s.deptName.toStdString() << ") -> " << (ok ? "SUCCESS" : "ALREADY EXISTS") << "\n";
    }

    // 3. Seed Sample Complaints across departments
    struct ComplaintSeed {
        string citizen;
        string contact;
        string category;
        string location;
        string desc;
        Priority priority;
        string status;
    };

    std::vector<ComplaintSeed> sampleComplaints = {
        {"Rajesh Sharma",  "9876543210", "Pothole",       "MG Road, Junction 4",        "Deep hazardous pothole causing traffic slowdowns and scooter skids.", HIGH, "Registered"},
        {"Priya Patel",    "9812345678", "Garbage",       "Sector 12 Community Market", "Overflowing municipal waste bins not cleared for 3 days.", MEDIUM, "Registered"},
        {"Amit Verma",     "9765432109", "Street Light",  "Park Street, Pole #42",      "Non-functional LED street light creating safety risk at night.", LOW, "In Progress"},
        {"Sunita Rao",     "9654321098", "Water Leakage", "Civil Lines, Near Tank #3",  "Major pipeline rupture with clean water flowing onto the road.", HIGH, "Urgent Processing"},
        {"Vikram Malhotra","9543210987", "Drainage",      "Railway Colony Lane 2",      "Clogged storm drainage overflowing during rain.", MEDIUM, "Registered"},
        {"Anjali Gupta",   "9432109876", "Pothole",       "Station Road Crossroad",     "Small pothole near pedestrian zebra crossing.", LOW, "Resolved"}
    };

    for (const auto& sc : sampleComplaints) {
        Complaint c;
        c.citizen = sc.citizen;
        c.contact = sc.contact;
        c.category = sc.category;
        c.location = sc.location;
        c.description = sc.desc;
        c.priority = sc.priority;
        c.department = departmentFor(sc.category);
        c.status = sc.status;

        int cid = 0;
        if (DatabaseManager::instance().registerComplaint(c, cid)) {
            std::cout << "  [+] Seeded Complaint #" << cid << " [" << c.category << " -> " << c.department << "]\n";
            if (sc.status != "Registered") {
                DatabaseManager::instance().updateComplaintStatus(superAdmin, cid, QString::fromStdString(sc.status), "Initial system setup status update");
            }
        }
    }

    std::cout << "\n[SEED] Data initialization complete! All accounts and complaints ready.\n";
    return 0;
}
