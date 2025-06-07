//DiskCleanup.h

#ifndef DISK_CLEANUP_H
#define DISK_CLEANUP_H

#include <windows.h>
#include <vector>
#include <string>

// ��������� ��� ������� "�����"
struct JunkCategoryInfo {
    std::wstring    categoryName;
    ULARGE_INTEGER  size;   // � ������
    std::wstring    path;   // ���� �� ���� ��� "RECYCLE_BIN"
};

// �������� ���� (��������� ����)
extern std::vector<JunkCategoryInfo> g_JunkCategories;
extern ULONGLONG g_TotalJunkFound;

// ������� ���������� ������� "�������� �����"
void CreateDiskCleanupControls(HWND hWndParent);

// ������ ����� ����� (���������� g_JunkCategories � ListView)
void AnalyzeDisk(HWND hwnd, HWND hListView);

// ����� ������ ������� (���� � ListView)
void CleanSelectedCategories(HWND hwnd, HWND hListView);

// ����� ��� ����� (�� �������� g_JunkCategories)
void CleanAllCategories(HWND hwnd, HWND hListView);

// (������� ������� ��� ���������� ListView �� �������, ������� ����� � AnalyzeDisk)
// ��� �������� ���������� ��� �������:
void PopulateDiskCleanupList(HWND hListView);

#endif // DISK_CLEANUP_H
