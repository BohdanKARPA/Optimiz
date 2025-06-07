//DiskCleanup.h

#ifndef DISK_CLEANUP_H
#define DISK_CLEANUP_H

#include <windows.h>
#include <vector>
#include <string>

// Структура для категорії "сміття"
struct JunkCategoryInfo {
    std::wstring    categoryName;
    ULARGE_INTEGER  size;   // у байтах
    std::wstring    path;   // шлях до теки або "RECYCLE_BIN"
};

// Глобальні змінні (перенесені сюди)
extern std::vector<JunkCategoryInfo> g_JunkCategories;
extern ULONGLONG g_TotalJunkFound;

// Створює контролери вкладки "Очищення диска"
void CreateDiskCleanupControls(HWND hWndParent);

// Виконує аналіз диска (заповнення g_JunkCategories і ListView)
void AnalyzeDisk(HWND hwnd, HWND hListView);

// Очищає вибрані категорії (теми з ListView)
void CleanSelectedCategories(HWND hwnd, HWND hListView);

// Очищає все сміття (всі елементи g_JunkCategories)
void CleanAllCategories(HWND hwnd, HWND hListView);

// (Функція напряму для наповнення ListView не потрібна, оскільки логіка в AnalyzeDisk)
// Але залишимо декларацію для повноти:
void PopulateDiskCleanupList(HWND hListView);

#endif // DISK_CLEANUP_H
