//Utils.cpp

#include "Utils.h"
#include <locale>
#include <algorithm>
#include <sstream>
#include <filesystem>
#include <shlobj.h>      // ��� SHEmptyRecycleBin
#include <shellapi.h>    // ��� ShellExecuteEx
#include <fstream>

// ============================
// ������� ��������� UTILS.H
// ============================
// ��������� WriteLog: �������� � ��������� ���� optimizer_log.txt
void WriteLog(const std::wstring& message) {
    // ³�������� ���� � ����� ������
    std::wofstream logFile(L"optimizer_log.txt", std::ios::app);
    if (!logFile.is_open()) {
        return;
    }

    // �������� �������� ���
    SYSTEMTIME st;
    GetLocalTime(&st);
    wchar_t timeBuf[64];
    swprintf(timeBuf, 64,
        L"[%04d-%02d-%02d %02d:%02d:%02d] ",
        st.wYear, st.wMonth, st.wDay,
        st.wHour, st.wMinute, st.wSecond);

    // ������ ���� ���� �� �����������
    logFile << timeBuf << message << std::endl;
    logFile.close();
}


// ������������ �� ������� � ����� �� �������� �������
std::wstring ToLowerStr(std::wstring s) {
    std::locale loc;
    for (auto& c : s) {
        c = std::tolower(c, loc);
    }
    return s;
}

// ���������� ���� ���������� � ����� (���������, "%TEMP%" -> "C:\Users\...\AppData\Local\Temp")
std::wstring ExpandEnvironmentStringsForPath(const std::wstring& envPath) {
    // ������� ��� ����������� �%APPDATA%� �� �����
    DWORD size = ExpandEnvironmentStringsW(envPath.c_str(), nullptr, 0);
    if (size == 0) return L"";

    std::wstring result;
    result.resize(size);
    ExpandEnvironmentStringsW(envPath.c_str(), &result[0], size);
    // ��������� ������ ����-���������
    if (!result.empty() && result.back() == L'\0') {
        result.pop_back();
    }
    return result;
}

// ��������� �����: 0B, 1.23 KB, 4.56 MB ����
std::wstring FormatBytes(ULONGLONG bytes) {
    // ������ ������� ��� ������� �123.4 MB� ����.
    const wchar_t* suffixes[] = { L"B", L"KB", L"MB", L"GB", L"TB" };
    int idx = 0;
    double dblBytes = static_cast<double>(bytes);
    while (dblBytes >= 1024.0 && idx < 4) {
        dblBytes /= 1024.0;
        ++idx;
    }
    wchar_t buf[64];
    swprintf(buf, 64, L"%.1f %s", dblBytes, suffixes[idx]);
    return std::wstring(buf);
}

// ���������� ���������� ����� ����
ULARGE_INTEGER GetDirectorySize(const std::wstring& dirPath) {
    ULARGE_INTEGER totalSize = {};
    WIN32_FIND_DATA findData;
    std::wstring searchPath = dirPath;
    if (searchPath.empty()) return totalSize;
    if (searchPath.back() != L'\\' && searchPath.back() != L'/') {
        searchPath.push_back(L'\\');
    }
    searchPath.append(L"*");

    HANDLE hFind = FindFirstFile(searchPath.c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE) return totalSize;

    do {
        const std::wstring name = findData.cFileName;
        if (name == L"." || name == L"..") continue;

        std::wstring itemPath = dirPath;
        if (itemPath.back() != L'\\' && itemPath.back() != L'/') {
            itemPath.push_back(L'\\');
        }
        itemPath.append(name);

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            // ������� ������ ����� �������� ���������
            if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) {
                ULARGE_INTEGER subSize = GetDirectorySize(itemPath);
                totalSize.QuadPart += subSize.QuadPart;
            }
        }
        else {
            ULARGE_INTEGER fileSize;
            fileSize.LowPart = findData.nFileSizeLow;
            fileSize.HighPart = findData.nFileSizeHigh;
            totalSize.QuadPart += fileSize.QuadPart;
        }
    } while (FindNextFile(hFind, &findData) != 0);

    FindClose(hFind);
    return totalSize;
}

// ��������� ���� ���� ���� (��� ���� ����)
void DeleteDirectoryContents(const std::wstring& dirPath) {
    try {
        // 1) ������ �������� � ��� ��������� ��������
        for (auto& p : std::filesystem::recursive_directory_iterator(
            dirPath,
            std::filesystem::directory_options::skip_permission_denied | std::filesystem::directory_options::follow_directory_symlink))
        {
            const std::wstring current = p.path().wstring();
            ::SetFileAttributesW(current.c_str(), FILE_ATTRIBUTE_NORMAL);
        }

        // 2) ��������� �� ������� ��������
        for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
            std::error_code ec;
            const auto& child = entry.path();
            if (std::filesystem::is_regular_file(child, ec)) {
                std::filesystem::remove(child, ec);
            }
            else if (std::filesystem::is_directory(child, ec)) {
                std::filesystem::remove_all(child, ec);
            }
        }

        // 3) ��������� ��������� �������� *.tmp ����� (���� �)
        std::wstring pattern = dirPath + L"\\*.tmp";
        WIN32_FIND_DATA fd;
        HANDLE hFind = FindFirstFile(pattern.c_str(), &fd);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                std::wstring tmpFilePath = dirPath + L"\\" + fd.cFileName;
                SetFileAttributesW(tmpFilePath.c_str(), FILE_ATTRIBUTE_NORMAL);
                DeleteFileW(tmpFilePath.c_str());
            } while (FindNextFile(hFind, &fd) != 0);
            FindClose(hFind);
        }
    }
    catch (...) {
        // �������� ������ �������
    }
}

// ������� ����� (��� ������������, ��� UI)
void EmptyRecycleBin() {
    SHEmptyRecycleBin(NULL, NULL, SHERB_NOCONFIRMATION | SHERB_NOPROGRESSUI | SHERB_NOSOUND);
}

// ����������, �� ���������� ����
bool IsUserAdmin() {
    BOOL bIsAdmin = FALSE;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID AdministratorsGroup = NULL;
    if (AllocateAndInitializeSid(
        &NtAuthority,
        2,
        SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS,
        0, 0, 0, 0, 0, 0,
        &AdministratorsGroup))
    {
        CheckTokenMembership(NULL, AdministratorsGroup, &bIsAdmin);
        FreeSid(AdministratorsGroup);
    }
    return (bIsAdmin == TRUE);
}

// �������� UAC, ���� �� ���� ���� �����
void RequestAdminRights(const wchar_t* exePath) {
    SHELLEXECUTEINFO sei = { sizeof(sei) };
    sei.lpVerb = L"runas";
    sei.lpFile = exePath;
    sei.hwnd = NULL;
    sei.nShow = SW_SHOWNORMAL;

    if (!ShellExecuteEx(&sei)) {
        DWORD error = GetLastError();
        if (error == ERROR_CANCELLED) {
            MessageBox(NULL,
                L"��� �������� ������ ������ ����� ������������. �������� ���� �������.",
                L"������� ���� �������",
                MB_OK | MB_ICONERROR);
        }
        else {
            wchar_t buf[256];
            wsprintf(buf, L"�� ������� ��������� ����� ������������. ��� �������: %lu", error);
            MessageBox(NULL, buf, L"������� ShellExecuteEx", MB_OK | MB_ICONERROR);
        }
        PostQuitMessage(1);
    }
    else {
        PostQuitMessage(0);
    }
}



