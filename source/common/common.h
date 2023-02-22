#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstdarg>

#include <array>
#include <unordered_map>
#include <string>
#include <algorithm>

#include <coreinit/memdefaultheap.h>
#include <coreinit/memfrmheap.h>
#include <coreinit/memexpheap.h>
#include <coreinit/filesystem.h>
#include <coreinit/debug.h>

#include <gx2/clear.h>
#include <gx2/draw.h>
#include <gx2/mem.h>
#include <gx2/registers.h>
#include <gx2/utils.h>
#include <gx2/event.h>

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

#include <romfs-wiiu.h>

#include "types.h"

[[noreturn]]
void CriticalErrorHandler(const char* msg, ...);