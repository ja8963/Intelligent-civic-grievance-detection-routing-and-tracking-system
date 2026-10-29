#include "complaint.h"
#include <stdexcept>
#include <sstream>
#include <algorithm>

using std::string;
using std::vector;
using std::ostringstream;

// ──────────────────────────────────────────
// departmentFor  –  map category to department name
// ──────────────────────────────────────────
string departmentFor(const string& category) {
    if (category == "Pothole")       return "Road";
    if (category == "Garbage")       return "Sanitation";
    if (category == "Street Light")  return "Electrical";
    if (category == "Water Leakage") return "Water";
    if (category == "Drainage")      return "Drainage";
    return "General";
}

// ══════════════════════════════════════════
// ComplaintList
// ══════════════════════════════════════════
ComplaintList::~ComplaintList() {
    clear();
}

void ComplaintList::clear() {
    ComplaintNode* cur = head;
    while (cur) {
        ComplaintNode* nxt = cur->next;
        delete cur;
        cur = nxt;
    }
    head = nullptr;
}

void ComplaintList::add(const Complaint& c) {
    ComplaintNode* node = new ComplaintNode(c);
    // Append at tail so insertion order is preserved
    if (!head) {
        head = node;
        return;
    }
    ComplaintNode* cur = head;
    while (cur->next) cur = cur->next;
    cur->next = node;
}

Complaint* ComplaintList::find(int id) {
    ComplaintNode* cur = head;
    while (cur) {
        if (cur->data.id == id) return &cur->data;
        cur = cur->next;
    }
    return nullptr;
}

vector<Complaint*> ComplaintList::getAllComplaints() {
    vector<Complaint*> result;
    ComplaintNode* cur = head;
    while (cur) {
        result.push_back(&cur->data);
        cur = cur->next;
    }
    return result;
}

// ══════════════════════════════════════════
// HashTable
// ══════════════════════════════════════════
HashTable::HashTable() {
    clear();
}

void HashTable::clear() {
    for (int i = 0; i < SIZE; ++i) {
        table[i].key   = -1;
        table[i].value = nullptr;
    }
}

void HashTable::insert(int id, Complaint* ptr) {
    int idx = hash(id);
    // Linear probing with collision handling
    for (int i = 0; i < SIZE; ++i) {
        int probe = (idx + i) % SIZE;
        if (table[probe].key == -1 || table[probe].key == id) {
            table[probe].key   = id;
            table[probe].value = ptr;
            return;
        }
    }
}

Complaint* HashTable::search(int id) const {
    int idx = hash(id);
    for (int i = 0; i < SIZE; ++i) {
        int probe = (idx + i) % SIZE;
        if (table[probe].key == -1)  return nullptr; // empty slot – key not present
        if (table[probe].key == id)  return table[probe].value;
    }
    return nullptr;
}

// ══════════════════════════════════════════
// ComplaintQueue  (FIFO)
// ══════════════════════════════════════════
ComplaintQueue::~ComplaintQueue() {
    clear();
}

void ComplaintQueue::clear() {
    QueueNode* cur = front;
    while (cur) {
        QueueNode* nxt = cur->next;
        delete cur;
        cur = nxt;
    }
    front = rear = nullptr;
    count = 0;
}

void ComplaintQueue::enqueue(Complaint* c) {
    QueueNode* node = new QueueNode(c);
    if (!rear) {
        front = rear = node;
    } else {
        rear->next = node;
        rear = node;
    }
    ++count;
}

bool ComplaintQueue::dequeue(Complaint*& out) {
    if (!front) return false;
    out = front->data;
    QueueNode* old = front;
    front = front->next;
    if (!front) rear = nullptr;
    delete old;
    --count;
    return true;
}

string ComplaintQueue::contentsAsString() const {
    if (!front) return "(empty)";
    ostringstream oss;
    QueueNode* cur = front;
    while (cur) {
        oss << "#" << cur->data->id
            << " [" << cur->data->citizen << "] "
            << cur->data->status;
        if (cur->next) oss << " \xe2\x86\x92 ";
        cur = cur->next;
    }
    return oss.str();
}

// ══════════════════════════════════════════
// PriorityQueue  (max-heap on Priority)
// ══════════════════════════════════════════
void PriorityQueue::heapifyUp(int idx) {
    while (idx > 0) {
        int parent = (idx - 1) / 2;
        if (heap[parent]->priority < heap[idx]->priority) {
            std::swap(heap[parent], heap[idx]);
            idx = parent;
        } else break;
    }
}

void PriorityQueue::heapifyDown(int idx) {
    int n = (int)heap.size();
    while (true) {
        int left  = 2 * idx + 1;
        int right = 2 * idx + 2;
        int largest = idx;
        if (left  < n && heap[left]->priority  > heap[largest]->priority) largest = left;
        if (right < n && heap[right]->priority > heap[largest]->priority) largest = right;
        if (largest == idx) break;
        std::swap(heap[largest], heap[idx]);
        idx = largest;
    }
}

void PriorityQueue::push(Complaint* c) {
    heap.push_back(c);
    heapifyUp((int)heap.size() - 1);
}

bool PriorityQueue::pop(Complaint*& out) {
    if (heap.empty()) return false;
    out = heap[0];
    heap[0] = heap.back();
    heap.pop_back();
    if (!heap.empty()) heapifyDown(0);
    return true;
}

Complaint* PriorityQueue::peekTop() const {
    return heap.empty() ? nullptr : heap[0];
}

// ══════════════════════════════════════════
// CivicSystem
// ══════════════════════════════════════════

// Internal helper: pick the right FIFO queue by department name
ComplaintQueue& CivicSystem::getQueue(const string& dept) {
    if (dept == "Road")       return road;
    if (dept == "Sanitation") return sanitation;
    if (dept == "Electrical") return electrical;
    if (dept == "Water")      return water;
    if (dept == "Drainage")   return drainage;
    return general;
}

void CivicSystem::clearAll() {
    complaints.clear();
    index.clear();
    road.clear();
    sanitation.clear();
    electrical.clear();
    water.clear();
    drainage.clear();
    general.clear();
    urgent.clear();
}

void CivicSystem::loadFromRecords(const vector<Complaint>& records) {
    clearAll();
    int maxId = 1000;
    for (const auto& rec : records) {
        complaints.add(rec);
        Complaint* ptr = complaints.find(rec.id);
        if (ptr) {
            index.insert(ptr->id, ptr);
            if (ptr->status != "Resolved" && ptr->status != "Closed") {
                getQueue(ptr->department).enqueue(ptr);
                if (ptr->priority == HIGH) {
                    urgent.push(ptr);
                }
            }
            if (ptr->id > maxId) maxId = ptr->id;
        }
    }
    nextId = maxId + 1;
}

bool CivicSystem::registerComplaint(const string& citizen,
                                    const string& contact,
                                    const string& category,
                                    const string& description,
                                    const string& location,
                                    int            priority,
                                    int*           outId,
                                    string*        outDept)
{
    if (citizen.empty() || category.empty()) return false;

    Complaint c;
    c.id          = nextId++;
    c.citizen     = citizen;
    c.contact     = contact;
    c.category    = category;
    c.description = description;
    c.location    = location;
    c.priority    = static_cast<Priority>(priority);
    c.department  = departmentFor(category);
    c.status      = "Registered";

    // Add to linked list
    complaints.add(c);

    // Get pointer into the list (stable pointer since list node is heap-allocated)
    Complaint* ptr = complaints.find(c.id);

    // Insert into hash table for O(1) lookup
    index.insert(c.id, ptr);

    // Enqueue in the department's FIFO queue
    getQueue(c.department).enqueue(ptr);

    // HIGH-priority complaints also go to the urgent priority queue
    if (c.priority == HIGH) {
        urgent.push(ptr);
    }

    if (outId)   *outId   = c.id;
    if (outDept) *outDept = c.department;
    return true;
}

Complaint* CivicSystem::searchById(int id) {
    return index.search(id);
}

bool CivicSystem::updateStatus(int id, const string& newStatus) {
    Complaint* ptr = index.search(id);
    if (!ptr) return false;
    ptr->status = newStatus;
    return true;
}

bool CivicSystem::processNextFromDept(const string& dept, Complaint& outComplaint) {
    Complaint* ptr = nullptr;
    bool ok = getQueue(dept).dequeue(ptr);
    if (!ok || !ptr) return false;
    ptr->status = "Resolved";
    outComplaint = *ptr;
    return true;
}

bool CivicSystem::processUrgent(Complaint& outComplaint) {
    Complaint* ptr = nullptr;
    bool ok = urgent.pop(ptr);
    if (!ok || !ptr) return false;
    ptr->status = "Urgent Processing";
    outComplaint = *ptr;
    return true;
}

vector<Complaint*> CivicSystem::getAllComplaints() {
    return complaints.getAllComplaints();
}

int CivicSystem::getTotalCount() {
    return (int)complaints.getAllComplaints().size();
}

int CivicSystem::getPendingCount() {
    int cnt = 0;
    for (Complaint* c : complaints.getAllComplaints()) {
        if (c->status != "Resolved" && c->status != "Closed") ++cnt;
    }
    return cnt;
}

int CivicSystem::getResolvedCount() {
    int cnt = 0;
    for (Complaint* c : complaints.getAllComplaints()) {
        if (c->status == "Resolved" || c->status == "Closed") ++cnt;
    }
    return cnt;
}

string CivicSystem::getQueueInfo(const string& dept) {
    return getQueue(dept).contentsAsString();
}
