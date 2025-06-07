//ProcessMonitor.h

#ifndef PROCESS_MONITOR_H
#define PROCESS_MONITOR_H

#include <windows.h>
#include <vector>
#include <string>

struct AggregatedProcessInfo {
    std::wstring displayName;         // ��'� �������
    int           instanceCount;      // ������� ����������
    ULONGLONG     totalWorkingSetSize; // ������� ��� (�����)
    double        totalCpuPercent;    // **���� CPU (%) ��� ���������� �������**
    ULONGLONG     totalIoRead;        // ������� ��������� (�����) ��� ��� ����������
    ULONGLONG     totalIoWrite;       // ������� �������� (�����) ��� ��� ����������
    double        lastIoGBps;       // �������� I/O (���/�) ��� ������� (���� ��� ����������)
};

// �������� ���� (��������� ���� � Main.cpp)
extern std::vector<AggregatedProcessInfo> g_AggregatedProcessList;
extern int  g_ProcessSortColumn; // �� ���� �������� �������
extern bool g_ProcessSortAsc;    // �������� ����������

// ������� ���������� ListView ��� ���������� �������
void CreateProcessMonitorControls(HWND hWndParent);

// �������� ListView ������ ��� �������
void PopulateProcessList(HWND hListView);

// �������-��������� ��� ���������� (CompareByColumn)
bool CompareByColumn(const AggregatedProcessInfo& a, const AggregatedProcessInfo& b);

#endif // PROCESS_MONITOR_H

