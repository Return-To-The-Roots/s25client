// $Id: TerrainRenderer.h 9357 2014-04-25 15:35:25Z FloSoft $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.
#ifndef TERRAIN_RENDERER_H_
#define TERRAIN_RENDERER_H_

#include "MapConsts.h"

class GameWorldViewer;
class GameWorldView;

struct MapTile
{
    int x;
    int y;
    unsigned int count;
    int xo;
    int yo;
};

struct BorderTile
{
    int offset;
    unsigned int count;
    int xo;
    int yo;
};

struct PreparedRoad
{
    unsigned char type;
    float xpos, ypos;
    float xpos2, ypos2;
    float color1, color2;
    unsigned char dir;

    PreparedRoad(unsigned char type, float xpos, float ypos, float xpos2, float ypos2, float color1, float color2, unsigned char dir) : type(type), xpos(xpos), ypos(ypos), xpos2(xpos2), ypos2(ypos2), color1(color1), color2(color2), dir(dir) {}

    bool operator<(const PreparedRoad b) const {return(type < b.type);}
};


/// Klasse, die für das grafische Anzeigen (Rendern) des Terrains zuständig ist
class TerrainRenderer
{
        struct Point
        {
            float x;
            float y;
        };

        struct ColorPoint
        {
            Point pos;
            float color;
        };

        struct Vertex
        {
            ColorPoint pos; // Position vom jeweiligen Punkt
            unsigned char terrain[2]; // Terrain der Dreiecke
            ColorPoint border[2]; // Mittelpunkt für Ränder
        };

        struct Color
        {
            float r;
            float g;
            float b;
        };

        struct Triangle
        {
            Point pos[3];
        };

        struct ColorTriangle
        {
            Color colors[3];
        };

        struct Borders
        {
            unsigned char left_right[2];
            unsigned char right_left[2];
            unsigned char top_down[2];
            unsigned int left_right_offset[2];
            unsigned int right_left_offset[2];
            unsigned int top_down_offset[2];
        };

        /// Breite und Höhe der Karte
        unsigned short width, height;

        Vertex* vertices;

        Triangle* gl_vertices;
        Triangle* gl_texcoords;
        ColorTriangle* gl_colors;

        unsigned int vbo_vertices;
        unsigned int vbo_texcoords;
        unsigned int vbo_colors;

        Borders* borders;
        unsigned int border_count;

    private:


        /// liefert den Vertex an der Stelle X, Y.
        Vertex& GetVertex(const MapCoord x, const MapCoord y) { return vertices[y * width + x]; }

        /// erzeugt die Terrain-Vertices.
        void GenerateVertices(const GameWorldViewer* gwb);
        /// erzeugt Vertex (update, wenn die Daten ggf. im Vertexbuffer ersetzt werden sollen, bei Veränderung)
        void UpdateVertexPos(const MapCoord x, const MapCoord y, const GameWorldViewer* gwv);
        void UpdateVertexColor(const MapCoord x, const MapCoord y, const GameWorldViewer* gwv);
        void UpdateVertexTerrain(const MapCoord x, const MapCoord y, const GameWorldViewer* gwv);
        /// erzeugt Rand-Vertex
        void UpdateBorderVertex(const MapCoord x, const MapCoord y, const GameWorldViewer* gwv);

        /// Erzeugt fertiges Dreieick für OpenGL
        void UpdateTrianglePos(const MapCoord x, const MapCoord y, const GameWorldViewer* gwv, const bool update);
        void UpdateTriangleColor(const MapCoord x, const MapCoord y, const GameWorldViewer* gwv, const bool update);
        void UpdateTriangleTerrain(const MapCoord x, const MapCoord y, const GameWorldViewer* gwv, const bool update);
        /// Erzeugt die Dreiecke für die Ränder
        void UpdateBorderTrianglePos(const MapCoord x, const MapCoord y, const GameWorldViewer* gwv, const bool update);
        void UpdateBorderTriangleColor(const MapCoord x, const MapCoord y, const GameWorldViewer* gwv, const bool update);
        void UpdateBorderTriangleTerrain(const MapCoord x, const MapCoord y, const GameWorldViewer* gwv, const bool update);

        /// liefert den Vertex-Farbwert an der Stelle X,Y
        float GetColor(const MapCoord x, const MapCoord y) { return GetVertex(x, y).pos.color; }
        /// liefert den X-Rand-Vertex an der Stelle X,Y
        float GetBX(const MapCoord x, const MapCoord y, unsigned char triangle) { return GetVertex(x, y).border[triangle].pos.x; }
        /// liefert den Y-Rand-Vertex an der Stelle X,Y
        float GetBY(const MapCoord x, const MapCoord y, unsigned char triangle) { return GetVertex(x, y).border[triangle].pos.y; }
        /// Liefert BX,BY um einen Punkt herum, beachtet auch Kartenränder (entspricht GetTerrainX)
        float GetBXAround(int x, int y, const unsigned char triangle, const unsigned char dir);
        float GetBYAround(int x, int y, const unsigned char triangle, const unsigned char dir);
        /// liefert den Rand-Vertex-Farbwert an der Stelle X,Y
        float GetBColor(const MapCoord x, const MapCoord y, unsigned char triangle) { return GetVertex(x, y).border[triangle].color; }

        /// Zeichnet die Wege
        void PrepareWays(GameWorldView* gwv);
        void PrepareWaysPoint(GameWorldView* gwv, unsigned short tx, unsigned short ty, int xo, int yo);

        void DrawWays(GameWorldView* gwv);


    public:

        TerrainRenderer();
        ~TerrainRenderer();

        /// erzeugt die OpenGL-Vertices.
        void GenerateOpenGL(const GameWorldViewer* gwv);


        /// zeichnet den Kartenausschnitt.
        void Draw(GameWorldView* gwv, unsigned int* water);

        /// Konvertiert "falsche Koordinaten", also im Minusbereich oder zu groß wegen Zeichnen, in "richtige Koordinaten"
        /// mit 0 <= x_out < width und 0 <= y_out < height
        void ConvertCoords(int x, int y, unsigned short& x_out, unsigned short& y_out, int* xo = 0, int* yo = 0) const;
        /// liefert den X-Vertex an der Stelle X,Y
        float GetTerrainX(const MapCoord x, const MapCoord y) { return GetVertex(x, y).pos.pos.x; }
        /// liefert den Y-Vertex an der Stelle X,Y
        float GetTerrainY(const MapCoord x, const MapCoord y) { return GetVertex(x, y).pos.pos.y; }
        /// liefert X-Vertex drumherum, korrigiert Koordinaten nicht
        float GetTerrainXAround(int x,  int y, const unsigned dir);
        float GetTerrainYAround(int x,  int y, const unsigned dir);

        /// Höhe eines Punktes wurde (durch Planierer) verändert --> updaten
        void AltitudeChanged(const MapCoord x, const MapCoord y, const GameWorldViewer* gwv);
        /// Sichtbarkeit eines Punktes verändert
        void VisibilityChanged(const MapCoord x, const MapCoord y, const GameWorldViewer* gwv);

        /// Berechnet Schattierungen der gesamten Map neu
        void UpdateAllColors(const GameWorldViewer* gwv);

};


#endif
