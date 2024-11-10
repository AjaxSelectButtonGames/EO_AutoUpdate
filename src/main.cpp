#include "Atlas_Config.h"
#include "EO_Packets.h"
#include <ws2tcpip.h>  // For getaddrinfo
#include <stdexcept>   // For std::invalid_argument
#include "UI.h"
#include <thread>
#include <atomic>

std::atomic<bool> uiRunning(true);

void uiThreadFunction(UI& ui) {
    ui.renderUI();
    uiRunning = false;
}

int main() {
    // Read configuration
    UI ui;
    std::thread uiThread(uiThreadFunction, std::ref(ui));

    Config config;
    if (!config.Read("config/setup.ini")) {
        std::cerr << "Failed to read configuration file.\n";
        ui.updateText("Failed to read configuration file.");
        uiRunning = false;
        uiThread.join();
        return 1;
    }

    std::string host = config["CONNECTION"]["Host"];
    std::string portStr = config["CONNECTION"]["Port"];
    std::string versionMajorStr = config["VERSION"]["version_major"];
    std::string versionMinorStr = config["VERSION"]["version_minor"];
    std::string versionPatchStr = config["VERSION"]["version_patch"];

    int port;
    unsigned char version_major, version_minor, version_patch;

    try {
        port = std::stoi(portStr);
        version_major = static_cast<unsigned char>(std::stoi(versionMajorStr));
        version_minor = static_cast<unsigned char>(std::stoi(versionMinorStr));
        version_patch = static_cast<unsigned char>(std::stoi(versionPatchStr));
    }
    catch (const std::invalid_argument& e) {
        std::cerr << "Invalid version or port number in configuration file: " << e.what() << "\n";
        ui.updateText("Invalid version or port number in configuration file.");
        uiRunning = false;
        uiThread.join();
        return 1;
    }
    catch (const std::out_of_range& e) {
        std::cerr << "Version or port number out of range in configuration file: " << e.what() << "\n";
        ui.updateText("Version or port number out of range in configuration file.");
        uiRunning = false;
        uiThread.join();
        return 1;
    }

    WSADATA wsaData;
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct addrinfo* result = NULL;
    struct addrinfo hints;

    // Initialize Winsock
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        std::cerr << "WSAStartup failed: " << iResult << "\n";
        uiRunning = false;
        uiThread.join();
        return 1;
    }
    std::cout << "Winsock initialized.\n";

    // Set up the hints address info structure
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;  // Allow IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    iResult = getaddrinfo(host.c_str(), portStr.c_str(), &hints, &result);
    if (iResult != 0) {
        std::cerr << "getaddrinfo failed: " << iResult << "\n";
        WSACleanup();
        uiRunning = false;
        uiThread.join();
        return 1;
    }

    // Attempt to connect to the first address returned by the call to getaddrinfo
    for (struct addrinfo* ptr = result; ptr != NULL; ptr = ptr->ai_next) {
        // Create a SOCKET for connecting to the server
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET) {
            std::cerr << "Error creating socket: " << WSAGetLastError() << "\n";
            WSACleanup();
            uiRunning = false;
            uiThread.join();
            return 1;
        }

        // Connect to the server
        iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (ConnectSocket == INVALID_SOCKET) {
        std::cerr << "Unable to connect to server.\n";
        WSACleanup();
        uiRunning = false;
        uiThread.join();
        return 1;
    }
    std::cout << "Connected to server.\n";

    // Send INIT_INIT packet
    std::vector<unsigned char> initPacket = createInitPacket(version_major, version_minor, version_patch);

    // Print packet contents for debugging
    std::cout << "Packet contents: ";
    for (unsigned char byte : initPacket) {
        printf("%02X ", byte);
    }
    std::cout << "\n";

    iResult = send(ConnectSocket, reinterpret_cast<const char*>(initPacket.data()), initPacket.size(), 0);
    if (iResult == SOCKET_ERROR) {
        std::cerr << "Send failed: " << WSAGetLastError() << "\n";
        ui.updateText("Send failed.");
        closesocket(ConnectSocket);
        WSACleanup();
        uiRunning = false;
        uiThread.join();
        return 1;
    }
    std::cout << "Packet sent.\n";
    ui.updateText("Packet sent.");
    ui.clearScreen();
    // Receive response from server
    char recvbuf[512];
    iResult = recv(ConnectSocket, recvbuf, sizeof(recvbuf), 0);
    if (iResult > 0) {
        std::vector<unsigned char> response(recvbuf, recvbuf + iResult);
        std::cout << "Response received.\n";
        ui.updateText("Response received.");
        parseServerResponse(response, ui);
    }
    else if (iResult == 0) {
        std::cout << "Connection closed by server.\n";
        ui.updateText("Connection closed by server.");
    }
    else {
        std::cerr << "recv failed: " << WSAGetLastError() << "\n";
        ui.updateText("recv failed.");
    }

    // Cleanups
    closesocket(ConnectSocket);
    WSACleanup();

    // Signal the UI thread to stop and wait for it to finish
    uiRunning = false;
    uiThread.join();
    return 0;
}
