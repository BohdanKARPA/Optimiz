// AutoMaintenance.cpp 

#include "AutoMaintenance.h"
#include "Utils.h"
#include "ResourceDefinitions.h"
#include <CommCtrl.h>
#include <windows.h>
#include <chrono>
#include <string>  // <-- Додано для std::wstring та std::to_wstring
#include <vector>  // <-- Додано для std::vector
#include <time.h> 

static const wchar_t* REG_PATH = L"Software\\Optimiz\\AutoMaintenance";

// Попереднє оголошення, якщо визначення функції нижче
void ClearAllBrowserCaches();

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
        WS_CHILD | WS_VISIBLE,
        x, y, w, 20,
        hWndParent, (HMENU)IDC_AUTO_MAINT_TITLE,
        GetModuleHandle(NULL), NULL);
    y += 30;

    // Періодичність (радіокнопки)
    CreateWindowExW(0, L"BUTTON", L"Щодня",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP,
        x, y, 100, 20,
        hWndParent, (HMENU)IDC_AUTO_MAINT_DAILY,
        GetModuleHandle(NULL), NULL);
    CreateWindowExW(0, L"BUTTON", L"Щотижня",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        x + 110, y, 100, 20,
        hWndParent, (HMENU)IDC_AUTO_MAINT_WEEKLY,
        GetModuleHandle(NULL), NULL);
    CreateWindowExW(0, L"BUTTON", L"При старті програми",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        x + 220, y, 150, 20,
        hWndParent, (HMENU)IDC_AUTO_MAINT_ONSTART,
        GetModuleHandle(NULL), NULL);
    y += 40;

    // Категорії очищення (чекбокси)
    CreateWindowExW(0, L"BUTTON", L"Тимчасові файли",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        x, y, 150, 20,
        hWndParent, (HMENU)IDC_AUTO_MAINT_CHECK_TEMP,
        GetModuleHandle(NULL), NULL);
    CreateWindowExW(0, L"BUTTON", L"Кеш браузерів",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        x + 160, y, 150, 20,
        hWndParent, (HMENU)IDC_AUTO_MAINT_CHECK_BROWSER,
        GetModuleHandle(NULL), NULL);
    CreateWindowExW(0, L"BUTTON", L"Кошик",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        x + 320, y, 100, 20,
        hWndParent, (HMENU)IDC_AUTO_MAINT_CHECK_RECYCLE,
        GetModuleHandle(NULL), NULL);
    y += 40;

    // Кнопка збереження
    CreateWindowExW(0, L"BUTTON", L"Зберегти",
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        x, y, 100, 25,
        hWndParent, (HMENU)IDC_AUTO_MAINT_SAVE,
        GetModuleHandle(NULL), NULL);


    y += 35;  // відступ під кнопкою "Зберегти"
    CreateWindowExW(
        0, L"STATIC", L"Наступний запуск: —",
        WS_CHILD | WS_VISIBLE,
        x, y, 300, 20,
        hWndParent, (HMENU)IDC_AUTO_MAINT_NEXT,
        GetModuleHandle(NULL), NULL
    );
}

bool LoadAutoMaintenanceSettings(AutoMaintenanceSettings& s) {
    HKEY hKey;
    DWORD type, data, size = sizeof(data);
    if (RegOpenKeyExW(HKEY_CURRENT_USER, REG_PATH, 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        // Якщо ключ не існує, встановлюємо значення за замовчуванням
        s.scheduleType = SCHEDULE_ON_STARTUP;
        s.intervalMinutes = 60;
        s.cleanTempFiles = true;
        s.cleanBrowserCache = true;
        s.cleanRecycleBin = false;
        return false;
    }

    if (RegQueryValueExW(hKey, L"ScheduleType", NULL, &type, (LPBYTE)&data, &size) == ERROR_SUCCESS) s.scheduleType = (ScheduleType)data;
    else s.scheduleType = SCHEDULE_ON_STARTUP;

    size = sizeof(data);
    if (RegQueryValueExW(hKey, L"IntervalMinutes", NULL, &type, (LPBYTE)&data, &size) == ERROR_SUCCESS) s.intervalMinutes = (int)data;
    else s.intervalMinutes = 60;

    size = sizeof(data);
    if (RegQueryValueExW(hKey, L"CleanTempFiles", NULL, &type, (LPBYTE)&data, &size) == ERROR_SUCCESS) s.cleanTempFiles = (bool)data;
    else s.cleanTempFiles = true;

    size = sizeof(data);
    if (RegQueryValueExW(hKey, L"CleanBrowserCache", NULL, &type, (LPBYTE)&data, &size) == ERROR_SUCCESS) s.cleanBrowserCache = (bool)data;
    else s.cleanBrowserCache = true;

    size = sizeof(data);
    if (RegQueryValueExW(hKey, L"CleanRecycleBin", NULL, &type, (LPBYTE)&data, &size) == ERROR_SUCCESS) s.cleanRecycleBin = (bool)data;
    else s.cleanRecycleBin = false;

    RegCloseKey(hKey);
    return true;
}

bool SaveAutoMaintenanceSettings(const AutoMaintenanceSettings& s) {
    HKEY hKey;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, REG_PATH, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) != ERROR_SUCCESS) return false;

    DWORD data;
    data = (DWORD)s.scheduleType;
    RegSetValueExW(hKey, L"ScheduleType", 0, REG_DWORD, (BYTE*)&data, sizeof(data));
    data = (DWORD)s.intervalMinutes;
    RegSetValueExW(hKey, L"IntervalMinutes", 0, REG_DWORD, (BYTE*)&data, sizeof(data));
    data = (DWORD)s.cleanTempFiles;
    RegSetValueExW(hKey, L"CleanTempFiles", 0, REG_DWORD, (BYTE*)&data, sizeof(data));
    data = (DWORD)s.cleanBrowserCache;
    RegSetValueExW(hKey, L"CleanBrowserCache", 0, REG_DWORD, (BYTE*)&data, sizeof(data));
    data = (DWORD)s.cleanRecycleBin;
    RegSetValueExW(hKey, L"CleanRecycleBin", 0, REG_DWORD, (BYTE*)&data, sizeof(data));

    RegCloseKey(hKey);
    return true;
}

void SetupAutoMaintenance(HWND hWnd) {
    KillTimer(hWnd, IDT_AUTO_MAINTENANCE);
    AutoMaintenanceSettings s;
    if (!LoadAutoMaintenanceSettings(s)) {
        SaveAutoMaintenanceSettings(s);
    }
    if (s.scheduleType == SCHEDULE_ON_STARTUP) {
        PerformAutoMaintenance();
    }
    else if (s.scheduleType == SCHEDULE_INTERVAL && s.intervalMinutes > 0) {
        UINT ms = (UINT)s.intervalMinutes * 60 * 1000;
        SetTimer(hWnd, IDT_AUTO_MAINTENANCE, ms, NULL);
    }
    {
        HWND hNext = GetDlgItem(hWnd, IDC_AUTO_MAINT_NEXT);
        if (hNext) {
            if (s.scheduleType == SCHEDULE_ON_STARTUP) {
                SetWindowTextW(hNext, L"Наступний запуск: при старті програми");
            }
            else {
                time_t now = time(NULL);
                time_t next = now + (time_t)s.intervalMinutes * 60;
                struct tm tmNext;
                localtime_s(&tmNext, &next);
                wchar_t buf[64];
                swprintf(buf, _countof(buf),
                    L"Наступний запуск: %02d.%02d.%04d %02d:%02d",
                    tmNext.tm_mday, tmNext.tm_mon + 1, tmNext.tm_year + 1900,
                    tmNext.tm_hour, tmNext.tm_min);
                SetWindowTextW(hNext, buf);
            }
        }
    }

}

// Допоміжна функція для очищення кешу всіх браузерів (визначення ПЕРЕД викликом)
void ClearAllBrowserCaches() {
    std::wstring localAppData = ExpandEnvironmentStringsForPath(L"%LOCALAPPDATA%");
    if (localAppData.empty()) {
        WriteLog(L"AutoMaintenance: Не вдалося отримати шлях %LOCALAPPDATA%");
        return;
    }

    // Edge
    std::vector<std::wstring> edgePaths = {
        localAppData + L"\\Microsoft\\Edge\\User Data\\Default\\Cache",
        localAppData + L"\\Microsoft\\Edge\\User Data\\Default\\Code Cache",
        localAppData + L"\\Microsoft\\Edge\\User Data\\Default\\GPUCache"
    };
    for (const auto& path : edgePaths) {
        DeleteDirectoryContents(path);
    }
    WriteLog(L"AutoMaintenance: Очищено кеш Microsoft Edge.");

    // Chrome
    std::vector<std::wstring> chromePaths = {
        localAppData + L"\\Google\\Chrome\\User Data\\Default\\Cache",
        localAppData + L"\\Google\\Chrome\\User Data\\Default\\Code Cache",
        localAppData + L"\\Google\\Chrome\\User Data\\Default\\GPUCache"
    };
    for (const auto& path : chromePaths) {
        DeleteDirectoryContents(path);
    }
    WriteLog(L"AutoMaintenance: Очищено кеш Google Chrome.");

    // Firefox
    std::wstring appData = ExpandEnvironmentStringsForPath(L"%APPDATA%");
    std::vector<std::wstring> ffProfileBasePaths;
    if (!appData.empty()) ffProfileBasePaths.push_back(appData + L"\\Mozilla\\Firefox\\Profiles");
    if (!localAppData.empty()) ffProfileBasePaths.push_back(localAppData + L"\\Mozilla\\Firefox\\Profiles");
    for (const auto& basePath : ffProfileBasePaths) {
        WIN32_FIND_DATA fd;
        HANDLE hFind = FindFirstFile((basePath + L"\\*").c_str(), &fd);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
                    (wcsstr(fd.cFileName, L".default") || wcsstr(fd.cFileName, L"-release"))) {
                    std::wstring profilePath = basePath + L"\\" + fd.cFileName;
                    DeleteDirectoryContents(profilePath + L"\\cache2");
                    DeleteDirectoryContents(profilePath + L"\\startupCache");
                }
            } while (FindNextFile(hFind, &fd) != 0);
            FindClose(hFind);
        }
    }
    WriteLog(L"AutoMaintenance: Очищено кеш Mozilla Firefox.");
}

void PerformAutoMaintenance() {
    WriteLog(L"Запуск автоматичного обслуговування...");
    AutoMaintenanceSettings s;
    LoadAutoMaintenanceSettings(s);
    int cleanedCount = 0;
    std::wstring reportMessage = L"Автоматичне обслуговування завершено.\n\n";
    if (s.cleanTempFiles) {
        wchar_t pathBuffer[MAX_PATH];
        if (GetWindowsDirectory(pathBuffer, MAX_PATH)) {
            std::wstring winTemp = std::wstring(pathBuffer) + L"\\Temp";
            DeleteDirectoryContents(winTemp);
            WriteLog(L"AutoMaintenance: Очищено системні тимчасові файли.");
        }
        if (GetTempPath(MAX_PATH, pathBuffer)) {
            std::wstring userTemp = pathBuffer;
            if (!userTemp.empty() && (userTemp.back() == L'\\' || userTemp.back() == L'/')) {
                userTemp.pop_back();
            }
            DeleteDirectoryContents(userTemp);
            WriteLog(L"AutoMaintenance: Очищено користувацькі тимчасові файли.");
        }
        reportMessage += L"• Тимчасові файли очищено.\n";
        cleanedCount++;
    }
    if (s.cleanBrowserCache) {
        ClearAllBrowserCaches();
        reportMessage += L"• Кеш браузерів очищено.\n";
        cleanedCount++;
    }
    if (s.cleanRecycleBin) {
        EmptyRecycleBin();
        WriteLog(L"AutoMaintenance: Очищено Кошик.");
        reportMessage += L"• Кошик очищено.\n";
        cleanedCount++;
    }
    WriteLog(L"Автоматичне обслуговування завершено.");
    if (cleanedCount > 0) {
        MessageBox(NULL, reportMessage.c_str(), L"Автоматичне обслуговування", MB_OK | MB_ICONINFORMATION);
    }
}