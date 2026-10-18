#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
using namespace std;

enum Priority { LOW = 1, MEDIUM = 2, HIGH = 3 };

string priorityName(Priority p) {
    if (p == HIGH) return "HIGH";
    if (p == MEDIUM) return "MEDIUM";
    return "LOW";
}

struct Complaint {
    int id;
    string citizen;
    string category;
    string department;
    Priority priority;
    string status;
};

// -------------------- Linked List --------------------
struct ComplaintNode {
    Complaint data;
    ComplaintNode* next;
    ComplaintNode(const Complaint& c) : data(c), next(nullptr) {}
};

class ComplaintList {
    ComplaintNode* head = nullptr;

public:
    void add(const Complaint& c) {
        ComplaintNode* node = new ComplaintNode(c);
        if (!head) {
            head = node;
            return;
        }
        ComplaintNode* temp = head;
        while (temp->next) temp = temp->next;
        temp->next = node;
    }

    Complaint* find(int id) {
        ComplaintNode* temp = head;
        while (temp) {
            if (temp->data.id == id) return &temp->data;
            temp = temp->next;
        }
        return nullptr;
    }

    void display() const {
        if (!head) {
            cout << "No complaints registered.\n";
            return;
        }
        cout << "\n--- Complaint List (Linked List Traversal) ---\n";
        ComplaintNode* temp = head;
        while (temp) {
            cout << "ID: " << temp->data.id
                 << " | Citizen: " << temp->data.citizen
                 << " | Category: " << temp->data.category
                 << " | Department: " << temp->data.department
                 << " | Priority: " << priorityName(temp->data.priority)
                 << " | Status: " << temp->data.status << '\n';
            temp = temp->next;
        }
    }

    ~ComplaintList() {
        while (head) {
            ComplaintNode* next = head->next;
            delete head;
            head = next;
        }
    }
};

// -------------------- Hash Table --------------------
class HashTable {
    static const int SIZE = 17;
    struct Entry {
        int id = -1;
        Complaint* complaint = nullptr;
    } table[SIZE];

    int hash(int id) const { return id % SIZE; }

public:
    void insert(Complaint* c) {
        int index = hash(c->id);
        for (int i = 0; i < SIZE; ++i) {
            int pos = (index + i) % SIZE;
            if (table[pos].id == -1 || table[pos].id == c->id) {
                table[pos].id = c->id;
                table[pos].complaint = c;
                return;
            }
        }
        cout << "Hash table is full.\n";
    }

    Complaint* search(int id) const {
        int index = hash(id);
        for (int i = 0; i < SIZE; ++i) {
            int pos = (index + i) % SIZE;
            if (table[pos].id == -1) return nullptr;
            if (table[pos].id == id) return table[pos].complaint;
        }
        return nullptr;
    }
};

// -------------------- FIFO Queue --------------------
struct QueueNode {
    Complaint* complaint;
    QueueNode* next;
    QueueNode(Complaint* c) : complaint(c), next(nullptr) {}
};

class ComplaintQueue {
    QueueNode* frontNode = nullptr;
    QueueNode* rearNode = nullptr;

public:
    void enqueue(Complaint* c) {
        QueueNode* node = new QueueNode(c);
        if (!rearNode) {
            frontNode = rearNode = node;
            return;
        }
        rearNode->next = node;
        rearNode = node;
    }

    Complaint* dequeue() {
        if (!frontNode) return nullptr;
        QueueNode* temp = frontNode;
        Complaint* c = temp->complaint;
        frontNode = frontNode->next;
        if (!frontNode) rearNode = nullptr;
        delete temp;
        return c;
    }

    bool empty() const { return frontNode == nullptr; }

    void display(const string& department) const {
        cout << "\n" << department << " FIFO Queue: ";
        if (!frontNode) {
            cout << "EMPTY\n";
            return;
        }
        QueueNode* temp = frontNode;
        while (temp) {
            cout << temp->complaint->id;
            if (temp->next) cout << " -> ";
            temp = temp->next;
        }
        cout << "\n";
    }

    ~ComplaintQueue() {
        while (frontNode) {
            QueueNode* next = frontNode->next;
            delete frontNode;
            frontNode = next;
        }
    }
};

// -------------------- Priority Queue / Max Heap --------------------
class PriorityQueue {
    vector<Complaint*> heap;

    bool higher(Complaint* a, Complaint* b) {
        if (a->priority != b->priority)
            return a->priority > b->priority;
        return a->id < b->id;
    }

    void heapifyUp(int i) {
        while (i > 0) {
            int parent = (i - 1) / 2;
            if (!higher(heap[i], heap[parent])) break;
            swap(heap[i], heap[parent]);
            i = parent;
        }
    }

    void heapifyDown(int i) {
        int n = heap.size();
        while (true) {
            int left = 2 * i + 1;
            int right = 2 * i + 2;
            int best = i;
            if (left < n && higher(heap[left], heap[best])) best = left;
            if (right < n && higher(heap[right], heap[best])) best = right;
            if (best == i) break;
            swap(heap[i], heap[best]);
            i = best;
        }
    }

public:
    void push(Complaint* c) {
        heap.push_back(c);
        heapifyUp((int)heap.size() - 1);
    }

    Complaint* pop() {
        if (heap.empty()) return nullptr;
        Complaint* top = heap[0];
        heap[0] = heap.back();
        heap.pop_back();
        if (!heap.empty()) heapifyDown(0);
        return top;
    }

    bool empty() const { return heap.empty(); }

    void display() const {
        cout << "\nPriority Queue / Heap: ";
        if (heap.empty()) {
            cout << "EMPTY\n";
            return;
        }
        for (Complaint* c : heap)
            cout << "[ID " << c->id << ", " << priorityName(c->priority) << "] ";
        cout << "\n";
    }
};

string departmentFor(const string& category) {
    if (category == "Pothole") return "Road";
    if (category == "Garbage") return "Sanitation";
    if (category == "Street Light") return "Electrical";
    if (category == "Water Leakage") return "Water";
    if (category == "Drainage") return "Drainage";
    return "General";
}

class CivicSystem {
    ComplaintList complaints;
    HashTable index;
    ComplaintQueue road, sanitation, electrical, water, drainage, general;
    PriorityQueue urgent;
    int nextId = 1001;

    ComplaintQueue& getQueue(const string& dept) {
        if (dept == "Road") return road;
        if (dept == "Sanitation") return sanitation;
        if (dept == "Electrical") return electrical;
        if (dept == "Water") return water;
        if (dept == "Drainage") return drainage;
        return general;
    }

public:
    void registerComplaint() {
        Complaint c;
        c.id = nextId++;
        cin.ignore();
        cout << "Citizen name: "; getline(cin, c.citizen);
        cout << "Category (Pothole/Garbage/Street Light/Water Leakage/Drainage): ";
        getline(cin, c.category);
        cout << "Priority (1=LOW, 2=MEDIUM, 3=HIGH): ";
        int p; cin >> p;
        if (p < 1 || p > 3) p = 1;
        c.priority = (Priority)p;
        c.department = departmentFor(c.category);
        c.status = "Registered";

        complaints.add(c);
        Complaint* stored = complaints.find(c.id);
        index.insert(stored);
        getQueue(c.department).enqueue(stored);
        if (c.priority == HIGH) urgent.push(stored);

        cout << "\nComplaint registered successfully.\n";
        cout << "Complaint ID: " << c.id << '\n';
        cout << "Department: " << c.department << '\n';
        cout << "Status: " << c.status << '\n';
    }

    void searchComplaint() {
        int id;
        cout << "Enter complaint ID: ";
        cin >> id;
        Complaint* c = index.search(id);
        if (!c) {
            cout << "Complaint not found.\n";
            return;
        }
        cout << "\n--- Hash Table Lookup Result ---\n";
        cout << "ID: " << c->id << '\n'
             << "Citizen: " << c->citizen << '\n'
             << "Category: " << c->category << '\n'
             << "Department: " << c->department << '\n'
             << "Priority: " << priorityName(c->priority) << '\n'
             << "Status: " << c->status << '\n';
    }

    void processNext(const string& dept) {
        Complaint* c = getQueue(dept).dequeue();
        if (!c) {
            cout << "No complaints waiting in " << dept << " department.\n";
            return;
        }
        c->status = "In Progress";
        cout << "Processed complaint " << c->id << " from " << dept << " FIFO queue.\n";
    }

    void processUrgent() {
        Complaint* c = urgent.pop();
        if (!c) {
            cout << "No urgent complaints.\n";
            return;
        }
        c->status = "Urgent Processing";
        cout << "Urgent complaint processed: ID " << c->id
             << " (" << priorityName(c->priority) << ")\n";
    }

    void display() const { complaints.display(); }

    void displayQueues() const {
        road.display("Road");
        sanitation.display("Sanitation");
        electrical.display("Electrical");
        water.display("Water");
        drainage.display("Drainage");
        general.display("General");
        urgent.display();
    }

    void updateStatus() {
        int id;
        cout << "Enter complaint ID: ";
        cin >> id;
        Complaint* c = index.search(id);
        if (!c) {
            cout << "Complaint not found.\n";
            return;
        }
        cin.ignore();
        cout << "New status: ";
        getline(cin, c->status);
        cout << "Status updated successfully.\n";
    }
};

void menu() {
    cout << "\n========== CIVIC GRIEVANCE MANAGEMENT SYSTEM ==========" << '\n';
    cout << "1. Register Complaint\n";
    cout << "2. Display All Complaints (Linked List)\n";
    cout << "3. Search Complaint (Hash Table)\n";
    cout << "4. Process Department Complaint (FIFO Queue)\n";
    cout << "5. Process Urgent Complaint (Priority Queue / Heap)\n";
    cout << "6. Update Complaint Status\n";
    cout << "7. Display Queues / Heap\n";
    cout << "0. Exit\n";
    cout << "Choose: ";
}

int main() {
    CivicSystem system;
    int choice;

    do {
        menu();
        cin >> choice;
        switch (choice) {
            case 1: system.registerComplaint(); break;
            case 2: system.display(); break;
            case 3: system.searchComplaint(); break;
            case 4: {
                cin.ignore();
                string dept;
                cout << "Department (Road/Sanitation/Electrical/Water/Drainage/General): ";
                getline(cin, dept);
                system.processNext(dept);
                break;
            }
            case 5: system.processUrgent(); break;
            case 6: system.updateStatus(); break;
            case 7: system.displayQueues(); break;
            case 0: cout << "Exiting...\n"; break;
            default: cout << "Invalid choice.\n";
        }
    } while (choice != 0);

    return 0;
}
