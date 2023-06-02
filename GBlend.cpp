#include "GBlend.h"
#include "GTools.h"

inline GPixel(kClear)(const GPixel& src, GPixel& dst){
    return 0;
}
inline GPixel(kSrc)(const GPixel& src, GPixel& dst){
    return src;
}
inline GPixel(kDst)(const GPixel& src, GPixel& dst){
    return dst;
}
inline GPixel(kSrcOver)(const GPixel& src, GPixel& dst){
    if(GPixel_GetA(dst) == 0)
        return src;
    
    unsigned srcMinus = 255 - GPixel_GetA(src);
    GPixel dstPixel = GPixel_PackARGB(
        div255(GPixel_GetA(dst) * srcMinus),
        div255(GPixel_GetR(dst) * srcMinus),
        div255(GPixel_GetG(dst) * srcMinus),
        div255(GPixel_GetB(dst) * srcMinus)
    );
    return src + dstPixel;
}
inline GPixel(kDstOver)(const GPixel& src, GPixel& dst){ 
    unsigned alpha = GPixel_GetA(dst);
    if(alpha == 0)
        return src;
    if(alpha == 255)
        return dst;
    unsigned dstMinus = 255 - alpha;
    GPixel srcPixel = GPixel_PackARGB(
        div255(GPixel_GetA(src) * dstMinus),
        div255(GPixel_GetR(src) * dstMinus),
        div255(GPixel_GetG(src) * dstMinus),
        div255(GPixel_GetB(src) * dstMinus)
    );
    return dst + srcPixel;
}
inline GPixel(kSrcIn)(const GPixel& src, GPixel& dst){
    unsigned alpha = GPixel_GetA(dst);
    if (alpha == 0)
            return 0;
    if(alpha == 255)
            return src;
    return GPixel_PackARGB(
        div255(GPixel_GetA(src) * alpha),
        div255(GPixel_GetR(src) * alpha),
        div255(GPixel_GetG(src) * alpha),
        div255(GPixel_GetB(src) * alpha)
    );
}
inline GPixel(kDstIn)(const GPixel& src, GPixel& dst){
    unsigned alpha = GPixel_GetA(src);
    if(GPixel_GetA(dst) == 0)
        return 0;   
    return GPixel_PackARGB(
        div255(GPixel_GetA(dst) * alpha),
        div255(GPixel_GetR(dst) * alpha),
        div255(GPixel_GetG(dst) * alpha),
        div255(GPixel_GetB(dst) * alpha)
    );
}
inline GPixel(kSrcOut)(const GPixel& src, GPixel& dst){
    if(GPixel_GetA(dst) == 0)
        return src;
    if(GPixel_GetA(dst) == 255)
        return 0;
    unsigned dstMinus = 255 - GPixel_GetA(dst);
    return GPixel_PackARGB(
        div255(GPixel_GetA(src) * dstMinus),
        div255(GPixel_GetR(src) * dstMinus),
        div255(GPixel_GetG(src) * dstMinus),
        div255(GPixel_GetB(src) * dstMinus)
    );
}
inline GPixel(kDstOut)(const GPixel& src, GPixel& dst){
    if(GPixel_GetA(dst) == 0)
        return 0;
    
    unsigned srcMinus = 255 - GPixel_GetA(src);
    return GPixel_PackARGB(
        div255(GPixel_GetA(dst) * srcMinus),
        div255(GPixel_GetR(dst) * srcMinus),
        div255(GPixel_GetG(dst) * srcMinus),
        div255(GPixel_GetB(dst) * srcMinus)
    );
}
inline GPixel(kSrcATop)(const GPixel& src, GPixel& dst){
    unsigned alpha = GPixel_GetA(dst);
    if(alpha == 0)
        return kClear(src, dst);
    // if(alpha == 255)
    //     return kSrcIn(src, dst);
    //if a = 0, return dstout
    //if a = 255, return srcin

    unsigned srcMinus = 255 - GPixel_GetA(src);
    
    return GPixel_PackARGB(
        alpha,
        div255((alpha * GPixel_GetR(src)) + (GPixel_GetR(dst) * srcMinus)),
        div255((alpha * GPixel_GetG(src)) + (GPixel_GetG(dst) * srcMinus)),
        div255((alpha * GPixel_GetB(src)) + (GPixel_GetB(dst) * srcMinus))
    );
}

inline GPixel(kDstATop)(const GPixel& src, GPixel& dst){
    //if a = 0, return srcout
    //if a = 255, return dstin

    unsigned dstMinus = 255 - GPixel_GetA(dst);
    // if(dstMinus == 255) return kSrcOut(src, dst);
    // if(dstMinus == 0) return kDstIn(src, dst);
    unsigned alpha = GPixel_GetA(src);
    return GPixel_PackARGB(
        alpha,
        div255((alpha * GPixel_GetR(dst)) + (GPixel_GetR(src) * dstMinus)),
        div255((alpha * GPixel_GetG(dst)) + (GPixel_GetG(src) * dstMinus)),
        div255((alpha * GPixel_GetB(dst)) + (GPixel_GetB(src) * dstMinus))
    );
}

inline GPixel(kXor)(const GPixel& src, GPixel& dst){
    unsigned minusDst = 255 - GPixel_GetA(dst);
    unsigned minusSrc = 255 - GPixel_GetA(src);
    return GPixel_PackARGB(
        div255((GPixel_GetA(src) * minusDst) + (GPixel_GetA(dst) * minusSrc)),
        div255((GPixel_GetR(src) * minusDst) + (GPixel_GetR(dst) * minusSrc)),
        div255((GPixel_GetG(src) * minusDst) + (GPixel_GetG(dst) * minusSrc)),
        div255((GPixel_GetB(src) * minusDst) + (GPixel_GetB(dst) * minusSrc))
    );
}

BlendProc changeBlend(GPixel source, GBlendMode mode){
    int alpha = GPixel_GetA(source);
    if(alpha == 0)
        return noColor[static_cast<int>(mode)];
    if(alpha == 255)
        return opaqueColor[static_cast<int>(mode)];
    else
        return color[static_cast<int>(mode)];

    //make list and use blendmode as index
    //make second list for alpha==0
    //make third list for alpha==255
}

void blit(const GPixel* src, GBitmap canvas, int top, int bottom, int left, int right, BlendProc proc) {
    for(int y = top; y < bottom; ++y)
        for(int x = left; x < right; ++x){
            GPixel* dst = canvas.getAddr(x, y);
            *dst = proc(*src, *dst);
        }
}
