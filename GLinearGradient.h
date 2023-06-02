#include "include/GShader.h"
#include "include/GMatrix.h"
#include "GTools.h"

class GLinearGradient : public GShader{
public:
    GLinearGradient(GPoint p0, GPoint p1, const GColor colors[], int count, TileMode tile) : fCount(count), fTileMode(tile){
        fColors = new GColor[count+2];
        fColors[0] = colors[0];
        fColors[0].pinToUnit();
        fColors[1] = fColors[0];
        for(int i = 2; i <= count; ++i)
            fColors[i] = colors[i-1];
        fColors[count+1] = fColors[count];
      
        GPoint e0 = p1 - p0;
        GMatrix(e0.x(), -e0.y(), p0.x(), e0.y(), e0.x(), p0.y()).invert(&fBasis); 
    }

    bool isOpaque() override{
        return true;
    }

    bool setContext(const GMatrix& ctm) override {
        bool success = ctm.invert(&fInv);
        fInv = fBasis * fInv;
        weight = fInv[0] * (fCount - 1);
        return success;
    }

    void shadeRow(int x, int y, int count, GPixel row[]) override {
        GPoint pt = {x + .5, y + .5};
        pt = fInv * pt;
        float t = pt.x() * (fCount - 1) + 1;

        for(int i = 0; i < count; ++i){
            t = std::min(t, (float)fCount);
            int j = (int)t; //floor to int
            float nextWeight = t - j;

            GColor currentColor = nextWeight * (fColors[j+1] - fColors[j]) + fColors[j];
            row[i] = makePixel(currentColor);
            t += weight;
        }
    }

private:
    GColor* fColors;
    int fCount;
    GMatrix fInv;
    GMatrix fBasis;
    float weight;
    TileMode fTileMode;
};

class GColorShader : public GShader{
public:
    GColorShader(const GColor colors[]){
        fPixel = makePixel(colors[0].pinToUnit());
    }

    bool isOpaque() override{
        return true;
    }

    bool setContext(const GMatrix& ctm) override {
        return true;
    }

    void shadeRow(int x, int y, int count, GPixel row[]) override {
        for(int i = 0; i < count; ++i){
            row[i] = fPixel;
        }
    }
private:
    GPixel fPixel;
};

class GLinearGradientDouble : public GShader{
public:
    GLinearGradientDouble(GPoint p0, GPoint p1, const GColor colors[], TileMode tile) : color1(colors[0]), color2(colors[1]), fTileMode(tile) {
        color1.pinToUnit();
        color2.pinToUnit();
        GPoint e0 = p1 - p0;
        GMatrix(e0.x(), -e0.y(), p0.x(), e0.y(), e0.x(), p0.y()).invert(&fBasis); 
    }

    bool isOpaque() override{
        if(color1.a == 1 && color2.a == 1)
            return true;
        return false;
    }

    bool setContext(const GMatrix& ctm) override {
        bool success = ctm.invert(&fInv);
        fInv = fBasis * fInv;
        delta = fInv[0] * (color2 - color1);
        return success;
    }

    void shadeRow(int x, int y, int count, GPixel row[]) override {
        GPoint pt = GPoint{x + .5, y + .5};
        pt = fInv * pt;  

        if(fTileMode == kRepeat){
            for(int i = 0; i < count; ++i){
                float t = pt.x() - (int)pt.x();
                GColor currentColor = t * (color2 - color1) + color1;
                row[i] = makePixel(currentColor);
                pt.fX += fInv[0];
            }
        }   
        else{
            pt.fX = std::max(0.f, std::min(1.f, pt.fX));
            GColor currentColor = color1 + pt.x() * (color2 - color1);

            for(int i = 0; i < count; ++i){      
                row[i] = makePixel(currentColor);
                currentColor += delta;
            }
        }
    }
private:
    GColor color1;
    GColor color2;
    GMatrix fInv;
    GMatrix fBasis;
    GColor delta;
    TileMode fTileMode;
};


std::unique_ptr<GShader> GCreateLinearGradient(GPoint p0, GPoint p1, const GColor colors[], int count, GShader::TileMode tile){
    if(count < 1)
        return nullptr;
    
    if(tile == GShader::kRepeat || tile == GShader::kClamp){
        if(count == 1)
            return std::unique_ptr<GShader>(new GColorShader(colors));
        if(count == 2)
            return std::unique_ptr<GShader>(new GLinearGradientDouble(p0, p1, colors, tile));
    }
    else if(tile == GShader::kMirror){
        GColor newColors[2*count - 1];
        for(int i = 0; i < count; ++i)
            newColors[i] = colors[i];
        for(int i = 0; i < count - 1; ++i)
            newColors[count + i] = colors[count - 2 - i];
        GPoint newP1 = p0 + 2 * (p1 - p0);
        return std::unique_ptr<GShader>(new GLinearGradient(p0, newP1, newColors, 2*count-1, tile));
    }

    return std::unique_ptr<GShader>(new GLinearGradient(p0, p1, colors, count, tile));


}