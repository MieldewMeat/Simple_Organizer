#include <fstream>
#include <filesystem>
#include <string>
#include <iostream>
#include <vector>
#include <conio.h>
#include <windows.h>
#include <cctype>

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
            system("CLS");
            cout << "--- Organizer ---\n\n";
            cout << " 1- Organize\n";
            cout << " 2- Add path\n";
            cout << " 3- Erase path\n";
            cout << " 4- Search path\n";
            cout << " 0- Quit\n";

            choice = _getch();
            system("CLS");

            switch (choice) {
                case '1':
                    organize();
                    break;
                case '2':
                    addPath();
                    break;
                case '3':
                    removePath();
                    break;
                case '4':
                    searchPath();
                    break;
                case '0':
                    exit();
                    return;
                default:
                    cout << "Invalid option.\n";
                    system("pause");
            }
        }
    }
    int exit() {
        ofstream output("paths.pth");
        cout << "Saving...\n";
        for (int i = 0; i < data.size(); i++) {
            if (output.is_open()) {
                output << data[i].ext << endl;
                output << data[i].path.string() << endl;
            } else {
                cout << "Error trying to save the file.\n";
                system("pause");
            }
        }
    }
    int organize() {
        string pathStr;
        fs::path sourceDir;

        while (true) {
            cout << "Insert the path to the pasta:\n > ";
            pathStr = getLineBetter();

            if (pathStr.empty()) return 0;

            sourceDir = fs::path(pathStr);

            if (fs::exists(sourceDir) && fs::is_directory(sourceDir)) {
                break;
            }

            cout << "That path does not exist\n";
            system("pause");
            system("CLS");
        }

        system("CLS");
        cout << "Organizing...\n";

        for (const auto& entry : fs::directory_iterator(sourceDir)) {
            if (!entry.is_regular_file()) continue;

            string extension = entry.path().extension().string();
            bool found = false;
            fs::path destFolder;

            for (const auto& rule : data) {
                if (extension == rule.ext) {
                    found = true;
                    destFolder = rule.path;
                    break;
                }
            }

            if (found) {
                if (!fs::exists(destFolder)) {
                    fs::create_directories(destFolder);
                }

                fs::path destPath = destFolder / entry.path().filename();

                cout << "Moving: " << entry.path().filename().string()
                     << " -> " << destPath.string() << endl;

                try {
                    fs::copy(entry.path(), destPath, fs::copy_options::overwrite_existing);
                    fs::remove(entry.path());
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
            system("CLS");
            cout << "Insert the path (ex: C:/Downloads/Imagens):\n > ";

            pathStr = getLineBetter();
            if (pathStr.empty()) return 0;

            if (fs::exists(pathStr) && fs::is_directory(pathStr)) break;

            cout << "That path does not exist. Try again.\n";
            system("pause");
        }

        while (true) {
            system("CLS");
            cout << "Insert an extension (ex: .png):\n > ";

            extStr = getLineBetter();
            if (extStr.empty()) return 0;

            if (extStr[0] == '.') {
                bool exists = false;

                for (const auto& d : data) {
                    if (d.ext == extStr) {
                        exists = true;
                        break;
                    }
                }

                if (!exists) break;
                system("CLS");
                cout << "Extension already exists, try another\n";
                system("pause");
                continue;
            }

            cout << "Extension invalid (must start with a dot, ex: .jpg).\n";
            system("pause");
        }

        data.emplace_back(pathStr, extStr);
        return 0;
    }

    int removePath() {
        string extStr;

        while (true) {
            system("CLS");
            cout << "Select the extension you want to remove:\n > ";

            extStr = getLineBetterSearch("Select the extension you want to remove");
            if (extStr.empty()) return 0;

            if (extStr[0] == '.') break;

            cout << "\nExtension invalid (must start with a dot, ex: .jpg).\n";
            system("pause");
        }

        bool erased = false;

        for (int i = 0; i < data.size(); i++) {
            if (data[i].ext == extStr) {
                data.erase(data.begin() + i);
                erased = true;
                break;
            }
        }

        if (!erased) cout << "\nError finding the extension\n";
        else cout << "\nExtension " << extStr << " removed.\n";

        system("pause");
        return 0;
    }

    int searchPath() {
        cout << "Search:\n > ";
        getLineBetterSearch("Search");
        return 0;
    }

private:
    string getClipboardText() {
        if (!OpenClipboard(nullptr)) return "";

        HANDLE hData = GetClipboardData(CF_TEXT);
        if (hData == nullptr) return "";

        char* pszText = static_cast<char*>(GlobalLock(hData));
        if (pszText == nullptr) return "";

        string text(pszText);

        GlobalUnlock(hData);
        CloseClipboard();

        return text;
    }

    void printSearch(string String) {
        for (int i = 0; i < data.size(); i++) {
            if (!String.empty() && data[i].ext.rfind(String, 0) == 0) {
                cout << data[i].ext << " -> " << data[i].path.string() << '\n';
            }
        }
    }
    string getLineBetterSearch(string Name) {
        string String = "";
        while (true) {
            if (_kbhit()) {
                char hold = _getch();

                if (hold == 13) break;
                if (hold == 27) return "";

                if (hold == 22) {
                    string clip = getClipboardText();
                    for (char c : clip) {
                        if (isprint(c)) String.push_back(c);
                    }
                }

                if (hold == 8) {
                    if (!String.empty()) String.pop_back();
                }

                if (isprint(hold)) {
                    String.push_back(hold);
                }

                system("CLS");
                cout << Name << ":\n > " << String << "\n\n";
                printSearch(String);
            }
        }
        return String;
    }

    string getLineBetter() {
        string String;

        while (true) {
            char hold = _getch();

            if (hold == 13) break;
            if (hold == 27) return "";

            if (hold == 8) {
                if (!String.empty()) {
                    cout << "\b \b";
                    String.pop_back();
                }
                continue;
            }

            if (hold == 22) {
                string clip = getClipboardText();
                for (char c : clip) {
                    if (isprint(c)) {
                        cout << c;
                        String.push_back(c);
                    }
                }
                continue;
            }

            if (isprint(hold)) {
                cout << hold;
                String.push_back(hold);
            }
        }

        return String;
    }

    void getPaths() {
        ifstream input("paths.pth");
        string ext, pathStr;

        while (getline(input, ext)) {
            if (getline(input, pathStr)) {
                data.emplace_back(pathStr, ext);
            }
        }
    }

    vector<storage> data;
};

int main() {
    SetConsoleCtrlHandler(NULL, TRUE);
    Organizer app;
    app.run();
    return 0;
}
