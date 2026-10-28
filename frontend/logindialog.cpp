#include "logindialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QMessageBox>
#include <QPixmap>
#include <QApplication>

// ── Stylesheet just for the login dialog ──────────────────
static const char* LOGIN_STYLE = R"(
QDialog {
    background-color: #f0f4f8;
}
QLabel#titleLabel {
    font-size: 22px;
    font-weight: bold;
    color: #1565C0;
}
QLabel#subtitleLabel {
    font-size: 13px;
    color: #555;
}
QLabel#fieldLabel {
    font-size: 13px;
    color: #333;
    font-weight: bold;
}
QLineEdit, QComboBox {
    border: 1px solid #bdbdbd;
    border-radius: 5px;
    padding: 8px;
    font-size: 13px;
    background: white;
    color: #111111;
    min-height: 20px;
}
QLineEdit:focus, QComboBox:focus {
    border: 2px solid #1976D2;
    color: #111111;
}
QComboBox QAbstractItemView { color: #111111; background: white; }
QPushButton#loginBtn {
    background-color: #1565C0;
    color: white;
    border: none;
    border-radius: 5px;
    padding: 10px;
    font-size: 14px;
    font-weight: bold;
    min-height: 40px;
}
QPushButton#loginBtn:hover   { background-color: #1976D2; }
QPushButton#loginBtn:pressed { background-color: #0D47A1; }
QLabel#errorLabel {
    color: #c62828;
    font-size: 12px;
}
QLabel#hintLabel {
    color: #1565C0;
    font-size: 11px;
    background: #e3f2fd;
    border-radius: 4px;
    padding: 6px 10px;
}
QFrame#card {
    background: white;
    border-radius: 12px;
    border: 1px solid #e0e0e0;
}
)";

// ─────────────────────────────────────────────────────────
// Constructor  –  build the login UI
// ─────────────────────────────────────────────────────────
LoginDialog::LoginDialog(QWidget* parent)
    : QDialog(parent), loggedInRole(UserRole::Citizen)
{
    setStyleSheet(LOGIN_STYLE);
    setWindowTitle("Civic Grievance System — Login");
    setFixedSize(420, 540);
    // Remove the ? help button from the title bar
    setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);

    // ── Outer layout ──────────────────────────────────────
    QVBoxLayout* outerLay = new QVBoxLayout(this);
    outerLay->setContentsMargins(30, 24, 30, 24);
    outerLay->setSpacing(0);

    // ── Top banner: icon + title ──────────────────────────
    QLabel* iconLabel = new QLabel("🏛", this);
    iconLabel->setStyleSheet("font-size: 48px;");
    iconLabel->setAlignment(Qt::AlignCenter);
    outerLay->addWidget(iconLabel);

    QLabel* titleLabel = new QLabel("Civic Grievance System", this);
    titleLabel->setObjectName("titleLabel");
    titleLabel->setAlignment(Qt::AlignCenter);
    outerLay->addWidget(titleLabel);

    QLabel* subtitleLabel = new QLabel("Please log in to continue", this);
    subtitleLabel->setObjectName("subtitleLabel");
    subtitleLabel->setAlignment(Qt::AlignCenter);
    outerLay->addWidget(subtitleLabel);

    outerLay->addSpacing(20);

    // ── White card containing the form ───────────────────
    QFrame* card = new QFrame(this);
    card->setObjectName("card");
    QVBoxLayout* cardLay = new QVBoxLayout(card);
    cardLay->setContentsMargins(24, 20, 24, 20);
    cardLay->setSpacing(14);

    // Role selector
    QLabel* roleLbl = new QLabel("Login As", card);
    roleLbl->setObjectName("fieldLabel");
    cardLay->addWidget(roleLbl);

    roleCombo = new QComboBox(card);
    roleCombo->addItem("👤  Citizen");
    roleCombo->addItem("🛠️  Admin");
    roleCombo->addItem("⭐  Super Admin");
    cardLay->addWidget(roleCombo);

    // Username
    QLabel* userLbl = new QLabel("Username", card);
    userLbl->setObjectName("fieldLabel");
    cardLay->addWidget(userLbl);

    usernameEdit = new QLineEdit(card);
    usernameEdit->setPlaceholderText("Enter your username");
    cardLay->addWidget(usernameEdit);

    // Password
    QLabel* passLbl = new QLabel("Password", card);
    passLbl->setObjectName("fieldLabel");
    cardLay->addWidget(passLbl);

    passwordEdit = new QLineEdit(card);
    passwordEdit->setPlaceholderText("Enter your password");
    passwordEdit->setEchoMode(QLineEdit::Password); // hides the typed text
    cardLay->addWidget(passwordEdit);

    // Hint label (shows credentials for demo purposes)
    hintLabel = new QLabel("", card);
    hintLabel->setObjectName("hintLabel");
    hintLabel->setWordWrap(true);
    cardLay->addWidget(hintLabel);

    // Error label (hidden until login fails)
    errorLabel = new QLabel("", card);
    errorLabel->setObjectName("errorLabel");
    errorLabel->setAlignment(Qt::AlignCenter);
    cardLay->addWidget(errorLabel);

    // Login button
    loginBtn = new QPushButton("Login", card);
    loginBtn->setObjectName("loginBtn");
    cardLay->addWidget(loginBtn);

    outerLay->addWidget(card);
    outerLay->addStretch();

    // ── Set initial hint ──────────────────────────────────
    onRoleChanged(0);  // show hint for "Citizen" by default

    // ── Connect signals to slots ──────────────────────────
    // When the role dropdown changes, update the hint label
    connect(roleCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &LoginDialog::onRoleChanged);

    // Login button click
    connect(loginBtn, &QPushButton::clicked, this, &LoginDialog::onLoginClicked);

    // Also allow pressing Enter inside the password field to log in
    connect(passwordEdit, &QLineEdit::returnPressed, this, &LoginDialog::onLoginClicked);
}

// ─────────────────────────────────────────────────────────
// onRoleChanged  –  update hint text whenever role is changed
// ─────────────────────────────────────────────────────────
void LoginDialog::onRoleChanged(int idx) {
    // Clear any old error
    errorLabel->setText("");

    // Show the correct demo credentials as a hint
    if (idx == 0) {
        hintLabel->setText("Demo → username: citizen  |  password: citizen123\n"
                           "Access: Register & Search");
    } else if (idx == 1) {
        hintLabel->setText("Demo → username: admin  |  password: admin123\n"
                           "Access: View All, Search, Update Status, Process");
    } else {
        hintLabel->setText("Demo → username: superadmin  |  password: super123\n"
                           "Access: Full access (Dashboard + all features)");
    }
}

// ─────────────────────────────────────────────────────────
// checkCredentials  –  simple hardcoded credential check
// Returns true if the username+password match the role
// ─────────────────────────────────────────────────────────
bool LoginDialog::checkCredentials(const QString& role,
                                   const QString& username,
                                   const QString& password)
{
    if (role == "Citizen") {
        return (username == "citizen" && password == "citizen123");
    }
    if (role == "Admin") {
        return (username == "admin" && password == "admin123");
    }
    if (role == "Super Admin") {
        return (username == "superadmin" && password == "super123");
    }
    return false;
}

// ─────────────────────────────────────────────────────────
// onLoginClicked  –  validate and accept the dialog
// ─────────────────────────────────────────────────────────
void LoginDialog::onLoginClicked() {
    QString username = usernameEdit->text().trimmed();
    QString password = passwordEdit->text();

    // Basic empty-field check
    if (username.isEmpty() || password.isEmpty()) {
        errorLabel->setText("⚠️  Please enter both username and password.");
        return;
    }

    // Get the selected role text (strip the emoji prefix)
    int idx = roleCombo->currentIndex();
    QString roleName;
    if (idx == 0) roleName = "Citizen";
    else if (idx == 1) roleName = "Admin";
    else roleName = "Super Admin";

    // Check if credentials are correct
    if (!checkCredentials(roleName, username, password)) {
        errorLabel->setText("❌  Invalid username or password. Please try again.");
        passwordEdit->clear();
        passwordEdit->setFocus();
        return;
    }

    // Credentials OK — set the role and close the dialog
    if (idx == 0) loggedInRole = UserRole::Citizen;
    else if (idx == 1) loggedInRole = UserRole::Admin;
    else loggedInRole = UserRole::SuperAdmin;

    accept();  // closes dialog with QDialog::Accepted result
}
