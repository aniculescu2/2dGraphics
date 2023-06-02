#include "include/GPath.h"

void GPath::addRect(const GRect& rect, Direction dir){
    if(dir == kCW_Direction){
        moveTo(rect.fLeft, rect.fTop);
        lineTo(rect.fRight, rect.fTop);
        lineTo(rect.fRight, rect.fBottom);
        lineTo(rect.fLeft, rect.fBottom);
    }
    else{
        moveTo(rect.fLeft, rect.fTop);
        lineTo(rect.fLeft, rect.fBottom);
        lineTo(rect.fRight, rect.fBottom);
        lineTo(rect.fRight, rect.fTop);
    }
}

GRect GPath::bounds() const {
    GRect bounds = GRect::WH(0, 0);
    if(fPts.size() == 0){
        return bounds;
    }
    bounds.fLeft = fPts[0].fX;
    bounds.fTop = fPts[0].fY;
    bounds.fRight = fPts[0].fX;
    bounds.fBottom = fPts[0].fY;
    for(int i = 1; i < fPts.size(); i++){
        if(fPts[i].fX < bounds.fLeft){
            bounds.fLeft = fPts[i].fX;
        }
        if(fPts[i].fX > bounds.fRight){
            bounds.fRight = fPts[i].fX;
        }
        if(fPts[i].fY < bounds.fTop){
            bounds.fTop = fPts[i].fY;
        }
        if(fPts[i].fY > bounds.fBottom){
            bounds.fBottom = fPts[i].fY;
        }
    }
    return bounds;
}

void GPath::transform(const GMatrix& matrix){
    for(int i = 0; i < fPts.size(); ++i)
        matrix.mapPoints(&fPts[i], 1);
}

void GPath::addPolygon(const GPoint pts[], int count){
    if(count < 3){
        return;
    }
    moveTo(pts[0]);
    for(int i = 1; i < count; i++){
        lineTo(pts[i]);
    }
}

void GPath::addCircle(GPoint center, float radius, GPath::Direction d){
    const float a = 1.00005519;
    const float b = 0.55342686;
    const float c = 0.99873585;

    GPoint points[13];
    points[0] = {0, a};
    points[1] = {b, c};
    points[2] = {c, b};
    points[3] = {a, 0};
    points[4] = {c, -b};
    points[5] = {b, -c};
    points[6] = {0, -a};
    points[7] = {-b, -c};
    points[8] = {-c, -b};
    points[9] = {-a, 0};
    points[10] = {-c, b};
    points[11] = {-b, c};
    points[12] = {0, a};
    
    GMatrix transform = GMatrix::Translate(center.fX, center.fY) * GMatrix::Scale(radius, radius);
    transform.mapPoints(points, 13);

    moveTo(points[0]);
    if(d == kCCW_Direction){ //WHYYYY??????
        for(int i = 1; i < 13; i+=3)
            cubicTo(points[i], points[i+1], points[i+2]);
    }
    else{
        for(int i = 11; i > 0; i-=3)
            cubicTo(points[i], points[i-1], points[i-2]);
    }
}


void GPath::ChopQuadAt(const GPoint* src, GPoint* dst, float t){
    GPoint A = src[0];
    GPoint B = src[1];
    GPoint C = src[2];
    GPoint AB = (1 - t) * A + t * B;
    GPoint BC = (1 - t) * B + t * C;
    GPoint ABC = (1 - t) * AB + t * BC;
    dst[0] = A;
    dst[1] = AB;
    dst[2] = ABC;
    dst[3] = BC;
    dst[4] = C;
}

void GPath::ChopCubicAt(const GPoint* src, GPoint* dst, float t){
    GPoint A = src[0];
    GPoint B = src[1];
    GPoint C = src[2];
    GPoint D = src[3];
    GPoint AB = (1 - t) * A + t * B;
    GPoint BC = (1 - t) * B + t * C;
    GPoint CD = (1 - t) * C + t * D;
    GPoint ABC = (1 - t) * AB + t * BC;
    GPoint BCD = (1 - t) * BC + t * CD;
    GPoint ABCD = (1 - t) * ABC + t * BCD;
    dst[0] = A;
    dst[1] = AB;
    dst[2] = ABC;
    dst[3] = ABCD;
    dst[4] = BCD;
    dst[5] = CD;
    dst[6] = D;

}
