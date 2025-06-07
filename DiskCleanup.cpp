//DiskCleanup.cpp

#include "DiskCleanup.h"
#include <windows.h>
#include <shellapi.h>       // SHQueryRecycleBin, SHEmptyRecycleBin
#include <psapi.h>          // EnumProcesses, GetProcessMemoryInfo, якщо потрібне
#include <CommCtrl.h>       // ListView, кнопки тощо
#include <algorithm>
#include <vector>
#include <string>
#include <shlobj.h>         // SHGetFolderPath, SHQueryRecycleBinW
#include <Shlwapi.h>        // PathFileExists

#pragma comment(lib, "Shlwapi.lib")     // Лінкуємо Shlwapi для PathFileExists

#include "Utils.h"          // WriteLog, FormatBytes, ExpandEnvironmentStringsForPath
#include "ResourceDefinitions.h"

// ———————————————————————
// Глобальні змінні (перенесені з Main.cpp)
// ———————————————————————
std::vector<JunkCategoryInfo> g_JunkCategories;
ULONGLONG g_TotalJunkFound = 0;

// ============================
// Створення контролів вкладки "Очищення диска"
// ============================
void CreateDiskCleanupControls(HWND hWndParent) {
    RECT rcClient;
    GetClientRect(hWndParent, &rcClient);

    int controlXOffset = 10;
    int controlYOffset = 10;
    int clientWidth = rcClient.right - rcClient.left;
    int listViewHeight = 300;
    int currentCtrlWidth = clientWidth - 2 * controlXOffset;
    if (currentCtrlWidth < 200) currentCtrlWidth = 200;

    // 1) ListView для категорій «сміття»
    HWND hListView = CreateWindowEx(
        0, WC_LISTVIEW, L"",
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_EX_FULLROWSELECT | LVS_SHOWSELALWAYS | WS_BORDER,
        controlXOffset, controlYOffset, currentCtrlWidth, listViewHeight,
        hWndParent, (HMENU)IDC_DISKCLEANUP_LISTVIEW, GetModuleHandle(NULL), NULL
    );
    ListView_SetExtendedListViewStyle(
        hListView,
        LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER
    );
    {
        LVCOLUMN lvc = {};
        lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
        // Колонка "Категорія"
        lvc.iSubItem = 0;
        lvc.pszText = (LPWSTR)L"Категорія";
        lvc.cx = currentCtrlWidth - 150 - GetSystemMetrics(SM_CXVSCROLL);
        lvc.fmt = LVCFMT_LEFT;
        ListView_InsertColumn(hListView, 0, &lvc);
        // Колонка "Розмір"
        lvc.iSubItem = 1;
        lvc.pszText = (LPWSTR)L"Розмір";
        lvc.cx = 167;
        lvc.fmt = LVCFMT_RIGHT;
        ListView_InsertColumn(hListView, 1, &lvc);
    }

    // 2) Кнопки нижче ListView
    controlYOffset += listViewHeight + 10;
    int buttonHeight = 30;
    int buttonWidth = 150;

    HWND hButtonAnalyze = CreateWindowEx(
        0, L"BUTTON", L"Аналізувати Диск",
        WS_TABSTOP | WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        controlXOffset, controlYOffset, buttonWidth, buttonHeight,
        hWndParent, (HMENU)IDC_DISKCLEANUP_ANALYZE_BUTTON, GetModuleHandle(NULL), NULL
    );
    HWND hButtonCleanSel = CreateWindowEx(
        0, L"BUTTON", L"Очистити вибране",
        WS_TABSTOP | WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        controlXOffset + buttonWidth + 10, controlYOffset, buttonWidth, buttonHeight,
        hWndParent, (HMENU)IDC_DISKCLEANUP_CLEAN_BUTTON, GetModuleHandle(NULL), NULL
    );
    HWND hButtonCleanAll = CreateWindowEx(
        0, L"BUTTON", L"Очистити все",
        WS_TABSTOP | WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        controlXOffset + 2 * (buttonWidth + 10), controlYOffset, buttonWidth, buttonHeight,
        hWndParent, (HMENU)IDC_DISKCLEANUP_CLEAN_ALL, GetModuleHandle(NULL), NULL
    );

    // 3) Статичні підписи та значення
    controlYOffset += buttonHeight + 10;
    int labelHeight = 20;
    int labelNameWidth = 150;

    HWND hStaticTotalLabel = CreateWindowEx(
        0, L"STATIC", L"Загалом знайдено:",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        controlXOffset, controlYOffset, labelNameWidth, labelHeight,
        hWndParent, (HMENU)IDC_DISKCLEANUP_TOTALJUNK_LABEL, GetModuleHandle(NULL), NULL
    );
    HWND hStaticTotalSize = CreateWindowEx(
        0, L"STATIC", L"0 B",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        controlXOffset + labelNameWidth + 5, controlYOffset,
        currentCtrlWidth - (labelNameWidth + 5), labelHeight,
        hWndParent, (HMENU)IDC_DISKCLEANUP_TOTALJUNK_SIZE, GetModuleHandle(NULL), NULL
    );

    // 6) "Вільно (C:)" / "0 B"
    controlYOffset += labelHeight + 5;
    HWND hStaticDiskFreeLabel = CreateWindowEx(
        0, L"STATIC", L"Вільно   (C:\\):",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        controlXOffset, controlYOffset, labelNameWidth, labelHeight,
        hWndParent, (HMENU)IDC_DISKCLEANUP_DISKFREE_LABEL, GetModuleHandle(NULL), NULL
    );
    HWND hStaticDiskFreeSize = CreateWindowEx(
        0, L"STATIC", L"0 B",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        controlXOffset + labelNameWidth + 5, controlYOffset,
        currentCtrlWidth - (labelNameWidth + 5), labelHeight,
        hWndParent, (HMENU)IDC_DISKCLEANUP_DISKFREE_SIZE, GetModuleHandle(NULL), NULL
    );
    // 5) "Зайнято (C:)" / "0 B"
    controlYOffset += labelHeight + 5;
    HWND hStaticDiskUsedLabel = CreateWindowEx(
        0, L"STATIC", L"Зайнято (C:\\):",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        controlXOffset, controlYOffset, labelNameWidth, labelHeight,
        hWndParent, (HMENU)IDC_DISKCLEANUP_DISKUSED_LABEL, GetModuleHandle(NULL), NULL
    );
    HWND hStaticDiskUsedSize = CreateWindowEx(
        0, L"STATIC", L"0 B",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        controlXOffset + labelNameWidth + 5, controlYOffset,
        currentCtrlWidth - (labelNameWidth + 5), labelHeight,
        hWndParent, (HMENU)IDC_DISKCLEANUP_DISKUSED_SIZE, GetModuleHandle(NULL), NULL
    );

    // 4) Статичні "Загалом (C:):" / "0 B"
    controlYOffset += labelHeight + 5;
    HWND hStaticDiskTotalLabel = CreateWindowEx(
        0, L"STATIC", L"Загалом (C:\\):",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        controlXOffset, controlYOffset, labelNameWidth, labelHeight,
        hWndParent, (HMENU)IDC_DISKCLEANUP_DISKTOTAL_LABEL, GetModuleHandle(NULL), NULL
    );
    HWND hStaticDiskTotalSize = CreateWindowEx(
        0, L"STATIC", L"0 B",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        controlXOffset + labelNameWidth + 5, controlYOffset,
        currentCtrlWidth - (labelNameWidth + 5), labelHeight,
        hWndParent, (HMENU)IDC_DISKCLEANUP_DISKTOTAL_SIZE, GetModuleHandle(NULL), NULL
    );
}


// ============================
// Аналіз диска (створює g_JunkCategories + заповнює ListView)
// ============================
void AnalyzeDisk(HWND hwnd, HWND hListView) {
    if (!hListView) return;

    // 0) Зберігаємо початкове вільне місце на диску C: (для подальших порівнянь)
    ULARGE_INTEGER initialFreeSpace = {};
    wchar_t winPathBuf[MAX_PATH];
    if (GetWindowsDirectory(winPathBuf, MAX_PATH)) {
        wchar_t rootDrive[4] = { winPathBuf[0], L':', L'\\', 0 };
        ULARGE_INTEGER freeBytesAvail, totalBytes, totalFreeBytes;
        if (GetDiskFreeSpaceEx(rootDrive, &freeBytesAvail, &totalBytes, &totalFreeBytes)) {
            initialFreeSpace.QuadPart = totalFreeBytes.QuadPart;
        }
    }

    // 1) Блокуємо кнопку "Аналізувати", щоб уникнути повторних натискань
    HWND hButtonAnalyze = GetDlgItem(hwnd, IDC_DISKCLEANUP_ANALYZE_BUTTON);
    if (hButtonAnalyze) EnableWindow(hButtonAnalyze, FALSE);
    SetCursor(LoadCursor(NULL, IDC_WAIT));

    // 2) Очищаємо попередній список у ListView та глобальні дані
    ListView_DeleteAllItems(hListView);
    g_JunkCategories.clear();
    g_TotalJunkFound = 0;

    wchar_t pathBuffer[MAX_PATH];

    // --- 3) Системні тимчасові файли (Windows\Temp) ---
    if (GetWindowsDirectory(pathBuffer, MAX_PATH)) {
        std::wstring winTemp = std::wstring(pathBuffer) + L"\\Temp";
        ULARGE_INTEGER winTempSize = GetDirectorySize(winTemp);
        g_JunkCategories.push_back({
            L"Системні тимчасові файли (Windows\\Temp)",
            winTempSize,
            winTemp
            });
        g_TotalJunkFound += winTempSize.QuadPart;
    }
    else {
        g_JunkCategories.push_back({
            L"Системні тимчасові файли (Windows\\Temp) (помилка)",
            {0},
            L""
            });
    }

    // --- 4) Системні лог-файли (%SystemRoot%\System32\LogFiles) замість заглушки ---
    {
        if (GetSystemDirectory(pathBuffer, MAX_PATH)) {
            std::wstring logFilesPath = std::wstring(pathBuffer) + L"\\LogFiles";
            if (PathFileExists(logFilesPath.c_str())) {
                ULARGE_INTEGER systemLogsSize = GetDirectorySize(logFilesPath);
                g_JunkCategories.push_back({
                    L"Системні лог-файли (System32\\LogFiles)",
                    systemLogsSize,
                    logFilesPath
                    });
                g_TotalJunkFound += systemLogsSize.QuadPart;
            }
            else {
                // Якщо папки LogFiles немає на диску
                g_JunkCategories.push_back({
                    L"Системні лог-файли (LogFiles не знайдено)",
                    {0},
                    L""
                    });
                WriteLog(L"Папка LogFiles не знайдена: " + logFilesPath);
            }
        }
        else {
            // Не вдалося отримати шлях %SystemRoot%
            g_JunkCategories.push_back({
                L"Системні лог-файли (помилка отримання шляху)",
                {0},
                L""
                });
            WriteLog(L"Не вдалося отримати шлях до %SystemRoot% для пошуку лог-файлів");
        }
    }

    // --- 5) Кошик ---
    {
        SHQUERYRBINFO sqrbi = {};
        sqrbi.cbSize = sizeof(SHQUERYRBINFO);
        ULARGE_INTEGER recycleSize = {};
        if (SHQueryRecycleBin(NULL, &sqrbi) == S_OK) {
            recycleSize.QuadPart = sqrbi.i64Size;
            g_JunkCategories.push_back({
                L"Кошик",
                recycleSize,
                L"RECYCLE_BIN"
                });
            g_TotalJunkFound += recycleSize.QuadPart;
        }
        else {
            g_JunkCategories.push_back({
                L"Кошик (помилка/немає даних)",
                {0},
                L""
                });
            WriteLog(L"Не вдалося отримати розмір Кошика");
        }
    }

    // --- 6) Користувацькі тимчасові файли (%TEMP%) ---
    if (GetTempPath(MAX_PATH, pathBuffer)) {
        std::wstring userTemp = pathBuffer;
        if (!userTemp.empty() && (userTemp.back() == L'\\' || userTemp.back() == L'/')) {
            userTemp.pop_back();
        }
        ULARGE_INTEGER userTempSize = GetDirectorySize(userTemp);
        g_JunkCategories.push_back({
            L"Користувацькі тимчасові файли (%TEMP%)",
            userTempSize,
            userTemp
            });
        g_TotalJunkFound += userTempSize.QuadPart;
    }
    else {
        g_JunkCategories.push_back({
            L"Користувацькі тимчасові файли (%TEMP%) (помилка)",
            {0},
            L""
            });
        WriteLog(L"Не вдалося отримати шлях %TEMP%");
    }

    // --- 7) Кеш Mozilla Firefox ---
    {
        std::wstring appData = ExpandEnvironmentStringsForPath(L"%APPDATA%");
        std::wstring localAppData = ExpandEnvironmentStringsForPath(L"%LOCALAPPDATA%");
        ULARGE_INTEGER firefoxTotalSize = {};
        std::wstring firefoxSamplePath;
        bool ffProfilesFound = false;
        std::vector<std::wstring> firefoxProfilePaths;
        if (!appData.empty()) firefoxProfilePaths.push_back(appData + L"\\Mozilla\\Firefox\\Profiles");
        if (!localAppData.empty()) firefoxProfilePaths.push_back(localAppData + L"\\Mozilla\\Firefox\\Profiles");

        for (const auto& basePath : firefoxProfilePaths) {
            WIN32_FIND_DATA fd;
            HANDLE hFindProfiles = FindFirstFile((basePath + L"\\*").c_str(), &fd);
            if (hFindProfiles != INVALID_HANDLE_VALUE) {
                do {
                    if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
                        (wcsstr(fd.cFileName, L".default") != nullptr ||
                            wcsstr(fd.cFileName, L".Default") != nullptr ||
                            wcsstr(fd.cFileName, L"-release") != nullptr ||
                            wcsstr(fd.cFileName, L"-esr") != nullptr))
                    {
                        ffProfilesFound = true;
                        std::wstring profilePath = basePath + L"\\" + fd.cFileName;
                        ULARGE_INTEGER cache2Size = GetDirectorySize(profilePath + L"\\cache2");
                        ULARGE_INTEGER startupSize = GetDirectorySize(profilePath + L"\\startupCache");
                        firefoxTotalSize.QuadPart += cache2Size.QuadPart;
                        firefoxTotalSize.QuadPart += startupSize.QuadPart;
                        if (firefoxSamplePath.empty()) {
                            firefoxSamplePath = profilePath + L"\\cache2";
                        }
                    }
                } while (FindNextFile(hFindProfiles, &fd) != 0);
                FindClose(hFindProfiles);
            }
        }
        if (ffProfilesFound || firefoxTotalSize.QuadPart > 0) {
            g_JunkCategories.push_back({
                L"Кеш Mozilla Firefox",
                firefoxTotalSize,
                firefoxSamplePath
                });
            g_TotalJunkFound += firefoxTotalSize.QuadPart;
        }
        else {
            g_JunkCategories.push_back({
                L"Кеш Mozilla Firefox (профілі не знайдено/порожньо)",
                {0},
                L""
                });
        }
    }

    // --- 8) Кеш Microsoft Edge ---
    {
        std::wstring localAppData = ExpandEnvironmentStringsForPath(L"%LOCALAPPDATA%");
        if (!localAppData.empty()) {
            std::vector<std::wstring> edgeCacheSubPaths = {
                L"\\Microsoft\\Edge\\User Data\\Default\\Cache",
                L"\\Microsoft\\Edge\\User Data\\Default\\Code Cache",
                L"\\Microsoft\\Edge\\User Data\\Default\\GPUCache"
            };
            ULARGE_INTEGER edgeTotalSize = {};
            std::wstring edgeSamplePath = localAppData + L"\\Microsoft\\Edge\\User Data\\Default\\Cache";
            for (const auto& subPath : edgeCacheSubPaths) {
                edgeTotalSize.QuadPart += GetDirectorySize(localAppData + subPath).QuadPart;
            }
            g_JunkCategories.push_back({
                L"Кеш Microsoft Edge",
                edgeTotalSize,
                edgeSamplePath
                });
            g_TotalJunkFound += edgeTotalSize.QuadPart;
        }
        else {
            g_JunkCategories.push_back({
                L"Кеш Microsoft Edge (шлях не знайдено)",
                {0},
                L""
                });
            WriteLog(L"Не вдалося отримати %LOCALAPPDATA% для Edge");
        }
    }

    // --- 9) Кеш Google Chrome ---
    {
        std::wstring localAppData = ExpandEnvironmentStringsForPath(L"%LOCALAPPDATA%");
        if (!localAppData.empty()) {
            std::vector<std::wstring> chromeCacheSubPaths = {
                L"\\Google\\Chrome\\User Data\\Default\\Cache",
                L"\\Google\\Chrome\\User Data\\Default\\Code Cache",
                L"\\Google\\Chrome\\User Data\\Default\\GPUCache"
            };
            ULARGE_INTEGER chromeTotalSize = {};
            std::wstring chromeSamplePath = localAppData + L"\\Google\\Chrome\\User Data\\Default\\Cache";
            for (const auto& subPath : chromeCacheSubPaths) {
                chromeTotalSize.QuadPart += GetDirectorySize(localAppData + subPath).QuadPart;
            }
            g_JunkCategories.push_back({
                L"Кеш Google Chrome",
                chromeTotalSize,
                chromeSamplePath
                });
            g_TotalJunkFound += chromeTotalSize.QuadPart;
        }
        else {
            g_JunkCategories.push_back({
                L"Кеш Google Chrome (шлях не знайдено)",
                {0},
                L""
                });
            WriteLog(L"Не вдалося отримати %LOCALAPPDATA% для Chrome");
        }
    }

    // --- 10) Наповнюємо ListView результатами аналізу ---
    {
        LVITEM lvi = {};
        for (size_t i = 0; i < g_JunkCategories.size(); ++i) {
            // Колонка 0: назва категорії
            lvi.mask = LVIF_TEXT | LVIF_PARAM;
            lvi.iItem = static_cast<int>(i);
            lvi.iSubItem = 0;
            lvi.pszText = (LPWSTR)g_JunkCategories[i].categoryName.c_str();
            lvi.lParam = (LPARAM)i;
            ListView_InsertItem(hListView, &lvi);

            // Колонка 1: розмір
            std::wstring sizeStr = FormatBytes(g_JunkCategories[i].size.QuadPart);
            lvi.mask = LVIF_TEXT;
            lvi.iItem = static_cast<int>(i);
            lvi.iSubItem = 1;
            lvi.pszText = (LPWSTR)sizeStr.c_str();
            ListView_SetItem(hListView, &lvi);
        }
    }

    // --- 11) Оновлюємо статичний текст "Загалом знайдено" ---
    std::wstring totalSizeStr = FormatBytes(g_TotalJunkFound);
    HWND hStaticTotalSize = GetDlgItem(hwnd, IDC_DISKCLEANUP_TOTALJUNK_SIZE);
    if (hStaticTotalSize) SetWindowText(hStaticTotalSize, totalSizeStr.c_str());

    // --- 12) Обчислюємо загальний, зайнятий та вільний простір диска C: ---
    {
        if (GetWindowsDirectory(winPathBuf, MAX_PATH)) {
            wchar_t rootDrive[4] = { winPathBuf[0], L':', L'\\', 0 };

            ULARGE_INTEGER freeBytesAvailable = {};
            ULARGE_INTEGER totalNumberOfBytes = {};
            ULARGE_INTEGER totalNumberOfFreeBytes = {};

            if (GetDiskFreeSpaceEx(rootDrive, &freeBytesAvailable, &totalNumberOfBytes, &totalNumberOfFreeBytes)) {
                ULONGLONG usedBytes = totalNumberOfBytes.QuadPart - totalNumberOfFreeBytes.QuadPart;
                std::wstring totalDiskStr = FormatBytes(totalNumberOfBytes.QuadPart);
                std::wstring usedDiskStr = FormatBytes(usedBytes);
                std::wstring freeDiskStr = FormatBytes(totalNumberOfFreeBytes.QuadPart);

                // Динамічно формуємо мітки для "Загалом (C:):", "Зайнято (C:):", "Вільно (C:):"
                wchar_t driveLabel[4] = { winPathBuf[0], L':', L'\\', 0 };
                std::wstring labelTotal = std::wstring(L"Загалом (") + driveLabel + L"):";
                HWND hLabelTotal = GetDlgItem(hwnd, IDC_DISKCLEANUP_DISKTOTAL_LABEL);
                if (hLabelTotal) SetWindowText(hLabelTotal, labelTotal.c_str());

                std::wstring labelUsed = std::wstring(L"Зайнято (") + driveLabel + L"):";
                HWND hLabelUsed = GetDlgItem(hwnd, IDC_DISKCLEANUP_DISKUSED_LABEL);
                if (hLabelUsed) SetWindowText(hLabelUsed, labelUsed.c_str());

                std::wstring labelFree = std::wstring(L"Вільно (") + driveLabel + L"):";
                HWND hLabelFree = GetDlgItem(hwnd, IDC_DISKCLEANUP_DISKFREE_LABEL);
                if (hLabelFree) SetWindowText(hLabelFree, labelFree.c_str());

                // Встановлюємо самі значення
                HWND hStaticTotal = GetDlgItem(hwnd, IDC_DISKCLEANUP_DISKTOTAL_SIZE);
                if (hStaticTotal) SetWindowText(hStaticTotal, totalDiskStr.c_str());

                HWND hStaticUsed = GetDlgItem(hwnd, IDC_DISKCLEANUP_DISKUSED_SIZE);
                if (hStaticUsed) SetWindowText(hStaticUsed, usedDiskStr.c_str());

                HWND hStaticFree = GetDlgItem(hwnd, IDC_DISKCLEANUP_DISKFREE_SIZE);
                if (hStaticFree) SetWindowText(hStaticFree, freeDiskStr.c_str());
            }
            else {
                // Якщо не вдалося отримати інформацію про диск
                HWND hStaticTotal = GetDlgItem(hwnd, IDC_DISKCLEANUP_DISKTOTAL_SIZE);
                if (hStaticTotal) SetWindowText(hStaticTotal, L"0 B");
                HWND hStaticUsed = GetDlgItem(hwnd, IDC_DISKCLEANUP_DISKUSED_SIZE);
                if (hStaticUsed) SetWindowText(hStaticUsed, L"0 B");
                HWND hStaticFree = GetDlgItem(hwnd, IDC_DISKCLEANUP_DISKFREE_SIZE);
                if (hStaticFree) SetWindowText(hStaticFree, L"0 B");
                WriteLog(L"GetDiskFreeSpaceEx не вдалося для диска C:");
            }
        }
    }

    // --- 13) Розблоковуємо кнопку "Аналізувати" і повертаємо курсор ---
    if (hButtonAnalyze) EnableWindow(hButtonAnalyze, TRUE);
    SetCursor(LoadCursor(NULL, IDC_ARROW));

    // (За потреби: тут можна зберегти initialFreeSpace для порівняння в Clean функціях)
}

// ============================
// Очищення вибраних категорій
// ============================
void CleanSelectedCategories(HWND hwnd, HWND hListView) {
    int selectedCount = ListView_GetSelectedCount(hListView);
     if (g_JunkCategories.empty()) {
        MessageBox(hwnd,
            L"Спочатку натисніть 'Аналізувати Диск', щоб знайти, що можна очистити.",
            L"Увага",
            MB_OK | MB_ICONINFORMATION);
        return;
    }
    if (selectedCount == 0) {
        MessageBox(hwnd,
            L"Будь ласка, виберіть щонайменше одну категорію для очищення.",
            L"Увага",
            MB_OK | MB_ICONINFORMATION);
        return;
    }
    if (MessageBox(hwnd,
        L"Ви впевнені, що бажаєте видалити обрані файли?",
        L"Підтвердження",
        MB_YESNO | MB_ICONQUESTION) != IDYES)
    {
        return;
    }

    std::vector<std::wstring> alreadyEmpty;
    std::vector<std::wstring> fullyCleaned;
    std::vector<std::wstring> partiallyCleaned;
    bool anyOperation = false;

    int index = -1;
    while (true) {
        index = ListView_GetNextItem(hListView, index, LVNI_SELECTED);
        if (index == -1) break;

        LVITEM lvi = {};
        lvi.mask = LVIF_PARAM;
        lvi.iItem = index;
        lvi.iSubItem = 0;
        ListView_GetItem(hListView, &lvi);
        int catIndex = (int)lvi.lParam;
        if (catIndex < 0 || catIndex >= (int)g_JunkCategories.size()) continue;

        JunkCategoryInfo& category = g_JunkCategories[catIndex];
        ULARGE_INTEGER origSize = {};
        if (category.path == L"RECYCLE_BIN") {
            SHQUERYRBINFO sqrbi = {};
            sqrbi.cbSize = sizeof(SHQUERYRBINFO);
            if (SHQueryRecycleBin(NULL, &sqrbi) == S_OK) {
                origSize.QuadPart = sqrbi.i64Size;
            }
        }
        else {
            if (!category.path.empty()) {
                origSize = GetDirectorySize(category.path);
            }
        }

        if (origSize.QuadPart == 0) {
            alreadyEmpty.push_back(category.categoryName);
            continue;
        }

        // Виконуємо очищення
        if (category.path == L"RECYCLE_BIN") {
            EmptyRecycleBin();
            anyOperation = true;
        }
        else {
            DeleteDirectoryContents(category.path);
            anyOperation = true;
        }

        // Перевіряємо новий розмір після очищення
        ULARGE_INTEGER newSize = {};
        if (category.path == L"RECYCLE_BIN") {
            SHQUERYRBINFO sqrbi = {};
            sqrbi.cbSize = sizeof(SHQUERYRBINFO);
            if (SHQueryRecycleBin(NULL, &sqrbi) == S_OK) {
                newSize.QuadPart = sqrbi.i64Size;
            }
        }
        else {
            if (!category.path.empty()) {
                newSize = GetDirectorySize(category.path);
            }
        }

        if (newSize.QuadPart == 0) {
            fullyCleaned.push_back(category.categoryName);
        }
        else if (newSize.QuadPart < origSize.QuadPart) {
            partiallyCleaned.push_back(category.categoryName);
        }
        else {
            alreadyEmpty.push_back(category.categoryName);
        }
    }

    // Формуємо підсумкове повідомлення
    std::wstring message;

    if (!alreadyEmpty.empty()) {
        message += L"Категорії, у яких нічого не було видалено (вже порожні або не вдалося очистити):\n";
        for (const auto& name : alreadyEmpty) {
            message += L"• " + name + L"\n";
        }
        message += L"\n";
    }
    if (!fullyCleaned.empty()) {
        message += L"Категорії, успішно повністю очищені:\n";
        for (const auto& name : fullyCleaned) {
            message += L"• " + name + L"\n";
        }
        message += L"\n";
    }
    if (!partiallyCleaned.empty()) {
        message += L"Категорії, очищені частково (залишилися заблоковані файли):\n";
        for (const auto& name : partiallyCleaned) {
            message += L"• " + name + L"\n";
        }
        message += L"\n";
    }

    if (!anyOperation) {
        MessageBox(hwnd, message.c_str(), L"Інформація", MB_OK | MB_ICONINFORMATION);
    }
    else {
        MessageBox(hwnd, message.c_str(), L"Результат очищення", MB_OK | MB_ICONINFORMATION);
        // Переаналізовуємо, щоб оновити розміри та список
        AnalyzeDisk(hwnd, hListView);
    }
}

// ============================
// Очищення усіх категорій (Clean All)
// ============================
void CleanAllCategories(HWND hwnd, HWND hListView) {
    if (g_JunkCategories.empty()) {
        MessageBox(hwnd,
            L"Спочатку натисніть 'Аналізувати Диск', щоб знайти, що можна очистити.",
            L"Увага",
            MB_OK | MB_ICONINFORMATION);
        return;
    }
    if (MessageBox(hwnd,
        L"Ви впевнені, що бажаєте очистити всі категорії? Це може зайняти деякий час.",
        L"Підтвердження",
        MB_YESNO | MB_ICONQUESTION) != IDYES)
    {
        return;
    }

    std::vector<std::wstring> alreadyEmpty;
    std::vector<std::wstring> fullyCleaned;
    std::vector<std::wstring> partiallyCleaned;
    bool anyOperation = false;

    for (size_t catIndex = 0; catIndex < g_JunkCategories.size(); ++catIndex) {
        JunkCategoryInfo& category = g_JunkCategories[catIndex];
        ULARGE_INTEGER origSize = {};
        if (category.path == L"RECYCLE_BIN") {
            SHQUERYRBINFO sqrbi = {};
            sqrbi.cbSize = sizeof(SHQUERYRBINFO);
            if (SHQueryRecycleBin(NULL, &sqrbi) == S_OK) {
                origSize.QuadPart = sqrbi.i64Size;
            }
        }
        else {
            if (!category.path.empty()) {
                origSize = GetDirectorySize(category.path);
            }
        }

        if (origSize.QuadPart == 0) {
            alreadyEmpty.push_back(category.categoryName);
            continue;
        }

        // Очищення
        if (category.path == L"RECYCLE_BIN") {
            EmptyRecycleBin();
            anyOperation = true;
        }
        else {
            DeleteDirectoryContents(category.path);
            anyOperation = true;
        }

        // Перевіряємо новий розмір
        ULARGE_INTEGER newSize = {};
        if (category.path == L"RECYCLE_BIN") {
            SHQUERYRBINFO sqrbi = {};
            sqrbi.cbSize = sizeof(SHQUERYRBINFO);
            if (SHQueryRecycleBin(NULL, &sqrbi) == S_OK) {
                newSize.QuadPart = sqrbi.i64Size;
            }
        }
        else {
            if (!category.path.empty()) {
                newSize = GetDirectorySize(category.path);
            }
        }

        if (newSize.QuadPart == 0) {
            fullyCleaned.push_back(category.categoryName);
        }
        else if (newSize.QuadPart < origSize.QuadPart) {
            partiallyCleaned.push_back(category.categoryName);
        }
        else {
            alreadyEmpty.push_back(category.categoryName);
        }
    }

    // Формуємо повідомлення
    std::wstring message;
    if (!alreadyEmpty.empty()) {
        message += L"Категорії, у яких нічого не було видалено (вже порожні або помилка):\n";
        for (const auto& name : alreadyEmpty) {
            message += L"• " + name + L"\n";
        }
        message += L"\n";
    }
    if (!fullyCleaned.empty()) {
        message += L"Категорії, успішно повністю очищені:\n";
        for (const auto& name : fullyCleaned) {
            message += L"• " + name + L"\n";
        }
        message += L"\n";
    }
    if (!partiallyCleaned.empty()) {
        message += L"Категорії, очищені частково (залишилися заблоковані файли):\n";
        for (const auto& name : partiallyCleaned) {
            message += L"• " + name + L"\n";
        }
        message += L"\n";
    }

    if (!anyOperation) {
        MessageBox(hwnd, message.c_str(), L"Інформація", MB_OK | MB_ICONINFORMATION);
    }
    else {
        MessageBox(hwnd, message.c_str(), L"Результат очищення", MB_OK | MB_ICONINFORMATION);
        // Переаналізовуємо
        AnalyzeDisk(hwnd, hListView);
    }
}

// Ця функція зайва, бо вся логіка у AnalyzeDisk виконує Populate
void PopulateDiskCleanupList(HWND hListView) {
    // реалізація не потрібна окремо
}
