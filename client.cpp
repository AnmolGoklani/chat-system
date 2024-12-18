#include <iostream>
#include <thread>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

using namespace std;

void receive_messages(int sock) {
    char buffer[1024];
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(sock, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) {
            cout << "\n[INFO] Disconnected from server.\n";
            break;
        }
        cout << "\n\033[1;32m[Message Received]\033[0m\n";
        cout << "----------------------------------\n";
        cout << buffer << endl;
        cout << "----------------------------------\n";
        cout << "To (username): ";
    }
}


int main() {
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    connect(client_socket, (sockaddr*)&server_addr, sizeof(server_addr));

    string username;
    cout << "Enter username: ";
    cin >> username;
    send(client_socket, username.c_str(), username.size(), 0);

    thread(receive_messages, client_socket).detach();

    string recipient, message;
    while (true) {
        cout << "To (username): ";
        cin >> recipient;
        cin.ignore();
        cout << "Message: ";
        getline(cin, message);

        string full_message = recipient + ":" + message;
        send(client_socket, full_message.c_str(), full_message.size(), 0);
    }

    close(client_socket);
    return 0;
}
