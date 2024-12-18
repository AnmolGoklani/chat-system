#include <iostream>
#include <string>
#include <cstring>
#include <thread>
#include <vector>
#include <map>
#include <mutex>
#include <fstream>
#include <netinet/in.h>
#include <unistd.h>

using namespace std;

#define PORT 8080

map<int, string> client_usernames;
mutex client_mutex;

void log_message(const string& message) {
    ofstream logfile("chat_log.txt", ios::app); 
    if (logfile.is_open()) {
        logfile << message << endl; 
        logfile.close();
    } else {
        cerr << "[ERROR] Unable to open chat log file.\n";
    }
}

void send_message(int client_socket, const string& message) {
    send(client_socket, message.c_str(), message.size(), 0);
}

void handle_client(int client_socket) {
    char buffer[1024];
    string username;

    memset(buffer, 0, sizeof(buffer));
    int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
    if (bytes_received > 0) {
        username = string(buffer);
        {
            lock_guard<mutex> lock(client_mutex);
            client_usernames[client_socket] = username;
        }
        cout << "[INFO] User connected: " << username << "\n";
        log_message("[INFO] User connected: " + username);
    }

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);

        if (bytes_received <= 0) {
            cout << "[INFO] User disconnected: " << username << "\n";
            log_message("[INFO] User disconnected: " + username);
            {
                lock_guard<mutex> lock(client_mutex);
                client_usernames.erase(client_socket);
            }
            close(client_socket);
            break;
        }

        string message(buffer);

        size_t delimiter_pos = message.find(':');
        if (delimiter_pos != string::npos) {
            string target_user = message.substr(0, delimiter_pos);
            string chat_message = message.substr(delimiter_pos + 1);

            {
                lock_guard<mutex> lock(client_mutex);
                bool user_found = false;

                for (const auto& pair : client_usernames) {
                    if (pair.second == target_user) {
                        string full_message = username + " to " + target_user + ": " + chat_message;
                        send_message(pair.first, full_message);
                        log_message(full_message);
                        user_found = true;
                        break;
                    }
                }

                if (!user_found) {
                    send_message(client_socket, "User not found: " + target_user);
                }
            }
        } else {
            send_message(client_socket, "Invalid message format. Use: <username>:<message>");
        }
    }
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Create socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    sockaddr_in server_addr;

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    cout << "[INFO] Server started on port " << PORT << "\n";

    while (true) {
        if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        thread(handle_client, new_socket).detach();
    }

    return 0;
}
