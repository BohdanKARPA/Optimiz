//ProcessMonitor.h

#ifndef PROCESS_MONITOR_H
#define PROCESS_MONITOR_H

#include <windows.h>
#include <vector>
#include <string>

struct AggregatedProcessInfo {  
    std::wstring displayName = L"";         // ім'я процесу  
    int           instanceCount = 0;       // кількість екземплярів  
    ULONGLONG     totalWorkingSetSize = 0; // сумарна ОЗП (байти)  
    double        totalCpuPercent = 0.0;   // **сума CPU (%) усіх екземплярів процесу**  
    ULONGLONG     totalIoRead = 0;         // сумарно прочитано (байти) для усіх екземплярів  
    ULONGLONG     totalIoWrite = 0;        // сумарно записано (байти) для усіх екземплярів  
    double        lastIoGBps = 0.0;        // швидкість I/O (Мбіт/с) для процесу (сума усіх екземплярів)  

};

// Глобальні змінні (перенесені сюди з Main.cpp)
extern std::vector<AggregatedProcessInfo> g_AggregatedProcessList;
extern int  g_ProcessSortColumn; // за якою колонкою сортуємо
extern bool g_ProcessSortAsc;    // напрямок сортування

// Створює контролери ListView для моніторингу процесів
void CreateProcessMonitorControls(HWND hWndParent);

// Заповнює ListView даними про процеси
void PopulateProcessList(HWND hListView);

// Функція-порівняння для сортування (CompareByColumn)
bool CompareByColumn(const AggregatedProcessInfo& a, const AggregatedProcessInfo& b);

#endif // PROCESS_MONITOR_H

