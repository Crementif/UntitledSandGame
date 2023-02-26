#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstdarg>

#include <array>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <locale>
#include <codecvt>
#include <algorithm>
#include <memory>
#include <chrono>

#include <coreinit/memdefaultheap.h>
#include <coreinit/memfrmheap.h>
#include <coreinit/memexpheap.h>
#include <coreinit/filesystem.h>
#include <coreinit/debug.h>
#include <coreinit/dynload.h>
#include <coreinit/time.h>

#include <gx2/clear.h>
#include <gx2/draw.h>
#include <gx2/mem.h>
#include <gx2/registers.h>
#include <gx2/utils.h>
#include <gx2/event.h>
#include <gx2/shaders.h>

#include <whb/file.h>
#include <whb/gfx.h>
#include <whb/sdcard.h>
#include <whb/log.h>
#include <whb/log_cafe.h>
#include <whb/log_udp.h>

#include <sndcore2/core.h>
#include <sndcore2/voice.h>
#include <sndcore2/drcvs.h>
#include <coreinit/core.h>
#include <coreinit/cache.h>
#include <coreinit/thread.h>
#include <coreinit/time.h>

#include <coreinit/filesystem.h>
#include <coreinit/memdefaultheap.h>
#include <nn/swkbd.h>

#include <romfs-wiiu.h>

#include "types.h"

[[noreturn]]
void CriticalErrorHandler(const char* msg, ...);

inline std::vector<u8> LoadFileToMem(const std::string path)
{
    std::ifstream fs((std::string("romfs:/") + path).c_str(), std::ios::in | std::ios::binary);
    if(!fs.is_open())
        CriticalErrorHandler("Failed to open file %s\n", path.c_str());
    std::vector<u8> data((std::istreambuf_iterator<char>(fs)), std::istreambuf_iterator<char>());
    return data;
}

inline double GetMillisecondTimestamp()
{
    double microSec = (double)(OSTicksToMicroseconds(OSGetTick()));
    return microSec / 1000.0;
}

inline f32 _InterpolateAngle(f32 current, f32 dest, f32 factor)
{
    f32 dist = fabs(dest - current);
    if(dist > M_PI)
    {
        if(current < dest)
            current += M_TWOPI;
        else if(current > dest)
            current -= M_TWOPI;
    }
    return current * (1.0f - factor) + dest * factor;
}

inline f32 _MoveAngleTowardsTarget(f32 current, f32 dest, f32 rate)
{
    f32 dist = fabs(dest - current);
    if(dist > M_PI)
    {
        if(current < dest)
            current += M_TWOPI;
        else if(current > dest)
            current -= M_TWOPI;
    }
    if(current < dest)
    {
        current += rate;
        if(current > dest)
            current = dest;
    }
    else if(current > dest)
    {
        current -= rate;
        if(current < dest)
            current = dest;
    }
    return current;
}