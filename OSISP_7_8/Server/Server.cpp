#include <iostream>
#include <winsock2.h>
#include <fstream>
#include <vector>
#include <thread>

#pragma comment(lib, "ws2_32.lib")

void HandleClient(SOCKET clientSocket, std::vector<SOCKET>& clients, const std::wstring& serverFolderPath);
void BroadcastFile(const wchar_t* filePath, const std::vector<SOCKET>& clients, SOCKET senderClient);
int main() {
    setlocale(LC_ALL, "Russian");
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Ошибка инициализации WinSock" << std::endl;
        return 1;
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Ошибка создания сокета" << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8080);

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Ошибка связывания сокета и адреса" << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "При прослушивании прорта 8080 произошла ошибка" << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Сервер прослушивает порт 8080" << std::endl;

    std::vector<SOCKET> clients;
    std::wstring serverFolderPath = L"M:\\ServerFiles\\";

    while (true) {
        SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Ошибка подключение клиента" << WSAGetLastError() << std::endl;
            closesocket(serverSocket);
            WSACleanup();
            return 1;
        }
        std::cout << "Подключен клиент." << std::endl;

        clients.push_back(clientSocket);

        std::thread(HandleClient, clientSocket, std::ref(clients), serverFolderPath).detach();
    }

    closesocket(serverSocket);
    WSACleanup();

    return 0;
}

void HandleClient(SOCKET clientSocket, std::vector<SOCKET>& clients, const std::wstring& serverFolderPath) {
    while (true) {
        std::streamsize fileSize;
        int bytesRead = recv(clientSocket, reinterpret_cast<char*>(&fileSize), sizeof(fileSize), 0);

        if (bytesRead <= 0) {
            std::cerr << "Ошибка получения размера файла или отключение клиента" << std::endl;
            return;
        }

        wchar_t fileName[MAX_PATH];
        bytesRead = recv(clientSocket, reinterpret_cast<char*>(fileName), sizeof(fileName), 0);

        if (bytesRead <= 0) {
            std::cerr << "Ошибка получения имени файла" << std::endl;
            return;
        }

        std::wstring fullFilePath = serverFolderPath + L"\\" + fileName;

        std::ofstream receivedFile(fullFilePath.c_str(), std::ios::binary);
        while (fileSize > 0) {
            char buffer[1024];
            bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);

            if (bytesRead <= 0) {
                std::cerr << "Ошибка получения содержимого файла" << std::endl;
                return;
            }

            receivedFile.write(buffer, bytesRead);
            fileSize -= bytesRead;
        }

        receivedFile.close();
        std::cout << "Файл получен успешно" << std::endl;

        BroadcastFile(fileName, clients, clientSocket);
    }
}


void BroadcastFile(const wchar_t* filePath, const std::vector<SOCKET>& clients, SOCKET senderClient) {
    std::wstring serverFolderPath = L"M:\\ServerFiles\\";
    std::wstring NewFilePath = serverFolderPath + filePath;
    std::ifstream file(NewFilePath, std::ios::binary);
    if (!file) {
        std::cerr << "Ошибка открытия файла для транслирования" << std::endl;
        return;
    }

    file.seekg(0, std::ios::end);
    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    const char* request = "REQUEST_FILE_TRANSFER";
    for (SOCKET clientSocket : clients) {
        if (clientSocket != senderClient) {
            send(clientSocket, request, strlen(request), 0);
        }
    }
    for (SOCKET clientSocket : clients) {
        if (clientSocket != senderClient) {
            send(clientSocket, reinterpret_cast<const char*>(&fileSize), sizeof(fileSize), 0);
            send(clientSocket, reinterpret_cast<const char*>(filePath), sizeof(wchar_t) * MAX_PATH, 0);
        }
    }

    char buffer[1024];
    while (!file.eof()) {
        file.read(buffer, sizeof(buffer));

        for (SOCKET clientSocket : clients) {
            if (clientSocket != senderClient) {
                send(clientSocket, buffer, file.gcount(), 0);
            }
        }
    }

    file.close();
    std::cout << "Файл транслирован."  << std::endl;
}