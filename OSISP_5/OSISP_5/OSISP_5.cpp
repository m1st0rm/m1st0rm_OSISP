#include <windows.h>
#include <CommCtrl.h>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <Shellapi.h>
#include <Msi.h>
#include <commdlg.h>
#include <shlobj.h>

#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "Msi.lib")

#define IDC_LISTVIEW 1000
#define IDC_LOAD_BUTTON 1001
#define IDC_UNINSTALL_BUTTON 1002
#define IDC_INSTALL_BUTTON 1003
#define IDC_UPDATE_BUTTON 1004

HWND listView;
HWND loadButton;
HWND uninstallButton;
HWND installButton;
HWND updateButton;
bool isMsgNeeded = false;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
bool LaunchUninstallerForApp(DWORD registryBits, const std::wstring& appName);

void GetInstalledApps() {
    std::vector<std::wstring> apps;

    HKEY hKey64;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall", 0, KEY_READ | KEY_WOW64_64KEY, &hKey64) == ERROR_SUCCESS) {
        DWORD index = 0;
        WCHAR subKeyName[255];
        DWORD subKeyNameSize = sizeof(subKeyName) / sizeof(subKeyName[0]);

        while (RegEnumKeyEx(hKey64, index, subKeyName, &subKeyNameSize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
            HKEY appKey;
            if (RegOpenKeyEx(hKey64, subKeyName, 0, KEY_READ, &appKey) == ERROR_SUCCESS) {
                WCHAR displayName[255];
                DWORD displayNameSize = sizeof(displayName) / sizeof(displayName[0]);

                if (RegQueryValueEx(appKey, L"DisplayName", NULL, NULL, (LPBYTE)displayName, &displayNameSize) == ERROR_SUCCESS) {
                    WCHAR uninstallString[255];
                    DWORD uninstallStringSize = sizeof(uninstallString) / sizeof(uninstallString[0]);
                    if (RegQueryValueEx(appKey, L"UninstallString", NULL, NULL, (LPBYTE)uninstallString, &uninstallStringSize) == ERROR_SUCCESS) {
                        
                            if (_wcsnicmp(uninstallString, L"MsiExec", 7) != 0) {
                                apps.push_back(displayName);
                            }
                        
                    }
                }

                RegCloseKey(appKey);
            }

            index++;
            subKeyNameSize = sizeof(subKeyName) / sizeof(subKeyName[0]);
        }

        RegCloseKey(hKey64);
    }

    HKEY hKey32;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall", 0, KEY_READ | KEY_WOW64_32KEY, &hKey32) == ERROR_SUCCESS) {
        DWORD index = 0;
        WCHAR subKeyName[255];
        DWORD subKeyNameSize = sizeof(subKeyName) / sizeof(subKeyName[0]);

        while (RegEnumKeyEx(hKey32, index, subKeyName, &subKeyNameSize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
            HKEY appKey;
            if (RegOpenKeyEx(hKey32, subKeyName, 0, KEY_READ, &appKey) == ERROR_SUCCESS) {
                WCHAR displayName[255];
                DWORD displayNameSize = sizeof(displayName) / sizeof(displayName[0]);

                if (RegQueryValueEx(appKey, L"DisplayName", NULL, NULL, (LPBYTE)displayName, &displayNameSize) == ERROR_SUCCESS) {
                    WCHAR uninstallString[255];
                    DWORD uninstallStringSize = sizeof(uninstallString) / sizeof(uninstallString[0]);
                    if (RegQueryValueEx(appKey, L"UninstallString", NULL, NULL, (LPBYTE)uninstallString, &uninstallStringSize) == ERROR_SUCCESS) {
                        
                            if (_wcsnicmp(uninstallString, L"MsiExec", 7) != 0) {
                                apps.push_back(displayName);
                            }
                        
                    }
                }

                RegCloseKey(appKey);
            }

            index++;
            subKeyNameSize = sizeof(subKeyName) / sizeof(subKeyName[0]);
        }

        RegCloseKey(hKey32);
    }

    HKEY hKeyCurrentUser;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall", 0, KEY_READ, &hKeyCurrentUser) == ERROR_SUCCESS) {
        DWORD index = 0;
        WCHAR subKeyName[255];
        DWORD subKeyNameSize = sizeof(subKeyName) / sizeof(subKeyName[0]);

        while (RegEnumKeyEx(hKeyCurrentUser, index, subKeyName, &subKeyNameSize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
            HKEY appKey;
            if (RegOpenKeyEx(hKeyCurrentUser, subKeyName, 0, KEY_READ, &appKey) == ERROR_SUCCESS) {
                WCHAR displayName[255];
                DWORD displayNameSize = sizeof(displayName) / sizeof(displayName[0]);

                if (RegQueryValueEx(appKey, L"DisplayName", NULL, NULL, (LPBYTE)displayName, &displayNameSize) == ERROR_SUCCESS) {
                    WCHAR uninstallString[255];
                    DWORD uninstallStringSize = sizeof(uninstallString) / sizeof(uninstallString[0]);
                    if (RegQueryValueEx(appKey, L"UninstallString", NULL, NULL, (LPBYTE)uninstallString, &uninstallStringSize) == ERROR_SUCCESS) {
                        if (_wcsnicmp(uninstallString, L"MsiExec", 7) != 0) {
                            apps.push_back(displayName);
                        }
                    }
                }

                RegCloseKey(appKey);
            }

            index++;
            subKeyNameSize = sizeof(subKeyName) / sizeof(subKeyName[0]);
        }

        RegCloseKey(hKeyCurrentUser);
    }

    ListView_DeleteAllItems(listView);

    for (size_t i = 0; i < apps.size(); ++i) {
        LVITEM lvItem;
        lvItem.mask = LVIF_TEXT;
        lvItem.iItem = static_cast<int>(i);
        lvItem.iSubItem = 0;
        lvItem.pszText = const_cast<LPWSTR>(apps[i].c_str());
        ListView_InsertItem(listView, &lvItem);
    }
}




void UninstallSelectedApp() {
    int selectedIndex = ListView_GetNextItem(listView, -1, LVNI_SELECTED);

    if (selectedIndex != -1) {
        LVITEM lvItem;
        lvItem.iItem = selectedIndex;
        lvItem.iSubItem = 0;
        lvItem.mask = LVIF_TEXT;
        lvItem.cchTextMax = 255;
        lvItem.pszText = new WCHAR[255];
        ListView_GetItem(listView, &lvItem);

        std::wstring appName = lvItem.pszText;
        delete[] lvItem.pszText;

        if (LaunchUninstallerForApp(KEY_WOW64_64KEY, appName)) {
            return;
        }

        if (LaunchUninstallerForApp(KEY_WOW64_32KEY, appName)) {
            return;
        }
    }
}


bool LaunchUninstallerForApp(DWORD registryBits, const std::wstring& appName) {
    HKEY hKey;
    DWORD accessFlags = KEY_READ | registryBits;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall", 0, accessFlags, &hKey) == ERROR_SUCCESS) {
        DWORD index = 0;
        WCHAR subKeyName[255];
        DWORD subKeyNameSize = sizeof(subKeyName) / sizeof(subKeyName[0]);

        while (RegEnumKeyEx(hKey, index, subKeyName, &subKeyNameSize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
            HKEY appKey;
            if (RegOpenKeyEx(hKey, subKeyName, 0, KEY_READ, &appKey) == ERROR_SUCCESS) {
                WCHAR displayName[255];
                DWORD displayNameSize = sizeof(displayName) / sizeof(displayName[0]);
                if (RegQueryValueEx(appKey, L"DisplayName", NULL, NULL, (LPBYTE)displayName, &displayNameSize) == ERROR_SUCCESS) {
                    if (appName == displayName) {
                        WCHAR uninstallString[255];
                        DWORD uninstallStringSize = sizeof(uninstallString) / sizeof(uninstallString[0]);
                        if (RegQueryValueEx(appKey, L"UninstallString", NULL, NULL, (LPBYTE)uninstallString, &uninstallStringSize) == ERROR_SUCCESS) {
                                HINSTANCE result = ShellExecute(0, L"open", uninstallString, 0, 0, SW_SHOWNORMAL);
                                if ((intptr_t)result > 32) {
                                    return true;
                                }
                                _wsystem(uninstallString);
                                RegCloseKey(appKey);
                                RegCloseKey(hKey);
                                return true;
                        }
                    }
                }

                RegCloseKey(appKey);
            }

            index++;
            subKeyNameSize = sizeof(subKeyName) / sizeof(subKeyName[0]);
        }

        RegCloseKey(hKey);
    }

    HKEY hKeyCurrentUser;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall", 0, accessFlags, &hKeyCurrentUser) == ERROR_SUCCESS) {
        DWORD index = 0;
        WCHAR subKeyName[255];
        DWORD subKeyNameSize = sizeof(subKeyName) / sizeof(subKeyName[0]);

        while (RegEnumKeyEx(hKeyCurrentUser, index, subKeyName, &subKeyNameSize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
            HKEY appKey;
            if (RegOpenKeyEx(hKeyCurrentUser, subKeyName, 0, KEY_READ, &appKey) == ERROR_SUCCESS) {
                WCHAR displayName[255];
                DWORD displayNameSize = sizeof(displayName) / sizeof(displayName[0]);
                if (RegQueryValueEx(appKey, L"DisplayName", NULL, NULL, (LPBYTE)displayName, &displayNameSize) == ERROR_SUCCESS) {
                    if (appName == displayName) {
                        WCHAR uninstallString[255];
                        DWORD uninstallStringSize = sizeof(uninstallString) / sizeof(uninstallString[0]);
                        if (RegQueryValueEx(appKey, L"UninstallString", NULL, NULL, (LPBYTE)uninstallString, &uninstallStringSize) == ERROR_SUCCESS) {
                            HINSTANCE result = ShellExecute(0, L"open", uninstallString, 0, 0, SW_SHOWNORMAL);
                            if ((intptr_t)result > 32) {
                                return true;
                            }
                            _wsystem(uninstallString);
                            RegCloseKey(appKey);
                            RegCloseKey(hKeyCurrentUser);
                            return true;
                        }
                    }
                }

                RegCloseKey(appKey);
            }

            index++;
            subKeyNameSize = sizeof(subKeyName) / sizeof(subKeyName[0]);
        }

        RegCloseKey(hKeyCurrentUser);
    }


    return false;
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASS windowClass = {};
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = GetModuleHandle(0);
    windowClass.lpszClassName = L"WindowClass";
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icex);
    RegisterClass(&windowClass);

    HWND hwnd = CreateWindowEx(
        0,
        L"WindowClass",
        L"Установка/удаление программ",
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX & ~WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT,
        1280, 700,
        0, 0,
        GetModuleHandle(0),
        0
    );

    if (hwnd == NULL) {
        return 1;
    }

    listView = CreateWindowEx(
        0,
        WC_LISTVIEW,
        L"",
        WS_CHILD | WS_VISIBLE | LVS_REPORT,
        10, 10, 1200, 600,
        hwnd,
        (HMENU)IDC_LISTVIEW,
        GetModuleHandle(0),
        0
    );

    LVCOLUMN lvColumn1;
    lvColumn1.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvColumn1.fmt = LVCFMT_LEFT;
    lvColumn1.cx = 1200;
    lvColumn1.pszText = const_cast<LPWSTR>(L"Название приложения");
    lvColumn1.iSubItem = 0;

    ListView_InsertColumn(listView, 0, &lvColumn1);

    loadButton = CreateWindow(
        L"BUTTON",
        L"Load",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        10, 620, 100, 30,
        hwnd,
        (HMENU)IDC_LOAD_BUTTON,
        GetModuleHandle(0),
        NULL
    );
    uninstallButton = CreateWindow(
        L"BUTTON",
        L"Uninstall",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        120, 620, 100, 30,
        hwnd,
        (HMENU)IDC_UNINSTALL_BUTTON,
        GetModuleHandle(0),
        NULL
    );
    installButton = CreateWindow(
        L"BUTTON",
        L"Install",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        230, 620, 100, 30,
        hwnd,
        (HMENU)IDC_INSTALL_BUTTON,
        GetModuleHandle(0),
        NULL
    );
    updateButton = CreateWindow(
        L"BUTTON",
        L"Update",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        340, 620, 100, 30,
        hwnd,
        (HMENU)IDC_UPDATE_BUTTON,
        GetModuleHandle(0),
        NULL
    );

    EnableWindow(installButton, FALSE);
    EnableWindow(uninstallButton, FALSE);
    EnableWindow(updateButton, FALSE);
    ShowWindow(hwnd, SW_SHOWNORMAL);
    UpdateWindow(hwnd);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

void RunInstaller() {
    COMDLG_FILTERSPEC filters[] = { L"Setup Executables", L"*.exe" };

    IFileOpenDialog* openFileDialog;
    HRESULT result = CoCreateInstance(
        CLSID_FileOpenDialog,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&openFileDialog)
    );

    if (SUCCEEDED(result)) {
        openFileDialog->SetFileTypes(ARRAYSIZE(filters), filters);

        result = openFileDialog->Show(NULL);

        if (SUCCEEDED(result)) {
            IShellItem* selectedItem;
            result = openFileDialog->GetResult(&selectedItem);
            if (SUCCEEDED(result)) {
                LPWSTR itemPath;
                result = selectedItem->GetDisplayName(SIGDN_FILESYSPATH, &itemPath);

                if (SUCCEEDED(result)) {
                    ShellExecute(NULL, L"open", itemPath, NULL, NULL, SW_SHOWNORMAL);
                }

                selectedItem->Release();
            }
        }

        openFileDialog->Release();
    }
}


LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_LOAD_BUTTON:
            std::thread(GetInstalledApps).detach();
            EnableWindow(loadButton, FALSE);
            EnableWindow(uninstallButton, TRUE);
            EnableWindow(installButton, TRUE);
            EnableWindow(updateButton, TRUE);
            break;
        case IDC_UNINSTALL_BUTTON:
            std::thread(UninstallSelectedApp).detach();
            isMsgNeeded = true;
            break;
        case IDC_UPDATE_BUTTON:
            std::thread(GetInstalledApps).detach();
            break;
        case IDC_INSTALL_BUTTON:
            MessageBox(nullptr, L"Для установки программы вам нужно выбрать установочный файл. Они обычно содержат в своём названии слово 'Setup'. ", L"Важное сообщение", MB_OK | MB_ICONINFORMATION);
            RunInstaller();
            isMsgNeeded = true;
            break;
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_ACTIVATE:
        if (LOWORD(wParam) != WA_INACTIVE && isMsgNeeded) {
            MessageBox(nullptr, L"Для корректной работы программы вне зависимости от результата выполнения установки/деинсталляции требуется обновить список установленных программ с помощью кнопки Update.", L"Важное сообщение", MB_OK | MB_ICONINFORMATION);
            isMsgNeeded = false;
        }
        break;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return 0;
}
