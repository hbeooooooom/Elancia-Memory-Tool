#include <windows.h>
#include <commctrl.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <vector>
#include <string>
#pragma comment(linker, "/SUBSYSTEM:WINDOWS")
#pragma comment(lib, "comctl32.lib")

// IDA 검색용 마커
const char ELANCIA_SPEED_HACK_FINDER_12345[] = "ELANCIA_SPEED_HACK_FINDER_12345";
const char ELANCIA_ATTACK_HACK_FINDER_67890[] = "ELANCIA_ATTACK_HACK_FINDER_67890";
const char SPEED_HOTKEY_MARKER_ABCDEF[] = "SPEED_HOTKEY_MARKER_ABCDEF";
const char CONNECTION_ERROR_MARKER_999[] = "CONNECTION_ERROR_MARKER_999";

// GUI 컨트롤 ID
#define IDC_PROCESS_LIST    1001
#define IDC_REFRESH_BTN     1002
#define IDC_SPEED_EDIT      1003
#define IDC_APPLY_SPEED     1004
#define IDC_PERFECT_ON      1005
#define IDC_NORMAL_DAMAGE   1006
#define IDC_DEBUG_CHECK     1007

// 전역 변수
HWND g_hMainWnd = NULL;
HWND g_hProcessList = NULL;
HWND g_hSpeedEdit = NULL;
HANDLE g_hTargetProcess = NULL;
DWORD g_targetPID = 0;
DWORD_PTR g_jelanciaBase = 0;
bool g_debugMode = false;

// 프로세스 메모리 읽기/쓰기 함수들
bool WriteMemory(HANDLE hProcess, DWORD_PTR address, const void* data, size_t size) {
    SIZE_T bytesWritten;
    return WriteProcessMemory(hProcess, (LPVOID)address, data, size, &bytesWritten) && bytesWritten == size;
}

bool ReadMemory(HANDLE hProcess, DWORD_PTR address, void* buffer, size_t size) {
    SIZE_T bytesRead;
    return ReadProcessMemory(hProcess, (LPVOID)address, buffer, size, &bytesRead) && bytesRead == size;
}

DWORD_PTR GetModuleBaseAddress(DWORD processID, const wchar_t* moduleName) {
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, processID);
    if (hSnap == INVALID_HANDLE_VALUE) return 0;

    MODULEENTRY32W modEntry;
    modEntry.dwSize = sizeof(modEntry);

    if (Module32FirstW(hSnap, &modEntry)) {
        do {
            if (_wcsicmp(modEntry.szModule, moduleName) == 0) {
                CloseHandle(hSnap);
                return (DWORD_PTR)modEntry.modBaseAddr;
            }
        } while (Module32NextW(hSnap, &modEntry));
    }

    CloseHandle(hSnap);
    return 0;
}

// 엘란시아 프로세스 목록 가져오기
std::vector<std::pair<DWORD, std::wstring>> GetElanciaProcesses() {
    std::vector<std::pair<DWORD, std::wstring>> processes;

    HWND hwnd = NULL;
    while ((hwnd = FindWindowExW(NULL, hwnd, L"Nexon.Elancia", NULL)) != NULL) {
        DWORD pid;
        GetWindowThreadProcessId(hwnd, &pid);

        wchar_t title[256];
        GetWindowTextW(hwnd, title, 256);

        if (wcslen(title) > 0) {
            processes.push_back(std::make_pair(pid, std::wstring(title)));
        }
    }

    return processes;
}

// 치트엔진 함수들
void CheatEngine_MoveSpeedTo(int speed) {
    if (!g_hTargetProcess || !g_jelanciaBase) return;

    // IDA 검색용 마커 사용
    const char* marker = ELANCIA_SPEED_HACK_FINDER_12345;

    // 메모리 주소들 (IDA 검색용 주석)
    const DWORD_PTR OFFSET_7F51 = 0x7F51; // IDA_SEARCH_OFFSET_1
    const DWORD_PTR OFFSET_7F52 = 0x7F52; // IDA_SEARCH_OFFSET_2
    const DWORD_PTR OFFSET_7F53 = 0x7F53; // IDA_SEARCH_OFFSET_3
    const DWORD_PTR OFFSET_7F54 = 0x7F54; // IDA_SEARCH_OFFSET_4
    const DWORD_PTR OFFSET_7F55 = 0x7F55; // IDA_SEARCH_OFFSET_5
    const DWORD_PTR OFFSET_7F56 = 0x7F56; // IDA_SEARCH_OFFSET_6

    const DWORD_PTR PLAYER_BASE_0058DAD4 = 0x0058DAD4; // IDA_SEARCH_PLAYER_BASE
    const DWORD_PTR SPEED_STRUCT_178 = 0x178;          // IDA_SEARCH_SPEED_STRUCT
    const DWORD_PTR RUN_SPEED_9C = 0x9C;               // IDA_SEARCH_RUN_OFFSET
    const DWORD_PTR WALK_SPEED_98 = 0x98;              // IDA_SEARCH_WALK_OFFSET
    const DWORD_PTR SWIM_SPEED_A4 = 0xA4;              // IDA_SEARCH_SWIM_OFFSET

    const BYTE NOP_BYTE_90 = 0x90; // NOP_INSTRUCTION_MARKER

    // 이동속도 변경 방지 패치들
    WriteMemory(g_hTargetProcess, g_jelanciaBase + OFFSET_7F51, &NOP_BYTE_90, 1);
    WriteMemory(g_hTargetProcess, g_jelanciaBase + OFFSET_7F52, &NOP_BYTE_90, 1);
    WriteMemory(g_hTargetProcess, g_jelanciaBase + OFFSET_7F53, &NOP_BYTE_90, 1);
    WriteMemory(g_hTargetProcess, g_jelanciaBase + OFFSET_7F54, &NOP_BYTE_90, 1);
    WriteMemory(g_hTargetProcess, g_jelanciaBase + OFFSET_7F55, &NOP_BYTE_90, 1);
    WriteMemory(g_hTargetProcess, g_jelanciaBase + OFFSET_7F56, &NOP_BYTE_90, 1);

    // 속도 설정
    DWORD_PTR playerPtr;
    if (ReadMemory(g_hTargetProcess, PLAYER_BASE_0058DAD4, &playerPtr, sizeof(playerPtr))) {
        DWORD_PTR speedStructPtr;
        if (ReadMemory(g_hTargetProcess, playerPtr + SPEED_STRUCT_178, &speedStructPtr, sizeof(speedStructPtr))) {
            // 달리기 속도
            WriteMemory(g_hTargetProcess, speedStructPtr + RUN_SPEED_9C, &speed, sizeof(speed));
            // 걷기 속도  
            WriteMemory(g_hTargetProcess, speedStructPtr + WALK_SPEED_98, &speed, sizeof(speed));
            // 헤엄치기 속도
            WriteMemory(g_hTargetProcess, speedStructPtr + SWIM_SPEED_A4, &speed, sizeof(speed));
        }
    }
}

void CheatEngine_AttackAlwaysPerFect() {
    if (!g_hTargetProcess) return;

    // IDA 검색용 마커 사용
    const char* marker = ELANCIA_ATTACK_HACK_FINDER_67890;

    const DWORD_PTR ATTACK_ADDR_4CFBC5 = 0x004CFBC5; // IDA_SEARCH_ATTACK_ADDR_1
    const DWORD_PTR ATTACK_ADDR_4D05CD = 0x004D05CD; // IDA_SEARCH_ATTACK_ADDR_2
    const BYTE PERFECT_BYTE_A2 = 0xA2;                // PERFECT_ATTACK_OPCODE

    WriteMemory(g_hTargetProcess, ATTACK_ADDR_4CFBC5, &PERFECT_BYTE_A2, 1);
    WriteMemory(g_hTargetProcess, ATTACK_ADDR_4D05CD, &PERFECT_BYTE_A2, 1);
}

void CheatEngine_AttackAlwaysNormal() {
    if (!g_hTargetProcess) return;

    const DWORD_PTR ATTACK_ADDR_4CFBC5 = 0x004CFBC5;
    const DWORD_PTR ATTACK_ADDR_4D05CD = 0x004D05CD;
    const BYTE NORMAL_BYTE_B6 = 0xB6;

    WriteMemory(g_hTargetProcess, ATTACK_ADDR_4CFBC5, &NORMAL_BYTE_B6, 1);
    WriteMemory(g_hTargetProcess, ATTACK_ADDR_4D05CD, &NORMAL_BYTE_B6, 1);
}

// GUI 이벤트 처리
void RefreshProcessList() {
    SendMessage(g_hProcessList, CB_RESETCONTENT, 0, 0);

    std::vector<std::pair<DWORD, std::wstring>> processes = GetElanciaProcesses();
    if (processes.empty()) {
        SendMessageW(g_hProcessList, CB_ADDSTRING, 0, (LPARAM)L"실행 중인 일랜 창 없음");
    }
    else {
        for (size_t i = 0; i < processes.size(); i++) {
            SendMessageW(g_hProcessList, CB_ADDSTRING, 0, (LPARAM)processes[i].second.c_str());
        }
    }
    SendMessage(g_hProcessList, CB_SETCURSEL, 0, 0);
}

void ConnectToProcess() {
    int sel = (int)SendMessage(g_hProcessList, CB_GETCURSEL, 0, 0);
    if (sel == CB_ERR) return;

    std::vector<std::pair<DWORD, std::wstring>> processes = GetElanciaProcesses();
    if (sel >= (int)processes.size()) return;

    g_targetPID = processes[sel].first;

    if (g_hTargetProcess) {
        CloseHandle(g_hTargetProcess);
    }

    g_hTargetProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, g_targetPID);
    if (!g_hTargetProcess) {
        MessageBoxW(g_hMainWnd, L"프로세스 열기에 실패했습니다.", L"오류", MB_ICONERROR);
        return;
    }

    g_jelanciaBase = GetModuleBaseAddress(g_targetPID, L"jelancia_core.dll");

    wchar_t msg[256];
    wsprintfW(msg, L"PID: %d 에 연결되었습니다.", g_targetPID);
    MessageBoxW(g_hMainWnd, msg, L"연결됨", MB_ICONINFORMATION);
}

void ApplySpeed() {
    if (!g_hTargetProcess) {
        MessageBoxW(g_hMainWnd, L"먼저 대상 프로세스를 선택하세요.", L"오류", MB_ICONERROR);
        return;
    }

    wchar_t speedText[32];
    GetWindowTextW(g_hSpeedEdit, speedText, 32);
    int speed = _wtoi(speedText);

    if (speed < 100 || speed > 1000) {
        MessageBoxW(g_hMainWnd, L"합리적 범위(100~1000)에서 설정하세요.\n기본 추천: 220~260", L"오류", MB_ICONERROR);
        return;
    }

    CheatEngine_MoveSpeedTo(speed);

    wchar_t msg[256];
    wsprintfW(msg, L"이동속도 적용: %d", speed);
    MessageBoxW(g_hMainWnd, msg, L"이동속도", MB_ICONINFORMATION);
}

// 핫키 처리 함수
void HandleHotkey() {
    if (!g_hTargetProcess) {
        MessageBoxA(g_hMainWnd, "먼저 대상 프로세스를 선택하세요.", CONNECTION_ERROR_MARKER_999, MB_ICONERROR);
        return;
    }

    wchar_t speedText[32];
    GetWindowTextW(g_hSpeedEdit, speedText, 32);
    int speed = _wtoi(speedText);

    if (speed == 0) speed = 240; // 기본값

    if (speed < 100 || speed > 1000) {
        MessageBoxA(g_hMainWnd, "합리적 범위(100~1000)에서 설정하세요.\n기본 추천: 220~260", "오류 - ALT_K_HOTKEY_MARKER_12345", MB_ICONERROR);
        return;
    }

    CheatEngine_MoveSpeedTo(speed);

    char msg[256];
    wsprintfA(msg, "Alt+K로 이동속도 적용: %d - %s", speed, SPEED_HOTKEY_MARKER_ABCDEF);
    MessageBoxA(g_hMainWnd, msg, "이동속도", MB_ICONINFORMATION);
}

// 윈도우 프로시저
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE:
    {
        CreateWindowW(L"STATIC", L"대상 프로세스(창) - ELANCIA_HACK_MARKER_12345:",
            WS_VISIBLE | WS_CHILD, 10, 10, 400, 20, hwnd, NULL, NULL, NULL);

        g_hProcessList = CreateWindowW(L"COMBOBOX", NULL,
            WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
            10, 35, 380, 200, hwnd, (HMENU)IDC_PROCESS_LIST, NULL, NULL);

        CreateWindowW(L"BUTTON", L"새로고침",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            398, 35, 80, 25, hwnd, (HMENU)IDC_REFRESH_BTN, NULL, NULL);

        CreateWindowW(L"STATIC", L"이동속도 (기본 220~260 권장) - SPEED_MARKER_67890:",
            WS_VISIBLE | WS_CHILD, 10, 70, 400, 20, hwnd, NULL, NULL, NULL);

        g_hSpeedEdit = CreateWindowW(L"EDIT", L"240",
            WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER,
            10, 95, 120, 25, hwnd, (HMENU)IDC_SPEED_EDIT, NULL, NULL);

        CreateWindowW(L"BUTTON", L"적용",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            138, 95, 60, 25, hwnd, (HMENU)IDC_APPLY_SPEED, NULL, NULL);

        CreateWindowW(L"STATIC", L"데미지 모드 - ATTACK_MARKER_ABCDEF:",
            WS_VISIBLE | WS_CHILD, 10, 130, 400, 20, hwnd, NULL, NULL, NULL);

        CreateWindowW(L"BUTTON", L"퍼펙트 ON",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            10, 155, 100, 25, hwnd, (HMENU)IDC_PERFECT_ON, NULL, NULL);

        CreateWindowW(L"BUTTON", L"기본으로 복원",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            118, 155, 120, 25, hwnd, (HMENU)IDC_NORMAL_DAMAGE, NULL, NULL);

        CreateWindowW(L"BUTTON", L"디버그 모드 - DEBUG_MARKER_999",
            WS_VISIBLE | WS_CHILD | BS_CHECKBOX,
            10, 190, 200, 20, hwnd, (HMENU)IDC_DEBUG_CHECK, NULL, NULL);

        RefreshProcessList();

        RegisterHotKey(hwnd, 1, MOD_ALT, 'K');
        RegisterHotKey(hwnd, 2, MOD_ALT, '1');
        RegisterHotKey(hwnd, 3, MOD_ALT, '2');
    }
    break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_REFRESH_BTN:
            RefreshProcessList();
            break;
        case IDC_PROCESS_LIST:
            if (HIWORD(wParam) == CBN_SELCHANGE) {
                ConnectToProcess();
            }
            break;
        case IDC_APPLY_SPEED:
            ApplySpeed();
            break;
        case IDC_PERFECT_ON:
            if (g_hTargetProcess) {
                CheatEngine_AttackAlwaysPerFect();
                MessageBoxW(hwnd, L"퍼펙트(2배) 적용", L"데미지", MB_ICONINFORMATION);
            }
            break;
        case IDC_NORMAL_DAMAGE:
            if (g_hTargetProcess) {
                CheatEngine_AttackAlwaysNormal();
                MessageBoxW(hwnd, L"기본(1배)로 복원", L"데미지", MB_ICONINFORMATION);
            }
            break;
        }
        break;

    case WM_HOTKEY:
        switch (wParam) {
        case 1: // Alt+K
            HandleHotkey();
            break;
        case 2: // Alt+1
            if (g_hTargetProcess) {
                CheatEngine_AttackAlwaysPerFect();
                MessageBoxW(hwnd, L"퍼펙트(2배) 적용", L"데미지", MB_ICONINFORMATION);
            }
            break;
        case 3: // Alt+2
            if (g_hTargetProcess) {
                CheatEngine_AttackAlwaysNormal();
                MessageBoxW(hwnd, L"기본(1배)로 복원", L"데미지", MB_ICONINFORMATION);
            }
            break;
        }
        break;

    case WM_CLOSE:
        if (g_hTargetProcess) {
            CloseHandle(g_hTargetProcess);
        }
        UnregisterHotKey(hwnd, 1);
        UnregisterHotKey(hwnd, 2);
        UnregisterHotKey(hwnd, 3);
        DestroyWindow(hwnd);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

// 메인 함수
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    const wchar_t CLASS_NAME[] = L"ElanciaHackWindow";

    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClassW(&wc);

    g_hMainWnd = CreateWindowExW(
        0,
        CLASS_NAME,
        L"이속 + 퍼펙트 - WINDOW_TITLE_MARKER",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 500, 250,
        NULL, NULL, hInstance, NULL
    );

    if (!g_hMainWnd) {
        return 0;
    }

    ShowWindow(g_hMainWnd, nCmdShow);
    UpdateWindow(g_hMainWnd);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}