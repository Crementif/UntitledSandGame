#include "render.h"

#include <gx2/clear.h>
#include <gx2/draw.h>
#include <gx2/mem.h>
#include <gx2/registers.h>
#include <gx2/utils.h>
#include <gx2/surface.h>

static GX2FetchShader s_defaultFetchShader;
static bool s_fetchShaderInitialized{false};

void _InitDefaultFetchShader()
{
    GX2AttribStream streams[2];
    GX2AttribStream& pos_stream = streams[0];
    pos_stream.location = 0;
    pos_stream.buffer = 0;
    pos_stream.offset = offsetof(Vertex, pos);
    pos_stream.format = GX2_ATTRIB_FORMAT_FLOAT_32_32;
    pos_stream.mask = GX2_SEL_MASK(GX2_SQ_SEL_X, GX2_SQ_SEL_Y, GX2_SQ_SEL_0, GX2_SQ_SEL_1);
    pos_stream.endianSwap = GX2_ENDIAN_SWAP_DEFAULT;
    pos_stream.type = GX2_ATTRIB_INDEX_PER_VERTEX;
    pos_stream.aluDivisor = 0;

    GX2AttribStream& tex_coord_stream = streams[1];
    tex_coord_stream.location = 1;
    tex_coord_stream.buffer = 0;
    tex_coord_stream.offset = offsetof(Vertex, tex_coord);
    tex_coord_stream.format = GX2_ATTRIB_FORMAT_FLOAT_32_32;
    tex_coord_stream.mask = GX2_SEL_MASK(GX2_SQ_SEL_X, GX2_SQ_SEL_Y, GX2_SQ_SEL_0, GX2_SQ_SEL_1);
    tex_coord_stream.endianSwap = GX2_ENDIAN_SWAP_DEFAULT;
    tex_coord_stream.type = GX2_ATTRIB_INDEX_PER_VERTEX;
    tex_coord_stream.aluDivisor = 0;

    u32 fetchShaderProgramSize = GX2CalcFetchShaderSizeEx(2, GX2_FETCH_SHADER_TESSELLATION_NONE, GX2_TESSELLATION_MODE_DISCRETE);
    void* fetchShaderProgramCode = MEMAllocFromDefaultHeapEx(fetchShaderProgramSize, GX2_SHADER_PROGRAM_ALIGNMENT);
    GX2InitFetchShaderEx(&s_defaultFetchShader, (u8*)fetchShaderProgramCode, 2, streams, GX2_FETCH_SHADER_TESSELLATION_NONE, GX2_TESSELLATION_MODE_DISCRETE);
}

std::string _LoadShaderSource(std::string_view shaderPath)
{
    std::ifstream fs(std::string(shaderPath).c_str(), std::ios::in | std::ios::binary);
    if (!fs.is_open()) {
        OSReport("Failed to open shader file from %s\n", std::string(shaderPath).c_str());
        return "";
    }
    std::vector<u8> data((std::istreambuf_iterator<char>(fs)), std::istreambuf_iterator<char>());
    return std::string{data.begin(), data.end()};
}

GX2ShaderSet* GX2ShaderSet::Create(const char *name) {
    GX2ShaderSet* shaderSet = new GX2ShaderSet();
    shaderSet->name = name;
    shaderSet->Load();
    return shaderSet;
}

void GX2ShaderSet::Load()
{
    if(!s_fetchShaderInitialized)
    {
        _InitDefaultFetchShader();
        s_fetchShaderInitialized = true;
    }

    std::string vsSource = _LoadShaderSource(std::string("TankTrap/shaders/") + name + ".vs");
    std::string psSource = _LoadShaderSource(std::string("TankTrap/shaders/") + name + ".ps");
    if (vsSource.empty() || psSource.empty()) {
        vsSource = _LoadShaderSource(std::string("romfs:/shaders/") + name + ".vs");
        psSource = _LoadShaderSource(std::string("romfs:/shaders/") + name + ".ps");
    }
    else {
        OSReport("Loaded shader %s from SD card\n", name.c_str());
    }

    char outputBuff[1024];
    GX2VertexShader* vs = GLSL_CompileVertexShader(vsSource.c_str(), outputBuff, sizeof(outputBuff), GLSL_COMPILER_FLAG_NONE);
    if(!vs) {
        WHBLogPrintf("Failed to compile vertex shader: %s", outputBuff);
        if (this->vertexShader)
            CriticalErrorHandler("Failed to compile vertex shader");
        else
            return;
    }
    GX2PixelShader* ps = GLSL_CompilePixelShader(psSource.c_str(), outputBuff, sizeof(outputBuff), GLSL_COMPILER_FLAG_NONE);
    if(!ps) {
        WHBLogPrintf("Failed to compile pixel shader: %s", outputBuff);
        if (this->fragmentShader == nullptr)
            CriticalErrorHandler("Failed to compile pixel shader");
        else
            return;
    }

    this->vertexShader = vs;
    this->fragmentShader = ps;
    this->fetchShader = &s_defaultFetchShader;
    this->Prepare();
    this->_lastReloadedShader = OSGetTime();
}

void GX2ShaderSet::Prepare()
{
    GX2Invalidate(GX2_INVALIDATE_MODE_CPU_SHADER, fetchShader->program, fetchShader->size);
    GX2Invalidate(GX2_INVALIDATE_MODE_CPU_SHADER, vertexShader->program, vertexShader->size);
    GX2Invalidate(GX2_INVALIDATE_MODE_CPU_SHADER, fragmentShader->program, fragmentShader->size);
}

void GX2ShaderSet::Activate()
{
    // you can tweak these settings to disallow certain shaders and the reload time
    if (this->name != "sprite" && (_lastReloadedShader + (OSTime)OSSecondsToTicks(5)) <= OSGetTime()) {
        this->Load();
    }

    GX2SetFetchShader(fetchShader);
    GX2SetVertexShader(vertexShader);
    GX2SetPixelShader(fragmentShader);
}