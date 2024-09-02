#include "debug.h"
#include "navigation.h"
#include "render.h"

bool DebugLog::s_enableLogging = false;
std::vector<std::string> DebugLog::s_logQueue = {};
OSTime DebugLog::s_lastInput = 0;

void DebugLog::Printf(const char* format, ...) {
    if (!s_enableLogging)
        return;

    va_list args;
    va_start(args, format);
    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    s_logQueue.emplace_back(buffer);
}

void DebugLog::Draw() {
    const bool activeInputDebounce = (s_lastInput + (OSTime)OSMillisecondsToTicks(600)) >= OSGetTime();
    if (pressedStart() && !activeInputDebounce) {
        s_lastInput = OSGetTime();
        s_enableLogging = !s_enableLogging;
    }

    for (u32 i = 0; i < s_logQueue.size(); i++) {
        Render::RenderText(20, 300 + (i * 16), 0x00, s_logQueue[i].c_str());
    }
    s_logQueue.clear();
}

std::vector<std::pair<unsigned long long, OSTick>> DebugProfile::startTimes = {};
std::vector<TimedSegment> DebugProfile::durations = {};


void DebugOverlay::AddPoint(s32 x, s32 y) {
    if (s_pointSprite == nullptr) {
        s_pointSprite = new Sprite("/tex/test_sprite_1px.tga", true);
    }
    s_points.emplace_back(x, y);
}

void DebugOverlay::Draw() {
    s_pointSprite->SetupSampler(false);
    for (const Vector2i& point : s_points) {

        Render::RenderSprite(s_pointSprite, point.x, point.y);
    }
    s_points.clear();
}

Sprite* DebugOverlay::s_pointSprite = nullptr;
std::vector<Vector2i> DebugOverlay::s_points = {};