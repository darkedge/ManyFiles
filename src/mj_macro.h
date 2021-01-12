#pragma once

#define STR(x)         #x
#define XSTR(x)        STR(x)
#define WSTR(x)        L##x
#define XWSTR(x)       WSTR(x)
#define WFILE          XWSTR(__FILE__)
#define __FILENAME__   WFILE
