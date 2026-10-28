#include <QApplication>
#include <QFont>
#include "logindialog.h"
#include "mainwindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Set a clean, readable font for the whole application
    QFont font("Segoe UI", 10);
    app.setFont(font);

    // ── Step 1: Show the login dialog ──────────────────────
    // The login dialog is shown first, before the main window.
    // If the user closes it without logging in, the app exits.
    LoginDialog loginDlg;
    if (loginDlg.exec() != QDialog::Accepted) {
        // User closed or cancelled the login window — exit cleanly
        return 0;
    }

    // ── Step 2: Get the role that logged in ────────────────
    UserRole role = loginDlg.getRole();

    // ── Step 3: Open the main window with that role ────────
    MainWindow w(role);
    w.setWindowTitle("Civic Grievance Management System");
    w.resize(1100, 700);
    w.show();

    return app.exec();
}
