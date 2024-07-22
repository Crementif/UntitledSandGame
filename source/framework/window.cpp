// Windowing library built on GX2 with basic operations inspired by glfw

#include "window.h"
#include "debug.h"

#include <coreinit/foreground.h>
#include <coreinit/memdefaultheap.h>
#include <coreinit/memfrmheap.h>
#include <coreinit/memheap.h>
#include <gx2/context.h>
#include <gx2/display.h>
#include <gx2/event.h>
#include <gx2/mem.h>
#include <gx2/registers.h>
#include <gx2/state.h>
#include <gx2/swap.h>
#include <proc_ui/procui.h>
#include <whb/sdcard.h>

static void*            gCmdlist              = NULL;
static GX2ContextState* gContext              = NULL;
static void*            gTvScanBuffer         = NULL;
static void*            gDrcScanBuffer        = NULL;
static GX2ColorBuffer   gColorBuffer;
static void*            gColorBufferImageData = NULL;
static GX2Texture       gColorBufferTexture;
static GX2ColorBuffer   gPostBuffer;
static void*            gPostBufferImageData = NULL;
static GX2Texture       gPostBufferTexture;
static GX2DepthBuffer   gDepthBuffer;
static void*            gDepthBufferImageData = NULL;
static GX2Texture       gDepthBufferTexture;
static MEMHeapHandle    gMEM1HeapHandle       = NULL;
static MEMHeapHandle    gFgHeapHandle         = NULL;

static bool gInitialized = false;
static bool gIsRunning = false;

static u32 gWindowWidth = 0;
static u32 gWindowHeight = 0;


void _GX2InitTextureFromColorBuffer(GX2Texture* texture, GX2ColorBuffer* colorBuffer)
{
    memset(texture, 0, sizeof(GX2Texture));
    texture->surface = colorBuffer->surface;
    texture->viewFirstSlice = 0;
    texture->viewNumSlices = 1;
    texture->viewFirstMip = 0;
    texture->viewNumMips = 1;
    texture->compMap = 0x0010203;
    GX2InitTextureRegs(texture);
}

void _GX2InitTextureFromDepthBuffer(GX2Texture* texture, GX2DepthBuffer* depthBuffer)
{
    memset(texture, 0, sizeof(GX2Texture));
    texture->surface = depthBuffer->surface;
    texture->viewFirstSlice = 0;
    texture->viewNumSlices = 1;
    texture->viewFirstMip = 0;
    texture->viewNumMips = 1;
    texture->compMap = 0x0010203;
    GX2InitTextureRegs(texture);
}

static bool WindowForegroundAcquire()
{
    // Get the MEM1 heap and Foreground Bucket heap handles
    gMEM1HeapHandle = MEMGetBaseHeapHandle(MEM_BASE_HEAP_MEM1);
    gFgHeapHandle = MEMGetBaseHeapHandle(MEM_BASE_HEAP_FG);

    if (!(gMEM1HeapHandle && gFgHeapHandle))
        return false;

    // Allocate TV scan buffer
    {
        GX2TVRenderMode tv_render_mode;

        // Get current TV scan mode
        GX2TVScanMode tv_scan_mode = GX2GetSystemTVScanMode();

        // Determine TV framebuffer dimensions (scan buffer, specifically)
        if (tv_scan_mode != GX2_TV_SCAN_MODE_576I && tv_scan_mode != GX2_TV_SCAN_MODE_480I
            && gWindowWidth >= 1920 && gWindowHeight >= 1080)
        {
            gWindowWidth = 1920;
            gWindowHeight = 1080;
            tv_render_mode = GX2_TV_RENDER_MODE_WIDE_1080P;
        }
        else if (gWindowWidth >= 1280 && gWindowHeight >= 720)
        {
            gWindowWidth = 1280;
            gWindowHeight = 720;
            tv_render_mode = GX2_TV_RENDER_MODE_WIDE_720P;
        }
        else if (gWindowWidth >= 850 && gWindowHeight >= 480)
        {
            gWindowWidth = 854;
            gWindowHeight = 480;
            tv_render_mode = GX2_TV_RENDER_MODE_WIDE_480P;
        }
        else // if (gWindowWidth >= 640 && gWindowHeight >= 480)
        {
            gWindowWidth = 640;
            gWindowHeight = 480;
            tv_render_mode = GX2_TV_RENDER_MODE_STANDARD_480P;
        }

        // Calculate TV scan buffer byte size
        u32 tv_scan_buffer_size, unk;
        GX2CalcTVSize(
            tv_render_mode,                       // Render Mode
            GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8, // Scan Buffer Surface Format
            GX2_BUFFERING_MODE_DOUBLE,            // Two buffers for double-buffering
            &tv_scan_buffer_size,                 // Output byte size
            &unk                                  // Unknown; seems like we have no use for it
        );

        // Allocate TV scan buffer
        gTvScanBuffer = MEMAllocFromFrmHeapEx(
            gFgHeapHandle,
            tv_scan_buffer_size,
            GX2_SCAN_BUFFER_ALIGNMENT // Required alignment
        );

        if (!gTvScanBuffer)
            return false;

        // Flush allocated buffer from CPU cache
        GX2Invalidate(GX2_INVALIDATE_MODE_CPU, gTvScanBuffer, tv_scan_buffer_size);

        // Set the current TV scan buffer
        GX2SetTVBuffer(
            gTvScanBuffer,                        // Scan Buffer
            tv_scan_buffer_size,                  // Scan Buffer Size
            tv_render_mode,                       // Render Mode
            GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8, // Scan Buffer Surface Format
            GX2_BUFFERING_MODE_DOUBLE             // Enable double-buffering
        );

        // Set the current TV scan buffer dimensions
        GX2SetTVScale(gWindowWidth, gWindowHeight);
    }

    // Allocate DRC (Gamepad) scan buffer
    {
        u32 drc_width = 854;
        u32 drc_height = 480;

        // Calculate DRC scan buffer byte size
        u32 drc_scan_buffer_size, unk;
        GX2CalcDRCSize(
            GX2_DRC_RENDER_MODE_SINGLE,           // Render Mode
            GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8, // Scan Buffer Surface Format
            GX2_BUFFERING_MODE_DOUBLE,            // Two buffers for double-buffering
            &drc_scan_buffer_size,                // Output byte size
            &unk                                  // Unknown; seems like we have no use for it
        );

        // Allocate DRC scan buffer
        gDrcScanBuffer = MEMAllocFromFrmHeapEx(
            gFgHeapHandle,
            drc_scan_buffer_size,
            GX2_SCAN_BUFFER_ALIGNMENT // Required alignment
        );

        if (!gDrcScanBuffer)
            return false;

        // Flush allocated buffer from CPU cache
        GX2Invalidate(GX2_INVALIDATE_MODE_CPU, gDrcScanBuffer, drc_scan_buffer_size);

        // Set the current DRC scan buffer
        GX2SetDRCBuffer(
            gDrcScanBuffer,                       // Scan Buffer
            drc_scan_buffer_size,                 // Scan Buffer Size
            GX2_DRC_RENDER_MODE_SINGLE,           // Render Mode
            GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8, // Scan Buffer Surface Format
            GX2_BUFFERING_MODE_DOUBLE             // Enable double-buffering
        );

        // Set the current DRC scan buffer dimensions
        GX2SetDRCScale(drc_width, drc_height);
    }

    auto initializeColorBuffer = [](GX2ColorBuffer& buffer, GX2Texture& bufferTexture, void*& bufferImageData, u32 bufferWidth, u32 bufferHeight) -> bool {
        // initialize buffer
        buffer.surface.dim = GX2_SURFACE_DIM_TEXTURE_2D;
        buffer.surface.width = bufferWidth;
        buffer.surface.height = bufferHeight;
        buffer.surface.depth = 1;
        buffer.surface.mipLevels = 1;
        buffer.surface.format = GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8;
        buffer.surface.aa = GX2_AA_MODE1X;
        buffer.surface.use = GX2_SURFACE_USE_TEXTURE_COLOR_BUFFER_TV;
        buffer.surface.mipmaps = NULL;
        buffer.surface.tileMode = GX2_TILE_MODE_TILED_1D_THIN1;
        buffer.surface.swizzle  = 0;
        buffer.viewMip = 0;
        buffer.viewFirstSlice = 0;
        buffer.viewNumSlices = 1;
        GX2CalcSurfaceSizeAndAlignment(&buffer.surface);
        GX2InitColorBufferRegs(&buffer);

        // initialize buffer data
        bufferImageData = MEMAllocFromFrmHeapEx(
            gMEM1HeapHandle,
            buffer.surface.imageSize, // Data byte size
            (int)buffer.surface.alignment  // Required alignment
        );
        if (!bufferImageData)
            return false;
        buffer.surface.image = bufferImageData;

        // initialize buffer texture
        _GX2InitTextureFromColorBuffer(&bufferTexture, &buffer);
        bufferTexture.surface.use = (GX2SurfaceUse)(bufferTexture.surface.use | GX2_SURFACE_USE_TEXTURE_COLOR_BUFFER_TV);
        memset(bufferTexture.surface.image, 0, bufferTexture.surface.imageSize);
        GX2Invalidate(GX2_INVALIDATE_MODE_CPU_TEXTURE, bufferTexture.surface.image, bufferTexture.surface.imageSize);

        GX2Invalidate(GX2_INVALIDATE_MODE_CPU, bufferImageData, buffer.surface.imageSize);
        return true;
    };

    if (!initializeColorBuffer(gColorBuffer, gColorBufferTexture, gColorBufferImageData, gWindowWidth, gWindowHeight))
        return false;

    if (!initializeColorBuffer(gPostBuffer, gPostBufferTexture, gPostBufferImageData, gWindowWidth, gWindowHeight))
        return false;

    // Initialize depth buffer
    gDepthBuffer.surface.dim = GX2_SURFACE_DIM_TEXTURE_2D;
    gDepthBuffer.surface.width = gWindowWidth;
    gDepthBuffer.surface.height = gWindowHeight;
    gDepthBuffer.surface.depth = 1;
    gDepthBuffer.surface.mipLevels = 1;
    gDepthBuffer.surface.format = GX2_SURFACE_FORMAT_FLOAT_R32;
    gDepthBuffer.surface.aa = GX2_AA_MODE1X;
    gDepthBuffer.surface.use = GX2_SURFACE_USE_TEXTURE | GX2_SURFACE_USE_DEPTH_BUFFER;
    gDepthBuffer.surface.mipmaps = NULL;
    gDepthBuffer.surface.tileMode = GX2_TILE_MODE_DEFAULT;
    gDepthBuffer.surface.swizzle  = 0;
    gDepthBuffer.viewMip = 0;
    gDepthBuffer.viewFirstSlice = 0;
    gDepthBuffer.viewNumSlices = 1;
    gDepthBuffer.hiZPtr = NULL;
    gDepthBuffer.hiZSize = 0;
    gDepthBuffer.depthClear = 1.0f;
    gDepthBuffer.stencilClear = 0;
    GX2CalcSurfaceSizeAndAlignment(&gDepthBuffer.surface);
    GX2InitDepthBufferRegs(&gDepthBuffer);

    // Allocate depth buffer data
    gDepthBufferImageData = MEMAllocFromFrmHeapEx(
        gMEM1HeapHandle,
        gDepthBuffer.surface.imageSize, // Data byte size
        gDepthBuffer.surface.alignment  // Required alignment
    );

    if (!gDepthBufferImageData)
        return false;

    gDepthBuffer.surface.image = gDepthBufferImageData;

    // Initialize depth buffer texture
    _GX2InitTextureFromDepthBuffer(&gDepthBufferTexture, &gDepthBuffer);
    memset(gDepthBufferTexture.surface.image, 0, gDepthBufferTexture.surface.imageSize);
    GX2Invalidate(GX2_INVALIDATE_MODE_CPU_TEXTURE, gDepthBufferTexture.surface.image, gDepthBufferTexture.surface.imageSize);

    // Flush allocated buffer from CPU cache
    GX2Invalidate(GX2_INVALIDATE_MODE_CPU, gDepthBufferImageData, gDepthBuffer.surface.imageSize);

    // Enable TV and DRC
    GX2SetTVEnable(true);
    GX2SetDRCEnable(true);

    // If not first time in foreground, restore the GX2 context state
    if (gIsRunning)
    {
        GX2SetContextState(gContext);
        GX2SetColorBuffer(&gColorBuffer, GX2_RENDER_TARGET_0);
        GX2SetDepthBuffer(&gDepthBuffer);
    }

    // Initialize GQR2 to GQR5
    asm volatile ("mtspr %0, %1" : : "i" (898), "r" (0x00040004));
    asm volatile ("mtspr %0, %1" : : "i" (899), "r" (0x00050005));
    asm volatile ("mtspr %0, %1" : : "i" (900), "r" (0x00060006));
    asm volatile ("mtspr %0, %1" : : "i" (901), "r" (0x00070007));

    return true;
}

static void WindowForegroundRelease()
{
    if (gIsRunning)
        ProcUIDrawDoneRelease();

    if (gFgHeapHandle)
    {
        MEMFreeToFrmHeap(gFgHeapHandle, MEM_FRM_HEAP_FREE_ALL);
        gFgHeapHandle = NULL;
    }
    gTvScanBuffer = NULL;
    gDrcScanBuffer = NULL;

    if (gMEM1HeapHandle)
    {
        MEMFreeToFrmHeap(gMEM1HeapHandle, MEM_FRM_HEAP_FREE_ALL);
        gMEM1HeapHandle = NULL;
    }
    gColorBufferImageData = NULL;
    gPostBufferImageData = NULL;
    gDepthBufferImageData = NULL;

    WHBUnmountSdCard();
}

void WindowInit(u32 width, u32 height, u32* pWidth, u32* pHeight)
{
    gWindowWidth = width;
    gWindowHeight = height;

    // Allocate GX2 command buffer
    gCmdlist = MEMAllocFromDefaultHeapEx(
        0x400000,                    // A very commonly used size in Nintendo games
        GX2_COMMAND_BUFFER_ALIGNMENT // Required alignment
    );

    if (!gCmdlist)
        WindowExit();

    // Several parameters to initialize GX2 with
    u32 initAttribs[] = {
        GX2_INIT_CMD_BUF_BASE, (uintptr_t)gCmdlist, // Command Buffer Base Address
        GX2_INIT_CMD_BUF_POOL_SIZE, 0x400000,       // Command Buffer Size
        GX2_INIT_ARGC, 0,                           // main() arguments count
        GX2_INIT_ARGV, (uintptr_t)NULL,             // main() arguments vector
        GX2_INIT_END                                // Ending delimiter
    };

    // Initialize GX2
    GX2Init(initAttribs);

    // Create TV and DRC scan, color and depth buffers
    if (!WindowForegroundAcquire())
        WindowExit();

    // Allocate context state instance
    gContext = (GX2ContextState*)MEMAllocFromDefaultHeapEx(
        sizeof(GX2ContextState),    // Size of context
        GX2_CONTEXT_STATE_ALIGNMENT // Required alignment
    );

    if (!gContext)
        WindowExit();

    // Initialize it to default state
    GX2SetupContextStateEx(gContext, false);

    // Make context of window current
    WindowMakeContextCurrent();

    // Set swap interval to 1 by default
    WindowSetSwapInterval(1);

    // Set the default viewport and scissor
    GX2SetViewport(0, 0, gWindowWidth, gWindowHeight, 0.0f, 1.0f);
    GX2SetScissor(0, 0, gWindowWidth, gWindowHeight);

    // Disable depth test
    GX2SetDepthOnlyControl(
        FALSE,                  // Depth Test;     equivalent to glDisable(GL_DEPTH_TEST)
        FALSE,                  // Depth Write;    equivalent to glDepthMask(GL_FALSE)
        GX2_COMPARE_FUNC_LEQUAL // Depth Function; equivalent to glDepthFunc(GL_LEQUAL)
    );

    // Initialize ProcUI
    ProcUIInit(&OSSavesDone_ReadyToRelease);

    gInitialized = true;
    gIsRunning = true;

    // Set the output framebuffer size pointers
    if (pWidth)
        *pWidth = gWindowWidth;
    if (pHeight)
        *pHeight = gWindowHeight;
}

void WindowMakeContextCurrent()
{
    GX2SetContextState(gContext);
    GX2SetColorBuffer(&gColorBuffer, GX2_RENDER_TARGET_0);
    GX2SetDepthBuffer(&gDepthBuffer);
}

void WindowSetSwapInterval(u32 swap_interval)
{
    GX2SetSwapInterval(swap_interval);
}

bool WindowIsRunning()
{
    return gIsRunning;
}

const OSTime goalFramerate = (OSTime)OSSecondsToTicks(1.0f/59.9f);
void WindowWaitForPreviousFrame(u32 framesToQueue) {
    if (!WindowIsRunning())
        return;

    GX2WaitForVsync();
}

void WindowSwapBuffers(bool usePostBuffer)
{
    // Make sure to flush all commands to GPU before copying the color buffer to the scan buffers
    // (Calling GX2DrawDone instead here causes slow-downs)
    GX2Flush();

    // Copy the color buffer to the TV and DRC scan buffers
    GX2CopyColorBufferToScanBuffer(usePostBuffer ? &gPostBuffer : &gColorBuffer, GX2_SCAN_TARGET_TV);
    GX2CopyColorBufferToScanBuffer(usePostBuffer ? &gPostBuffer : &gColorBuffer, GX2_SCAN_TARGET_DRC);
    // Flip
    GX2SwapScanBuffers();

    // Reset context state for next frame
    GX2SetContextState(gContext);

    // Flush all commands to GPU before GX2WaitForFlip since it will block the CPU
    GX2Flush();

    // Make sure TV and DRC are enabled
    GX2SetTVEnable(true);
    GX2SetDRCEnable(true);

    // Wait until swapping is done
    // DebugProfile::Start("Swap Buffers -> GX2DrawDone");
    // GX2DrawDone();
    // DebugProfile::End("Swap Buffers -> GX2DrawDone");

    // DebugProfile::Start("Swap Buffers -> GX2WaitForFlip");
    // GX2WaitForFlip();
    // DebugProfile::End("Swap Buffers -> GX2WaitForFlip");

    // ProcUI
    ProcUIStatus status = ProcUIProcessMessages(true);
    ProcUIStatus previous_status = status;

    if (status == PROCUI_STATUS_RELEASE_FOREGROUND)
    {
        WindowForegroundRelease();
        status = ProcUIProcessMessages(true);
    }

    if (status == PROCUI_STATUS_EXITING || (previous_status == PROCUI_STATUS_RELEASE_FOREGROUND && status == PROCUI_STATUS_IN_FOREGROUND && !WindowForegroundAcquire()))
    {
        ProcUIShutdown();
        WindowExit();
    }
}

#ifdef __cplusplus
extern "C" {
#endif
    void OSBlockThreadsOnExit(void);
    void _Exit(int);
#ifdef __cplusplus
}
#endif

void WindowExit()
{
    gIsRunning = false;

    if (gCmdlist)
    {
        MEMFreeToDefaultHeap(gCmdlist);
        gCmdlist = NULL;
    }

    if (gContext)
    {
        MEMFreeToDefaultHeap(gContext);
        gContext = NULL;
    }

    WindowForegroundRelease();

    OSBlockThreadsOnExit();
    _Exit(-1);
}

GX2ColorBuffer* WindowGetColorBuffer()
{
    return &gColorBuffer;
}

GX2ColorBuffer* WindowGetPostBuffer()
{
    return &gPostBuffer;
}

GX2DepthBuffer* WindowGetDepthBuffer()
{
    return &gDepthBuffer;
}

GX2Texture* WindowGetColorBufferTexture()
{
    return &gColorBufferTexture;
}

GX2Texture* WindowGetPostBufferTexture()
{
    return &gColorBufferTexture;
}

GX2Surface* WindowGetColorBufferSurface()
{
    return &gColorBuffer.surface;
}

GX2Surface* WindowGetPostBufferSurface()
{
    return &gColorBuffer.surface;
}

GX2Texture* WindowGetDepthBufferTexture()
{
    return &gDepthBufferTexture;
}

s32 WindowGetWidth()
{
    return gWindowWidth;
}

s32 WindowGetHeight()
{
    return gWindowHeight;
}

