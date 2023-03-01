//
// Created by paul on 25/02/23.
//

#ifndef CRYPTUSBDRIVE_DEBUG_H
#define CRYPTUSBDRIVE_DEBUG_H

#include <stdarg.h>

#define UDP_LOGGING_ON

#ifdef UDP_LOGGING_ON
extern void __debug_log(const char *fmt, ...);
#define DEBUG_LOG(format, args...) __debug_log(format, args)
#else
#define DEBUG_LOG(...) printf(format, args)
#endif

#endif //CRYPTUSBDRIVE_DEBUG_H
