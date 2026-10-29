#include <iostream>
#include <cassert>
#include <QCoreApplication>
#include "databasemanager.h"
#include "complaint.h"
#include "user.h"

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "[-] FAILED: " << msg << " (" << __FILE__ << ":" << __LINE__ << ")\n"; \
            return 1; \
        } else { \
            std::cout << "[+] PASSED: " << msg << "\n"; \
        } \
    } while (0)

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    std::cout << "========================================================\n";
    std::cout << "  RUNNING COMPREHENSIVE END-TO-END TEST SUITE\n";
    std::cout << "========================================================\n\n";

    // ── 1. Database Connection ──
    bool connected = DatabaseManager::instance().connectToDatabase("127.0.0.1", 3306, "civic_grievance", "root", "12345678");
    TEST_ASSERT(connected, "Database connection established to MySQL 8.0");

    // ── 2. Authentication Tests ──
    std::cout << "\n[2. Authentication & Credential Verification]\n";
    auto superOpt = DatabaseManager::instance().authenticateUser("superadmin", "Admin@123");
    TEST_ASSERT(superOpt.has_value(), "Super Admin login with valid credentials");
    User superUser = superOpt.value();
    TEST_ASSERT(superUser.isSuperAdmin(), "User object correctly identified as SuperAdmin");
    TEST_ASSERT(!superUser.departmentId.has_value(), "SuperAdmin has NULL department ID");

    auto roadOpt = DatabaseManager::instance().authenticateUser("road_admin", "Road@123");
    TEST_ASSERT(roadOpt.has_value(), "Road Dept Admin login with valid credentials");
    User roadUser = roadOpt.value();
    TEST_ASSERT(roadUser.isDeptAdmin(), "User object correctly identified as DeptAdmin");
    TEST_ASSERT(roadUser.departmentId.has_value() && roadUser.departmentId.value() == 1, "Road Admin assigned to Department 1");

    auto invalidPass = DatabaseManager::instance().authenticateUser("superadmin", "WrongPassword!99");
    TEST_ASSERT(!invalidPass.has_value(), "Login fails with invalid password");

    auto invalidUser = DatabaseManager::instance().authenticateUser("nonexistent_user", "AnyPassword");
    TEST_ASSERT(!invalidUser.has_value(), "Login fails with nonexistent username");

    // ── 3. Role-Based Access Control (RBAC) Data Isolation Tests ──
    std::cout << "\n[3. RBAC Backend & Query Isolation Enforcement]\n";
    auto allComplaints = DatabaseManager::instance().getComplaints(superUser);
    TEST_ASSERT(!allComplaints.empty(), "Super Admin can query all complaints across departments");

    auto roadComplaints = DatabaseManager::instance().getComplaints(roadUser);
    for (const auto& c : roadComplaints) {
        TEST_ASSERT(c.department == "Road", "DeptAdmin query results are strictly isolated to their department");
    }

    // Identify a non-Road complaint to test unauthorized access
    int foreignComplaintId = -1;
    for (const auto& c : allComplaints) {
        if (c.department != "Road") {
            foreignComplaintId = c.id;
            break;
        }
    }
    TEST_ASSERT(foreignComplaintId != -1, "Found complaint from another department for security test");

    // Attempt direct lookup of foreign complaint by Dept Admin
    auto foreignLookup = DatabaseManager::instance().getComplaintById(roadUser, foreignComplaintId);
    TEST_ASSERT(!foreignLookup.has_value(), "DeptAdmin cannot view details of foreign department complaint");

    // Attempt unauthorized status update of foreign complaint by Dept Admin
    bool illegalUpdate = DatabaseManager::instance().updateComplaintStatus(roadUser, foreignComplaintId, "Resolved", "Hacked attempt");
    TEST_ASSERT(!illegalUpdate, "DeptAdmin is blocked from updating complaints belonging to other departments");

    // ── 4. Complaint Registration, Auto-Routing & Persistence ──
    std::cout << "\n[4. Complaint Registration & Auto-Routing]\n";
    Complaint newComp;
    newComp.citizen = "Automated Test Citizen";
    newComp.contact = "555-0199";
    newComp.category = "Drainage";
    newComp.location = "Test Lane 99, Block B";
    newComp.description = "Automated verification drainage overflow";
    newComp.priority = HIGH;
    newComp.department = departmentFor(newComp.category);
    TEST_ASSERT(newComp.department == "Drainage", "Category 'Drainage' auto-routed to 'Drainage' department");

    int generatedId = 0;
    bool regOk = DatabaseManager::instance().registerComplaint(newComp, generatedId);
    TEST_ASSERT(regOk && generatedId > 0, "New complaint registered in MySQL with auto-generated ID");

    // Verify persisted complaint details
    auto fetchedComp = DatabaseManager::instance().getComplaintById(superUser, generatedId);
    TEST_ASSERT(fetchedComp.has_value(), "Persisted complaint retrievable from MySQL");
    TEST_ASSERT(fetchedComp->citizen == newComp.citizen, "Persisted citizen name matches");
    TEST_ASSERT(fetchedComp->priority == HIGH, "Persisted priority matches");

    // ── 5. Status Transition & Audit Trail Verification ──
    std::cout << "\n[5. Status Updates & Audit History Logging]\n";
    bool updateOk = DatabaseManager::instance().updateComplaintStatus(superUser, generatedId, "In Progress", "Work crew dispatched to site");
    TEST_ASSERT(updateOk, "Status updated to 'In Progress'");

    auto history = DatabaseManager::instance().getComplaintHistory(superUser, generatedId);
    TEST_ASSERT(!history.empty(), "Status change audit history recorded in status_history table");
    const auto& lastAudit = history.back();
    TEST_ASSERT(lastAudit.newStatus == "In Progress", "Audit record reflects new status");
    TEST_ASSERT(lastAudit.notes == "Work crew dispatched to site", "Audit record preserves operator notes");
    TEST_ASSERT(lastAudit.changedByUsername == superUser.username, "Audit record logs operator username");

    // ── 6. Super Admin Account & Department Management ──
    std::cout << "\n[6. User & Department Management RBAC]\n";
    QString testUser = "test_sub_admin_" + QString::number(generatedId);
    bool createByDeptAdmin = DatabaseManager::instance().createUser(roadUser, testUser, "Pass@123", UserRole::DeptAdmin, 2);
    TEST_ASSERT(!createByDeptAdmin, "DeptAdmin is strictly rejected from creating user accounts");

    bool createBySuperAdmin = DatabaseManager::instance().createUser(superUser, testUser, "Pass@123", UserRole::DeptAdmin, 2);
    TEST_ASSERT(createBySuperAdmin, "Super Admin successfully creates new Department Admin account");

    auto createdUserOpt = DatabaseManager::instance().authenticateUser(testUser, "Pass@123");
    TEST_ASSERT(createdUserOpt.has_value(), "Newly created DeptAdmin can successfully authenticate");
    TEST_ASSERT(createdUserOpt->departmentId.value() == 2, "Assigned department ID is correct");

    // Reassign department
    bool reassignOk = DatabaseManager::instance().updateUserDepartment(superUser, createdUserOpt->id, 5); // Reassign to Drainage (id=5)
    TEST_ASSERT(reassignOk, "Super Admin successfully reassigns department");
    auto updatedUserOpt = DatabaseManager::instance().authenticateUser(testUser, "Pass@123");
    TEST_ASSERT(updatedUserOpt->departmentId.value() == 5, "Reassigned department verified in database");

    // Clean up test user
    bool deleteOk = DatabaseManager::instance().deleteUser(superUser, createdUserOpt->id);
    TEST_ASSERT(deleteOk, "Super Admin successfully deletes test user");

    // ── 7. Data Structures & Algorithms (DSA) In-Memory Sync ──
    std::cout << "\n[7. In-Memory Data Structures (List, Hash Table, Queues, Max-Heap)]\n";
    CivicSystem system;
    auto allRecs = DatabaseManager::instance().loadAllComplaintsForSystem();
    system.loadFromRecords(allRecs);
    TEST_ASSERT(system.getTotalCount() == (int)allRecs.size(), "In-memory Linked List loaded all database records");

    Complaint* cachedComp = system.searchById(generatedId);
    TEST_ASSERT(cachedComp != nullptr && cachedComp->id == generatedId, "O(1) Hash Table lookup locates complaint by ID");

    Complaint urgentItem;
    bool urgentOk = system.processUrgent(urgentItem);
    TEST_ASSERT(urgentOk && urgentItem.priority == HIGH, "Max-Heap Priority Queue popped high-priority emergency");

    std::cout << "\n========================================================\n";
    std::cout << "  ALL TESTS COMPLETED SUCCESSFULLY! ZERO ERRORS FOUND.\n";
    std::cout << "========================================================\n";
    return 0;
}
