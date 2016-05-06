// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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
#include "Point.h"
#include <boost/array.hpp>
#include <boost/core/noncopyable.hpp>
#include <vector>

class GameWorldViewer;
class GameWorldView;

/// Klasse, die für das grafische Anzeigen (Rendern) des Terrains zuständig ist
class TerrainRenderer: private boost::noncopyable
{
public:
    typedef Point<float> PointF;
    typedef Point<int> PointI;

    TerrainRenderer();
    ~TerrainRenderer();

    /// Generate OpenGL structs and init data
    void GenerateOpenGL(const GameWorldViewer& gwv);

    /// Draws the map as seen through the given view. Optionally returns percentage of water drawn
    void Draw(const GameWorldView& gwv, unsigned int* water);

    /// Converts given point into a MapPoint (0 <= x < width and 0 <= y < height)
    /// Optionally returns offset of returned point to original point in pixels (for drawing)
    MapPoint ConvertCoords(const PointI pt, Point<int>* offset = 0) const;
    /// Get position of node in pixels (VertexPos)
    PointF GetNodePos(const MapPoint pt) { return GetVertex(pt).pos; }
    /// Get neighbour position of a node (VertexPos) potentially shifted so that the returned value is next to GetNodePos(pt)
    PointF GetNeighbourPos(MapPoint pt, const unsigned dir);

    /// Callback function for altitude changes
    void AltitudeChanged(const MapPoint pt, const GameWorldViewer& gwv);
    /// Callback function for visibility changes
    void VisibilityChanged(const MapPoint pt, const GameWorldViewer& gwv);

    /// Recalculates all colors on the map
    void UpdateAllColors(const GameWorldViewer& gwv);

private:

    struct MapTile
    {
        unsigned tileOffset;
        unsigned int count;
        PointI posOffset;
        MapTile(unsigned tileOffset, PointI posOffset): tileOffset(tileOffset), count(1), posOffset(posOffset){}
    };

    struct BorderTile
    {
        unsigned tileOffset;
        unsigned count;
        PointI posOffset;
        BorderTile(unsigned tileOffset, PointI posOffset): tileOffset(tileOffset), count(1), posOffset(posOffset){}
    };

    struct PreparedRoad
    {
        unsigned char type;
        PointI pos, pos2;
        float color1, color2;
        unsigned char dir;

        PreparedRoad(unsigned char type, PointI pos, PointI pos2, float color1, float color2, unsigned char dir): type(type), pos(pos), pos2(pos2), color1(color1), color2(color2), dir(dir) {}

        bool operator<(const PreparedRoad& b) const { return(type < b.type); }
    };

    struct Vertex
    {
        PointF pos; // Position vom jeweiligen Punkt
        float color;
        boost::array<PointF, 2> borderPos; // Mittelpunkt für Ränder
        boost::array<float, 2>  borderColor;
    };

    struct Color
    {
        float r;
        float g;
        float b;
    };

    typedef boost::array<PointF, 3> Triangle;
    typedef boost::array<Color, 3> ColorTriangle;

    struct Borders
    {
        boost::array<unsigned char, 2 > left_right;
        boost::array<unsigned char, 2 > right_left;
        boost::array<unsigned char, 2 > top_down;
        boost::array<unsigned, 2 > left_right_offset;
        boost::array<unsigned, 2 > right_left_offset;
        boost::array<unsigned, 2 > top_down_offset;
    };

    /// Size of the map
    unsigned short width, height;
    /// Map sized array of vertex related data
    std::vector<Vertex> vertices;
    /// Map sized array with terrain indices/textures (bottom, bottom right of node)
    std::vector<boost::array<unsigned char, 2> > terrain;

    std::vector<Triangle> gl_vertices;
    std::vector<Triangle> gl_texcoords;
    std::vector<ColorTriangle> gl_colors;

    unsigned int vbo_vertices;
    unsigned int vbo_texcoords;
    unsigned int vbo_colors;
    bool vboBuffersUsed;

    std::vector<Borders> borders;

    typedef boost::array<std::vector<PreparedRoad>, 4> PreparedRoads;

    /// Returns the index of a vertex. Used to access vertices and borders
    unsigned GetVertexIdx(const MapPoint pt) const { return static_cast<unsigned>(pt.y) * static_cast<unsigned>(width) + static_cast<unsigned>(pt.x); }
    /// Returns the index of the first triangle (each point has 2). Used to access gl_* structs
    unsigned GetTriangleIdx(const MapPoint pt) const { return GetVertexIdx(pt) * 2; }

    /// liefert den Vertex an der Stelle X, Y.
    Vertex& GetVertex(const MapPoint pt) { return vertices[GetVertexIdx(pt)]; }
    const Vertex& GetVertex(const MapPoint pt) const { return vertices[GetVertexIdx(pt)]; }

    /// erzeugt die Terrain-Vertices.
    void GenerateVertices(const GameWorldViewer& gwv);
    /// erzeugt Vertex (update, wenn die Daten ggf. im Vertexbuffer ersetzt werden sollen, bei Veränderung)
    void UpdateVertexPos(const MapPoint pt, const GameWorldViewer& gwv);
    void UpdateVertexColor(const MapPoint pt, const GameWorldViewer& gwv);
    void UpdateVertexTerrain(const MapPoint pt, const GameWorldViewer& gwv);
    /// erzeugt Rand-Vertex
    void UpdateBorderVertex(const MapPoint pt, const GameWorldViewer& gwv);

    /// Erzeugt fertiges Dreieck für OpenGL
    void UpdateTrianglePos(const MapPoint pt, const GameWorldViewer& gwv, const bool update);
    void UpdateTriangleColor(const MapPoint pt, const GameWorldViewer& gwv, const bool update);
    void UpdateTriangleTerrain(const MapPoint pt, const GameWorldViewer& gwv, const bool update);
    /// Erzeugt die Dreiecke für die Ränder
    void UpdateBorderTrianglePos(const MapPoint pt, const GameWorldViewer& gwv, const bool update);
    void UpdateBorderTriangleColor(const MapPoint pt, const GameWorldViewer& gwv, const bool update);
    void UpdateBorderTriangleTerrain(const MapPoint pt, const GameWorldViewer& gwv, const bool update);

    /// liefert den Vertex-Farbwert an der Stelle X,Y
    float GetColor(const MapPoint pt) { return GetVertex(pt).color; }
    /// liefert den Rand-Vertex an der Stelle X,Y
    PointF GetB(const MapPoint pt, unsigned char triangle) const { return GetVertex(pt).borderPos[triangle]; }
    /// Liefert BX,BY um einen Punkt herum, beachtet auch Kartenränder (entspricht GetTerrainX)
    PointF GetBAround(const MapPoint pt, const unsigned char triangle, const unsigned char dir);
    /// liefert den Rand-Vertex-Farbwert an der Stelle X,Y
    float GetBColor(const MapPoint pt, unsigned char triangle) { return GetVertex(pt).borderColor[triangle]; }

    /// Zeichnet die Wege
    void PrepareWaysPoint(PreparedRoads& sorted_roads, const GameWorldView& gwv, MapPoint t, const PointI& offset);

    void DrawWays(const PreparedRoads& sorted_roads);
};

#endif
