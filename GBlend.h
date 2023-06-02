#include "include/GPixel.h"
#include "include/GBlendMode.h"
#include "include/GBitmap.h"

typedef GPixel (*BlendProc)(const GPixel& src, GPixel& dst);
BlendProc changeBlend(GPixel source, GBlendMode mode);
void blit(const GPixel* src, GBitmap canvas, int top, int bottom, int left, int right, BlendProc proc);

GPixel(kClear)(const GPixel& src, GPixel& dst);
GPixel(kSrc)(const GPixel& src, GPixel& dst);
GPixel(kDst)(const GPixel& src, GPixel& dst);
GPixel(kSrcOver)(const GPixel& src, GPixel& dst);
GPixel(kDstOver)(const GPixel& src, GPixel& dst);
GPixel(kSrcIn)(const GPixel& src, GPixel& dst);
GPixel(kDstIn)(const GPixel& src, GPixel& dst);
GPixel(kSrcOut)(const GPixel& src, GPixel& dst);
GPixel(kDstOut)(const GPixel& src, GPixel& dst);
GPixel(kSrcATop)(const GPixel& src, GPixel& dst);
GPixel(kDstATop)(const GPixel& src, GPixel& dst);
GPixel(kXor)(const GPixel& src, GPixel& dst);

static const BlendProc noColor[] = {
    kClear,
    kClear,
    kDst,
    kDst,
    kDst,
    kClear,
    kClear,
    kClear,
    kDst,
    kDst,
    kClear,
    kDst
};

static const BlendProc opaqueColor[12] = {
    kClear,
    kSrc,
    kDst,
    kSrc,
    kDstOver,
    kSrcIn,
    kDst,
    kSrcOut,
    kClear,
    kSrcIn,
    kDstATop,
    kSrcOut
};

static const BlendProc color[12] = {
    kClear,
    kSrc,
    kDst,
    kSrcOver,
    kDstOver,
    kSrcIn,
    kDstIn,
    kSrcOut,
    kDstOut,
    kSrcATop,
    kDstATop,
    kXor
};
