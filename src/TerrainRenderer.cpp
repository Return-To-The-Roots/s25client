﻿// $Id: TerrainRenderer.cpp 9518 2014-11-30 09:22:47Z marcus $
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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "defines.h"
#include "TerrainRenderer.h"

#include "drivers/VideoDriverWrapper.h"
#include "gameData/MapConsts.h"
#include "GameWorld.h"
#include "Settings.h"
#include "GameClient.h"
#include <cstdlib>
#include <cmath>

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

TerrainRenderer::TerrainRenderer() :
	width(0), height(0),
    vertices(NULL),
    gl_vertices(NULL), gl_texcoords(NULL), gl_colors(NULL),
    vbo_vertices(0), vbo_texcoords(0), vbo_colors(0),
    borders(NULL), border_count(0)
{}

TerrainRenderer::~TerrainRenderer()
{
    if(SETTINGS.video.vbo)
    {
        glDeleteBuffersARB(1, (const GLuint*)&vbo_vertices);
        glDeleteBuffersARB(1, (const GLuint*)&vbo_texcoords);
        glDeleteBuffersARB(1, (const GLuint*)&vbo_colors);
    }

    delete[] vertices;

    delete[] gl_vertices;
    delete[] gl_texcoords;
    delete[] gl_colors;

    delete[] borders;
}

void GetPointAround(int& x, int& y, const unsigned dir)
{
    switch(dir)
    {
        case 0: x = x - 1; break;
        case 1: x = x - !(y&1); break;
        case 2: x = x + (y&1); break;
        case 3: x = x + 1; break;
        case 4: x = x + (y&1); break;
        case 5: x = x - !(y&1); break;
    }

    switch(dir)
    {
        default: break;
        case 1:
        case 2: --y; break;
        case 4:
        case 5: ++y; break;
    }
}

float TerrainRenderer::GetTerrainXAround(int x,  int y, const unsigned dir)
{
    GetPointAround(x, y, dir);

    int xo;
    MapPoint t = ConvertCoords(x, y, &xo);

    return GetTerrainX(t) + xo;
}

float TerrainRenderer::GetTerrainYAround(int x,  int y, const unsigned dir)
{
    GetPointAround(x, y, dir);

    int yo;
    MapPoint t = ConvertCoords(x, y, NULL, &yo);

    return GetTerrainY(t) + yo;
}

float TerrainRenderer::GetBXAround(int x, int y, const unsigned char triangle, const unsigned char dir)
{
    GetPointAround(x, y, dir);

    int xo;
    MapPoint t = ConvertCoords(x, y, &xo, NULL);

    return GetBX(t, triangle) + xo;
}

float TerrainRenderer::GetBYAround(int x, int y, const unsigned char triangle, const unsigned char dir)
{
    GetPointAround(x, y, dir);

    int yo;
    MapPoint t = ConvertCoords(x, y, NULL, &yo);

    return GetBY(t, triangle) + yo;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  erzeugt die Terrain-Vertices.
 *
 *  @author OLiver
 */
void TerrainRenderer::GenerateVertices(const GameWorldViewer* gwv)
{
    delete[] vertices;
    vertices = new Vertex[width * height];
    memset(vertices, 0, sizeof(Vertex) * width * height);

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
void TerrainRenderer::UpdateVertexPos(const MapPoint pt, const GameWorldViewer* gwv)
{
    GetVertex(pt).pos.pos.x = float(pt.x * TR_W + ( (pt.y & 1) ? TR_W / 2 : 0) );
    GetVertex(pt).pos.pos.y = float(pt.y * TR_H - HEIGHT_FACTOR * gwv->GetNode(pt).altitude + HEIGHT_FACTOR * 0x0A );
}


void TerrainRenderer::UpdateVertexColor(const MapPoint pt, const GameWorldViewer* gwv)
{
    switch(gwv->GetVisibility(pt))
    {
            // Unsichtbar -> schwarz
        case VIS_INVISIBLE: GetVertex(pt).pos.color = 0.0f; break;
            // Fog of War -> abgedunkelt
        case VIS_FOW: GetVertex(pt).pos.color = float(gwv->GetNode(pt).shadow + 0x40) / float(0xFF) / 2; break;
            // Normal sichtbar
        case VIS_VISIBLE: GetVertex(pt).pos.color = float(gwv->GetNode(pt).shadow  + 0x40) / float(0xFF); break;
    }
}

void TerrainRenderer::UpdateVertexTerrain(const MapPoint pt, const GameWorldViewer* gwv)
{
    if(gwv->GetNode(pt).t1 < 20)
        GetVertex(pt).terrain[0] = gwv->GetNode(pt).t1;

    if(gwv->GetNode(pt).t2 < 20)
        GetVertex(pt).terrain[1] = gwv->GetNode(pt).t2;
}

/// erzeugt Rand-Vertex
void TerrainRenderer::UpdateBorderVertex(const MapPoint pt, const GameWorldViewer* gwv)
{
    /// @todo GetTerrainX und Co durch GetTerrainXA ausdrücken
    GetVertex(pt).border[0].pos.x = ( GetTerrainXAround(pt.x, pt.y, 5) + GetTerrainX(pt) + GetTerrainXAround(pt.x, pt.y, 4) ) / 3.0f;
    GetVertex(pt).border[0].pos.y = ( GetTerrainYAround(pt.x, pt.y, 5) + GetTerrainY(pt) + GetTerrainYAround(pt.x, pt.y, 4) ) / 3.0f;
    GetVertex(pt).border[0].color = ( GetColor(gwv->GetNeighbour(pt, 5)) + GetColor(pt) + GetColor(gwv->GetNeighbour(pt, 4)) ) / 3.0f;

    GetVertex(pt).border[1].pos.x = ( GetTerrainXAround(pt.x, pt.y, 3) + GetTerrainX(pt) + GetTerrainXAround(pt.x, pt.y, 4) ) / 3.0f;
    GetVertex(pt).border[1].pos.y = ( GetTerrainYAround(pt.x, pt.y, 3) + GetTerrainY(pt) + GetTerrainYAround(pt.x, pt.y, 4) ) / 3.0f;
    GetVertex(pt).border[1].color = ( GetColor(gwv->GetNeighbour(pt, 3)) + GetColor(pt) + GetColor(gwv->GetNeighbour(pt, 4)) ) / 3.0f;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  erzeugt die OpenGL-Vertices.
 *
 *  @author OLiver
 *  @author FloSoft
 */
void TerrainRenderer::GenerateOpenGL(const GameWorldViewer* gwv)
{
    width = gwv->GetWidth();
    height = gwv->GetHeight();
    LandscapeType lt = gwv->GetLandscapeType();

    GenerateVertices(gwv);

    unsigned int offset = width * height * 2;

    // Ränder zählen
    borders = new Borders[width * height];
    for(MapCoord y = 0; y < height; ++y)
    {
        for(MapCoord x = 0; x < width; ++x)
        {
            MapPoint pt(x, y);
            unsigned char t1 = gwv->GetNode(pt).t1;
            unsigned char t2 = gwv->GetNode(pt).t2;
            unsigned int pos = GetTRIdx(pt);

            if( (borders[pos].left_right[0] = BORDER_TABLES[lt][t1][t2][1]) )
            {
                borders[pos].left_right_offset[0] = offset + border_count;
                ++border_count;
            }
            if( (borders[pos].left_right[1] = BORDER_TABLES[lt][t1][t2][0]) )
            {
                borders[pos].left_right_offset[1] = offset + border_count;
                ++border_count;
            }

            t1 = gwv->GetNodeAround(pt, 3).t1;
            if( (borders[pos].right_left[0] = BORDER_TABLES[lt][t2][t1][1]) )
            {
                borders[pos].right_left_offset[0] = offset + border_count;
                ++border_count;
            }
            if( (borders[pos].right_left[1] = BORDER_TABLES[lt][t2][t1][0]) )
            {
                borders[pos].right_left_offset[1] = offset + border_count;
                ++border_count;
            }

            t1 = gwv->GetNode(pt).t1;
            t2 = gwv->GetNodeAround(pt, 5).t2;
            if( (borders[pos].top_down[0] = BORDER_TABLES[lt][t1][t2][1]) )
            {
                borders[pos].top_down_offset[0] = offset + border_count;
                ++border_count;
            }
            if( (borders[pos].top_down[1] = BORDER_TABLES[lt][t1][t2][0]) )
            {
                borders[pos].top_down_offset[1] = offset + border_count;
                ++border_count;
            }
        }
    }

    gl_vertices = new Triangle[offset + border_count];
    gl_texcoords = new Triangle[offset + border_count];
    gl_colors = new ColorTriangle[offset + border_count];

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
    //unsigned buffer_size = (offset ) * 2 * 3 * sizeof(float);

    if(SETTINGS.video.vbo)
    {
        // Generiere und Binde den Vertex Buffer
        glGenBuffersARB(1, (GLuint*)&vbo_vertices);
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_vertices);
        glBufferDataARB(GL_ARRAY_BUFFER_ARB, (offset + border_count) * 3 * 2 * sizeof(float), gl_vertices, GL_STATIC_DRAW_ARB);
        glVertexPointer(2, GL_FLOAT, 0, NULL);

        // Generiere und Binde den Textur Koordinaten Buffer
        glGenBuffersARB(1, (GLuint*)&vbo_texcoords);
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_texcoords);
        glBufferDataARB(GL_ARRAY_BUFFER_ARB, (offset + border_count) * 3 * 2 * sizeof(float), gl_texcoords, GL_STATIC_DRAW_ARB );
        glTexCoordPointer(2, GL_FLOAT, 0, NULL);

        // Generiere und Binde den Color Buffer
        glGenBuffersARB(1, (GLuint*)&vbo_colors);
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_colors);
        glBufferDataARB(GL_ARRAY_BUFFER_ARB, (offset + border_count) * 3 * 3 * sizeof(float), gl_colors, GL_STATIC_DRAW_ARB );
        glColorPointer(3, GL_FLOAT, 0, NULL);
    }
    else
    {
        glVertexPointer(2, GL_FLOAT, 0, gl_vertices);
        glTexCoordPointer(2, GL_FLOAT, 0, gl_texcoords);
        glColorPointer(3, GL_FLOAT, 0, gl_colors);
    }
}

/// Erzeugt fertiges Dreieick für OpenGL

void TerrainRenderer::UpdateTrianglePos(const MapPoint pt, const GameWorldViewer* gwv, const bool update)
{
    unsigned int pos = 2 * width * pt.y + pt.x * 2;

    gl_vertices[pos].pos[0].x = GetTerrainXAround(pt.x, pt.y, 4);
    gl_vertices[pos].pos[0].y = GetTerrainYAround(pt.x, pt.y, 4);
    gl_vertices[pos].pos[1].x = GetTerrainX(pt);
    gl_vertices[pos].pos[1].y = GetTerrainY(pt);
    gl_vertices[pos].pos[2].x = GetTerrainXAround(pt.x, pt.y, 5);
    gl_vertices[pos].pos[2].y = GetTerrainYAround(pt.x, pt.y, 5);

    ++pos;

    gl_vertices[pos].pos[0].x = GetTerrainX(pt);
    gl_vertices[pos].pos[0].y = GetTerrainY(pt);
    gl_vertices[pos].pos[1].x = GetTerrainXAround(pt.x, pt.y, 4);
    gl_vertices[pos].pos[1].y = GetTerrainYAround(pt.x, pt.y, 4);
    gl_vertices[pos].pos[2].x = GetTerrainXAround(pt.x, pt.y, 3);
    gl_vertices[pos].pos[2].y = GetTerrainYAround(pt.x, pt.y, 3);

    if(update && SETTINGS.video.vbo)
    {
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_vertices);
        glBufferSubDataARB(GL_ARRAY_BUFFER_ARB, (pos - 1) * 3 * 2 * sizeof(float),
                           2 * 3 * 2 * sizeof(float), &gl_vertices[pos - 1]);
    }
}

void TerrainRenderer::UpdateTriangleColor(const MapPoint pt, const GameWorldViewer* gwv, const bool update)
{
    unsigned int pos = 2 * width * pt.y + pt.x * 2;

    gl_colors[pos].colors[0].r = gl_colors[pos].colors[0].g = gl_colors[pos].colors[0].b = GetColor(gwv->GetNeighbour(pt, 4));
    gl_colors[pos].colors[1].r = gl_colors[pos].colors[1].g = gl_colors[pos].colors[1].b = GetColor(pt);
    gl_colors[pos].colors[2].r = gl_colors[pos].colors[2].g = gl_colors[pos].colors[2].b = GetColor(gwv->GetNeighbour(pt, 5));

    ++pos;

    gl_colors[pos].colors[0].r = gl_colors[pos].colors[0].g = gl_colors[pos].colors[0].b = GetColor(pt);
    gl_colors[pos].colors[1].r = gl_colors[pos].colors[1].g = gl_colors[pos].colors[1].b = GetColor(gwv->GetNeighbour(pt, 4));
    gl_colors[pos].colors[2].r = gl_colors[pos].colors[2].g = gl_colors[pos].colors[2].b = GetColor(gwv->GetNeighbour(pt, 3));


    /// Bei Vertexbuffern das die Daten aktualisieren
    if(update && SETTINGS.video.vbo)
    {
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_colors);
        glBufferSubDataARB(GL_ARRAY_BUFFER_ARB, (pos - 1) * 3 * 3 * sizeof(float),
                           2 * 3 * 3 * sizeof(float), &gl_colors[pos - 1]);
    }
}

void TerrainRenderer::UpdateTriangleTerrain(const MapPoint pt, const GameWorldViewer* gwv, const bool update)
{
    unsigned int pos = 2 * width * pt.y + pt.x * 2;

    unsigned char t1 = gwv->GetNode(pt).t1;
    gl_texcoords[pos].pos[0].x = (t1 == 14 || t1 == 15) ? 0.4375f   : 0.45f;
    gl_texcoords[pos].pos[0].y = (t1 == 14 || t1 == 15) ? 0.0f      : 0.45f;
    gl_texcoords[pos].pos[1].y = (t1 == 14 || t1 == 15) ? 0.445312f : 0.0f;
    gl_texcoords[pos].pos[1].x = (t1 == 14 || t1 == 15) ? 0.0f      : 0.225f;
    gl_texcoords[pos].pos[2].x = (t1 == 14 || t1 == 15) ? 0.84375f  : 0.0f;
    gl_texcoords[pos].pos[2].y = (t1 == 14 || t1 == 15) ? 0.445312f : 0.45f;

    ++pos;

    unsigned char t2 = gwv->GetNode(pt).t2;
    gl_texcoords[pos].pos[0].x = (t2 == 14 || t2 == 15) ? 0.4375f   : 0.0f;
    gl_texcoords[pos].pos[0].y = (t2 == 14 || t2 == 15) ? 0.859375f : 0.0f;
    gl_texcoords[pos].pos[1].x = (t2 == 14 || t2 == 15) ? 0.84375f  : 0.235f;
    gl_texcoords[pos].pos[1].y = (t2 == 14 || t2 == 15) ? 0.445312f : 0.45f;
    gl_texcoords[pos].pos[2].x = (t2 == 14 || t2 == 15) ? 0.0f      : 0.47f;
    gl_texcoords[pos].pos[2].y = (t2 == 14 || t2 == 15) ? 0.445312f : 0.0f;


    /// Bei Vertexbuffern das die Daten aktualisieren
    if(update && SETTINGS.video.vbo)
    {
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_texcoords);
        glBufferSubDataARB(GL_ARRAY_BUFFER_ARB, (pos - 1) * 3 * 2 * sizeof(float),
                           2 * 3 * 2 * sizeof(float), &gl_texcoords[pos - 1]);
    }
}

/// Erzeugt die Dreiecke für die Ränder
void TerrainRenderer::UpdateBorderTrianglePos(const MapPoint pt, const GameWorldViewer* gwv, const bool update)
{
    unsigned int pos = GetTRIdx(pt);

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

            gl_vertices[offset].pos[i ? 0 : 2].x = GetTerrainX(pt);
            gl_vertices[offset].pos[i ? 0 : 2].y = GetTerrainY(pt);
            gl_vertices[offset].pos[1        ].x = GetTerrainXAround(pt.x, pt.y, 4);
            gl_vertices[offset].pos[1        ].y = GetTerrainYAround(pt.x, pt.y, 4);
            gl_vertices[offset].pos[i ? 2 : 0].x = GetBX(pt, i);
            gl_vertices[offset].pos[i ? 2 : 0].y = GetBY(pt, i);

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

            gl_vertices[offset].pos[i ? 2 : 0].x = GetTerrainXAround(pt.x, pt.y, 4);
            gl_vertices[offset].pos[i ? 2 : 0].y = GetTerrainYAround(pt.x, pt.y, 4);
            gl_vertices[offset].pos[1        ].x = GetTerrainXAround(pt.x, pt.y, 3);
            gl_vertices[offset].pos[1        ].y = GetTerrainYAround(pt.x, pt.y, 3);

            if(i == 0)
            {
                gl_vertices[offset].pos[2].x = GetBX(pt, 1);
                gl_vertices[offset].pos[2].y = GetBY(pt, 1);
            }
            else
            {
                gl_vertices[offset].pos[0].x = GetBXAround(pt.x, pt.y, 0, 3);
                gl_vertices[offset].pos[0].y = GetBYAround(pt.x, pt.y, 0, 3);
            }

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

            gl_vertices[offset].pos[i ? 2 : 0].x = GetTerrainXAround(pt.x, pt.y, 5);
            gl_vertices[offset].pos[i ? 2 : 0].y = GetTerrainYAround(pt.x, pt.y, 5);
            gl_vertices[offset].pos[1        ].x = GetTerrainXAround(pt.x, pt.y, 4);
            gl_vertices[offset].pos[1        ].y = GetTerrainYAround(pt.x, pt.y, 4);

            if(i == 0)
            {
                gl_vertices[offset].pos[2].x = GetBX(pt, i);
                gl_vertices[offset].pos[2].y = GetBY(pt, i);
            }
            else
            {
                //x - i + i * rt, y + i, i
                gl_vertices[offset].pos[0].x = GetBXAround(pt.x, pt.y, i, 5);
                gl_vertices[offset].pos[0].y = GetBYAround(pt.x, pt.y, i, 5);
            }

            ++count_borders;
        }
    }

    /// Bei Vertexbuffern das die Daten aktualisieren
    if(update && SETTINGS.video.vbo)
    {
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_vertices);
        glBufferSubDataARB(GL_ARRAY_BUFFER_ARB, first_offset * 3 * 2 * sizeof(float),
                           count_borders * 3 * 2  * sizeof(float), &gl_vertices[first_offset]);
    }
}

void TerrainRenderer::UpdateBorderTriangleColor(const MapPoint pt, const GameWorldViewer* gwv, const bool update)
{
    unsigned int pos = GetTRIdx(pt);

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

            gl_colors[offset].colors[i ? 0 : 2].r = gl_colors[offset].colors[i ? 0 : 2].g = gl_colors[offset].colors[i ? 0 : 2].b = GetColor(pt);
            gl_colors[offset].colors[1        ].r = gl_colors[offset].colors[1        ].g = gl_colors[offset].colors[1        ].b = GetColor(gwv->GetNeighbour(pt, 4));
            gl_colors[offset].colors[i ? 2 : 0].r = gl_colors[offset].colors[i ? 2 : 0].g = gl_colors[offset].colors[i ? 2 : 0].b = GetBColor(pt, i);

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

            gl_colors[offset].colors[i ? 2 : 0].r = gl_colors[offset].colors[i ? 2 : 0].g = gl_colors[offset].colors[i ? 2 : 0].b = GetColor(gwv->GetNeighbour(pt, 4));
            gl_colors[offset].colors[1        ].r = gl_colors[offset].colors[1        ].g = gl_colors[offset].colors[1        ].b = GetColor(gwv->GetNeighbour(pt, 3));
            gl_colors[offset].colors[i ? 0 : 2].r = gl_colors[offset].colors[i ? 0 : 2].g = gl_colors[offset].colors[i ? 0 : 2].b = GetBColor(MapPoint(pt.x + i, pt.y), i ? 0 : 1);

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

            gl_colors[offset].colors[i ? 2 : 0].r = gl_colors[offset].colors[i ? 2 : 0].g = gl_colors[offset].colors[i ? 2 : 0].b = GetColor(gwv->GetNeighbour(pt, 5));
            gl_colors[offset].colors[1        ].r = gl_colors[offset].colors[1        ].g = gl_colors[offset].colors[1        ].b = GetColor(gwv->GetNeighbour(pt, 4));

            if(i == 0)
                gl_colors[offset].colors[2].r = gl_colors[offset].colors[2].g = gl_colors[offset].colors[2].b = GetBColor(pt, i);
            else
                gl_colors[offset].colors[0].r = gl_colors[offset].colors[0].g = gl_colors[offset].colors[0].b = GetBColor(gwv->GetNeighbour(pt, 5), i);

            ++count_borders;
        }
    }

    /// Bei Vertexbuffern das die Daten aktualisieren
    if(update && SETTINGS.video.vbo)
    {
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_colors);
        glBufferSubDataARB(GL_ARRAY_BUFFER_ARB, first_offset * 3 * 3 * sizeof(float),
                           count_borders * 3 * 3 * sizeof(float), &gl_colors[first_offset]);
    }
}

void TerrainRenderer::UpdateBorderTriangleTerrain(const MapPoint pt, const GameWorldViewer* gwv, const bool update)
{
    unsigned int pos = GetTRIdx(pt);

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

            gl_texcoords[offset].pos[i ? 0 : 2].x = 0.0f;
            gl_texcoords[offset].pos[i ? 0 : 2].y = 0.0f;
            gl_texcoords[offset].pos[1        ].x = 1.0f;
            gl_texcoords[offset].pos[1        ].y = 0.0f;
            gl_texcoords[offset].pos[i ? 2 : 0].x = 0.5f;
            gl_texcoords[offset].pos[i ? 2 : 0].y = 1.0f;

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

            gl_texcoords[offset].pos[i ? 2 : 0].x = 0.0f;
            gl_texcoords[offset].pos[i ? 2 : 0].y = 0.0f;
            gl_texcoords[offset].pos[1        ].x = 1.0f;
            gl_texcoords[offset].pos[1        ].y = 0.0f;
            gl_texcoords[offset].pos[i ? 0 : 2].x = 0.5f;
            gl_texcoords[offset].pos[i ? 0 : 2].y = 1.0f;

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

            gl_texcoords[offset].pos[i ? 2 : 0].x = 0.0f;
            gl_texcoords[offset].pos[i ? 2 : 0].y = 0.0f;
            gl_texcoords[offset].pos[1        ].x = 1.0f;
            gl_texcoords[offset].pos[1        ].y = 0.0f;
            gl_texcoords[offset].pos[i ? 0 : 2].x = 0.5f;
            gl_texcoords[offset].pos[i ? 0 : 2].y = 1.0f;

            ++count_borders;
        }
    }

    /// Bei Vertexbuffern das die Daten aktualisieren
    if(update && SETTINGS.video.vbo)
    {
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_texcoords);
        glBufferSubDataARB(GL_ARRAY_BUFFER_ARB, first_offset * 3 * 2 * sizeof(float),
                           count_borders * 3 * 2  * sizeof(float), &gl_texcoords[first_offset]);
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  zeichnet den Kartenausschnitt.
 *
 *  @author OLiver
 *  @author FloSoft
 */
void TerrainRenderer::Draw(GameWorldView* gwv, unsigned int* water)
{
    assert(gl_vertices);
    assert(borders);

    /*  if ((gwv->GetXOffset() == gwv->terrain_last_xoffset) && (gwv->GetYOffset() == gwv->terrain_last_yoffset) && (gwv->terrain_list != 0) && (GAMECLIENT.GetGlobalAnimation(4, 5, 4, 0) == gwv->terrain_last_global_animation))
        {
            glCallList(gwv->terrain_list);
            *water = gwv->terrain_last_water;
            return;
        }

        gwv->terrain_last_xoffset = gwv->GetXOffset();
        gwv->terrain_last_yoffset = gwv->GetYOffset();
        gwv->terrain_last_global_animation = GAMECLIENT.GetGlobalAnimation(4, 5, 4, 0);

        if (gwv->terrain_list == 0)
            gwv->terrain_list = glGenLists(1);

        glNewList(gwv->terrain_list, GL_COMPILE_AND_EXECUTE);*/

    PrepareWays(gwv);

    // nach Texture in Listen sortieren
    std::list<MapTile> sorted_textures[16];
    std::list<BorderTile> sorted_borders[5];

    int lastxo = 0, lastyo = 0;
    int xo, yo;
    unsigned int offset = width * height * 2;

    // Beim zeichnen immer nur beginnen, wo man auch was sieht
    for(int y = gwv->GetFirstPt().y; y < gwv->GetLastPt().y; ++y)
    {
        unsigned char last = 255;
        unsigned char last_border = 255;

        for(int x = gwv->GetFirstPt().x; x < gwv->GetLastPt().x; ++x)
        {
            MapPoint tP = ConvertCoords(x, y, &xo, &yo);

            unsigned char t = gwv->GetGameWorldViewer()->GetNode(tP).t1;
            if(xo != lastxo || yo != lastyo)
                last = 255;

            if(t == last)
                ++sorted_textures[t].back().count;
            else
            {
                MapTile tmp = {tP.x * 2, tP.y, 1, xo, yo};
                sorted_textures[t].push_back(tmp);
            }

            last = t;

            t = gwv->GetGameWorldViewer()->GetNode(tP).t2;

            if(t == last)
                ++sorted_textures[t].back().count;
            else
            {
                MapTile tmp = {tP.x * 2 + 1, tP.y, 1, xo, yo};
                sorted_textures[t].push_back(tmp);
            }

            last = t;

            unsigned char tiles[6] =
            {
                borders[GetTRIdx(tP)].left_right[0],
                borders[GetTRIdx(tP)].left_right[1],
                borders[GetTRIdx(tP)].right_left[0],
                borders[GetTRIdx(tP)].right_left[1],
                borders[GetTRIdx(tP)].top_down[0],
                borders[GetTRIdx(tP)].top_down[1],
            };

            unsigned int offsets[6] =
            {
                borders[GetTRIdx(tP)].left_right_offset[0],
                borders[GetTRIdx(tP)].left_right_offset[1],
                borders[GetTRIdx(tP)].right_left_offset[0],
                borders[GetTRIdx(tP)].right_left_offset[1],
                borders[GetTRIdx(tP)].top_down_offset[0],
                borders[GetTRIdx(tP)].top_down_offset[1],
            };

            for(unsigned char i = 0; i < 6; ++i)
            {
                if(tiles[i])
                {
                    if(tiles[i] == last_border)
                        ++sorted_borders[last_border - 1].back().count;
                    else
                    {
                        last_border = tiles[i];
                        BorderTile tmp = { (int)offsets[ i ], 1, xo, yo};
                        sorted_borders[last_border - 1].push_back(tmp);
                    }
                    ++offset;
                }
            }

            PrepareWaysPoint(gwv, tP, xo, yo);

            lastxo = xo;
            lastyo = yo;
        }
    }

    if (water)
    {
        unsigned water_count = 0;

        for(std::list<MapTile>::iterator it = sorted_textures[TT_WATER].begin(); it != sorted_textures[TT_WATER].end(); ++it)
        {
            water_count += it->count;
        }

        if( (gwv->GetLastPt().x - gwv->GetFirstPt().x) && (gwv->GetLastPt().y - gwv->GetFirstPt().y) )
            *water = 50 * water_count / ( (gwv->GetLastPt().x - gwv->GetFirstPt().x) * (gwv->GetLastPt().y - gwv->GetFirstPt().y) );
        else
            *water = 0;
    }

    lastxo = 0;
    lastyo = 0;

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
        glVertexPointer(2, GL_FLOAT, 0, gl_vertices);
        glTexCoordPointer(2, GL_FLOAT, 0, gl_texcoords);
        glColorPointer(3, GL_FLOAT, 0, gl_colors);
    }

    // Arrays aktivieren
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);

    // Modulate2x
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);
    glTexEnvf(GL_TEXTURE_ENV, GL_RGB_SCALE_EXT, 2.0f);

    // Verschieben gem#ß x und y offset
    glTranslatef( float(-gwv->GetXOffset()), float(-gwv->GetYOffset()), 0.0f);

    // Alphablending aus
    glDisable(GL_BLEND);

    for(unsigned char i = 0; i < 16; ++i)
    {
        if(!sorted_textures[i].empty())
        {
            switch(i)
            {
                case TT_WATER:
                    VIDEODRIVER.BindTexture(GetImage(water, GAMECLIENT.GetGlobalAnimation(8, 5, 2, 0))->GetTexture());
                    break;
                case TT_LAVA:
                    VIDEODRIVER.BindTexture(GetImage(lava, GAMECLIENT.GetGlobalAnimation(4, 5, 4, 0))->GetTexture());
                    break;
                default:
                    VIDEODRIVER.BindTexture(GetImage(textures, i)->GetTexture());
            }

            for(std::list<MapTile>::iterator it = sorted_textures[i].begin(); it != sorted_textures[i].end(); ++it)
            {
                if(it->xo != lastxo || it->yo != lastyo)
                {
                    glTranslatef( float(it->xo - lastxo), float(it->yo - lastyo), 0.0f);
                    lastxo = it->xo;
                    lastyo = it->yo;
                }

                glDrawArrays(GL_TRIANGLES, it->y * width * 3 * 2 + it->x * 3, it->count * 3);
            }
        }
    }

    glEnable(GL_BLEND);

    glLoadIdentity();
    glTranslatef( float(-gwv->GetXOffset()), float(-gwv->GetYOffset()), 0.0f);

    lastxo = 0;
    lastyo = 0;
    for(unsigned short i = 0; i < 5; ++i)
    {
        if(!sorted_borders[i].empty())
        {
            VIDEODRIVER.BindTexture(GetImage(borders, i)->GetTexture());

            for(std::list<BorderTile>::iterator it = sorted_borders[i].begin(); it != sorted_borders[i].end(); ++it)
            {
                if(it->xo != lastxo || it->yo != lastyo)
                {
                    glTranslatef( float(it->xo - lastxo), float(it->yo - lastyo), 0.0f);
                    lastxo = it->xo;
                    lastyo = it->yo;
                }
                glDrawArrays(GL_TRIANGLES, it->offset * 3, it->count * 3);
            }
        }
    }

    glLoadIdentity();

    DrawWays(gwv);

    // Wieder zurück ins normale modulate
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    /*  glEndList();

        gwv->terrain_last_water = *water;*/
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
MapPoint TerrainRenderer::ConvertCoords(int x, int y, int* xo, int* yo) const
{
    MapPoint ptOut;
	if (x < 0)
	{
	    if (xo)
	    	*xo = -TR_W * width;
		ptOut.x = static_cast<MapCoord>(width + (x % width));
	} else
	{
	    if (xo)
	    	*xo = (x / width) * (TR_W * width);
		ptOut.x = static_cast<MapCoord>(x % width);
	}
	
	if (y < 0)
	{
	    if (yo)
	    	*yo = -TR_H * height;
		ptOut.y = static_cast<MapCoord>(height + (y % height));
	} else
	{
	    if (yo)
	    	*yo = (y / height) * (TR_H * height);
		ptOut.y = static_cast<MapCoord>(y % height);
	}
    return ptOut;
}

struct GL_T2F_C3F_V3F_Struct
{
    GLfloat tx, ty;
    GLfloat r, g, b;
    GLfloat x, y, z;
};

void TerrainRenderer::PrepareWays(GameWorldView* gwv)
{
    for (unsigned char type = 0; type < 4; type++)
    {
        gwv->sorted_roads[type].clear();
    }
}

void TerrainRenderer::PrepareWaysPoint(GameWorldView* gwv, MapPoint t, int xo, int yo)
{
    float xpos = GetTerrainX(t) - gwv->GetXOffset() + xo;
    float ypos = GetTerrainY(t) - gwv->GetYOffset() + yo;

    // Wegtypen für die drei Richtungen
    unsigned char type;

    Visibility visibility = gwv->GetGameWorldViewer()->GetVisibility(t);

    for(unsigned dir = 0; dir < 3; ++dir)
    {
        if ((type = gwv->GetGameWorldViewer()->GetVisibleRoad(t, dir, visibility)))
        {
            MapPoint ta = gwv->GetGameWorldViewer()->GetNeighbour(t, 3 + dir);

            float xpos2 = GetTerrainX(ta) - gwv->GetXOffset() + xo;
            float ypos2 = GetTerrainY(ta) - gwv->GetYOffset() + yo;

            // Gehen wir über einen Kartenrand (horizontale Richung?)
            if(std::abs(xpos - xpos2) >= gwv->GetGameWorldViewer()->GetWidth() * TR_W / 2)
            {
                if(std::abs(xpos2 - int(gwv->GetGameWorldViewer()->GetWidth())*TR_W - xpos) < std::abs(xpos - xpos2))
                    xpos2 -= gwv->GetGameWorldViewer()->GetWidth() * TR_W;
                else
                    xpos2 += gwv->GetGameWorldViewer()->GetWidth() * TR_W;
            }
            // Und dasselbe für vertikale Richtung
            if(std::abs(ypos - ypos2) >= gwv->GetGameWorldViewer()->GetHeight() * TR_H / 2)
            {
                if(std::abs(ypos2 - int(gwv->GetGameWorldViewer()->GetHeight())*TR_H - ypos) < std::abs(ypos - ypos2))
                    ypos2 -= gwv->GetGameWorldViewer()->GetHeight() * TR_H;
                else
                    ypos2 += gwv->GetGameWorldViewer()->GetHeight() * TR_H;
            }

            --type;

            // Wegtypen "konvertieren"
            switch(type)
            {
                case RoadSegment::RT_DONKEY:
                case RoadSegment::RT_NORMAL:
                {
                    unsigned t1 = gwv->GetGameWorldViewer()->GetTerrainAround(t, dir + 2);
                    unsigned t2 = gwv->GetGameWorldViewer()->GetTerrainAround(t, dir + 3);

                    // Prüfen, ob Bergwerge gezeichnet werden müssen, indem man guckt, ob der Weg einen
                    // Berg "streift" oder auch eine Bergwiese
                    if(( (t1 >= TT_MOUNTAIN1 && t1 <= TT_MOUNTAIN4) || t1 == TT_MOUNTAINMEADOW)
                            || ( (t2 >= TT_MOUNTAIN1  && t2 <= TT_MOUNTAIN4) || t2 == TT_MOUNTAINMEADOW))
                        type = 3;

                } break;
                case RoadSegment::RT_BOAT:
                {
                    type = 2;
                } break;
            }

            gwv->sorted_roads[type].push_back(
                PreparedRoad(type,
                             xpos, ypos, xpos2, ypos2,
                             GetColor(t), GetColor(ta),
                             dir
                            )
            );
        }
    }
}

void TerrainRenderer::DrawWays(GameWorldView* gwv)
{
    const float begin_end_coords[24] =
    {
        -3.0f, -3.0f,
        -3.0f, 3.0f,
        -3.0f, 3.0f,
        -3.0f, -3.0f,

        3.0f, -3.0f,
        -3.0f, 3.0f,
        -3.0f, 3.0f,
        3.0f, -3.0f,

        3.0f, 3.0f,
        -3.0f, -3.0f,
        -3.0f, -3.0f,
        3.0f, 3.0f,
    };

    if (SETTINGS.video.vbo)
    {
        // unbind VBO
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
    }

    for (unsigned char type = 0; type < 4; type++)
    {
        unsigned int i = 0;
        GL_T2F_C3F_V3F_Struct* tmp = new GL_T2F_C3F_V3F_Struct[gwv->sorted_roads[type].size() * 4];

        for (std::list<PreparedRoad>::iterator it = gwv->sorted_roads[type].begin(); it != gwv->sorted_roads[type].end(); ++it)
        {
            tmp[i].tx = 0.0f;
            tmp[i].ty = 0.0f;

            tmp[i].r = (*it).color1;
            tmp[i].g = (*it).color1;
            tmp[i].b = (*it).color1;

            tmp[i].x = (*it).xpos + begin_end_coords[(*it).dir * 8];
            tmp[i].y = (*it).ypos + begin_end_coords[(*it).dir * 8 + 1];
            tmp[i].z = 0.0f;

            i++;

            tmp[i].tx = 0.0f;
            tmp[i].ty = 1.0f;

            tmp[i].r = (*it).color1;
            tmp[i].g = (*it).color1;
            tmp[i].b = (*it).color1;

            tmp[i].x = (*it).xpos + begin_end_coords[(*it).dir * 8 + 2];
            tmp[i].y = (*it).ypos + begin_end_coords[(*it).dir * 8 + 3];
            tmp[i].z = 0.0f;

            i++;

            tmp[i].tx = 0.78f;
            tmp[i].ty = 1.0f;

            tmp[i].r = (*it).color2;
            tmp[i].g = (*it).color2;
            tmp[i].b = (*it).color2;

            tmp[i].x = (*it).xpos2 + begin_end_coords[(*it).dir * 8 + 4];
            tmp[i].y = (*it).ypos2 + begin_end_coords[(*it).dir * 8 + 5];
            tmp[i].z = 0.0f;

            i++;

            tmp[i].tx = 0.78f;
            tmp[i].ty = 0.0f;

            tmp[i].r = (*it).color2;
            tmp[i].g = (*it).color2;
            tmp[i].b = (*it).color2;

            tmp[i].x = (*it).xpos2 + begin_end_coords[(*it).dir * 8 + 6];
            tmp[i].y = (*it).ypos2 + begin_end_coords[(*it).dir * 8 + 7];
            tmp[i].z = 0.0f;

            i++;
        }

        glInterleavedArrays(GL_T2F_C3F_V3F, 0, tmp);
        VIDEODRIVER.BindTexture(GetImage(roads, type)->GetTexture());
        glDrawArrays(GL_QUADS, 0, i);

        delete[] tmp;
    }
}

void TerrainRenderer::AltitudeChanged(const MapPoint pt, const GameWorldViewer* gwv)
{
    // den selbst sowieso die Punkte darum updaten, da sich bei letzteren die Schattierung geändert haben könnte
    UpdateVertexPos(pt, gwv);
    UpdateVertexColor(pt, gwv);

    for(unsigned i = 0; i < 6; ++i)
        UpdateVertexColor(gwv->GetNeighbour(pt, i), gwv);


    // und für die Ränder
    UpdateBorderVertex(pt, gwv);

    for(unsigned i = 0; i < 6; ++i)
        UpdateBorderVertex(gwv->GetNeighbour(pt, i), gwv);

    // den selbst sowieso die Punkte darum updaten, da sich bei letzteren die Schattierung geändert haben könnte
    UpdateTrianglePos(pt, gwv, true);
    UpdateTriangleColor(pt, gwv, true);

    for(unsigned i = 0; i < 6; ++i)
    {
        UpdateTrianglePos(gwv->GetNeighbour(pt, i), gwv, true);
        UpdateTriangleColor(gwv->GetNeighbour(pt, i), gwv, true);
    }


    // Auch im zweiten Kreis drumherum die Dreiecke neu berechnen, da die durch die Schattenänderung der umliegenden
    // Punkte auch geändert werden könnten
    for(unsigned i = 0; i < 12; ++i)
        UpdateTriangleColor(gwv->GetNeighbour2(pt, i), gwv, true);


    // und für die Ränder
    UpdateBorderTrianglePos(pt, gwv, true);
    UpdateBorderTriangleColor(pt, gwv, true);

    for(unsigned i = 0; i < 6; ++i)
    {
        UpdateBorderTrianglePos(gwv->GetNeighbour(pt, i), gwv, true);
        UpdateBorderTriangleColor(gwv->GetNeighbour(pt, i), gwv, true);
    }

    for(unsigned i = 0; i < 12; ++i)
        UpdateBorderTriangleColor(gwv->GetNeighbour2(pt, i), gwv, true);
}

void TerrainRenderer::VisibilityChanged(const MapPoint pt, const GameWorldViewer* gwv)
{
    /// Noch kein Terrain gebaut? abbrechen
    if(!vertices)
        return;

    UpdateVertexColor(pt, gwv);
    for(unsigned i = 0; i < 6; ++i)
        UpdateVertexColor(gwv->GetNeighbour(pt, i), gwv);

    // und für die Ränder
    UpdateBorderVertex(pt, gwv);
    for(unsigned i = 0; i < 6; ++i)
        UpdateBorderVertex(gwv->GetNeighbour(pt, i), gwv);

    // den selbst sowieso die Punkte darum updaten, da sich bei letzteren die Schattierung geändert haben könnte
    UpdateTriangleColor(pt, gwv, true);
    for(unsigned i = 0; i < 6; ++i)
        UpdateTriangleColor(gwv->GetNeighbour(pt, i), gwv, true);

    // und für die Ränder
    UpdateBorderTriangleColor(pt, gwv, true);
    for(unsigned i = 0; i < 6; ++i)
        UpdateBorderTriangleColor(gwv->GetNeighbour(pt, i), gwv, true);
}


void TerrainRenderer::UpdateAllColors(const GameWorldViewer* gwv)
{
    for(MapCoord y = 0; y < height; ++y)
    {
        for(MapCoord x = 0; x < width; ++x)
        {
            UpdateVertexColor(MapPoint(x, y), gwv);
        }
    }

    for(MapCoord y = 0; y < height; ++y)
    {
        for(MapCoord x = 0; x < width; ++x)
        {
            UpdateBorderVertex(MapPoint(x, y), gwv);
        }
    }

    for(MapCoord y = 0; y < height; ++y)
    {
        for(MapCoord x = 0; x < width; ++x)
        {
            UpdateTriangleColor(MapPoint(x, y), gwv, false);
        }
    }

    for(MapCoord y = 0; y < height; ++y)
    {
        for(MapCoord x = 0; x < width; ++x)
        {
            UpdateBorderTriangleColor(MapPoint(x, y), gwv, false);
        }
    }


    if(SETTINGS.video.vbo)
    {
        // Generiere und Binde den Color Buffer
        glGenBuffersARB(1, (GLuint*)&vbo_colors);
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_colors);
        glBufferDataARB(GL_ARRAY_BUFFER_ARB, (width * height * 2 + border_count) * 3 * 3 * sizeof(float), gl_colors, GL_STATIC_DRAW_ARB );
        glColorPointer(3, GL_FLOAT, 0, NULL);
    }
    else
        glColorPointer(3, GL_FLOAT, 0, gl_colors);
}
