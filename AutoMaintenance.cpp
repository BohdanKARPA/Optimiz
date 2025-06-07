//AutoMaintenance.cpp
#include "AutoMaintenance.h"
#include <CommCtrl.h>
#include "ResourceDefinitions.h"
#include <windows.h>
#include <chrono>

static const wchar_t* REG_PATH = L"Software\\Optimiz\\AutoMaintenance";

// Вікно налаштувань — статичні змінні для зручності
struct {
    HWND hParent;
    AutoMaintenanceSettings settings;
} g_am_ctx;

// Функція створює групу радіокнопок та чекбоксів для налаштувань
void CreateAutoMaintenanceControls(HWND hWndParent) {
    RECT rc;
    GetClientRect(hWndParent, &rc);
    int x = 10, y = 10;
    int w = rc.right - rc.left - 20;

    // Заголовок
    CreateWindowExW(0, L"STATIC", L"Налаштування автоматичного обслуговування",
        WS_CHILD,
        x, y, w, 20,
        hWndParent, (HMENU)IDC_AUTO_MAINT_TITLE,
        GetModuleHandle(NULL), NULL);
    y += 30;

    // Періодичність (радіокнопки)
    CreateWindowExW(0, L"BUTTON", L"Щодня",
        WS_CHILD | BS_AUTORADIOBUTTON,
        x, y, 100, 20,
        hWndParent, (HMENU)IDC_AUTO_MAINT_DAILY,
        GetModuleHandle(NULL), NULL);
    CreateWindowExW(0, L"BUTTON", L"Щотижня",
        WS_CHILD | BS_AUTORADIOBUTTON,
        x + 110, y, 100, 20,
        hWndParent, (HMENU)IDC_AUTO_MAINT_WEEKLY,
        GetModuleHandle(NULL), NULL);
    CreateWindowExW(0, L"BUTTON", L"При старті програми",
        WS_CHILD | BS_AUTORADIOBUTTON,
        x + 220, y, 150, 20,
        hWndParent, (HMENU)IDC_AUTO_MAINT_ONSTART,
        GetModuleHandle(NULL), NULL);
    y += 30; // 30

    // Категорії очищення (чекбокси)
    CreateWindowExW(0, L"BUTTON", L"Тимчасові файли",
        WS_CHILD | BS_AUTOCHECKBOX,
        x, y, 150, 20,
        hWndParent, (HMENU)IDC_AUTO_MAINT_CHECK_TEMP,
        GetModuleHandle(NULL), NULL);
    CreateWindowExW(0, L"BUTTON", L"Кеш браузерів",
        WS_CHILD | BS_AUTOCHECKBOX,
        x + 160, y, 150, 20,
        hWndParent, (HMENU)IDC_AUTO_MAINT_CHECK_BROWSER,
        GetModuleHandle(NULL), NULL);
    CreateWindowExW(0, L"BUTTON", L"Кошик",
        WS_CHILD | BS_AUTOCHECKBOX,
        x + 320, y, 100, 20,
        hWndParent, (HMENU)IDC_AUTO_MAINT_CHECK_RECYCLE,
        GetModuleHandle(NULL), NULL);
    y += 30; // 30

    // Кнопка збереження
    CreateWindowExW(0, L"BUTTON", L"Зберегти",
        WS_CHILD | BS_DEFPUSHBUTTON,
        x, y, 100, 25,
        hWndParent, (HMENU)IDC_AUTO_MAINT_SAVE,
        GetModuleHandle(NULL), NULL);
}

bool LoadAutoMaintenanceSettings(AutoMaintenanceSettings& s) {
    HKEY hKey;
    DWORD type, data, size = sizeof(data);
    if (RegCreateKeyExW(HKEY_CURRENT_USER, REG_PATH, 0, NULL, 0, KEY_READ, NULL, &hKey, NULL) != ERROR_SUCCESS)
        return false;
    // ScheduleType
    if (RegQueryValueExW(hKey, L"ScheduleType", NULL, &type, (LPBYTE)&data, &size) == ERROR_SUCCESS)
        s.scheduleType = (ScheduleType)data;
    else
        s.scheduleType = SCHEDULE_ON_STARTUP;
    // Interval
    size = sizeof(data);
    if (RegQueryValueExW(hKey, L"IntervalMinutes", NULL, &type, (LPBYTE)&data, &size) == ERROR_SUCCESS)
        s.intervalMinutes = (int)data;
    else
        s.intervalMinutes = 60;  // за замовчанням щогодини
    RegCloseKey(hKey);
    return true;
}

bool SaveAutoMaintenanceSettings(const AutoMaintenanceSettings& s) {
    HKEY hKey;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, REG_PATH, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) != ERROR_SUCCESS)
        return false;
    DWORD data = (DWORD)s.scheduleType;
    RegSetValueExW(hKey, L"ScheduleType", 0, REG_DWORD, (BYTE*)&data, sizeof(data));
    data = (DWORD)s.intervalMinutes;
    RegSetValueExW(hKey, L"IntervalMinutes", 0, REG_DWORD, (BYTE*)&data, sizeof(data));
    RegCloseKey(hKey);
    return true;
}

void SetupAutoMaintenance(HWND hWnd) {
    // Прибираємо старий таймер
    KillTimer(hWnd, IDT_AUTO_MAINTENANCE);

    AutoMaintenanceSettings s;
    if (!LoadAutoMaintenanceSettings(s))
        return;

    // Якщо запускати одразу при старті
    if (s.scheduleType == SCHEDULE_ON_STARTUP) {
        PerformAutoMaintenance();
    }
    // Якщо з інтервалом
    else if (s.scheduleType == SCHEDULE_INTERVAL && s.intervalMinutes > 0) {
        UINT ms = (UINT)std::chrono::minutes(s.intervalMinutes).count();
        SetTimer(hWnd, IDT_AUTO_MAINTENANCE, ms, NULL);
    }
}


// Процедура вікна налаштувань
LRESULT CALLBACK AutoMaintSettingsWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        // Завантажити поточні налаштування
        LoadAutoMaintenanceSettings(g_am_ctx.settings);
        // Створити контролі
        CreateWindowExW(0, L"BUTTON", L"Запуск при старті",
            WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
            10, 10, 150, 20, hWnd, (HMENU)IDC_AM_RADIO_STARTUP, NULL, NULL);

        CreateWindowExW(0, L"BUTTON", L"Інтервал (хвилин):",
            WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
            10, 40, 150, 20, hWnd, (HMENU)IDC_AM_RADIO_INTERVAL, NULL, NULL);

        CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", NULL,
            WS_CHILD | WS_VISIBLE | ES_NUMBER,
            170, 40, 50, 20, hWnd, (HMENU)IDC_AM_EDIT_INTERVAL, NULL, NULL);

        CreateWindowExW(0, L"BUTTON", L"OK",
            WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
            50, 80, 60, 25, hWnd, (HMENU)IDC_AM_BUTTON_OK, NULL, NULL);

        CreateWindowExW(0, L"BUTTON", L"Відмінити",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            130, 80, 80, 25, hWnd, (HMENU)IDC_AM_BUTTON_CANCEL, NULL, NULL);

        // Встановити стан кнопок/полів за поточними налаштуваннями
        if (g_am_ctx.settings.scheduleType == SCHEDULE_ON_STARTUP) {
            SendMessageW(GetDlgItem(hWnd, IDC_AM_RADIO_STARTUP), BM_SETCHECK, BST_CHECKED, 0);
            EnableWindow(GetDlgItem(hWnd, IDC_AM_EDIT_INTERVAL), FALSE);
        }
        else {
            SendMessageW(GetDlgItem(hWnd, IDC_AM_RADIO_INTERVAL), BM_SETCHECK, BST_CHECKED, 0);
            EnableWindow(GetDlgItem(hWnd, IDC_AM_EDIT_INTERVAL), TRUE);
        }
        SetWindowTextW(GetDlgItem(hWnd, IDC_AM_EDIT_INTERVAL), std::to_wstring(g_am_ctx.settings.intervalMinutes).c_str());
        return 0;
    }

    case WM_COMMAND: {
        switch (LOWORD(wParam)) {
        case IDC_AM_RADIO_STARTUP:
            g_am_ctx.settings.scheduleType = SCHEDULE_ON_STARTUP;
            EnableWindow(GetDlgItem(hWnd, IDC_AM_EDIT_INTERVAL), FALSE);
            break;
        case IDC_AM_RADIO_INTERVAL:
            g_am_ctx.settings.scheduleType = SCHEDULE_INTERVAL;
            EnableWindow(GetDlgItem(hWnd, IDC_AM_EDIT_INTERVAL), TRUE);
            break;
        case IDC_AM_BUTTON_OK: {
            if (g_am_ctx.settings.scheduleType == SCHEDULE_INTERVAL) {
                BOOL ok;
                int val = GetDlgItemInt(hWnd, IDC_AM_EDIT_INTERVAL, &ok, FALSE);
                if (ok && val > 0) g_am_ctx.settings.intervalMinutes = val;
            }
            SaveAutoMaintenanceSettings(g_am_ctx.settings);
            // Переналаштувати таймер
            SetupAutoMaintenance(g_am_ctx.hParent);
            DestroyWindow(hWnd);
            break;
        }
        case IDC_AM_BUTTON_CANCEL:
            DestroyWindow(hWnd);
            break;
        }
        return 0;
    }

    case WM_DESTROY:
        UnregisterClassW(AUTO_MAINT_SETTINGS_CLASS, GetModuleHandle(NULL));
        return 0;
    }

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

// Функція, що створює та показує вікно налаштувань
void ShowAutoMaintSettingsWindow(HWND hParent) {
    g_am_ctx.hParent = hParent;

    HINSTANCE hInst = GetModuleHandle(NULL);
    WNDCLASSW wc = { };
    wc.lpfnWndProc = AutoMaintSettingsWndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = AUTO_MAINT_SETTINGS_CLASS;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassW(&wc);

    // Розміри вікна
    int width = 280, height = 150;
    RECT rc = { 0, 0, width, height };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

    CreateWindowExW(
        0,
        AUTO_MAINT_SETTINGS_CLASS,
        L"Налаштування автообслуговування",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left, rc.bottom - rc.top,
        hParent,
        NULL,
        hInst,
        NULL
    );
}

void PerformAutoMaintenance() {
    // Приклад виклику: видалити все сміття за замовчанням
    // RunDiskCleanup();      // <-- розкоментуйте та поправте на свою функцію
    // Якщо у вас є окремі API:
    // ClearWindowsTemp();
    // ClearUserTemp();
    // ClearBrowserCache();
}
