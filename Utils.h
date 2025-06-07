//Utils.h

#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <windows.h>

// ����������� ����� �� �������� ������� (���������� ������)
std::wstring ToLowerStr(std::wstring s);

// �������� ���� ���������� � ����� (���������, "%APPDATA%" -> "C:\Users\...\AppData\Roaming")
std::wstring ExpandEnvironmentStringsForPath(const std::wstring& unexpandedPath);

// ������� ������� ����� � "X.XX KB/MB/GB/etc."
std::wstring FormatBytes(ULONGLONG bytes);

// ���������� �������� ��������� ����� � ��������
ULARGE_INTEGER GetDirectorySize(const std::wstring& dirPath);

// ������� ���� ���� ���� (����� + ������)
void DeleteDirectoryContents(const std::wstring& dirPath);

// �������� ������� WriteLog
void WriteLog(const std::wstring& message);

// ����� ����� ��� UI (SHEmptyRecycleBin)
void EmptyRecycleBin();

// ��������, �� �������� ������� � ������� ������������
bool IsUserAdmin();

// ������ ��������� ���� ����� UAC, ���� �� ����
void RequestAdminRights(const wchar_t* exePath);

#endif // UTILS_H
