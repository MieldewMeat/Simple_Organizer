#include <fstream>
#include <filesystem>
#include <string>
#include <iostream>
#include <vector>
#include <limits>

using namespace std;

namespace fs = std::filesystem;

struct storage {
    storage(string Path, string Ext) {
        path = fs::path(Path);
        ext = Ext;
    }
    storage() {}
    fs::path path;
    string ext;
};

class Organizer {
public:
    Organizer() {
        getPaths();
    }

    void run() {
        char choice;
        while (true) {
            cout << "--- Organizer ---\n\n";
            cout << " 1- Organize\n";
            cout << " 2- Add path\n";
            cout << " 0- Quit\n";
            cout << " > ";

            cin >> choice;

            cin.ignore(numeric_limits<streamsize>::max(), '\n');

            switch (choice) {
                case '1':
                    organize();
                    break;
                case '2':
                    addPath();
                    break;
                case '0':
                    cout << "Exiting...\n";
                    return;
                default:
                    cout << "Invalid option.\n";
                    system("pause");
            }
        }
    }

    int organize() {
        string pathStr;
        fs::path sourceDir;

        while (true) {
            cout << "Insert the path to the pasta:\n > ";

            getline(cin, pathStr);

            sourceDir = fs::path(pathStr);

            if (fs::exists(sourceDir) && fs::is_directory(sourceDir)) {
                break;
            }
            cout << "That path does not exist\n";
            system("pause");
        }

        cout << "\nOrganizing...\n";

        for (const auto& entry : fs::directory_iterator(sourceDir)) {
            if (!entry.is_regular_file()) continue;

            string extension = entry.path().extension().string();

            bool finded = false;
            fs::path destFolder;

            for (const auto& rule : data) {

                if (extension == rule.ext) {
                    finded = true;
                    destFolder = rule.path;
                    break;
                }
            }

            if (finded) {
                if (!fs::exists(destFolder)) {
                    fs::create_directories(destFolder);
                }

                fs::path destPath = destFolder / entry.path().filename();

                cout << "Moving: " << entry.path().filename().string()
                     << " -> " << destPath.string() << endl;

                try {
                    fs::rename(entry.path(), destPath);
                }
                catch (const fs::filesystem_error& e) {
                    cout << "Error moving: " << e.what() << endl;
                }
            }
        }
        cout << "\nFinished\n";
        system("pause");
        return 0;
    }

    int addPath() {
        string pathStr, extStr;

        while (true) {
            cout << "Insert the path (ex: C:/Downloads/Imagens):\n > ";
            getline(cin, pathStr);

            if (fs::exists(pathStr) && fs::is_directory(pathStr)) {
                break;
            }
            cout << "That path does not exist. Try again.\n";
            system("pause");
        }

        while (true) {
            cout << "Insert a extension (ex: .png):\n > ";
            cin >> extStr;

            if (!extStr.empty() && extStr[0] == '.') {
                break;
            }
            cout << "Extension invalid (must start with a dot, ex: .jpg).\n";

            cin.ignore(numeric_limits<streamsize>::max(), '\n');
        }

        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        data.emplace_back(pathStr, extStr);

        ofstream output("paths.pth", ios::app);
        if (output.is_open()) {
            output << extStr << endl;
            output << pathStr << endl;
            output.close();
            cout << "\nAdding with success!\n";
        }
        else {
            cout << "Error trying to save the file.\n";
        }

        system("pause");
        return 0;
    }

private:
    void getPaths() {
        ifstream input("paths.pth");
        string ext, pathStr;

        while (getline(input, ext)) {
            if (getline(input, pathStr)) {
                data.emplace_back(pathStr, ext);
            }
        }
    };

    vector<storage> data;
};

int main() {
    Organizer app;
    app.run();
    return 0;
}
