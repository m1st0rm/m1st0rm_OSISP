#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <iostream>
#include <commdlg.h>
#include <fstream>
#include <shlobj.h>
#include <thread>
#pragma comment(lib, "ws2_32.lib")

HINSTANCE hInst;
HWND hwndButtonSelectDir, hwndButtonSendFile, hwnd;
WCHAR saveFolderPath[MAX_PATH] = L"C:\\Users\\Professional\\Desktop";
SOCKET clientSocket;
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

void ConnectServer();
void ReceiveFile();
void OpenFile(HWND hwnd);
void SelectDirectory(HWND hwnd);
void SocketListener();
void SendFile(const wchar_t* filePath);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    hInst = hInstance;

    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"MyWindowClass";

    if (!RegisterClass(&wc)) {
        MessageBox(NULL, L"Ошибка регистрации класса окна.", L"Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    hwnd = CreateWindow(
        L"MyWindowClass", L"FileSender", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 300,
        NULL, NULL, hInstance, NULL
    );

    if (hwnd == NULL) {
        MessageBox(NULL, L"Ошибка создания окна.", L"Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
    std::thread socketListenerThread(SocketListener);
    socketListenerThread.detach();

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    closesocket(clientSocket);
    WSACleanup();
    return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        // Создание кнопок
        hwndButtonSelectDir = CreateWindow(
            L"BUTTON", L"Выбрать директорию", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            100, 100, 200, 30, hwnd, (HMENU)2, hInst, NULL
        );

        hwndButtonSendFile = CreateWindow(
            L"BUTTON", L"Отправить файл", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            100, 150, 200, 30, hwnd, (HMENU)3, hInst, NULL
        );
        ConnectServer();
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case 2:
            SelectDirectory(hwnd);
            break;

        case 3:
            OpenFile(hwnd);
            break;
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return 0;
}

void SelectDirectory(HWND hwnd) {
    BROWSEINFO browseInfo = { 0 };
    browseInfo.hwndOwner = hwnd;
    browseInfo.pidlRoot = NULL;
    browseInfo.pszDisplayName = NULL;
    browseInfo.lpszTitle = L"Выберите директорию";
    browseInfo.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

    LPITEMIDLIST pidl = SHBrowseForFolder(&browseInfo);
    if (pidl != NULL) {
        SHGetPathFromIDList(pidl, saveFolderPath);
        CoTaskMemFree(pidl);
    }
}

void ConnectServer() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) == SOCKET_ERROR) {
        MessageBox(hwnd, L"Ошибка инициализации WinSock", L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) {
        MessageBox(hwnd, L"Ошибка создания сокета", L"Error", MB_OK | MB_ICONERROR);
        WSACleanup();
        return;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    if (inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr) != 1) {
        MessageBox(hwnd, L"Некорректный IP-адрес сервера", L"Error", MB_OK | MB_ICONERROR);
        WSACleanup();
        return;
    }
    serverAddr.sin_port = htons(8080);

    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        MessageBox(hwnd, L"Ошибка соединения с сервером", L"Error", MB_OK | MB_ICONERROR);
        closesocket(clientSocket);
        WSACleanup();
        return;
    }
    MessageBox(hwnd, L"Соеденинение с сервером успешно установлено", L"Уведомление", MB_OK);
}

void OpenFile(HWND hwnd) {
    OPENFILENAME ofn;
    wchar_t szFile[260];

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = szFile;
    ofn.lpstrFile[0] = '\0';
    ofn.nMaxFile = sizeof(szFile) / sizeof(szFile[0]);
    ofn.lpstrFilter = L"All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileName(&ofn) == TRUE) {
        SendFile(szFile);
    }
}


void SendFile(const wchar_t* filePath) {

    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        MessageBox(hwnd, L"Ошибка открытия файла для отправки", L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    file.seekg(0, std::ios::end);
    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    if (send(clientSocket, reinterpret_cast<const char*>(&fileSize), sizeof(fileSize), 0) <= 0) {
        MessageBox(hwnd, L"Ошибка отправки размера файла", L"Error", MB_OK | MB_ICONERROR);
        file.close();
        return;
    }

    const wchar_t* fileName = wcsrchr(filePath, L'\\');
    if (!fileName) {
        fileName = filePath;
    }
    else {
        fileName++;
    }

    if (send(clientSocket, reinterpret_cast<const char*>(fileName), sizeof(wchar_t) * MAX_PATH, 0) <= 0) {
        MessageBox(hwnd, L"Ошибка отправки имени файла", L"Error", MB_OK | MB_ICONERROR);
        file.close();
        return;
    }

    char buffer[1024];
    while (!file.eof()) {
        file.read(buffer, sizeof(buffer));

        if (send(clientSocket, buffer, file.gcount(), 0) <= 0) {
            MessageBox(hwnd, L"Ошибка отправки файла", L"Error", MB_OK | MB_ICONERROR);
            file.close();
            return;
        }
    }

    file.close();
    MessageBox(hwnd, L"Файл отправлен успешно", L"Success", MB_OK | MB_ICONINFORMATION);
}

void SocketListener() {
    while (true) {
        char buffer[1024];
        int bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);

        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';

            if (strcmp(buffer, "REQUEST_FILE_TRANSFER") == 0) {
                ReceiveFile();
            }

        }

    }
}

void ReceiveFile() {

    std::streamsize fileSize;
    int bytesRead = recv(clientSocket, reinterpret_cast<char*>(&fileSize), sizeof(fileSize), 0);

    if (bytesRead <= 0) {
        MessageBox(hwnd, L"Ошибка получения размера файла", L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    wchar_t filePath[MAX_PATH];
    bytesRead = recv(clientSocket, reinterpret_cast<char*>(filePath), sizeof(filePath), 0);

    if (bytesRead <= 0) {
        MessageBox(hwnd, L"Ошибка получения имени файла", L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    std::wstring fullFilePath = saveFolderPath;
    fullFilePath += L"\\";
    fullFilePath += filePath;

    std::ofstream receivedFile(fullFilePath, std::ios::binary);
    char buffer[1024];
    while (fileSize > 0) {
        bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);

        if (bytesRead <= 0) {
            MessageBox(hwnd, L"Ошибка получения содержимого файла", L"Error", MB_OK | MB_ICONERROR);
            return;
        }

        receivedFile.write(buffer, bytesRead);
        fileSize -= bytesRead;
    }

    receivedFile.close();
    MessageBox(hwnd, L"Файл получен успешно", L"Success", MB_OK | MB_ICONINFORMATION);
}