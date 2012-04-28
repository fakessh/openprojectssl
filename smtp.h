#ifndef _SMTP_H
#define _SMTP_H

#if defined(SUNOS_5)
#define OS_SUNOS5
typedef short u_int16_t;
typedef int u_int32_t;
typedef char u_int8_t;
#elif defined(__GNUC__) && (defined(__APPLE_CPP__) || defined(__APPLE_CC__) || defined(__MACOS_CLASSIC__))
#define OS_MACOSX
#elif defined(__GUNC__) && defined(__linux__)
#define OS_LINUX && OS_UBUNTU
#include <linux/types.h>
#else
#error "Unsupported operating system"
#endif 

