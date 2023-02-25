#pragma once
#include "../common/common.h"

inline OSDynLoad_Module s_glslCompilerModule = nullptr;
inline GX2VertexShader* (*GLSL_CompileVertexShader)(const char* shaderSource, char* infoLogOut, int infoLogMaxLength);
inline GX2PixelShader* (*GLSL_CompilePixelShader)(const char* shaderSource, char* infoLogOut, int infoLogMaxLength);

#ifdef __cplusplus
extern "C" {
#endif
    int32_t OSGetSystemMode();
#ifdef __cplusplus
}
#endif

static bool GLSL_Init()
{
    if (s_glslCompilerModule != nullptr)
        return false;

    OSDynLoad_Error r = OSDynLoad_Acquire(OSGetSystemMode() == 0 ? "glslcompiler" : "~/wiiu/libs/glslcompiler.rpl", &s_glslCompilerModule);
    OSReport("GLSLCompiler_Init: r = %d\n", r);
    if (r != OS_DYNLOAD_OK)
        return false;

    // find exports
    void (*_InitGLSLCompiler)() = nullptr;
    OSDynLoad_FindExport(s_glslCompilerModule, OS_DYNLOAD_EXPORT_FUNC, "InitGLSLCompiler", (void**)&_InitGLSLCompiler);
    OSDynLoad_FindExport(s_glslCompilerModule, OS_DYNLOAD_EXPORT_FUNC, "CompileVertexShader", (void**)&GLSL_CompileVertexShader);
    OSDynLoad_FindExport(s_glslCompilerModule, OS_DYNLOAD_EXPORT_FUNC, "CompilePixelShader", (void**)&GLSL_CompilePixelShader);
    OSReport("GLSLCompiler_Init: Calling InitGLSLCompiler...\n");
    _InitGLSLCompiler();
    OSReport("GLSLCompiler_Init: Done\n");

    return true;
}

static bool GLSL_Shutdown()
{
    if (s_glslCompilerModule == nullptr)
        return false;

    void (*_DestroyGLSLCompiler)() = nullptr;
    OSDynLoad_FindExport(s_glslCompilerModule, OS_DYNLOAD_EXPORT_FUNC, "DestroyGLSLCompiler", (void**)&_DestroyGLSLCompiler);
    OSReport("GLSLCompiler_Shutdown: Calling DestroyGLSLCompiler...\n");
    _DestroyGLSLCompiler();

    OSDynLoad_Release(s_glslCompilerModule);
    return true;
}

