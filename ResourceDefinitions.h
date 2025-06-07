//ResourceDefinitions.h

#ifndef RESOURCE_DEFINITIONS_H
#define RESOURCE_DEFINITIONS_H

// Ідентифікатори команд у меню:
#define IDM_FILE_EXIT                      101
#define IDM_MONITORING                     201
#define IDM_DISK_CLEANUP                   202

#define IDM_SETTINGS_AUTO_MAINTENANCE      301
 
#define IDM_HELP                           401

// Ідентифікатори контролів:
#define IDC_PROCESSLISTVIEW               1001

#define IDC_DISKCLEANUP_LISTVIEW          2001
#define IDC_DISKCLEANUP_ANALYZE_BUTTON    2002
#define IDC_DISKCLEANUP_CLEAN_BUTTON      2003
#define IDC_DISKCLEANUP_TOTALJUNK_LABEL   2004
#define IDC_DISKCLEANUP_TOTALJUNK_SIZE    2005
#define IDC_DISKCLEANUP_CLEAN_ALL         2006
#define IDC_DISKCLEANUP_DISKTOTAL_LABEL   2007
#define IDC_DISKCLEANUP_DISKTOTAL_SIZE    2008
#define IDC_DISKCLEANUP_DISKUSED_LABEL    2009
#define IDC_DISKCLEANUP_DISKUSED_SIZE     2010
#define IDC_DISKCLEANUP_DISKFREE_LABEL    2011
#define IDC_DISKCLEANUP_DISKFREE_SIZE     2012

 // Ідентифікатори контролів для вкладки «Автоматичне обслуговування»
#define IDC_AUTO_MAINT_TITLE        3001
#define IDC_AUTO_MAINT_DAILY        3002
#define IDC_AUTO_MAINT_WEEKLY       3003
#define IDC_AUTO_MAINT_ONSTART      3004
#define IDC_AUTO_MAINT_CHECK_TEMP   3005
#define IDC_AUTO_MAINT_CHECK_BROWSER 3006
#define IDC_AUTO_MAINT_CHECK_RECYCLE 3007
#define IDC_AUTO_MAINT_SAVE         3008
#define IDC_AUTO_MAINT_EDIT_INTERVAL 3009

#define IDM_SETTINGS_AUTOMAINT            3010
// Ідентифікатор таймера для оновлення моніторингу
#define IDT_MONITOR_REFRESH               5001
#define IDT_AUTOMAINT_TIMER               5002

#

#endif // RESOURCE_DEFINITIONS_H
