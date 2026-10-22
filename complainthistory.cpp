#include <iostream>
#include <string>
#include <vector>
using namespace std;

struct ComplaintHistory {
    int complaintId;
    vector<string> statusHistory;
};

int main() {
    ComplaintHistory c;
    int choice;
    string status;

    cout << "Enter Complaint ID: ";
    cin >> c.complaintId;

    c.statusHistory.push_back("Registered");

    do {
        cout << "\n1. Update Complaint Status";
        cout << "\n2. View Status History";
        cout << "\n3. Exit";
        cout << "\nEnter choice: ";
        cin >> choice;

        if (choice == 1) {
            cin.ignore();

            cout << "Enter new status: ";
            getline(cin, status);

            c.statusHistory.push_back(status);
            cout << "Status updated successfully!\n";
        }
        else if (choice == 2) {
            cout << "\nComplaint ID: " << c.complaintId;
            cout << "\nStatus History:\n";

            for (int i = 0; i < c.statusHistory.size(); i++) {
                cout << i + 1 << ". "
                     << c.statusHistory[i] << endl;
            }
        }
        else if (choice != 3) {
            cout << "Invalid choice!\n";
        }

    } while (choice != 3);

    return 0;
}