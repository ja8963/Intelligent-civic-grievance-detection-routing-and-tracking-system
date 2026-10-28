#include "mainwindow.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QScrollArea>
#include <QHeaderView>
#include <QMessageBox>
#include <QSplitter>
#include <QSpacerItem>
#include <QSizePolicy>
#include <QString>
#include <QFont>

// ─────────────────────────────────────────
// Global application stylesheet
// ─────────────────────────────────────────
static const char* APP_STYLE = R"(
QMainWindow { background-color: #f0f4f8; }
QWidget      { color: #111111; }

QWidget#sidebar {
    background-color: #1565C0;
    min-width: 200px;
}

QPushButton#navBtn {
    background-color: transparent;
    color: white;
    text-align: left;
    padding: 12px 20px;
    border: none;
    font-size: 14px;
    border-radius: 0px;
}
QPushButton#navBtn:hover   { background-color: #1976D2; }
QPushButton#navBtn:checked { background-color: #0D47A1; font-weight: bold; }

QLabel#pageTitle {
    font-size: 22px;
    font-weight: bold;
    color: #1565C0;
}

QTableWidget {
    background-color: white;
    border: 1px solid #e0e0e0;
    gridline-color: #e0e0e0;
    font-size: 13px;
}
QHeaderView::section {
    background-color: #1565C0;
    color: white;
    padding: 8px;
    font-weight: bold;
    border: none;
}

QPushButton {
    background-color: #1976D2;
    color: white;
    border: none;
    padding: 8px 18px;
    border-radius: 4px;
    font-size: 13px;
}
QPushButton:hover   { background-color: #1565C0; }
QPushButton:pressed { background-color: #0D47A1; }

QLineEdit, QComboBox {
    border: 1px solid #bdbdbd;
    border-radius: 4px;
    padding: 6px;
    font-size: 13px;
    background: white;
    color: #111111;
}
QLineEdit:focus, QComboBox:focus { border: 2px solid #1976D2; color: #111111; }
QComboBox QAbstractItemView { color: #111111; background: white; }

QScrollArea { border: none; background: transparent; }

QMessageBox {
    background-color: #ffffff;
    color: #111111;
}
QMessageBox QLabel {
    color: #111111;
    background-color: transparent;
    font-size: 13px;
}
QMessageBox QPushButton {
    background-color: #1976D2;
    color: white;
    border: none;
    border-radius: 4px;
    padding: 8px 24px;
    font-size: 13px;
    min-width: 80px;
}
QMessageBox QPushButton:hover { background-color: #1565C0; }
)";

// ─────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────
MainWindow::MainWindow(UserRole role, QWidget* parent)
    : QMainWindow(parent), currentRole(role)
{
    // Apply the stylesheet to the whole application
    qApp->setStyleSheet(APP_STYLE);

    // Central widget: sidebar + stacked pages side by side
    QWidget*     central  = new QWidget(this);
    QHBoxLayout* rootLay  = new QHBoxLayout(central);
    rootLay->setContentsMargins(0, 0, 0, 0);
    rootLay->setSpacing(0);

    // Build navigation sidebar
    QWidget* nav = createNavBar();
    rootLay->addWidget(nav);

    // Build all pages
    dashboardPage = buildDashboardPage();
    registerPage  = buildRegisterPage();
    viewPage      = buildViewPage();
    searchPage    = buildSearchPage();
    updatePage    = buildUpdatePage();
    processPage   = buildProcessPage();

    // Stack widget holds all pages
    stack = new QStackedWidget(this);
    stack->addWidget(dashboardPage);   // index 0
    stack->addWidget(registerPage);    // index 1
    stack->addWidget(viewPage);        // index 2
    stack->addWidget(searchPage);      // index 3
    stack->addWidget(updatePage);      // index 4
    stack->addWidget(processPage);     // index 5

    rootLay->addWidget(stack, 1);

    setCentralWidget(central);

    // Start on the correct page depending on role
    if (currentRole == UserRole::Citizen) {
        showRegisterPage();
    } else if (currentRole == UserRole::Admin) {
        showViewPage();
    } else {
        showDashboard();  // SuperAdmin starts on dashboard
    }
}

// ─────────────────────────────────────────
// createNavBar – left blue sidebar with nav buttons
// ─────────────────────────────────────────
QWidget* MainWindow::createNavBar() {
    QWidget*     sidebar = new QWidget(this);
    sidebar->setObjectName("sidebar");
    sidebar->setMinimumWidth(210);
    sidebar->setMaximumWidth(210);

    QVBoxLayout* lay = new QVBoxLayout(sidebar);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);

    // App title at the top of the sidebar
    QLabel* appTitle = new QLabel("🏛  Civic Grievance\nManagement", sidebar);
    appTitle->setStyleSheet("color: white; font-size: 15px; font-weight: bold; "
                            "padding: 20px 14px 16px 14px;");
    appTitle->setWordWrap(true);
    lay->addWidget(appTitle);

    // Show which role is logged in
    QString roleText;
    if (currentRole == UserRole::Citizen)       roleText = "👤 Citizen";
    else if (currentRole == UserRole::Admin)     roleText = "🛠️ Admin";
    else                                         roleText = "⭐ Super Admin";

    QLabel* roleBadge = new QLabel(roleText, sidebar);
    roleBadge->setStyleSheet(
        "color: #90CAF9; font-size: 11px; padding: 0px 14px 14px 14px;");
    lay->addWidget(roleBadge);

    // Thin divider line
    QFrame* line = new QFrame(sidebar);
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet("background-color: #1976D2; margin: 0 10px;");
    line->setFixedHeight(1);
    lay->addWidget(line);
    lay->addSpacing(6);

    // Helper to create a nav button
    auto makeBtn = [&](const QString& icon, const QString& label) -> QPushButton* {
        QPushButton* btn = new QPushButton(icon + "  " + label, sidebar);
        btn->setObjectName("navBtn");
        btn->setCheckable(true);
        btn->setAutoExclusive(true);
        btn->setMinimumHeight(48);
        btn->setCursor(Qt::PointingHandCursor);
        lay->addWidget(btn);
        return btn;
    };

    // Create all nav buttons
    QPushButton* btnDash    = makeBtn("📊", "Dashboard");
    QPushButton* btnReg     = makeBtn("📝", "Register Complaint");
    QPushButton* btnView    = makeBtn("📋", "View All Complaints");
    QPushButton* btnSearch  = makeBtn("🔍", "Search");
    QPushButton* btnUpdate  = makeBtn("✏️",  "Update Status");
    QPushButton* btnProcess = makeBtn("⚙️",  "Process Complaints");

    // ── Role-based visibility ──────────────────────────────
    // Citizen: can only Register and Search
    // Admin:   can View All, Search, Update, Process (no Dashboard)
    // SuperAdmin: everything
    if (currentRole == UserRole::Citizen) {
        btnDash->setVisible(false);
        btnView->setVisible(false);
        btnUpdate->setVisible(false);
        btnProcess->setVisible(false);
    } else if (currentRole == UserRole::Admin) {
        btnDash->setVisible(false);
    }
    // SuperAdmin sees all buttons

    lay->addStretch();  // push buttons to top

    // ── Connect buttons to show* slots ────────────────────
    connect(btnDash,    &QPushButton::clicked, this, &MainWindow::showDashboard);
    connect(btnReg,     &QPushButton::clicked, this, &MainWindow::showRegisterPage);
    connect(btnView,    &QPushButton::clicked, this, &MainWindow::showViewPage);
    connect(btnSearch,  &QPushButton::clicked, this, &MainWindow::showSearchPage);
    connect(btnUpdate,  &QPushButton::clicked, this, &MainWindow::showUpdatePage);
    connect(btnProcess, &QPushButton::clicked, this, &MainWindow::showProcessPage);

    // Choose the default selected button depending on role
    if (currentRole == UserRole::Citizen) {
        btnReg->setChecked(true);
    } else if (currentRole == UserRole::Admin) {
        btnView->setChecked(true);
    } else {
        btnDash->setChecked(true);
    }

    return sidebar;
}

// ─────────────────────────────────────────
// makeStatCard – coloured box showing a big number + title
// ─────────────────────────────────────────
QFrame* MainWindow::makeStatCard(const QString& title, QLabel*& numLabel, const QString& colour) {
    QFrame* card = new QFrame;
    card->setStyleSheet(QString("QFrame { background: %1; border-radius: 10px; }").arg(colour));
    card->setMinimumSize(160, 110);

    QVBoxLayout* lay = new QVBoxLayout(card);
    lay->setAlignment(Qt::AlignCenter);

    numLabel = new QLabel("0", card);
    numLabel->setStyleSheet("color: white; font-size: 36px; font-weight: bold;");
    numLabel->setAlignment(Qt::AlignCenter);

    QLabel* titleLbl = new QLabel(title, card);
    titleLbl->setStyleSheet("color: white; font-size: 13px;");
    titleLbl->setAlignment(Qt::AlignCenter);

    lay->addWidget(numLabel);
    lay->addWidget(titleLbl);
    return card;
}

// ─────────────────────────────────────────
// buildDashboardPage
// ─────────────────────────────────────────
QWidget* MainWindow::buildDashboardPage() {
    QWidget*     page = new QWidget;
    QVBoxLayout* lay  = new QVBoxLayout(page);
    lay->setContentsMargins(28, 24, 28, 24);
    lay->setSpacing(20);

    QLabel* title = new QLabel("Dashboard", page);
    title->setObjectName("pageTitle");
    lay->addWidget(title);

    // ── Stat cards row ──
    QHBoxLayout* cardRow = new QHBoxLayout;
    cardRow->setSpacing(20);

    QFrame* totalCard    = makeStatCard("Total Complaints", totalLabel,    "#1565C0");
    QFrame* pendingCard  = makeStatCard("Pending",          pendingLabel,  "#E65100");
    QFrame* resolvedCard = makeStatCard("Resolved",         resolvedLabel, "#2E7D32");

    cardRow->addWidget(totalCard);
    cardRow->addWidget(pendingCard);
    cardRow->addWidget(resolvedCard);
    cardRow->addStretch();
    lay->addLayout(cardRow);

    // ── Recent complaints table ──
    QLabel* recentTitle = new QLabel("Recent Complaints (latest 5)", page);
    recentTitle->setStyleSheet("font-size: 16px; font-weight: bold; color: #333;");
    lay->addWidget(recentTitle);

    recentTable = new QTableWidget(0, 6, page);
    recentTable->setHorizontalHeaderLabels({"ID", "Citizen", "Category", "Department", "Priority", "Status"});
    recentTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    recentTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    recentTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    recentTable->setAlternatingRowColors(true);
    recentTable->verticalHeader()->setVisible(false);
    lay->addWidget(recentTable);

    return page;
}

// ─────────────────────────────────────────
// buildRegisterPage
// ─────────────────────────────────────────
QWidget* MainWindow::buildRegisterPage() {
    QWidget*     page = new QWidget;
    QVBoxLayout* lay  = new QVBoxLayout(page);
    lay->setContentsMargins(28, 24, 28, 24);
    lay->setSpacing(16);

    QLabel* title = new QLabel("Register a New Complaint", page);
    title->setObjectName("pageTitle");
    lay->addWidget(title);

    // Scroll area so small windows still work
    QScrollArea* scroll = new QScrollArea(page);
    scroll->setWidgetResizable(true);

    QWidget*     inner  = new QWidget;
    QFormLayout* form   = new QFormLayout(inner);
    form->setSpacing(14);
    form->setContentsMargins(0, 0, 40, 0);
    form->setLabelAlignment(Qt::AlignRight);

    nameInput = new QLineEdit;
    nameInput->setPlaceholderText("Enter citizen full name");

    categoryCombo = new QComboBox;
    categoryCombo->addItems({"Pothole", "Garbage", "Street Light", "Water Leakage", "Drainage"});

    descInput = new QLineEdit;
    descInput->setPlaceholderText("Briefly describe the issue");

    locationInput = new QLineEdit;
    locationInput->setPlaceholderText("Address or landmark");

    priorityCombo = new QComboBox;
    priorityCombo->addItems({"Low", "Medium", "High"});
    priorityCombo->setCurrentIndex(1); // default Medium

    QPushButton* registerBtn = new QPushButton("  Register Complaint  ");
    registerBtn->setMinimumHeight(40);

    form->addRow("Citizen Name *",  nameInput);
    form->addRow("Category *",      categoryCombo);
    form->addRow("Description",     descInput);
    form->addRow("Location",        locationInput);
    form->addRow("Priority",        priorityCombo);
    form->addRow("",                registerBtn);

    scroll->setWidget(inner);
    lay->addWidget(scroll);

    connect(registerBtn, &QPushButton::clicked, this, &MainWindow::onRegisterClicked);

    return page;
}

// ─────────────────────────────────────────
// buildViewPage
// ─────────────────────────────────────────
QWidget* MainWindow::buildViewPage() {
    QWidget*     page = new QWidget;
    QVBoxLayout* lay  = new QVBoxLayout(page);
    lay->setContentsMargins(28, 24, 28, 24);
    lay->setSpacing(16);

    QLabel* title = new QLabel("All Complaints", page);
    title->setObjectName("pageTitle");
    lay->addWidget(title);

    QPushButton* refreshBtn = new QPushButton("🔄  Refresh");
    refreshBtn->setMaximumWidth(140);
    lay->addWidget(refreshBtn);

    allComplaintsTable = new QTableWidget(0, 8, page);
    allComplaintsTable->setHorizontalHeaderLabels(
        {"ID", "Citizen Name", "Category", "Department", "Priority", "Status", "Description", "Location"});
    allComplaintsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    allComplaintsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    allComplaintsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    allComplaintsTable->setAlternatingRowColors(true);
    allComplaintsTable->verticalHeader()->setVisible(false);
    lay->addWidget(allComplaintsTable);

    connect(refreshBtn, &QPushButton::clicked, this, &MainWindow::refreshAllComplaintsTable);

    return page;
}

// ─────────────────────────────────────────
// buildSearchPage
// ─────────────────────────────────────────
QWidget* MainWindow::buildSearchPage() {
    QWidget*     page = new QWidget;
    QVBoxLayout* lay  = new QVBoxLayout(page);
    lay->setContentsMargins(28, 24, 28, 24);
    lay->setSpacing(16);

    QLabel* title = new QLabel("Search Complaint by ID", page);
    title->setObjectName("pageTitle");
    lay->addWidget(title);

    // Search bar row
    QHBoxLayout* row = new QHBoxLayout;
    searchIdInput = new QLineEdit;
    searchIdInput->setPlaceholderText("Enter Complaint ID (e.g. 1001)");
    searchIdInput->setMaximumWidth(300);

    QPushButton* searchBtn = new QPushButton("Search");
    searchBtn->setMaximumWidth(100);

    row->addWidget(searchIdInput);
    row->addWidget(searchBtn);
    row->addStretch();
    lay->addLayout(row);

    // Result display
    searchResultLabel = new QLabel("Enter an ID above and click Search.", page);
    searchResultLabel->setStyleSheet("background: white; border: 1px solid #e0e0e0; "
                                     "border-radius: 6px; padding: 16px; font-size: 13px; color: #333;");
    searchResultLabel->setWordWrap(true);
    searchResultLabel->setMinimumHeight(220);
    searchResultLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    searchResultLabel->setTextFormat(Qt::RichText);
    lay->addWidget(searchResultLabel);
    lay->addStretch();

    connect(searchBtn,    &QPushButton::clicked, this, &MainWindow::onSearchClicked);
    connect(searchIdInput, &QLineEdit::returnPressed, this, &MainWindow::onSearchClicked);

    return page;
}

// ─────────────────────────────────────────
// buildUpdatePage
// ─────────────────────────────────────────
QWidget* MainWindow::buildUpdatePage() {
    QWidget*     page = new QWidget;
    QVBoxLayout* lay  = new QVBoxLayout(page);
    lay->setContentsMargins(28, 24, 28, 24);
    lay->setSpacing(16);

    QLabel* title = new QLabel("Update Complaint Status", page);
    title->setObjectName("pageTitle");
    lay->addWidget(title);

    QFormLayout* form = new QFormLayout;
    form->setSpacing(14);
    form->setLabelAlignment(Qt::AlignRight);
    form->setContentsMargins(0, 0, 40, 0);

    updateIdInput = new QLineEdit;
    updateIdInput->setPlaceholderText("e.g. 1001");
    updateIdInput->setMaximumWidth(220);

    statusCombo = new QComboBox;
    statusCombo->addItems({"Registered", "In Progress", "Urgent Processing", "Resolved", "Closed"});
    statusCombo->setMaximumWidth(280);

    QPushButton* updateBtn = new QPushButton("  Update Status  ");
    updateBtn->setMaximumWidth(180);
    updateBtn->setMinimumHeight(38);

    form->addRow("Complaint ID *", updateIdInput);
    form->addRow("New Status *",   statusCombo);
    form->addRow("",               updateBtn);

    lay->addLayout(form);

    updateResultLabel = new QLabel("", page);
    updateResultLabel->setStyleSheet("font-size: 13px; color: #1565C0; padding: 6px;");
    lay->addWidget(updateResultLabel);
    lay->addStretch();

    connect(updateBtn, &QPushButton::clicked, this, &MainWindow::onUpdateClicked);

    return page;
}

// ─────────────────────────────────────────
// buildProcessPage
// ─────────────────────────────────────────
QWidget* MainWindow::buildProcessPage() {
    QWidget*     page = new QWidget;
    QVBoxLayout* lay  = new QVBoxLayout(page);
    lay->setContentsMargins(28, 24, 28, 24);
    lay->setSpacing(20);

    QLabel* title = new QLabel("Process Complaints", page);
    title->setObjectName("pageTitle");
    lay->addWidget(title);

    // ── Section 1: FIFO ──
    QFrame* fifoCard = new QFrame;
    fifoCard->setStyleSheet("QFrame { background: white; border-radius: 8px; "
                            "border: 1px solid #e0e0e0; padding: 10px; }");
    QVBoxLayout* fifoLay = new QVBoxLayout(fifoCard);

    QLabel* fifoTitle = new QLabel("Process by Department (FIFO Queue)", fifoCard);
    fifoTitle->setStyleSheet("font-size: 15px; font-weight: bold; color: #1565C0;");
    fifoLay->addWidget(fifoTitle);

    QHBoxLayout* fifoRow = new QHBoxLayout;
    deptCombo = new QComboBox;
    deptCombo->addItems({"Road", "Sanitation", "Electrical", "Water", "Drainage", "General"});
    deptCombo->setMaximumWidth(200);

    QPushButton* processBtn = new QPushButton("Process Next (FIFO)");
    fifoRow->addWidget(new QLabel("Department:"));
    fifoRow->addWidget(deptCombo);
    fifoRow->addWidget(processBtn);
    fifoRow->addStretch();
    fifoLay->addLayout(fifoRow);

    lay->addWidget(fifoCard);

    // ── Section 2: Heap ──
    QFrame* heapCard = new QFrame;
    heapCard->setStyleSheet("QFrame { background: white; border-radius: 8px; "
                            "border: 1px solid #e0e0e0; padding: 10px; }");
    QVBoxLayout* heapLay = new QVBoxLayout(heapCard);

    QLabel* heapTitle = new QLabel("Process Highest Priority (Max-Heap)", heapCard);
    heapTitle->setStyleSheet("font-size: 15px; font-weight: bold; color: #E65100;");
    heapLay->addWidget(heapTitle);

    QLabel* heapDesc = new QLabel("Removes and processes the complaint with the highest priority "
                                   "(High > Medium > Low) from the urgent queue.", heapCard);
    heapDesc->setWordWrap(true);
    heapDesc->setStyleSheet("color: #555; font-size: 12px;");
    heapLay->addWidget(heapDesc);

    QPushButton* urgentBtn = new QPushButton("Process Highest Priority (Heap)");
    urgentBtn->setStyleSheet("background-color: #E65100;");
    heapLay->addWidget(urgentBtn);

    lay->addWidget(heapCard);

    // ── Result label ──
    processResultLabel = new QLabel("", page);
    processResultLabel->setStyleSheet("background: white; border: 1px solid #e0e0e0; "
                                      "border-radius: 6px; padding: 14px; font-size: 13px; color: #333;");
    processResultLabel->setWordWrap(true);
    processResultLabel->setMinimumHeight(100);
    processResultLabel->setTextFormat(Qt::RichText);
    lay->addWidget(processResultLabel);
    lay->addStretch();

    connect(processBtn, &QPushButton::clicked, this, &MainWindow::onProcessDeptClicked);
    connect(urgentBtn,  &QPushButton::clicked, this, &MainWindow::onProcessUrgentClicked);

    return page;
}

// ─────────────────────────────────────────
// Navigation slots
// ─────────────────────────────────────────
void MainWindow::showDashboard() {
    refreshDashboard();
    stack->setCurrentIndex(0);
}

void MainWindow::showRegisterPage() {
    stack->setCurrentIndex(1);
}

void MainWindow::showViewPage() {
    refreshAllComplaintsTable();
    stack->setCurrentIndex(2);
}

void MainWindow::showSearchPage() {
    stack->setCurrentIndex(3);
}

void MainWindow::showUpdatePage() {
    stack->setCurrentIndex(4);
}

void MainWindow::showProcessPage() {
    stack->setCurrentIndex(5);
}

// ─────────────────────────────────────────
// refreshDashboard – update stat cards + recent table
// ─────────────────────────────────────────
void MainWindow::refreshDashboard() {
    totalLabel->setText(QString::number(system.getTotalCount()));
    pendingLabel->setText(QString::number(system.getPendingCount()));
    resolvedLabel->setText(QString::number(system.getResolvedCount()));

    // Populate the recent-5 table
    auto all = system.getAllComplaints();
    recentTable->setRowCount(0);

    // Take up to last 5 entries
    int start = (int)all.size() > 5 ? (int)all.size() - 5 : 0;
    for (int i = (int)all.size() - 1; i >= start; --i) {
        Complaint* c = all[i];
        int row = recentTable->rowCount();
        recentTable->insertRow(row);
        recentTable->setItem(row, 0, new QTableWidgetItem(QString::number(c->id)));
        recentTable->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(c->citizen)));
        recentTable->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(c->category)));
        recentTable->setItem(row, 3, new QTableWidgetItem(QString::fromStdString(c->department)));
        recentTable->setItem(row, 4, new QTableWidgetItem(QString::fromStdString(priorityName(c->priority))));
        recentTable->setItem(row, 5, new QTableWidgetItem(QString::fromStdString(c->status)));
    }
}

// ─────────────────────────────────────────
// refreshAllComplaintsTable
// ─────────────────────────────────────────
void MainWindow::refreshAllComplaintsTable() {
    auto all = system.getAllComplaints();
    allComplaintsTable->setRowCount(0);

    for (Complaint* c : all) {
        int row = allComplaintsTable->rowCount();
        allComplaintsTable->insertRow(row);
        allComplaintsTable->setItem(row, 0, new QTableWidgetItem(QString::number(c->id)));
        allComplaintsTable->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(c->citizen)));
        allComplaintsTable->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(c->category)));
        allComplaintsTable->setItem(row, 3, new QTableWidgetItem(QString::fromStdString(c->department)));
        allComplaintsTable->setItem(row, 4, new QTableWidgetItem(QString::fromStdString(priorityName(c->priority))));
        allComplaintsTable->setItem(row, 5, new QTableWidgetItem(QString::fromStdString(c->status)));
        allComplaintsTable->setItem(row, 6, new QTableWidgetItem(QString::fromStdString(c->description)));
        allComplaintsTable->setItem(row, 7, new QTableWidgetItem(QString::fromStdString(c->location)));
    }
}

// ─────────────────────────────────────────
// onRegisterClicked
// ─────────────────────────────────────────
void MainWindow::onRegisterClicked() {
    QString name     = nameInput->text().trimmed();
    QString category = categoryCombo->currentText();
    QString desc     = descInput->text().trimmed();
    QString location = locationInput->text().trimmed();
    QString prioStr  = priorityCombo->currentText();

    // Validate required fields
    if (name.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Citizen name is required.");
        nameInput->setFocus();
        return;
    }
    if (category.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Please select a category.");
        return;
    }

    // Map priority combo text to int
    int priority = 2; // Medium default
    if (prioStr == "Low")    priority = 1;
    if (prioStr == "Medium") priority = 2;
    if (prioStr == "High")   priority = 3;

    int    newId   = 0;
    std::string newDept;

    bool ok = system.registerComplaint(
        name.toStdString(),
        category.toStdString(),
        desc.toStdString(),
        location.toStdString(),
        priority,
        &newId,
        &newDept
    );

    if (ok) {
        QMessageBox::information(this, "Complaint Registered",
            QString("✅ Complaint registered successfully!\n\n"
                    "Complaint ID : %1\n"
                    "Department   : %2\n\n"
                    "Please note your Complaint ID for future reference.")
                .arg(newId)
                .arg(QString::fromStdString(newDept)));

        // Clear form
        nameInput->clear();
        descInput->clear();
        locationInput->clear();
        categoryCombo->setCurrentIndex(0);
        priorityCombo->setCurrentIndex(1);

        refreshDashboard();
    } else {
        QMessageBox::critical(this, "Error", "Failed to register complaint. Please try again.");
    }
}

// ─────────────────────────────────────────
// onSearchClicked
// ─────────────────────────────────────────
void MainWindow::onSearchClicked() {
    QString input = searchIdInput->text().trimmed();
    if (input.isEmpty()) {
        QMessageBox::warning(this, "Input Required", "Please enter a Complaint ID.");
        return;
    }

    bool numOk = false;
    int  id    = input.toInt(&numOk);
    if (!numOk) {
        QMessageBox::warning(this, "Invalid Input", "Complaint ID must be a number.");
        return;
    }

    Complaint* c = system.searchById(id);
    if (!c) {
        searchResultLabel->setText(
            "<span style='color:#c62828; font-size:14px;'>❌ Complaint not found.</span><br>"
            "<span style='color:#555;'>No complaint with ID " + input + " exists in the system.</span>"
        );
        return;
    }

    // Format the result as rich HTML
    searchResultLabel->setText(
        QString("<b style='font-size:16px; color:#1565C0;'>Complaint #%1</b><br><br>"
                "<table style='font-size:13px; line-height:1.8;'>"
                "<tr><td><b>Citizen&nbsp;&nbsp;&nbsp;</b></td><td>%2</td></tr>"
                "<tr><td><b>Category&nbsp;&nbsp;</b></td><td>%3</td></tr>"
                "<tr><td><b>Department</b></td><td>%4</td></tr>"
                "<tr><td><b>Priority&nbsp;&nbsp;</b></td><td>%5</td></tr>"
                "<tr><td><b>Status&nbsp;&nbsp;&nbsp;&nbsp;</b></td><td>%6</td></tr>"
                "<tr><td><b>Location&nbsp;&nbsp;</b></td><td>%7</td></tr>"
                "<tr><td><b>Description</b></td><td>%8</td></tr>"
                "</table>")
            .arg(c->id)
            .arg(QString::fromStdString(c->citizen))
            .arg(QString::fromStdString(c->category))
            .arg(QString::fromStdString(c->department))
            .arg(QString::fromStdString(priorityName(c->priority)))
            .arg(QString::fromStdString(c->status))
            .arg(QString::fromStdString(c->location))
            .arg(QString::fromStdString(c->description))
    );
}

// ─────────────────────────────────────────
// onUpdateClicked
// ─────────────────────────────────────────
void MainWindow::onUpdateClicked() {
    QString input = updateIdInput->text().trimmed();
    if (input.isEmpty()) {
        QMessageBox::warning(this, "Input Required", "Please enter a Complaint ID.");
        return;
    }

    bool numOk = false;
    int  id    = input.toInt(&numOk);
    if (!numOk) {
        QMessageBox::warning(this, "Invalid Input", "Complaint ID must be a number.");
        return;
    }

    QString newStatus = statusCombo->currentText();
    bool ok = system.updateStatus(id, newStatus.toStdString());

    if (ok) {
        updateResultLabel->setStyleSheet("font-size:13px; color:#2E7D32; padding:6px;");
        updateResultLabel->setText(
            QString("✅ Status of Complaint #%1 updated to '%2' successfully.").arg(id).arg(newStatus));
        refreshDashboard();
    } else {
        updateResultLabel->setStyleSheet("font-size:13px; color:#c62828; padding:6px;");
        updateResultLabel->setText(
            QString("❌ Complaint #%1 not found. Please check the ID.").arg(id));
    }
}

// ─────────────────────────────────────────
// onProcessDeptClicked – FIFO dequeue
// ─────────────────────────────────────────
void MainWindow::onProcessDeptClicked() {
    QString dept = deptCombo->currentText();
    Complaint out;
    bool ok = system.processNextFromDept(dept.toStdString(), out);

    if (ok) {
        processResultLabel->setText(
            QString("<b style='color:#2E7D32;'>✅ Processed (FIFO) from %1 department:</b><br><br>"
                    "Complaint ID : <b>%2</b><br>"
                    "Citizen      : %3<br>"
                    "Category     : %4<br>"
                    "Priority     : %5<br>"
                    "New Status   : <b>Resolved</b>")
                .arg(dept)
                .arg(out.id)
                .arg(QString::fromStdString(out.citizen))
                .arg(QString::fromStdString(out.category))
                .arg(QString::fromStdString(priorityName(out.priority)))
        );
        refreshDashboard();
    } else {
        processResultLabel->setText(
            QString("<span style='color:#c62828;'>❌ No pending complaints in the <b>%1</b> "
                    "department queue.</span>").arg(dept));
    }
}

// ─────────────────────────────────────────
// onProcessUrgentClicked – max-heap pop
// ─────────────────────────────────────────
void MainWindow::onProcessUrgentClicked() {
    Complaint out;
    bool ok = system.processUrgent(out);

    if (ok) {
        processResultLabel->setText(
            QString("<b style='color:#E65100;'>⚡ Processed Highest Priority Complaint:</b><br><br>"
                    "Complaint ID : <b>%1</b><br>"
                    "Citizen      : %2<br>"
                    "Category     : %3<br>"
                    "Department   : %4<br>"
                    "Priority     : %5<br>"
                    "New Status   : <b>Urgent Processing</b>")
                .arg(out.id)
                .arg(QString::fromStdString(out.citizen))
                .arg(QString::fromStdString(out.category))
                .arg(QString::fromStdString(out.department))
                .arg(QString::fromStdString(priorityName(out.priority)))
        );
        refreshDashboard();
    } else {
        processResultLabel->setText(
            "<span style='color:#c62828;'>❌ The urgent priority queue is empty. "
            "Register some HIGH priority complaints first.</span>");
    }
}
