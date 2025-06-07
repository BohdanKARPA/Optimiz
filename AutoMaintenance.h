#ifndef AUTOMAINTENANCE_H
#define AUTOMAINTENANCE_H

#include <windows.h>
#define IDT_AUTO_MAINTENANCE 1001

// ID-��� ��������
#define IDC_AM_RADIO_STARTUP   2001
#define IDC_AM_RADIO_INTERVAL  2002
#define IDC_AM_EDIT_INTERVAL   2003
#define IDC_AM_BUTTON_OK       2004
#define IDC_AM_BUTTON_CANCEL   2005

// ��� ����� ��� ���� �����������
static const wchar_t* AUTO_MAINT_SETTINGS_CLASS = L"AutoMaintSettingsClass";

// ������� �� ���������� ������� ������������ ���������������
void CreateAutoMaintenanceControls(HWND hWndParent);

// ���� ����������
enum ScheduleType {
    SCHEDULE_ON_STARTUP,
    SCHEDULE_INTERVAL
};

struct AutoMaintenanceSettings {
    ScheduleType scheduleType;   // ���� ����� ������
    int intervalMinutes;         // �������� � ��������, ���� SCHEDULE_INTERVAL
};

// ��������� ������������ � ������ (��� �����)
bool LoadAutoMaintenanceSettings(AutoMaintenanceSettings& settings);
// ������ ������������
bool SaveAutoMaintenanceSettings(const AutoMaintenanceSettings& settings);
// ��������� ������/������ �� ���� ��������������
void SetupAutoMaintenance(HWND hWnd);
// �������, �� ������ ���� �����������
void ShowAutoMaintSettingsWindow(HWND hParent);
// �������, ��� ������� ������ ��������
void PerformAutoMaintenance();

#endif // AUTOMAINTENANCE_H
