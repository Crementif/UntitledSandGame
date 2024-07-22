#include "render.h"

#include <gx2/clear.h>
#include <gx2/draw.h>
#include <gx2/mem.h>
#include <gx2/registers.h>
#include <gx2/utils.h>
#include <gx2/event.h>
#include <gx2/surface.h>
#include <whb/gfx.h>

RenderState::E_TRANSPARENCY_MODE RenderState::sRenderTransparencyMode = RenderState::E_TRANSPARENCY_MODE::OPAQUE;
GX2ColorControlReg RenderState::sRenderColorControl_transparency;
GX2ColorControlReg RenderState::sRenderColorControl_noTransparency;
GX2BlendControlReg RenderState::sRenderBlendReg_transparency;
GX2ShaderMode RenderState::sShaderMode = GX2_SHADER_MODE_UNIFORM_REGISTER;

void RenderState::Init()
{
    GX2InitColorControlReg(&sRenderColorControl_transparency, GX2_LOGIC_OP_COPY, 0x01, GX2_FALSE, GX2_TRUE);
    GX2InitColorControlReg(&sRenderColorControl_noTransparency, GX2_LOGIC_OP_COPY, 0x00, GX2_FALSE, GX2_TRUE);
    GX2InitBlendControlReg(&sRenderBlendReg_transparency,
                           GX2_RENDER_TARGET_0,
                           GX2_BLEND_MODE_SRC_ALPHA, GX2_BLEND_MODE_INV_SRC_ALPHA, GX2_BLEND_COMBINE_MODE_ADD,
                           GX2_FALSE, // no separate alpha blend
                           GX2_BLEND_MODE_SRC_ALPHA, GX2_BLEND_MODE_INV_SRC_ALPHA, GX2_BLEND_COMBINE_MODE_ADD);
    SetShaderMode(GX2_SHADER_MODE_UNIFORM_BLOCK);
}

void RenderState::ReapplyState()
{
    auto transparencyMode = sRenderTransparencyMode;
    sRenderTransparencyMode = (E_TRANSPARENCY_MODE)-1;
    SetTransparencyMode(transparencyMode);
    auto shaderMode = sShaderMode;
    sShaderMode = (GX2ShaderMode)-1;
    SetShaderMode(shaderMode);
}

void RenderState::SetShaderMode(GX2ShaderMode mode)
{
    if (sShaderMode != mode) {
        GX2SetShaderModeEx(mode, 0x30, 0x40, 0x0, 0x0, 0xc8, 0xc0);
        sShaderMode = mode;
    }
}

void RenderState::SetTransparencyMode(E_TRANSPARENCY_MODE mode)
{
    if(sRenderTransparencyMode != mode)
    {
        GX2SetColorControlReg(mode == E_TRANSPARENCY_MODE::ADDITIVE ? &sRenderColorControl_transparency : &sRenderColorControl_noTransparency);
        if(mode == E_TRANSPARENCY_MODE::ADDITIVE)
            GX2SetBlendControlReg(&sRenderBlendReg_transparency);
        sRenderTransparencyMode = mode;
    }
}