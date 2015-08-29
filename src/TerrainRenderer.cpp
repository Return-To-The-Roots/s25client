// $Id: TerrainRenderer.cpp 9518 2014-11-30 09:22:47Z marcus $
//
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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "defines.h"
#include "TerrainRenderer.h"

#include "drivers/VideoDriverWrapper.h"
#include "gameData/MapConsts.h"
#include "GameWorld.h"
#include "Settings.h"
#include "GameClient.h"
#include "MapGeometry.h"
#include "gameData/TerrainData.h"
#include <cstdlib>
#include <cmath>
#include <boost/scoped_array.hpp>

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

TerrainRenderer::TerrainRenderer() :
	width(0), height(0),
    vbo_vertices(0), vbo_texcoords(0), vbo_colors(0)
{}

TerrainRenderer::~TerrainRenderer()
{
    if(SETTINGS.video.vbo)
    {
        glDeleteBuffersARB(1, (const GLuint*)&vbo_vertices);
        glDeleteBuffersARB(1, (const GLuint*)&vbo_texcoords);
        glDeleteBuffersARB(1, (const GLuint*)&vbo_colors);
    }
}

TerrainRenderer::PointF TerrainRenderer::GetTerrainAround(MapPoint pt, const unsigned dir)
{
    PointI ptNb = GetPointAround(PointI(pt), dir);

    PointI offset;
    MapPoint t = ConvertCoords(ptNb, &offset);

    return GetTerrain(t) + PointF(offset);
}

TerrainRenderer::PointF TerrainRenderer::GetBAround(const MapPoint pt, const unsigned char triangle, const unsigned char dir)
{
    PointI ptNb = GetPointAround(PointI(pt), dir);

    Point<int> offset;
    MapPoint t = ConvertCoords(ptNb, &offset);

    return GetB(t, triangle) + PointF(offset);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  erzeugt die Terrain-Vertices.
 *
 *  @author OLiver
 */
void TerrainRenderer::GenerateVertices(const GameWorldViewer& gwv)
{
    vertices.clear();
    vertices.resize(width * height);

    // Terrain generieren
    for(MapCoord y = 0; y < height; ++y)
    {
        for(MapCoord x = 0; x < width; ++x)
        {
            MapPoint pt(x, y);
            UpdateVertexPos(pt, gwv);
            UpdateVertexColor(pt, gwv);
            UpdateVertexTerrain(pt, gwv);
        }
    }

    // Ränder generieren
    for(MapCoord y = 0; y < height; ++y)
    {
        for(MapCoord x = 0; x < width; ++x)
        {
            MapPoint pt(x, y);
            UpdateBorderVertex(pt, gwv);
        }
    }
}

/// erzeugt Vertex
void TerrainRenderer::UpdateVertexPos(const MapPoint pt, const GameWorldViewer& gwv)
{
    GetVertex(pt).pos.pos.x = float(pt.x * TR_W + ( (pt.y & 1) ? TR_W / 2 : 0) );
    GetVertex(pt).pos.pos.y = float(pt.y * TR_H - HEIGHT_FACTOR * gwv.GetNode(pt).altitude + HEIGHT_FACTOR * 0x0A );
}


void TerrainRenderer::UpdateVertexColor(const MapPoint pt, const GameWorldViewer& gwv)
{
    switch(gwv.GetVisibility(pt))
    {
            // Unsichtbar -> schwarz
        case VIS_INVISIBLE: GetVertex(pt).pos.color = 0.0f; break;
            // Fog of War -> abgedunkelt
        case VIS_FOW: GetVertex(pt).pos.color = float(gwv.GetNode(pt).shadow + 0x40) / float(0xFF) / 2; break;
            // Normal sichtbar
        case VIS_VISIBLE: GetVertex(pt).pos.color = float(gwv.GetNode(pt).shadow  + 0x40) / float(0xFF); break;
    }
}

void TerrainRenderer::UpdateVertexTerrain(const MapPoint pt, const GameWorldViewer& gwv)
{
    const MapNode& node = gwv.GetNode(pt);
    GetVertex(pt).terrain[0] = node.t1;
    GetVertex(pt).terrain[1] = node.t2;
}

/// erzeugt Rand-Vertex
void TerrainRenderer::UpdateBorderVertex(const MapPoint pt, const GameWorldViewer& gwv)
{
    /// @todo GetTerrainX und Co durch GetTerrainXA ausdrücken
    Vertex& vertex = GetVertex(pt);
    vertex.border[0].pos = ( GetTerrainAround(pt, 5) + GetTerrain(pt) + GetTerrainAround(pt, 4) ) / 3.0f;
    vertex.border[0].color = ( GetColor(gwv.GetNeighbour(pt, 5)) + GetColor(pt) + GetColor(gwv.GetNeighbour(pt, 4)) ) / 3.0f;

    vertex.border[1].pos = ( GetTerrainAround(pt, 3) + GetTerrain(pt) + GetTerrainAround(pt, 4) ) / 3.0f;
    vertex.border[1].color = ( GetColor(gwv.GetNeighbour(pt, 3)) + GetColor(pt) + GetColor(gwv.GetNeighbour(pt, 4)) ) / 3.0f;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  erzeugt die OpenGL-Vertices.
 *
 *  @author OLiver
 *  @author FloSoft
 */
void TerrainRenderer::GenerateOpenGL(const GameWorldViewer& gwv)
{
    width = gwv.GetWidth();
    height = gwv.GetHeight();
    LandscapeType lt = gwv.GetLandscapeType();

    GenerateVertices(gwv);

    // We have 2 triangles per map point
    unsigned int triangleCount = width * height * 2;

    // Ränder zählen
    borders.resize(width * height);
    for(MapCoord y = 0; y < height; ++y)
    {
        for(MapCoord x = 0; x < width; ++x)
        {
            MapPoint pt(x, y);
            TerrainType t1 = gwv.GetNode(pt).t1;
            TerrainType t2 = gwv.GetNode(pt).t2;
            unsigned int pos = GetVertexIdx(pt);

            if( (borders[pos].left_right[0] = TerrainData::GetEdgeType(lt, t2, t1)) )
                borders[pos].left_right_offset[0] = triangleCount++;
            if( (borders[pos].left_right[1] = TerrainData::GetEdgeType(lt, t1, t2)) )
                borders[pos].left_right_offset[1] = triangleCount++;

            t1 = gwv.GetNodeAround(pt, 3).t1;
            if( (borders[pos].right_left[0] = TerrainData::GetEdgeType(lt, t1, t2)) )
                borders[pos].right_left_offset[0] = triangleCount++;
            if( (borders[pos].right_left[1] = TerrainData::GetEdgeType(lt, t2, t1)) )
                borders[pos].right_left_offset[1] = triangleCount++;

            t1 = gwv.GetNode(pt).t1;
            t2 = gwv.GetNodeAround(pt, 5).t2;
            if( (borders[pos].top_down[0] = TerrainData::GetEdgeType(lt, t2, t1)) )
                borders[pos].top_down_offset[0] = triangleCount++;
            if( (borders[pos].top_down[1] = TerrainData::GetEdgeType(lt, t1, t2)) )
                borders[pos].top_down_offset[1] = triangleCount++;
        }
    }

    gl_vertices.resize(triangleCount);
    gl_texcoords.resize(triangleCount);
    gl_colors.resize(triangleCount);

    // Normales Terrain erzeugen
    for(MapCoord y = 0; y < height; ++y)
    {
        for(MapCoord x = 0; x < width; ++x)
        {
            MapPoint pt(x, y);
            UpdateTrianglePos(pt, gwv, false);
            UpdateTriangleColor(pt, gwv, false);
            UpdateTriangleTerrain(pt, gwv, false);
        }
    }

    // Ränder erzeugen
    for(MapCoord y = 0; y < height; ++y)
    {
        for(MapCoord x = 0; x < width; ++x)
        {
            MapPoint pt(x, y);
            UpdateBorderTrianglePos(pt, gwv, false);
            UpdateBorderTriangleColor(pt, gwv, false);
            UpdateBorderTriangleTerrain(pt, gwv, false);
        }
    }

    if(SETTINGS.video.vbo)
    {
        // Generiere und Binde den Vertex Buffer
        glGenBuffersARB(1, (GLuint*)&vbo_vertices);
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_vertices);
        glBufferDataARB(GL_ARRAY_BUFFER_ARB, gl_vertices.size() * sizeof(Triangle), &gl_vertices.front(), GL_STATIC_DRAW_ARB);
        glVertexPointer(2, GL_FLOAT, 0, NULL);

        // Generiere und Binde den Textur Koordinaten Buffer
        glGenBuffersARB(1, (GLuint*)&vbo_texcoords);
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_texcoords);
        glBufferDataARB(GL_ARRAY_BUFFER_ARB, gl_texcoords.size() * sizeof(Triangle), &gl_texcoords.front(), GL_STATIC_DRAW_ARB );
        glTexCoordPointer(2, GL_FLOAT, 0, NULL);

        // Generiere und Binde den Color Buffer
        glGenBuffersARB(1, (GLuint*)&vbo_colors);
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_colors);
        glBufferDataARB(GL_ARRAY_BUFFER_ARB, gl_colors.size() * sizeof(ColorTriangle), &gl_colors.front(), GL_STATIC_DRAW_ARB );
        glColorPointer(3, GL_FLOAT, 0, NULL);
    }
    else
    {
        glVertexPointer(2, GL_FLOAT, 0, &gl_vertices.front());
        glTexCoordPointer(2, GL_FLOAT, 0, &gl_texcoords.front());
        glColorPointer(3, GL_FLOAT, 0, &gl_colors.front());
    }
}

/// Erzeugt fertiges Dreieick für OpenGL

void TerrainRenderer::UpdateTrianglePos(const MapPoint pt, const GameWorldViewer& gwv, const bool update)
{
    unsigned int pos = GetTriangleIdx(pt);

    gl_vertices[pos].pos[0] = GetTerrainAround(pt, 4);
    gl_vertices[pos].pos[1] = GetTerrain(pt);
    gl_vertices[pos].pos[2] = GetTerrainAround(pt, 5);

    ++pos;

    gl_vertices[pos].pos[0] = GetTerrain(pt);
    gl_vertices[pos].pos[1] = GetTerrainAround(pt, 4);
    gl_vertices[pos].pos[2] = GetTerrainAround(pt, 3);

    if(update && SETTINGS.video.vbo)
    {
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_vertices);
        glBufferSubDataARB(GL_ARRAY_BUFFER_ARB, (pos - 1) * sizeof(Triangle), 2 * sizeof(Triangle), &gl_vertices[pos - 1]);
    }
}

void TerrainRenderer::UpdateTriangleColor(const MapPoint pt, const GameWorldViewer& gwv, const bool update)
{
    unsigned int pos = GetTriangleIdx(pt);

    gl_colors[pos].colors[0].r = gl_colors[pos].colors[0].g = gl_colors[pos].colors[0].b = GetColor(gwv.GetNeighbour(pt, 4));
    gl_colors[pos].colors[1].r = gl_colors[pos].colors[1].g = gl_colors[pos].colors[1].b = GetColor(pt);
    gl_colors[pos].colors[2].r = gl_colors[pos].colors[2].g = gl_colors[pos].colors[2].b = GetColor(gwv.GetNeighbour(pt, 5));

    ++pos;

    gl_colors[pos].colors[0].r = gl_colors[pos].colors[0].g = gl_colors[pos].colors[0].b = GetColor(pt);
    gl_colors[pos].colors[1].r = gl_colors[pos].colors[1].g = gl_colors[pos].colors[1].b = GetColor(gwv.GetNeighbour(pt, 4));
    gl_colors[pos].colors[2].r = gl_colors[pos].colors[2].g = gl_colors[pos].colors[2].b = GetColor(gwv.GetNeighbour(pt, 3));


    /// Bei Vertexbuffern das die Daten aktualisieren
    if(update && SETTINGS.video.vbo)
    {
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_colors);
        glBufferSubDataARB(GL_ARRAY_BUFFER_ARB, (pos - 1) * sizeof(ColorTriangle), 2 * sizeof(ColorTriangle), &gl_colors[pos - 1]);
    }
}

void TerrainRenderer::UpdateTriangleTerrain(const MapPoint pt, const GameWorldViewer& gwv, const bool update)
{
    unsigned int pos = GetTriangleIdx(pt);

    // TODO: Check those offsets for lava2-lava4
    TerrainType t1 = gwv.GetNode(pt).t1;
    bool isAnimated = TerrainData::IsAnimated(t1);
    gl_texcoords[pos].pos[0].x = (isAnimated) ? 0.4375f   : 0.45f;
    gl_texcoords[pos].pos[0].y = (isAnimated) ? 0.0f      : 0.45f;
    gl_texcoords[pos].pos[1].y = (isAnimated) ? 0.445312f : 0.0f;
    gl_texcoords[pos].pos[1].x = (isAnimated) ? 0.0f      : 0.225f;
    gl_texcoords[pos].pos[2].x = (isAnimated) ? 0.84375f  : 0.0f;
    gl_texcoords[pos].pos[2].y = (isAnimated) ? 0.445312f : 0.45f;

    ++pos;

    TerrainType t2 = gwv.GetNode(pt).t2;
    isAnimated = TerrainData::IsAnimated(t2);
    gl_texcoords[pos].pos[0].x = (isAnimated) ? 0.4375f   : 0.0f;
    gl_texcoords[pos].pos[0].y = (isAnimated) ? 0.859375f : 0.0f;
    gl_texcoords[pos].pos[1].x = (isAnimated) ? 0.84375f  : 0.235f;
    gl_texcoords[pos].pos[1].y = (isAnimated) ? 0.445312f : 0.45f;
    gl_texcoords[pos].pos[2].x = (isAnimated) ? 0.0f      : 0.47f;
    gl_texcoords[pos].pos[2].y = (isAnimated) ? 0.445312f : 0.0f;


    /// Bei Vertexbuffern das die Daten aktualisieren
    if(update && SETTINGS.video.vbo)
    {
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_texcoords);
        glBufferSubDataARB(GL_ARRAY_BUFFER_ARB, (pos - 1) * sizeof(Triangle), 2 * sizeof(Triangle), &gl_texcoords[pos - 1]);
    }
}

/// Erzeugt die Dreiecke für die Ränder
void TerrainRenderer::UpdateBorderTrianglePos(const MapPoint pt, const GameWorldViewer& gwv, const bool update)
{
    unsigned int pos = GetVertexIdx(pt);

    // Für VBO-Aktualisierung:
    // Erzeugte Ränder zählen
    unsigned count_borders = 0;
    // Erstes Offset merken
    unsigned first_offset = 0;


    // Rand links - rechts
    for(unsigned char i = 0; i < 2; ++i)
    {
        if(!borders[pos].left_right[i])
            continue;
        unsigned int offset = borders[pos].left_right_offset[i];

        if(!first_offset)
            first_offset = offset;

        gl_vertices[offset].pos[i ? 0 : 2] = GetTerrain(pt);
        gl_vertices[offset].pos[1        ] = GetTerrainAround(pt, 4);
        gl_vertices[offset].pos[i ? 2 : 0] = GetB(pt, i);

        ++count_borders;
    }

    // Rand rechts - links
    for(unsigned char i = 0; i < 2; ++i)
    {
        if(!borders[pos].right_left[i])
            continue;
        unsigned int offset = borders[pos].right_left_offset[i];

        if(!first_offset)
            first_offset = offset;

        gl_vertices[offset].pos[i ? 2 : 0] = GetTerrainAround(pt, 4);
        gl_vertices[offset].pos[1        ] = GetTerrainAround(pt, 3);

        if(i == 0)
            gl_vertices[offset].pos[2] = GetB(pt, 1);
        else
            gl_vertices[offset].pos[0] = GetBAround(pt, 0, 3);

        ++count_borders;
    }

    // Rand oben - unten
    for(unsigned char i = 0; i < 2; ++i)
    {
        if(!borders[pos].top_down[i])
            continue;
        unsigned int offset = borders[pos].top_down_offset[i];

        if(!first_offset)
            first_offset = offset;

        gl_vertices[offset].pos[i ? 2 : 0] = GetTerrainAround(pt, 5);
        gl_vertices[offset].pos[1        ] = GetTerrainAround(pt, 4);

        if(i == 0)
            gl_vertices[offset].pos[2] = GetB(pt, i);
        else
            gl_vertices[offset].pos[0] = GetBAround(pt, i, 5); //x - i + i * rt, y + i, i

        ++count_borders;
    }

    /// Bei Vertexbuffern das die Daten aktualisieren
    if(update && SETTINGS.video.vbo)
    {
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_vertices);
        glBufferSubDataARB(GL_ARRAY_BUFFER_ARB, first_offset * sizeof(Triangle), count_borders * sizeof(Triangle), &gl_vertices[first_offset]);
    }
}

void TerrainRenderer::UpdateBorderTriangleColor(const MapPoint pt, const GameWorldViewer& gwv, const bool update)
{
    unsigned int pos = GetVertexIdx(pt);

    // Für VBO-Aktualisierung:
    // Erzeugte Ränder zählen
    unsigned count_borders = 0;
    // Erstes Offset merken
    unsigned first_offset = 0;


    // Rand links - rechts
    for(unsigned char i = 0; i < 2; ++i)
    {
        if(!borders[pos].left_right[i])
            continue;
        unsigned int offset = borders[pos].left_right_offset[i];

        if(!first_offset)
            first_offset = offset;

        gl_colors[offset].colors[i ? 0 : 2].r = gl_colors[offset].colors[i ? 0 : 2].g = gl_colors[offset].colors[i ? 0 : 2].b = GetColor(pt);
        gl_colors[offset].colors[1        ].r = gl_colors[offset].colors[1        ].g = gl_colors[offset].colors[1        ].b = GetColor(gwv.GetNeighbour(pt, 4));
        gl_colors[offset].colors[i ? 2 : 0].r = gl_colors[offset].colors[i ? 2 : 0].g = gl_colors[offset].colors[i ? 2 : 0].b = GetBColor(pt, i);

        ++count_borders;
    }

    // Rand rechts - links
    for(unsigned char i = 0; i < 2; ++i)
    {
        if(!borders[pos].right_left[i])
            continue;
        unsigned int offset = borders[pos].right_left_offset[i];

        if(!first_offset)
            first_offset = offset;

        gl_colors[offset].colors[i ? 2 : 0].r = gl_colors[offset].colors[i ? 2 : 0].g = gl_colors[offset].colors[i ? 2 : 0].b = GetColor(gwv.GetNeighbour(pt, 4));
        gl_colors[offset].colors[1        ].r = gl_colors[offset].colors[1        ].g = gl_colors[offset].colors[1        ].b = GetColor(gwv.GetNeighbour(pt, 3));
        MapPoint pt2(pt.x + i, pt.y);
        if(pt2.x >= width)
            pt2.x -= width;
        gl_colors[offset].colors[i ? 0 : 2].r = gl_colors[offset].colors[i ? 0 : 2].g = gl_colors[offset].colors[i ? 0 : 2].b = GetBColor(pt2, i ? 0 : 1);

        ++count_borders;
    }

    // Rand oben - unten
    for(unsigned char i = 0; i < 2; ++i)
    {
        if(!borders[pos].top_down[i])
            continue;
        unsigned int offset = borders[pos].top_down_offset[i];

        if(!first_offset)
            first_offset = offset;

        gl_colors[offset].colors[i ? 2 : 0].r = gl_colors[offset].colors[i ? 2 : 0].g = gl_colors[offset].colors[i ? 2 : 0].b = GetColor(gwv.GetNeighbour(pt, 5));
        gl_colors[offset].colors[1        ].r = gl_colors[offset].colors[1        ].g = gl_colors[offset].colors[1        ].b = GetColor(gwv.GetNeighbour(pt, 4));

        if(i == 0)
            gl_colors[offset].colors[2].r = gl_colors[offset].colors[2].g = gl_colors[offset].colors[2].b = GetBColor(pt, i);
        else
            gl_colors[offset].colors[0].r = gl_colors[offset].colors[0].g = gl_colors[offset].colors[0].b = GetBColor(gwv.GetNeighbour(pt, 5), i);

        ++count_borders;
    }

    /// Bei Vertexbuffern das die Daten aktualisieren
    if(update && SETTINGS.video.vbo)
    {
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_colors);
        glBufferSubDataARB(GL_ARRAY_BUFFER_ARB, first_offset * sizeof(ColorTriangle), count_borders * sizeof(ColorTriangle), &gl_colors[first_offset]);
    }
}

void TerrainRenderer::UpdateBorderTriangleTerrain(const MapPoint pt, const GameWorldViewer& gwv, const bool update)
{
    unsigned int pos = GetVertexIdx(pt);

    // Für VBO-Aktualisierung:
    // Erzeugte Ränder zählen
    unsigned count_borders = 0;
    // Erstes Offset merken
    unsigned first_offset = 0;


    // Rand links - rechts
    for(unsigned char i = 0; i < 2; ++i)
    {
        if(borders[pos].left_right[i])
        {
            unsigned int offset = borders[pos].left_right_offset[i];

            if(!first_offset)
                first_offset = offset;

            gl_texcoords[offset].pos[i ? 0 : 2] = PointF(0.0f, 0.0f);
            gl_texcoords[offset].pos[1        ] = PointF(1.0f, 0.0f);
            gl_texcoords[offset].pos[i ? 2 : 0] = PointF(0.5f, 1.0f);

            ++count_borders;
        }
    }

    // Rand rechts - links
    for(unsigned char i = 0; i < 2; ++i)
    {
        if(borders[pos].right_left[i])
        {
            unsigned int offset = borders[pos].right_left_offset[i];

            if(!first_offset)
                first_offset = offset;

            gl_texcoords[offset].pos[i ? 2 : 0] = PointF(0.0f, 0.0f);
            gl_texcoords[offset].pos[1        ] = PointF(1.0f, 0.0f);
            gl_texcoords[offset].pos[i ? 0 : 2] = PointF(0.5f, 1.0f);

            ++count_borders;
        }
    }

    // Rand oben - unten
    for(unsigned char i = 0; i < 2; ++i)
    {
        if(borders[pos].top_down[i])
        {
            unsigned int offset = borders[pos].top_down_offset[i];

            if(!first_offset)
                first_offset = offset;

            gl_texcoords[offset].pos[i ? 2 : 0] = PointF(0.0f, 0.0f);
            gl_texcoords[offset].pos[1        ] = PointF(1.0f, 0.0f);
            gl_texcoords[offset].pos[i ? 0 : 2] = PointF(0.5f, 1.0f);

            ++count_borders;
        }
    }

    /// Bei Vertexbuffern das die Daten aktualisieren
    if(update && SETTINGS.video.vbo)
    {
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_texcoords);
        glBufferSubDataARB(GL_ARRAY_BUFFER_ARB, first_offset * sizeof(Triangle), count_borders * sizeof(Triangle), &gl_texcoords[first_offset]);
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  zeichnet den Kartenausschnitt.
 *
 *  @author OLiver
 *  @author FloSoft
 */
void TerrainRenderer::Draw(const GameWorldView& gwv, unsigned int* water)
{
    assert(!gl_vertices.empty());
    assert(!borders.empty());

    /*  if ((gwv.GetXOffset() == gwv.terrain_last_xoffset) && (gwv.GetYOffset() == gwv.terrain_last_yoffset) && (gwv.terrain_list != 0) && (GAMECLIENT.GetGlobalAnimation(4, 5, 4, 0) == gwv.terrain_last_global_animation))
        {
            glCallList(gwv.terrain_list);
            *water = gwv.terrain_last_water;
            return;
        }

        gwv.terrain_last_xoffset = gwv.GetXOffset();
        gwv.terrain_last_yoffset = gwv.GetYOffset();
        gwv.terrain_last_global_animation = GAMECLIENT.GetGlobalAnimation(4, 5, 4, 0);

        if (gwv.terrain_list == 0)
            gwv.terrain_list = glGenLists(1);

        glNewList(gwv.terrain_list, GL_COMPILE_AND_EXECUTE);*/

    // nach Texture in Listen sortieren
    boost::array< std::vector<MapTile>, TT_COUNT> sorted_textures;
    boost::array< std::vector<BorderTile>, 5> sorted_borders;
    PreparedRoads sorted_roads;

    Point<int> lastOffset(0, 0);
 
    // Beim zeichnen immer nur beginnen, wo man auch was sieht
    for(int y = gwv.GetFirstPt().y; y < gwv.GetLastPt().y; ++y)
    {
        unsigned char lastTerrain = 255;
        unsigned char lastBorder  = 255;

        for(int x = gwv.GetFirstPt().x; x < gwv.GetLastPt().x; ++x)
        {
            Point<int> posOffset;
            MapPoint tP = ConvertCoords(Point<int>(x, y), &posOffset);

            TerrainType t = gwv.GetGameWorldViewer()->GetNode(tP).t1;
            if(posOffset != lastOffset)
                lastTerrain = 255;

            if(t == lastTerrain && tP != MapPoint(0, 0))
                ++sorted_textures[t].back().count;
            else
            {
                MapTile tmp(GetTriangleIdx(tP), posOffset);
                sorted_textures[t].push_back(tmp);
                lastTerrain = t;
            }

            t = gwv.GetGameWorldViewer()->GetNode(tP).t2;

            if(t == lastTerrain)
                ++sorted_textures[t].back().count;
            else
            {
                MapTile tmp(GetTriangleIdx(tP) + 1, posOffset);
                sorted_textures[t].push_back(tmp);
            }

            lastTerrain = t;

            const Borders& curBorders = borders[GetVertexIdx(tP)];
            boost::array<unsigned char, 6> tiles =
            {{
                curBorders.left_right[0],
                curBorders.left_right[1],
                curBorders.right_left[0],
                curBorders.right_left[1],
                curBorders.top_down[0],
                curBorders.top_down[1]
            }};

            // Offsets into gl_* arrays
            boost::array<unsigned, 6> offsets = 
            {{
                curBorders.left_right_offset[0],
                curBorders.left_right_offset[1],
                curBorders.right_left_offset[0],
                curBorders.right_left_offset[1],
                curBorders.top_down_offset[0],
                curBorders.top_down_offset[1]
            }};

            for(unsigned char i = 0; i < 6; ++i)
            {
                if(!tiles[i])
                    continue;
                if(tiles[i] == lastBorder)
                {
                    BorderTile& curTile = sorted_borders[lastBorder - 1].back();
                    // Check that we did not wrap around the map and the expected offset matches
                    if(curTile.tileOffset + curTile.count == offsets[i])
                    {
                        ++curTile.count;
                        continue;
                    }
                }
                lastBorder = tiles[i];
                BorderTile tmp(offsets[i], posOffset);
                sorted_borders[lastBorder - 1].push_back(tmp);
            }

            PrepareWaysPoint(sorted_roads, gwv, tP, posOffset);

            lastOffset = posOffset;
        }
    }

    if (water)
    {
        unsigned water_count = 0;

        for(unsigned char t = 0; t < TT_COUNT; ++t){
            if(!TerrainData::IsWater(TerrainType(t)))
                continue;
            for(std::vector<MapTile>::iterator it = sorted_textures[t].begin(); it != sorted_textures[t].end(); ++it)
            {
                water_count += it->count;
            }
        }

        PointI diff = gwv.GetLastPt() - gwv.GetFirstPt();
        if( diff.x && diff.y )
            *water = 50 * water_count / ( diff.x * diff.y );
        else
            *water = 0;
    }

    lastOffset = PointI(0, 0);

    if(SETTINGS.video.vbo)
    {
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_vertices);
        glVertexPointer(2, GL_FLOAT, 0, NULL);

        glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_texcoords);
        glTexCoordPointer(2, GL_FLOAT, 0, NULL);

        glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_colors);
        glColorPointer(3, GL_FLOAT, 0, NULL);
    }
    else
    {
        glVertexPointer(2, GL_FLOAT, 0, &gl_vertices.front());
        glTexCoordPointer(2, GL_FLOAT, 0, &gl_texcoords.front());
        glColorPointer(3, GL_FLOAT, 0, &gl_colors.front());
    }

    // Arrays aktivieren
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);

    // Modulate2x
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);
    glTexEnvf(GL_TEXTURE_ENV, GL_RGB_SCALE_EXT, 2.0f);

    // Verschieben gem#ß x und y offset
    glTranslatef( float(-gwv.GetXOffset()), float(-gwv.GetYOffset()), 0.0f);

    // Alphablending aus
    glDisable(GL_BLEND);

    for(unsigned char t = 0; t < TT_COUNT; ++t)
    {
        if(sorted_textures[t].empty())
            continue;
        unsigned animationFrame;
        TerrainType tt = TerrainType(t);
        if(TerrainData::IsLava(tt))
            animationFrame = GAMECLIENT.GetGlobalAnimation(TerrainData::GetFrameCount(tt), 5, 4, 0);
        else if(TerrainData::IsWater(tt))
            animationFrame = GAMECLIENT.GetGlobalAnimation(TerrainData::GetFrameCount(tt), 5, 2, 0);
        else
            animationFrame = 0;

        VIDEODRIVER.BindTexture(LOADER.GetTerrainTexture(tt, animationFrame).GetTexture());

        for(std::vector<MapTile>::iterator it = sorted_textures[t].begin(); it != sorted_textures[t].end(); ++it)
        {
            if(it->posOffset != lastOffset)
            {
                PointI trans = it->posOffset - lastOffset;
                glTranslatef( float(trans.x), float(trans.y), 0.0f);
                lastOffset = it->posOffset;
            }

            assert(it->tileOffset + it->count <= width * height * 2u);
            glDrawArrays(GL_TRIANGLES, it->tileOffset * 3, it->count * 3); // Arguments are in Elements. 1 triangle has 3 values
        }
    }

    glEnable(GL_BLEND);

    glLoadIdentity();
    glTranslatef( float(-gwv.GetXOffset()), float(-gwv.GetYOffset()), 0.0f);

    lastOffset = PointI(0, 0);
    for(unsigned short i = 0; i < 5; ++i)
    {
        if(sorted_borders[i].empty())
            continue;
        VIDEODRIVER.BindTexture(GetImage(borders, i)->GetTexture());

        for(std::vector<BorderTile>::iterator it = sorted_borders[i].begin(); it != sorted_borders[i].end(); ++it)
        {
            if(it->posOffset != lastOffset)
            {
                PointI trans = it->posOffset - lastOffset;
                glTranslatef( float(trans.x), float(trans.y), 0.0f);
                lastOffset = it->posOffset;
            }
            assert(it->tileOffset + it->count <= gl_vertices.size());
            glDrawArrays(GL_TRIANGLES, it->tileOffset * 3, it->count * 3); // Arguments are in Elements. 1 triangle has 3 values
        }
    }
    glLoadIdentity();

    DrawWays(sorted_roads);

    // Wieder zurück ins normale modulate
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
}


///////////////////////////////////////////////////////////////////////////////
/**
 *  Konvertiert die Koordinaten.
 *
 *  @param[in,out] x  Die X-Koordinate
 *  @param[in,out] y  Die Y-Koordinate
 *  @param[out]    xo Das X-Offset
 *  @param[out]    yo Das Y-Offset
 *
 *  @author OLiver
 */
MapPoint TerrainRenderer::ConvertCoords(const PointI pt, Point<int>* offset) const
{
    MapPoint ptOut;
	if (pt.x < 0)
	{
	    if (offset)
	    	offset->x = -TR_W * width;
		ptOut.x = static_cast<MapCoord>(width + (pt.x % width));
	} else
	{
	    if (offset)
	    	offset->x = (pt.x / width) * (TR_W * width);
		ptOut.x = static_cast<MapCoord>(pt.x % width);
	}
	
	if (pt.y < 0)
	{
	    if (offset)
	    	offset->y = -TR_H * height;
		ptOut.y = static_cast<MapCoord>(height + (pt.y % height));
	} else
	{
	    if (offset)
	    	offset->y = (pt.y / height) * (TR_H * height);
		ptOut.y = static_cast<MapCoord>(pt.y % height);
	}
    assert(ptOut.x >= 0 && ptOut.x < width);
    assert(ptOut.y >= 0 && ptOut.y < height);
    return ptOut;
}

struct GL_T2F_C3F_V3F_Struct
{
    GLfloat tx, ty;
    GLfloat r, g, b;
    GLfloat x, y, z;
};

void TerrainRenderer::PrepareWaysPoint(PreparedRoads& sorted_roads, const GameWorldView& gwv, MapPoint t, PointI offset)
{
    PointI startPos = PointI(GetTerrain(t)) - gwv.GetOffset() + offset;

    Visibility visibility = gwv.GetGameWorldViewer()->GetVisibility(t);

	int totalWidth  = gwv.GetGameWorldViewer()->GetWidth()  * TR_W;
	int totalHeight = gwv.GetGameWorldViewer()->GetHeight() * TR_H;

    // Wegtypen für die drei Richtungen
    for(unsigned dir = 0; dir < 3; ++dir)
    {
        unsigned char type = gwv.GetGameWorldViewer()->GetVisibleRoad(t, dir, visibility);
        if (!type)
            continue;
        MapPoint ta = gwv.GetGameWorldViewer()->GetNeighbour(t, 3 + dir);

        PointI endPos = PointI(GetTerrain(ta)) - gwv.GetOffset() + offset;
        PointI diff = startPos - endPos;

        // Gehen wir über einen Kartenrand (horizontale Richung?)
        if(std::abs(diff.x) >= totalWidth / 2)
        {
            if(std::abs(endPos.x - totalWidth - startPos.x) < std::abs(diff.x))
                endPos.x -= totalWidth;
            else
                endPos.x += totalWidth;
        }
        // Und dasselbe für vertikale Richtung
        if(std::abs(diff.y) >= totalHeight / 2)
        {
            if(std::abs(endPos.y - totalHeight - startPos.y) < std::abs(diff.y))
                endPos.y -= totalHeight;
            else
                endPos.y += totalHeight;
        }

        --type;

        // Wegtypen "konvertieren"
        switch(type)
        {
            case RoadSegment::RT_DONKEY:
            case RoadSegment::RT_NORMAL:
            {
                TerrainType t1 = gwv.GetGameWorldViewer()->GetTerrainAround(t, dir + 2);
                TerrainType t2 = gwv.GetGameWorldViewer()->GetTerrainAround(t, dir + 3);

                // Prüfen, ob Bergwege gezeichnet werden müssen, indem man guckt, ob der Weg einen
                // Berg "streift" oder auch eine Bergwiese
                if(TerrainData::IsMountain(t1) || TerrainData::IsMountain(t2))
                    type = 3;

                break;
            }
            case RoadSegment::RT_BOAT:
                type = 2;
                break;
        }

        sorted_roads[type].push_back(
            PreparedRoad(type, startPos, endPos, GetColor(t), GetColor(ta), dir)
        );
    }
}

void TerrainRenderer::DrawWays(const PreparedRoads& sorted_roads)
{
    // 2D Array: [3][4]
    static const boost::array<PointI, 12> begin_end_coords =
    {{
        PointI(-3, -3),
        PointI(-3,  3),
        PointI(-3,  3),
        PointI(-3, -3),

        PointI( 3, -3),
        PointI(-3,  3),
        PointI(-3,  3),
        PointI( 3, -3),

        PointI( 3,  3),
        PointI(-3, -3),
        PointI(-3, -3),
        PointI( 3,  3)
    }};

    if (SETTINGS.video.vbo)
    {
        // unbind VBO
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
    }

    int type = 0;
    for (PreparedRoads::const_iterator itRoad = sorted_roads.begin(); itRoad != sorted_roads.end(); ++itRoad, ++type)
    {
        unsigned int i = 0;
        boost::scoped_array<GL_T2F_C3F_V3F_Struct> tmp(new GL_T2F_C3F_V3F_Struct[itRoad->size() * 4]);

        for (std::vector<PreparedRoad>::const_iterator it = itRoad->begin(); it != itRoad->end(); ++it)
        {
            assert(it->dir < 3); // begin_end_coords has 3 dir entries
            tmp[i].tx = 0.0f;
            tmp[i].ty = 0.0f;

            tmp[i].r = it->color1;
            tmp[i].g = it->color1;
            tmp[i].b = it->color1;

            PointI tmpP = it->pos + begin_end_coords[it->dir * 4];
            tmp[i].x = GLfloat(tmpP.x);
            tmp[i].y = GLfloat(tmpP.y);
            tmp[i].z = 0.0f;

            i++;

            tmp[i].tx = 0.0f;
            tmp[i].ty = 1.0f;

            tmp[i].r = it->color1;
            tmp[i].g = it->color1;
            tmp[i].b = it->color1;

            tmpP = it->pos + begin_end_coords[it->dir * 4 + 1];
            tmp[i].x = GLfloat(tmpP.x);
            tmp[i].y = GLfloat(tmpP.y);
            tmp[i].z = 0.0f;

            i++;

            tmp[i].tx = 0.78f;
            tmp[i].ty = 1.0f;

            tmp[i].r = it->color2;
            tmp[i].g = it->color2;
            tmp[i].b = it->color2;

            tmpP = it->pos2 + begin_end_coords[it->dir * 4 + 2];
            tmp[i].x = GLfloat(tmpP.x);
            tmp[i].y = GLfloat(tmpP.y);
            tmp[i].z = 0.0f;

            i++;

            tmp[i].tx = 0.78f;
            tmp[i].ty = 0.0f;

            tmp[i].r = it->color2;
            tmp[i].g = it->color2;
            tmp[i].b = it->color2;

            tmpP = it->pos2 + begin_end_coords[it->dir * 4 + 3];
            tmp[i].x = GLfloat(tmpP.x);
            tmp[i].y = GLfloat(tmpP.y);
            tmp[i].z = 0.0f;

            i++;
        }

        glInterleavedArrays(GL_T2F_C3F_V3F, 0, tmp.get());
        VIDEODRIVER.BindTexture(GetImage(roads, type)->GetTexture());
        glDrawArrays(GL_QUADS, 0, i);
    }
}

void TerrainRenderer::AltitudeChanged(const MapPoint pt, const GameWorldViewer& gwv)
{
    // den selbst sowieso die Punkte darum updaten, da sich bei letzteren die Schattierung geändert haben könnte
    UpdateVertexPos(pt, gwv);
    UpdateVertexColor(pt, gwv);

    for(unsigned i = 0; i < 6; ++i)
        UpdateVertexColor(gwv.GetNeighbour(pt, i), gwv);


    // und für die Ränder
    UpdateBorderVertex(pt, gwv);

    for(unsigned i = 0; i < 6; ++i)
        UpdateBorderVertex(gwv.GetNeighbour(pt, i), gwv);

    // den selbst sowieso die Punkte darum updaten, da sich bei letzteren die Schattierung geändert haben könnte
    UpdateTrianglePos(pt, gwv, true);
    UpdateTriangleColor(pt, gwv, true);

    for(unsigned i = 0; i < 6; ++i)
    {
        UpdateTrianglePos(gwv.GetNeighbour(pt, i), gwv, true);
        UpdateTriangleColor(gwv.GetNeighbour(pt, i), gwv, true);
    }


    // Auch im zweiten Kreis drumherum die Dreiecke neu berechnen, da die durch die Schattenänderung der umliegenden
    // Punkte auch geändert werden könnten
    for(unsigned i = 0; i < 12; ++i)
        UpdateTriangleColor(gwv.GetNeighbour2(pt, i), gwv, true);


    // und für die Ränder
    UpdateBorderTrianglePos(pt, gwv, true);
    UpdateBorderTriangleColor(pt, gwv, true);

    for(unsigned i = 0; i < 6; ++i)
    {
        UpdateBorderTrianglePos(gwv.GetNeighbour(pt, i), gwv, true);
        UpdateBorderTriangleColor(gwv.GetNeighbour(pt, i), gwv, true);
    }

    for(unsigned i = 0; i < 12; ++i)
        UpdateBorderTriangleColor(gwv.GetNeighbour2(pt, i), gwv, true);
}

void TerrainRenderer::VisibilityChanged(const MapPoint pt, const GameWorldViewer& gwv)
{
    /// Noch kein Terrain gebaut? abbrechen
    if(vertices.empty())
        return;

    UpdateVertexColor(pt, gwv);
    for(unsigned i = 0; i < 6; ++i)
        UpdateVertexColor(gwv.GetNeighbour(pt, i), gwv);

    // und für die Ränder
    UpdateBorderVertex(pt, gwv);
    for(unsigned i = 0; i < 6; ++i)
        UpdateBorderVertex(gwv.GetNeighbour(pt, i), gwv);

    // den selbst sowieso die Punkte darum updaten, da sich bei letzteren die Schattierung geändert haben könnte
    UpdateTriangleColor(pt, gwv, true);
    for(unsigned i = 0; i < 6; ++i)
        UpdateTriangleColor(gwv.GetNeighbour(pt, i), gwv, true);

    // und für die Ränder
    UpdateBorderTriangleColor(pt, gwv, true);
    for(unsigned i = 0; i < 6; ++i)
        UpdateBorderTriangleColor(gwv.GetNeighbour(pt, i), gwv, true);
}


void TerrainRenderer::UpdateAllColors(const GameWorldViewer& gwv)
{
    for(MapCoord y = 0; y < height; ++y)
        for(MapCoord x = 0; x < width; ++x)
            UpdateVertexColor(MapPoint(x, y), gwv);

    for(MapCoord y = 0; y < height; ++y)
        for(MapCoord x = 0; x < width; ++x)
            UpdateBorderVertex(MapPoint(x, y), gwv);

    for(MapCoord y = 0; y < height; ++y)
        for(MapCoord x = 0; x < width; ++x)
            UpdateTriangleColor(MapPoint(x, y), gwv, false);

    for(MapCoord y = 0; y < height; ++y)
        for(MapCoord x = 0; x < width; ++x)
            UpdateBorderTriangleColor(MapPoint(x, y), gwv, false);

    if(SETTINGS.video.vbo)
    {
        // Generiere und Binde den Color Buffer
        glGenBuffersARB(1, (GLuint*)&vbo_colors);
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_colors);
        glBufferDataARB(GL_ARRAY_BUFFER_ARB, gl_colors.size() * sizeof(ColorTriangle), &gl_colors.front(), GL_STATIC_DRAW_ARB );
        glColorPointer(3, GL_FLOAT, 0, NULL);
    }
    else
        glColorPointer(3, GL_FLOAT, 0, &gl_colors.front());
}
