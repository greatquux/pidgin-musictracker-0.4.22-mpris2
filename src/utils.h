#ifndef _UTILS_H_
#define _UTILS_H_

#include <stdio.h>
#include <pcre.h>

#ifndef WIN32
#include <dbus/dbus-glib.h>
#else
#include <windows.h>
#endif

void trace(const char *str, ...);
int readline(FILE* file, char *buf, int len);
const char *parse_value(const char *line, const char* key);
char *put_field(char *buf, char identifier, const char *field);
void trim(char *buf);
char *build_pref(const char *format, const char* str1, const char* str2);

pcre* regex(const char* pattern, int options);
int capture(pcre* re, const char *text, int len, ...);

#ifndef WIN32
DBusGConnection *connection;
gboolean dbus_g_init_connection(void);
gboolean dbus_g_running(const char *name);
#endif

#ifdef WIN32
char *wchar_to_utf8(wchar_t *wstring);
char *GetWindowTitleUtf8(HWND hWnd);
#endif

#endif // _UTILS_H_
