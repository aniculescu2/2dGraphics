#include "include/GShader.h"
#include "include/GMatrix.h"
#include "include/GBitmap.h"

class MyShader : public GShader{
public:
    MyShader(const GBitmap& device, const GMatrix& localInverse, GShader::TileMode mode) : fDevice(device), fLocalInverse(localInverse), fMode(mode) {}

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

    /**
     *  Given a row of pixels in device space [x, y] ... [x + count - 1, y], return the
     *  corresponding src pixels in row[0...count - 1]. The caller must ensure that row[]
     *  can hold at least [count] entries.
     */
    void shadeRow(int x, int y, int count, GPixel row[]) override {
        GPoint pt = {x + .5f, y + .5f};
        GMatrix tileMatrix = GMatrix(1.0f/fDevice.width(), 0, 0, 0, 1.0f/fDevice.height(), 0) * fInv;

        if(fMode == kRepeat){
            tileMatrix.mapPoints(&pt, 1);
            for(int i = 0; i < count; ++i){
                int x1 = (int) ((pt.x() - floor(pt.x())) * fDevice.width());
                int y1 = (int) ((pt.y() - floor(pt.y())) * fDevice.height());

                GPixel* pixel = fDevice.getAddr(x1, y1);
                row[i] = *pixel;
                pt.fX += tileMatrix[0];
                pt.fY += tileMatrix[3];
            }
        }
        else if(fMode == kMirror){
            tileMatrix.mapPoints(&pt, 1);
            for(int i = 0; i < count; ++i){
                float x1 = pt.x() * .5f;
                x1 = x1 - floorf(x1);
                float y1 = pt.y() * .5f;
                y1 = y1 - floorf(y1);

                if(x1 > .5f)
                    x1 = 2*(1.f - x1);
                else
                    x1 = 2*x1;
                if(y1 > .5f)
                    y1 = 2*(1.f - y1);
                else
                    y1 = 2*y1;

                int newX1 = (int) (x1 * fDevice.width());
                int newY1 = (int) (y1 * fDevice.height());

                GPixel* pixel = fDevice.getAddr(newX1, newY1);
                row[i] = *pixel;
                pt.fX += tileMatrix[0];
                pt.fY += tileMatrix[3];

            }
        }
        else{
            float maxX = fDevice.width() - 1;
            float maxY = fDevice.height() - 1;

            fInv.mapPoints(&pt, 1); //finv * pt
        
            if(fInv[3] == 0){
                int roundY = (int)std::min(maxY, std::max(floorf(pt.y()), 0.f));
                for(int i = 0; i < count; ++i){
                int roundX;
                float fx;
                
                fx = floorf(pt.x());
                fx = std::max(0.f, fx);
                fx = std::min(fx, maxX);

                roundX = (int)fx;
                    
                GPixel* pixel = fDevice.getAddr(roundX, roundY);
                row[i] = *pixel;
                pt.fX += fInv[0];
                }
                return;
            }
            
            for(int i = 0; i < count; ++i){
                
                int roundX, roundY;
                float fx, fy;
                
                fx = floorf(pt.x());
                fy = floorf(pt.y());
                fx = std::max(0.f, fx);
                fx = std::min(fx, maxX);
                fy = std::max(0.f, fy);
                fy = std::min(fy, maxY);

                roundX = (int)fx;
                roundY = (int)fy;
                
                GPixel* pixel = fDevice.getAddr(roundX, roundY);
                row[i] = *pixel;
                pt.fX += fInv[0];
                pt.fY += fInv[3];
            }
        }
    }

private:
    GBitmap fDevice;
    GMatrix fLocalInverse;
    GMatrix fInv;
    GShader::TileMode fMode;
};

/**
 *  Return a subclass of GShader that draws the specified bitmap and the local inverse.
 *  Returns null if the either parameter is invalid.
 */
std::unique_ptr<GShader> GCreateBitmapShader(const GBitmap& bitmap, const GMatrix& localInverse, GShader::TileMode tile){
    if(bitmap.width() <= 0 || bitmap.height() <= 0 || !bitmap.pixels())
        return nullptr;
    return std::unique_ptr<GShader>(new MyShader(bitmap, localInverse, tile));
}