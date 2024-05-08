#ifndef __ALINIX_KERNEL_LOG_H_UACPI_INCLUDED
#define __ALINIX_KERNEL_LOG_H_UACPI_INCLUDED

#include <alinix/ucapi/context.h>
#include <alinix/ucapi/types.h>

#define uacpi_log_lvl(lvl, ...) \
    do { if (uacpi_rt_should_log(lvl)) uacpi_kernel_log(lvl, __VA_ARGS__); } while (0)

#define uacpi_debug(...) uacpi_log_lvl(UACPI_LOG_DEBUG, __VA_ARGS__)
#define uacpi_trace(...) uacpi_log_lvl(UACPI_LOG_TRACE, __VA_ARGS__)
#define uacpi_info(...)  uacpi_log_lvl(UACPI_LOG_INFO, __VA_ARGS__)
#define uacpi_warn(...)  uacpi_log_lvl(UACPI_LOG_WARN, __VA_ARGS__)
#define uacpi_error(...) uacpi_log_lvl(UACPI_LOG_ERROR, __VA_ARGS__)

#endif