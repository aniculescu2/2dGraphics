#include "GTools.h"

static void createEdge(GPoint p0, GPoint p1, std::vector<edge*>& edges, int winding){
    if(p0.y() > p1.y()){ //swap points if p0 is below p1
        std::swap(p0, p1);
    }
    edge* newEdge = (edge*)malloc(sizeof(edge));
    newEdge->top = GRoundToInt(p0.y());
    newEdge->bottom = GRoundToInt(p1.y());
    if(newEdge->top == newEdge->bottom){
        return;
    }
    newEdge->m = ((p1.x() - p0.x()) / (p1.y() - p0.y()));
    newEdge->curX = p0.x() + newEdge->m * (newEdge->top - p0.y() + .5f); //x at pixel center
    newEdge->winding = winding;
    
    edges.push_back(newEdge);
}

void clip(GPoint p0, GPoint p1, GRect canvas, std::vector<edge*>& edges){
    int winding = -1;
    if(p0.y() == p1.y()){ //if the line is horizontal
        return;
    }
    if(p0.y() > p1.y()){ //swap points if p0 is below p1
        std::swap(p0, p1);
        winding = 1;
    }

    if(p1.y() < canvas.top() || p0.y() > canvas.bottom()){ //if the line is completely outside the canvas
        return;
    }

    if(p0.y() < canvas.top()){ //if p0 is above the canvas
        p0.fX = p0.x() + (p1.x() - p0.x()) * (canvas.top()-p0.y()) / (p1.y() - p0.y());
        p0.fY = canvas.top();
    }

    if(p1.y() > canvas.bottom()){ //if p1 is below the canvas
        p1.fX = p1.x() + (p0.x() - p1.x()) * (p1.y()-canvas.bottom()) / (p1.y() - p0.y());
        p1.fY = canvas.bottom();
    }

    if(p0.x() > p1.x()){ //swap points if p0 is to the right of p1
        std::swap(p0, p1);
    }

    if(p1.x() < canvas.left()){ //if the line is completely outside the canvas
        p0.fX = canvas.left();
        p1.fX = canvas.left();
        createEdge(p0, p1, edges, winding);
        return;
    }

    if(p0.x() > canvas.right()){
        p0.fX = canvas.right();
        p1.fX = canvas.right();
        createEdge(p0, p1, edges, winding);
        return;
    }

    if(p0.x() < canvas.left()){ //if p0 is to the left of the canvas
        GPoint phantomPoint = {canvas.left(), p0.y()};
        p0.fY = p0.y() + (p1.y() - p0.y()) * (p0.x() - canvas.left()) / (p0.x() - p1.x());
        p0.fX = canvas.left();

       createEdge(phantomPoint, p0, edges, winding);
    }

    if(p1.x() > canvas.right()){ //if p1 is to the right of the canvas
        GPoint phantomPoint = {canvas.right(), p1.y()};
        p1.fY = p1.y() + (p0.y() - p1.y()) * (p1.x() - canvas.right()) / (p1.x() - p0.x());
        p1.fX = canvas.right();

        createEdge(phantomPoint, p1, edges, winding);
    }

    createEdge(p0, p1, edges, winding);
}

bool edge_sorter(edge* const& e1, edge* const& e2){
    if(e1->top == e2->top)
        return e1->curX < e2->curX;
    return e1->top < e2->top;
}

bool edge_sorter2(edge* const& e1, edge* const& e2){
    return e1->curX < e2->curX;
}
