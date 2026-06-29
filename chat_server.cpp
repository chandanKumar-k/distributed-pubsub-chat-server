#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <map>
#include <algorithm>
#include <cstring>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

// =======================================================
// MODULE 1: INDIVIDUAL CHAT USER STRUCT REFERENCE
// =======================================================
struct ChatClient {
    SOCKET socket;
    string username;
    string room;
};
// Forward declare ChatServer for thread starter params
class ChatServer;

// Helper struct and thread starter for Win32 threads
struct ThreadParams {
    ChatServer* server;
    SOCKET sock;
};

// Forward declaration: defined after ChatServer so the type is complete
DWORD WINAPI clientThreadProc(LPVOID lpParam);

// =======================================================
// MODULE 2: DISTRIBUTED PUB-SUB CHAT CORE SERVER
// =======================================================
class ChatServer {
private:
    SOCKET listeningSocket;
    vector<ChatClient> clients;
    int portNumber;

public:
    ChatServer(int port) : portNumber(port) {}

    bool bootNetworkStack() {
        WSADATA wsaData;
        return (WSAStartup(MAKEWORD(2, 2), &wsaData) == 0);
    }

    // BROADCAST ALGORITHM: Sends messages ONLY to users in the same topic room channel
    void broadcastMessage(const string& message, string targetRoom, SOCKET senderSocket) {
        for (const auto& client : clients) {
            // Rule: Match room channels, but don't loop back and send the message to the sender
            if (client.room == targetRoom && client.socket != senderSocket) {
                send(client.socket, message.c_str(), message.size(), 0);
            }
        }
    }

    // MULTI-THREADED CLIENT WORKER: This runs concurrently for EVERY connected user!
    void handleClientWorker(SOCKET clientSocket) {
        char buffer[4096];
        string username = "User_" + to_string(clientSocket);
        string currentRoom = "Global_Lobby";

        // Register the new user inside our active structural vector array ledger
        ChatClient newClient = {clientSocket, username, currentRoom};
        clients.push_back(newClient);

        // Send protocol welcome card down the network socket stream pipe
        string welcome = "--- WELCOME TO PESU DISTRIBUTED CLOUD CHAT CLUSTER ---\r\n";
        welcome += "Your default handle is: " + username + "\r\n";
        welcome += "Available Commands: /join <room_name> | /name <new_name> | /quit\r\n";
        welcome += "======================================================\r\n\r\n";
        send(clientSocket, welcome.c_str(), welcome.size(), 0);

        broadcastMessage(">>> [SYSTEM]: " + username + " entered the lobby.\r\n", currentRoom, clientSocket);

        // Continuous packet intercept loop state
        while (true) {
            ZeroMemory(buffer, 4096);
            int bytesReceived = recv(clientSocket, buffer, 4096, 0);
            
            if (bytesReceived <= 0) {
                cout << ">>> [NETWORK SIGNAL]: Connection drop detected for client " << username << "\n";
                break;
            }

            string rawMsg(buffer, bytesReceived);
            // Clean up Windows carriage returns (\r\n) from incoming string tokens
            rawMsg.erase(remove(rawMsg.begin(), rawMsg.end(), '\r'), rawMsg.end());
            rawMsg.erase(remove(rawMsg.begin(), rawMsg.end(), '\n'), rawMsg.end());

            if (rawMsg.empty()) continue;

            // --- COMMAND ROUTING ROUTINES ---
            if (rawMsg.substr(0, 5) == "/join") {
                string oldRoom = currentRoom;
                string newRoom = rawMsg.substr(6);
                
                // Update room state across memory arrays
                currentRoom = newRoom;
                for (auto& c : clients) {
                    if (c.socket == clientSocket) { c.room = newRoom; break; }
                }

                broadcastMessage(">>> [SYSTEM]: " + username + " left this room to join #" + newRoom + "\r\n", oldRoom, clientSocket);
                string confirmation = ">>> [SYSTEM]: Successfully subscribed to topic channel #" + newRoom + "\r\n\r\n";
                send(clientSocket, confirmation.c_str(), confirmation.size(), 0);
                broadcastMessage(">>> [SYSTEM]: " + username + " subscribed to this topic channel.\r\n", currentRoom, clientSocket);
            } 
            else if (rawMsg.substr(0, 5) == "/name") {
                string oldName = username;
                string newName = rawMsg.substr(6);
                username = newName;
                for (auto& c : clients) {
                    if (c.socket == clientSocket) { c.username = newName; break; }
                }
                string confirmation = ">>> [SYSTEM]: Display handle changed to: " + newName + "\r\n\r\n";
                send(clientSocket, confirmation.c_str(), confirmation.size(), 0);
                broadcastMessage(">>> [SYSTEM]: " + oldName + " renamed to " + newName + "\r\n", currentRoom, clientSocket);
            } 
            else if (rawMsg == "/quit") {
                break;
            } 
            else {
                // Regular messaging chat pipeline routing data payload
                string formattedPayload = "[" + currentRoom + "] " + username + ": " + rawMsg + "\r\n";
                broadcastMessage(formattedPayload, currentRoom, clientSocket);
            }
        }

        // Clean up memory and trace arrays cleanly upon disconnection drop events
        broadcastMessage(">>> [SYSTEM]: " + username + " disconnected from network grid.\r\n", currentRoom, clientSocket);
        closesocket(clientSocket);
        
        // Remove from clients array securely
        for (auto it = clients.begin(); it != clients.end(); ++it) {
            if (it->socket == clientSocket) { clients.erase(it); break; }
        }
    }

    void startServer() {
        if (!bootNetworkStack()) return;

        listeningSocket = socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in serverHint;
        serverHint.sin_family = AF_INET;
        serverHint.sin_port = htons(portNumber);
        serverHint.sin_addr.S_un.S_addr = INADDR_ANY;

        bind(listeningSocket, (sockaddr*)&serverHint, sizeof(serverHint));
        listen(listeningSocket, SOMAXCONN);

        cout << "====================================================\n";
        cout << "   PESU DISTRIBUTED CHAT ENGINE CORE v3.0 ONLINE     \n";
        cout << "   Asynchronous Broker Core listening on Port: " << portNumber << "\n";
        cout << "====================================================\n\n";

        while (true) {
            SOCKET clientSocket = accept(listeningSocket, nullptr, nullptr);
            if (clientSocket != INVALID_SOCKET) {
                cout << ">>> [CONNECTION]: Spawning hardware thread for Socket ID: " << clientSocket << "\n";
                
                // CRUCIAL: Spawn a background thread to handle this user independently!
                // This keeps our main `accept()` loop free to grab the next user instantly.
                // Spawn a Win32 thread to handle the client (avoids std::thread portability issues)
                ThreadParams* params = new ThreadParams{ this, clientSocket };
                HANDLE hThread = CreateThread(nullptr, 0, clientThreadProc, params, 0, nullptr);
                if (hThread) CloseHandle(hThread);
            }
        }
    }

    ~ChatServer() {
        closesocket(listeningSocket);
        WSACleanup();
    }
};

DWORD WINAPI clientThreadProc(LPVOID lpParam) {
    ThreadParams* tp = static_cast<ThreadParams*>(lpParam);
    if (tp && tp->server) {
        tp->server->handleClientWorker(tp->sock);
    }
    delete tp;
    return 0;
}

int main() {
    ChatServer systemBroker(8080);
    systemBroker.startServer();
    return 0;
}
