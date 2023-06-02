#include "include/GShader.h"
#include "include/GMatrix.h"

class ProxyShader : public GShader {
public:
    ProxyShader(GShader* shader, const GMatrix& extraTransform)
        : fRealShader(shader), fExtraTransform(extraTransform) {}

    bool isOpaque() override { return fRealShader->isOpaque(); }

    bool setContext(const GMatrix& ctm) override {
        return fRealShader->setContext(ctm * fExtraTransform);
    }
    
    void shadeRow(int x, int y, int count, GPixel row[]) override {
        fRealShader->shadeRow(x, y, count, row);
    }

private:
    GShader* fRealShader;
    GMatrix  fExtraTransform;
};