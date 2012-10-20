#include <wchar.h>
#define T(t) (t*)0;
#define F(t,n) {t *y = &x.n;}
#define C(n) switch(n){case n:;}
static void f()
{
#ifdef _POSIX_C_SOURCE
T(FILE)
T(locale_t)
T(va_list)
#endif
T(mbstate_t)
T(size_t)
T(wchar_t)
T(wint_t)
C(WCHAR_MAX)
C(WCHAR_MIN)
C(WEOF)
{void *x=NULL;}
wint_t(*p_getwchar)(void) = getwchar;
size_t(*p_mbrlen)(const char*restrict,size_t,mbstate_t*restrict) = mbrlen;
size_t(*p_mbrtowc)(wchar_t*restrict,const char*restrict,size_t,mbstate_t*restrict) = mbrtowc;
int(*p_mbsinit)(const mbstate_t*) = mbsinit;
size_t(*p_mbsrtowcs)(wchar_t*restrict,const char**restrict,size_t,mbstate_t*restrict) = mbsrtowcs;
wint_t(*p_putwchar)(wchar_t) = putwchar;
size_t(*p_wcrtomb)(char*restrict,wchar_t,mbstate_t*restrict) = wcrtomb;
wchar_t*(*p_wcscat)(wchar_t*restrict,const wchar_t*restrict) = wcscat;
wchar_t*(*p_wcschr)(const wchar_t*,wchar_t) = wcschr;
int(*p_wcscmp)(const wchar_t*,const wchar_t*) = wcscmp;
int(*p_wcscoll)(const wchar_t*,const wchar_t*) = wcscoll;
wchar_t*(*p_wcscpy)(wchar_t*restrict,const wchar_t*restrict) = wcscpy;
size_t(*p_wcscspn)(const wchar_t*,const wchar_t*) = wcscspn;
size_t(*p_wcsftime)(wchar_t*restrict,size_t,const wchar_t*restrict,const struct tm*restrict) = wcsftime;
size_t(*p_wcslen)(const wchar_t*) = wcslen;
wchar_t*(*p_wcsncat)(wchar_t*restrict,const wchar_t*restrict,size_t) = wcsncat;
int(*p_wcsncmp)(const wchar_t*,const wchar_t*,size_t) = wcsncmp;
wchar_t*(*p_wcsncpy)(wchar_t*restrict,const wchar_t*restrict,size_t) = wcsncpy;
wchar_t*(*p_wcspbrk)(const wchar_t*,const wchar_t*) = wcspbrk;
wchar_t*(*p_wcsrchr)(const wchar_t*,wchar_t) = wcsrchr;
size_t(*p_wcsrtombs)(char*restrict,const wchar_t**restrict,size_t,mbstate_t*restrict) = wcsrtombs;
size_t(*p_wcsspn)(const wchar_t*,const wchar_t*) = wcsspn;
wchar_t*(*p_wcsstr)(const wchar_t*restrict,const wchar_t*restrict) = wcsstr;
double(*p_wcstod)(const wchar_t*restrict,wchar_t**restrict) = wcstod;
float(*p_wcstof)(const wchar_t*restrict,wchar_t**restrict) = wcstof;
wchar_t*(*p_wcstok)(wchar_t*restrict,const wchar_t*restrict,wchar_t**restrict) = wcstok;
long(*p_wcstol)(const wchar_t*restrict,wchar_t**restrict,int) = wcstol;
long double(*p_wcstold)(const wchar_t*restrict,wchar_t**restrict) = wcstold;
long long(*p_wcstoll)(const wchar_t*restrict,wchar_t**restrict,int) = wcstoll;
unsigned long(*p_wcstoul)(const wchar_t*restrict,wchar_t**restrict,int) = wcstoul;
unsigned long long(*p_wcstoull)(const wchar_t*restrict,wchar_t**restrict,int) = wcstoull;
size_t(*p_wcsxfrm)(wchar_t*restrict,const wchar_t*restrict,size_t) = wcsxfrm;
wchar_t*(*p_wmemchr)(const wchar_t*,wchar_t,size_t) = wmemchr;
int(*p_wmemcmp)(const wchar_t*,const wchar_t*,size_t) = wmemcmp;
wchar_t*(*p_wmemcpy)(wchar_t*restrict,const wchar_t*restrict,size_t) = wmemcpy;
wchar_t*(*p_wmemmove)(wchar_t*,const wchar_t*,size_t) = wmemmove;
wchar_t*(*p_wmemset)(wchar_t*,wchar_t,size_t) = wmemset;
#ifdef _POSIX_C_SOURCE
size_t(*p_mbsnrtowcs)(wchar_t*restrict,const char**restrict,size_t,size_t,mbstate_t*restrict) = mbsnrtowcs;
FILE*(*p_open_wmemstream)(wchar_t**,size_t*) = open_wmemstream;
wchar_t*(*p_wcpcpy)(wchar_t*restrict,const wchar_t*restrict) = wcpcpy;
wchar_t*(*p_wcpncpy)(wchar_t*restrict,const wchar_t*restrict,size_t) = wcpncpy;
int(*p_wcscasecmp)(const wchar_t*,const wchar_t*) = wcscasecmp;
int(*p_wcscasecmp_l)(const wchar_t*,const wchar_t*,locale_t) = wcscasecmp_l;
int(*p_wcscoll_l)(const wchar_t*,const wchar_t*,locale_t) = wcscoll_l;
wchar_t*(*p_wcsdup)(const wchar_t*) = wcsdup;
int(*p_wcsncasecmp)(const wchar_t*,const wchar_t*,size_t) = wcsncasecmp;
int(*p_wcsncasecmp_l)(const wchar_t*,const wchar_t*,size_t,locale_t) = wcsncasecmp_l;
size_t(*p_wcsnlen)(const wchar_t*,size_t) = wcsnlen;
size_t(*p_wcsnrtombs)(char*restrict,const wchar_t**restrict,size_t,size_t,mbstate_t*restrict) = wcsnrtombs;
size_t(*p_wcsxfrm_l)(wchar_t*restrict,const wchar_t*restrict,size_t,locale_t) = wcsxfrm_l;
#endif
#ifdef _XOPEN_SOURCE
int(*p_wcswidth)(const wchar_t*,size_t) = wcswidth;
int(*p_wcwidth)(wchar_t) = wcwidth;
#endif
}
