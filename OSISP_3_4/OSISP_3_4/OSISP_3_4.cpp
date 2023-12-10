#include <windows.h>
#include <TlHelp32.h>
#include <CommCtrl.h>
#include <Psapi.h>
#include <stdio.h>
#include <thread>
#include <string>
#include <vector>

#pragma comment(lib, "Comctl32.lib")

HINSTANCE hInst;
HWND hWnd;
HWND hListView;
HWND hUpdateButton;
HWND hPauseProcessButton;
HWND hUnpauseProcessButton;
HWND hTerminateProcessButton;
HWND hMemoryUsageMBLabel;
HWND hMemoryUsagePercentageLabel;
HWND hMemoryUsageMB;
HWND hMemoryUsagePercentage;
struct ProcessInfo {
    DWORD dwProcessId;
};
std::vector<ProcessInfo> processInfoList;
#define IDC_UPDATE_BUTTON 1001
#define IDC_PAUSE_PROCESS_BUTTON 1002
#define IDC_UNPAUSE_PROCESS_BUTTON 1003
#define IDC_TERMINATE_PROCESS_BUTTON 1004

void GetProcessList(HWND hListView);
void PauseProcess(HWND hListView);
void UnpauseProcess(HWND hListView);
void TerminateProcessC(HWND hListView);

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    hInst = hInstance;
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = L"MyMemoryMonitorApp";
    wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);
    RegisterClassEx(&wcex);

    INITCOMMONCONTROLSEX icex;
    icex.dwICC = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icex);
    hWnd = CreateWindow(L"MyMemoryMonitorApp", L"Memory Monitor", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

    hListView = CreateWindowEx(0, WC_LISTVIEW, L"", WS_VISIBLE | WS_CHILD | WS_BORDER | LVS_REPORT | LVS_SINGLESEL,
        10, 10, 800, 650, hWnd, NULL, hInst, NULL);

    LV_COLUMN lvc;
    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
    lvc.fmt = LVCFMT_LEFT;
    lvc.cx = 150;
    lvc.pszText = const_cast<LPWSTR>(L"Имя процесса");;
    ListView_InsertColumn(hListView, 0, &lvc);
    lvc.cx = 150;
    lvc.pszText = const_cast<LPWSTR>(L"Память (MB)");
    ListView_InsertColumn(hListView, 1, &lvc);
    lvc.cx = 150;
    lvc.pszText = const_cast<LPWSTR>(L"Память (%)");
    ListView_InsertColumn(hListView, 2, &lvc);

    hMemoryUsageMBLabel = CreateWindow(L"STATIC", L"Использование памяти (MB) :",
        WS_VISIBLE | WS_CHILD, 820, 10, 200, 20, hWnd, NULL, hInst, NULL);
    hMemoryUsagePercentageLabel = CreateWindow(L"STATIC", L"Использование памяти (%) :",
        WS_VISIBLE | WS_CHILD, 820, 40, 200, 20, hWnd, NULL, hInst, NULL);

    hMemoryUsageMB = CreateWindow(L"STATIC", L"",
        WS_VISIBLE | WS_CHILD, 1030, 10, 100, 20, hWnd, NULL, hInst, NULL);

    hMemoryUsagePercentage = CreateWindow(L"STATIC", L"",
        WS_VISIBLE | WS_CHILD, 1030, 40, 100, 20, hWnd, NULL, hInst, NULL);

    hUpdateButton = CreateWindow(L"BUTTON", L"Load", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        10, 660, 140, 30, hWnd, (HMENU)IDC_UPDATE_BUTTON, hInst, NULL);
    hPauseProcessButton = CreateWindow(L"BUTTON", L"Pause Process", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        160, 660, 140, 30, hWnd, (HMENU)IDC_PAUSE_PROCESS_BUTTON, hInst, NULL);
    hUnpauseProcessButton = CreateWindow(L"BUTTON", L"Unpause Process", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        310, 660, 140, 30, hWnd, (HMENU)IDC_UNPAUSE_PROCESS_BUTTON, hInst, NULL);
    hTerminateProcessButton = CreateWindow(L"BUTTON", L"Terminate Process", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        460, 660, 140, 30, hWnd, (HMENU)IDC_TERMINATE_PROCESS_BUTTON, hInst, NULL);


    if (!hWnd)
    {
        return FALSE;
    }
    EnableWindow(hPauseProcessButton, FALSE);
    EnableWindow(hUnpauseProcessButton, FALSE);
    EnableWindow(hTerminateProcessButton, FALSE);
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        int wmEvent = HIWORD(wParam);
        switch (wmId) 
        {
        case IDC_PAUSE_PROCESS_BUTTON:
        {
            std::thread(PauseProcess, hListView).detach();
            break;
        }
        case IDC_UNPAUSE_PROCESS_BUTTON:
        {
            std::thread(UnpauseProcess, hListView).detach();
            break;
        }
        case IDC_UPDATE_BUTTON:
        {
            SetWindowText(hUpdateButton, L"Update");
            std::thread(GetProcessList, hListView).detach();
            EnableWindow(hPauseProcessButton, TRUE);
            EnableWindow(hUnpauseProcessButton, TRUE);
            EnableWindow(hTerminateProcessButton, TRUE);
            break;
        }
        case IDC_TERMINATE_PROCESS_BUTTON:
        {
            std::thread(TerminateProcessC, hListView).detach();
            break;
        }
        }
    }
    break;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
        return 0;
    }
    break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

void GetProcessList(HWND hListView)
{
    double AllMemoryUsageMB = 0;
    double AllMemoryUsagePercentage = 0;
    ListView_DeleteAllItems(hListView);
    processInfoList.clear();
    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32;
    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE)
    {
        return;
    }
    pe32.dwSize = sizeof(PROCESSENTRY32);
    if (Process32First(hProcessSnap, &pe32))
    {
        do
        {
            LVITEM lvi;
            lvi.mask = LVIF_TEXT;
            lvi.iItem = 0;
            lvi.iSubItem = 0;
            lvi.pszText = pe32.szExeFile;
            processInfoList.push_back({ pe32.th32ProcessID });
            ListView_InsertItem(hListView, &lvi);

            PROCESS_MEMORY_COUNTERS_EX pmc;
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
            if (hProcess)
            {
                if (GetProcessMemoryInfo(hProcess, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc)))
                {
                    lvi.iSubItem = 1;
                    double memoryInMB = (double)(pmc.PrivateUsage / (1024.0 * 1024.0)); // Рабочий набор в мегабайтах
                    WCHAR szMemoryUsage[64];
                    swprintf_s(szMemoryUsage, L"%.2lf", memoryInMB);
                    AllMemoryUsageMB += memoryInMB;
                    lvi.pszText = szMemoryUsage;
                    ListView_SetItem(hListView, &lvi);
                    lvi.iSubItem = 2;
                    MEMORYSTATUSEX memInfo;
                    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
                    GlobalMemoryStatusEx(&memInfo);
                    double memoryUsagePercentage = (memoryInMB / (memInfo.ullTotalPhys / (1024.0 * 1024.0))) * 100.0; // Использование памяти в процентах
                    WCHAR szMemoryUsagePercentage[64];
                    swprintf_s(szMemoryUsagePercentage, L"%.2lf", memoryUsagePercentage);
                    AllMemoryUsagePercentage += memoryUsagePercentage;
                    lvi.pszText = szMemoryUsagePercentage;
                    ListView_SetItem(hListView, &lvi);
                }
                CloseHandle(hProcess);
            }
        } while (Process32Next(hProcessSnap, &pe32));
        std::wstring AllMemoryUsageMBAsString = std::to_wstring(AllMemoryUsageMB);
        SetWindowText(hMemoryUsageMB, AllMemoryUsageMBAsString.c_str());
        std::wstring AllMemoryUsagePercentageAsString = std::to_wstring(AllMemoryUsagePercentage);
        SetWindowText(hMemoryUsagePercentage, AllMemoryUsagePercentageAsString.c_str());
    }
    CloseHandle(hProcessSnap);
}

void PauseProcess(HWND hListView) {
    int selectedIdx = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);
    if (selectedIdx != -1) {
        if (selectedIdx < processInfoList.size()) {
            DWORD dwProcessId = processInfoList[processInfoList.size() - selectedIdx - 1].dwProcessId;

            HANDLE hThreadSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
            if (hThreadSnapshot != INVALID_HANDLE_VALUE) {
                THREADENTRY32 te32;
                te32.dwSize = sizeof(THREADENTRY32);

                if (Thread32First(hThreadSnapshot, &te32)) {
                    do {
                        if (te32.th32OwnerProcessID == dwProcessId) {
                            HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, te32.th32ThreadID);
                            if (hThread != NULL) {
                                SuspendThread(hThread);
                                CloseHandle(hThread);
                            }
                        }
                    } while (Thread32Next(hThreadSnapshot, &te32));
                }
                CloseHandle(hThreadSnapshot);
            }
        }
        else
        {
            return;
        }
    }
    else {
        return;
    }
    MessageBox(NULL, L"Процесс успешно приостановлен", L"OK", MB_ICONINFORMATION);
    GetProcessList(hListView);
}

void UnpauseProcess(HWND hListView) {
    int selectedIdx = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);
    if (selectedIdx != -1) {
        if (selectedIdx < processInfoList.size()) {
            DWORD dwProcessId = processInfoList[processInfoList.size() - selectedIdx - 1].dwProcessId;

            HANDLE hThreadSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
            if (hThreadSnapshot != INVALID_HANDLE_VALUE) {
                THREADENTRY32 te32;
                te32.dwSize = sizeof(THREADENTRY32);

                if (Thread32First(hThreadSnapshot, &te32)) {
                    do {
                        if (te32.th32OwnerProcessID == dwProcessId) {
                            HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, te32.th32ThreadID);
                            if (hThread != NULL) {
                                ResumeThread(hThread);
                                CloseHandle(hThread);
                            }
                        }
                    } while (Thread32Next(hThreadSnapshot, &te32));
                }
                CloseHandle(hThreadSnapshot);
            }

        }
        else
        {
            return;
        }
    }
    else
    {
        return;
    }
    MessageBox(NULL, L"Процесс успешно возобновлён", L"OK", MB_ICONINFORMATION);
    GetProcessList(hListView);
}

void TerminateProcessC(HWND hListView) {
    int selectedIdx = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);
    if (selectedIdx != -1) {
        if (selectedIdx < processInfoList.size()) {
            DWORD dwProcessId = processInfoList[processInfoList.size() - selectedIdx - 1].dwProcessId;
            HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, dwProcessId);
            if (hProcess != NULL) {
                if (TerminateProcess(hProcess, 0)) {

                }
                else {
                    return;
                }
                CloseHandle(hProcess);
            }
            else {
                return;
            }
        }
        else {
            return;
        }
    }
    else {
        return;
    }
    MessageBox(NULL, L"Процесс успешно завершён", L"OK", MB_ICONINFORMATION);
    GetProcessList(hListView);
}


