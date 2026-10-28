#pragma once

#include <QMainWindow>
#include <QStackedWidget>
#include <QTableWidget>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QListWidget>
#include <QFrame>
#include <QPushButton>

#include "complaint.h"
#include "logindialog.h"   // for UserRole enum

// ──────────────────────────────────────────
// Main application window
// ──────────────────────────────────────────
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    // role controls which nav buttons are visible
    explicit MainWindow(UserRole role, QWidget* parent = nullptr);
    ~MainWindow() override = default;

private slots:
    // Navigation slots – switch the visible page
    void showDashboard();
    void showRegisterPage();
    void showViewPage();
    void showSearchPage();
    void showUpdatePage();
    void showProcessPage();

    // Action slots
    void onRegisterClicked();
    void onSearchClicked();
    void onUpdateClicked();
    void onProcessDeptClicked();
    void onProcessUrgentClicked();

    // Refresh helpers
    void refreshDashboard();
    void refreshAllComplaintsTable();

private:
    // ── Data ──
    CivicSystem system;   // the core backend object
    UserRole    currentRole;  // stores which role is logged in

    // ── Layout widgets ──
    QStackedWidget* stack;        // switches between pages

    // ── Pages ──
    QWidget* dashboardPage;
    QWidget* registerPage;
    QWidget* viewPage;
    QWidget* searchPage;
    QWidget* updatePage;
    QWidget* processPage;

    // ── Dashboard stat labels ──
    QLabel* totalLabel;
    QLabel* pendingLabel;
    QLabel* resolvedLabel;
    QTableWidget* recentTable;    // shows last 5 complaints on dashboard

    // ── View-all table ──
    QTableWidget* allComplaintsTable;

    // ── Register form inputs ──
    QLineEdit* nameInput;
    QLineEdit* descInput;
    QLineEdit* locationInput;
    QComboBox* categoryCombo;
    QComboBox* priorityCombo;

    // ── Search page ──
    QLineEdit* searchIdInput;
    QLabel*    searchResultLabel;

    // ── Update page ──
    QLineEdit* updateIdInput;
    QComboBox* statusCombo;
    QLabel*    updateResultLabel;

    // ── Process page ──
    QComboBox* deptCombo;
    QLabel*    processResultLabel;

    // ── Builder helpers ──
    QWidget* createNavBar();
    QWidget* buildDashboardPage();
    QWidget* buildRegisterPage();
    QWidget* buildViewPage();
    QWidget* buildSearchPage();
    QWidget* buildUpdatePage();
    QWidget* buildProcessPage();

    // Utility: creates a coloured stat card widget
    QFrame* makeStatCard(const QString& title, QLabel*& numLabel, const QString& colour);
};
