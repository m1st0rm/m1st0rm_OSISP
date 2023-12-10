#include "framework.h"
#include "OSISP_1_2.h"
#include <windows.h>
#include <commdlg.h>
#include <Richedit.h>
#include "resource.h"

#define MAX_CONTENT_SIZE 4096

HINSTANCE hInst;
HWND hwnd;
HWND hwndEdit;
TCHAR currentFileName[MAX_PATH] = _T("");
HFONT hFont = NULL;

#define ID_FILE_OPEN 1
#define ID_FILE_SAVE 2
#define ID_FILE_CREATE 3
#define ID_FILE_SAVEAS 4
#define ID_EDIT_COPY 5
#define ID_EDIT_PASTE 6
#define ID_STYLES_CHANGE_BACKGROUND_COLOR 7
#define ID_STYLES_CHANGE_TEXT_STYLE 8
COLORREF currentBackgroundColor = RGB(255, 255, 255);
bool isEditingEnabled = false;
HHOOK g_hHook = NULL;

void OpenFile(HWND hwnd);
void SaveFile(HWND hwnd);
void CreateFile(HWND hwnd);
void SaveFileAs(HWND hwnd);
bool IsTextModified(HWND hwndEdit);
void UpdateWindowTitle(HWND hwnd);
void ChangeTextStyle(HWND hwndEdit);
void ChangeBackgroundColor(HWND hwnd);
void SwitchToLightTheme(HWND hwndEdit);
void SwitchToDarkTheme(HWND hwndEdit);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    hInst = hInstance;

    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("Sample"), NULL };
    RegisterClassEx(&wc);

    hwnd = CreateWindow(_T("Sample"), _T("Текстовый редактор"), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, NULL, NULL, hInstance, NULL);

    if (!hwnd) return 0;

    HMENU hMenu = CreateMenu();
    HMENU hSubMenu = CreatePopupMenu();
    AppendMenu(hSubMenu, MF_STRING, ID_FILE_OPEN, _T("Open"));
    AppendMenu(hSubMenu, MF_STRING | MF_GRAYED, ID_FILE_SAVE, _T("Save"));
    AppendMenu(hSubMenu, MF_STRING | MF_GRAYED, ID_FILE_SAVEAS, _T("Save As"));
    AppendMenu(hSubMenu, MF_STRING, ID_FILE_CREATE, _T("Create"));
    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hSubMenu, _T("File"));
    HMENU hEditMenu = CreateMenu();
    AppendMenu(hEditMenu, MF_STRING | MF_GRAYED, ID_EDIT_PASTE, _T("Paste"));
    AppendMenu(hEditMenu, MF_STRING | MF_GRAYED, ID_EDIT_COPY, _T("Copy"));
    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hEditMenu, _T("Edit"));
    HMENU hStylesMenu = CreateMenu();
    AppendMenu(hStylesMenu, MF_STRING | MF_GRAYED, ID_STYLES_CHANGE_BACKGROUND_COLOR, _T("Change Background Color"));
    AppendMenu(hStylesMenu, MF_STRING | MF_GRAYED, ID_STYLES_CHANGE_TEXT_STYLE, _T("Change Text Style"));
    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hStylesMenu, _T("Styles"));
    SetMenu(hwnd, hMenu);

    LoadLibrary(TEXT("Msftedit.dll"));
    hwndEdit = CreateWindowEx(WS_EX_CLIENTEDGE,MSFTEDIT_CLASS,NULL,WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL,0, 0, 0, 0,hwnd,(HMENU)IDC_TEXT_EDIT,hInst, NULL);
    EnableWindow(hwndEdit, FALSE);
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);
    g_hHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, GetModuleHandle(NULL), 0);
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            KBDLLHOOKSTRUCT* kbdStruct = (KBDLLHOOKSTRUCT*)lParam;

            if (kbdStruct->vkCode == 'B' && GetAsyncKeyState(VK_CONTROL) < 0) {
                SwitchToDarkTheme(hwndEdit);
            }
            else if (kbdStruct->vkCode == 'W' && GetAsyncKeyState(VK_CONTROL) < 0) {
                SwitchToLightTheme(hwndEdit);
            }
        }
    }

    return CallNextHookEx(g_hHook, nCode, wParam, lParam);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_SIZE:
        RECT rcClient;
        GetClientRect(hwnd, &rcClient);
        MoveWindow(hwndEdit, rcClient.left, rcClient.top, rcClient.right, rcClient.bottom, TRUE);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_FILE_OPEN:
            if (IsTextModified(hwndEdit)) {
                int result = MessageBox(hwnd, _T("Сохранить изменения перед открытием нового файла?"), _T("Подтверждение"), MB_YESNOCANCEL | MB_ICONQUESTION);
                if (result == IDYES) {
                    SaveFile(hwnd);
                }
                else if (result == IDCANCEL) {
                    return 0;
                }
            }
            OpenFile(hwnd);
            SetFocus(hwndEdit);
            break;
        case ID_FILE_SAVE:
            SaveFile(hwnd);
            break;
        case ID_EDIT_COPY:
            if (OpenClipboard(hwnd)) {
                EmptyClipboard();
                CloseClipboard();
            }
            SendMessage(hwndEdit, WM_COPY, 0, 0);
            break;

        case ID_EDIT_PASTE:
            if (OpenClipboard(hwnd)) {
                if (IsClipboardFormatAvailable(CF_TEXT)) {
                    HANDLE hClipboardData = GetClipboardData(CF_TEXT);
                    if (hClipboardData) {
                        char* clipboardText = static_cast<char*>(GlobalLock(hClipboardData));
                        if (clipboardText) {
                            int textLength = MultiByteToWideChar(CP_ACP, 0, clipboardText, -1, NULL, 0);
                            if (textLength > 0) {
                                wchar_t* utf8Text = new wchar_t[textLength];
                                MultiByteToWideChar(CP_ACP, 0, clipboardText, -1, utf8Text, textLength);
                                int cursorPos = SendMessage(hwndEdit, EM_GETSEL, 0, 0);
                                SendMessage(hwndEdit, EM_REPLACESEL, TRUE, (LPARAM)utf8Text);

                                delete[] utf8Text;
                            }
                        }
                        GlobalUnlock(hClipboardData);
                    }
                }
                CloseClipboard();
            }
            break;
        case ID_FILE_SAVEAS:
            SaveFileAs(hwnd);
            break;
        case ID_FILE_CREATE:
            if (IsTextModified(hwndEdit)) {
                int result = MessageBox(hwnd, _T("Сохранить изменения перед созданием нового файла?"), _T("Подтверждение"), MB_YESNOCANCEL | MB_ICONQUESTION);
                if (result == IDYES) {
                    SaveFile(hwnd);
                }
                else if (result == IDCANCEL) {
                    return 0;
                }
            }
            CreateFile(hwnd);
            SetFocus(hwndEdit);
            break;
        case ID_STYLES_CHANGE_TEXT_STYLE:
            if (IsWindowEnabled(hwndEdit)) {
                ChangeTextStyle(hwndEdit);
            }
            break;
        case ID_STYLES_CHANGE_BACKGROUND_COLOR:
            ChangeBackgroundColor(hwndEdit);
            break;
        }

        EnableMenuItem(GetMenu(hwnd), ID_FILE_SAVE, IsWindowEnabled(hwndEdit) ? MF_ENABLED : MF_GRAYED);
        EnableMenuItem(GetMenu(hwnd), ID_FILE_SAVEAS, IsWindowEnabled(hwndEdit) ? MF_ENABLED : MF_GRAYED);
        EnableMenuItem(GetMenu(hwnd), ID_EDIT_COPY, IsWindowEnabled(hwndEdit) ? MF_ENABLED : MF_GRAYED);
        EnableMenuItem(GetMenu(hwnd), ID_EDIT_PASTE, IsWindowEnabled(hwndEdit) ? MF_ENABLED : MF_GRAYED);
        EnableMenuItem(GetMenu(hwnd), ID_STYLES_CHANGE_BACKGROUND_COLOR, IsWindowEnabled(hwndEdit) ? MF_ENABLED : MF_GRAYED);
        EnableMenuItem(GetMenu(hwnd), ID_STYLES_CHANGE_TEXT_STYLE, IsWindowEnabled(hwndEdit) ? MF_ENABLED : MF_GRAYED);
        DrawMenuBar(hwnd);
        break;

    case WM_CLOSE:
        if (IsTextModified(hwndEdit)) {
            int result = MessageBox(hwnd, _T("Сохранить изменения перед закрытием?"), _T("Сохранение"), MB_YESNOCANCEL | MB_ICONQUESTION);
            if (result == IDYES) {
                SaveFile(hwnd);
            }
            else if (result == IDCANCEL) {
                return 0;
            }
        }
        DestroyWindow(hwnd);
        break;


    case WM_DROPFILES:
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, message, wParam, lParam);
    }
    return 0;
}


void OpenFile(HWND hwnd)
{
    OPENFILENAME ofn;
    TCHAR szFileName[MAX_PATH] = _T("");

    ZeroMemory(&ofn, sizeof(OPENFILENAME));
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = _T("Text Files (*.txt)\0*.txt\0");
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

    if (GetOpenFileName(&ofn)) {
        HANDLE hFile = CreateFile(szFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

        if (hFile != INVALID_HANDLE_VALUE) {
            DWORD dwFileSize = GetFileSize(hFile, NULL);
            if (dwFileSize != INVALID_FILE_SIZE) {
                char* buffer = new char[dwFileSize + 1];
                DWORD dwRead;

                if (ReadFile(hFile, buffer, dwFileSize, &dwRead, NULL)) {
                    buffer[dwRead] = '\0';
                    SendMessage(hwndEdit, WM_SETTEXT, 0, (LPARAM)_T(""));
                    SetWindowTextA(hwndEdit, buffer);
                }

                delete[] buffer;
            }

            CloseHandle(hFile);
            lstrcpy(currentFileName, szFileName);
            EnableWindow(hwndEdit, TRUE);
            isEditingEnabled = true;
            UpdateWindowTitle(hwnd);
        }
    }
}

void SaveFile(HWND hwnd)
{
    if (lstrlen(currentFileName) == 0) {
        OPENFILENAME ofn;
        TCHAR szFileName[MAX_PATH] = _T("");

        ZeroMemory(&ofn, sizeof(OPENFILENAME));
        ofn.lStructSize = sizeof(OPENFILENAME);
        ofn.hwndOwner = hwnd;
        ofn.lpstrFilter = _T("Text Files (*.txt)\0*.txt\0");
        ofn.lpstrFile = szFileName;
        ofn.nMaxFile = MAX_PATH;
        ofn.Flags = OFN_OVERWRITEPROMPT;
        ofn.lpstrDefExt = _T("txt");

        if (!GetSaveFileName(&ofn)) {
            return;
        }

        lstrcpy(currentFileName, szFileName);
        UpdateWindowTitle(hwnd);
    }

    HANDLE hFile = CreateFile(currentFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile != INVALID_HANDLE_VALUE) {
        int textLength = GetWindowTextLength(hwndEdit);
        if (textLength > 0) {
            char* buffer = new char[textLength + 1];
            GetWindowTextA(hwndEdit, buffer, textLength + 1);

            DWORD dwWritten;
            WriteFile(hFile, buffer, textLength, &dwWritten, NULL);

            delete[] buffer;
        }

        CloseHandle(hFile);
    }
}

void CreateFile(HWND hwnd)
{
    OPENFILENAME ofn;
    TCHAR szFileName[MAX_PATH] = _T("");

    ZeroMemory(&ofn, sizeof(OPENFILENAME));
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = _T("Text Files (*.txt)\0*.txt\0");
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    ofn.lpstrDefExt = _T("txt");
    ofn.lpstrTitle = _T("Создание нового файла");

    if (GetSaveFileName(&ofn)) {
        lstrcpy(currentFileName, szFileName);
        UpdateWindowTitle(hwnd);
        SetWindowText(hwndEdit, _T(""));
        HANDLE hFile = CreateFile(currentFileName, 0, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile != INVALID_HANDLE_VALUE) {
            CloseHandle(hFile);
            EnableWindow(hwndEdit, TRUE);
            isEditingEnabled = true;
        }
    }
}

void SaveFileAs(HWND hwnd) {
    OPENFILENAME ofn;
    TCHAR szFileName[MAX_PATH] = _T("");

    ZeroMemory(&ofn, sizeof(OPENFILENAME));
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = _T("Text Files (*.txt)\0*.txt\0");
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_OVERWRITEPROMPT;
    ofn.lpstrDefExt = _T("txt");

    if (GetSaveFileName(&ofn)) {
        lstrcpy(currentFileName, szFileName);
        UpdateWindowTitle(hwnd);

        HANDLE hFile = CreateFile(currentFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

        if (hFile != INVALID_HANDLE_VALUE) {
            int textLength = GetWindowTextLength(hwndEdit);
            if (textLength > 0) {
                char* buffer = new char[textLength + 1];
                GetWindowTextA(hwndEdit, buffer, textLength + 1);

                DWORD dwWritten;
                WriteFile(hFile, buffer, textLength, &dwWritten, NULL);

                delete[] buffer;
            }

            CloseHandle(hFile);
        }
    }
}


bool IsTextModified(HWND hwndEdit) {
    return IsWindowEnabled(hwndEdit);
}

void UpdateWindowTitle(HWND hwnd) {
    if (lstrlen(currentFileName) > 0) {
        SetWindowText(hwnd, currentFileName);
    }
    else {
        SetWindowText(hwnd, _T("Текстовый редактор"));
    }
}

void ChangeTextStyle(HWND hwndEdit) {
    LOGFONT lfont;
    CHOOSEFONT cFont;
    ZeroMemory(&cFont, sizeof(CHOOSEFONT));
    cFont.lStructSize = sizeof(CHOOSEFONT);
    cFont.hwndOwner = hwndEdit;
    cFont.lpLogFont = &lfont;
    cFont.Flags = CF_SCREENFONTS | CF_EFFECTS | CF_SELECTSCRIPT;
    cFont.nFontType = RUSSIAN_CHARSET;

    if (ChooseFont(&cFont)) {
        HFONT hfont = CreateFontIndirect(cFont.lpLogFont);

        CHARFORMAT cf;
        memset(&cf, 0, sizeof(CHARFORMAT));
        cf.cbSize = sizeof(CHARFORMAT);
        cf.dwMask = CFM_COLOR | CFM_FACE | CFM_SIZE | CFM_BOLD | CFM_ITALIC | CFM_UNDERLINE | CFM_STRIKEOUT;
        cf.crTextColor = cFont.rgbColors;
        wcscpy_s(cf.szFaceName, LF_FACESIZE, cFont.lpLogFont->lfFaceName);
        cf.yHeight = cFont.lpLogFont->lfHeight * 20;
        cf.dwEffects = 0;
        if (cFont.lpLogFont->lfWeight == FW_BOLD) {
            cf.dwEffects |= CFE_BOLD;
        }
        if (cFont.lpLogFont->lfItalic) {
            cf.dwEffects |= CFE_ITALIC;
        }
        if (cFont.lpLogFont->lfUnderline) {
            cf.dwEffects |= CFE_UNDERLINE;
        }
        if (cFont.lpLogFont->lfStrikeOut) {
            cf.dwEffects |= CFE_STRIKEOUT;
        }
        SendMessage(hwndEdit, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);
    }
}

void ChangeBackgroundColor(HWND hwnd)
{
    CHOOSECOLOR cc;
    COLORREF acrCustClr[16] = { NULL };
    ZeroMemory(&cc, sizeof(cc));
    cc.lStructSize = sizeof(cc);
    cc.hwndOwner = hwnd;
    cc.lpCustColors = (LPDWORD)acrCustClr;
    cc.rgbResult = currentBackgroundColor;
    cc.Flags = CC_FULLOPEN | CC_RGBINIT;

    if (ChooseColor(&cc))
    {
        currentBackgroundColor = cc.rgbResult;
        SendMessage(hwndEdit, EM_SETBKGNDCOLOR, 0, currentBackgroundColor);
    }

    InvalidateRect(hwnd, NULL, TRUE);
}


void SwitchToLightTheme(HWND hwndEdit) {
    CHARFORMAT cf;
    memset(&cf, 0, sizeof(CHARFORMAT));
    cf.cbSize = sizeof(CHARFORMAT);
    cf.dwMask = CFM_COLOR;
    cf.crTextColor = RGB(0, 0, 0);
    currentBackgroundColor = RGB(0, 0, 0);
    SendMessage(hwndEdit, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);
    SendMessage(hwndEdit, EM_SETBKGNDCOLOR, 0, RGB(255, 255, 255));
}

void SwitchToDarkTheme(HWND hwndEdit) {
    CHARFORMAT cf;
    memset(&cf, 0, sizeof(CHARFORMAT));
    cf.cbSize = sizeof(CHARFORMAT);
    cf.dwMask = CFM_COLOR;
    cf.crTextColor = RGB(255, 255, 255);
    currentBackgroundColor = RGB(255, 255, 255);
    SendMessage(hwndEdit, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);
    SendMessage(hwndEdit, EM_SETBKGNDCOLOR, 0, RGB(0, 0, 0)); 
}