#include "include/GPixel.h"
#include "include/GColor.h"
#include "include/GPoint.h"
#include "include/GRect.h"

#include "GEdge.h"


static inline unsigned int div255(unsigned int x){
    x += 128;
    return (x + (x >> 8)) >> 8;
}

static inline GPixel makePixel(const GColor& color){
    if(color.a == 1)
        return GPixel_PackARGB(255, GRoundToInt(color.r*255), GRoundToInt(color.g*255), GRoundToInt(color.b*255));
    return GPixel_PackARGB(GRoundToInt(color.a*255), GRoundToInt(color.r*color.a*255), GRoundToInt(color.g*color.a*255), GRoundToInt(color.b*color.a*255));
}

void clip(GPoint, GPoint, GRect, std::vector<edge*>&);
bool edge_sorter(edge* const& e1, edge* const& e2);
bool edge_sorter2(edge* const& e1, edge* const& e2);
