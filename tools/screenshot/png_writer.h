#pragma once

// Minimal, dependency-free PNG writer for the screenshot tool. Emits an
// uncompressed ("stored" deflate blocks) zlib stream — larger files than a
// real compressor, but this only ever runs a few dozen times to produce
// docs/screenshots/*.png, so file size doesn't matter and there is no need
// to vendor a compression library for it.

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace ScreenshotTool {
namespace detail {

inline std::uint32_t Crc32(const unsigned char* data, std::size_t length,
                           std::uint32_t crc = 0xffffffffu) {
    static std::uint32_t table[256];
    static bool initialized = false;
    if (!initialized) {
        for (std::uint32_t n = 0; n < 256; ++n) {
            std::uint32_t c = n;
            for (int k = 0; k < 8; ++k)
                c = (c & 1) ? (0xedb88320u ^ (c >> 1)) : (c >> 1);
            table[n] = c;
        }
        initialized = true;
    }
    for (std::size_t i = 0; i < length; ++i)
        crc = table[(crc ^ data[i]) & 0xffu] ^ (crc >> 8);
    return crc;
}

inline void PutBigEndian32(std::vector<unsigned char>& out, std::uint32_t value) {
    out.push_back(static_cast<unsigned char>((value >> 24) & 0xff));
    out.push_back(static_cast<unsigned char>((value >> 16) & 0xff));
    out.push_back(static_cast<unsigned char>((value >> 8) & 0xff));
    out.push_back(static_cast<unsigned char>(value & 0xff));
}

inline void WriteChunk(std::vector<unsigned char>& out, const char type[4],
                       const unsigned char* data, std::size_t length) {
    PutBigEndian32(out, static_cast<std::uint32_t>(length));
    const std::size_t type_offset = out.size();
    out.insert(out.end(), type, type + 4);
    out.insert(out.end(), data, data + length);
    const std::uint32_t crc =
        Crc32(&out[type_offset], length + 4) ^ 0xffffffffu;
    PutBigEndian32(out, crc);
}

inline std::uint32_t Adler32(const unsigned char* data, std::size_t length) {
    std::uint32_t a = 1, b = 0;
    constexpr std::uint32_t kMod = 65521u;
    for (std::size_t i = 0; i < length; ++i) {
        a = (a + data[i]) % kMod;
        b = (b + a) % kMod;
    }
    return (b << 16) | a;
}

} // namespace detail

// `rgba` must contain width*height*4 bytes, top-to-bottom, row-major.
inline bool WritePng(const std::string& path, int width, int height,
                    const unsigned char* rgba) {
    // Raw scanlines: one filter-type byte (0 = None) followed by the pixel
    // row, per PNG spec.
    const std::size_t stride = static_cast<std::size_t>(width) * 4;
    std::vector<unsigned char> raw;
    raw.reserve((stride + 1) * static_cast<std::size_t>(height));
    for (int y = 0; y < height; ++y) {
        raw.push_back(0);
        raw.insert(raw.end(), rgba + y * stride, rgba + y * stride + stride);
    }

    // zlib stream: 2-byte header, one or more "stored" (uncompressed) deflate
    // blocks (max 65535 bytes each), 4-byte Adler-32 trailer.
    std::vector<unsigned char> zlib_stream;
    zlib_stream.push_back(0x78);
    zlib_stream.push_back(0x01);
    std::size_t offset = 0;
    while (true) {
        const std::size_t remaining = raw.size() - offset;
        const std::size_t block_size = std::min<std::size_t>(remaining, 65535);
        const bool final_block = (offset + block_size) >= raw.size();
        zlib_stream.push_back(final_block ? 1 : 0);
        const std::uint16_t len = static_cast<std::uint16_t>(block_size);
        const std::uint16_t nlen = static_cast<std::uint16_t>(~len);
        zlib_stream.push_back(static_cast<unsigned char>(len & 0xff));
        zlib_stream.push_back(static_cast<unsigned char>((len >> 8) & 0xff));
        zlib_stream.push_back(static_cast<unsigned char>(nlen & 0xff));
        zlib_stream.push_back(static_cast<unsigned char>((nlen >> 8) & 0xff));
        zlib_stream.insert(zlib_stream.end(), raw.data() + offset,
                           raw.data() + offset + block_size);
        offset += block_size;
        if (final_block) break;
    }
    const std::uint32_t adler = detail::Adler32(raw.data(), raw.size());
    detail::PutBigEndian32(zlib_stream, adler);

    std::vector<unsigned char> png;
    static const unsigned char kSignature[8] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n'};
    png.insert(png.end(), kSignature, kSignature + 8);

    unsigned char ihdr[13];
    ihdr[0] = static_cast<unsigned char>((width >> 24) & 0xff);
    ihdr[1] = static_cast<unsigned char>((width >> 16) & 0xff);
    ihdr[2] = static_cast<unsigned char>((width >> 8) & 0xff);
    ihdr[3] = static_cast<unsigned char>(width & 0xff);
    ihdr[4] = static_cast<unsigned char>((height >> 24) & 0xff);
    ihdr[5] = static_cast<unsigned char>((height >> 16) & 0xff);
    ihdr[6] = static_cast<unsigned char>((height >> 8) & 0xff);
    ihdr[7] = static_cast<unsigned char>(height & 0xff);
    ihdr[8] = 8;  // bit depth
    ihdr[9] = 6;  // color type: RGBA
    ihdr[10] = 0; // compression method
    ihdr[11] = 0; // filter method
    ihdr[12] = 0; // interlace method
    detail::WriteChunk(png, "IHDR", ihdr, sizeof(ihdr));
    detail::WriteChunk(png, "IDAT", zlib_stream.data(), zlib_stream.size());
    detail::WriteChunk(png, "IEND", nullptr, 0);

    std::FILE* file = std::fopen(path.c_str(), "wb");
    if (!file) return false;
    const std::size_t written = std::fwrite(png.data(), 1, png.size(), file);
    std::fclose(file);
    return written == png.size();
}

} // namespace ScreenshotTool
