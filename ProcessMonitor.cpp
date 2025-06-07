// ProcessMonitor.cpp

#include "ProcessMonitor.h"
#include "Utils.h"           // для ToLowerStr()
#include "ResourceDefinitions.h"

#include <windows.h>
#include <psapi.h>           // EnumProcesses, GetProcessMemoryInfo
#include <CommCtrl.h>        // ListView
#include <pdh.h>             // PDH-лічильники (вирізано використання для диска)
#pragma comment(lib, "pdh.lib")

#include <map>
#include <algorithm>
#include <sstream>
#include <iomanip>

// ====================
// Глобальні змінні
// ====================
std::vector<AggregatedProcessInfo> g_AggregatedProcessList;
int  g_ProcessSortColumn = 0;   // за замовчуванням сортуємо за Ім'ям
bool g_ProcessSortAsc = true;

static PDH_HQUERY    g_diskQuery = nullptr;
static PDH_HCOUNTER  g_diskPctCounter = nullptr;
// Загальний об’єм фізичної пам’яті (байти)
static ULONGLONG totalPhysicalMemory = 0;
void InitDiskCounter()
{
    if (g_diskQuery != nullptr)
        return; // Уже ініціалізовано

    // 1) Відкриваємо PDH-запит
    PDH_STATUS status = PdhOpenQuery(nullptr, 0, &g_diskQuery);
    if (status != ERROR_SUCCESS || g_diskQuery == nullptr) {
        // Не вдалося ініціалізувати PDH
        g_diskQuery = nullptr;
        return;
    }

    // 2) Додаємо лічильник \PhysicalDisk(_Total)\% Disk Time
    //    Зверніть увагу: якщо у вашій системі замість "% Disk Time" використовується "Active Time",
    //    можна взяти \PhysicalDisk(_Total)\% Active Time
    status = PdhAddCounter(
        g_diskQuery,
        L"\\PhysicalDisk(_Total)\\% Disk Time",
        0,
        &g_diskPctCounter
    );

    if (status != ERROR_SUCCESS) {
        // Не вдалося додати лічильник
        PdhCloseQuery(g_diskQuery);
        g_diskQuery = nullptr;
        g_diskPctCounter = nullptr;
        return;
    }

    // 3) Збираємо початкові дані (щоб наступного разу PdhCollectQueryData міг обчислити дельту)
    PdhCollectQueryData(g_diskQuery);
}

// Ініціалізація totalPhysicalMemory
void InitMemory()
{
    if (totalPhysicalMemory != 0)
        return;

    MEMORYSTATUSEX statex = {};
    statex.dwLength = sizeof(statex);
    if (GlobalMemoryStatusEx(&statex)) {
        totalPhysicalMemory = statex.ullTotalPhys;
    }
    else {
        totalPhysicalMemory = 1; // щоб уникнути ділення на нуль
    }
}

// ============================
// Сортування за колоною
// ============================
bool CompareByColumn(const AggregatedProcessInfo& a, const AggregatedProcessInfo& b) {
    switch (g_ProcessSortColumn) {
    case 0: // Ім'я
        return g_ProcessSortAsc
            ? (a.displayName > b.displayName)
            : (a.displayName < b.displayName);

    case 1: // Екземпляри
        return g_ProcessSortAsc
            ? (a.instanceCount > b.instanceCount)
            : (a.instanceCount < b.instanceCount);

    case 2: // ЦП (%)
        return g_ProcessSortAsc
            ? (a.totalCpuPercent > b.totalCpuPercent)
            : (a.totalCpuPercent < b.totalCpuPercent);

    case 3: // Пам'ять (МБ)
        return g_ProcessSortAsc
            ? (a.totalWorkingSetSize > b.totalWorkingSetSize)
            : (a.totalWorkingSetSize < b.totalWorkingSetSize);

    case 4: // Диск (Мб/с)
        return g_ProcessSortAsc
            ? (a.lastIoGBps > b.lastIoGBps)
            : (a.lastIoGBps < b.lastIoGBps);

    default:
        return false;
    }
}

// ============================
// Створення контролів ListView
// ============================
void CreateProcessMonitorControls(HWND hWndParent) {
    RECT rcClient;
    GetClientRect(hWndParent, &rcClient);

    HWND hListView = CreateWindowExW(
        0, WC_LISTVIEW, L"",
        WS_CHILD | WS_CLIPSIBLINGS | LVS_REPORT | LVS_SINGLESEL | WS_BORDER | LVS_SHOWSELALWAYS,
        0, 0, rcClient.right, rcClient.bottom,
        hWndParent, (HMENU)IDC_PROCESSLISTVIEW, GetModuleHandle(NULL), NULL
    );
    if (!hListView) return;

    ListView_SetExtendedListViewStyle(
        hListView,
        LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER
    );

    // Налаштовуємо колонки:
    LVCOLUMNW lvc = {};
    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;

    // 0: Ім'я
    lvc.iSubItem = 0;
    lvc.pszText = (LPWSTR)L"Ім'я";
    lvc.cx = 190;
    lvc.fmt = LVCFMT_LEFT;
    ListView_InsertColumn(hListView, 0, &lvc);

    // 1: Екземпляри
    lvc.iSubItem = 1;
    lvc.pszText = (LPWSTR)L"Екземпляри";
    lvc.cx = 100;
    lvc.fmt = LVCFMT_RIGHT;
    ListView_InsertColumn(hListView, 1, &lvc);

    // 2: ЦП (%)
    lvc.iSubItem = 2;
    lvc.pszText = (LPWSTR)L"ЦП (%)";
    lvc.cx = 100;
    lvc.fmt = LVCFMT_RIGHT;
    ListView_InsertColumn(hListView, 2, &lvc);

    // 3: Пам'ять (МБ)
    lvc.iSubItem = 3;
    lvc.pszText = (LPWSTR)L"Пам'ять (МБ)";
    lvc.cx = 100;
    lvc.fmt = LVCFMT_RIGHT;
    ListView_InsertColumn(hListView, 3, &lvc);

    // 4: Диск (Мбіт/с)    
    lvc.iSubItem = 4;
    lvc.pszText = (LPWSTR)L"Диск (Мбіт/с)";
    lvc.cx = 125;
    lvc.fmt = LVCFMT_RIGHT;
    ListView_InsertColumn(hListView, 4, &lvc);
}

// ============================
// Заповнення ListView даними
// ============================
void PopulateProcessList(HWND hListView) {
    if (!hListView) return;

    // 1) Ініціалізація пам’яті (отримати totalPhysicalMemory)
    InitMemory();

    // 2) Вимикаємо перерисовку, очищаємо ListView та вектор
    SendMessage(hListView, WM_SETREDRAW, FALSE, 0);
    ListView_DeleteAllItems(hListView);
    g_AggregatedProcessList.clear();

    // 3) Перебір процесів через EnumProcesses
    std::map<std::wstring, AggregatedProcessInfo> aggregatedMap;
    DWORD aProcesses[2048];
    DWORD cbNeeded = 0;

    if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded)) {
        SendMessage(hListView, WM_SETREDRAW, TRUE, 0);
        RedrawWindow(hListView, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
        return;
    }
    DWORD cProcesses = cbNeeded / sizeof(DWORD);

    // Кількість логічних ядер (для обчислення CPU %)
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    DWORD numProcessors = sysInfo.dwNumberOfProcessors;

    // Карта попередніх значень I/O (Read+Write) для кожного імені процесу
    static std::map<std::wstring, ULONGLONG> prevIoMap;

    for (DWORD i = 0; i < cProcesses; ++i) {
        if (aProcesses[i] == 0) continue;

        HANDLE hProcess = OpenProcess(
            PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_QUERY_LIMITED_INFORMATION,
            FALSE,
            aProcesses[i]
        );
        if (!hProcess) continue;

        // --- (3.1) Отримуємо ім'я процесу ---
        TCHAR szProcessName[MAX_PATH] = TEXT("<невідомо>");
        HMODULE hMod;
        DWORD cbNeededMod;
        if (EnumProcessModulesEx(hProcess, &hMod, sizeof(hMod), &cbNeededMod, LIST_MODULES_DEFAULT)) {
            GetModuleBaseNameW(hProcess, hMod, szProcessName, sizeof(szProcessName) / sizeof(TCHAR));
        }

        std::wstring procName = szProcessName;
        std::wstring keyName = ToLowerStr(procName);

        // --- (3.2) RAM (Working Set у байтах) ---
        PROCESS_MEMORY_COUNTERS_EX pmc = {};
        ULONGLONG workingSet = 0;
        if (GetProcessMemoryInfo(hProcess, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
            workingSet = pmc.WorkingSetSize;
        }

        // --- (3.3) CPU Time (Kernel + User) для обчислення CPU (%) ---
        FILETIME ftCreation = {}, ftExit = {}, ftKernel = {}, ftUser = {};
        ULONGLONG procCpuTime = 0;
        ULONGLONG creationTime100ns = 0;
        if (GetProcessTimes(hProcess, &ftCreation, &ftExit, &ftKernel, &ftUser)) {
            ULARGE_INTEGER uliK = {}, uliU = {};
            uliK.LowPart = ftKernel.dwLowDateTime;
            uliK.HighPart = ftKernel.dwHighDateTime;
            uliU.LowPart = ftUser.dwLowDateTime;
            uliU.HighPart = ftUser.dwHighDateTime;
            procCpuTime = uliK.QuadPart + uliU.QuadPart;
            creationTime100ns = ((ULARGE_INTEGER*)&ftCreation)->QuadPart;
        }

        // Обчислюємо CPU (%) для цього екземпляра процесу:
        FILETIME ftNow = {};
        GetSystemTimeAsFileTime(&ftNow);
        ULONGLONG now100ns = ((ULARGE_INTEGER*)&ftNow)->QuadPart;
        ULONGLONG elapsedTime100ns = (creationTime100ns < now100ns)
            ? (now100ns - creationTime100ns)
            : 1ULL;
        double cpuPercent = 0.0;
        if (elapsedTime100ns > 0) {
            cpuPercent = (static_cast<double>(procCpuTime) / static_cast<double>(elapsedTime100ns))
                * 100.0 / static_cast<double>(numProcessors);
        }

        // --- (3.4) I/O Counters (Read + Write байти) ---
        IO_COUNTERS ioCounters = {};
        ULONGLONG ioRead = 0, ioWrite = 0;
        if (GetProcessIoCounters(hProcess, &ioCounters)) {
            ioRead = ioCounters.ReadTransferCount;
            ioWrite = ioCounters.WriteTransferCount;
        }

        CloseHandle(hProcess);

        // --- (3.5) Агрегуємо інформацію ---
        auto it = aggregatedMap.find(keyName);
        if (it == aggregatedMap.end()) {
            // Новий запис
            AggregatedProcessInfo info;
            info.displayName = procName;
            info.instanceCount = 1;
            info.totalWorkingSetSize = workingSet;
            info.totalCpuPercent = cpuPercent;
            info.totalIoRead = ioRead;
            info.totalIoWrite = ioWrite;

            // Обчислюємо початковий lastIoGBps:
            ULONGLONG prevTotalIo = 0;
            auto itPrev = prevIoMap.find(keyName);
            if (itPrev != prevIoMap.end()) {
                prevTotalIo = itPrev->second;
            }
            ULONGLONG currentTotalIo = ioRead + ioWrite;
            ULONGLONG deltaBytes = (currentTotalIo >= prevTotalIo) ? (currentTotalIo - prevTotalIo) : 0ULL;

            // Переводимо deltaBytes (байти/с) у **мегабіти/секунду**:
            double mbps = (static_cast<double>(deltaBytes) * 8.0) / (1024.0 * 1024.0);
            info.lastIoGBps = mbps; // хоч назва поля "lastIoGBps", але тут зберігаємо значення в Мбіт/с
            prevIoMap[keyName] = currentTotalIo;

            aggregatedMap[keyName] = info;
        }
        else {
            // Оновлюємо існуючий запис
            auto& agg = it->second;
            agg.instanceCount++;
            agg.totalWorkingSetSize += workingSet;
            agg.totalCpuPercent += cpuPercent;
            agg.totalIoRead += ioRead;
            agg.totalIoWrite += ioWrite;

            ULONGLONG prevTotalIo = 0;
            auto itPrev = prevIoMap.find(keyName);
            if (itPrev != prevIoMap.end()) {
                prevTotalIo = itPrev->second;
            }
            ULONGLONG currentTotalIo = agg.totalIoRead + agg.totalIoWrite;
            ULONGLONG deltaBytes = (currentTotalIo >= prevTotalIo) ? (currentTotalIo - prevTotalIo) : 0ULL;

            // Переводимо deltaBytes (байти/с) у **мегабіти/секунду**:
            double mbps = (static_cast<double>(deltaBytes) * 8.0) / (1024.0 * 1024.0);
            agg.lastIoGBps = mbps; // зберігаємо в Мбіт/с
            prevIoMap[keyName] = currentTotalIo;
        }
    }

    // 5) Переносимо дані у вектор та сортуємо
    for (auto& p : aggregatedMap) {
        g_AggregatedProcessList.push_back(p.second);
    }
    std::sort(g_AggregatedProcessList.begin(), g_AggregatedProcessList.end(), CompareByColumn);

    // 6) Обчислюємо загальні підсумки:
    double grandTotalCpuPct = 0.0;
    double grandTotalMemPct = 0.0;
    double grandTotalDiskMbps = 0.0;

    for (const auto& proc : g_AggregatedProcessList) {
        grandTotalCpuPct += proc.totalCpuPercent;
        double memPctForProc = (static_cast<double>(proc.totalWorkingSetSize) / static_cast<double>(totalPhysicalMemory)) * 100.0;
        grandTotalMemPct += memPctForProc;
        grandTotalDiskMbps += proc.lastIoGBps;   // тут lastIoGBps вже містить Мбіт/с
    }

    // 7) Заповнюємо ListView рядками (нижні колонки: ім'я, екземпляри, CPU%, МБ, ГБ/с)
    LVITEMW lvi = {};
    for (size_t idx = 0; idx < g_AggregatedProcessList.size(); ++idx) {
        const auto& proc = g_AggregatedProcessList[idx];

        // 7.1) Ім'я
        lvi.mask = LVIF_TEXT;
        lvi.iItem = static_cast<int>(idx);
        lvi.iSubItem = 0;
        lvi.pszText = (LPWSTR)proc.displayName.c_str();
        ListView_InsertItem(hListView, &lvi);

        // 7.2) Екземпляри
        {
            std::wstring val = std::to_wstring(proc.instanceCount);
            lvi.mask = LVIF_TEXT;
            lvi.iSubItem = 1;
            lvi.pszText = (LPWSTR)val.c_str();
            ListView_SetItem(hListView, &lvi);
        }

        // 7.3) CPU (%)
        {
            std::wstringstream wss;
            wss << std::fixed << std::setprecision(1) << proc.totalCpuPercent << L" %";
            std::wstring val = wss.str();
            lvi.mask = LVIF_TEXT;
            lvi.iSubItem = 2;
            lvi.pszText = (LPWSTR)val.c_str();
            ListView_SetItem(hListView, &lvi);
        }

        // 7.4) Пам'ять (МБ)
        {
            double mb = static_cast<double>(proc.totalWorkingSetSize) / (1024.0 * 1024.0);
            std::wstringstream wss;
            wss << std::fixed << std::setprecision(1) << mb << L" МБ";
            std::wstring val = wss.str();
            lvi.mask = LVIF_TEXT;
            lvi.iSubItem = 3;
            lvi.pszText = (LPWSTR)val.c_str();
            ListView_SetItem(hListView, &lvi);
        }

        // 7.5) Диск (Гб/с)
        {
            std::wstringstream wss;
            wss << std::fixed << std::setprecision(1) << proc.lastIoGBps << L" Мбіт/с";
            std::wstring val = wss.str();
            lvi.mask = LVIF_TEXT;
            lvi.iSubItem = 4;
            lvi.pszText = (LPWSTR)val.c_str();
            ListView_SetItem(hListView, &lvi);
        }
    }

    // 8) Оновлюємо заголовки колонок динамічно:
    HWND hHeader = ListView_GetHeader(hListView);
    if (hHeader) {
        HDITEMW hdi = {};
        hdi.mask = HDI_TEXT;

        // 8.1) ЦП (%) → “YY % ЦП”
        {
            WCHAR buffer[64];
            swprintf(buffer, 64, L"%.0f %% ЦП", grandTotalCpuPct);
            hdi.pszText = buffer;
            Header_SetItem(hHeader, 2, &hdi);
        }

        // 8.2) Пам'ять (%) → “XX % Пам'ять”
        {
            WCHAR buffer[64];
            swprintf(buffer, 64, L"%.0f %% Пам'ять", grandTotalMemPct);
            hdi.pszText = buffer;
            Header_SetItem(hHeader, 3, &hdi);
        }

        // 8.3) Диск (Гб/с) → “ZZ.ZZZ Гб/с Диск”
        {
            WCHAR buffer[64];
            swprintf(buffer, 64, L"%.0f Мбіт/с Диск", grandTotalDiskMbps);
            hdi.pszText = buffer;
            Header_SetItem(hHeader, 4, &hdi);
        }
    }

    // 9) Вмикаємо перерисовку та оновлюємо ListView
    SendMessage(hListView, WM_SETREDRAW, TRUE, 0);
    RedrawWindow(hListView, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
}
