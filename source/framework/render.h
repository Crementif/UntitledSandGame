#pragma once
#include "../common/common.h"

class Sprite
{
    friend class Render;
public:
    Sprite(const char* path, bool hasTransparency = false);
    struct GX2Texture* GetTexture() { return m_tex; }

private:
    struct GX2Texture* m_tex;
    s32 m_width;
    s32 m_height;
    bool m_hasTransparency;
};

class Render
{
public:
    static void Init();
    static void Shutdown();
    static void BeginFrame();
    static bool IsRunning();
    static void SwapBuffers();

    static void BeginSpriteRendering();
    static void EndSpriteRendering();

    static void RenderSprite(Sprite* sprite, s32 x, s32 y);
    static void RenderSpriteScreenRelative(Sprite* sprite, s32 x, s32 y);
    static void RenderSpritePortionScreenRelative(Sprite*, s32, s32, u32, u32, u32, u32);

    static void SetCameraPosition(Vector2f pos);
    static Vector2f GetCameraPosition(); // returns camera position, pixel-aligned
    static Vector2f GetUnfilteredCameraPosition(); // returns the original vector passed to SetCameraPosition

    static void RenderText(u32 x, u32 y, u8 textSize, u8 blackLevel, const char* text, ...);
protected:
    static Sprite* sBigFontTextureBlack;
    static Sprite* sBigFontTextureWhite;
    static Sprite* sSmallFontTextureBlack;
    static Sprite* sSmallFontTextureWhite;
};