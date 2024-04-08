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

struct HashedString {
    unsigned long long hash;
    const char* orig;

    consteval HashedString(const char* str): hash(consteval_CalcHashString(str)), orig(str) {}

    operator unsigned long long() const { return hash; }
    operator const char*() const { return orig; }
};

struct TimedSegment {
    unsigned long long hash;
    const char* orig;
    OSTick duration;

    TimedSegment(const HashedString& segmentName, OSTick initDuration): hash(segmentName.hash), orig(segmentName.orig), duration(initDuration) {}
};

class DebugProfile {
public:
    static void Start(const HashedString&& segmentName) {
        startTimes.emplace_back(segmentName.hash, OSGetTick());
    }

    static void End(const HashedString&& segmentName) {
        // iterate in reverse to allow the same segment to be more properly timed when nested
        auto startedTime = std::find_if(startTimes.rbegin(), startTimes.rend(), [&segmentName](const auto& pair) { return pair.first == segmentName.hash; });

        // create a TimedSegment object to store the duration or append to an existing one
        for (auto& [_, orig, duration] : durations) {
            if (orig == segmentName.orig) {
                duration = OSGetTick() - startedTime->second;
                startTimes.erase(startedTime.base());
                return;
            }
        }

        // didn't find an existing segment, so create a new one
        durations.emplace_back(segmentName, OSGetTick() - startedTime->second);
        startTimes.erase(startedTime.base());
    }

    static void Print() {
        for (auto& [_, orig, duration] : durations) {
            DebugLog::Printf("%s time: %.3f ms", orig, (double)OSTicksToMilliseconds(duration)/1000.0);
        }
        durations.clear();
    }

private:
    static std::vector<std::pair<unsigned long long, OSTick>> startTimes;
    static std::vector<TimedSegment> durations;
};