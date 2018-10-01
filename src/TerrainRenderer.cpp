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

#include "rttrDefines.h" // IWYU pragma: keep
#include "TerrainRenderer.h"
#include "ExtensionList.h"
#include "GlobalVars.h"
#include "Loader.h"
#include "Settings.h"
#include "drivers/VideoDriverWrapper.h"
#include "helpers/containerUtils.h"
#include "helpers/converters.h"
#include "network/GameClient.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "ogl/oglIncludes.h"
#include "world/GameWorldBase.h"
#include "world/GameWorldViewer.h"
#include "world/MapGeometry.h"
#include "gameData/MapConsts.h"
#include "gameData/TerrainDesc.h"
#include "libsiedler2/Archiv.h"
#include "libsiedler2/ArchivItem_PaletteAnimation.h"
#include "libutil/Log.h"
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/foreach.hpp>
#include <boost/smart_ptr/scoped_array.hpp>
#include <cstdlib>
#include <set>

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

glArchivItem_Bitmap* new_clone(const glArchivItem_Bitmap& bmp)
{
    return dynamic_cast<glArchivItem_Bitmap*>(bmp.clone());
}

TerrainRenderer::TerrainRenderer() : size_(0, 0), vbo_vertices(0), vbo_texcoords(0), vbo_colors(0), vboBuffersUsed(false) {}

TerrainRenderer::~TerrainRenderer()
{
    if(vboBuffersUsed)
    {
        const GLuint vbos[3] = {vbo_vertices, vbo_texcoords, vbo_colors};
        glDeleteBuffersARB(3, vbos);
    }
}

TerrainRenderer::PointF TerrainRenderer::GetNeighbourVertexPos(MapPoint pt, const unsigned dir) const
{
    // Note: We want the real neighbour point which might be outside of the map to get the offset right
    Position ptNb = ::GetNeighbour(Position(pt), Direction::fromInt(dir));

    Position offset;
    MapPoint t = ConvertCoords(ptNb, &offset);

    return GetVertexPos(t) + PointF(offset);
}

TerrainRenderer::PointF TerrainRenderer::GetNeighbourBorderPos(const MapPoint pt, const unsigned char triangle,
                                                               const unsigned char dir) const
{
    // Note: We want the real neighbour point which might be outside of the map to get the offset right
    Position ptNb = ::GetNeighbour(Position(pt), Direction::fromInt(dir));

    Position offset;
    MapPoint t = ConvertCoords(ptNb, &offset);

    return GetBorderPos(t, triangle) + PointF(offset);
}

void TerrainRenderer::LoadTextures(const WorldDescription& desc)
{
    terrainTextures.clear();
    edgeTextures.clear();
    roadTextures.clear();

    std::set<DescIdx<LandscapeDesc> > usedLandscapes;
    std::set<DescIdx<TerrainDesc> > usedTerrains;
    std::set<DescIdx<EdgeDesc> > usedEdges;
    typedef boost::array<DescIdx<TerrainDesc>, 2> TerrainPair;
    BOOST_FOREACH(const TerrainPair& ts, terrain)
    {
        BOOST_FOREACH(const DescIdx<TerrainDesc>& tIdx, ts)
        {
            if(!helpers::contains(usedTerrains, tIdx))
            {
                usedTerrains.insert(tIdx);
                DescIdx<EdgeDesc> edge = desc.get(tIdx).edgeType;
                if(!!edge)
                    usedEdges.insert(edge);
                DescIdx<LandscapeDesc> lt = desc.get(tIdx).landscape;
                if(!!lt)
                    usedLandscapes.insert(lt);
            }
        }
    }
    terrainTextures.resize(usedTerrains.rbegin()->value + 1);
    edgeTextures.resize(usedEdges.rbegin()->value + 1);
    roadTextures.resize((usedLandscapes.rbegin()->value + 1) * LandscapeDesc::NUM_ROADTYPES);

    BOOST_FOREACH(DescIdx<TerrainDesc> curIdx, usedTerrains)
    {
        const TerrainDesc& cur = desc.get(curIdx);
        std::string textureName = boost::algorithm::to_lower_copy(bfs::path(cur.texturePath).stem().string());
        glArchivItem_Bitmap* texBmp = LOADER.GetImageN(textureName, 0);
        if(!texBmp)
            throw std::runtime_error("Invalid texture '" + cur.texturePath + "' for terrain '" + cur.name + "'");
        if(cur.palAnimIdx >= 0)
        {
            libsiedler2::ArchivItem* animItem = LOADER.GetInfoN(textureName)[cur.palAnimIdx];
            if(!texBmp->getPalette() || !animItem || animItem->getBobType() != libsiedler2::BOBTYPE_PALETTE_ANIM)
            {
                LOG.write("Invalid palette animation '%1%' for '%2%'") % unsigned(cur.palAnimIdx) % cur.name;
                terrainTextures[curIdx.value].textures.push_back(LOADER.ExtractTexture(*texBmp, cur.posInTexture));
                terrainTextures[curIdx.value].textures.back().GetTexture();
            } else
            {
                libsiedler2::ArchivItem_PaletteAnimation& anim = static_cast<libsiedler2::ArchivItem_PaletteAnimation&>(*animItem);
                libsiedler2::Archiv* textures =
                  LOADER.ExtractAnimatedTexture(*texBmp, cur.posInTexture, anim.firstClr, anim.lastClr - anim.firstClr + 1);
                for(unsigned i = 0; i < textures->size(); i++)
                {
                    terrainTextures[curIdx.value].textures.push_back(dynamic_cast<glArchivItem_Bitmap*>(textures->release(i)));
                    terrainTextures[curIdx.value].textures.back().GetTexture();
                }
            }
        } else
        {
            terrainTextures[curIdx.value].textures.push_back(LOADER.ExtractTexture(*texBmp, cur.posInTexture));
            terrainTextures[curIdx.value].textures.back().GetTexture();
        }
        TerrainDesc::Triangle trianglePos = cur.GetRSUTriangle();
        Position texOrigin = cur.posInTexture.getOrigin();
        const glArchivItem_Bitmap& texture = terrainTextures[curIdx.value].textures[0];
        Triangle& rsuCoord = terrainTextures[curIdx.value].rsuCoords;
        PointF texSize(texture.GetTexSize());
        rsuCoord[0] = PointF(trianglePos.tip - texOrigin);
        rsuCoord[1] = PointF(trianglePos.left - texOrigin);
        rsuCoord[2] = PointF(trianglePos.right - texOrigin);
        // Normalize to texture size
        for(unsigned i = 0; i < 3; i++)
            rsuCoord[i] /= texSize;

        Triangle& usdCoord = terrainTextures[curIdx.value].usdCoords;
        trianglePos = cur.GetUSDTriangle();
        usdCoord[0] = PointF(trianglePos.left - texOrigin);
        usdCoord[1] = PointF(trianglePos.tip - texOrigin);
        usdCoord[2] = PointF(trianglePos.right - texOrigin);
        // Normalize to texture size
        for(unsigned i = 0; i < 3; i++)
            usdCoord[i] /= texSize;
    }
    BOOST_FOREACH(DescIdx<EdgeDesc> curIdx, usedEdges)
    {
        const EdgeDesc& cur = desc.get(curIdx);
        std::string textureName = boost::algorithm::to_lower_copy(bfs::path(cur.texturePath).stem().string());
        glArchivItem_Bitmap* texBmp = LOADER.GetImageN(textureName, 0);
        if(!texBmp)
            throw std::runtime_error("Invalid texture '" + cur.texturePath + "' for edge '" + cur.name + "'");
        edgeTextures[curIdx.value].reset(LOADER.ExtractTexture(*texBmp, cur.posInTexture));
        edgeTextures[curIdx.value]->GetTexture(); // Init texture
    }
    BOOST_FOREACH(DescIdx<LandscapeDesc> curIdx, usedLandscapes)
    {
        const LandscapeDesc& cur = desc.get(curIdx);
        for(unsigned i = 0; i < cur.roadTexDesc.size(); i++)
        {
            std::string textureName = boost::algorithm::to_lower_copy(bfs::path(cur.roadTexDesc[i].texturePath).stem().string());
            glArchivItem_Bitmap* texBmp = LOADER.GetImageN(textureName, 0);
            if(!texBmp)
                throw std::runtime_error("Invalid texture '" + cur.roadTexDesc[i].texturePath + "' for road in landscape '" + cur.name
                                         + "'");
            roadTextures[curIdx.value * cur.NUM_ROADTYPES + i].reset(LOADER.ExtractTexture(*texBmp, cur.roadTexDesc[i].posInTexture));
            roadTextures[curIdx.value * cur.NUM_ROADTYPES + i]->GetTexture(); // Init texture
        }
    }
}

void TerrainRenderer::GenerateVertices(const GameWorldViewer& gwv)
{
    // Terrain generieren
    RTTR_FOREACH_PT(MapPoint, size_)
    {
        UpdateVertexPos(pt, gwv);
        UpdateVertexColor(pt, gwv);
        LoadVertexTerrain(pt, gwv);
    }

    // Ränder generieren
    RTTR_FOREACH_PT(MapPoint, size_)
        UpdateBorderVertex(pt);
}

void TerrainRenderer::UpdateVertexPos(const MapPoint pt, const GameWorldViewer& gwv)
{
    GetVertex(pt).pos = Point<float>(gwv.GetWorld().GetNodePos(pt));
}

void TerrainRenderer::UpdateVertexColor(const MapPoint pt, const GameWorldViewer& gwv)
{
    float shadow = static_cast<float>(gwv.GetNode(pt).shadow);
    float clr = -1.f / (256.f * 256.f) * shadow * shadow + 1.f / 90.f * shadow + 0.38f;
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

void TerrainRenderer::LoadVertexTerrain(const MapPoint pt, const GameWorldViewer& gwv)
{
    const MapNode& node = gwv.GetNode(pt);
    terrain[GetVertexIdx(pt)][0] = node.t1;
    terrain[GetVertexIdx(pt)][1] = node.t2;
}

void TerrainRenderer::UpdateBorderVertex(const MapPoint pt)
{
    Vertex& vertex = GetVertex(pt);
    vertex.borderPos[0] = (GetNeighbourVertexPos(pt, 5) + GetVertexPos(pt) + GetNeighbourVertexPos(pt, 4)) / 3.0f;
    vertex.borderColor[0] =
      (GetColor(GetNeighbour(pt, Direction::SOUTHWEST)) + GetColor(pt) + GetColor(GetNeighbour(pt, Direction::SOUTHEAST))) / 3.0f;

    vertex.borderPos[1] = (GetNeighbourVertexPos(pt, 3) + GetVertexPos(pt) + GetNeighbourVertexPos(pt, 4)) / 3.0f;
    vertex.borderColor[1] =
      (GetColor(GetNeighbour(pt, Direction::EAST)) + GetColor(pt) + GetColor(GetNeighbour(pt, Direction::SOUTHEAST))) / 3.0f;
}

void TerrainRenderer::Init(const MapExtent& size)
{
    size_ = size;
    // Clear first, so they are default-initialized
    vertices.clear();
    terrain.clear();
    borders.clear();
    vertices.resize(size_.x * size_.y);
    terrain.resize(vertices.size());
    borders.resize(size_.x * size_.y);

    gl_vertices.clear();
    gl_texcoords.clear();
    gl_colors.clear();
    // We have 2 triangles per map point
    gl_vertices.resize(vertices.size() * 2);
    gl_texcoords.resize(gl_vertices.size());
    gl_colors.resize(gl_vertices.size());
}

/// Gets the edge type that t1 draws over t2. 0 = None, else edgeType + 1
static uint8_t GetEdgeType(const TerrainDesc& t1, const TerrainDesc& t2)
{
    if(!t1.edgeType || t1.edgePriority <= t2.edgePriority)
        return 0;
    else
        return t1.edgeType.value + 1;
}

/**
 *  erzeugt die OpenGL-Vertices.
 */
void TerrainRenderer::GenerateOpenGL(const GameWorldViewer& gwv)
{
    const GameWorldBase& world = gwv.GetWorld();
    Init(world.GetSize());

    GenerateVertices(gwv);
    const WorldDescription& desc = world.GetDescription();
    LoadTextures(desc);

    // Add extra vertices for borders
    unsigned numTriangles = gl_vertices.size();
    RTTR_FOREACH_PT(MapPoint, size_)
    {
        const unsigned pos = GetVertexIdx(pt);
        const TerrainDesc& t1 = desc.get(terrain[pos][0]);
        const TerrainDesc& t2 = desc.get(terrain[pos][1]);
        const TerrainDesc& t3 = desc.get(terrain[GetVertexIdx(GetNeighbour(pt, Direction::EAST))][0]);
        const TerrainDesc& t4 = desc.get(terrain[GetVertexIdx(GetNeighbour(pt, Direction::SOUTHWEST))][1]);

        if((borders[pos].left_right[0] = GetEdgeType(t2, t1)))
            borders[pos].left_right_offset[0] = numTriangles++;
        if((borders[pos].left_right[1] = GetEdgeType(t1, t2)))
            borders[pos].left_right_offset[1] = numTriangles++;

        if((borders[pos].right_left[0] = GetEdgeType(t3, t2)))
            borders[pos].right_left_offset[0] = numTriangles++;
        if((borders[pos].right_left[1] = GetEdgeType(t2, t3)))
            borders[pos].right_left_offset[1] = numTriangles++;

        if((borders[pos].top_down[0] = GetEdgeType(t4, t1)))
            borders[pos].top_down_offset[0] = numTriangles++;
        if((borders[pos].top_down[1] = GetEdgeType(t1, t4)))
            borders[pos].top_down_offset[1] = numTriangles++;
    }

    gl_vertices.resize(numTriangles);
    gl_texcoords.resize(numTriangles);
    gl_colors.resize(numTriangles);

    // Normales Terrain erzeugen
    RTTR_FOREACH_PT(MapPoint, size_)
    {
        UpdateTrianglePos(pt, false);
        UpdateTriangleColor(pt, false);
        UpdateTriangleTerrain(pt, false);
    }

    // Ränder erzeugen
    RTTR_FOREACH_PT(MapPoint, size_)
    {
        UpdateBorderTrianglePos(pt, false);
        UpdateBorderTriangleColor(pt, false);
        UpdateBorderTriangleTerrain(pt, false);
    }

    if(SETTINGS.video.vbo && GLOBALVARS.ext_vbo)
    {
        // Create and fill the 3 VBOs for vertices, texCoords and colors
        GLuint vbos[3];
        glGenBuffersARB(3, vbos);
        BOOST_STATIC_ASSERT_MSG(sizeof(vbo_vertices) >= sizeof(GLuint), "Cannot store Gluint in vbo variable!");
        if(!vbos[0] || !vbos[1] || !vbos[2])
            glDeleteBuffersARB(3, vbos);
        else
        {
            vbo_vertices = vbos[0];
            vbo_texcoords = vbos[1];
            vbo_colors = vbos[2];

            glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_vertices);
            glBufferDataARB(GL_ARRAY_BUFFER_ARB, gl_vertices.size() * sizeof(Triangle), &gl_vertices.front(), GL_STATIC_DRAW_ARB);

            glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_texcoords);
            glBufferDataARB(GL_ARRAY_BUFFER_ARB, gl_texcoords.size() * sizeof(Triangle), &gl_texcoords.front(), GL_STATIC_DRAW_ARB);

            glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_colors);
            glBufferDataARB(GL_ARRAY_BUFFER_ARB, gl_colors.size() * sizeof(ColorTriangle), &gl_colors.front(), GL_STATIC_DRAW_ARB);

            // Unbind VBO to not interfere with other program parts
            glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
            vboBuffersUsed = true;
        }
    }
}

void TerrainRenderer::UpdateTrianglePos(const MapPoint pt, bool updateVBO)
{
    unsigned pos = GetTriangleIdx(pt);

    gl_vertices[pos][0] = GetVertexPos(pt);
    gl_vertices[pos][1] = GetNeighbourVertexPos(pt, 5);
    gl_vertices[pos][2] = GetNeighbourVertexPos(pt, 4);

    ++pos;

    gl_vertices[pos][0] = GetVertexPos(pt);
    gl_vertices[pos][1] = GetNeighbourVertexPos(pt, 4);
    gl_vertices[pos][2] = GetNeighbourVertexPos(pt, 3);

    if(updateVBO && vboBuffersUsed)
    {
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_vertices);
        glBufferSubDataARB(GL_ARRAY_BUFFER_ARB, (pos - 1) * sizeof(Triangle), 2 * sizeof(Triangle), &gl_vertices[pos - 1]);
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
    }
}

void TerrainRenderer::UpdateTriangleColor(const MapPoint pt, bool updateVBO)
{
    unsigned pos = GetTriangleIdx(pt);

    Color& clr0 = gl_colors[pos][0];
    Color& clr1 = gl_colors[pos][1];
    Color& clr2 = gl_colors[pos][2];
    clr0.r = clr0.g = clr0.b = GetColor(pt);
    clr1.r = clr1.g = clr1.b = GetColor(GetNeighbour(pt, Direction::SOUTHWEST));
    clr2.r = clr2.g = clr2.b = GetColor(GetNeighbour(pt, Direction::SOUTHEAST));

    ++pos;

    Color& clr3 = gl_colors[pos][0];
    Color& clr4 = gl_colors[pos][1];
    Color& clr5 = gl_colors[pos][2];
    clr3.r = clr3.g = clr3.b = GetColor(pt);
    clr4.r = clr4.g = clr4.b = GetColor(GetNeighbour(pt, Direction::SOUTHEAST));
    clr5.r = clr5.g = clr5.b = GetColor(GetNeighbour(pt, Direction::EAST));

    if(updateVBO && vboBuffersUsed)
    {
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_colors);
        glBufferSubDataARB(GL_ARRAY_BUFFER_ARB, (pos - 1) * sizeof(ColorTriangle), 2 * sizeof(ColorTriangle), &gl_colors[pos - 1]);
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
    }
}

void TerrainRenderer::UpdateTriangleTerrain(const MapPoint pt, bool updateVBO)
{
    const unsigned nodeIdx = GetVertexIdx(pt);
    const DescIdx<TerrainDesc> t1 = terrain[nodeIdx][0];
    const DescIdx<TerrainDesc> t2 = terrain[nodeIdx][1];

    const unsigned triangleIdx = GetTriangleIdx(pt);
    gl_texcoords[triangleIdx] = terrainTextures[t1.value].rsuCoords;
    gl_texcoords[triangleIdx + 1] = terrainTextures[t2.value].usdCoords;

    if(updateVBO && vboBuffersUsed)
    {
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_texcoords);
        glBufferSubDataARB(GL_ARRAY_BUFFER_ARB, (triangleIdx - 1) * sizeof(Triangle), 2 * sizeof(Triangle), &gl_texcoords[triangleIdx - 1]);
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
    }
}

/// Erzeugt die Dreiecke für die Ränder
void TerrainRenderer::UpdateBorderTrianglePos(const MapPoint pt, bool updateVBO)
{
    unsigned pos = GetVertexIdx(pt);

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
        unsigned offset = borders[pos].left_right_offset[i];

        if(!first_offset)
            first_offset = offset;

        gl_vertices[offset][i ? 0 : 2] = GetVertexPos(pt);
        gl_vertices[offset][1] = GetNeighbourVertexPos(pt, 4);
        gl_vertices[offset][i ? 2 : 0] = GetBorderPos(pt, i);

        ++count_borders;
    }

    // Rand rechts - links
    for(unsigned char i = 0; i < 2; ++i)
    {
        if(!borders[pos].right_left[i])
            continue;
        unsigned offset = borders[pos].right_left_offset[i];

        if(!first_offset)
            first_offset = offset;

        gl_vertices[offset][i ? 2 : 0] = GetNeighbourVertexPos(pt, 4);
        gl_vertices[offset][1] = GetNeighbourVertexPos(pt, 3);

        if(i == 0)
            gl_vertices[offset][2] = GetBorderPos(pt, 1);
        else
            gl_vertices[offset][0] = GetNeighbourBorderPos(pt, 0, 3);

        ++count_borders;
    }

    // Rand oben - unten
    for(unsigned char i = 0; i < 2; ++i)
    {
        if(!borders[pos].top_down[i])
            continue;
        unsigned offset = borders[pos].top_down_offset[i];

        if(!first_offset)
            first_offset = offset;

        gl_vertices[offset][i ? 2 : 0] = GetNeighbourVertexPos(pt, 5);
        gl_vertices[offset][1] = GetNeighbourVertexPos(pt, 4);

        if(i == 0)
            gl_vertices[offset][2] = GetBorderPos(pt, i);
        else
            gl_vertices[offset][0] = GetNeighbourBorderPos(pt, i, 5);

        ++count_borders;
    }

    if(updateVBO && vboBuffersUsed)
    {
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_vertices);
        glBufferSubDataARB(GL_ARRAY_BUFFER_ARB, first_offset * sizeof(Triangle), count_borders * sizeof(Triangle),
                           &gl_vertices[first_offset]);
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
    }
}

void TerrainRenderer::UpdateBorderTriangleColor(const MapPoint pt, bool updateVBO)
{
    unsigned pos = GetVertexIdx(pt);

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
        unsigned offset = borders[pos].left_right_offset[i];

        if(!first_offset)
            first_offset = offset;

        gl_colors[offset][i ? 0 : 2].r = gl_colors[offset][i ? 0 : 2].g = gl_colors[offset][i ? 0 : 2].b = GetColor(pt);             //-V807
        gl_colors[offset][1].r = gl_colors[offset][1].g = gl_colors[offset][1].b = GetColor(GetNeighbour(pt, Direction::SOUTHEAST)); //-V807
        gl_colors[offset][i ? 2 : 0].r = gl_colors[offset][i ? 2 : 0].g = gl_colors[offset][i ? 2 : 0].b = GetBorderColor(pt, i);    //-V807

        ++count_borders;
    }

    // Rand rechts - links
    for(unsigned char i = 0; i < 2; ++i)
    {
        if(!borders[pos].right_left[i])
            continue;
        unsigned offset = borders[pos].right_left_offset[i];

        if(!first_offset)
            first_offset = offset;

        gl_colors[offset][i ? 2 : 0].r = gl_colors[offset][i ? 2 : 0].g = gl_colors[offset][i ? 2 : 0].b =
          GetColor(GetNeighbour(pt, Direction::SOUTHEAST));
        gl_colors[offset][1].r = gl_colors[offset][1].g = gl_colors[offset][1].b = GetColor(GetNeighbour(pt, Direction::EAST));
        MapPoint pt2(pt.x + i, pt.y);
        if(pt2.x >= size_.x)
            pt2.x -= size_.x;
        gl_colors[offset][i ? 0 : 2].r = gl_colors[offset][i ? 0 : 2].g = gl_colors[offset][i ? 0 : 2].b = GetBorderColor(pt2, i ? 0 : 1);

        ++count_borders;
    }

    // Rand oben - unten
    for(unsigned char i = 0; i < 2; ++i)
    {
        if(!borders[pos].top_down[i])
            continue;
        unsigned offset = borders[pos].top_down_offset[i];

        if(!first_offset)
            first_offset = offset;

        gl_colors[offset][i ? 2 : 0].r = gl_colors[offset][i ? 2 : 0].g = gl_colors[offset][i ? 2 : 0].b =
          GetColor(GetNeighbour(pt, Direction::SOUTHWEST));
        gl_colors[offset][1].r = gl_colors[offset][1].g = gl_colors[offset][1].b = GetColor(GetNeighbour(pt, Direction::SOUTHEAST));

        if(i == 0)
            gl_colors[offset][2].r = gl_colors[offset][2].g = gl_colors[offset][2].b = GetBorderColor(pt, i); //-V807
        else
            gl_colors[offset][0].r = gl_colors[offset][0].g = gl_colors[offset][0].b =
              GetBorderColor(GetNeighbour(pt, Direction::SOUTHWEST), i); //-V807

        ++count_borders;
    }

    if(updateVBO && vboBuffersUsed)
    {
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_colors);
        glBufferSubDataARB(GL_ARRAY_BUFFER_ARB, first_offset * sizeof(ColorTriangle), count_borders * sizeof(ColorTriangle),
                           &gl_colors[first_offset]);
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
    }
}

void TerrainRenderer::UpdateBorderTriangleTerrain(const MapPoint pt, bool updateVBO)
{
    unsigned pos = GetVertexIdx(pt);

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
            unsigned offset = borders[pos].left_right_offset[i];

            if(!first_offset)
                first_offset = offset;

            const glArchivItem_Bitmap& texture = *edgeTextures[borders[pos].left_right[i] - 1];
            Extent bmpSize = texture.GetSize();
            PointF texSize(texture.GetTexSize());

            gl_texcoords[offset][i ? 0 : 2] = PointF(0.0f, 0.0f);
            gl_texcoords[offset][1] = PointF(bmpSize.x / texSize.x, 0.0f);
            gl_texcoords[offset][i ? 2 : 0] = PointF(bmpSize.x / texSize.x / 2.f, bmpSize.y / texSize.y);

            ++count_borders;
        }
    }

    // Rand rechts - links
    for(unsigned char i = 0; i < 2; ++i)
    {
        if(borders[pos].right_left[i])
        {
            unsigned offset = borders[pos].right_left_offset[i];

            if(!first_offset)
                first_offset = offset;

            const glArchivItem_Bitmap& texture = *edgeTextures[borders[pos].right_left[i] - 1];
            Extent bmpSize = texture.GetSize();
            PointF texSize(texture.GetTexSize());

            gl_texcoords[offset][i ? 2 : 0] = PointF(0.0f, 0.0f);
            gl_texcoords[offset][1] = PointF(bmpSize.x / texSize.x, 0.0f);
            gl_texcoords[offset][i ? 0 : 2] = PointF(bmpSize.x / texSize.x / 2.f, bmpSize.y / texSize.y);

            ++count_borders;
        }
    }

    // Rand oben - unten
    for(unsigned char i = 0; i < 2; ++i)
    {
        if(borders[pos].top_down[i])
        {
            unsigned offset = borders[pos].top_down_offset[i];

            if(!first_offset)
                first_offset = offset;

            const glArchivItem_Bitmap& texture = *edgeTextures[borders[pos].top_down[i] - 1];
            Extent bmpSize = texture.GetSize();
            PointF texSize(texture.GetTexSize());

            gl_texcoords[offset][i ? 2 : 0] = PointF(0.0f, 0.0f);
            gl_texcoords[offset][1] = PointF(bmpSize.x / texSize.x, 0.0f);
            gl_texcoords[offset][i ? 0 : 2] = PointF(bmpSize.x / texSize.x / 2.f, bmpSize.y / texSize.y);

            ++count_borders;
        }
    }

    if(updateVBO && vboBuffersUsed)
    {
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_texcoords);
        glBufferSubDataARB(GL_ARRAY_BUFFER_ARB, first_offset * sizeof(Triangle), count_borders * sizeof(Triangle),
                           &gl_texcoords[first_offset]);
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
    }
}

/**
 *  zeichnet den Kartenausschnitt.
 */
void TerrainRenderer::Draw(const Position& firstPt, const Position& lastPt, const GameWorldViewer& gwv, unsigned* water) const
{
    RTTR_Assert(!gl_vertices.empty());
    RTTR_Assert(!borders.empty());

    // nach Texture in Listen sortieren
    std::vector<std::vector<MapTile> > sorted_textures(terrainTextures.size());
    std::vector<std::vector<BorderTile> > sorted_borders(edgeTextures.size());
    PreparedRoads sorted_roads(roadTextures.size());

    Position lastOffset(0, 0);

    // Beim zeichnen immer nur beginnen, wo man auch was sieht
    for(int y = firstPt.y; y <= lastPt.y; ++y)
    {
        unsigned char lastTerrain = 255;
        unsigned char lastBorder = 255;

        for(int x = firstPt.x; x <= lastPt.x; ++x)
        {
            Position posOffset;
            MapPoint tP = ConvertCoords(Position(x, y), &posOffset);

            unsigned char t = terrain[GetVertexIdx(tP)][0].value;
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

            t = terrain[GetVertexIdx(tP)][1].value;

            if(t == lastTerrain)
                ++sorted_textures[t].back().count;
            else
            {
                MapTile tmp(GetTriangleIdx(tP) + 1, posOffset);
                sorted_textures[t].push_back(tmp);
            }

            lastTerrain = t;

            const Borders& curBorders = borders[GetVertexIdx(tP)];
            boost::array<unsigned char, 6> tiles = {{curBorders.left_right[0], curBorders.left_right[1], curBorders.right_left[0],
                                                     curBorders.right_left[1], curBorders.top_down[0], curBorders.top_down[1]}};

            // Offsets into gl_* arrays
            boost::array<unsigned, 6> offsets = {{curBorders.left_right_offset[0], curBorders.left_right_offset[1],
                                                  curBorders.right_left_offset[0], curBorders.right_left_offset[1],
                                                  curBorders.top_down_offset[0], curBorders.top_down_offset[1]}};

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

    if(water)
    {
        unsigned water_count = 0;
        const WorldDescription& desc = gwv.GetWorld().GetDescription();
        for(DescIdx<TerrainDesc> t(0); t.value < sorted_textures.size(); ++t.value)
        {
            if(desc.get(t).kind != TerrainKind::WATER)
                continue;
            BOOST_FOREACH(const MapTile& tile, sorted_textures[t.value])
                water_count += tile.count;
        }

        Position diff = lastPt - firstPt;
        if(diff.x && diff.y)
            *water = 50 * water_count / (diff.x * diff.y);
        else
            *water = 0;
    }

    lastOffset = Position(0, 0);

    // Arrays aktivieren
    glEnableClientState(GL_COLOR_ARRAY);

    if(vboBuffersUsed)
    {
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_vertices);
        glVertexPointer(2, GL_FLOAT, 0, 0);

        glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_texcoords);
        glTexCoordPointer(2, GL_FLOAT, 0, 0);

        glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_colors);
        glColorPointer(3, GL_FLOAT, 0, 0);
    } else
    {
        glVertexPointer(2, GL_FLOAT, 0, &gl_vertices.front());
        glTexCoordPointer(2, GL_FLOAT, 0, &gl_texcoords.front());
        glColorPointer(3, GL_FLOAT, 0, &gl_colors.front());
    }

    // Modulate2x
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);
    glTexEnvf(GL_TEXTURE_ENV, GL_RGB_SCALE_EXT, 2.0f);

    // Alphablending aus
    glDisable(GL_BLEND);

    glPushMatrix();
    for(unsigned t = 0; t < sorted_textures.size(); ++t)
    {
        if(sorted_textures[t].empty())
            continue;
        unsigned animationFrame;
        unsigned numFrames = terrainTextures[t].textures.size();
        if(numFrames > 1)
            animationFrame = GAMECLIENT.GetGlobalAnimation(numFrames, 5 * numFrames, 16, 0); // We have 5/16 per frame
        else
            animationFrame = 0;

        VIDEODRIVER.BindTexture(terrainTextures[t].textures[animationFrame].GetTextureNoCreate());

        for(std::vector<MapTile>::iterator it = sorted_textures[t].begin(); it != sorted_textures[t].end(); ++it)
        {
            if(it->posOffset != lastOffset)
            {
                Position trans = it->posOffset - lastOffset;
                glTranslatef(float(trans.x), float(trans.y), 0.0f);
                lastOffset = it->posOffset;
            }

            RTTR_Assert(it->tileOffset + it->count <= size_.x * size_.y * 2u);
            glDrawArrays(GL_TRIANGLES, it->tileOffset * 3, it->count * 3); // Arguments are in Elements. 1 triangle has 3 values
        }
    }
    glPopMatrix();

    glEnable(GL_BLEND);

    lastOffset = Position(0, 0);
    glPushMatrix();
    for(unsigned i = 0; i < sorted_borders.size(); ++i)
    {
        if(sorted_borders[i].empty())
            continue;
        VIDEODRIVER.BindTexture(edgeTextures[i]->GetTextureNoCreate());

        for(std::vector<BorderTile>::iterator it = sorted_borders[i].begin(); it != sorted_borders[i].end(); ++it)
        {
            if(it->posOffset != lastOffset)
            {
                Position trans = it->posOffset - lastOffset;
                glTranslatef(float(trans.x), float(trans.y), 0.0f);
                lastOffset = it->posOffset;
            }
            RTTR_Assert(it->tileOffset + it->count <= gl_vertices.size());
            glDrawArrays(GL_TRIANGLES, it->tileOffset * 3, it->count * 3); // Arguments are in Elements. 1 triangle has 3 values
        }
    }
    glPopMatrix();

    // unbind VBO
    if(vboBuffersUsed)
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);

    DrawWays(sorted_roads);

    glDisableClientState(GL_COLOR_ARRAY);
    // Wieder zurück ins normale modulate
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
}

MapPoint TerrainRenderer::ConvertCoords(const Position pt, Position* offset) const
{
    if(offset)
    {
        // We need the screen offset by which we shifted the point
        // this is the world size (in screen units) times the number we shifted the point
        // This factor is floor(pos/size) which we do here in integer
        // and avoid implementation defined (prior C++11) rounding for negative values by using the integer ceil on the inverse
        if(pt.x >= 0)
            offset->x = pt.x / size_.x;
        else
            offset->x = -((-pt.x + size_.x - 1) / size_.x);
        if(pt.y >= 0)
            offset->y = pt.y / size_.y;
        else
            offset->y = -((-pt.y + size_.y - 1) / size_.y);
        offset->x *= (TR_W * size_.x);
        offset->y *= (TR_H * size_.y);
    }
    MapPoint ptOut = MakeMapPoint(pt, size_);
    RTTR_Assert(ptOut.x < size_.x);
    RTTR_Assert(ptOut.y < size_.y);
    RTTR_Assert(!offset || pt.x - ptOut.x == offset->x / TR_W);
    RTTR_Assert(!offset || pt.y - ptOut.y == offset->y / TR_H);
    return ptOut;
}

void TerrainRenderer::PrepareWaysPoint(PreparedRoads& sorted_roads, const GameWorldViewer& gwViewer, MapPoint pt,
                                       const Position& offset) const
{
    const WorldDescription& desc = gwViewer.GetWorld().GetDescription();
    Position startPos = Position(GetVertexPos(pt)) + offset;

    Visibility visibility = gwViewer.GetVisibility(pt);

    int totalWidth = size_.x * TR_W;
    int totalHeight = size_.y * TR_H;

    // Wegtypen für die drei Richtungen
    for(unsigned dir = 0; dir < 3; ++dir)
    {
        unsigned char type = gwViewer.GetVisibleRoad(pt, dir, visibility);
        if(!type)
            continue;
        Direction targetDir = Direction::fromInt(3 + dir);
        MapPoint ta = gwViewer.GetNeighbour(pt, targetDir);

        Position endPos = Position(GetVertexPos(ta)) + offset;
        Position diff = startPos - endPos;

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

        RoadSegment::RoadType rt = RoadSegment::RoadType(type - 1);
        // The gfx road type is:
        // Boat for boat roads
        // else Mountain left or right is a mountain terrain
        // else Upgraded for Donkey roads
        // else Normal
        uint8_t gfxRoadType;
        if(rt == RoadSegment::RT_BOAT)
            gfxRoadType = desc.get(gwViewer.GetWorld().GetLeftTerrain(pt, targetDir)).landscape.value * LandscapeDesc::NUM_ROADTYPES
                          + LandscapeDesc::Boat;
        else
        {
            const TerrainDesc& lTerrain = desc.get(gwViewer.GetWorld().GetLeftTerrain(pt, targetDir));
            if(lTerrain.kind == TerrainKind::MOUNTAIN)
                gfxRoadType = lTerrain.landscape.value * LandscapeDesc::NUM_ROADTYPES + LandscapeDesc::Mountain;
            else
            {
                const TerrainDesc& rTerrain = desc.get(gwViewer.GetWorld().GetRightTerrain(pt, targetDir));
                if(rTerrain.kind == TerrainKind::MOUNTAIN)
                    gfxRoadType = rTerrain.landscape.value * LandscapeDesc::NUM_ROADTYPES + LandscapeDesc::Mountain;
                else
                    gfxRoadType = lTerrain.landscape.value * LandscapeDesc::NUM_ROADTYPES
                                  + ((rt == RoadSegment::RT_DONKEY) ? LandscapeDesc::Upgraded : LandscapeDesc::Normal);
            }
        }

        sorted_roads[gfxRoadType].push_back(PreparedRoad(startPos, endPos, GetColor(pt), GetColor(ta), dir));
    }
}

struct Tex2C3Ver2
{
    GLfloat tx, ty;
    GLfloat r, g, b;
    GLfloat x, y;
};

void TerrainRenderer::DrawWays(const PreparedRoads& sorted_roads) const
{
    // 2D Array: [3][4]
    static const boost::array<Position, 12> begin_end_coords = {{Position(0, -3), Position(0, 3), Position(3, 3), Position(3, -3),
                                                                 Position(2, -4), Position(-4, 2), Position(0, 6), Position(6, 0),
                                                                 Position(4, 2), Position(-2, -4), Position(-6, 0), Position(0, 6)}};

    size_t maxSize = 0;
    BOOST_FOREACH(const boost::container::vector<PreparedRoad>& sorted_road, sorted_roads)
        maxSize = std::max(maxSize, sorted_road.size());

    if(maxSize == 0)
        return;

    boost::scoped_array<Tex2C3Ver2> vertexData(new Tex2C3Ver2[maxSize * 4]);
    // These should still be enabled
    RTTR_Assert(glIsEnabled(GL_VERTEX_ARRAY));
    RTTR_Assert(glIsEnabled(GL_TEXTURE_COORD_ARRAY));
    RTTR_Assert(glIsEnabled(GL_COLOR_ARRAY));
    glVertexPointer(2, GL_FLOAT, sizeof(Tex2C3Ver2), &vertexData[0].x);
    glTexCoordPointer(2, GL_FLOAT, sizeof(Tex2C3Ver2), &vertexData[0].tx);
    glColorPointer(3, GL_FLOAT, sizeof(Tex2C3Ver2), &vertexData[0].r);

    int type = 0;
    for(PreparedRoads::const_iterator itRoad = sorted_roads.begin(); itRoad != sorted_roads.end(); ++itRoad, ++type)
    {
        if(itRoad->empty())
            continue;
        Tex2C3Ver2* curVertexData = vertexData.get();
        const glArchivItem_Bitmap& texture = *roadTextures[type];
        PointF scaledTexSize = texture.GetSize() / PointF(texture.GetTexSize());

        for(PreparedRoads::value_type::const_iterator it = itRoad->begin(); it != itRoad->end(); ++it)
        {
            RTTR_Assert(it->dir < 3); // begin_end_coords has 3 dir entries
            curVertexData->tx = 0.0f;
            curVertexData->ty = 0.0f;
            curVertexData->r = curVertexData->g = curVertexData->b = it->color1;
            Position tmpP = it->pos + begin_end_coords[it->dir * 4];
            curVertexData->x = GLfloat(tmpP.x);
            curVertexData->y = GLfloat(tmpP.y);

            curVertexData++;

            curVertexData->tx = 0.0f;
            curVertexData->ty = scaledTexSize.y;
            curVertexData->r = curVertexData->g = curVertexData->b = it->color1;
            tmpP = it->pos + begin_end_coords[it->dir * 4 + 1];
            curVertexData->x = GLfloat(tmpP.x);
            curVertexData->y = GLfloat(tmpP.y);

            curVertexData++;

            curVertexData->tx = scaledTexSize.x;
            curVertexData->ty = scaledTexSize.y;
            curVertexData->r = curVertexData->g = curVertexData->b = it->color2;
            tmpP = it->pos2 + begin_end_coords[it->dir * 4 + 2];
            curVertexData->x = GLfloat(tmpP.x);
            curVertexData->y = GLfloat(tmpP.y);

            curVertexData++;

            curVertexData->tx = scaledTexSize.x;
            curVertexData->ty = 0.0f;
            curVertexData->r = curVertexData->g = curVertexData->b = it->color2;
            tmpP = it->pos2 + begin_end_coords[it->dir * 4 + 3];
            curVertexData->x = GLfloat(tmpP.x);
            curVertexData->y = GLfloat(tmpP.y);

            curVertexData++;
        }

        VIDEODRIVER.BindTexture(roadTextures[type]->GetTextureNoCreate());
        glDrawArrays(GL_QUADS, 0, itRoad->size() * 4);
    }
    // Note: No glDisableClientState as we did not enable it
}

void TerrainRenderer::AltitudeChanged(const MapPoint pt, const GameWorldViewer& gwv)
{
    // den selbst sowieso die Punkte darum updaten, da sich bei letzteren die Schattierung geändert haben könnte
    UpdateVertexPos(pt, gwv);
    UpdateVertexColor(pt, gwv);

    for(unsigned i = 0; i < 6; ++i)
        UpdateVertexColor(gwv.GetNeighbour(pt, Direction::fromInt(i)), gwv);

    // und für die Ränder
    UpdateBorderVertex(pt);

    for(unsigned i = 0; i < 6; ++i)
        UpdateBorderVertex(gwv.GetNeighbour(pt, Direction::fromInt(i)));

    // den selbst sowieso die Punkte darum updaten, da sich bei letzteren die Schattierung geändert haben könnte
    UpdateTrianglePos(pt, true);
    UpdateTriangleColor(pt, true);

    for(unsigned i = 0; i < 6; ++i)
    {
        UpdateTrianglePos(gwv.GetNeighbour(pt, Direction::fromInt(i)), true);
        UpdateTriangleColor(gwv.GetNeighbour(pt, Direction::fromInt(i)), true);
    }

    // Auch im zweiten Kreis drumherum die Dreiecke neu berechnen, da die durch die Schattenänderung der umliegenden
    // Punkte auch geändert werden könnten
    for(unsigned i = 0; i < 12; ++i)
        UpdateTriangleColor(gwv.GetWorld().GetNeighbour2(pt, i), true);

    // und für die Ränder
    UpdateBorderTrianglePos(pt, true);
    UpdateBorderTriangleColor(pt, true);

    for(unsigned i = 0; i < 6; ++i)
    {
        UpdateBorderTrianglePos(gwv.GetNeighbour(pt, Direction::fromInt(i)), true);
        UpdateBorderTriangleColor(gwv.GetNeighbour(pt, Direction::fromInt(i)), true);
    }

    for(unsigned i = 0; i < 12; ++i)
        UpdateBorderTriangleColor(gwv.GetWorld().GetNeighbour2(pt, i), true);
}

void TerrainRenderer::VisibilityChanged(const MapPoint pt, const GameWorldViewer& gwv)
{
    /// Noch kein Terrain gebaut? abbrechen
    if(vertices.empty())
        return;

    UpdateVertexColor(pt, gwv);
    for(unsigned i = 0; i < 6; ++i)
        UpdateVertexColor(gwv.GetNeighbour(pt, Direction::fromInt(i)), gwv);

    // und für die Ränder
    UpdateBorderVertex(pt);
    for(unsigned i = 0; i < 6; ++i)
        UpdateBorderVertex(gwv.GetNeighbour(pt, Direction::fromInt(i)));

    // den selbst sowieso die Punkte darum updaten, da sich bei letzteren die Schattierung geändert haben könnte
    UpdateTriangleColor(pt, true);
    for(unsigned i = 0; i < 6; ++i)
        UpdateTriangleColor(gwv.GetNeighbour(pt, Direction::fromInt(i)), true);

    // und für die Ränder
    UpdateBorderTriangleColor(pt, true);
    for(unsigned i = 0; i < 6; ++i)
        UpdateBorderTriangleColor(gwv.GetNeighbour(pt, Direction::fromInt(i)), true);
}

void TerrainRenderer::UpdateAllColors(const GameWorldViewer& gwv)
{
    RTTR_FOREACH_PT(MapPoint, size_)
        UpdateVertexColor(pt, gwv);

    RTTR_FOREACH_PT(MapPoint, size_)
        UpdateBorderVertex(pt);

    RTTR_FOREACH_PT(MapPoint, size_)
        UpdateTriangleColor(pt, false);

    RTTR_FOREACH_PT(MapPoint, size_)
        UpdateBorderTriangleColor(pt, false);

    if(vboBuffersUsed)
    {
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_colors);
        glBufferDataARB(GL_ARRAY_BUFFER_ARB, gl_colors.size() * sizeof(ColorTriangle), &gl_colors.front(), GL_STATIC_DRAW_ARB);
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
    }
}

MapPoint TerrainRenderer::GetNeighbour(const MapPoint& pt, const Direction dir) const
{
    return MakeMapPoint(::GetNeighbour(Position(pt), dir), size_);
}
