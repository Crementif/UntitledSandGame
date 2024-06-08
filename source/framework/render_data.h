#pragma once

__attribute__((aligned(GX2_SHADER_PROGRAM_ALIGNMENT))) static const u32 triangle_VSH_program[] = {
    0x00000000, 0x00008009, 0x20000000, 0x00000ca0,
    0x3ca00000, 0x88060094, 0x00400100, 0x88042014,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x02001f00, 0x900c4000, 0x02041f00, 0x900c4020,
    0xfd001f80, 0x900c2060, 0x0000803f, 0x00000000
};

static const GX2AttribVar triangle_VSH_attribVars[] = {
    {"v_inPos", GX2_SHADER_VAR_TYPE_FLOAT3, 0, 0},
    {"v_inTexCoord", GX2_SHADER_VAR_TYPE_FLOAT2, 0, 1}
};

static const GX2VertexShader triangle_VSH = {
    {
        0x00000103,
        0x00000000,
        0x00000000,
        0x00000001,
        {
            0xffffff00, 0xffffffff,
            0xffffffff, 0xffffffff,
            0xffffffff, 0xffffffff,
            0xffffffff, 0xffffffff,
            0xffffffff, 0xffffffff
        },
        0x00000000,
        0xfffffffc,
        0x00000002,
        {
            0x00000000, 0x00000001, 0x000000ff, 0x000000ff,
            0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff,
            0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff,
            0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff,
            0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff,
            0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff,
            0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff,
            0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff
        },
        0x00000000,
        0x0000000e,
        0x00000010
    },
    sizeof(triangle_VSH_program),
    (void*)triangle_VSH_program,
    GX2_SHADER_MODE_UNIFORM_REGISTER,
    0,
    NULL,
    0,
    NULL,
    0,
    NULL,
    0,
    NULL,
    0,
    NULL,
    2,
    (GX2AttribVar*)triangle_VSH_attribVars,
    0,
    FALSE,
    {0x00000000, 0x00000000, 0x00000000, 0x00000000}
};

__attribute__((aligned(GX2_SHADER_PROGRAM_ALIGNMENT))) static const u32 triangle_PSH_program[] = {
    0x10000000, 0x0000c080, 0x00000000, 0x88062094,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x10000000, 0x00100df0, 0x00008010, 0xecdfea0d
};

static const GX2SamplerVar triangle_PSH_samplerVars[] = {
    {"texture", GX2_SAMPLER_VAR_TYPE_SAMPLER_2D, 0}
};

static const GX2PixelShader triangle_PSH = {
    {
        0x00000001,
        0x00000002,
        0x14000001,
        0x00000000,
        0x00000001,
        {
            0x00000100, 0x00000000, 0x00000000, 0x00000000,
            0x00000000, 0x00000000, 0x00000000, 0x00000000,
            0x00000000, 0x00000000, 0x00000000, 0x00000000,
            0x00000000, 0x00000000, 0x00000000, 0x00000000,
            0x00000000, 0x00000000, 0x00000000, 0x00000000,
            0x00000000, 0x00000000, 0x00000000, 0x00000000,
            0x00000000, 0x00000000, 0x00000000, 0x00000000,
            0x00000000, 0x00000000, 0x00000000, 0x00000000
        },
        0x0000000f,
        0x00000001,
        0x00000010,
        0x00000000
    },
    sizeof(triangle_PSH_program),
    (void*)triangle_PSH_program,
    GX2_SHADER_MODE_UNIFORM_REGISTER,
    0,
    NULL,
    0,
    NULL,
    0,
    NULL,
    0,
    NULL,
    1,
    (GX2SamplerVar*)triangle_PSH_samplerVars
};
