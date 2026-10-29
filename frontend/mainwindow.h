#pragma once

#include <QMainWindow>
#include <QStackedWidget>
#include <QTableWidget>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QTextEdit>
#include <QFrame>
#include <QPushButton>

#include "complaint.h"
#include "user.h"
#include "databasemanager.h"

// ──────────────────────────────────────────
// Main application window
// ──────────────────────────────────────────
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(const User& user, QWidget* parent = nullptr);
    ~MainWindow() override = default;

private slots:
    // Navigation slots
    void showDashboard();
    void showRegisterPage();
    void showViewPage();
    void showSearchPage();
    void showUpdatePage();
    void showProcessPage();
    void showUserManagementPage();

    // Action slots
    void onRegisterClicked();
    void onSearchClicked();
    void onUpdateClicked();
    void onProcessDeptClicked();
    void onProcessUrgentClicked();

    // Super Admin User Management slots
    void onCreateUserClicked();
    void onReassignDepartmentClicked();
    void onDeleteUserClicked();

    // Table interaction slots
    void onComplaintTableDoubleClicked(int row, int col);
    void onViewHistoryClicked();

    // Filters and search
    void onFilterChanged();

    // Refresh helpers
    void refreshDashboard();
    void refreshAllComplaintsTable();
    void refreshUserManagementTable();

private:
    // ── Session & Core Data ──
    User currentUser;
    CivicSystem system;   // in-memory data structures (Linked list, Hash table, FIFO queues, Heap)

    // ── Layout widgets ──
    QStackedWidget* stack;

    // ── Pages ──
    QWidget* dashboardPage;
    QWidget* registerPage;
    QWidget* viewPage;
    QWidget* searchPage;
    QWidget* updatePage;
    QWidget* processPage;
    QWidget* userManagementPage; // Super Admin only

    // ── Navigation Buttons ──
    QPushButton* navDashBtn;
    QPushButton* navRegBtn;
    QPushButton* navViewBtn;
    QPushButton* navSearchBtn;
    QPushButton* navUpdateBtn;
    QPushButton* navProcessBtn;
    QPushButton* navUsersBtn;

    // ── Dashboard stat labels ──
    QLabel* totalLabel;
    QLabel* pendingLabel;
    QLabel* inProgressLabel;
    QLabel* resolvedLabel;
    QLabel* urgentLabel;
    QTableWidget* recentTable;
    QTableWidget* deptStatsTable; // For Super Admin

    // ── View-all table & filters ──
    QTableWidget* allComplaintsTable;
    QLineEdit*    filterSearchInput;
    QComboBox*    filterStatusCombo;

    // ── Register form inputs ──
    QLineEdit* nameInput;
    QLineEdit* contactInput;
    QComboBox* categoryCombo;
    QLabel*    autoDeptLabel;
    QComboBox* priorityCombo;
    QLineEdit* locationInput;
    QTextEdit* descInput;
    QLabel*    registerResultLabel;

    // ── Search page ──
    QLineEdit* searchIdInput;
    QLabel*    searchResultLabel;
    QTableWidget* searchResultTable;

    // ── Update page ──
    QLineEdit* updateIdInput;
    QComboBox* statusCombo;
    QLineEdit* updateNotesInput;
    QLabel*    updateResultLabel;

    // ── Process page ──
    QComboBox* deptCombo;
    QLabel*    queueContentsLabel;
    QLabel*    processResultLabel;

    // ── User Management (Super Admin) ──
    QTableWidget* usersTable;
    QLineEdit* newUsernameInput;
    QLineEdit* newPasswordInput;
    QComboBox* newRoleCombo;
    QComboBox* newDeptCombo;
    QLabel* userMgmtResultLabel;

    // ── Builder helpers ──
    QWidget* createNavBar();
    QWidget* buildDashboardPage();
    QWidget* buildRegisterPage();
    QWidget* buildViewPage();
    QWidget* buildSearchPage();
    QWidget* buildUpdatePage();
    QWidget* buildProcessPage();
    QWidget* buildUserManagementPage();

    // Helper modals
    void showComplaintDetailsDialog(int complaintId);

    // Utility: creates a coloured stat card widget
    QFrame* makeStatCard(const QString& title, QLabel*& numLabel, const QString& colour);
    void syncInMemoryWithDatabase();
};
