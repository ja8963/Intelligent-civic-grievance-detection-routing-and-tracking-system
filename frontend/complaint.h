#pragma once

#include <string>
#include <vector>

using std::string;
using std::vector;

// Priority levels for complaints
enum Priority { LOW = 1, MEDIUM = 2, HIGH = 3 };

// Convert priority enum to human-readable string
inline string priorityName(Priority p) {
    if (p == HIGH)   return "High";
    if (p == MEDIUM) return "Medium";
    return "Low";
}

inline Priority priorityFromName(const string& name) {
    if (name == "High") return HIGH;
    if (name == "Medium") return MEDIUM;
    return LOW;
}

// A single civic complaint record
struct Complaint {
    int      id = 0;
    string   citizen;
    string   contact;      // Citizen contact details (phone/email)
    string   category;
    string   department;
    Priority priority = LOW;
    string   status;
    string   description;
    string   location;
    string   createdAt;    // Timestamp
};

// ──────────────────────────────────────────
// Singly-linked list node
// ──────────────────────────────────────────
struct ComplaintNode {
    Complaint       data;
    ComplaintNode*  next;
    explicit ComplaintNode(const Complaint& c) : data(c), next(nullptr) {}
};

// ──────────────────────────────────────────
// Singly-linked list that stores all complaints
// ──────────────────────────────────────────
class ComplaintList {
public:
    ComplaintList() : head(nullptr) {}
    ~ComplaintList();

    void add(const Complaint& c);
    Complaint* find(int id);                  // returns pointer into the list or nullptr
    vector<Complaint*> getAllComplaints();     // traverse and return all pointers
    void clear();

private:
    ComplaintNode* head;
};

// ──────────────────────────────────────────
// Hash table with linear probing (SIZE = 1009 for robust capacity)
// Maps complaint-id -> pointer into the linked list for O(1) lookup
// ──────────────────────────────────────────
class HashTable {
    static const int SIZE = 1009;
    struct Entry {
        int key;          // complaint id, -1 means empty
        Complaint* value; // pointer into the linked list
        Entry() : key(-1), value(nullptr) {}
    };
    Entry table[SIZE];

    int hash(int key) const { return (key % SIZE + SIZE) % SIZE; }

public:
    HashTable();
    void     insert(int id, Complaint* ptr);
    Complaint* search(int id) const;
    void     clear();
};

// ──────────────────────────────────────────
// FIFO queue node
// ──────────────────────────────────────────
struct QueueNode {
    Complaint*  data;
    QueueNode*  next;
    explicit QueueNode(Complaint* c) : data(c), next(nullptr) {}
};

// ──────────────────────────────────────────
// FIFO complaint queue (one per department)
// ──────────────────────────────────────────
class ComplaintQueue {
public:
    ComplaintQueue() : front(nullptr), rear(nullptr), count(0) {}
    ~ComplaintQueue();

    void enqueue(Complaint* c);
    bool dequeue(Complaint*& out);   // out gets the front complaint; returns false if empty
    bool empty() const  { return count == 0; }
    int  size()  const  { return count; }
    void clear();

    // Returns a formatted string listing all items currently in the queue
    string contentsAsString() const;

private:
    QueueNode* front;
    QueueNode* rear;
    int        count;
};

// ──────────────────────────────────────────
// Max-heap priority queue (sorted by Priority enum value)
// ──────────────────────────────────────────
class PriorityQueue {
public:
    bool empty()  const { return heap.empty(); }
    int  size()   const { return (int)heap.size(); }

    void push(Complaint* c);
    bool pop(Complaint*& out);   // removes and returns the highest-priority complaint
    Complaint* peekTop() const;  // view without removing
    void clear() { heap.clear(); }

private:
    vector<Complaint*> heap;

    void heapifyUp(int idx);
    void heapifyDown(int idx);
};

// ──────────────────────────────────────────
// Helper: decide which department handles a category
// ──────────────────────────────────────────
string departmentFor(const string& category);

// ──────────────────────────────────────────
// Top-level system object used by the GUI & cached in memory
// ──────────────────────────────────────────
class CivicSystem {
public:
    CivicSystem() : nextId(1001) {}

    // Register a new complaint; returns true and fills *outId with the new id
    bool registerComplaint(const string& citizen,
                           const string& contact,
                           const string& category,
                           const string& description,
                           const string& location,
                           int            priority,
                           int*           outId = nullptr,
                           string*        outDept = nullptr);

    // Overload for backward compatibility
    bool registerComplaint(const string& citizen,
                           const string& category,
                           const string& description,
                           const string& location,
                           int            priority,
                           int*           outId = nullptr,
                           string*        outDept = nullptr) {
        return registerComplaint(citizen, "", category, description, location, priority, outId, outDept);
    }

    // Populate system from loaded database records
    void loadFromRecords(const vector<Complaint>& records);

    // Search by complaint ID; returns nullptr if not found
    Complaint* searchById(int id);

    // Change the status string of a complaint; returns false if id not found
    bool updateStatus(int id, const string& newStatus);

    // Dequeue the next complaint from a specific department's FIFO queue
    bool processNextFromDept(const string& dept, Complaint& outComplaint);

    // Pop the top complaint from the priority (max-heap) queue
    bool processUrgent(Complaint& outComplaint);

    // Return all complaints in insertion order
    vector<Complaint*> getAllComplaints();

    int getTotalCount();
    int getPendingCount();
    int getResolvedCount();

    // Returns a human-readable summary of what is currently in a dept queue
    string getQueueInfo(const string& dept);

    void clearAll();

private:
    ComplaintList  complaints;   // master linked list
    HashTable      index;        // id -> Complaint* for O(1) lookup
    ComplaintQueue road;
    ComplaintQueue sanitation;
    ComplaintQueue electrical;
    ComplaintQueue water;
    ComplaintQueue drainage;
    ComplaintQueue general;
    PriorityQueue  urgent;       // HIGH-priority complaints go here too
    int            nextId;

    ComplaintQueue& getQueue(const string& dept);
};
