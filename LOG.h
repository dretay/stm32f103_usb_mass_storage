#pragma once

//simple debug logging
#include "SEGGER_RTT.h"

#if DEBUG == 1
#define LOG(...) SEGGER_RTT_printf(0, __VA_ARGS__)
#define _ERROR(fmt, args...) LOG("\n%s%s[ERROR] %s%s:%s:%d: "fmt, RTT_CTRL_RESET, RTT_CTRL_BG_BRIGHT_RED, RTT_CTRL_RESET, __FILE__, __FUNCTION__, __LINE__, args)
#define _WARN(fmt, args...) LOG("\n%s%s[WARN] %s%s:%s:%d: "fmt, RTT_CTRL_RESET, RTT_CTRL_BG_BRIGHT_YELLOW, RTT_CTRL_RESET, __FILE__, __FUNCTION__, __LINE__, args)
#define _INFO(fmt, args...) LOG("\n%s[INFO] %s:%s:%d: "fmt, RTT_CTRL_RESET, __FILE__, __FUNCTION__, __LINE__, args)
#define _DEBUG(fmt, args...) LOG("\n%s[DEBUG] %s:%s:%d: "fmt, RTT_CTRL_RESET, __FILE__, __FUNCTION__, __LINE__, args)
#else
#define LOG(...)
#endif
