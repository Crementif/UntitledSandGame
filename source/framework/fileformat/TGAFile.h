#pragma once
#include <cstdint>
#include <span>

#include <coreinit/debug.h>

struct TGA_HEADER
{
    u8  identsize;          // size of ID field that follows 18 byte header (0 usually)
    u8  colourmaptype;      // type of colour map 0=none, 1=has palette
    u8  imagetype;          // type of image 0=none,1=indexed,2=rgb,3=grey,+8=rle packed

    u8 colourmapstart[2];     // first colour map entry in palette
    u8 colourmaplength[2];    // number of colours in palette
    u8  colourmapbits;      // number of bits per palette entry 15,16,24,32

    u16 xstart;             // image x origin
    u16 ystart;             // image y origin
    u16 width;              // image width in pixels
    u16 height;             // image height in pixels
    u8  bits;               // image bits per pixel 8,16,24,32
    u8  descriptor;         // image descriptor bits (vh flip bits)
};

class TGALoader
{
public:
    bool LoadFromTGA(std::span<uint8_t> tgaFileData)
    {
        TGA_HEADER* tgaHeader = (TGA_HEADER*)tgaFileData.data();

        u32 width = _swapU16(tgaHeader->width);
        u32 height = _swapU16(tgaHeader->height);

        m_width = width;
        m_height = height;

        m_bpp = tgaHeader->bits;
        uint8_t* pixelBaseIn = (uint8_t*)(tgaFileData.data()+sizeof(TGA_HEADER));
        if(tgaHeader->bits == 24)
        {
            // 3 byte format
            m_tgaData.resize(width * height * 3);
            for(u32 y=0; y<height; y++)
            {
                uint8_t* tga_bgra_data = pixelBaseIn + ((height - y - 1) * width) * 3;
                uint8_t* out_data = (uint8_t*)m_tgaData.data() + (y * width) * 3;
                for(u32 x=0; x<width; x++)
                {
                    out_data[2] = tga_bgra_data[0];
                    out_data[1] = tga_bgra_data[1];
                    out_data[0] = tga_bgra_data[2];
                    tga_bgra_data += 3;
                    out_data += 3;
                }
            }
        }
        else if(tgaHeader->bits == 32)
        {
            // 4 byte format
            m_tgaData.resize(width * height * 4);
            for(u32 y=0; y<height; y++)
            {
                uint8_t* tga_bgra_data = pixelBaseIn + ((height - y - 1) * width) * 4;
                uint8_t* out_data = (uint8_t*)m_tgaData.data() + (y * width) * 4;
                for(u32 x=0; x<width; x++)
                {
                    out_data[2] = tga_bgra_data[0];
                    out_data[1] = tga_bgra_data[1];
                    out_data[0] = tga_bgra_data[2];
                    out_data[3] = tga_bgra_data[3];
                    tga_bgra_data += 4;
                    out_data += 4;
                }
            }
        }
        else
        {
            OSFatal("TGAFile: Unsupported bit width");
            // todo
        }
        return true;
    }

    uint32_t GetBPP() const
    {
        return m_bpp;
    }

    uint32_t GetWidth() const
    {
        return m_width;
    }

    uint32_t GetHeight() const
    {
        return m_height;
    }

    // for 24bit: RR GG BB
    // for 32bit: RR GG BB AA (needs fixing?)
    uint8_t* GetData()
    {
        return m_tgaData.data();
    }

private:
    std::vector<uint8_t> m_tgaData;
    uint32_t m_width;
    uint32_t m_height;
    uint8_t m_bpp;
};
