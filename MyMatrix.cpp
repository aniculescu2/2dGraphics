#include "include/GMatrix.h"

GMatrix::GMatrix() {
        fMat[0] = fMat[4] = 1;
        fMat[1] = fMat[2] = fMat[3] = fMat[5] = 0;
    }  // initialize to identity

GMatrix GMatrix::Translate(float tx, float ty){
    GMatrix m;
    m.fMat[2] = tx;
    m.fMat[5] = ty;
    return m;
}
GMatrix GMatrix::Scale(float sx, float sy){
    GMatrix m;
    m.fMat[0] = sx;
    m.fMat[4] = sy;
    return m;
}
GMatrix GMatrix::Rotate(float radians){
    GMatrix m;
    m.fMat[0] = cos(radians);
    m.fMat[1] = -sin(radians);
    m.fMat[3] = sin(radians);
    m.fMat[4] = cos(radians);
    return m;
}

    /**
     *  Return the product of two matrices: a * b
     */
GMatrix GMatrix::Concat(const GMatrix& a, const GMatrix& b){
    GMatrix m;
    m.fMat[0] = a.fMat[0] * b.fMat[0] + a.fMat[1] * b.fMat[3];
    m.fMat[1] = a.fMat[0] * b.fMat[1] + a.fMat[1] * b.fMat[4];
    m.fMat[2] = a.fMat[0] * b.fMat[2] + a.fMat[1] * b.fMat[5] + a.fMat[2];
    m.fMat[3] = a.fMat[3] * b.fMat[0] + a.fMat[4] * b.fMat[3];
    m.fMat[4] = a.fMat[3] * b.fMat[1] + a.fMat[4] * b.fMat[4];
    m.fMat[5] = a.fMat[3] * b.fMat[2] + a.fMat[4] * b.fMat[5] + a.fMat[5];
    return m;
}

    /*
     *  Compute the inverse of this matrix, and store it in the "inverse" parameter, being
     *  careful to handle the case where 'inverse' might alias this matrix.
     *
     *  If this matrix is invertible, return true. If not, return false, and ignore the
     *  'inverse' parameter.
     */
bool GMatrix::invert(GMatrix* inverse) const{
    GMatrix m;
    float det = fMat[0] * fMat[4] - fMat[1] * fMat[3];
    if (det == 0) {
        return false;
    }
    float invDet = 1 / det;
    m.fMat[0] = fMat[4] * invDet;
    m.fMat[1] = -fMat[1] * invDet;
    m.fMat[2] = (fMat[1] * fMat[5] - fMat[2] * fMat[4]) * invDet;
    m.fMat[3] = -fMat[3] * invDet;
    m.fMat[4] = fMat[0] * invDet;
    m.fMat[5] = (fMat[3] * fMat[2] - fMat[0] * fMat[5]) * invDet;
    *inverse = m;
    return true;
}

    /**
     *  Transform the set of points in src, storing the resulting points in dst, by applying this
     *  matrix. It is the caller's responsibility to allocate dst to be at least as large as src.
     *
     *  [ a  b  c ] [ x ]     x' = ax + by + c
     *  [ d  e  f ] [ y ]     y' = dx + ey + f
     *  [ 0  0  1 ] [ 1 ]
     *
     *  Note: It is legal for src and dst to point to the same memory (however, they may not
     *  partially overlap). Thus the following is supported.
     *
     *  GPoint pts[] = { ... };
     *  matrix.mapPoints(pts, pts, count);
     */
void GMatrix::mapPoints(GPoint dst[], const GPoint src[], int count) const{
    for(int i = 0; i < count; ++i){
        GPoint p = src[i];
        dst[i].fX = fMat[0] * p.x() + fMat[1] * p.y() + fMat[2];
        dst[i].fY = fMat[3] * p.x() + fMat[4] * p.y() + fMat[5];
    }
}