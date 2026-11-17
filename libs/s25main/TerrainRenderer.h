// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Point.h"
#include "ogl/VBO.h"
#include "gameTypes/Direction.h"
#include "gameTypes/MapCoordinates.h"
#include "gameTypes/MapTypes.h"
#include "gameData/DescIdx.h"
#include "gameData/DescriptionVector.h"
#include <boost/noncopyable.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <array>
#include <memory>
#include <vector>

class GameWorldViewer;
class glArchivItem_Bitmap;
struct EdgeDesc;
struct TerrainDesc;
struct WorldDescription;

glArchivItem_Bitmap* new_clone(const glArchivItem_Bitmap& bmp);

/// Klasse, die für das grafische Anzeigen (Rendern) des Terrains zuständig ist
class TerrainRenderer : private boost::noncopyable
{
public:
    TerrainRenderer();
    ~TerrainRenderer();

    /// Generates data structures (uninitialized)
    void init(const MapExtent& size);
    /// Generate OpenGL structs and init data (also calls Init)
    void generateOpenGL(const GameWorldViewer& gwv);

    /// Draws the map between the given points. Optionally returns percentage of water drawn
    void draw(const Position& firstPt, const Position& lastPt, const GameWorldViewer& gwv, unsigned* water) const;

    /// Converts given point into a MapPoint (0 <= x < width and 0 <= y < height)
    /// Optionally returns offset of returned point to original point in pixels (for drawing)
    MapPoint convertCoords(Position pt, Position* offset = nullptr) const;
    /// Get position of node in pixels (VertexPos)
    PointF getVertexPos(const MapPoint pt) const { return getVertex(pt).pos; }
    /// Get neighbour position of a node (VertexPos) potentially shifted so that the returned value is next to
    /// getNodePos(pt)
    PointF getNeighbourVertexPos(MapPoint pt, Direction dir) const;

    /// Callback function for altitude changes
    void altitudeChanged(MapPoint pt, const GameWorldViewer& gwv);
    /// Callback function for visibility changes
    void visibilityChanged(MapPoint pt, const GameWorldViewer& gwv);

    /// Recalculates all colors on the map
    void updateAllColors(const GameWorldViewer& gwv);

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
        RoadDir dir;

        PreparedRoad(Position pos, Position pos2, float color1, float color2, RoadDir dir)
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

    using Triangle = std::array<PointF, 3>;
    using ColorTriangle = std::array<Color, 3>;

    struct Borders
    {
        std::array<DescIdx<EdgeDesc>, 2> left_right;
        std::array<DescIdx<EdgeDesc>, 2> right_left;
        std::array<DescIdx<EdgeDesc>, 2> top_down;
        std::array<unsigned, 2> left_right_offset;
        std::array<unsigned, 2> right_left_offset;
        std::array<unsigned, 2> top_down_offset;
    };

    struct TerrainTexture
    {
        boost::ptr_vector<glArchivItem_Bitmap> textures;
        Triangle usdCoords, rsuCoords;
    };

    using PreparedRoads = std::vector<std::vector<PreparedRoad>>;

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

    using BmpPtr = std::unique_ptr<glArchivItem_Bitmap>;
    DescriptionVector<TerrainTexture, TerrainDesc> terrainTextures;
    DescriptionVector<BmpPtr, EdgeDesc> edgeTextures;
    /// Flat 2D array: [Landscape][RoadType]
    std::vector<BmpPtr> roadTextures;

    /// Returns the index of a vertex. Used to access vertices and borders
    unsigned getVertexIdx(const MapPoint pt) const
    {
        return static_cast<unsigned>(pt.y) * static_cast<unsigned>(size_.x) + static_cast<unsigned>(pt.x);
    }
    /// Returns the index of the first triangle (each point has 2). Used to access gl_* structs
    unsigned getTriangleIdx(const MapPoint pt) const { return getVertexIdx(pt) * 2; }
    /// Return the coordinates of the neighbour node
    MapPoint getNeighbour(const MapPoint& pt, Direction dir) const;

    /// liefert den Vertex an der Stelle X, Y.
    Vertex& getVertex(const MapPoint pt) { return vertices[getVertexIdx(pt)]; }
    const Vertex& getVertex(const MapPoint pt) const { return vertices[getVertexIdx(pt)]; }

    void loadTextures(const WorldDescription& desc);

    /// Creates and initializes (map-)vertices for the viewer
    void generateVertices(const GameWorldViewer& gwv);
    /// Updates (map-)vertex attributes
    void updateVertexPos(MapPoint pt, const GameWorldViewer& gwv);
    void updateVertexColor(MapPoint pt, const GameWorldViewer& gwv);
    void loadVertexTerrain(MapPoint pt, const GameWorldViewer& gwv);
    /// Update (map-)border vertex attributes
    void updateBorderVertex(MapPoint pt);

    /// Fills OGL vertex data from map vertex data (updateVBO = true updates also VBO if used)
    void updateTrianglePos(MapPoint pt, bool updateVBO);
    void updateTriangleColor(MapPoint pt, bool updateVBO);
    void updateTriangleTerrain(MapPoint pt, bool updateVBO);
    /// Fills OGL border vertex data from map vertex data
    void updateBorderTrianglePos(MapPoint pt, bool updateVBO);
    void updateBorderTriangleColor(MapPoint pt, bool updateVBO);
    void updateBorderTriangleTerrain(MapPoint pt, bool updateVBO);

    /// liefert den Vertex-Farbwert an der Stelle X,Y
    float getColor(const MapPoint pt) const { return getVertex(pt).color; }
    /// liefert den Rand-Vertex an der Stelle X,Y
    PointF getBorderPos(const MapPoint pt, unsigned char triangle) const { return getVertex(pt).borderPos[triangle]; }
    /// Get neighbour border position of a node (VertexPos) potentially shifted so that the returned value is next to
    /// getBorderPos(pt)
    PointF getNeighbourBorderPos(MapPoint pt, unsigned char triangle, Direction dir) const;
    /// liefert den Rand-Vertex-Farbwert an der Stelle X,Y
    float getBorderColor(const MapPoint pt, unsigned char triangle) const
    {
        return getVertex(pt).borderColor[triangle];
    }

    /// Adds possible roads from the given point to the prepared data struct
    void prepareWaysPoint(PreparedRoads& sorted_roads, const GameWorldViewer& gwViewer, MapPoint pt,
                          const Position& offset) const;
    /// Draw the prepared roads
    void drawWays(const PreparedRoads& sorted_roads) const;
};
