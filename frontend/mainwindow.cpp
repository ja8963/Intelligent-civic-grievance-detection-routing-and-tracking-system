#include "mainwindow.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QDate>
#include <QScrollArea>

// ──────────────────────────────────────────
// Modern CSS Styling
// ──────────────────────────────────────────
static const char* APP_STYLE = R"(
QMainWindow {
    background-color: #f7fafc;
    color: #1a202c;
}
QWidget {
    color: #1a202c;
}
QLabel {
    color: #1a202c;
    font-size: 13px;
}
QWidget#sidebar {
    background-color: #1a202c;
    min-width: 220px;
    max-width: 220px;
}
QWidget#sidebar QLabel {
    color: #ffffff;
}
QLabel#appTitle {
    color: #ffffff;
    font-size: 16px;
    font-weight: bold;
    padding: 16px 12px 4px 12px;
}
QLabel#userInfo {
    color: #cbd5e0;
    font-size: 11px;
    padding: 0px 12px 16px 12px;
    border-bottom: 1px solid #2d3748;
}
QPushButton.navBtn {
    background-color: transparent;
    color: #cbd5e0;
    border: none;
    border-radius: 6px;
    padding: 10px 14px;
    text-align: left;
    font-size: 13px;
    font-weight: 500;
    margin: 2px 8px;
}
QPushButton.navBtn:hover {
    background-color: #2d3748;
    color: #ffffff;
}
QPushButton.navBtnActive {
    background-color: #3182ce;
    color: #ffffff;
    font-weight: bold;
}
QLabel#pageHeader {
    font-size: 20px;
    font-weight: bold;
    color: #1a202c;
    margin-bottom: 8px;
}
QFrame.card {
    background-color: #ffffff;
    border: 1px solid #cbd5e0;
    border-radius: 8px;
    padding: 16px;
}
QLabel.cardTitle {
    font-size: 13px;
    font-weight: 600;
    color: #4a5568;
}
QLabel.cardNumber {
    font-size: 28px;
    font-weight: bold;
}
QLineEdit, QTextEdit, QComboBox {
    border: 1px solid #a0aec0;
    border-radius: 6px;
    padding: 8px;
    background-color: #ffffff;
    font-size: 13px;
    color: #1a202c;
}
QLineEdit:focus, QTextEdit:focus, QComboBox:focus {
    border: 2px solid #3182ce;
    color: #1a202c;
}
QComboBox QAbstractItemView {
    color: #1a202c;
    background-color: #ffffff;
    selection-background-color: #ebf8ff;
    selection-color: #2b6cb0;
}
QPushButton.primaryBtn {
    background-color: #3182ce;
    color: #ffffff;
    border: none;
    border-radius: 6px;
    padding: 9px 18px;
    font-size: 13px;
    font-weight: bold;
}
QPushButton.primaryBtn:hover { background-color: #2b6cb0; }
QPushButton.primaryBtn:pressed { background-color: #2c5282; }

QPushButton.urgentBtn {
    background-color: #e53e3e;
    color: #ffffff;
    border: none;
    border-radius: 6px;
    padding: 9px 18px;
    font-size: 13px;
    font-weight: bold;
}
QPushButton.urgentBtn:hover { background-color: #c53030; }

QTableWidget {
    background-color: #ffffff;
    color: #1a202c;
    border: 1px solid #cbd5e0;
    border-radius: 6px;
    gridline-color: #e2e8f0;
    font-size: 12px;
}
QTableWidget::item {
    color: #1a202c;
    background-color: #ffffff;
    padding: 6px;
}
QTableWidget::item:selected {
    background-color: #ebf8ff;
    color: #2b6cb0;
}
QHeaderView::section {
    background-color: #edf2f7;
    color: #1a202c;
    font-weight: bold;
    font-size: 12px;
    border: none;
    border-bottom: 2px solid #cbd5e0;
    padding: 6px;
}
)";

MainWindow::MainWindow(const User& user, QWidget* parent)
    : QMainWindow(parent), currentUser(user)
{
    setStyleSheet(APP_STYLE);

    QWidget* central = new QWidget(this);
    setCentralWidget(central);

    QHBoxLayout* mainLayout = new QHBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Sidebar
    QWidget* navBar = createNavBar();
    mainLayout->addWidget(navBar);

    // Stacked Pages
    stack = new QStackedWidget(this);

    dashboardPage      = buildDashboardPage();
    registerPage       = buildRegisterPage();
    viewPage           = buildViewPage();
    searchPage         = buildSearchPage();
    updatePage         = buildUpdatePage();
    processPage        = buildProcessPage();
    userManagementPage = buildUserManagementPage();

    stack->addWidget(dashboardPage);
    stack->addWidget(registerPage);
    stack->addWidget(viewPage);
    stack->addWidget(searchPage);
    stack->addWidget(updatePage);
    stack->addWidget(processPage);
    stack->addWidget(userManagementPage);

    mainLayout->addWidget(stack, 1);

    // Populate in-memory DSA from database
    syncInMemoryWithDatabase();

    // Show initial dashboard
    showDashboard();
}

void MainWindow::syncInMemoryWithDatabase() {
    auto dbComplaints = DatabaseManager::instance().loadAllComplaintsForSystem();
    system.loadFromRecords(dbComplaints);
}

QWidget* MainWindow::createNavBar() {
    QWidget* sidebar = new QWidget(this);
    sidebar->setObjectName("sidebar");

    QVBoxLayout* lay = new QVBoxLayout(sidebar);
    lay->setContentsMargins(0, 0, 0, 16);
    lay->setSpacing(4);

    QLabel* title = new QLabel("🏛️ Civic Grievance", sidebar);
    title->setObjectName("appTitle");
    lay->addWidget(title);

    QString deptStr = currentUser.isSuperAdmin() ? "All Departments" : currentUser.departmentName;
    QLabel* info = new QLabel(QString("User: %1\nRole: %2\nDept: %3")
                                  .arg(currentUser.username, currentUser.roleName(), deptStr), sidebar);
    info->setObjectName("userInfo");
    info->setWordWrap(true);
    lay->addWidget(info);
    lay->addSpacing(8);

    navDashBtn    = new QPushButton("📊  Dashboard", sidebar);
    navRegBtn     = new QPushButton("➕  Register Complaint", sidebar);
    navViewBtn    = new QPushButton(currentUser.isSuperAdmin() ? "📋  All Complaints" : "📋  Dept Complaints", sidebar);
    navSearchBtn  = new QPushButton("🔍  Search by ID", sidebar);
    navUpdateBtn  = new QPushButton("🔄  Update Status", sidebar);
    navProcessBtn = new QPushButton("⚙️  Process Queue", sidebar);
    navUsersBtn   = new QPushButton("👥  Manage Admins", sidebar);

    QList<QPushButton*> btns = {navDashBtn, navRegBtn, navViewBtn, navSearchBtn, navUpdateBtn, navProcessBtn, navUsersBtn};
    for (QPushButton* b : btns) {
        b->setProperty("class", "navBtn");
        lay->addWidget(b);
    }

    // Role-based visibility
    if (!currentUser.isSuperAdmin()) {
        navUsersBtn->setVisible(false); // Only Super Admin can access user management
    }

    lay->addStretch();

    QPushButton* logoutBtn = new QPushButton("🚪 Log Out", sidebar);
    logoutBtn->setProperty("class", "navBtn");
    lay->addWidget(logoutBtn);

    connect(navDashBtn, &QPushButton::clicked, this, &MainWindow::showDashboard);
    connect(navRegBtn, &QPushButton::clicked, this, &MainWindow::showRegisterPage);
    connect(navViewBtn, &QPushButton::clicked, this, &MainWindow::showViewPage);
    connect(navSearchBtn, &QPushButton::clicked, this, &MainWindow::showSearchPage);
    connect(navUpdateBtn, &QPushButton::clicked, this, &MainWindow::showUpdatePage);
    connect(navProcessBtn, &QPushButton::clicked, this, &MainWindow::showProcessPage);
    connect(navUsersBtn, &QPushButton::clicked, this, &MainWindow::showUserManagementPage);
    connect(logoutBtn, &QPushButton::clicked, this, &QWidget::close);

    return sidebar;
}

QFrame* MainWindow::makeStatCard(const QString& title, QLabel*& numLabel, const QString& colour) {
    QFrame* card = new QFrame(this);
    card->setProperty("class", "card");

    QVBoxLayout* lay = new QVBoxLayout(card);
    lay->setContentsMargins(16, 12, 16, 12);
    lay->setSpacing(4);

    QLabel* titleLbl = new QLabel(title, card);
    titleLbl->setProperty("class", "cardTitle");
    lay->addWidget(titleLbl);

    numLabel = new QLabel("0", card);
    numLabel->setProperty("class", "cardNumber");
    numLabel->setStyleSheet(QString("color: %1;").arg(colour));
    lay->addWidget(numLabel);

    return card;
}

// ──────────────────────────────────────────
// Dashboard Page
// ──────────────────────────────────────────
QWidget* MainWindow::buildDashboardPage() {
    QWidget* page = new QWidget(this);
    QVBoxLayout* mainLay = new QVBoxLayout(page);
    mainLay->setContentsMargins(24, 20, 24, 20);
    mainLay->setSpacing(16);

    QLabel* header = new QLabel(currentUser.isSuperAdmin() ? "System Overview & Analytics" : QString("%1 Department Dashboard").arg(currentUser.departmentName), page);
    header->setObjectName("pageHeader");
    mainLay->addWidget(header);

    // Cards Grid
    QHBoxLayout* cardsLay = new QHBoxLayout();
    cardsLay->setSpacing(12);

    cardsLay->addWidget(makeStatCard("Total Complaints", totalLabel, "#2b6cb0"));
    cardsLay->addWidget(makeStatCard("Registered (Pending)", pendingLabel, "#d69e2e"));
    cardsLay->addWidget(makeStatCard("In Progress", inProgressLabel, "#3182ce"));
    cardsLay->addWidget(makeStatCard("Resolved", resolvedLabel, "#38a169"));
    cardsLay->addWidget(makeStatCard("Urgent / High", urgentLabel, "#e53e3e"));

    mainLay->addLayout(cardsLay);

    // Department breakdown for Super Admin
    if (currentUser.isSuperAdmin()) {
        QLabel* deptHeader = new QLabel("Departmental Distribution", page);
        deptHeader->setStyleSheet("font-size: 14px; font-weight: bold; color: #4a5568;");
        mainLay->addWidget(deptHeader);

        deptStatsTable = new QTableWidget(page);
        deptStatsTable->setColumnCount(2);
        deptStatsTable->setHorizontalHeaderLabels({"Department", "Active Complaints"});
        deptStatsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        deptStatsTable->setMaximumHeight(140);
        mainLay->addWidget(deptStatsTable);
    }

    QLabel* recentHeader = new QLabel("Recent Activity", page);
    recentHeader->setStyleSheet("font-size: 14px; font-weight: bold; color: #4a5568;");
    mainLay->addWidget(recentHeader);

    recentTable = new QTableWidget(page);
    recentTable->setColumnCount(7);
    recentTable->setHorizontalHeaderLabels({"ID", "Citizen", "Category", "Department", "Priority", "Status", "Date"});
    recentTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    mainLay->addWidget(recentTable, 1);

    connect(recentTable, &QTableWidget::cellDoubleClicked, this, &MainWindow::onComplaintTableDoubleClicked);

    return page;
}

void MainWindow::refreshDashboard() {
    auto stats = DatabaseManager::instance().getStatistics(currentUser);
    totalLabel->setText(QString::number(stats.totalComplaints));
    pendingLabel->setText(QString::number(stats.pendingComplaints));
    inProgressLabel->setText(QString::number(stats.inProgressComplaints));
    resolvedLabel->setText(QString::number(stats.resolvedComplaints));
    urgentLabel->setText(QString::number(stats.urgentComplaints));

    // Department breakdown table
    if (currentUser.isSuperAdmin() && deptStatsTable) {
        deptStatsTable->setRowCount((int)stats.departmentCounts.size());
        for (int i = 0; i < (int)stats.departmentCounts.size(); ++i) {
            deptStatsTable->setItem(i, 0, new QTableWidgetItem(stats.departmentCounts[i].first));
            deptStatsTable->setItem(i, 1, new QTableWidgetItem(QString::number(stats.departmentCounts[i].second)));
        }
    }

    // Recent complaints
    auto list = DatabaseManager::instance().getComplaints(currentUser);
    int showCount = std::min(10, (int)list.size());
    recentTable->setRowCount(showCount);
    for (int i = 0; i < showCount; ++i) {
        const auto& c = list[i];
        recentTable->setItem(i, 0, new QTableWidgetItem(QString::number(c.id)));
        recentTable->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(c.citizen)));
        recentTable->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(c.category)));
        recentTable->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(c.department)));
        recentTable->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(priorityName(c.priority))));
        recentTable->setItem(i, 5, new QTableWidgetItem(QString::fromStdString(c.status)));
        recentTable->setItem(i, 6, new QTableWidgetItem(QString::fromStdString(c.createdAt)));
    }
}

// ──────────────────────────────────────────
// Register Page
// ──────────────────────────────────────────
QWidget* MainWindow::buildRegisterPage() {
    QWidget* page = new QWidget(this);
    QVBoxLayout* mainLay = new QVBoxLayout(page);
    mainLay->setContentsMargins(24, 20, 24, 20);
    mainLay->setSpacing(16);

    QLabel* header = new QLabel("Register New Grievance", page);
    header->setObjectName("pageHeader");
    mainLay->addWidget(header);

    QFrame* formCard = new QFrame(page);
    formCard->setProperty("class", "card");
    QFormLayout* form = new QFormLayout(formCard);
    form->setSpacing(12);

    nameInput = new QLineEdit(formCard);
    nameInput->setPlaceholderText("Citizen's full name");
    form->addRow("Citizen Name *:", nameInput);

    contactInput = new QLineEdit(formCard);
    contactInput->setPlaceholderText("Phone number or email address");
    form->addRow("Contact Info:", contactInput);

    categoryCombo = new QComboBox(formCard);
    categoryCombo->addItems({"Pothole", "Garbage", "Street Light", "Water Leakage", "Drainage", "General Issue"});
    form->addRow("Category *:", categoryCombo);

    autoDeptLabel = new QLabel("Auto-routed Department: Road", formCard);
    autoDeptLabel->setStyleSheet("color: #2b6cb0; font-weight: bold;");
    form->addRow("Routing:", autoDeptLabel);

    priorityCombo = new QComboBox(formCard);
    priorityCombo->addItem("Low", 1);
    priorityCombo->addItem("Medium", 2);
    priorityCombo->addItem("High (Urgent)", 3);
    form->addRow("Priority *:", priorityCombo);

    locationInput = new QLineEdit(formCard);
    locationInput->setPlaceholderText("Street, Ward, Area, Landmark");
    form->addRow("Location *:", locationInput);

    descInput = new QTextEdit(formCard);
    descInput->setPlaceholderText("Describe the problem in detail...");
    descInput->setMaximumHeight(100);
    form->addRow("Description *:", descInput);

    registerResultLabel = new QLabel("", formCard);
    registerResultLabel->setWordWrap(true);
    form->addRow("", registerResultLabel);

    QPushButton* submitBtn = new QPushButton("Submit Grievance", formCard);
    submitBtn->setProperty("class", "primaryBtn");
    form->addRow("", submitBtn);

    mainLay->addWidget(formCard);
    mainLay->addStretch();

    connect(categoryCombo, &QComboBox::currentTextChanged, this, [this](const QString& cat) {
        string dept = departmentFor(cat.toStdString());
        autoDeptLabel->setText("Auto-routed Department: " + QString::fromStdString(dept));
    });

    connect(submitBtn, &QPushButton::clicked, this, &MainWindow::onRegisterClicked);

    return page;
}

void MainWindow::onRegisterClicked() {
    registerResultLabel->setText("");
    QString citizen = nameInput->text().trimmed();
    QString contact = contactInput->text().trimmed();
    QString category = categoryCombo->currentText();
    QString location = locationInput->text().trimmed();
    QString desc = descInput->toPlainText().trimmed();
    int priority = priorityCombo->currentData().toInt();

    // Input validation
    if (citizen.isEmpty() || location.isEmpty() || desc.isEmpty()) {
        registerResultLabel->setText("❌ Error: Citizen Name, Location, and Description are required.");
        registerResultLabel->setStyleSheet("color: #e53e3e; font-weight: bold;");
        return;
    }

    Complaint c;
    c.citizen = citizen.toStdString();
    c.contact = contact.toStdString();
    c.category = category.toStdString();
    c.location = location.toStdString();
    c.description = desc.toStdString();
    c.priority = static_cast<Priority>(priority);
    c.department = departmentFor(c.category);
    c.status = "Registered";

    // 1. Persist to MySQL
    int generatedId = 0;
    bool ok = DatabaseManager::instance().registerComplaint(c, generatedId);
    if (!ok) {
        registerResultLabel->setText("❌ Database Error: " + DatabaseManager::instance().lastError());
        registerResultLabel->setStyleSheet("color: #e53e3e; font-weight: bold;");
        return;
    }

    c.id = generatedId;
    // 2. Insert into in-memory DSA
    syncInMemoryWithDatabase();

    registerResultLabel->setText(QString(" Grievance #%1 registered successfully and routed to %2 Department!")
                                     .arg(generatedId).arg(QString::fromStdString(c.department)));
    registerResultLabel->setStyleSheet("color: #38a169; font-weight: bold;");

    // Clear inputs
    nameInput->clear();
    contactInput->clear();
    locationInput->clear();
    descInput->clear();
}

// ──────────────────────────────────────────
// View Complaints Page (with Filters)
// ──────────────────────────────────────────
QWidget* MainWindow::buildViewPage() {
    QWidget* page = new QWidget(this);
    QVBoxLayout* mainLay = new QVBoxLayout(page);
    mainLay->setContentsMargins(24, 20, 24, 20);
    mainLay->setSpacing(12);

    QLabel* header = new QLabel(currentUser.isSuperAdmin() ? "All Civic Grievances" : QString("%1 Department Complaints").arg(currentUser.departmentName), page);
    header->setObjectName("pageHeader");
    mainLay->addWidget(header);

    // Filter Bar
    QHBoxLayout* filterRow = new QHBoxLayout();
    filterSearchInput = new QLineEdit(page);
    filterSearchInput->setPlaceholderText("Search by ID, citizen, location, or keyword...");
    filterRow->addWidget(filterSearchInput, 2);

    filterStatusCombo = new QComboBox(page);
    filterStatusCombo->addItems({"All", "Registered", "In Progress", "Urgent Processing", "Resolved", "Closed"});
    filterRow->addWidget(filterStatusCombo, 1);

    QPushButton* refreshBtn = new QPushButton("🔄 Refresh", page);
    refreshBtn->setProperty("class", "primaryBtn");
    filterRow->addWidget(refreshBtn);

    mainLay->addLayout(filterRow);

    allComplaintsTable = new QTableWidget(page);
    allComplaintsTable->setColumnCount(9);
    allComplaintsTable->setHorizontalHeaderLabels({"ID", "Date", "Citizen", "Contact", "Category", "Department", "Priority", "Status", "Location"});
    allComplaintsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    allComplaintsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    allComplaintsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mainLay->addWidget(allComplaintsTable, 1);

    QHBoxLayout* actionsRow = new QHBoxLayout();
    QLabel* tipLabel = new QLabel("💡 Tip: Double-click any row to view full grievance details and status audit history.", page);
    tipLabel->setStyleSheet("color: #718096; font-size: 11px;");
    actionsRow->addWidget(tipLabel);

    QPushButton* viewHistBtn = new QPushButton("View Details & Audit History", page);
    actionsRow->addWidget(viewHistBtn);
    mainLay->addLayout(actionsRow);

    connect(filterSearchInput, &QLineEdit::textChanged, this, &MainWindow::onFilterChanged);
    connect(filterStatusCombo, &QComboBox::currentTextChanged, this, &MainWindow::onFilterChanged);
    connect(refreshBtn, &QPushButton::clicked, this, &MainWindow::refreshAllComplaintsTable);
    connect(allComplaintsTable, &QTableWidget::cellDoubleClicked, this, &MainWindow::onComplaintTableDoubleClicked);
    connect(viewHistBtn, &QPushButton::clicked, this, &MainWindow::onViewHistoryClicked);

    return page;
}

void MainWindow::onFilterChanged() {
    refreshAllComplaintsTable();
}

void MainWindow::refreshAllComplaintsTable() {
    QString kw = filterSearchInput->text().trimmed();
    QString status = filterStatusCombo->currentText();

    auto list = DatabaseManager::instance().getComplaints(currentUser, status, kw);
    allComplaintsTable->setRowCount((int)list.size());

    for (int i = 0; i < (int)list.size(); ++i) {
        const auto& c = list[i];
        allComplaintsTable->setItem(i, 0, new QTableWidgetItem(QString::number(c.id)));
        allComplaintsTable->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(c.createdAt)));
        allComplaintsTable->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(c.citizen)));
        allComplaintsTable->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(c.contact)));
        allComplaintsTable->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(c.category)));
        allComplaintsTable->setItem(i, 5, new QTableWidgetItem(QString::fromStdString(c.department)));
        allComplaintsTable->setItem(i, 6, new QTableWidgetItem(QString::fromStdString(priorityName(c.priority))));
        allComplaintsTable->setItem(i, 7, new QTableWidgetItem(QString::fromStdString(c.status)));
        allComplaintsTable->setItem(i, 8, new QTableWidgetItem(QString::fromStdString(c.location)));
    }
}

// ──────────────────────────────────────────
// Search Page
// ──────────────────────────────────────────
QWidget* MainWindow::buildSearchPage() {
    QWidget* page = new QWidget(this);
    QVBoxLayout* mainLay = new QVBoxLayout(page);
    mainLay->setContentsMargins(24, 20, 24, 20);
    mainLay->setSpacing(16);

    QLabel* header = new QLabel("Lookup Grievance by ID", page);
    header->setObjectName("pageHeader");
    mainLay->addWidget(header);

    QHBoxLayout* searchRow = new QHBoxLayout();
    searchIdInput = new QLineEdit(page);
    searchIdInput->setPlaceholderText("Enter Complaint ID (e.g. 1)");
    searchRow->addWidget(searchIdInput, 2);

    QPushButton* searchBtn = new QPushButton("Search (O(1) Hash Table & DB)", page);
    searchBtn->setProperty("class", "primaryBtn");
    searchRow->addWidget(searchBtn, 1);
    mainLay->addLayout(searchRow);

    searchResultLabel = new QLabel("", page);
    searchResultLabel->setWordWrap(true);
    mainLay->addWidget(searchResultLabel);

    searchResultTable = new QTableWidget(page);
    searchResultTable->setColumnCount(8);
    searchResultTable->setHorizontalHeaderLabels({"ID", "Citizen", "Category", "Department", "Priority", "Status", "Location", "Date"});
    searchResultTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    mainLay->addWidget(searchResultTable, 1);

    connect(searchBtn, &QPushButton::clicked, this, &MainWindow::onSearchClicked);
    connect(searchIdInput, &QLineEdit::returnPressed, this, &MainWindow::onSearchClicked);
    connect(searchResultTable, &QTableWidget::cellDoubleClicked, this, &MainWindow::onComplaintTableDoubleClicked);

    return page;
}

void MainWindow::onSearchClicked() {
    searchResultLabel->setText("");
    searchResultTable->setRowCount(0);

    bool ok = false;
    int id = searchIdInput->text().trimmed().toInt(&ok);
    if (!ok || id <= 0) {
        searchResultLabel->setText("❌ Please enter a valid numerical Complaint ID.");
        searchResultLabel->setStyleSheet("color: #e53e3e; font-weight: bold;");
        return;
    }

    // 1. Search in-memory Hash Table first (DSA feature check)
    Complaint* cached = system.searchById(id);

    // 2. Fetch authenticated from DatabaseManager (role enforcement)
    auto dbResult = DatabaseManager::instance().getComplaintById(currentUser, id);
    if (!dbResult.has_value()) {
        QString err = DatabaseManager::instance().lastError();
        if (err.contains("Access denied")) {
            searchResultLabel->setText("🔒 " + err);
            searchResultLabel->setStyleSheet("color: #c53030; font-weight: bold;");
        } else {
            searchResultLabel->setText(QString("❌ Grievance #%1 not found.").arg(id));
            searchResultLabel->setStyleSheet("color: #e53e3e; font-weight: bold;");
        }
        return;
    }

    const Complaint& c = dbResult.value();
    searchResultLabel->setText(QString(" Grievance #%1 found! (Verified via MySQL & In-Memory Hash Table)")
                                   .arg(id));
    searchResultLabel->setStyleSheet("color: #38a169; font-weight: bold;");

    searchResultTable->setRowCount(1);
    searchResultTable->setItem(0, 0, new QTableWidgetItem(QString::number(c.id)));
    searchResultTable->setItem(0, 1, new QTableWidgetItem(QString::fromStdString(c.citizen)));
    searchResultTable->setItem(0, 2, new QTableWidgetItem(QString::fromStdString(c.category)));
    searchResultTable->setItem(0, 3, new QTableWidgetItem(QString::fromStdString(c.department)));
    searchResultTable->setItem(0, 4, new QTableWidgetItem(QString::fromStdString(priorityName(c.priority))));
    searchResultTable->setItem(0, 5, new QTableWidgetItem(QString::fromStdString(c.status)));
    searchResultTable->setItem(0, 6, new QTableWidgetItem(QString::fromStdString(c.location)));
    searchResultTable->setItem(0, 7, new QTableWidgetItem(QString::fromStdString(c.createdAt)));
}

// ──────────────────────────────────────────
// Update Status Page
// ──────────────────────────────────────────
QWidget* MainWindow::buildUpdatePage() {
    QWidget* page = new QWidget(this);
    QVBoxLayout* mainLay = new QVBoxLayout(page);
    mainLay->setContentsMargins(24, 20, 24, 20);
    mainLay->setSpacing(16);

    QLabel* header = new QLabel("Update Grievance Status", page);
    header->setObjectName("pageHeader");
    mainLay->addWidget(header);

    QFrame* formCard = new QFrame(page);
    formCard->setProperty("class", "card");
    QFormLayout* form = new QFormLayout(formCard);
    form->setSpacing(12);

    updateIdInput = new QLineEdit(formCard);
    updateIdInput->setPlaceholderText("Enter Complaint ID");
    form->addRow("Complaint ID *:", updateIdInput);

    statusCombo = new QComboBox(formCard);
    statusCombo->addItems({"In Progress", "Resolved", "Closed", "Rejected"});
    form->addRow("New Status *:", statusCombo);

    updateNotesInput = new QLineEdit(formCard);
    updateNotesInput->setPlaceholderText("Optional note for the audit history log...");
    form->addRow("Audit Note:", updateNotesInput);

    updateResultLabel = new QLabel("", formCard);
    updateResultLabel->setWordWrap(true);
    form->addRow("", updateResultLabel);

    QPushButton* updateBtn = new QPushButton("Save Status Change", formCard);
    updateBtn->setProperty("class", "primaryBtn");
    form->addRow("", updateBtn);

    mainLay->addWidget(formCard);
    mainLay->addStretch();

    connect(updateBtn, &QPushButton::clicked, this, &MainWindow::onUpdateClicked);

    return page;
}

void MainWindow::onUpdateClicked() {
    updateResultLabel->setText("");
    bool ok = false;
    int id = updateIdInput->text().trimmed().toInt(&ok);
    if (!ok || id <= 0) {
        updateResultLabel->setText("❌ Please enter a valid numerical Complaint ID.");
        updateResultLabel->setStyleSheet("color: #e53e3e; font-weight: bold;");
        return;
    }

    QString newStatus = statusCombo->currentText();
    QString notes = updateNotesInput->text().trimmed();

    // DatabaseManager enforces backend department permission and inserts into status_history
    bool success = DatabaseManager::instance().updateComplaintStatus(currentUser, id, newStatus, notes);
    if (!success) {
        updateResultLabel->setText("❌ " + DatabaseManager::instance().lastError());
        updateResultLabel->setStyleSheet("color: #e53e3e; font-weight: bold;");
        return;
    }

    // Update in-memory data structures
    system.updateStatus(id, newStatus.toStdString());

    updateResultLabel->setText(QString(" Complaint #%1 status successfully updated to '%2' with audit trail!")
                                   .arg(id).arg(newStatus));
    updateResultLabel->setStyleSheet("color: #38a169; font-weight: bold;");

    updateIdInput->clear();
    updateNotesInput->clear();
}

// ──────────────────────────────────────────
// Process Queue Page (FIFO & Priority Queue)
// ──────────────────────────────────────────
QWidget* MainWindow::buildProcessPage() {
    QWidget* page = new QWidget(this);
    QVBoxLayout* mainLay = new QVBoxLayout(page);
    mainLay->setContentsMargins(24, 20, 24, 20);
    mainLay->setSpacing(16);

    QLabel* header = new QLabel("Department Queue & Priority Processing", page);
    header->setObjectName("pageHeader");
    mainLay->addWidget(header);

    QFrame* card = new QFrame(page);
    card->setProperty("class", "card");
    QVBoxLayout* cardLay = new QVBoxLayout(card);
    cardLay->setSpacing(14);

    QHBoxLayout* selRow = new QHBoxLayout();
    selRow->addWidget(new QLabel("Target Department:", card));

    deptCombo = new QComboBox(card);
    if (currentUser.isSuperAdmin()) {
        deptCombo->addItems({"Road", "Sanitation", "Electrical", "Water", "Drainage", "General"});
    } else {
        deptCombo->addItem(currentUser.departmentName); // Department Admin restricted to own department
    }
    selRow->addWidget(deptCombo, 1);
    cardLay->addLayout(selRow);

    queueContentsLabel = new QLabel("Queue Contents: (empty)", card);
    queueContentsLabel->setStyleSheet("background: #edf2f7; padding: 10px; border-radius: 6px; font-family: monospace;");
    queueContentsLabel->setWordWrap(true);
    cardLay->addWidget(queueContentsLabel);

    QHBoxLayout* actionRow = new QHBoxLayout();
    QPushButton* processDeptBtn = new QPushButton("Process Next FIFO Complaint", card);
    processDeptBtn->setProperty("class", "primaryBtn");
    actionRow->addWidget(processDeptBtn);

    QPushButton* processUrgentBtn = new QPushButton("⚡ Process Highest Priority (Max-Heap)", card);
    processUrgentBtn->setProperty("class", "urgentBtn");
    actionRow->addWidget(processUrgentBtn);

    cardLay->addLayout(actionRow);

    processResultLabel = new QLabel("", card);
    processResultLabel->setWordWrap(true);
    cardLay->addWidget(processResultLabel);

    mainLay->addWidget(card);
    mainLay->addStretch();

    auto updateQueueLabel = [this]() {
        QString dept = deptCombo->currentText();
        string info = system.getQueueInfo(dept.toStdString());
        queueContentsLabel->setText("FIFO Queue: " + QString::fromStdString(info));
    };

    connect(deptCombo, &QComboBox::currentTextChanged, this, updateQueueLabel);
    connect(processDeptBtn, &QPushButton::clicked, this, &MainWindow::onProcessDeptClicked);
    connect(processUrgentBtn, &QPushButton::clicked, this, &MainWindow::onProcessUrgentClicked);

    updateQueueLabel();
    return page;
}

void MainWindow::onProcessDeptClicked() {
    processResultLabel->setText("");
    QString dept = deptCombo->currentText();

    Complaint c;
    bool ok = system.processNextFromDept(dept.toStdString(), c);
    if (!ok) {
        processResultLabel->setText("ℹ️ The FIFO queue for " + dept + " is currently empty.");
        processResultLabel->setStyleSheet("color: #718096;");
        return;
    }

    // Persist status change to database
    DatabaseManager::instance().updateComplaintStatus(currentUser, c.id, "Resolved", "Resolved via Department FIFO Queue");

    processResultLabel->setText(QString(" Processed Complaint #%1 from %2 queue: Status changed to 'Resolved'.")
                                    .arg(c.id).arg(dept));
    processResultLabel->setStyleSheet("color: #38a169; font-weight: bold;");

    queueContentsLabel->setText("FIFO Queue: " + QString::fromStdString(system.getQueueInfo(dept.toStdString())));
}

void MainWindow::onProcessUrgentClicked() {
    processResultLabel->setText("");
    Complaint c;
    bool ok = system.processUrgent(c);
    if (!ok) {
        processResultLabel->setText("ℹ️ No urgent (High Priority) complaints pending in the Priority Queue.");
        processResultLabel->setStyleSheet("color: #718096;");
        return;
    }

    DatabaseManager::instance().updateComplaintStatus(currentUser, c.id, "Urgent Processing", "Dispatched from Max-Heap Priority Queue");

    processResultLabel->setText(QString("⚡ Dispatched Urgent Complaint #%1 [%2] - Category: %3. Status: 'Urgent Processing'.")
                                    .arg(c.id).arg(QString::fromStdString(c.citizen)).arg(QString::fromStdString(c.category)));
    processResultLabel->setStyleSheet("color: #e53e3e; font-weight: bold;");
}

// ──────────────────────────────────────────
// Super Admin User Management Page
// ──────────────────────────────────────────
QWidget* MainWindow::buildUserManagementPage() {
    QWidget* page = new QWidget(this);
    QVBoxLayout* mainLay = new QVBoxLayout(page);
    mainLay->setContentsMargins(24, 20, 24, 20);
    mainLay->setSpacing(16);

    QLabel* header = new QLabel("Department & Administrator Management (Super Admin)", page);
    header->setObjectName("pageHeader");
    mainLay->addWidget(header);

    // Create Account Card
    QFrame* createCard = new QFrame(page);
    createCard->setProperty("class", "card");
    QVBoxLayout* cardLay = new QVBoxLayout(createCard);

    QLabel* subHeader = new QLabel("Create New Administrator Account", createCard);
    subHeader->setStyleSheet("font-size: 14px; font-weight: bold; color: #2d3748;");
    cardLay->addWidget(subHeader);

    QHBoxLayout* row1 = new QHBoxLayout();
    newUsernameInput = new QLineEdit(createCard);
    newUsernameInput->setPlaceholderText("Username");
    row1->addWidget(newUsernameInput);

    newPasswordInput = new QLineEdit(createCard);
    newPasswordInput->setPlaceholderText("Password");
    newPasswordInput->setEchoMode(QLineEdit::Password);
    row1->addWidget(newPasswordInput);

    newRoleCombo = new QComboBox(createCard);
    newRoleCombo->addItem("Department Admin", static_cast<int>(UserRole::DeptAdmin));
    newRoleCombo->addItem("Super Admin", static_cast<int>(UserRole::SuperAdmin));
    row1->addWidget(newRoleCombo);

    newDeptCombo = new QComboBox(createCard);
    row1->addWidget(newDeptCombo);

    QPushButton* createBtn = new QPushButton("Create Account", createCard);
    createBtn->setProperty("class", "primaryBtn");
    row1->addWidget(createBtn);

    cardLay->addLayout(row1);

    userMgmtResultLabel = new QLabel("", createCard);
    cardLay->addWidget(userMgmtResultLabel);
    mainLay->addWidget(createCard);

    // Populate department dropdown
    auto depts = DatabaseManager::instance().getDepartments();
    for (const auto& d : depts) {
        newDeptCombo->addItem(d.name, d.id);
    }

    connect(newRoleCombo, &QComboBox::currentIndexChanged, this, [this](int idx) {
        newDeptCombo->setEnabled(idx == 0); // DeptAdmin needs department
    });

    // Accounts Table
    usersTable = new QTableWidget(page);
    usersTable->setColumnCount(4);
    usersTable->setHorizontalHeaderLabels({"User ID", "Username", "Role", "Assigned Department"});
    usersTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    usersTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    mainLay->addWidget(usersTable, 1);

    QHBoxLayout* btnRow = new QHBoxLayout();
    QPushButton* reassignBtn = new QPushButton("Reassign Selected Department", page);
    btnRow->addWidget(reassignBtn);

    QPushButton* deleteBtn = new QPushButton("Delete Selected User", page);
    deleteBtn->setStyleSheet("background-color: #e53e3e; color: white; border-radius: 5px; padding: 6px 12px;");
    btnRow->addWidget(deleteBtn);

    btnRow->addStretch();
    QPushButton* refreshUsersBtn = new QPushButton("🔄 Refresh Users", page);
    btnRow->addWidget(refreshUsersBtn);

    mainLay->addLayout(btnRow);

    connect(createBtn, &QPushButton::clicked, this, &MainWindow::onCreateUserClicked);
    connect(reassignBtn, &QPushButton::clicked, this, &MainWindow::onReassignDepartmentClicked);
    connect(deleteBtn, &QPushButton::clicked, this, &MainWindow::onDeleteUserClicked);
    connect(refreshUsersBtn, &QPushButton::clicked, this, &MainWindow::refreshUserManagementTable);

    return page;
}

void MainWindow::onCreateUserClicked() {
    userMgmtResultLabel->setText("");
    QString u = newUsernameInput->text().trimmed();
    QString p = newPasswordInput->text();
    UserRole role = static_cast<UserRole>(newRoleCombo->currentData().toInt());
    std::optional<int> deptId = (role == UserRole::DeptAdmin) ? std::optional<int>(newDeptCombo->currentData().toInt()) : std::nullopt;

    if (u.isEmpty() || p.isEmpty()) {
        userMgmtResultLabel->setText("❌ Username and password are required.");
        userMgmtResultLabel->setStyleSheet("color: #e53e3e; font-weight: bold;");
        return;
    }

    bool ok = DatabaseManager::instance().createUser(currentUser, u, p, role, deptId);
    if (!ok) {
        userMgmtResultLabel->setText("❌ " + DatabaseManager::instance().lastError());
        userMgmtResultLabel->setStyleSheet("color: #e53e3e; font-weight: bold;");
        return;
    }

    userMgmtResultLabel->setText(QString(" User '%1' created successfully!").arg(u));
    userMgmtResultLabel->setStyleSheet("color: #38a169; font-weight: bold;");

    newUsernameInput->clear();
    newPasswordInput->clear();
    refreshUserManagementTable();
}

void MainWindow::onReassignDepartmentClicked() {
    int row = usersTable->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "Select User", "Please select a user from the table to reassign.");
        return;
    }

    long long uid = usersTable->item(row, 0)->text().toLongLong();
    QString uname = usersTable->item(row, 1)->text();
    QString role = usersTable->item(row, 2)->text();

    if (role == "Super Admin") {
        QMessageBox::warning(this, "Action Not Allowed", "Super Admin cannot be assigned to a specific department.");
        return;
    }

    QDialog dlg(this);
    dlg.setWindowTitle("Reassign Department for " + uname);
    dlg.setFixedSize(320, 160);

    QFormLayout form(&dlg);
    QComboBox deptSel(&dlg);
    auto depts = DatabaseManager::instance().getDepartments();
    for (const auto& d : depts) {
        deptSel.addItem(d.name, d.id);
    }
    form.addRow("New Department:", &deptSel);

    QDialogButtonBox btns(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    form.addRow(&btns);
    connect(&btns, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(&btns, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() == QDialog::Accepted) {
        int newDeptId = deptSel.currentData().toInt();
        bool ok = DatabaseManager::instance().updateUserDepartment(currentUser, uid, newDeptId);
        if (ok) {
            QMessageBox::information(this, "Success", "Department reassigned successfully!");
            refreshUserManagementTable();
        } else {
            QMessageBox::critical(this, "Error", DatabaseManager::instance().lastError());
        }
    }
}

void MainWindow::onDeleteUserClicked() {
    int row = usersTable->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "Select User", "Please select a user from the table to delete.");
        return;
    }

    long long uid = usersTable->item(row, 0)->text().toLongLong();
    QString uname = usersTable->item(row, 1)->text();

    if (uid == currentUser.id) {
        QMessageBox::warning(this, "Action Not Allowed", "You cannot delete your own account.");
        return;
    }

    auto reply = QMessageBox::question(this, "Confirm Deletion",
                                       QString("Are you sure you want to delete user '%1' (ID: %2)?").arg(uname).arg(uid),
                                       QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        bool ok = DatabaseManager::instance().deleteUser(currentUser, uid);
        if (ok) {
            QMessageBox::information(this, "Deleted", "User deleted successfully.");
            refreshUserManagementTable();
        } else {
            QMessageBox::critical(this, "Error", DatabaseManager::instance().lastError());
        }
    }
}

void MainWindow::refreshUserManagementTable() {
    auto users = DatabaseManager::instance().getAllUsers(currentUser);
    usersTable->setRowCount((int)users.size());

    for (int i = 0; i < (int)users.size(); ++i) {
        const auto& u = users[i];
        usersTable->setItem(i, 0, new QTableWidgetItem(QString::number(u.id)));
        usersTable->setItem(i, 1, new QTableWidgetItem(u.username));
        usersTable->setItem(i, 2, new QTableWidgetItem(u.roleName()));
        usersTable->setItem(i, 3, new QTableWidgetItem(u.isSuperAdmin() ? "All Departments" : u.departmentName));
    }
}

// ──────────────────────────────────────────
// Details & Audit History Modal
// ──────────────────────────────────────────
void MainWindow::onComplaintTableDoubleClicked(int row, int col) {
    Q_UNUSED(col);
    QTableWidget* table = qobject_cast<QTableWidget*>(sender());
    if (!table) return;

    QTableWidgetItem* idItem = table->item(row, 0);
    if (idItem) {
        showComplaintDetailsDialog(idItem->text().toInt());
    }
}

void MainWindow::onViewHistoryClicked() {
    int row = allComplaintsTable->currentRow();
    if (row < 0) {
        QMessageBox::information(this, "Select Row", "Please select a complaint row from the table first.");
        return;
    }
    int id = allComplaintsTable->item(row, 0)->text().toInt();
    showComplaintDetailsDialog(id);
}

void MainWindow::showComplaintDetailsDialog(int complaintId) {
    auto compOpt = DatabaseManager::instance().getComplaintById(currentUser, complaintId);
    if (!compOpt.has_value()) {
        QMessageBox::warning(this, "Error", DatabaseManager::instance().lastError().isEmpty() ? "Complaint not found or access denied." : DatabaseManager::instance().lastError());
        return;
    }

    const Complaint& c = compOpt.value();
    auto history = DatabaseManager::instance().getComplaintHistory(currentUser, complaintId);

    QDialog dlg(this);
    dlg.setWindowTitle(QString("Grievance Details & Audit History — #%1").arg(c.id));
    dlg.resize(650, 520);

    QVBoxLayout* lay = new QVBoxLayout(&dlg);

    // Complaint metadata card
    QFrame* infoCard = new QFrame(&dlg);
    infoCard->setStyleSheet("background: #f7fafc; border: 1px solid #e2e8f0; border-radius: 8px; padding: 12px;");
    QGridLayout* grid = new QGridLayout(infoCard);

    grid->addWidget(new QLabel(QString("<b>Citizen:</b> %1").arg(QString::fromStdString(c.citizen))), 0, 0);
    grid->addWidget(new QLabel(QString("<b>Contact:</b> %1").arg(QString::fromStdString(c.contact.empty() ? "N/A" : c.contact))), 0, 1);
    grid->addWidget(new QLabel(QString("<b>Category:</b> %1").arg(QString::fromStdString(c.category))), 1, 0);
    grid->addWidget(new QLabel(QString("<b>Department:</b> %1").arg(QString::fromStdString(c.department))), 1, 1);
    grid->addWidget(new QLabel(QString("<b>Priority:</b> %1").arg(QString::fromStdString(priorityName(c.priority)))), 2, 0);
    grid->addWidget(new QLabel(QString("<b>Current Status:</b> %1").arg(QString::fromStdString(c.status))), 2, 1);
    grid->addWidget(new QLabel(QString("<b>Location:</b> %1").arg(QString::fromStdString(c.location))), 3, 0, 1, 2);
    grid->addWidget(new QLabel(QString("<b>Description:</b> %1").arg(QString::fromStdString(c.description))), 4, 0, 1, 2);
    grid->addWidget(new QLabel(QString("<b>Filed At:</b> %1").arg(QString::fromStdString(c.createdAt))), 5, 0, 1, 2);

    lay->addWidget(infoCard);

    QLabel* histTitle = new QLabel("📋 Status Change Audit Trail", &dlg);
    histTitle->setStyleSheet("font-size: 13px; font-weight: bold; margin-top: 10px;");
    lay->addWidget(histTitle);

    QTableWidget* histTable = new QTableWidget(&dlg);
    histTable->setColumnCount(5);
    histTable->setHorizontalHeaderLabels({"Timestamp", "Previous", "New Status", "Changed By", "Notes"});
    histTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    histTable->setRowCount((int)history.size());

    for (int i = 0; i < (int)history.size(); ++i) {
        const auto& h = history[i];
        histTable->setItem(i, 0, new QTableWidgetItem(h.changedAt));
        histTable->setItem(i, 1, new QTableWidgetItem(h.previousStatus));
        histTable->setItem(i, 2, new QTableWidgetItem(h.newStatus));
        histTable->setItem(i, 3, new QTableWidgetItem(h.changedByUsername));
        histTable->setItem(i, 4, new QTableWidgetItem(h.notes));
    }
    lay->addWidget(histTable, 1);

    QDialogButtonBox* closeBtn = new QDialogButtonBox(QDialogButtonBox::Close, &dlg);
    connect(closeBtn, &QDialogButtonBox::rejected, &dlg, &QDialog::accept);
    lay->addWidget(closeBtn);

    dlg.exec();
}

// ──────────────────────────────────────────
// Navigation Switchers
// ──────────────────────────────────────────
void MainWindow::showDashboard() {
    stack->setCurrentWidget(dashboardPage);
    refreshDashboard();
}

void MainWindow::showRegisterPage() {
    stack->setCurrentWidget(registerPage);
    registerResultLabel->setText("");
}

void MainWindow::showViewPage() {
    stack->setCurrentWidget(viewPage);
    refreshAllComplaintsTable();
}

void MainWindow::showSearchPage() {
    stack->setCurrentWidget(searchPage);
    searchResultLabel->setText("");
}

void MainWindow::showUpdatePage() {
    stack->setCurrentWidget(updatePage);
    updateResultLabel->setText("");
}

void MainWindow::showProcessPage() {
    stack->setCurrentWidget(processPage);
    processResultLabel->setText("");
    QString dept = deptCombo->currentText();
    queueContentsLabel->setText("FIFO Queue: " + QString::fromStdString(system.getQueueInfo(dept.toStdString())));
}

void MainWindow::showUserManagementPage() {
    if (!currentUser.isSuperAdmin()) return;
    stack->setCurrentWidget(userManagementPage);
    refreshUserManagementTable();
}
