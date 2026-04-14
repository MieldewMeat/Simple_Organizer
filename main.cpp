#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <string>
#include <cctype>
#include <thread>
#include <chrono>
#include <algorithm>

#ifdef _WIN32
    #include <windows.h>
    #include <conio.h>
    #define OS_WINDOWS 1

    void setupConsole() {
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD dwMode = 0;
        GetConsoleMode(hOut, &dwMode);
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hOut, dwMode);
        SetConsoleCtrlHandler(NULL, TRUE);
    }
    void restoreConsole() {}
#else
    #include <termios.h>
    #include <unistd.h>
    #include <sys/select.h>
    #define OS_WINDOWS 0

    termios orig_termios;

    void setupConsole() {
        tcgetattr(STDIN_FILENO, &orig_termios);
        termios new_termios = orig_termios;
        new_termios.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
    }

    void restoreConsole() {
        tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
    }
#endif

namespace fs = std::filesystem;
using namespace std;

// --- Cores e Estilos ---
namespace UI {
    const string RESET = "\033[0m";
    const string BOLD = "\033[1m";
    const string RED = "\033[31m";
    const string GREEN = "\033[32m";
    const string YELLOW = "\033[33m";
    const string CYAN = "\033[36m";
    const string CLEAR = "\033[2J\033[H";
}

// --- Funções de Input Unificadas ---

bool keybindhit() {
    #ifdef _WIN32
        return _kbhit();
    #else
        struct timeval tv = {0L, 0L};
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);
        return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);
    #endif
}

char readChar() {
    #ifdef _WIN32
        return _getch();
    #else
        char buf = 0;
        if (read(STDIN_FILENO, &buf, 1) < 0) return 0;
        return buf;
    #endif
}

void pauseExec() {
    cout << UI::YELLOW << "\n[Pressione qualquer tecla para continuar]" << UI::RESET << flush;
    while(!keybindhit()) this_thread::sleep_for(chrono::milliseconds(10));
    readChar();
}

string getClipboardText() {
    string text;
    #ifdef _WIN32
        if (OpenClipboard(nullptr)) {
            HANDLE hData = GetClipboardData(CF_TEXT);
            if (hData) {
                char* pszText = static_cast<char*>(GlobalLock(hData));
                if (pszText) { text = pszText; GlobalUnlock(hData); }
            }
            CloseClipboard();
        }
    #elif __APPLE__
        FILE* pipe = popen("pbpaste 2>/dev/null", "r");
        if (pipe) {
            char buffer[128];
            while (fgets(buffer, sizeof(buffer), pipe)) text += buffer;
            pclose(pipe);
        }
    #elif __linux__
        FILE* pipe = popen("xclip -selection clipboard -o 2>/dev/null", "r");
        if (pipe) {
            char buffer[128];
            while (fgets(buffer, sizeof(buffer), pipe)) text += buffer;
            pclose(pipe);
        }
    #endif
    while (!text.empty() && (text.back() == '\n' || text.back() == '\r')) text.pop_back();
    return text;
}

// --- Estruturas de Dados ---

struct Storage {
    fs::path path;
    string ext;
    Storage(string p, string e) : path(p), ext(e) {}
};

class Organizer {
    vector<Storage> data;

public:
    Organizer() { loadPaths(); }

    void run() {
        char choice;
        while (true) {
            cout << UI::CLEAR;
            printUI();

            choice = readChar();
            cout << UI::CLEAR;

            switch (choice) {
                case '1': organizeFiles(); break;
                case '2': addPath(); break;
                case '3': removePath(); break;
                case '4': searchPath(); break;
                case '0': return;
                default:
                    cout << UI::RED << "Opcao invalida." << UI::RESET << endl;
                    pauseExec();
            }
        }
    }

private:
    void printUI() {
        cout << UI::BOLD << "============================" << UI::RESET << "\n";
        cout << UI::BOLD << "       FILE ORGANIZER       " << UI::RESET << "\n";
        cout << UI::BOLD << "============================" << UI::RESET << "\n\n";

        cout << UI::CYAN << " 1" << UI::RESET << " - Organizar Arquivos\n";
        cout << UI::CYAN << " 2" << UI::RESET << " - Adicionar Regra\n";
        cout << UI::CYAN << " 3" << UI::RESET << " - Remover Regra\n";
        cout << UI::CYAN << " 4" << UI::RESET << " - Buscar Regra\n";
        cout << UI::RED    << " 0" << UI::RESET << " - Sair\n";
        cout << "\n > ";
        cout.flush();
    }

    void loadPaths() {
        ifstream input("paths.pth");
        string ext, p;
        while (getline(input, ext) && getline(input, p)) {
            data.emplace_back(p, ext);
        }
    }

    void savePaths() {
        ofstream output("paths.pth");
        for (const auto& d : data) {
            output << d.ext << "\n" << d.path.string() << "\n";
        }
    }

    string getLineInput(const string& prompt) {
        string buffer;
        cout << prompt << "\n > " << flush;

        while (true) {
            char c = readChar();

            if (c == 13 || c == 10) {
                cout << "\n";
                return buffer;
            }
            if (c == 27) {
                cout << UI::RED << "[Cancelado]" << UI::RESET << "\n";
                return "";
            }
            if (c == 8 || c == 127) {
                if (!buffer.empty()) {
                    buffer.pop_back();
                    cout << "\b \b" << flush;
                }
            }
            else if (c == 22) {
                string clip = getClipboardText();
                for (char ch : clip) {
                    if (isprint(ch)) {
                        buffer.push_back(ch);
                        cout << ch;
                    }
                }
            }
            else if (isprint(c)) {
                buffer.push_back(c);
                cout << c;
            }
            cout.flush();
        }
    }

    string searchInput(const string& prompt) {
        string buffer, bufferLast;
        while (true) {
            cout << UI::CLEAR;
            cout << prompt << "\n > " << UI::BOLD << buffer << UI::RESET << "\n\n";

            printSearchResults(bufferLast);

            while(!keybindhit()) this_thread::sleep_for(chrono::milliseconds(20));

            char c = readChar();

            if (c == 13 || c == 10) return buffer;
            if (c == 27) return "";
            if (c == 8 || c == 127) { if (!buffer.empty()) buffer.pop_back(); }
            else if (isprint(c)) buffer.push_back(c);

            int last;
            for (int i = 0; i < buffer.size(); i++) {
                last = buffer.size()-i;
                if (buffer[last] == '.') break;
            }
            bufferLast = "";
            if (last == 0) bufferLast = buffer;
            else {
                for (int i = last; i < buffer.size(); i++) {
                    if (buffer[i] == ' ') continue;
                    bufferLast.push_back(buffer[i]);
                }
            }
        }
    }

    void printSearchResults(const string& filter) {
        cout << UI::CYAN << "-- Regras Atuais --" << UI::RESET << "\n";
        bool found = false;
        for (const auto& d : data) {
            if (filter.empty() || d.ext.find(filter) != string::npos) {
                cout << " " << UI::GREEN << d.ext << UI::RESET << " -> " << d.path.string() << "\n";
                found = true;
            }
        }
        if (!found) cout << UI::RED << " Nenhuma correspondencia." << UI::RESET << "\n";
    }

    void organizeFiles() {
        string pathStr = getLineInput("Pasta para organizar");
        if (pathStr.empty()) return;

        fs::path src(pathStr);
        if (!fs::exists(src) || !fs::is_directory(src)) {
            cout << UI::RED << "Caminho invalido." << UI::RESET << "\n";
            pauseExec();
            return;
        }

        cout << UI::YELLOW << "Organizando..." << UI::RESET << "\n";
        int count = 0;

        for (const auto& entry : fs::directory_iterator(src)) {
            if (!entry.is_regular_file()) continue;
            string ext = entry.path().extension().string();

            for (const auto& rule : data) {
                if (ext == rule.ext) {
                    try {
                        if (!fs::exists(rule.path)) fs::create_directories(rule.path);
                        fs::rename(entry.path(), rule.path / entry.path().filename());
                        cout << "Movido: " << entry.path().filename().string() << "\n";
                        count++;
                    } catch (...) {
                        cout << UI::RED << "Falha ao mover: " << entry.path().filename().string() << UI::RESET << "\n";
                    }
                    break;
                }
            }
        }
        cout << UI::GREEN << "\nConcluido! " << count << " arquivos movidos." << UI::RESET << "\n";
        pauseExec();
    }

    void addPath() {
        string p, e, holder;
        while (true) {
            p = getLineInput("Caminho de destino");
            if (p.empty()) return;
            if (!fs::exists(p)) {
                cout << UI::CLEAR << UI::RED << "Caminho nao existe." << UI::RESET << "\n\n";
            } else break;
        }

        cout << UI::CLEAR;

        while (true){
            e = getLineInput("Extensao (ex: .png)");
            if (e.empty()) return;
            for (int i = 0; i < e.size(); i++) {
                switch (e[i]) {
                    case '.':
                        if (holder[0]=='.') {
                            data.emplace_back(p, holder);
                            holder = ".";
                        } else {
                            holder = ".";
                        }
                        break;
                    case ' ':
                        break;
                    default:
                        holder.push_back(e[i]);
                        break;
                }
            }
            if (holder[0] != '.') {
                cout << UI::CLEAR << UI::RED << "Extensao invalida: " << holder << "." << UI::RESET << "\n";
            } else break;
        }

        data.emplace_back(p, holder);
        savePaths();
        cout << UI::GREEN << "Regra salva!" << UI::RESET << "\n";
        pauseExec();
    }

    void removePath() {
        string holder;
        string e = searchInput("Extensao para remover");
        if (e.empty()) return;

        for (int i = 0; i < e.size(); i++) {
            switch (e[i]) {
                case '.':
                    if (holder[0]=='.') {
                        auto it = remove_if(data.begin(), data.end(), [&holder](const Storage& s){ return s.ext == holder; });
                        if (it != data.end()) {
                            data.erase(it, data.end());
                            savePaths();
                            cout << UI::GREEN << "Extensao: " << UI::RESET << holder << " Removido." << UI::RESET << "\n";
                        } else {
                            cout << UI::RED << "Extensao: " << UI::RESET << holder << UI::RED << " Não encontrado." << UI::RESET << "\n";
                        }
                    }
                    holder = ".";
                    break;
                case ' ':
                    break;
                default:
                    holder.push_back(e[i]);
                    break;
            }
        }

        auto it = remove_if(data.begin(), data.end(), [&holder](const Storage& s){ return s.ext == holder; });
        if (it != data.end()) {
            data.erase(it, data.end());
            savePaths();
            cout << UI::GREEN << "Extensao: " << UI::RESET << holder << " Removido." << UI::RESET << "\n";
        } else {
            cout << UI::RED << "Extensao: " << UI::RESET << holder << UI::RED << " Não encontrado." << UI::RESET << "\n";
        }

        pauseExec();
    }

    void searchPath() {
        searchInput("Buscar extensao");
    }
};

int main() {
    setupConsole();
    atexit(restoreConsole);

    Organizer app;
    app.run();

    cout << UI::CLEAR << "Saindo...\n";
    return 0;
}
