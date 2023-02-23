#pragma once
#include "../common/common.h"

class Sprite
{
    friend class Render;
public:
    Sprite(const char* path, bool hasTransparency = false);
    Sprite(u32 width, u32 height, bool hasTransparency); // create sprite with uninitialized pixels
    struct GX2Texture* GetTexture() { return m_tex; }

    void SetPixel(u32 x, u32 y, u32 color);
    void FlushCache();

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
    static void RenderSprite(Sprite* sprite, s32 x, s32 y, s32 pxWidth, s32 pxHeight);
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