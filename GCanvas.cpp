#include <stack>

#include "include/GCanvas.h"
#include "include/GShader.h"
#include "include/GPath.h"
#include "include/GFinal.h"

#include "GBlend.h"
#include "GEdge.h"
#include "GLinearGradient.h"
#include "ProxyShader.h"


class MyCanvas : public GCanvas {
public:
    MyCanvas(const GBitmap& device) : fDevice(device) {fCTM = std::stack<GMatrix>(); fCTM.push(GMatrix());}
    
    void save(){
        fCTM.push(fCTM.top());
    }

    void restore(){
        fCTM.pop();
    }

    void concat(const GMatrix& matrix){
        fCTM.top() = fCTM.top() * matrix;
    }

    void drawPaint(const GPaint& source) override{
        int width = fDevice.width();
        int height = fDevice.height();

        if(source.getShader()){
            drawRect(GRect::WH(width, height), source);
            return;
        }

        GPixel srcPixel = makePixel(source.getColor());
        BlendProc ptr = changeBlend(srcPixel, source.getBlendMode());
        if(ptr == kDst)
            return;

        blit(&srcPixel, fDevice, 0, height, 0, width, ptr);
        
    }

    void drawRect(const GRect& rect, const GPaint& source) override{
        GIRect intRect = rect.round();
        int width = fDevice.width();
        int height = fDevice.height();

        int32_t left = intRect.x();
        int32_t top = intRect.y();
        int32_t right = intRect.right();
        int32_t bottom = intRect.bottom();

        BlendProc ptr;
        GPixel srcPixel = makePixel(source.getColor());
        GShader* shader = source.getShader();

        //transform points
        GPoint points[4] = {{left, top}, {right, top}, {right, bottom}, {left, bottom}};
        GPoint tPoints[4];
        fCTM.top().mapPoints(tPoints, points, 4);

        //if rotated, draw polygon
        if(tPoints[0].y() != tPoints[1].y()){
            drawConvexPolygon(points, 4, source);
            return;
        }
        else{ //else, draw rect
            left = (int)std::max(0.f, tPoints[0].x());
            top = (int)std::max(0.f, tPoints[0].y());
            right = (int)std::min((float)width, tPoints[2].x());
            bottom = (int)std::min((float)height, tPoints[2].y());

            if(shader != nullptr){ //if shader, shade row
                //check is opaque
                shader->setContext(fCTM.top());
                GPixel* row = new GPixel[right-left];
                if(shader->isOpaque())
                {
                    ptr = opaqueColor[static_cast<int>(source.getBlendMode())];
                    if(ptr == kDst)
                            return;

                    for(int y = top; y < bottom; ++y){
                        shader->shadeRow(left, y, right-left, row);
                            for(int x = left; x < right; ++x){
                                GPixel* dst = fDevice.getAddr(x, y);
                                *dst = ptr(row[x-left], *dst);
                            }
                    }
                    return;
                }
                for(int y = top; y < bottom; ++y){
                    shader->shadeRow(left, y, right-left, row);
                    for(int x = left; x < right; ++x){
                        ptr = changeBlend(row[x-left], source.getBlendMode());
                        if(ptr == kDst)
                            continue;
                        GPixel* dst = fDevice.getAddr(x, y);
                        *dst = ptr(row[x-left], *dst);
                    }
                }
                delete[] row;
                return;
            }
            // else, blit rect
            ptr = changeBlend(srcPixel, source.getBlendMode());
            if(ptr == kDst)
                return;
            blit(&srcPixel, fDevice, top, bottom, left, right, ptr);
        }
    }

    void drawConvexPolygon(const GPoint points[], int count, const GPaint& source) override{
        if(count < 3) return;
        std::vector<edge*> edges;
        edges.reserve(count);
        GRect bounds = {0, 0, fDevice.width(), fDevice.height()};
        GPoint tPoints[count];
        GMatrix topMatrix = fCTM.top();
        topMatrix.mapPoints(tPoints, points, count);
        
        //clipping creates edges, call clipper for each pair of points
        for(int i = 0; i < count-1; ++i){
            clip(tPoints[i], tPoints[i+1], bounds, edges);
        }
        clip(tPoints[count-1], tPoints[0], bounds, edges);
        if(edges.size() < 2) return;

        //sort edges
        std::sort(edges.begin(), edges.end(), edge_sorter);

        GShader *shader = source.getShader();

        int L, R;
        float x0, x1;
        BlendProc ptr;
        if(shader == nullptr){
            GPixel srcPixel = makePixel(source.getColor());
            ptr = changeBlend(srcPixel, source.getBlendMode());
            if(ptr == kDst)
                return;
            //blitter loop
            
            for(int y = edges[0]->top; y < edges.back()->bottom; ++y){
                x0 = edges[0]->curX;
                x1 = edges[1]->curX;
                L = GRoundToInt(x0);
                R = GRoundToInt(x1);

                if(L > R)
                    std::swap(L, R);

                blit(&srcPixel, fDevice, y, y+1, L, R, ptr);

                edges[0]->curX += edges[0]->m;
                edges[1]->curX += edges[1]->m;
                if(edges[1]->lastY(y))
                    edges.erase(edges.begin()+1);
                if(edges[0]->lastY(y))
                    edges.erase(edges.begin());
                
                if(edges.size() == 0) break;
            }
        }
        else{
            shader->setContext(topMatrix);
            for(int y = edges[0]->top; y < edges.back()->bottom; ++y){
                x0 = edges[0]->curX;
                x1 = edges[1]->curX;
                L = GRoundToInt(x0);
                R = GRoundToInt(x1);

                if(L > R)
                    std::swap(L, R);

                GPixel* row = new GPixel[R-L];
                shader->shadeRow(L, y, R-L, row);
                if(shader->isOpaque())
                {
                    ptr = opaqueColor[static_cast<int>(source.getBlendMode())];
                    if(ptr == kDst)
                            return;
                    for(int x = L; x < R; ++x){
                        GPixel* dst = fDevice.getAddr(x, y);
                        *dst = ptr(row[x-L], *dst);
                    }   
                }
                else{
                    for(int x = L; x < R; ++x){
                        ptr = changeBlend(row[x-L], source.getBlendMode());
                        if(ptr == kDst)
                            continue;
                        GPixel* dst = fDevice.getAddr(x, y);
                        *dst = ptr(row[x-L], *dst);
                    }
                }
                delete[] row;

                edges[0]->curX += edges[0]->m;
                edges[1]->curX += edges[1]->m;
                if(edges[1]->lastY(y))
                    edges.erase(edges.begin()+1);
                if(edges[0]->lastY(y))
                    edges.erase(edges.begin());
                
                if(edges.size() == 0) break;
            }
        }
    }

    void drawPath(const GPath& path, const GPaint& source) override{
        if(path.countPoints() < 3) return;
        std::vector<edge*> edges;
        edges.reserve(path.countPoints());
        GRect bounds = {0, 0, fDevice.width(), fDevice.height()};
        GPath tPath = path;
        GMatrix topMatrix = fCTM.top();
        tPath.transform(topMatrix);
        

        GPath::Edger edger = {tPath};
        GPath::Verb edgerVerb;
        GPoint edgerPts[4];

        //clipping creates edges, call clipper for each pair of points
        while ((edgerVerb = edger.next(edgerPts)) != GPath::kDone) {
            if (edgerVerb == GPath::kLine || edgerVerb == GPath::kMove) {
                clip(edgerPts[0], edgerPts[1], bounds, edges);
            }
            if(edgerVerb == GPath::kQuad){
                //tolerance = .25
                //math found in docs
                GPoint A = edgerPts[0];
                GPoint B = edgerPts[1];
                GPoint C = edgerPts[2];
                GPoint D = (A - 2*B + C)*.25f;
                int k = GCeilToInt(sqrtf(D.length() * 4.f));
                float divisions = (float)1/k;
                float t = divisions;
                GPoint p0 = A;
                GPoint p1;
                for(int i = 0; i < k-1; ++i){
                    p1.fX = A.x() * (1-t)*(1-t) + 2*B.x()*t*(1-t) + C.x()*t*t;
                    p1.fY = A.y() * (1-t)*(1-t) + 2*B.y()*t*(1-t) + C.y()*t*t;
                    clip(p0, p1, bounds, edges);
                    p0 = p1;
                    t += divisions;
                }
                clip(p0, C, bounds, edges);
            }
            if(edgerVerb == GPath::kCubic){
                GPoint A = edgerPts[0];
                GPoint B = edgerPts[1];
                GPoint C = edgerPts[2];
                GPoint D = edgerPts[3];
                GPoint E0 = A + 2*B + C;
                GPoint E1 = B + 2*C + D;
                GPoint E;
                E.fX = std::max(fabs(E0.x()), fabs(E1.x()));
                E.fY = std::max(fabs(E0.y()), fabs(E1.y()));
                int k = GCeilToInt(sqrtf(E.length()*3.f*.25f * 4.f));
                float divisions = (float)1/k;
                float t = divisions;
                GPoint p0 = A;
                GPoint p1;
                for(int i = 0; i < k-1; ++i){
                    p1.fX = A.x() * (1-t)*(1-t)*(1-t) + 3*B.x()*t*(1-t)*(1-t) + 3*C.x()*t*t*(1-t) + D.x()*t*t*t;
                    p1.fY = A.y() * (1-t)*(1-t)*(1-t) + 3*B.y()*t*(1-t)*(1-t) + 3*C.y()*t*t*(1-t) + D.y()*t*t*t;
                    clip(p0, p1, bounds, edges);
                    p0 = p1;
                    t += divisions;
                }
                clip(p0, D, bounds, edges);
            }
        }
        if(edges.size() < 2) return;
        //sort edges
        std::sort(edges.begin(), edges.end(), edge_sorter);

        GShader *shader = source.getShader();
        BlendProc ptr;
        int y = edges[0]->top;
        int accum, i, L, R;
        if(shader == nullptr){
            GPixel srcPixel = makePixel(source.getColor());
            ptr = changeBlend(srcPixel, source.getBlendMode());
            if(ptr == kDst){
                return;
            }
            while(y < bounds.bottom()){
                accum = 0;
                i = 0;
        
                while(i < edges.size() && edges[i]->active(y)){
                    float x = edges[i]->curX;

                    if (accum == 0) L = GRoundToInt(x);
                    accum += edges[i]->winding;
                    if(accum == 0)  blit(&srcPixel, fDevice, y, y+1, L, GRoundToInt(x), ptr);

                    edges[i]->curX += edges[i]->m;
                    if(edges[i]->lastY(y)){
                        edges.erase(edges.begin()+i);
                        if(edges.size() == 0) return;
                    }
                    else ++i;
                }

                assert(accum == 0);
                ++y;

                while(i < edges.size() && edges[i]->active(y))
                    ++i;

                //resort in x
                std::sort(edges.begin(), edges.begin()+i, &edge_sorter2);
            }
        }
        else{
            shader->setContext(topMatrix);
            while(y < bounds.bottom()){
                accum = 0;
                i = 0;
        
                while(i < edges.size() && edges[i]->active(y)){
                    float x = edges[i]->curX;
                    if (accum == 0) L = GRoundToInt(x);

                    accum += edges[i]->winding;
                    if(accum == 0){
                        R = GRoundToInt(x);
                        GPixel* row = new GPixel[R-L];
                        shader->shadeRow(L, y, R-L, row);
                        if(shader->isOpaque())
                        {
                            ptr = opaqueColor[static_cast<int>(source.getBlendMode())];
                            if(ptr == kDst)
                                    return;
                            for(int x = L; x < R; ++x){
                                GPixel* dst = fDevice.getAddr(x, y);
                                *dst = ptr(row[x-L], *dst);
                            }  
                        }
                        else{
                            for(int x = L; x < R; ++x){
                                ptr = changeBlend(row[x-L], source.getBlendMode());
                                if(ptr == kDst)
                                    continue;
                                GPixel* dst = fDevice.getAddr(x, y);
                                *dst = ptr(row[x-L], *dst);
                            }
                        }
                        delete[] row;
                    }

                    edges[i]->curX += edges[i]->m;
                    if(edges[i]->lastY(y)){
                        edges.erase(edges.begin()+i);
                        if(edges.size() == 0) return;
                    }
                    else ++i;
                }

                assert(accum == 0);
                ++y;

                while(i < edges.size() && edges[i]->active(y))
                    ++i;

                //resort in x
                std::sort(edges.begin(), edges.begin()+i, &edge_sorter2);
            }
        }

    }

    void drawMesh(const GPoint verts[], const GColor colors[], const GPoint texs[], int count, const int indices[], const GPaint& source) override{
        std::vector<edge*> edges;
        edges.reserve(count);
        GRect bounds = {0, 0, fDevice.width(), fDevice.height()};
        GPoint tPoints[count*3];
        GMatrix topMatrix = fCTM.top();
        topMatrix.mapPoints(tPoints, verts, count*3);

        if(texs != nullptr && colors != nullptr){
            for(int i = 0; i < count*3; i+=3){
                GMatrix coordMatrix = GMatrix(verts[indices[i+2]].x() - verts[indices[i]].x(), verts[indices[i+1]].x() - verts[indices[i]].x(), verts[indices[i]].x(), 
                                              verts[indices[i+2]].y() - verts[indices[i]].y(), verts[indices[i+1]].y() - verts[indices[i]].y(), verts[indices[i]].y());
                GMatrix textureMatrix = GMatrix(texs[indices[i+2]].x() - texs[indices[i]].x(), texs[indices[i+1]].x() - texs[indices[i]].x(), texs[indices[i]].x(), 
                                                texs[indices[i+2]].y() - texs[indices[i]].y(), texs[indices[i+1]].y() - texs[indices[i]].y(), texs[indices[i]].y());
                GMatrix textureInverse;
                textureMatrix.invert(&textureInverse);

                ProxyShader textShader(source.getShader(), coordMatrix * textureInverse);
                textShader.setContext(topMatrix);

                GColor newColors[3] = {colors[indices[i]], colors[indices[i+1]], colors[indices[i+2]]};
                GPoint newPoints[3] = {tPoints[indices[i]], tPoints[indices[i+1]], tPoints[indices[i+2]]};
                
                GMatrix colorMatrix = GMatrix(newPoints[2].x() - newPoints[0].x(), newPoints[1].x() - newPoints[0].x(), newPoints[0].x(), 
                                              newPoints[2].y() - newPoints[0].y(), newPoints[1].y() - newPoints[0].y(), newPoints[0].y());
                
                GMatrix colorInvert;
                colorMatrix.invert(&colorInvert);

                clip(newPoints[0], newPoints[1], bounds, edges);
                clip(newPoints[1], newPoints[2], bounds, edges);
                clip(newPoints[2], newPoints[0], bounds, edges);
                if(edges.size() < 2) return;

                //sort edges
                std::sort(edges.begin(), edges.end(), edge_sorter);

                int L, R;
                float x0, x1;

                for(int y = edges[0]->top; y < edges.back()->bottom; ++y){
                    x0 = edges[0]->curX;
                    x1 = edges[1]->curX;
                    L = GRoundToInt(x0);
                    R = GRoundToInt(x1);

                    if(L > R)
                        std::swap(L, R);

                    GPixel* row = new GPixel[R-L];
                    textShader.shadeRow(L, y, R-L, row);
                    for(int x = L; x < R; ++x){
                        GColor color;
                        GPoint point = colorInvert * GPoint{x+.5f, y+.5f};

                        color = newColors[2] * point.x() + newColors[1] * point.y() + newColors[0] * (1-point.x()-point.y());

                        GPixel colorPixel = makePixel(color);
                        GPixel textPixel = row[x-L];

                        int a = div255(GPixel_GetA(colorPixel) * GPixel_GetA(textPixel));
                        int r = div255(GPixel_GetR(colorPixel) * GPixel_GetR(textPixel));
                        int g = div255(GPixel_GetG(colorPixel) * GPixel_GetG(textPixel));
                        int b = div255(GPixel_GetB(colorPixel) * GPixel_GetB(textPixel));

                        GPixel *dst = fDevice.getAddr(x, y);
                        *dst = GPixel_PackARGB(a, r, g, b);
                    }
                    delete[] row;

                    edges[0]->curX += edges[0]->m;
                    edges[1]->curX += edges[1]->m;
                    if(edges[1]->lastY(y))
                        edges.erase(edges.begin()+1);
                    if(edges[0]->lastY(y))
                        edges.erase(edges.begin());
                    
                    if(edges.size() == 0) break;
                }
            }
            return;
        }
        if(texs != nullptr){
            GMatrix coordMatrix = GMatrix(verts[2].x() - verts[0].x(), verts[1].x() - verts[0].x(), verts[0].x(), 
                                          verts[2].y() - verts[0].y(), verts[1].y() - verts[0].y(), verts[0].y());
            
            GMatrix textureMatrix = GMatrix(texs[2].x() - texs[0].x(), texs[1].x() - texs[0].x(), texs[0].x(), 
                                            texs[2].y() - texs[0].y(), texs[1].y() - texs[0].y(), texs[0].y());
            GMatrix textureInverse;
            textureMatrix.invert(&textureInverse);

            ProxyShader textShader(source.getShader(), coordMatrix * textureInverse);
            textShader.setContext(topMatrix);
                
            clip(tPoints[0], tPoints[1], bounds, edges);
            clip(tPoints[1], tPoints[2], bounds, edges);
            clip(tPoints[2], tPoints[0], bounds, edges);
            if(edges.size() < 2) return;

            //sort edges
            std::sort(edges.begin(), edges.end(), edge_sorter);

            int L, R;
            float x0, x1;

            for(int y = edges[0]->top; y < edges.back()->bottom; ++y){
                x0 = edges[0]->curX;
                x1 = edges[1]->curX;
                L = GRoundToInt(x0);
                R = GRoundToInt(x1);

                if(L > R)
                    std::swap(L, R);

                GPixel* row = new GPixel[R-L];
                textShader.shadeRow(L, y, R-L, row);
                for(int x = L; x < R; ++x){
                    GPixel* dst = fDevice.getAddr(x, y);
                    *dst = row[x-L];
                }
                delete[] row;

                edges[0]->curX += edges[0]->m;
                edges[1]->curX += edges[1]->m;
                if(edges[1]->lastY(y))
                    edges.erase(edges.begin()+1);
                if(edges[0]->lastY(y))
                    edges.erase(edges.begin());
                
                if(edges.size() == 0) break;
            }
            return;
        }
        if(colors != nullptr){
            for(int i = 0; i < count*3; i+=3){
                GColor newColors[3] = {colors[indices[i]], colors[indices[i+1]], colors[indices[i+2]]};
                GPoint newPoints[3] = {tPoints[indices[i]], tPoints[indices[i+1]], tPoints[indices[i+2]]};
                
                GMatrix colorMatrix = GMatrix(newPoints[2].x() - newPoints[0].x(), newPoints[1].x() - newPoints[0].x(), newPoints[0].x(), 
                                              newPoints[2].y() - newPoints[0].y(), newPoints[1].y() - newPoints[0].y(), newPoints[0].y());
                
                GMatrix colorInvert;
                colorMatrix.invert(&colorInvert);
                    
                clip(newPoints[0], newPoints[1], bounds, edges);
                clip(newPoints[1], newPoints[2], bounds, edges);
                clip(newPoints[2], newPoints[0], bounds, edges);
                if(edges.size() < 2) return;

                //sort edges
                std::sort(edges.begin(), edges.end(), edge_sorter);

                int L, R;
                float x0, x1;

                for(int y = edges[0]->top; y < edges.back()->bottom; ++y){
                    x0 = edges[0]->curX;
                    x1 = edges[1]->curX;
                    L = GRoundToInt(x0);
                    R = GRoundToInt(x1);

                    if(L > R)
                        std::swap(L, R);

                    for(int x = L; x < R; ++x){
                        GPoint point = colorInvert * GPoint{x+.5f, y+.5f};

                        GColor color = newColors[2] * point.x() + newColors[1] * point.y() + newColors[0] * (1-point.x()-point.y());

                        GPixel* dst = fDevice.getAddr(x, y);
                        *dst = makePixel(color);
                    }

                    edges[0]->curX += edges[0]->m;
                    edges[1]->curX += edges[1]->m;
                    if(edges[1]->lastY(y))
                        edges.erase(edges.begin()+1);
                    if(edges[0]->lastY(y))
                        edges.erase(edges.begin());
                    
                    if(edges.size() == 0) break;
                }
            }
            return;
        }
    }

    void drawQuad(const GPoint verts[4], const GColor colors[4], const GPoint texs[4], int level, const GPaint& source) override{
        int divisions = level + 2;
        float step = 1.0f/(divisions - 1);
        GPoint topLeft = verts[0];
        GPoint topRight = verts[1];
        GPoint bottomRight = verts[2];
        GPoint bottomLeft = verts[3];

        GPoint quad[divisions][divisions];
        GPoint quadTex[divisions][divisions];
        GColor quadColor[divisions][divisions];

        if(colors != nullptr){
            GColor color1 = colors[0];
            GColor color2 = colors[1];

            for(int i = 0; i < divisions; ++i){
                for(int j = 0; j < divisions; ++j){
                    GPoint point = topLeft + (topRight - topLeft) * j * step;
                    GColor color = color1 + (color2 - color1) * j * step;
                    quad[i][j] = point;
                    quadColor[i][j] = color;
                }
                topLeft += (verts[3] - verts[0]) * step;
                topRight += (verts[2] - verts[1]) * step;
                color1 += (colors[3] - colors[0]) * step;
                color2 += (colors[2] - colors[1]) * step;
            }

            for(int i = 0; i < divisions-1; ++i){
                for(int j = 0; j < divisions-1; ++j){
                    GPoint topLeftTri = quad[i][j];
                    GPoint topRightTri = quad[i][j+1];
                    GPoint bottomLeftTri = quad[i+1][j];
                    GPoint bottomRightTri = quad[i+1][j+1];

                    GColor topLeftColor = quadColor[i][j];
                    GColor topRightColor = quadColor[i][j+1];
                    GColor bottomLeftColor = quadColor[i+1][j];
                    GColor bottomRightColor = quadColor[i+1][j+1];

                    GPoint triVerts[3] = {topLeftTri, topRightTri, bottomLeftTri};
                    GColor triColors[3] = {topLeftColor, topRightColor, bottomLeftColor};
                    GPoint triVerts2[3] = {topRightTri, bottomRightTri, bottomLeftTri};
                    GColor triColors2[3] = {topRightColor, bottomRightColor, bottomLeftColor};
                    int indices[3] = {0, 1, 2};

                    drawMesh(triVerts, triColors, nullptr, 1, indices, source);
                    drawMesh(triVerts2, triColors2, nullptr, 1, indices, source);
                }
            }
            return;
        }

        if(texs != nullptr){
            GPoint tex1 = texs[0];
            GPoint tex2 = texs[1];

            for(int i = 0; i < divisions; ++i){
                for(int j = 0; j < divisions; ++j){
                    GPoint point = topLeft + (topRight - topLeft) * j * step;
                    GPoint texture = tex1 + (tex2 - tex1) * j * step;
                    quad[i][j] = point;
                    quadTex[i][j] = texture;
                }
                topLeft += (verts[3] - verts[0]) * step;
                topRight += (verts[2] - verts[1]) * step;
                tex1 += (texs[3] - texs[0]) * step;
                tex2 += (texs[2] - texs[1]) * step;
            }

            for(int i = 0; i < divisions-1; ++i){
                for(int j = 0; j < divisions-1; ++j){
                    GPoint topLeftTri = quad[i][j];
                    GPoint topRightTri = quad[i][j+1];
                    GPoint bottomLeftTri = quad[i+1][j];
                    GPoint bottomRightTri = quad[i+1][j+1];

                    GPoint topLeftTex = quadTex[i][j];
                    GPoint topRightTex = quadTex[i][j+1];
                    GPoint bottomLeftTex = quadTex[i+1][j];
                    GPoint bottomRightTex = quadTex[i+1][j+1];

                    GPoint triVerts[3] = {topLeftTri, topRightTri, bottomLeftTri};
                    GPoint triTex[3] = {topLeftTex, topRightTex, bottomLeftTex};
                    GPoint triVerts2[3] = {topRightTri, bottomRightTri, bottomLeftTri};
                    GPoint triTex2[3] = {topRightTex, bottomRightTex, bottomLeftTex};
                    int indices[3] = {0, 1, 2};

                    drawMesh(triVerts, nullptr, triTex, 1, indices, source);
                    drawMesh(triVerts2, nullptr, triTex2, 1, indices, source);
                }
            }
            return;
        }


    }

private:
    const GBitmap fDevice;
    std::stack<GMatrix> fCTM;
};

std::unique_ptr<GCanvas> GCreateCanvas(const GBitmap& device) {
    return std::unique_ptr<GCanvas>(new MyCanvas(device));
}

std::string GDrawSomething(GCanvas* canvas, GISize dim){
    return "something";
}