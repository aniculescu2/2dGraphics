#ifndef GEdge_DEFINED
#define GEdge_DEFINED

#include <assert.h>

struct edge{ 
    int top, bottom;
    float m, curX;
    int winding;
    
    bool lastY(int y) const{
        assert(y >= top && y < bottom);
        return y == bottom - 1;
    }

    bool active(int y) const{
        assert(y >= 0 && y < bottom);
        return y >= top && y < bottom;
    }
};

#endif