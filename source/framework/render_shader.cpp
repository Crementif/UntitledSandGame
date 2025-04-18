#include "render.h"

#include <gx2/clear.h>
#include <gx2/draw.h>
#include <gx2/mem.h>
#include <gx2/registers.h>
#include <gx2/utils.h>
#include <gx2/surface.h>

#include "../common/shader_serializer.h"

std::vector<u8> _LoadShaderFile(const std::string_view shaderName, std::string_view extension) {
#ifdef DEBUG
    std::string name = std::string(shaderName).append(extension);
    std::ifstream fs(("./shaders/" + name).c_str(), std::ios::in | std::ios::binary);
    if (!fs.is_open()) {
        fs = std::ifstream(("fs:/vol/content/shaders/" + name).c_str(), std::ios::in | std::ios::binary);
    }
#else
    std::string name = std::string(shaderName).append(extension);
    std::ifstream fs(("fs:/vol/content/shaders/" + name).c_str(), std::ios::in | std::ios::binary);
#endif
    if(!fs.is_open()) {
        CriticalErrorHandler("Failed to open file shaders/%s\n", name.c_str());
    }
    std::vector<u8> data((std::istreambuf_iterator<char>(fs)), std::istreambuf_iterator<char>());
    return data;
}

std::string _LoadShaderSource(const std::string_view shaderName, std::string_view extension) {
    auto sourceFile = _LoadShaderFile(shaderName, extension);
    return std::string(sourceFile.begin(), sourceFile.end());
}

GX2ShaderSet::GX2ShaderSet(const std::string_view name)
{
    if (!s_defaultFetchShaderInitialized)
        InitDefaultFetchShader();

    if (GLSL_CompilePixelShader != nullptr) {
        WHBLogPrintf("Compiling and loading shaders for %s", name.data());
        std::string vsSource = _LoadShaderSource(name, ".vs");
        std::string psSource = _LoadShaderSource(name, ".ps");

        char outputBuff[1024];
        GX2VertexShader* vs = GLSL_CompileVertexShader(vsSource.c_str(), outputBuff, sizeof(outputBuff), GLSL_COMPILER_FLAG_NONE);
        if (!vs) {
            WHBLogPrintf("Failed to compile vertex shader: %s", outputBuff);
            return;
        }
        GX2PixelShader* ps = GLSL_CompilePixelShader(psSource.c_str(), outputBuff, sizeof(outputBuff), GLSL_COMPILER_FLAG_NONE);
        if (!ps) {
            WHBLogPrintf("Failed to compile pixel shader: %s", outputBuff);
            return;
        }
        this->precompiled = false;
        this->vertexShader = vs;
        this->fragmentShader = ps;
    }
    else {
        WHBLogPrintf("Loading precompiled shaders for %s", name.data());
        auto vertexBytes = _LoadShaderFile(name, ".vs.gsh");
        auto pixelBytes = _LoadShaderFile(name, ".ps.gsh");

        this->precompiled = true;
        this->vertexShader = WHBGfxLoadGFDVertexShader(0, vertexBytes.data());
        this->fragmentShader = WHBGfxLoadGFDPixelShader(0, pixelBytes.data());
    }

    this->fetchShader = &s_defaultFetchShader;
    this->Prepare();
    this->compiledSuccessfully = true;
}

GX2ShaderSet::~GX2ShaderSet() {
    if (vertexShader && precompiled) {
        WHBGfxFreeVertexShader(vertexShader);
        vertexShader = nullptr;
    }
    if (fragmentShader && precompiled) {
        WHBGfxFreePixelShader(fragmentShader);
        fragmentShader = nullptr;
    }
}

void GX2ShaderSet::Prepare() const
{
    GX2Invalidate(GX2_INVALIDATE_MODE_CPU_SHADER, fetchShader->program, fetchShader->size);
    GX2Invalidate(GX2_INVALIDATE_MODE_CPU_SHADER, vertexShader->program, vertexShader->size);
    GX2Invalidate(GX2_INVALIDATE_MODE_CPU_SHADER, fragmentShader->program, fragmentShader->size);
}

void GX2ShaderSet::Activate() const
{
    GX2SetFetchShader(fetchShader);
    GX2SetVertexShader(vertexShader);
    GX2SetPixelShader(fragmentShader);
}

GX2FetchShader GX2ShaderSet::s_defaultFetchShader;
bool GX2ShaderSet::s_defaultFetchShaderInitialized = false;

void GX2ShaderSet::InitDefaultFetchShader()
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
    s_defaultFetchShaderInitialized = true;
}


void GX2ShaderSet::SerializeToFile(std::string path) {
    std::ofstream vertexFile(path+".precompiled.vs", std::ios::binary);
    if (!vertexFile.is_open()) {
        CriticalErrorHandler("Failed to open file %s\n", path.data());
    }
    std::vector<uint8_t> vertexBytes = SerializeVertexShader(this->vertexShader);
    vertexFile.write(reinterpret_cast<const char*>(vertexBytes.data()), vertexBytes.size());
    vertexFile.close();


    std::ofstream pixelFile(path+".precompiled.ps", std::ios::binary);
    if (!pixelFile.is_open()) {
        CriticalErrorHandler("Failed to open file %s\n", path.data());
    }

    std::vector<uint8_t> pixelBytes = SerializePixelShader(this->fragmentShader);
    pixelFile.write(reinterpret_cast<const char*>(pixelBytes.data()), pixelBytes.size());
    pixelFile.close();
}