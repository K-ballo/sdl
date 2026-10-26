// Eggs.SDL
//
// Copyright (c) 2026 Agustin Berge
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <SDL3/SDL_version.h>

// One row per SDL pixel format: X(name, SDL value). The endian-dependent
// aliases such as rgba32 are rows too. The rows are split by the SDL release
// that introduced them, those of a newer release than the one compiled against
// are left out.
#define EGGS_SDL_PIXEL_FORMATS_3_2(X)               \
    X(unknown, SDL_PIXELFORMAT_UNKNOWN)             \
    X(index1lsb, SDL_PIXELFORMAT_INDEX1LSB)         \
    X(index1msb, SDL_PIXELFORMAT_INDEX1MSB)         \
    X(index2lsb, SDL_PIXELFORMAT_INDEX2LSB)         \
    X(index2msb, SDL_PIXELFORMAT_INDEX2MSB)         \
    X(index4lsb, SDL_PIXELFORMAT_INDEX4LSB)         \
    X(index4msb, SDL_PIXELFORMAT_INDEX4MSB)         \
    X(index8, SDL_PIXELFORMAT_INDEX8)               \
    X(rgb332, SDL_PIXELFORMAT_RGB332)               \
    X(xrgb4444, SDL_PIXELFORMAT_XRGB4444)           \
    X(xbgr4444, SDL_PIXELFORMAT_XBGR4444)           \
    X(xrgb1555, SDL_PIXELFORMAT_XRGB1555)           \
    X(xbgr1555, SDL_PIXELFORMAT_XBGR1555)           \
    X(argb4444, SDL_PIXELFORMAT_ARGB4444)           \
    X(rgba4444, SDL_PIXELFORMAT_RGBA4444)           \
    X(abgr4444, SDL_PIXELFORMAT_ABGR4444)           \
    X(bgra4444, SDL_PIXELFORMAT_BGRA4444)           \
    X(argb1555, SDL_PIXELFORMAT_ARGB1555)           \
    X(rgba5551, SDL_PIXELFORMAT_RGBA5551)           \
    X(abgr1555, SDL_PIXELFORMAT_ABGR1555)           \
    X(bgra5551, SDL_PIXELFORMAT_BGRA5551)           \
    X(rgb565, SDL_PIXELFORMAT_RGB565)               \
    X(bgr565, SDL_PIXELFORMAT_BGR565)               \
    X(rgb24, SDL_PIXELFORMAT_RGB24)                 \
    X(bgr24, SDL_PIXELFORMAT_BGR24)                 \
    X(xrgb8888, SDL_PIXELFORMAT_XRGB8888)           \
    X(rgbx8888, SDL_PIXELFORMAT_RGBX8888)           \
    X(xbgr8888, SDL_PIXELFORMAT_XBGR8888)           \
    X(bgrx8888, SDL_PIXELFORMAT_BGRX8888)           \
    X(argb8888, SDL_PIXELFORMAT_ARGB8888)           \
    X(rgba8888, SDL_PIXELFORMAT_RGBA8888)           \
    X(abgr8888, SDL_PIXELFORMAT_ABGR8888)           \
    X(bgra8888, SDL_PIXELFORMAT_BGRA8888)           \
    X(xrgb2101010, SDL_PIXELFORMAT_XRGB2101010)     \
    X(xbgr2101010, SDL_PIXELFORMAT_XBGR2101010)     \
    X(argb2101010, SDL_PIXELFORMAT_ARGB2101010)     \
    X(abgr2101010, SDL_PIXELFORMAT_ABGR2101010)     \
    X(rgb48, SDL_PIXELFORMAT_RGB48)                 \
    X(bgr48, SDL_PIXELFORMAT_BGR48)                 \
    X(rgba64, SDL_PIXELFORMAT_RGBA64)               \
    X(argb64, SDL_PIXELFORMAT_ARGB64)               \
    X(bgra64, SDL_PIXELFORMAT_BGRA64)               \
    X(abgr64, SDL_PIXELFORMAT_ABGR64)               \
    X(rgb48_float, SDL_PIXELFORMAT_RGB48_FLOAT)     \
    X(bgr48_float, SDL_PIXELFORMAT_BGR48_FLOAT)     \
    X(rgba64_float, SDL_PIXELFORMAT_RGBA64_FLOAT)   \
    X(argb64_float, SDL_PIXELFORMAT_ARGB64_FLOAT)   \
    X(bgra64_float, SDL_PIXELFORMAT_BGRA64_FLOAT)   \
    X(abgr64_float, SDL_PIXELFORMAT_ABGR64_FLOAT)   \
    X(rgb96_float, SDL_PIXELFORMAT_RGB96_FLOAT)     \
    X(bgr96_float, SDL_PIXELFORMAT_BGR96_FLOAT)     \
    X(rgba128_float, SDL_PIXELFORMAT_RGBA128_FLOAT) \
    X(argb128_float, SDL_PIXELFORMAT_ARGB128_FLOAT) \
    X(bgra128_float, SDL_PIXELFORMAT_BGRA128_FLOAT) \
    X(abgr128_float, SDL_PIXELFORMAT_ABGR128_FLOAT) \
    X(yv12, SDL_PIXELFORMAT_YV12)                   \
    X(iyuv, SDL_PIXELFORMAT_IYUV)                   \
    X(yuy2, SDL_PIXELFORMAT_YUY2)                   \
    X(uyvy, SDL_PIXELFORMAT_UYVY)                   \
    X(yvyu, SDL_PIXELFORMAT_YVYU)                   \
    X(nv12, SDL_PIXELFORMAT_NV12)                   \
    X(nv21, SDL_PIXELFORMAT_NV21)                   \
    X(p010, SDL_PIXELFORMAT_P010)                   \
    X(external_oes, SDL_PIXELFORMAT_EXTERNAL_OES)   \
    X(rgba32, SDL_PIXELFORMAT_RGBA32)               \
    X(argb32, SDL_PIXELFORMAT_ARGB32)               \
    X(bgra32, SDL_PIXELFORMAT_BGRA32)               \
    X(abgr32, SDL_PIXELFORMAT_ABGR32)               \
    X(rgbx32, SDL_PIXELFORMAT_RGBX32)               \
    X(xrgb32, SDL_PIXELFORMAT_XRGB32)               \
    X(bgrx32, SDL_PIXELFORMAT_BGRX32)               \
    X(xbgr32, SDL_PIXELFORMAT_XBGR32)

#define EGGS_SDL_PIXEL_FORMATS_3_4(X) X(mjpg, SDL_PIXELFORMAT_MJPG)

#if SDL_VERSION_ATLEAST(3, 4, 0)
#    define EGGS_SDL_PIXEL_FORMATS(X) \
        EGGS_SDL_PIXEL_FORMATS_3_2(X) EGGS_SDL_PIXEL_FORMATS_3_4(X)
#else
#    define EGGS_SDL_PIXEL_FORMATS(X) EGGS_SDL_PIXEL_FORMATS_3_2(X)
#endif
