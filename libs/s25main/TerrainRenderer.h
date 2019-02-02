// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "Point.h"
#include "ogl/VBO.h"
#include "gameTypes/MapCoordinates.h"
#include "gameTypes/MapTypes.h"
#include "gameData/DescIdx.h"
#include <boost/noncopyable.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <array>
#include <memory>
#include <vector>

struct Direction;
class GameWorldViewer;
class glArchivItem_Bitmap;
struct TerrainDesc;
struct WorldDescription;

glArchivItem_Bitmap* new_clone(const glArchivItem_Bitmap& bmp);

/// Klasse, die für das grafische Anzeigen (Rendern) des Terrains zuständig ist
class TerrainRenderer : private boost::noncopyable
{
public:
    typedef Point<float> PointF;

    TerrainRenderer();
    ~TerrainRenderer();

    /// Generates data structures (uninitialized)
    void Init(const MapExtent& size);
    /// Generate OpenGL structs and init data (also calls Init)
    void GenerateOpenGL(const GameWorldViewer& gwv);

    /// Draws the map between the given points. Optionally returns percentage of water drawn
    void Draw(const Position& firstPt, const Position& lastPt, const GameWorldViewer& gwv, unsigned* water) const;

    /// Converts given point into a MapPoint (0 <= x < width and 0 <= y < height)
    /// Optionally returns offset of returned point to original point in pixels (for drawing)
    MapPoint ConvertCoords(const Position pt, Position* offset = 0) const;
    /// Get position of node in pixels (VertexPos)
    PointF GetVertexPos(const MapPoint pt) const { return GetVertex(pt).pos; }
    /// Get neighbour position of a node (VertexPos) potentially shifted so that the returned value is next to GetNodePos(pt)
    PointF GetNeighbourVertexPos(MapPoint pt, unsigned dir) const;

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
        unsigned count;
        Position posOffset;
        MapTile(unsigned tileOffset, Position posOffset) : tileOffset(tileOffset), count(1), posOffset(posOffset) {}
    };

    struct BorderTile
    {
        unsigned tileOffset;
        unsigned count;
        Position posOffset;
        BorderTile(unsigned tileOffset, Position posOffset) : tileOffset(tileOffset), count(1), posOffset(posOffset) {}
    };

    struct PreparedRoad
    {
        Position pos, pos2;
        float color1, color2;
        unsigned char dir;

        PreparedRoad(Position pos, Position pos2, float color1, float color2, unsigned char dir)
            : pos(pos), pos2(pos2), color1(color1), color2(color2), dir(dir)
        {}
    };

    struct Vertex
    {
        PointF pos; // Position vom jeweiligen Punkt
        float color;
        std::array<PointF, 2> borderPos; // Mittelpunkt für Ränder
        std::array<float, 2> borderColor;
    };

    struct Color
    {
        float r;
        float g;
        float b;
    };

    typedef std::array<PointF, 3> Triangle;
    typedef std::array<Color, 3> ColorTriangle;

    struct Borders
    {
        std::array<unsigned char, 2> left_right;
        std::array<unsigned char, 2> right_left;
        std::array<unsigned char, 2> top_down;
        std::array<unsigned, 2> left_right_offset;
        std::array<unsigned, 2> right_left_offset;
        std::array<unsigned, 2> top_down_offset;
    };

    struct TerrainTexture
    {
        boost::ptr_vector<glArchivItem_Bitmap> textures;
        Triangle usdCoords, rsuCoords;
    };

    typedef std::vector<std::vector<PreparedRoad>> PreparedRoads;

    /// Size of the map
    MapExtent size_;
    /// Map sized array of vertex related data
    std::vector<Vertex> vertices;
    /// Map sized array with terrain indices/textures (bottom, bottom right of node)
    std::vector<std::array<DescIdx<TerrainDesc>, 2>> terrain;

    std::vector<Triangle> gl_vertices;
    std::vector<Triangle> gl_texcoords;
    std::vector<ColorTriangle> gl_colors;

    ogl::VBO<Triangle> vbo_vertices;
    ogl::VBO<Triangle> vbo_texcoords;
    ogl::VBO<ColorTriangle> vbo_colors;

    std::vector<Borders> borders;

    typedef std::unique_ptr<glArchivItem_Bitmap> BmpPtr;
    std::vector<TerrainTexture> terrainTextures;
    std::vector<BmpPtr> edgeTextures;
    /// Flat 2D array: [Landscape][RoadType]
    std::vector<BmpPtr> roadTextures;

    /// Returns the index of a vertex. Used to access vertices and borders
    unsigned GetVertexIdx(const MapPoint pt) const
    {
        return static_cast<unsigned>(pt.y) * static_cast<unsigned>(size_.x) + static_cast<unsigned>(pt.x);
    }
    /// Returns the index of the first triangle (each point has 2). Used to access gl_* structs
    unsigned GetTriangleIdx(const MapPoint pt) const { return GetVertexIdx(pt) * 2; }
    /// Return the coordinates of the neighbour node
    MapPoint GetNeighbour(const MapPoint& pt, const Direction dir) const;

    /// liefert den Vertex an der Stelle X, Y.
    Vertex& GetVertex(const MapPoint pt) { return vertices[GetVertexIdx(pt)]; }
    const Vertex& GetVertex(const MapPoint pt) const { return vertices[GetVertexIdx(pt)]; }

    void LoadTextures(const WorldDescription& desc);

    /// Creates and initializes (map-)vertices for the viewer
    void GenerateVertices(const GameWorldViewer& gwv);
    /// Updates (map-)vertex attributes
    void UpdateVertexPos(const MapPoint pt, const GameWorldViewer& gwv);
    void UpdateVertexColor(const MapPoint pt, const GameWorldViewer& gwv);
    void LoadVertexTerrain(const MapPoint pt, const GameWorldViewer& gwv);
    /// Update (map-)border vertex attributes
    void UpdateBorderVertex(const MapPoint pt);

    /// Fills OGL vertex data from map vertex data (updateVBO = true updates also VBO if used)
    void UpdateTrianglePos(const MapPoint pt, bool updateVBO);
    void UpdateTriangleColor(const MapPoint pt, bool updateVBO);
    void UpdateTriangleTerrain(const MapPoint pt, bool updateVBO);
    /// Fills OGL border vertex data from map vertex data
    void UpdateBorderTrianglePos(const MapPoint pt, bool updateVBO);
    void UpdateBorderTriangleColor(const MapPoint pt, bool updateVBO);
    void UpdateBorderTriangleTerrain(const MapPoint pt, bool updateVBO);

    /// liefert den Vertex-Farbwert an der Stelle X,Y
    float GetColor(const MapPoint pt) const { return GetVertex(pt).color; }
    /// liefert den Rand-Vertex an der Stelle X,Y
    PointF GetBorderPos(const MapPoint pt, unsigned char triangle) const { return GetVertex(pt).borderPos[triangle]; }
    /// Get neighbour border position of a node (VertexPos) potentially shifted so that the returned value is next to GetBorderPos(pt)
    PointF GetNeighbourBorderPos(const MapPoint pt, unsigned char triangle, unsigned char dir) const;
    /// liefert den Rand-Vertex-Farbwert an der Stelle X,Y
    float GetBorderColor(const MapPoint pt, unsigned char triangle) const { return GetVertex(pt).borderColor[triangle]; }

    /// Adds possible roads from the given point to the prepared data struct
    void PrepareWaysPoint(PreparedRoads& sorted_roads, const GameWorldViewer& gwViewer, MapPoint pt, const Position& offset) const;
    /// Draw the prepared roads
    void DrawWays(const PreparedRoads& sorted_roads) const;
};

#endif
