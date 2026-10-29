# Intelligent Civic Grievance Detection, Routing and Tracking System

A complete desktop application built with **C++17**, **Qt 6 Widgets**, and a **MySQL 8.0** relational database for managing civic complaints, automating department routing, enforcing role-based access control, and tracking status audit histories.

---

## 🏛️ System Features & Roles

### 1. Two-Tier Role-Based Access Control (RBAC)
- **Super Admin**:
  - Full system oversight across all municipal departments.
  - Create and manage Department Administrator accounts.
  - Assign and reassign administrators to specific departments.
  - View cross-department grievance metrics and statistics.
  - View, search, and update any civic complaint.
- **Department Admin**:
  - Assigned strictly to a single municipal department (**Road, Sanitation, Electrical, Water, Drainage, or General**).
  - Can only view, search, and update grievances routed to their assigned department.
  - Access permissions are enforced at the **backend and database query level** (never relying solely on UI restrictions).
  - Processes department-specific FIFO complaint queues and urgent tasks.

### 2. Core Complaint Management
- **Automatic Routing**: Based on selected grievance categories:
  - *Pothole* &rarr; Road Department
  - *Garbage* &rarr; Sanitation Department
  - *Street Light* &rarr; Electrical Department
  - *Water Leakage* &rarr; Water Department
  - *Drainage* &rarr; Drainage Department
  - *General Issue* &rarr; General Department
- **Priority & Urgent Queue**: Complaints flagged as *High Priority* automatically enqueue into a **Max-Heap Priority Queue** for urgent dispatch.
- **Audit Trail & History**: Every status transition (*Registered &rarr; In Progress &rarr; Resolved &rarr; Closed*) is permanently logged with timestamps, operator user ID, and notes in the `status_history` table.

---

## 🧠 Data Structures & Algorithms (DSA) Implemented

1. **Singly-Linked List (`ComplaintList`)**:
   - Maintains all complaints in chronologically preserved order in heap memory.
2. **Hash Table with Linear Probing (`HashTable`)**:
   - Maps `complaint_id -> Complaint*` with a capacity of 1009 slots and collision resolution for $O(1)$ lookup performance.
3. **FIFO Queues (`ComplaintQueue`)**:
   - Dedicated FIFO linked queues per municipal department ensuring complaints are served in order of arrival.
4. **Max-Heap Priority Queue (`PriorityQueue`)**:
   - Binary max-heap prioritizing high-urgency civic emergencies.

---

## 🗄️ Database Architecture (MySQL 8.0)

The relational schema is defined in [`schema.sql`](schema.sql):
- **`departments`**: Stores municipal departments (`Road`, `Sanitation`, `Electrical`, `Water`, `Drainage`, `General`).
- **`users`**: User accounts with unique usernames, salted multi-iteration SHA-256 password hashes, roles (`SuperAdmin`, `DeptAdmin`), and foreign key `department_id` (NULL for Super Admin).
- **`complaints`**: Grievances with primary key `complaint_id`, citizen name & contact details, category, location, priority, status, department ID, and timestamps.
- **`status_history`**: Audit log recording previous status, new status, user ID who executed the change, audit notes, and timestamp.

---

## 🚀 Setup & Execution Guide

### 1. Database Setup
Ensure MySQL Server 8.0 is running. Load the database schema:

```powershell
Get-Content schema.sql | & "C:\Program Files\MySQL\MySQL Server 8.0\bin\mysql.exe" -u root -p
```
*(Enter your MySQL root password when prompted)*

### 2. Building the Project
The project uses CMake and requires **Qt 6 (Widgets, Sql)** and a C++17 compatible compiler (e.g. MinGW 13.1+ or MSVC).

```powershell
# Set Qt environment
$env:Path = "C:\Qt\Tools\mingw1310_64\bin;C:\Qt\Tools\Ninja;C:\Qt\6.12.0\mingw_64\bin;" + $env:Path

# Configure build with CMake
cmake -S frontend -B build -G "Ninja" -DCMAKE_PREFIX_PATH="C:/Qt/6.12.0/mingw_64"

# Compile executable
ninja -C build
```

### 3. Launching & Initial Super Admin Provisioning
Run the compiled executable:
```powershell
.\build\CivicGrievanceGUI.exe
```

1. On first launch, click **⚙️ Database Connection** to enter your MySQL root password and verify connection to `civic_grievance`.
2. Click **Create Super Admin** to initialize your initial administrator account with your chosen secure username and password.
3. Log in with your new credentials to access the **Super Admin Portal**.
4. From the **Manage Admins** tab, create Department Admin accounts for municipal workers.
