//Utils.h

#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <windows.h>

// Преобразовує рядок до нижнього регістру (враховуючи локаль)
std::wstring ToLowerStr(std::wstring s);

// Розгортає змінні середовища у шляху (наприклад, "%APPDATA%" -> "C:\Users\...\AppData\Roaming")
std::wstring ExpandEnvironmentStringsForPath(const std::wstring& unexpandedPath);

// Форматує кількість байтів у "X.XX KB/MB/GB/etc."
std::wstring FormatBytes(ULONGLONG bytes);

// Рекурсивно обчислює загальний розмір в директорії
ULARGE_INTEGER GetDirectorySize(const std::wstring& dirPath);

// Видаляє весь вміст теки (файли + підтеки)
void DeleteDirectoryContents(const std::wstring& dirPath);

// Прототип функції WriteLog
void WriteLog(const std::wstring& message);

// Очищає Кошик без UI (SHEmptyRecycleBin)
void EmptyRecycleBin();

// Перевіряє, чи запущено додаток з правами адміністратора
bool IsUserAdmin();

// Запитує підвищення прав через UAC, якщо не адмін
void RequestAdminRights(const wchar_t* exePath);

#endif // UTILS_H
