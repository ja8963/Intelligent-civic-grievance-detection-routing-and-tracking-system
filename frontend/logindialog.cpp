#include "logindialog.h"
#include "databasemanager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QMessageBox>
#include <QApplication>
#include <QDialogButtonBox>
#include <QSpinBox>

static const char* LOGIN_STYLE = R"(
QDialog {
    background-color: #f4f6f9;
}
QLabel#titleLabel {
    font-size: 22px;
    font-weight: bold;
    color: #1a365d;
}
QLabel#subtitleLabel {
    font-size: 13px;
    color: #4a5568;
}
QLabel#fieldLabel {
    font-size: 13px;
    color: #2d3748;
    font-weight: 600;
}
QLineEdit {
    border: 1px solid #cbd5e0;
    border-radius: 6px;
    padding: 8px 12px;
    font-size: 13px;
    background: #ffffff;
    color: #1a202c;
    min-height: 22px;
}
QLineEdit:focus {
    border: 2px solid #3182ce;
}
QPushButton#loginBtn {
    background-color: #2b6cb0;
    color: white;
    border: none;
    border-radius: 6px;
    padding: 10px;
    font-size: 14px;
    font-weight: bold;
    min-height: 38px;
}
QPushButton#loginBtn:hover   { background-color: #3182ce; }
QPushButton#loginBtn:pressed { background-color: #2c5282; }

QPushButton.secondaryBtn {
    background-color: transparent;
    color: #2b6cb0;
    border: 1px solid #2b6cb0;
    border-radius: 5px;
    padding: 6px 10px;
    font-size: 12px;
    font-weight: 600;
}
QPushButton.secondaryBtn:hover {
    background-color: #ebf8ff;
}

QLabel#errorLabel {
    color: #e53e3e;
    font-size: 12px;
    font-weight: bold;
}
QLabel#statusLabel {
    color: #2b6cb0;
    font-size: 12px;
    background: #ebf8ff;
    border-radius: 6px;
    padding: 8px 12px;
    border: 1px solid #bee3f8;
}
QFrame#card {
    background: #ffffff;
    border-radius: 12px;
    border: 1px solid #e2e8f0;
}
)";

LoginDialog::LoginDialog(QWidget* parent)
    : QDialog(parent)
{
    setStyleSheet(LOGIN_STYLE);
    setWindowTitle("Civic Grievance System — Portal Login");
    setFixedSize(440, 560);
    setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);

    QVBoxLayout* outerLay = new QVBoxLayout(this);
    outerLay->setContentsMargins(32, 24, 32, 24);
    outerLay->setSpacing(0);

    QLabel* iconLabel = new QLabel("🏛️", this);
    iconLabel->setStyleSheet("font-size: 44px;");
    iconLabel->setAlignment(Qt::AlignCenter);
    outerLay->addWidget(iconLabel);

    QLabel* titleLabel = new QLabel("Intelligent Civic Grievance", this);
    titleLabel->setObjectName("titleLabel");
    titleLabel->setAlignment(Qt::AlignCenter);
    outerLay->addWidget(titleLabel);

    QLabel* subtitleLabel = new QLabel("Administrative Access & Tracking Portal", this);
    subtitleLabel->setObjectName("subtitleLabel");
    subtitleLabel->setAlignment(Qt::AlignCenter);
    outerLay->addWidget(subtitleLabel);

    outerLay->addSpacing(16);

    QFrame* card = new QFrame(this);
    card->setObjectName("card");
    QVBoxLayout* cardLay = new QVBoxLayout(card);
    cardLay->setContentsMargins(24, 20, 24, 20);
    cardLay->setSpacing(12);

    statusLabel = new QLabel("", card);
    statusLabel->setObjectName("statusLabel");
    statusLabel->setWordWrap(true);
    cardLay->addWidget(statusLabel);

    QLabel* userLbl = new QLabel("Username", card);
    userLbl->setObjectName("fieldLabel");
    cardLay->addWidget(userLbl);

    usernameEdit = new QLineEdit(card);
    usernameEdit->setPlaceholderText("Enter your assigned username");
    cardLay->addWidget(usernameEdit);

    QLabel* passLbl = new QLabel("Password", card);
    passLbl->setObjectName("fieldLabel");
    cardLay->addWidget(passLbl);

    passwordEdit = new QLineEdit(card);
    passwordEdit->setPlaceholderText("Enter your password");
    passwordEdit->setEchoMode(QLineEdit::Password);
    cardLay->addWidget(passwordEdit);

    errorLabel = new QLabel("", card);
    errorLabel->setObjectName("errorLabel");
    errorLabel->setAlignment(Qt::AlignCenter);
    cardLay->addWidget(errorLabel);

    loginBtn = new QPushButton("Log In", card);
    loginBtn->setObjectName("loginBtn");
    cardLay->addWidget(loginBtn);

    QHBoxLayout* btnRow = new QHBoxLayout();
    dbSettingsBtn = new QPushButton("⚙️ Database Connection", card);
    dbSettingsBtn->setProperty("class", "secondaryBtn");
    btnRow->addWidget(dbSettingsBtn);

    setupAdminBtn = new QPushButton("Create Super Admin", card);
    setupAdminBtn->setProperty("class", "secondaryBtn");
    setupAdminBtn->setVisible(false);
    btnRow->addWidget(setupAdminBtn);

    cardLay->addLayout(btnRow);
    outerLay->addWidget(card);

    connect(loginBtn, &QPushButton::clicked, this, &LoginDialog::onLoginClicked);
    connect(passwordEdit, &QLineEdit::returnPressed, this, &LoginDialog::onLoginClicked);
    connect(dbSettingsBtn, &QPushButton::clicked, this, &LoginDialog::onDbSettingsClicked);
    connect(setupAdminBtn, &QPushButton::clicked, this, &LoginDialog::onSetupAdminClicked);

    // Initial database connection check
    if (!DatabaseManager::instance().isConnected()) {
        DatabaseManager::instance().connectToDatabase("127.0.0.1", 3306, "civic_grievance", "root", "");
    }
    updateDbStatus();
}

void LoginDialog::updateDbStatus() {
    DatabaseManager& db = DatabaseManager::instance();
    if (!db.isConnected()) {
        statusLabel->setText("⚠️ Database: Not connected. Click 'Database Connection' below to configure.");
        statusLabel->setStyleSheet("color: #c53030; background: #fff5f5; border: 1px solid #feb2b2; padding: 8px; border-radius: 6px;");
        setupAdminBtn->setVisible(false);
    } else {
        if (!db.hasAnySuperAdmin()) {
            statusLabel->setText("ℹ️ Connected to MySQL. No Super Admin account found. Please create one.");
            statusLabel->setStyleSheet("color: #2b6cb0; background: #ebf8ff; border: 1px solid #bee3f8; padding: 8px; border-radius: 6px;");
            setupAdminBtn->setVisible(true);
        } else {
            statusLabel->setText(" Connected to MySQL database (civic_grievance). Ready for login.");
            statusLabel->setStyleSheet("color: #276749; background: #f0fff4; border: 1px solid #9ae6b4; padding: 8px; border-radius: 6px;");
            setupAdminBtn->setVisible(false);
        }
    }
}

void LoginDialog::onLoginClicked() {
    errorLabel->setText("");
    QString user = usernameEdit->text().trimmed();
    QString pass = passwordEdit->text();

    if (user.isEmpty() || pass.isEmpty()) {
        errorLabel->setText("Please enter both username and password.");
        return;
    }

    DatabaseManager& db = DatabaseManager::instance();
    if (!db.isConnected()) {
        errorLabel->setText("Cannot login: Database is not connected.");
        return;
    }

    auto authResult = db.authenticateUser(user, pass);
    if (!authResult.has_value()) {
        errorLabel->setText(db.lastError().isEmpty() ? "Invalid username or password." : db.lastError());
        return;
    }

    authenticatedUser = authResult.value();
    accept();
}

void LoginDialog::onDbSettingsClicked() {
    QDialog dlg(this);
    dlg.setWindowTitle("Configure MySQL Database Connection");
    dlg.setFixedSize(380, 300);

    QFormLayout form(&dlg);
    QLineEdit hostEdit("127.0.0.1", &dlg);
    QSpinBox portBox(&dlg);
    portBox.setRange(1, 65535);
    portBox.setValue(3306);
    QLineEdit dbEdit("civic_grievance", &dlg);
    QLineEdit userEdit("root", &dlg);
    QLineEdit passEdit(&dlg);
    passEdit.setEchoMode(QLineEdit::Password);
    passEdit.setPlaceholderText("MySQL root password");

    form.addRow("Host:", &hostEdit);
    form.addRow("Port:", &portBox);
    form.addRow("Database:", &dbEdit);
    form.addRow("Username:", &userEdit);
    form.addRow("Password:", &passEdit);

    QDialogButtonBox btns(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    form.addRow(&btns);
    connect(&btns, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(&btns, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() == QDialog::Accepted) {
        bool ok = DatabaseManager::instance().connectToDatabase(
            hostEdit.text().trimmed(),
            portBox.value(),
            dbEdit.text().trimmed(),
            userEdit.text().trimmed(),
            passEdit.text()
        );

        if (!ok) {
            QMessageBox::critical(this, "Connection Error",
                                  "Failed to connect to MySQL database:\n" + DatabaseManager::instance().lastError());
        } else {
            QMessageBox::information(this, "Success", "Connected to MySQL successfully!");
        }
        updateDbStatus();
    }
}

void LoginDialog::onSetupAdminClicked() {
    QDialog dlg(this);
    dlg.setWindowTitle("Create Initial Super Admin");
    dlg.setFixedSize(360, 240);

    QFormLayout form(&dlg);
    QLineEdit userEdit("admin", &dlg);
    QLineEdit passEdit(&dlg);
    passEdit.setEchoMode(QLineEdit::Password);
    QLineEdit confirmEdit(&dlg);
    confirmEdit.setEchoMode(QLineEdit::Password);

    form.addRow("Super Admin Username:", &userEdit);
    form.addRow("Password:", &passEdit);
    form.addRow("Confirm Password:", &confirmEdit);

    QDialogButtonBox btns(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    form.addRow(&btns);
    connect(&btns, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(&btns, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() == QDialog::Accepted) {
        if (userEdit.text().trimmed().isEmpty() || passEdit.text().isEmpty()) {
            QMessageBox::warning(this, "Validation Error", "Username and password cannot be empty.");
            return;
        }
        if (passEdit.text() != confirmEdit.text()) {
            QMessageBox::warning(this, "Validation Error", "Passwords do not match.");
            return;
        }

        bool ok = DatabaseManager::instance().ensureInitialSuperAdmin(userEdit.text().trimmed(), passEdit.text());
        if (ok) {
            QMessageBox::information(this, "Admin Created",
                                    "Super Admin created successfully!\nYou can now log in with these credentials.");
            usernameEdit->setText(userEdit.text().trimmed());
            passwordEdit->clear();
            updateDbStatus();
        } else {
            QMessageBox::critical(this, "Error", "Failed to create Super Admin:\n" + DatabaseManager::instance().lastError());
        }
    }
}
