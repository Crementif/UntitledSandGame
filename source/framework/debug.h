#pragma once
#include "../common/common.h"

class DebugLog {
public:
    static void Printf(const char* format, ...);
    static void Draw();
    static void SetLoggingEnabled(bool enable) { s_enableLogging = enable; }
    static bool IsLoggingEnabled() { return s_enableLogging; }

private:
    static bool s_enableLogging;
    static OSTime s_lastInput;
    static std::vector<std::string> s_logQueue;
};