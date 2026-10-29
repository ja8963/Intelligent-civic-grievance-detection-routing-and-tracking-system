#include <iostream>
#include <cassert>
#include "complaint.h"
#include "user.h"
#include "databasemanager.h"

int main() {
    std::cout << "[TEST] Starting Civic Grievance System DSA & RBAC Tests...\n";

    // 1. Test Password Hashing and Verification
    QString password = "MySecurePassword123!";
    QString hash = DatabaseManager::hashPassword(password);
    assert(!hash.isEmpty());
    assert(hash != password);
    assert(DatabaseManager::verifyPassword(password, hash));
    assert(!DatabaseManager::verifyPassword("WrongPassword", hash));
    std::cout << "  [PASS] Password hashing & verification passed.\n";

    // 2. Test Priority Queue (Max-Heap)
    PriorityQueue pq;
    Complaint c1{1, "Alice", "123", "Pothole", "Road", LOW, "Registered", "Desc", "Loc", ""};
    Complaint c2{2, "Bob", "456", "Garbage", "Sanitation", HIGH, "Registered", "Desc", "Loc", ""};
    Complaint c3{3, "Charlie", "789", "Street Light", "Electrical", MEDIUM, "Registered", "Desc", "Loc", ""};

    pq.push(&c1);
    pq.push(&c2);
    pq.push(&c3);

    Complaint* top = nullptr;
    assert(pq.pop(top));
    assert(top->id == 2); // HIGH priority should come out first
    assert(pq.pop(top));
    assert(top->id == 3); // MEDIUM priority second
    assert(pq.pop(top));
    assert(top->id == 1); // LOW priority last
    std::cout << "  [PASS] Max-Heap PriorityQueue correctly prioritizes emergencies.\n";

    // 3. Test FIFO Queue
    ComplaintQueue queue;
    queue.enqueue(&c1);
    queue.enqueue(&c3);
    assert(queue.size() == 2);
    Complaint* front = nullptr;
    assert(queue.dequeue(front));
    assert(front->id == 1); // FIFO order
    assert(queue.dequeue(front));
    assert(front->id == 3);
    assert(queue.empty());
    std::cout << "  [PASS] FIFO ComplaintQueue correctly operates in arrival order.\n";

    // 4. Test Hash Table
    HashTable ht;
    ht.insert(c1.id, &c1);
    ht.insert(c2.id, &c2);
    assert(ht.search(1) == &c1);
    assert(ht.search(2) == &c2);
    assert(ht.search(999) == nullptr);
    std::cout << "  [PASS] HashTable O(1) search and retrieval passed.\n";

    // 5. Test Department Routing Logic
    assert(departmentFor("Pothole") == "Road");
    assert(departmentFor("Garbage") == "Sanitation");
    assert(departmentFor("Street Light") == "Electrical");
    assert(departmentFor("Water Leakage") == "Water");
    assert(departmentFor("Drainage") == "Drainage");
    assert(departmentFor("Unknown") == "General");
    std::cout << "  [PASS] Automated category-to-department routing passed.\n";

    // 6. Test User Roles & Department Access Model
    User superAdmin{1, "admin", UserRole::SuperAdmin, std::nullopt, ""};
    assert(superAdmin.isSuperAdmin());
    assert(!superAdmin.isDeptAdmin());

    User roadAdmin{2, "road_admin", UserRole::DeptAdmin, 1, "Road"};
    assert(roadAdmin.isDeptAdmin());
    assert(!roadAdmin.isSuperAdmin());
    assert(roadAdmin.departmentId.value() == 1);
    std::cout << "  [PASS] RBAC User model tests passed.\n";

    std::cout << "All automated verification tests passed successfully!\n";
    return 0;
}
