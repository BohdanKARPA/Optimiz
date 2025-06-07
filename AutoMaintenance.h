#ifndef AUTOMAINTENANCE_H
#define AUTOMAINTENANCE_H

#include <windows.h>
#define IDT_AUTO_MAINTENANCE 1001

// ID-шки контролів
#define IDC_AM_RADIO_STARTUP   2001
#define IDC_AM_RADIO_INTERVAL  2002
#define IDC_AM_EDIT_INTERVAL   2003
#define IDC_AM_BUTTON_OK       2004
#define IDC_AM_BUTTON_CANCEL   2005

// Ім’я класу для вікна налаштувань
static const wchar_t* AUTO_MAINT_SETTINGS_CLASS = L"AutoMaintSettingsClass";

// Створює всі контролери вкладки «Автоматичне обслуговування»
void CreateAutoMaintenanceControls(HWND hWndParent);

// Типи планування
enum ScheduleType {
    SCHEDULE_ON_STARTUP,
    SCHEDULE_INTERVAL
};

struct AutoMaintenanceSettings {
    ScheduleType scheduleType;   // який режим обрано
    int intervalMinutes;         // інтервал у хвилинах, якщо SCHEDULE_INTERVAL
};

// Завантажує налаштування з реєстру (або файлу)
bool LoadAutoMaintenanceSettings(AutoMaintenanceSettings& settings);
// Зберігає налаштування
bool SaveAutoMaintenanceSettings(const AutoMaintenanceSettings& settings);
// Налаштовує таймер/запуск за цими налаштуваннями
void SetupAutoMaintenance(HWND hWnd);
// Функція, що показує вікно налаштувань
void ShowAutoMaintSettingsWindow(HWND hParent);
// Функція, яка реально виконує очищення
void PerformAutoMaintenance();

#endif // AUTOMAINTENANCE_H
