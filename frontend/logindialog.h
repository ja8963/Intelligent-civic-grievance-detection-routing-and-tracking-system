#pragma once

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include "user.h"

class LoginDialog : public QDialog {
    Q_OBJECT

public:
    explicit LoginDialog(QWidget* parent = nullptr);

    // Call this after exec() returns Accepted to get the authenticated user
    User getAuthenticatedUser() const { return authenticatedUser; }

private slots:
    void onLoginClicked();
    void onDbSettingsClicked();
    void onSetupAdminClicked();

private:
    QLineEdit*   usernameEdit;
    QLineEdit*   passwordEdit;
    QPushButton* loginBtn;
    QPushButton* dbSettingsBtn;
    QPushButton* setupAdminBtn;
    QLabel*      errorLabel;
    QLabel*      statusLabel;

    User authenticatedUser;

    void updateDbStatus();
};
