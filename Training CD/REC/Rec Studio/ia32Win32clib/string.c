/*
 * string.h
 *
 * Definitions for memory and string functions.
 */

#ifndef _STRING_H_
#define	_STRING_H_

#include "_ansi.h"
#include <sys/reent.h>

#define __need_size_t
#include <stddef.h>

#ifndef NULL
#define NULL 0
#endif

_BEGIN_STD_C

_PTR 	 _EXFUN(memchr,(const _PTR buffer, int ch, size_t size)) {}
int 	 _EXFUN(memcmp,(const _PTR p1, const _PTR p2, size_t size)) {}
_PTR 	 _EXFUN(memcpy,(_PTR dst, const _PTR src, size_t size)) {}
_PTR	 _EXFUN(memmove,(_PTR dst, const _PTR src, size_t size)) {}
_PTR	 _EXFUN(memset,(_PTR buffer, int ch, size_t size)) {}
char 	*_EXFUN(strcat,(char *dst, const char *src)) {}
char 	*_EXFUN(strchr,(const char *str, int ch)) {}
int	 _EXFUN(strcmp,(const char *s1, const char *s2)) {}
int	 _EXFUN(strcoll,(const char *s1, const char *s2)) {}
char 	*_EXFUN(strcpy,(char *dst, const char *src)) {}
size_t	 _EXFUN(strcspn,(const char *s1, const char *s2)) {}
char 	*_EXFUN(strerror,(int num)) {}
size_t	 _EXFUN(strlen,(const char *str)) {}
char 	*_EXFUN(strncat,(char *dst, const char *src, size_t size)) {}
int	 _EXFUN(strncmp,(const char *s1, const char *s2, size_t size)) {}
char 	*_EXFUN(strncpy,(char *dst, const char *src, size_t size)) {}
char 	*_EXFUN(strpbrk,(const char *s1, const char *s2)) {}
char 	*_EXFUN(strrchr,(const char *str, int ch)) {}
size_t	 _EXFUN(strspn,(const char *s1, const char *s2)) {}
char 	*_EXFUN(strstr,(const char *s1, const char *s2)) {}

#ifndef _REENT_ONLY
char 	*_EXFUN(strtok,(char *src, const char *tok)) {}
#endif

size_t	 _EXFUN(strxfrm,(char *s1, const char *s2, size_t size)) {}

#ifndef __STRICT_ANSI__
char 	*_EXFUN(strtok_r,(char *str, const char *tok, char **pstr)) {}

int	 _EXFUN(bcmp,(const void *p1, const void *p2, size_t size)) {}
void	 _EXFUN(bcopy,(const void *src, void *dst, size_t size)) {}
void	 _EXFUN(bzero,(void *ptr, size_t size)) {}
int	 _EXFUN(ffs,(int val)) {}
char 	*_EXFUN(index,(const char *str, int ch)) {}
_PTR	 _EXFUN(memccpy,(_PTR dst, const _PTR src, int ch, size_t size)) {}
_PTR	 _EXFUN(mempcpy,(_PTR dst, const _PTR src, size_t size)) {}
char 	*_EXFUN(rindex,(const char *str, int ch)) {}
int	 _EXFUN(strcasecmp,(const char *s1, const char *s2)) {}
char 	*_EXFUN(strdup,(const char *str)) {}
char 	*_EXFUN(_strdup_r,(struct _reent *dst, const char *src)) {}
char 	*_EXFUN(strndup,(const char *str, size_t size)) {}
char 	*_EXFUN(_strndup_r,(struct _reent *dst, const char *src, size_t size)) {}
char 	*_EXFUN(strerror_r,(int num, char *dst, size_t size)) {}
size_t	 _EXFUN(strlcat,(char *dst, const char *src, size_t size)) {}
size_t	 _EXFUN(strlcpy,(char *dst, const char *src, size_t size)) {}
int	 _EXFUN(strncasecmp,(const char *s1, const char *s2, size_t size)) {}
size_t	 _EXFUN(strnlen,(const char *str, size_t size)) {}
char 	*_EXFUN(strsep,(char **pstr, const char *str)) {}
char	*_EXFUN(strlwr,(char *str)) {}
char	*_EXFUN(strupr,(char *str)) {}
#ifdef __CYGWIN__
#ifndef DEFS_H	/* Kludge to work around problem compiling in gdb */
const char  *_EXFUN(strsignal, (int __signo));
#endif
int     _EXFUN(strtosigno, (const char *__name));
#elif defined(__linux__)
char	*_EXFUN(strsignal, (int __signo));
#endif

/* These function names are used on Windows and perhaps other systems.  */
#ifndef strcmpi
#define strcmpi strcasecmp
#endif
#ifndef stricmp
#define stricmp strcasecmp
#endif
#ifndef strncmpi
#define strncmpi strncasecmp
#endif
#ifndef strnicmp
#define strnicmp strncasecmp
#endif

#endif /* ! __STRICT_ANSI__ */

_END_STD_C

#endif /* _STRING_H_ */
