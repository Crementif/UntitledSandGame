#pragma once
#include <cstdint>
#include <cstring>
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
        bool isRLE = (tgaHeader->imagetype & 0x08) != 0; // Check if RLE compression is used

        m_width = width;
        m_height = height;
        m_bpp = tgaHeader->bits;
        uint8_t* pixelBaseIn = (uint8_t*)(tgaFileData.data() + sizeof(TGA_HEADER));

        u32 pixelSize = tgaHeader->bits / 8;
        u32 dataSize = width * height * pixelSize;
        m_tgaData.resize(dataSize);

        if (isRLE) {
            // Handle RLE decoding with vertical flipping
            u32 pixelCount = 0;
            while (pixelCount < width * height) {
                uint8_t packetHeader = *pixelBaseIn++;
                uint8_t packetType = packetHeader & 0x80;
                u32 packetSize = (packetHeader & 0x7F) + 1;

                if (packetType) {
                    // Run-length packet
                    for (u32 i = 0; i < packetSize; ++i) {
                        u32 row = (height - 1 - (pixelCount / width));
                        u32 col = pixelCount % width;
                        std::copy_n(pixelBaseIn, pixelSize, &m_tgaData[(row * width + col) * pixelSize]);
                        pixelCount++;
                    }
                    pixelBaseIn += pixelSize;
                }
                else {
                    // Raw packet
                    for (u32 i = 0; i < packetSize; ++i) {
                        u32 row = (height - 1 - (pixelCount / width));
                        u32 col = pixelCount % width;
                        std::copy_n(pixelBaseIn, pixelSize, &m_tgaData[(row * width + col) * pixelSize]);
                        pixelBaseIn += pixelSize;
                        pixelCount++;
                    }
                }
            }
        }
        else {
            // Handle non-RLE data with vertical flipping
            for (u32 y = 0; y < height; ++y) {
                uint8_t* srcRow = pixelBaseIn + (height - 1 - y) * width * pixelSize;
                uint8_t* dstRow = m_tgaData.data() + y * width * pixelSize;
                std::memcpy(dstRow, srcRow, width * pixelSize);
            }
        }

        // Flip and swap color channels
        for (u32 y = 0; y < height; ++y) {
            for (u32 x = 0; x < width; ++x) {
                uint8_t* pixel = &m_tgaData[((height - 1 - y) * width + x) * pixelSize];
                std::swap(pixel[0], pixel[2]); // Swap red and blue channels
            }
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
