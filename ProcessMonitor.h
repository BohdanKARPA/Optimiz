//ProcessMonitor.h

#ifndef PROCESS_MONITOR_H
#define PROCESS_MONITOR_H

#include <windows.h>
#include <vector>
#include <string>

struct AggregatedProcessInfo {  
    std::wstring displayName = L"";         // ��'� �������  
    int           instanceCount = 0;       // ������� ����������  
    ULONGLONG     totalWorkingSetSize = 0; // ������� ��� (�����)  
    double        totalCpuPercent = 0.0;   // **���� CPU (%) ��� ���������� �������**  
    ULONGLONG     totalIoRead = 0;         // ������� ��������� (�����) ��� ��� ����������  
    ULONGLONG     totalIoWrite = 0;        // ������� �������� (�����) ��� ��� ����������  
    double        lastIoGBps = 0.0;        // �������� I/O (���/�) ��� ������� (���� ��� ����������)  

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

