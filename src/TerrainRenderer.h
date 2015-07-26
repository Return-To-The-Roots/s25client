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

#include "gameTypes/MapTypes.h"

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


/// Klasse, die f�r das grafische Anzeigen (Rendern) des Terrains zust�ndig ist
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
            ColorPoint border[2]; // Mittelpunkt f�r R�nder
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

        /// Breite und H�he der Karte
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

        unsigned GetTRIdx(const MapPoint pt)
        { return static_cast<unsigned>(pt.y) * static_cast<unsigned>(width) + static_cast<unsigned>(pt.x); }

        /// liefert den Vertex an der Stelle X, Y.
        Vertex& GetVertex(const MapPoint pt) { return vertices[GetTRIdx(pt)]; }

        /// erzeugt die Terrain-Vertices.
        void GenerateVertices(const GameWorldViewer* gwb);
        /// erzeugt Vertex (update, wenn die Daten ggf. im Vertexbuffer ersetzt werden sollen, bei Ver�nderung)
        void UpdateVertexPos(const MapPoint pt, const GameWorldViewer* gwv);
        void UpdateVertexColor(const MapPoint pt, const GameWorldViewer* gwv);
        void UpdateVertexTerrain(const MapPoint pt, const GameWorldViewer* gwv);
        /// erzeugt Rand-Vertex
        void UpdateBorderVertex(const MapPoint pt, const GameWorldViewer* gwv);

        /// Erzeugt fertiges Dreieick f�r OpenGL
        void UpdateTrianglePos(const MapPoint pt, const GameWorldViewer* gwv, const bool update);
        void UpdateTriangleColor(const MapPoint pt, const GameWorldViewer* gwv, const bool update);
        void UpdateTriangleTerrain(const MapPoint pt, const GameWorldViewer* gwv, const bool update);
        /// Erzeugt die Dreiecke f�r die R�nder
        void UpdateBorderTrianglePos(const MapPoint pt, const GameWorldViewer* gwv, const bool update);
        void UpdateBorderTriangleColor(const MapPoint pt, const GameWorldViewer* gwv, const bool update);
        void UpdateBorderTriangleTerrain(const MapPoint pt, const GameWorldViewer* gwv, const bool update);

        /// liefert den Vertex-Farbwert an der Stelle X,Y
        float GetColor(const MapPoint pt) { return GetVertex(pt).pos.color; }
        /// liefert den X-Rand-Vertex an der Stelle X,Y
        float GetBX(const MapPoint pt, unsigned char triangle) { return GetVertex(pt).border[triangle].pos.x; }
        /// liefert den Y-Rand-Vertex an der Stelle X,Y
        float GetBY(const MapPoint pt, unsigned char triangle) { return GetVertex(pt).border[triangle].pos.y; }
        /// Liefert BX,BY um einen Punkt herum, beachtet auch Kartenr�nder (entspricht GetTerrainX)
        float GetBXAround(int x, int y, const unsigned char triangle, const unsigned char dir);
        float GetBYAround(int x, int y, const unsigned char triangle, const unsigned char dir);
        /// liefert den Rand-Vertex-Farbwert an der Stelle X,Y
        float GetBColor(const MapPoint pt, unsigned char triangle) { return GetVertex(pt).border[triangle].color; }

        /// Zeichnet die Wege
        void PrepareWays(GameWorldView* gwv);
        void PrepareWaysPoint(GameWorldView* gwv, MapPoint t, int xo, int yo);

        void DrawWays(GameWorldView* gwv);


    public:

        TerrainRenderer();
        ~TerrainRenderer();

        /// erzeugt die OpenGL-Vertices.
        void GenerateOpenGL(const GameWorldViewer* gwv);


        /// zeichnet den Kartenausschnitt.
        void Draw(GameWorldView* gwv, unsigned int* water);

        /// Konvertiert "falsche Koordinaten", also im Minusbereich oder zu gro� wegen Zeichnen, in "richtige Koordinaten"
        /// mit 0 <= x_out < width und 0 <= y_out < height
        MapPoint ConvertCoords(int x, int y, int* xo = 0, int* yo = 0) const;
        /// liefert den X-Vertex an der Stelle X,Y
        float GetTerrainX(const MapPoint pt) { return GetVertex(pt).pos.pos.x; }
        /// liefert den Y-Vertex an der Stelle X,Y
        float GetTerrainY(const MapPoint pt) { return GetVertex(pt).pos.pos.y; }
        /// liefert X-Vertex drumherum, korrigiert Koordinaten nicht
        float GetTerrainXAround(int x,  int y, const unsigned dir);
        float GetTerrainYAround(int x,  int y, const unsigned dir);

        /// H�he eines Punktes wurde (durch Planierer) ver�ndert --> updaten
        void AltitudeChanged(const MapPoint pt, const GameWorldViewer* gwv);
        /// Sichtbarkeit eines Punktes ver�ndert
        void VisibilityChanged(const MapPoint pt, const GameWorldViewer* gwv);

        /// Berechnet Schattierungen der gesamten Map neu
        void UpdateAllColors(const GameWorldViewer* gwv);

};


#endif
