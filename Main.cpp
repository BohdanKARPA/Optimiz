//Main.cpp
#include <windows.h>
#include <CommCtrl.h>
#pragma comment(lib, "Comctl32.lib")

#include "AutoMaintenance.h"
#include "ResourceDefinitions.h"
#include "Utils.h"
#include "ProcessMonitor.h"
#include "DiskCleanup.h"

#include <string>

// -----------------------------
// Глобальні змінні
// -----------------------------
static HMENU        hMenuBar_global = NULL;    // Дескриптор головного меню
static UINT         activeMenuID_global = 0;       // Який пункт зараз активний
// (Шрифти для Owner-Draw тут не використовуються, бо ми малюємо просто текст із ▶)
static HWND hMainWnd = NULL;

// Оригінальні назви пунктів меню
static const wchar_t* MENU_TEXT_MONITOR = L"Моніторинг ресурсів";
static const wchar_t* MENU_TEXT_DISK = L"Очищення диска";
static const wchar_t* MENU_TEXT_AUTOMAIN = L"Автоматичне обслуговування";
static const wchar_t* MENU_TEXT_HELP = L"Довідка";
// Глобальна змінна для екземпляра (перенесено з код.txt)
HINSTANCE hInst_global;

// Прототип віконної процедури
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Допоміжна функція: оновлює текст пунктів головного меню,
// ставлячи “▶ ” перед тим, який зараз activeID
void UpdateMenuText(HWND hwnd, UINT activeID)
{
    // 1) Пункт "Моніторинг ресурсів"
    if (activeID == IDM_MONITORING) {
        std::wstring temp = L"▶ " + std::wstring(MENU_TEXT_MONITOR);
        ModifyMenu(hMenuBar_global,
            IDM_MONITORING,
            MF_BYCOMMAND | MF_STRING,
            IDM_MONITORING,
            temp.c_str());
    }
    else {
        ModifyMenu(hMenuBar_global,
            IDM_MONITORING,
            MF_BYCOMMAND | MF_STRING,
            IDM_MONITORING,
            MENU_TEXT_MONITOR);
    }

    // 2) Пункт "Очищення диска"
    if (activeID == IDM_DISK_CLEANUP) {
        std::wstring temp = L"▶ " + std::wstring(MENU_TEXT_DISK);
        ModifyMenu(hMenuBar_global,
            IDM_DISK_CLEANUP,
            MF_BYCOMMAND | MF_STRING,
            IDM_DISK_CLEANUP,
            temp.c_str());
    }
    else {
        ModifyMenu(hMenuBar_global,
            IDM_DISK_CLEANUP,
            MF_BYCOMMAND | MF_STRING,
            IDM_DISK_CLEANUP,
            MENU_TEXT_DISK);
    }

    // 4) Пункт "Автоматичне обслуговування"
    if (activeID == IDM_SETTINGS_AUTO_MAINTENANCE) {
        std::wstring temp = L"▶ " + std::wstring(MENU_TEXT_AUTOMAIN);
        ModifyMenu(hMenuBar_global,
            IDM_SETTINGS_AUTO_MAINTENANCE,
            MF_BYCOMMAND | MF_STRING,
            IDM_SETTINGS_AUTO_MAINTENANCE,
            temp.c_str());
    }
    else {
        ModifyMenu(hMenuBar_global,
            IDM_SETTINGS_AUTO_MAINTENANCE,
            MF_BYCOMMAND | MF_STRING,
            IDM_SETTINGS_AUTO_MAINTENANCE,
            MENU_TEXT_AUTOMAIN);
    }

    // Після кожного оновлення обов'язково перемалюємо меню
    DrawMenuBar(hwnd);
}

// ============================
// Точка входу wWinMain
// ============================
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    hInst_global = hInstance;

    // Перевіряємо права адміністратора
    wchar_t exePath[MAX_PATH];
    GetModuleFileName(NULL, exePath, MAX_PATH);

    if (!IsUserAdmin()) {
        int msgboxID = MessageBox(
            NULL,
            L"Для виконання деяких операцій програмі потрібні права адміністратора.\n\nЗапустити програму з правами адміністратора?",
            L"Потрібні права адміністратора",
            MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON1
        );
        if (msgboxID == IDYES) {
            RequestAdminRights(exePath);
            return 0;
        }
        else {
            MessageBox(NULL, L"Програма не може функціонувати без прав адміністратора. Завершення.", L"Обмежений доступ", MB_OK | MB_ICONINFORMATION);
            return 1;
        }
    }

    // Підвищуємо пріоритет процесу
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

    // Ініціалізуємо ListView-контроли
    INITCOMMONCONTROLSEX icex = {};
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_LISTVIEW_CLASSES | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icex);

    // Реєструємо клас вікна
    const wchar_t CLASS_NAME[] = L"MyOptimizerWindowClass";
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    if (!RegisterClass(&wc)) {
        MessageBox(NULL, L"Не вдалося зареєструвати клас вікна!", L"Помилка", MB_ICONERROR | MB_OK);
        return 0;
    }

    // ================================
    // Створюємо головне меню
    // ================================
    HMENU hMenubar = CreateMenu();
    HMENU hMenuFile = CreateMenu();

    AppendMenu(hMenuFile, MF_STRING, IDM_FILE_EXIT, L"&Вихід");
    AppendMenu(hMenubar , MF_POPUP , (UINT_PTR)hMenuFile, L"&Файл");

    AppendMenu(hMenubar, MF_STRING, IDM_MONITORING, MENU_TEXT_MONITOR);
    AppendMenu(hMenubar, MF_STRING, IDM_DISK_CLEANUP, MENU_TEXT_DISK);
    AppendMenu(hMenubar, MF_STRING, IDM_SETTINGS_AUTO_MAINTENANCE, MENU_TEXT_AUTOMAIN);
    AppendMenu(hMenubar, MF_STRING, IDM_HELP, MENU_TEXT_HELP);

    // Зберігаємо дескриптор головного меню в глобальну змінну
    hMenuBar_global = hMenubar;

    // Створюємо вікно, передаючи йому наше меню
    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        L"Оптимізатор ОС Windows",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT,
        650, 600,
        NULL,
        hMenubar,       // <- передаємо сюди дескриптор меню
        hInstance,
        NULL
    );

    if (hwnd == NULL) {
        MessageBox(NULL, L"Не вдалося створити вікно!", L"Помилка", MB_ICONERROR | MB_OK);
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    SetupAutoMaintenance(hwnd);
    // Відразу показати “Моніторинг ресурсів” і поставити ▶ перед ним
    SendMessage(hwnd, WM_COMMAND, MAKELONG(IDM_MONITORING, 0), 0);

    // Головний цикл повідомлень
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

// ============================
// Віконна процедура
// ============================
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    // Статичні HWND, які прив’язуються у WM_CREATE
    static HWND hListView_ProcessMonitor = NULL;
    static HWND hStatic_CPUOverall = NULL;
    static HWND hStatic_MemoryOverall = NULL;
    static HWND hStatic_IOReadOverall = NULL;
    static HWND hStatic_IOWriteOverall = NULL;
    static HWND hStatic_DiskUsageOverall = NULL;

    static HWND hListView_DiskCleanup = NULL;
    static HWND hButton_AnalyzeDisk = NULL;
    static HWND hButton_CleanSelected = NULL;
    static HWND hButton_CleanAll = NULL;
    static HWND hStatic_TotalJunkLabel = NULL;
    static HWND hStatic_TotalJunkSize = NULL;
    static HWND hStatic_DiskTotalLabel = NULL;
    static HWND hStatic_DiskTotalSize = NULL;
    static HWND hStatic_DiskUsedLabel = NULL;
    static HWND hStatic_DiskUsedSize = NULL;
    static HWND hStatic_DiskFreeLabel = NULL;
    static HWND hStatic_DiskFreeSize = NULL;

    static HWND hListView_Autostart = NULL;
    static HWND hStatic_AutoMaintTitle = NULL;
    static HWND hRadio_Daily, hRadio_Weekly, hRadio_OnStart = NULL;
    static HWND hCheck_Temp, hCheck_Browser, hCheck_Recycle = NULL;
    static HWND hButton_SaveSettings = NULL;

    switch (uMsg) {
    case WM_CREATE:
        // Створюємо контролери для кожної вкладки
        CreateProcessMonitorControls(hwnd);
        hListView_ProcessMonitor = GetDlgItem(hwnd, IDC_PROCESSLISTVIEW);
        hStatic_CPUOverall = CreateWindowEx(0, L"STATIC", L"", WS_CHILD, 0, 0, 0, 0, hwnd, NULL, GetModuleHandle(NULL), NULL);
        hStatic_MemoryOverall = CreateWindowEx(0, L"STATIC", L"", WS_CHILD, 0, 0, 0, 0, hwnd, NULL, GetModuleHandle(NULL), NULL);
        hStatic_IOReadOverall = CreateWindowEx(0, L"STATIC", L"", WS_CHILD, 0, 0, 0, 0, hwnd, NULL, GetModuleHandle(NULL), NULL);
        hStatic_IOWriteOverall = CreateWindowEx(0, L"STATIC", L"", WS_CHILD, 0, 0, 0, 0, hwnd, NULL, GetModuleHandle(NULL), NULL);
        hStatic_DiskUsageOverall = CreateWindowEx(0, L"STATIC", L"", WS_CHILD, 0, 0, 0, 0, hwnd, NULL, GetModuleHandle(NULL), NULL);

        CreateDiskCleanupControls(hwnd);
        hListView_DiskCleanup = GetDlgItem(hwnd, IDC_DISKCLEANUP_LISTVIEW);
        hButton_AnalyzeDisk = GetDlgItem(hwnd, IDC_DISKCLEANUP_ANALYZE_BUTTON);
        hButton_CleanSelected = GetDlgItem(hwnd, IDC_DISKCLEANUP_CLEAN_BUTTON);
        hButton_CleanAll = GetDlgItem(hwnd, IDC_DISKCLEANUP_CLEAN_ALL);
        hStatic_TotalJunkLabel = GetDlgItem(hwnd, IDC_DISKCLEANUP_TOTALJUNK_LABEL);
        hStatic_TotalJunkSize = GetDlgItem(hwnd, IDC_DISKCLEANUP_TOTALJUNK_SIZE);
        hStatic_DiskTotalLabel = GetDlgItem(hwnd, IDC_DISKCLEANUP_DISKTOTAL_LABEL);
        hStatic_DiskTotalSize = GetDlgItem(hwnd, IDC_DISKCLEANUP_DISKTOTAL_SIZE);
        hStatic_DiskUsedLabel = GetDlgItem(hwnd, IDC_DISKCLEANUP_DISKUSED_LABEL);
        hStatic_DiskUsedSize = GetDlgItem(hwnd, IDC_DISKCLEANUP_DISKUSED_SIZE);
        hStatic_DiskFreeLabel = GetDlgItem(hwnd, IDC_DISKCLEANUP_DISKFREE_LABEL);
        hStatic_DiskFreeSize = GetDlgItem(hwnd, IDC_DISKCLEANUP_DISKFREE_SIZE);

        // Створюємо контролери вкладки «Автоматичне обслуговування»
        CreateAutoMaintenanceControls(hwnd);
        // Отримуємо дескриптори нових контролів
        hStatic_AutoMaintTitle = GetDlgItem(hwnd, IDC_AUTO_MAINT_TITLE);
        hRadio_Daily = GetDlgItem(hwnd, IDC_AUTO_MAINT_DAILY);
        hRadio_Weekly = GetDlgItem(hwnd, IDC_AUTO_MAINT_WEEKLY);
        hRadio_OnStart = GetDlgItem(hwnd, IDC_AUTO_MAINT_ONSTART);
        hCheck_Temp = GetDlgItem(hwnd, IDC_AUTO_MAINT_CHECK_TEMP);
        hCheck_Browser = GetDlgItem(hwnd, IDC_AUTO_MAINT_CHECK_BROWSER);
        hCheck_Recycle = GetDlgItem(hwnd, IDC_AUTO_MAINT_CHECK_RECYCLE);
        hButton_SaveSettings = GetDlgItem(hwnd, IDC_AUTO_MAINT_SAVE);

       break;

        // ============================
       // Оновлено: Додаємо обробку двойного кліку по автозавантаженню
       // ============================
    case WM_NOTIFY: {
        LPNMHDR pnmhdr = (LPNMHDR)lParam;

        // --- Існуючий код для ProcessMonitor (не змінюємо) ---
        if (pnmhdr->hwndFrom == hListView_ProcessMonitor && pnmhdr->code == LVN_COLUMNCLICK) {
            LPNMLISTVIEW pnmv = (LPNMLISTVIEW)lParam;
            int clickedCol = pnmv->iSubItem;

            if (g_ProcessSortColumn == clickedCol) {
                g_ProcessSortAsc = !g_ProcessSortAsc;
            }
            else {
                g_ProcessSortColumn = clickedCol;
                g_ProcessSortAsc = true;
            }
            PopulateProcessList(hListView_ProcessMonitor);
        }

       

    } break;

    case WM_COMMAND: {
        int wmId = LOWORD(wParam);

        // Спершу ховаємо всі вкладки, щоби не було накладання
        switch (wmId) {
        case IDM_MONITORING:
        case IDM_DISK_CLEANUP:       
        case IDM_SETTINGS_AUTO_MAINTENANCE:
            // — Моніторинг —
            if (hListView_ProcessMonitor)    ShowWindow(hListView_ProcessMonitor, SW_HIDE);
            if (hStatic_CPUOverall)          ShowWindow(hStatic_CPUOverall, SW_HIDE);
            if (hStatic_MemoryOverall)       ShowWindow(hStatic_MemoryOverall, SW_HIDE);
            if (hStatic_IOReadOverall)       ShowWindow(hStatic_IOReadOverall, SW_HIDE);
            if (hStatic_IOWriteOverall)      ShowWindow(hStatic_IOWriteOverall, SW_HIDE);
            if (hStatic_DiskUsageOverall)    ShowWindow(hStatic_DiskUsageOverall, SW_HIDE);

            // — Очищення диска —
            if (hListView_DiskCleanup)       ShowWindow(hListView_DiskCleanup, SW_HIDE);
            if (hButton_AnalyzeDisk)         ShowWindow(hButton_AnalyzeDisk, SW_HIDE);
            if (hButton_CleanSelected)       ShowWindow(hButton_CleanSelected, SW_HIDE);
            if (hButton_CleanAll)            ShowWindow(hButton_CleanAll, SW_HIDE);
            if (hStatic_TotalJunkLabel)      ShowWindow(hStatic_TotalJunkLabel, SW_HIDE);
            if (hStatic_TotalJunkSize)       ShowWindow(hStatic_TotalJunkSize, SW_HIDE);
            if (hStatic_DiskTotalLabel)      ShowWindow(hStatic_DiskTotalLabel, SW_HIDE);
            if (hStatic_DiskTotalSize)       ShowWindow(hStatic_DiskTotalSize, SW_HIDE);
            if (hStatic_DiskUsedLabel)       ShowWindow(hStatic_DiskUsedLabel, SW_HIDE);
            if (hStatic_DiskUsedSize)        ShowWindow(hStatic_DiskUsedSize, SW_HIDE);
            if (hStatic_DiskFreeLabel)       ShowWindow(hStatic_DiskFreeLabel, SW_HIDE);
            if (hStatic_DiskFreeSize)        ShowWindow(hStatic_DiskFreeSize, SW_HIDE);

            if (hStatic_AutoMaintTitle) ShowWindow(hStatic_AutoMaintTitle, SW_HIDE);
            if (hRadio_Daily) ShowWindow(hRadio_Daily, SW_HIDE);
            if (hRadio_Weekly) ShowWindow(hRadio_Weekly, SW_HIDE);
            if (hRadio_OnStart)ShowWindow(hRadio_OnStart, SW_HIDE);
            if (hCheck_Temp)ShowWindow(hCheck_Temp, SW_HIDE);
            if (hCheck_Browser)ShowWindow(hCheck_Browser, SW_HIDE);
            if (hCheck_Recycle) ShowWindow(hCheck_Recycle, SW_HIDE);
            if (hButton_SaveSettings)ShowWindow(hButton_SaveSettings, SW_HIDE);

            // Зупиняємо таймер моніторингу (якщо він був)
            KillTimer(hwnd, IDT_MONITOR_REFRESH);
            break;
        }

        // Далі обробляємо самі команди
        switch (wmId) {
        case IDM_FILE_EXIT:
            DestroyWindow(hwnd);
            break;

        case IDM_MONITORING:
            // — показуємо вкладку "Моніторинг ресурсів" —
            if (hListView_ProcessMonitor) {
                SetTimer(hwnd, IDT_MONITOR_REFRESH, 1000, NULL);
                PopulateProcessList(hListView_ProcessMonitor);
                ShowWindow(hStatic_CPUOverall, SW_SHOW);
                ShowWindow(hStatic_MemoryOverall, SW_SHOW);
                ShowWindow(hStatic_IOReadOverall, SW_SHOW);
                ShowWindow(hStatic_IOWriteOverall, SW_SHOW);
                ShowWindow(hStatic_DiskUsageOverall, SW_SHOW);
                ShowWindow(hListView_ProcessMonitor, SW_SHOW);
                SetFocus(hListView_ProcessMonitor);
            }
            // — Оновлюємо ▶ у меню для "Моніторинг ресурсів" —
            UpdateMenuText(hwnd, IDM_MONITORING);
            break;

        case IDM_DISK_CLEANUP:
            // — показуємо вкладку "Очищення диска" —
            if (hListView_DiskCleanup) {
                ListView_DeleteAllItems(hListView_DiskCleanup);
                ShowWindow(hListView_DiskCleanup, SW_SHOW);
            }
            if (hButton_AnalyzeDisk)    ShowWindow(hButton_AnalyzeDisk, SW_SHOW);
            if (hButton_CleanSelected)  ShowWindow(hButton_CleanSelected, SW_SHOW);
            if (hButton_CleanAll)       ShowWindow(hButton_CleanAll, SW_SHOW);
            if (hStatic_TotalJunkLabel) ShowWindow(hStatic_TotalJunkLabel, SW_SHOW);
            if (hStatic_TotalJunkSize)  ShowWindow(hStatic_TotalJunkSize, SW_SHOW);
            if (hStatic_DiskTotalLabel) ShowWindow(hStatic_DiskTotalLabel, SW_SHOW);
            if (hStatic_DiskTotalSize)  ShowWindow(hStatic_DiskTotalSize, SW_SHOW);
            if (hStatic_DiskUsedLabel)  ShowWindow(hStatic_DiskUsedLabel, SW_SHOW);
            if (hStatic_DiskUsedSize)   ShowWindow(hStatic_DiskUsedSize, SW_SHOW);
            if (hStatic_DiskFreeLabel)  ShowWindow(hStatic_DiskFreeLabel, SW_SHOW);
            if (hStatic_DiskFreeSize)   ShowWindow(hStatic_DiskFreeSize, SW_SHOW);

            // — Оновлюємо ▶ у меню для "Очищення диска" —
            UpdateMenuText(hwnd, IDM_DISK_CLEANUP);
            break;

       
        case IDM_SETTINGS_AUTO_MAINTENANCE:
            // — Автоматичне обслуговування —
            if (hListView_ProcessMonitor){
       
            ShowWindow(hStatic_AutoMaintTitle, SW_SHOW);
            ShowWindow(hRadio_Daily, SW_SHOW);
            ShowWindow(hRadio_Weekly, SW_SHOW);
            ShowWindow(hRadio_OnStart, SW_SHOW);
            ShowWindow(hCheck_Temp, SW_SHOW);
            ShowWindow(hCheck_Browser, SW_SHOW);
            ShowWindow(hCheck_Recycle, SW_SHOW);
            ShowWindow(hButton_SaveSettings, SW_SHOW);
            }
            UpdateMenuText(hwnd, IDM_SETTINGS_AUTO_MAINTENANCE);
            break;

            // ===============================================
            // Обробка кнопки "Аналізувати Диск"
            // ===============================================
        case IDC_DISKCLEANUP_ANALYZE_BUTTON: {
            if (hListView_DiskCleanup) {
                AnalyzeDisk(hwnd, hListView_DiskCleanup);
            }
        } break;

        case IDC_DISKCLEANUP_CLEAN_BUTTON: {
            if (hListView_DiskCleanup) {
                CleanSelectedCategories(hwnd, hListView_DiskCleanup);
            }
        } break;

        case IDC_DISKCLEANUP_CLEAN_ALL: {
            if (hListView_DiskCleanup) {
                CleanAllCategories(hwnd, hListView_DiskCleanup);
            }
        } break;
        case IDC_AUTO_MAINT_SAVE: {
            // тут можна зчитати стани:
            BOOL daily = IsDlgButtonChecked(hwnd, IDC_AUTO_MAINT_DAILY);
            BOOL weekly = IsDlgButtonChecked(hwnd, IDC_AUTO_MAINT_WEEKLY);
            BOOL onstart = IsDlgButtonChecked(hwnd, IDC_AUTO_MAINT_ONSTART);
            BOOL tmp = IsDlgButtonChecked(hwnd, IDC_AUTO_MAINT_CHECK_TEMP);
            BOOL brw = IsDlgButtonChecked(hwnd, IDC_AUTO_MAINT_CHECK_BROWSER);
            BOOL rec = IsDlgButtonChecked(hwnd, IDC_AUTO_MAINT_CHECK_RECYCLE);
            // TODO: зберегти у файл/реєстр/налаштування
               // зберегти та запустити/перезапустити таймер
            AutoMaintenanceSettings s;
            s.scheduleType = IsDlgButtonChecked(hwnd, IDC_AUTO_MAINT_ONSTART)
                ? SCHEDULE_ON_STARTUP
                : SCHEDULE_INTERVAL;
            if (s.scheduleType == SCHEDULE_INTERVAL) {
                BOOL ok;
                int val = GetDlgItemInt(hwnd, IDC_AUTO_MAINT_EDIT_INTERVAL, &ok, FALSE);
                s.intervalMinutes = ok && val > 0 ? val : 60;
            }
            SaveAutoMaintenanceSettings(s);
            SetupAutoMaintenance(hwnd);
            MessageBox(hwnd, L"Налаштування збережено.", L"Автоматичне обслуговування", MB_OK | MB_ICONINFORMATION);
        } break;
        case IDM_SETTINGS_AUTOMAINT:
            // викликаємо наше вікно налаштувань
            ShowAutoMaintSettingsWindow(hMainWnd);
            break;

        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
        }
    } break;

    case WM_TIMER:
        if (wParam == IDT_MONITOR_REFRESH) {
            if (hListView_ProcessMonitor && IsWindowVisible(hListView_ProcessMonitor)) {
                PopulateProcessList(hListView_ProcessMonitor);
            }
        }
        else if (wParam == IDT_AUTO_MAINTENANCE) {
            PerformAutoMaintenance();
        }
        break;

    case WM_SIZE: {
        RECT rcClient;
        GetClientRect(hwnd, &rcClient);
        int clientWidth = rcClient.right - rcClient.left;
        int clientHeight = rcClient.bottom - rcClient.top;

        // --- Моніторинг процесів — ListView ---
        if (hListView_ProcessMonitor && IsWindowVisible(hListView_ProcessMonitor)) {
            int yOffset = 0;
            SetWindowPos(hListView_ProcessMonitor, NULL,
                0, yOffset,
                clientWidth,
                clientHeight - yOffset,
                SWP_NOZORDER);

            // Фіксовані ширини для стовпців 1–4
            const int widthCol1 = 100;   // “Екземпляри”
            const int widthCol2 = 100;   // “ЦП (%)”
            const int widthCol3 = 100;   // “Пам’ять (МБ)”
            const int widthCol4 = 125;   // “Диск (Гб/с)”
            int totalFixedWidth = widthCol1 + widthCol2 + widthCol3 + widthCol4;

            // Ширина останнього стовпця (index 0), щоб заповнити залишок
            int scrollBarWidth = GetSystemMetrics(SM_CXVSCROLL);
            int widthCol0 = clientWidth - totalFixedWidth - scrollBarWidth;
            if (widthCol0 < 190) widthCol0 = 190; // мінімальна ширина, щоб не “захлиналoся”

            ListView_SetColumnWidth(hListView_ProcessMonitor, 0, widthCol0);
            ListView_SetColumnWidth(hListView_ProcessMonitor, 1, widthCol1);
            ListView_SetColumnWidth(hListView_ProcessMonitor, 2, widthCol2);
            ListView_SetColumnWidth(hListView_ProcessMonitor, 3, widthCol3);
            ListView_SetColumnWidth(hListView_ProcessMonitor, 4, widthCol4);

        }

        // --- Очищення диска ---
        if (hListView_DiskCleanup && IsWindowVisible(hListView_DiskCleanup)) {

            int controlXOffset = 10;
            int controlYOffset = 10;
            int clientW = clientWidth;
            int listViewH = 300;

            // ListView
            SetWindowPos(hListView_DiskCleanup, NULL,
                controlXOffset, controlYOffset,
                clientW - 2 * controlXOffset, listViewH,
                SWP_NOZORDER);
            ListView_SetColumnWidth(hListView_DiskCleanup, 0, (clientW - 2 * controlXOffset) - 150 - GetSystemMetrics(SM_CXVSCROLL));
            ListView_SetColumnWidth(hListView_DiskCleanup, 1, 167);

            // Кнопки та статичні елементи
            int buttonHeight = 30;
            int buttonWidth = 150;
            int buttonY = controlYOffset + listViewH + 10;

            SetWindowPos(hButton_AnalyzeDisk, NULL,
                controlXOffset, buttonY,
                buttonWidth, buttonHeight,
                SWP_NOZORDER);
            SetWindowPos(hButton_CleanSelected, NULL,
                controlXOffset + buttonWidth + 10, buttonY,
                buttonWidth, buttonHeight,
                SWP_NOZORDER);
            SetWindowPos(hButton_CleanAll, NULL,
                controlXOffset + 2 * (buttonWidth + 10), buttonY,
                buttonWidth, buttonHeight,
                SWP_NOZORDER);

            int labelY = buttonY + buttonHeight + 10;
            int labelTotalNameW = 150;
            SetWindowPos(hStatic_TotalJunkLabel, NULL,
                controlXOffset, labelY,
                labelTotalNameW, 20,
                SWP_NOZORDER);
            SetWindowPos(hStatic_TotalJunkSize, NULL,
                controlXOffset + labelTotalNameW + 5, labelY,
                (clientW - 2 * controlXOffset) - (labelTotalNameW + 5), 20,
                SWP_NOZORDER);

            // Далі статичні "Загалом (C:)", "Зайнято (C:)", "Вільно (C:)"
            int offsetY2 = labelY + 25;
            SetWindowPos(hStatic_DiskFreeLabel, NULL,
                controlXOffset, offsetY2,
                labelTotalNameW, 20,
                SWP_NOZORDER);
            SetWindowPos(hStatic_DiskFreeSize, NULL,
                controlXOffset + labelTotalNameW + 5, offsetY2,
                (clientW - 2 * controlXOffset) - (labelTotalNameW + 5), 20,
                SWP_NOZORDER);
            offsetY2 += 25;
            SetWindowPos(hStatic_DiskUsedLabel, NULL,
                controlXOffset, offsetY2,
                labelTotalNameW, 20,
                SWP_NOZORDER);
            SetWindowPos(hStatic_DiskUsedSize, NULL,
                controlXOffset + labelTotalNameW + 5, offsetY2,
                (clientW - 2 * controlXOffset) - (labelTotalNameW + 5), 20,
                SWP_NOZORDER);
            offsetY2 += 25;
            SetWindowPos(hStatic_DiskTotalLabel, NULL,
                controlXOffset, offsetY2,
                labelTotalNameW, 20,
                SWP_NOZORDER);
            SetWindowPos(hStatic_DiskTotalSize, NULL,
                controlXOffset + labelTotalNameW + 5, offsetY2,
                (clientW - 2 * controlXOffset) - (labelTotalNameW + 5), 20,
                SWP_NOZORDER);
            
        }            
    } break;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
        EndPaint(hwnd, &ps);
    } break;

    case WM_DESTROY:
        KillTimer(hwnd, IDT_MONITOR_REFRESH);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}
