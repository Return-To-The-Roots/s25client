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
#include "defines.h" // IWYU pragma: keep
#include "TerrainRenderer.h"

#include "drivers/VideoDriverWrapper.h"
#include "gameData/MapConsts.h"
#include "Settings.h"
#include "GameClient.h"
#include "world/MapGeometry.h"
#include "world/GameWorldView.h"
#include "gameData/TerrainData.h"
#include "ExtensionList.h"
#include "Loader.h"
#include "helpers/roundToNextPow2.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "libsiedler2/src/ArchivInfo.h"
#include "ogl/oglIncludes.h"
#include <boost/smart_ptr/scoped_array.hpp>
#include <cstdlib>

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

/* Terrain rendering works like that:
 * Every point is associated with 2 triangles:
 *    x____
 *   /0\ 1/
 *  /___\/
 *
 * So each point in Vertex-Array stores:
 *      its coordinates, the 2 textures, the color (shade) and border information (TODO: What are they?)
 * 
 * For OGL we have one entry per triangle in:
 *  - gl_vertices: Vertex (position) (All 3 coordinates of a triangle)
 *  - gl_texCoords: Texture coordinates for the triangle
 *  - gl_colors: Color (shade) at each point of the triangle
 * 
 * Drawing then binds a texture and draws all adjacent vertices with the same texture in one call by
 * providing an index and a count into the above arrays.
 */

TerrainRenderer::TerrainRenderer() :
	width(0), height(0),
    vbo_vertices(0), vbo_texcoords(0), vbo_colors(0), vboBuffersUsed(false)
{}

TerrainRenderer::~TerrainRenderer()
{
    if(vboBuffersUsed)
    {
        glDeleteBuffersARB(1, (const GLuint*)&vbo_vertices);
        glDeleteBuffersARB(1, (const GLuint*)&vbo_texcoords);
        glDeleteBuffersARB(1, (const GLuint*)&vbo_colors);
    }
}

TerrainRenderer::PointF TerrainRenderer::GetTerrainAround(MapPoint pt, const unsigned dir)
{
    PointI ptNb = GetNeighbour(PointI(pt), Direction::fromInt(dir));

    PointI offset;
    MapPoint t = ConvertCoords(ptNb, &offset);

    return GetNodePos(t) + PointF(offset);
}

TerrainRenderer::PointF TerrainRenderer::GetBAround(const MapPoint pt, const unsigned char triangle, const unsigned char dir)
{
    PointI ptNb = GetNeighbour(PointI(pt), Direction::fromInt(dir));

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
    int x = pt.x * TR_W;
    if(pt.y & 1)
        x += TR_W / 2;
    GetVertex(pt).pos.x = float(x);
    GetVertex(pt).pos.y = float(pt.y * TR_H - HEIGHT_FACTOR * gwv.GetNode(pt).altitude + HEIGHT_FACTOR * 0x0A );
}


void TerrainRenderer::UpdateVertexColor(const MapPoint pt, const GameWorldViewer& gwv)
{
    float shadow = static_cast<float>(gwv.GetNode(pt).shadow);
    float clr = -1.f/(256.f*256.f) * shadow*shadow + 1.f/90.f * shadow + 0.38f;
    switch(gwv.GetVisibility(pt))
    {
    case VIS_INVISIBLE:
        // Unsichtbar -> schwarz
        GetVertex(pt).color = 0.0f;
        break;
    case VIS_FOW:
        // Fog of War -> abgedunkelt
        GetVertex(pt).color = clr / 4.f;
        break;
    case VIS_VISIBLE:
        // Normal sichtbar
        GetVertex(pt).color = clr / 2.f;
        break;
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
    vertex.borderPos[0] = ( GetTerrainAround(pt, 5) + GetNodePos(pt) + GetTerrainAround(pt, 4) ) / 3.0f;
    vertex.borderColor[0] = ( GetColor(gwv.GetNeighbour(pt, 5)) + GetColor(pt) + GetColor(gwv.GetNeighbour(pt, 4)) ) / 3.0f;

    vertex.borderPos[1] = ( GetTerrainAround(pt, 3) + GetNodePos(pt) + GetTerrainAround(pt, 4) ) / 3.0f;
    vertex.borderColor[1] = ( GetColor(gwv.GetNeighbour(pt, 3)) + GetColor(pt) + GetColor(gwv.GetNeighbour(pt, 4)) ) / 3.0f;
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
            TerrainType t1 = gwv.GetNode(pt).t1; //-V807
            TerrainType t2 = gwv.GetNode(pt).t2;
            unsigned int pos = GetVertexIdx(pt);

            if( (borders[pos].left_right[0] = TerrainData::GetEdgeType(lt, t2, t1)) )
                borders[pos].left_right_offset[0] = triangleCount++;
            if( (borders[pos].left_right[1] = TerrainData::GetEdgeType(lt, t1, t2)) )
                borders[pos].left_right_offset[1] = triangleCount++;

            t1 = gwv.GetNeighbourNode(pt, 3).t1;
            if( (borders[pos].right_left[0] = TerrainData::GetEdgeType(lt, t1, t2)) )
                borders[pos].right_left_offset[0] = triangleCount++;
            if( (borders[pos].right_left[1] = TerrainData::GetEdgeType(lt, t2, t1)) )
                borders[pos].right_left_offset[1] = triangleCount++;

            t1 = gwv.GetNode(pt).t1;
            t2 = gwv.GetNeighbourNode(pt, 5).t2;
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

        vboBuffersUsed = true;
    }
    else
    {
        glVertexPointer(2, GL_FLOAT, 0, &gl_vertices.front());
        glTexCoordPointer(2, GL_FLOAT, 0, &gl_texcoords.front());
        glColorPointer(3, GL_FLOAT, 0, &gl_colors.front());
    }
}

/// Erzeugt fertiges Dreieick für OpenGL

void TerrainRenderer::UpdateTrianglePos(const MapPoint pt, const GameWorldViewer&  /*gwv*/, const bool update)
{
    unsigned int pos = GetTriangleIdx(pt);

    gl_vertices[pos][0] = GetTerrainAround(pt, 4);
    gl_vertices[pos][1] = GetNodePos(pt);
    gl_vertices[pos][2] = GetTerrainAround(pt, 5);

    ++pos;

    gl_vertices[pos][0] = GetNodePos(pt);
    gl_vertices[pos][1] = GetTerrainAround(pt, 4);
    gl_vertices[pos][2] = GetTerrainAround(pt, 3);

    if(update && vboBuffersUsed)
    {
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_vertices);
        glBufferSubDataARB(GL_ARRAY_BUFFER_ARB, (pos - 1) * sizeof(Triangle), 2 * sizeof(Triangle), &gl_vertices[pos - 1]);
    }
}

void TerrainRenderer::UpdateTriangleColor(const MapPoint pt, const GameWorldViewer& gwv, const bool update)
{
    unsigned int pos = GetTriangleIdx(pt);

    Color& clr0 = gl_colors[pos][0];
    Color& clr1 = gl_colors[pos][1];
    Color& clr2 = gl_colors[pos][2];
    clr0.r = clr0.g = clr0.b = GetColor(gwv.GetNeighbour(pt, 4));
    clr1.r = clr1.g = clr1.b = GetColor(pt);
    clr2.r = clr2.g = clr2.b = GetColor(gwv.GetNeighbour(pt, 5));

    ++pos;

    Color& clr3 = gl_colors[pos][0];
    Color& clr4 = gl_colors[pos][1];
    Color& clr5 = gl_colors[pos][2];
    clr3.r = clr3.g = clr3.b = GetColor(pt);
    clr4.r = clr4.g = clr4.b = GetColor(gwv.GetNeighbour(pt, 4));
    clr5.r = clr5.g = clr5.b = GetColor(gwv.GetNeighbour(pt, 3));


    /// Bei Vertexbuffern das die Daten aktualisieren
    if(update && vboBuffersUsed)
    {
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_colors);
        glBufferSubDataARB(GL_ARRAY_BUFFER_ARB, (pos - 1) * sizeof(ColorTriangle), 2 * sizeof(ColorTriangle), &gl_colors[pos - 1]);
    }
}

void TerrainRenderer::UpdateTriangleTerrain(const MapPoint pt, const GameWorldViewer& gwv, const bool update)
{
    const MapNode& node = gwv.GetNode(pt);
    unsigned int pos = GetTriangleIdx(pt);

    Triangle& texCoord = gl_texcoords[pos];
    if(!TerrainData::IsAnimated(node.t1))
    {
        texCoord[1].x = 0.225f;
        texCoord[1].y = 0.f;
        texCoord[2].x = 0.f; //-V807
        texCoord[2].y = 0.45f;
        texCoord[0].x = 0.45f;
        texCoord[0].y = texCoord[2].y;
    }else
    {
        // We use the full texture as it already consists of 2 triangles
        // But we need to make sure to only use the correct part of it (texture sizes are powers of 2)
        // Note: Better would be to use the actual textures, but they are not loaded when this is called during game start
        Rect texRect = TerrainData::GetPosInTexture(node.t1);
        int w = texRect.right - texRect.left;
        int h = texRect.bottom - texRect.top;
        RTTR_Assert(w > 0 && h > 0);
        unsigned texW = helpers::roundToNextPowerOfTwo(w);
        unsigned texH = helpers::roundToNextPowerOfTwo(h);

        float texScaleW = 1.f / texW;
        float texScaleH = 1.f / texH;
        // Tip of the triangle is in the middle in x
        texCoord[1].x = (w + 1) / 2.f * texScaleW;
        texCoord[1].y = 0.f;
        // Bottom of the triangle is in the middle in y
        texCoord[2].x = 0.f;
        texCoord[2].y = (h + 1) / 2.f * texScaleH;
        texCoord[0].x = (w - 1)       * texScaleW;
        texCoord[0].y = texCoord[2].y;
    }

    Triangle& texCoord2 = gl_texcoords[pos+1];
    if(!TerrainData::IsAnimated(node.t2))
    {
        texCoord2[1].x = 0.235f;
        texCoord2[1].y = 0.45f;
        texCoord2[2].x = 0.47f; //-V807
        texCoord2[2].y = 0.0f;
        texCoord2[0].x = 0.0f;
        texCoord2[0].y = texCoord2[2].y;
    }else
    {
        Rect texRect = TerrainData::GetPosInTexture(node.t1);
        int w = texRect.right - texRect.left;
        int h = texRect.bottom - texRect.top;
        RTTR_Assert(w > 0 && h > 0);
        unsigned texW = helpers::roundToNextPowerOfTwo(w);
        unsigned texH = helpers::roundToNextPowerOfTwo(h);
        float texScaleW = 1.f / texW;
        float texScaleH = 1.f / texH;
        // Bottom tip of the triangle is in the middle in x
        texCoord2[1].x = (w + 1) / 2.f * texScaleW;
        texCoord2[1].y = (h - 1)       * texScaleH;
        // Top of the triangle is in the middle in y
        texCoord2[2].x = (w - 1)       * texScaleW;
        texCoord2[2].y = (h + 1) / 2.f * texScaleH;
        texCoord2[0].x = 0.f;
        texCoord2[0].y = texCoord2[2].y;
    }

    /// Bei Vertexbuffern das die Daten aktualisieren
    if(update && vboBuffersUsed)
    {
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_texcoords);
        glBufferSubDataARB(GL_ARRAY_BUFFER_ARB, (pos - 1) * sizeof(Triangle), 2 * sizeof(Triangle), &gl_texcoords[pos - 1]);
    }
}

/// Erzeugt die Dreiecke für die Ränder
void TerrainRenderer::UpdateBorderTrianglePos(const MapPoint pt, const GameWorldViewer&  /*gwv*/, const bool update)
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

        gl_vertices[offset][i ? 0 : 2] = GetNodePos(pt);
        gl_vertices[offset][1        ] = GetTerrainAround(pt, 4);
        gl_vertices[offset][i ? 2 : 0] = GetB(pt, i);

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

        gl_vertices[offset][i ? 2 : 0] = GetTerrainAround(pt, 4);
        gl_vertices[offset][1        ] = GetTerrainAround(pt, 3);

        if(i == 0)
            gl_vertices[offset][2] = GetB(pt, 1);
        else
            gl_vertices[offset][0] = GetBAround(pt, 0, 3);

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

        gl_vertices[offset][i ? 2 : 0] = GetTerrainAround(pt, 5);
        gl_vertices[offset][1        ] = GetTerrainAround(pt, 4);

        if(i == 0)
            gl_vertices[offset][2] = GetB(pt, i);
        else
            gl_vertices[offset][0] = GetBAround(pt, i, 5); //x - i + i * rt, y + i, i

        ++count_borders;
    }

    /// Bei Vertexbuffern das die Daten aktualisieren
    if(update && vboBuffersUsed)
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

        gl_colors[offset][i ? 0 : 2].r = gl_colors[offset][i ? 0 : 2].g = gl_colors[offset][i ? 0 : 2].b = GetColor(pt); //-V807
        gl_colors[offset][1        ].r = gl_colors[offset][1        ].g = gl_colors[offset][1        ].b = GetColor(gwv.GetNeighbour(pt, 4)); //-V807
        gl_colors[offset][i ? 2 : 0].r = gl_colors[offset][i ? 2 : 0].g = gl_colors[offset][i ? 2 : 0].b = GetBColor(pt, i); //-V807

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

        gl_colors[offset][i ? 2 : 0].r = gl_colors[offset][i ? 2 : 0].g = gl_colors[offset][i ? 2 : 0].b = GetColor(gwv.GetNeighbour(pt, 4));
        gl_colors[offset][1        ].r = gl_colors[offset][1        ].g = gl_colors[offset][1        ].b = GetColor(gwv.GetNeighbour(pt, 3));
        MapPoint pt2(pt.x + i, pt.y);
        if(pt2.x >= width)
            pt2.x -= width;
        gl_colors[offset][i ? 0 : 2].r = gl_colors[offset][i ? 0 : 2].g = gl_colors[offset][i ? 0 : 2].b = GetBColor(pt2, i ? 0 : 1);

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

        gl_colors[offset][i ? 2 : 0].r = gl_colors[offset][i ? 2 : 0].g = gl_colors[offset][i ? 2 : 0].b = GetColor(gwv.GetNeighbour(pt, 5));
        gl_colors[offset][1        ].r = gl_colors[offset][1        ].g = gl_colors[offset][1        ].b = GetColor(gwv.GetNeighbour(pt, 4));

        if(i == 0)
            gl_colors[offset][2].r = gl_colors[offset][2].g = gl_colors[offset][2].b = GetBColor(pt, i); //-V807
        else
            gl_colors[offset][0].r = gl_colors[offset][0].g = gl_colors[offset][0].b = GetBColor(gwv.GetNeighbour(pt, 5), i); //-V807

        ++count_borders;
    }

    /// Bei Vertexbuffern das die Daten aktualisieren
    if(update && vboBuffersUsed)
    {
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_colors);
        glBufferSubDataARB(GL_ARRAY_BUFFER_ARB, first_offset * sizeof(ColorTriangle), count_borders * sizeof(ColorTriangle), &gl_colors[first_offset]);
    }
}

void TerrainRenderer::UpdateBorderTriangleTerrain(const MapPoint pt, const GameWorldViewer&  /*gwv*/, const bool update)
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

            gl_texcoords[offset][i ? 0 : 2] = PointF(0.0f, 0.0f);
            gl_texcoords[offset][1        ] = PointF(1.0f, 0.0f);
            gl_texcoords[offset][i ? 2 : 0] = PointF(0.5f, 1.0f);

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

            gl_texcoords[offset][i ? 2 : 0] = PointF(0.0f, 0.0f);
            gl_texcoords[offset][1        ] = PointF(1.0f, 0.0f);
            gl_texcoords[offset][i ? 0 : 2] = PointF(0.5f, 1.0f);

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

            gl_texcoords[offset][i ? 2 : 0] = PointF(0.0f, 0.0f);
            gl_texcoords[offset][1        ] = PointF(1.0f, 0.0f);
            gl_texcoords[offset][i ? 0 : 2] = PointF(0.5f, 1.0f);

            ++count_borders;
        }
    }

    /// Bei Vertexbuffern das die Daten aktualisieren
    if(update && vboBuffersUsed)
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
    RTTR_Assert(!gl_vertices.empty());
    RTTR_Assert(!borders.empty());

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
    for(int y = gwv.GetFirstPt().y; y <= gwv.GetLastPt().y; ++y)
    {
        unsigned char lastTerrain = 255;
        unsigned char lastBorder  = 255;

        for(int x = gwv.GetFirstPt().x; x <= gwv.GetLastPt().x; ++x)
        {
            Point<int> posOffset;
            MapPoint tP = ConvertCoords(Point<int>(x, y), &posOffset);

            TerrainType t = gwv.GetViewer().GetNode(tP).t1;
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

            t = gwv.GetViewer().GetNode(tP).t2;

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

    if(vboBuffersUsed)
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

    // Alphablending aus
    glDisable(GL_BLEND);

    glPushMatrix();
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

            RTTR_Assert(it->tileOffset + it->count <= width * height * 2u);
            glDrawArrays(GL_TRIANGLES, it->tileOffset * 3, it->count * 3); // Arguments are in Elements. 1 triangle has 3 values
        }
    }
    glPopMatrix();

    glEnable(GL_BLEND);

    lastOffset = PointI(0, 0);
    glPushMatrix();
    for(unsigned short i = 0; i < 5; ++i)
    {
        if(sorted_borders[i].empty())
            continue;
        VIDEODRIVER.BindTexture(dynamic_cast<glArchivItem_Bitmap*>(LOADER.borders.get(i))->GetTexture());

        for(std::vector<BorderTile>::iterator it = sorted_borders[i].begin(); it != sorted_borders[i].end(); ++it)
        {
            if(it->posOffset != lastOffset)
            {
                PointI trans = it->posOffset - lastOffset;
                glTranslatef( float(trans.x), float(trans.y), 0.0f);
                lastOffset = it->posOffset;
            }
            RTTR_Assert(it->tileOffset + it->count <= gl_vertices.size());
            glDrawArrays(GL_TRIANGLES, it->tileOffset * 3, it->count * 3); // Arguments are in Elements. 1 triangle has 3 values
        }
    }
    glPopMatrix();

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
    RTTR_Assert(ptOut.x < width && ptOut.y < height);
    return ptOut;
}

struct GL_T2F_C3F_V3F_Struct
{
    GLfloat tx, ty;
    GLfloat r, g, b;
    GLfloat x, y, z;
};

void TerrainRenderer::PrepareWaysPoint(PreparedRoads& sorted_roads, const GameWorldView& gwv, MapPoint t, const PointI& offset)
{
    PointI startPos = PointI(GetNodePos(t)) + offset;

    GameWorldViewer& gwViewer = gwv.GetViewer();
    Visibility visibility = gwViewer.GetVisibility(t);

	int totalWidth  = gwViewer.GetWidth()  * TR_W;
	int totalHeight = gwViewer.GetHeight() * TR_H;

    // Wegtypen für die drei Richtungen
    for(unsigned dir = 0; dir < 3; ++dir)
    {
        unsigned char type = gwViewer.GetVisibleRoad(t, dir, visibility);
        if (!type)
            continue;
        MapPoint ta = gwViewer.GetNeighbour(t, 3 + dir);

        PointI endPos = PointI(GetNodePos(ta)) + offset;
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
                TerrainType t1 = gwViewer.GetTerrainAround(t, dir + 2);
                TerrainType t2 = gwViewer.GetTerrainAround(t, dir + 3);

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

    if (vboBuffersUsed)
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
            RTTR_Assert(it->dir < 3); // begin_end_coords has 3 dir entries
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
        VIDEODRIVER.BindTexture(dynamic_cast<glArchivItem_Bitmap*>(LOADER.roads.get(type))->GetTexture());
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

    if(vboBuffersUsed)
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
