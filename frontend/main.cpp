#include <QApplication>
#include <QFont>
#include "logindialog.h"
#include "mainwindow.h"
#include "databasemanager.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Set clean, readable font
    QFont font("Segoe UI", 10);
    app.setFont(font);

    // Step 1: Open Login Dialog
    LoginDialog loginDlg;
    if (loginDlg.exec() != QDialog::Accepted) {
        return 0; // User exited login dialog
    }

    // Step 2: Retrieve the authenticated user from database
    User user = loginDlg.getAuthenticatedUser();

    // Step 3: Open Main Window with user's role and assigned department
    MainWindow w(user);
    w.setWindowTitle(QString("Intelligent Civic Grievance System — %1 (%2)")
                         .arg(user.username, user.roleName()));
    w.resize(1150, 750);
    w.show();

    return app.exec();
}
