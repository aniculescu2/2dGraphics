#include "include/GFinal.h"
#include "include/GBitmap.h"
#include "GTools.h"

class Final : public GFinal {
public:
    Final() {}

    /**
     *  Return a radial-gradient shader.
     *
     *  This is a shader defined by a circle with center and a radius.
     *  The array of colors are evenly distributed between the center (color[0]) out to
     *  the radius (color[count-1]). Beyond the radius, it respects the TileMode.
     */
    std::unique_ptr<GShader> createRadialGradient(GPoint center, float radius,
                                                          const GColor colors[], int count,
                                                          GShader::TileMode mode) {
        class GRadialGradientShader : public GShader {
            public:
                GRadialGradientShader (GPoint center, float radius, const GColor colors[], int count, GShader::TileMode mode) : fCenter(center), fRadius(radius), fCount(count), fMode(mode) {
                    fColors = new GColor[count+1];
                    fColors[0] = colors[0];
                    fColors[0].pinToUnit();
                    // fColors[1] = fColors[0];
                    for(int i = 1; i < count; ++i)
                        fColors[i] = colors[i];
                    fColors[count] = fColors[count-1];
                }

                bool isOpaque(){
                    for(int i = 0; i < fCount; ++i){
                        if(fColors[i].a < 1){
                            return false;
                        }
                    }
                    return true;
                }
                
                bool setContext(const GMatrix& ctm) override {
                    bool success = ctm.invert(&fInv);
                    return success;
                }

                void shadeRow(int x, int y, int count, GPixel row[]) override {
                    GPoint pt = {x + .5f, y + .5f};
                    fInv.mapPoints(&pt, 1);
                    for(int i = 0; i < count; ++i){
                        float dx = pt.x() - fCenter.x();
                        float dy = pt.y() - fCenter.y();

                        float distance = sqrtf(dx*dx + dy*dy);

                        float t = distance / fRadius;

                        if(fMode == kRepeat){
                            if(t > 1)
                                t = t - floor(t);
                        }
                        if(fMode == kMirror){
                            if(t > 1){
                                if(GFloorToInt(t) % 2 == 1){
                                    t = t - floor(t);
                                    t = 1 - t;  
                                }
                                else{
                                    t = t - floor(t);
                                }
                            }
                        }
                        t = std::max(t, 0.0f);
                        t = std::min(t, 1.0f);
                        

                        int j = floor(t * (fCount - 1));
                        float span = 1.0f / (fCount - 1);
                        float start = j * span;
                        if(t != 0 && t != 1)
                            t = (t - start) / span;

                        GColor currentColor = t * (fColors[j+1] - fColors[j]) + fColors[j];
                        row[i] = makePixel(currentColor);
                        pt.fX += fInv[0];
                        pt.fY += fInv[3];
                    }
                }

            private:
                GMatrix fInv;
                GPoint fCenter;
                float fRadius;
                GColor* fColors;
                int fCount;
                GShader::TileMode fMode;
        };
        if(count < 1)
            return nullptr;

        return std::unique_ptr<GShader>(new GRadialGradientShader(center, radius, colors, count, mode));
    }

    /**
     * Return a bitmap shader that performs kClamp tiling with a bilinear filter on each sample, respecting the
     * localMatrix.
     *
     * This is in contrast to the existing GCreateBitmapShader, which performs "nearest neightbor" sampling
     * when it fetches a pixel from the src bitmap.
     */
    std::unique_ptr<GShader> createBilerpShader(const GBitmap& device, const GMatrix& localMatrix) {
        class BilerpShader : public GShader {
        public:
            BilerpShader(const GBitmap& device, const GMatrix& localInverse) : fDevice(device), fLocalInverse(localInverse) {}

            // Return true iff all of the GPixels that may be returned by this shader will be opaque.
            bool isOpaque() override{
                return fDevice.isOpaque();
            }

            // The draw calls in GCanvas must call this with the CTM before any calls to shadeSpan().
            bool setContext(const GMatrix& ctm) override {
                bool success = ctm.invert(&fInv);
                fInv = fLocalInverse * fInv;
                return success;
            }

            void shadeRow(int x, int y, int count, GPixel row[]) override {
                GPoint pt = {x + .5f, y + .5f};
                float maxX = fDevice.width() - 1;
                float maxY = fDevice.height() - 1;
                fInv.mapPoints(&pt, 1); 
                for(int i = 0; i < count; ++i){
                    float fx, fy;
                
                    fx = pt.x()-.5;
                    fy = pt.y()-.5;
                    fx = std::max(0.f, fx);
                    fx = std::min(fx, maxX);
                    fy = std::max(0.f, fy);
                    fy = std::min(fy, maxY);

                    float u = fx - floorf(fx);
                    float v = fy - floorf(fy);
                    GPixel* APixel = fDevice.getAddr(floorf(fx), floorf(fy));
                    GPixel* BPixel = fDevice.getAddr(ceilf(fx), floorf(fy));
                    GPixel* CPixel = fDevice.getAddr(floorf(fx), ceilf(fy));
                    GPixel* DPixel = fDevice.getAddr(ceilf(fx), ceilf(fy));

                    GColor A = GColor::RGBA(GPixel_GetR(*APixel)/255.f, GPixel_GetG(*APixel)/255.f, GPixel_GetB(*APixel)/255.f, GPixel_GetA(*APixel)/255.f);
                    GColor B = GColor::RGBA(GPixel_GetR(*BPixel)/255.f, GPixel_GetG(*BPixel)/255.f, GPixel_GetB(*BPixel)/255.f, GPixel_GetA(*BPixel)/255.f);
                    GColor C = GColor::RGBA(GPixel_GetR(*CPixel)/255.f, GPixel_GetG(*CPixel)/255.f, GPixel_GetB(*CPixel)/255.f, GPixel_GetA(*CPixel)/255.f);
                    GColor D = GColor::RGBA(GPixel_GetR(*DPixel)/255.f, GPixel_GetG(*DPixel)/255.f, GPixel_GetB(*DPixel)/255.f, GPixel_GetA(*DPixel)/255.f);

                    GColor interp = (1-u)*(1-v)*A + u*(1-v)*B + (1-u)*v*C + u*v*D;


                    row[i] = makePixel(interp);
                    pt.fX += fInv[0];
                    pt.fY += fInv[3];
                }
            }
        private:
            GBitmap fDevice;
            GMatrix fLocalInverse;
            GMatrix fInv;
        };

        return std::unique_ptr<GShader>(new BilerpShader(device, localMatrix));
    }

    /**
     *  Add contour(s) to the specified path that will draw a line from p0 to p1 with the specified
     *  width and CapType. Note that "width" is the distance from one side of the stroke to the
     *  other, ala its thickness.
     */
    void addLine(GPath* path, GPoint p0, GPoint p1, float width, CapType cap) {
        float slope = (p1.y() - p0.y()) / (p1.x() - p0.x());
        float angle = atan(-1/slope);
        float dx = width/2 * sin(angle);
        float dy = width/2 * cos(angle);

        GPoint left = (p0.x() < p1.x()) ? p0 : p1;
        GPoint right = (p0.x() < p1.x()) ? p1 : p0;

        if(slope == 0){
            dx = 0;
            dy = width/2;
        }

        if(cap == kSquare){
            float angle = atan(slope);
            float dx = width/2 * sin(angle);
            float dy = width/2 * cos(angle);
            if(slope == 0){
                dx = width/2;
                dy = 0;
            }
            left = {p0.x() - dx, p0.y() - dy};
            right = {p1.x() + dx, p1.y() + dy};
        }

        path->moveTo({left.x() + dx, left.y() + dy});
        path->lineTo({left.x() - dx, left.y() - dy});
        path->lineTo({right.x() - dx, right.y() - dy});
        path->lineTo({right.x() + dx, right.y() + dy});

        if(cap == kRound){
            float radius = width/2;
            GPath::Direction direction = GPath::kCW_Direction;
            if(slope > 0){
                if(p0.y() < p1.y())
                    direction = GPath::kCW_Direction;
                else direction = GPath::kCCW_Direction;
            }
            else{
                if(p0.y() < p1.y())
                    direction = GPath::kCCW_Direction;
                else direction = GPath::kCW_Direction;
            }
                
            path->addCircle(p0, radius, direction);
            path->addCircle(p1, radius, direction);
        }
        
    }

     /*
     *  Draw the corresponding mesh constructed from a quad with each side defined by a
     *  quadratic bezier, evaluating them to produce "level" interior lines (same convention
     *  as drawQuad().
     *
     *  pts[0]    pts[1]    pts[2]
     *  pts[7]              pts[3]
     *  pts[6]    pts[5]    pts[4]
     *
     *  Evaluate values within the mesh using the Coons Patch formulation:
     *
     *  value(u,v) = TB(u,v) + LR(u,v) - Corners(u,v)
     *
     *  Where
     *      TB is computed by first evaluating the Top and Bottom curves at (u), and then
     *      linearly interpolating those points by (v)
     *      LR is computed by first evaluating the Left and Right curves at (v), and then
     *      linearly interpolating those points by (u)
     *      Corners is computed by our standard "drawQuad" evaluation using the 4 corners 0,2,4,6
     */
    virtual void drawQuadraticCoons(GCanvas* canvas, const GPoint pts[8], const GPoint tex[4],
                                    int level, const GPaint& paint) {}
};

/**
 *  Implement this to return ain instance of your subclass of GFinal.
 */
std::unique_ptr<GFinal> GCreateFinal(){
    return std::unique_ptr<GFinal>(new Final());
}