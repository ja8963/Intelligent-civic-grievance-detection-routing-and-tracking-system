#pragma once

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>

// ──────────────────────────────────────────────────────────
// UserRole  –  which type of user just logged in
// ──────────────────────────────────────────────────────────
enum class UserRole {
    Citizen,    // Can: Register complaint, Search by ID
    Admin,      // Can: View All, Search, Update Status, Process Complaints
    SuperAdmin  // Can: Everything (Dashboard + all pages)
};

// ──────────────────────────────────────────────────────────
// LoginDialog  –  shown before the main window opens
//
// It has:
//   - A role selector  (Citizen / Admin / Super Admin)
//   - A username field
//   - A password field
//   - Login button
//
// Hardcoded credentials (good enough for a college demo):
//   Citizen    : username = "citizen"    password = "citizen123"
//   Admin      : username = "admin"      password = "admin123"
//   Super Admin: username = "superadmin" password = "super123"
// ──────────────────────────────────────────────────────────
class LoginDialog : public QDialog {
    Q_OBJECT

public:
    explicit LoginDialog(QWidget* parent = nullptr);

    // Call this after exec() returns Accepted to get the role
    UserRole getRole() const { return loggedInRole; }

private slots:
    void onLoginClicked();       // checks credentials and accepts/rejects
    void onRoleChanged(int idx); // updates the placeholder hints when role changes

private:
    QComboBox*   roleCombo;
    QLineEdit*   usernameEdit;
    QLineEdit*   passwordEdit;
    QPushButton* loginBtn;
    QLabel*      errorLabel;    // shows "Invalid credentials" message
    QLabel*      hintLabel;     // shows credential hints per role

    UserRole loggedInRole;      // set when login succeeds

    // Helper: verifies username+password for the selected role
    bool checkCredentials(const QString& role,
                          const QString& username,
                          const QString& password);
};
