#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <vector>
#include <curl/curl.h>
#include "zip_file.hpp"
#include <direct.h> // For _mkdir
#include <windows.h> // For CreateDirectory
#include <regex>
#include <fstream>
#include <sstream>
#include "UI.h"



#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "libcurl.lib")

// Server connection details (replace with actual server IP and port)
const char* SERVER_IP = "69.10.49.30";
const int SERVER_PORT = 8078;

// Function to create the INIT_INIT packet
std::vector<unsigned char> createInitPacket(unsigned char version_major, unsigned char version_minor, unsigned char version_patch) {
    unsigned char packet_length = 0x14;
    unsigned char null_terminator = 0xFE;
    unsigned char packet_family = 0xFF;
    unsigned char packet_action = 0xFF;
    unsigned char challenge[] = { 0x88, 0x7D, 0x09 };
    unsigned char additional_value = 0x72;
    unsigned char hdid_length = 0x0A;
    const char* hdid = "8756433135";  // Example HDID, 10 bytes

    // Build the packet using a vector
    std::vector<unsigned char> packet;
    packet.push_back(packet_length);
    packet.push_back(null_terminator);
    packet.push_back(packet_family);
    packet.push_back(packet_action);
    packet.insert(packet.end(), std::begin(challenge), std::end(challenge));
    packet.push_back(version_major);
    packet.push_back(version_minor);
    packet.push_back(version_patch);
    packet.push_back(additional_value);
    packet.push_back(hdid_length);
    packet.insert(packet.end(), hdid, hdid + hdid_length);

    return packet;
}


// Helper function to write data to a file
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    FILE* fp = (FILE*)userp;
    fwrite(contents, size, nmemb, fp);
    return totalSize;
}



// Function to download a file from a URL
bool downloadFile(const std::string& url, const std::string& outputPath, bool isHtml = false) {
    CURL* curl;
    FILE* fp;
    CURLcode res;

    curl = curl_easy_init();
    if (curl) {
        fopen_s(&fp, outputPath.c_str(), isHtml ? "w" : "wb");
        if (fp == nullptr) {
            std::cerr << "Failed to open file: " << outputPath << "\n";
            curl_easy_cleanup(curl);
            return false;
        }

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

        // Enable following redirects
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        // Optionally, set a maximum number of redirects to prevent infinite loops
        curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);

        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        fclose(fp);
        return (res == CURLE_OK);
    }
    return false;
}



// Function to convert std::string to std::wstring
std::wstring stringToWString(const std::string& str) {
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

// Function to extract a ZIP file using zip_file.hpp
bool extractZip(const std::string& zipPath, const std::string& extractPath) {
    try {
        miniz_cpp::zip_file file(zipPath);

        for (auto& entry : file.infolist()) {
            std::string outPath = extractPath + "/" + entry.filename;

            // Check if the entry is in the config/ directory
            if (entry.filename.find("config/") == 0) {
                std::cout << "Skipping extraction of: " << entry.filename << "\n";
                continue; // Skip this entry
            }

            if (entry.filename.back() == '/') {
                // Create directory
                std::wstring wOutPath = stringToWString(outPath);
                CreateDirectory(wOutPath.c_str(), NULL);
            }
            else {
                // Extract file
                std::ofstream outFile(outPath, std::ios::binary);
                outFile << file.read(entry.filename);
                outFile.close();
            }
        }
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to extract ZIP file: " << e.what() << "\n";
        return false;
    }
}


std::string extractZipLink(const std::string& htmlFilePath) {
    std::ifstream htmlFile(htmlFilePath);
    std::stringstream buffer;
    buffer << htmlFile.rdbuf();
    std::string htmlContent = buffer.str();
    htmlFile.close();
    // Updated regex pattern to match href with client/0_04/ and ending in .zip
    std::regex zipRegex(R"(<a\s+[^>]*href=['\"](https?://www\.endless-online\.com/client/0_04/[^\"']+\.zip)['\"][^>]*>)");
    std::smatch match;
    if (std::regex_search(htmlContent, match, zipRegex)) {
        return match[1].str();  // Return the matched URL
    }
    return "";
}

void updateConfigFile(const std::string& filePath, unsigned char major, unsigned char minor, unsigned char patch) {
    std::ifstream configFile(filePath);
    if (!configFile.is_open()) {
        std::cerr << "Failed to open configuration file for reading.\n";
        return;
    }

    std::stringstream buffer;
    buffer << configFile.rdbuf();
    std::string content = buffer.str();
    configFile.close();

    // Update the version information
    std::regex majorRegex(R"(version_major=\d+)");
    std::regex minorRegex(R"(version_minor=\d+)");
    std::regex patchRegex(R"(version_patch=\d+)");
    content = std::regex_replace(content, majorRegex, "version_major=" + std::to_string(major));
    content = std::regex_replace(content, minorRegex, "version_minor=" + std::to_string(minor));
    content = std::regex_replace(content, patchRegex, "version_patch=" + std::to_string(patch));

    std::ofstream outFile(filePath);
    if (!outFile.is_open()) {
        std::cerr << "Failed to open configuration file for writing.\n";
        return;
    }
    outFile << content;
    outFile.close();
}


// Function to parse the server's response
void parseServerResponse(const std::vector<unsigned char>& response, UI& ui) {
    std::cout << "Received raw response: ";
    for (unsigned char byte : response) {
        printf("%02X ", byte);
    }
    std::cout << "\nResponse length: " << response.size() << " bytes\n";

    if (response.size() < 7) {
        std::cout << "Response is too short to contain required fields.\n";
        ui.UIText = "Response is too short to contain required fields.";
        return;
    }

    unsigned char packet_length = response[0];
    unsigned char null_terminator = response[1];
    unsigned char packet_family = response[2];
    unsigned char packet_action = response[3];
    unsigned char reply_code = response[4];

    if (reply_code == 1 && packet_length == 0x3C) {
        if (response.size() < 10) {
            std::cout << "Response is too short for OutOfDate reply.\n";
            ui.UIText = "Response is too short for OutOfDate reply.";
            return;
        }
        unsigned char major = response[5];
        unsigned char minor = response[6];
        unsigned char patch = response[7];
        std::cout << "Server reply: OutOfDate. Required version: "
            << static_cast<int>(major) << "."
            << static_cast<int>(minor) << "."
            << static_cast<int>(patch) << "\n";
        std::string extra_info(response.begin() + 8, response.end());
        std::cout << "Additional information: " << extra_info << "\n";

        // Update UI text
        ui.UIText = "Server reply: OutOfDate. Required version: " +
            std::to_string(static_cast<int>(major)) + "." +
            std::to_string(static_cast<int>(minor)) + "." +
            std::to_string(static_cast<int>(patch));

        // Extract URL from extra_info
        std::string url = extra_info.substr(extra_info.find("http"));
        std::cout << "Downloading update from: " << url << "\n";

        // Download the HTML file
        std::string htmlPath = "download.html";
        if (downloadFile(url, htmlPath, true)) {
            std::cout << "HTML download successful.\n";
            ui.UIText = "HTML download successful.";

            // Extract the ZIP file link from the HTML
            std::string zipUrl = extractZipLink(htmlPath);
            if (!zipUrl.empty()) {
                std::cout << "Found ZIP file link: " << zipUrl << "\n";
                ui.UIText = "Found ZIP file link: " + zipUrl;

                // Download the ZIP file
                std::string zipPath = "update.zip";
                if (downloadFile(zipUrl, zipPath)) {
                    std::cout << "ZIP download successful.\n";
                    ui.UIText = "ZIP download successful.";

                    // Extract the ZIP file
                    if (extractZip(zipPath, ".")) {
                        std::cout << "Extraction successful.\n";
                        ui.UIText = "Extraction successful.";
                        SDL_Delay(300);
                        ui.UIText = "Updated! Complete please close program";
                        updateConfigFile("config/setup.ini", major, minor, patch);
                    }
                    else {
                        std::cerr << "Extraction failed.\n";
                        ui.UIText = "Extraction failed.";
                    }
                }
                else {
                    std::cerr << "ZIP download failed.\n";
                    ui.UIText = "ZIP download failed.";
                }
            }
            else {
                std::cerr << "Failed to find ZIP file link in HTML.\n";
                ui.UIText = "Failed to find ZIP file link in HTML.";
            }
        }
        else {
            std::cerr << "HTML download failed.\n";
            ui.UIText = "HTML download failed.";
        }
    }
    else if (reply_code == 2 && packet_length == 0x10) {
        if (response.size() < 17) {
            std::cout << "Response is too short for Ok reply.\n";
            ui.UIText = "Response is too short for Ok reply.";
            return;
        }
        unsigned char seq1 = response[5];
        unsigned char seq2 = response[6];
        unsigned char server_encryption_multiple = response[7];
        unsigned char client_encryption_multiple = response[8];
        unsigned short player_id = (response[9] << 8) | response[10];
        std::cout << "Server reply: Ok. seq1: " << static_cast<int>(seq1)
            << ", seq2: " << static_cast<int>(seq2)
            << ", server_encryption_multiple: " << static_cast<int>(server_encryption_multiple)
            << ", client_encryption_multiple: " << static_cast<int>(client_encryption_multiple)
            << ", player_id: " << player_id << "\n";
        ui.UIText = "Server reply: Ok. seq1: " + std::to_string(static_cast<int>(seq1)) +
            ", seq2: " + std::to_string(static_cast<int>(seq2)) +
            ", server_encryption_multiple: " + std::to_string(static_cast<int>(server_encryption_multiple)) +
            ", client_encryption_multiple: " + std::to_string(static_cast<int>(client_encryption_multiple)) +
            ", player_id: " + std::to_string(player_id);
    }
    else if (reply_code == 3) {
        std::cout << "Server reply: Banned. Packet length: " << static_cast<int>(packet_length)
            << ", Reply code: " << static_cast<int>(reply_code) << "\n";
        ui.UIText = "Server reply: Banned. Packet length: " + std::to_string(static_cast<int>(packet_length)) +
            ", Reply code: " + std::to_string(static_cast<int>(reply_code));
    }
    else {
        std::cout << "Unexpected reply code or packet length: " << static_cast<int>(reply_code)
            << " with packet length " << static_cast<int>(packet_length) << "\n";
        ui.UIText = "Unexpected reply code or packet length: " + std::to_string(static_cast<int>(reply_code)) +
            " with packet length " + std::to_string(static_cast<int>(packet_length));
    }
}
