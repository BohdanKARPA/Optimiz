#include "AutoMaintenance.h"
#include <CommCtrl.h>
#include "ResourceDefinitions.h"

// Функція створює групу радіокнопок та чекбоксів для налаштувань
void CreateAutoMaintenanceControls(HWND hWndParent) {
    RECT rc;
    GetClientRect(hWndParent, &rc);
    int x = 10, y = 10;
    int w = rc.right - rc.left - 20;

    // Заголовок
    CreateWindowExW(0, L"STATIC", L"Налаштування автоматичного обслуговування",
        WS_CHILD | WS_VISIBLE,
        x, y, w, 20,
        hWndParent, (HMENU)IDC_AUTO_MAINT_TITLE,
        GetModuleHandle(NULL), NULL);
    y += 30;

    // Періодичність (радіокнопки)
    CreateWindowExW(0, L"BUTTON", L"Щодня",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        x, y, 100, 20,
        hWndParent, (HMENU)IDC_AUTO_MAINT_DAILY,
        GetModuleHandle(NULL), NULL);
    CreateWindowExW(0, L"BUTTON", L"Щотижня",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        x + 110, y, 100, 20,
        hWndParent, (HMENU)IDC_AUTO_MAINT_WEEKLY,
        GetModuleHandle(NULL), NULL);
    CreateWindowExW(0, L"BUTTON", L"При старті програми",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        x + 220, y, 150, 20,
        hWndParent, (HMENU)IDC_AUTO_MAINT_ONSTART,
        GetModuleHandle(NULL), NULL);
    y += 30; // 30

    // Категорії очищення (чекбокси)
    CreateWindowExW(0, L"BUTTON", L"Тимчасові файли",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        x, y, 150, 20,
        hWndParent, (HMENU)IDC_AUTO_MAINT_CHECK_TEMP,
        GetModuleHandle(NULL), NULL);
    CreateWindowExW(0, L"BUTTON", L"Кеш браузерів",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        x + 160, y, 150, 20,
        hWndParent, (HMENU)IDC_AUTO_MAINT_CHECK_BROWSER,
        GetModuleHandle(NULL), NULL);
    CreateWindowExW(0, L"BUTTON", L"Кошик",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        x + 320, y, 100, 20,
        hWndParent, (HMENU)IDC_AUTO_MAINT_CHECK_RECYCLE,
        GetModuleHandle(NULL), NULL);
    y += 30; // 30

    // Кнопка збереження
    CreateWindowExW(0, L"BUTTON", L"Зберегти",
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        x, y, 100, 25,
        hWndParent, (HMENU)IDC_AUTO_MAINT_SAVE,
        GetModuleHandle(NULL), NULL);
}
