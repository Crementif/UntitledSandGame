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

std::string _LoadShaderSource(std::string_view shaderName, std::string_view extension)
{
    std::string name = std::string(shaderName).append(extension);
    std::ifstream fs((std::string("romfs:/shaders/") + name).c_str(), std::ios::in | std::ios::binary);
    if(!fs.is_open())
        CriticalErrorHandler("Failed to open file shaders/%s\n", name.c_str());
    std::vector<u8> data((std::istreambuf_iterator<char>(fs)), std::istreambuf_iterator<char>());
    return std::string{data.begin(), data.end()};
}

GX2ShaderSet* GX2ShaderSet::Load(const char *name)
{
    if(!s_fetchShaderInitialized)
    {
        _InitDefaultFetchShader();
        s_fetchShaderInitialized = true;
    }

    GX2ShaderSet* shaderSet = new GX2ShaderSet();

    std::string vsSource = _LoadShaderSource(name, ".vs");
    std::string psSource = _LoadShaderSource(name, ".ps");

    char outputBuff[1024];
    GX2VertexShader* vs = GLSL_CompileVertexShader(vsSource.c_str(), outputBuff, sizeof(outputBuff), GLSL_COMPILER_FLAG_NONE);
    if(!vs) {
        WHBLogPrintf("Failed to compile vertex shader: %s", outputBuff);
        CriticalErrorHandler("Failed to compile vertex shader");
    }
    GX2PixelShader* ps = GLSL_CompilePixelShader(psSource.c_str(), outputBuff, sizeof(outputBuff), GLSL_COMPILER_FLAG_NONE);
    if(!ps) {
        WHBLogPrintf("Failed to compile pixel shader: %s", outputBuff);
        CriticalErrorHandler("Failed to compile pixel shader");
    }

    shaderSet->vertexShader = vs;
    shaderSet->fragmentShader = ps;
    shaderSet->fetchShader = &s_defaultFetchShader;
    shaderSet->Prepare();
    return shaderSet;
}

void GX2ShaderSet::Prepare()
{
    GX2Invalidate(GX2_INVALIDATE_MODE_CPU_SHADER, fetchShader->program, fetchShader->size);
    GX2Invalidate(GX2_INVALIDATE_MODE_CPU_SHADER, vertexShader->program, vertexShader->size);
    GX2Invalidate(GX2_INVALIDATE_MODE_CPU_SHADER, fragmentShader->program, fragmentShader->size);
}

void GX2ShaderSet::Activate()
{
    GX2SetFetchShader(fetchShader);
    GX2SetVertexShader(vertexShader);
    GX2SetPixelShader(fragmentShader);
}