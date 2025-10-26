// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "TerrainRenderer.h"
#include "Loader.h"
#include "RttrForeachPt.h"
#include "Settings.h"
#include "drivers/VideoDriverWrapper.h"
#include "helpers/EnumArray.h"
#include "helpers/Range.h"
#include "helpers/containerUtils.h"
#include "network/GameClient.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "world/GameWorldBase.h"
#include "world/GameWorldViewer.h"
#include "world/MapGeometry.h"
#include "gameData/MapConsts.h"
#include "gameData/TerrainDesc.h"
#include "libsiedler2/Archiv.h"
#include "libsiedler2/ArchivItem_PaletteAnimation.h"
#include "s25util/Log.h"
#include <glad/glad.h>
#include <boost/pointer_cast.hpp>
#include <boost/range/adaptor/indexed.hpp>
#include <cstdlib>
#include <openglCfg.hpp>
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

namespace {
// gl4es has some problems with GL_MODULATE so we need to change the colors without using a gl function
#if RTTR_OGL_GL4ES
constexpr float TEXTURE_COLOR_DIVISOR = 1;
#else
constexpr float TEXTURE_COLOR_DIVISOR = 2;
#endif
} // namespace

glArchivItem_Bitmap* new_clone(const glArchivItem_Bitmap& bmp)
{
    return dynamic_cast<glArchivItem_Bitmap*>(bmp.clone());
}

TerrainRenderer::TerrainRenderer() : size_(0, 0) {}
TerrainRenderer::~TerrainRenderer() = default;

static constexpr unsigned getFlatIndex(DescIdx<LandscapeDesc> ls, LandRoadType road)
{
    return ls.value * helpers::NumEnumValues_v<LandRoadType> + rttr::enum_cast(road);
}

PointF TerrainRenderer::GetNeighbourVertexPos(MapPoint pt, const Direction dir) const
{
    // Note: We want the real neighbour point which might be outside of the map to get the offset right
    Position ptNb = ::GetNeighbour(Position(pt), dir);

    Position offset;
    MapPoint t = ConvertCoords(ptNb, &offset);

    return GetVertexPos(t) + PointF(offset);
}

PointF TerrainRenderer::GetNeighbourBorderPos(const MapPoint pt, const unsigned char triangle,
                                              const Direction dir) const
{
    // Note: We want the real neighbour point which might be outside of the map to get the offset right
    Position ptNb = ::GetNeighbour(Position(pt), dir);

    Position offset;
    MapPoint t = ConvertCoords(ptNb, &offset);

    return GetBorderPos(t, triangle) + PointF(offset);
}

void TerrainRenderer::LoadTextures(const WorldDescription& desc)
{
    terrainTextures.clear();
    edgeTextures.clear();
    roadTextures.clear();

    std::set<DescIdx<LandscapeDesc>> usedLandscapes;
    std::set<DescIdx<TerrainDesc>> usedTerrains;
    std::set<DescIdx<EdgeDesc>> usedEdges;
    using TerrainPair = std::array<DescIdx<TerrainDesc>, 2>;
    for(const TerrainPair& ts : terrain)
    {
        for(const DescIdx<TerrainDesc>& tIdx : ts)
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
    roadTextures.resize((usedLandscapes.rbegin()->value + 1) * helpers::NumEnumValues_v<LandRoadType>);

    for(DescIdx<TerrainDesc> curIdx : usedTerrains)
    {
        const TerrainDesc& cur = desc.get(curIdx);
        const auto textureName = ResourceId::make(bfs::path(cur.texturePath));
        glArchivItem_Bitmap* texBmp = LOADER.GetImageN(textureName, 0);
        if(!texBmp)
            throw std::runtime_error("Invalid texture '" + cur.texturePath + "' for terrain '" + cur.name + "'");
        if(cur.palAnimIdx >= 0)
        {
            libsiedler2::ArchivItem* animItem = LOADER.GetArchive(textureName)[cur.palAnimIdx];
            if(!texBmp->getPalette() || !animItem || animItem->getBobType() != libsiedler2::BobType::PaletteAnim)
            {
                LOG.write("Invalid palette animation '%1%' for '%2%'") % unsigned(cur.palAnimIdx) % cur.name;
                terrainTextures[curIdx].textures.push_back(LOADER.ExtractTexture(*texBmp, cur.posInTexture));
            } else
            {
                auto& anim = static_cast<libsiedler2::ArchivItem_PaletteAnimation&>(*animItem);
                std::unique_ptr<libsiedler2::Archiv> textures = LOADER.ExtractAnimatedTexture(
                  *texBmp, cur.posInTexture, anim.firstClr, anim.lastClr - anim.firstClr + 1);
                for(unsigned i = 0; i < textures->size(); i++)
                {
                    terrainTextures[curIdx].textures.push_back(
                      boost::dynamic_pointer_cast<glArchivItem_Bitmap>(textures->release(i)));
                }
            }
        } else
        {
            terrainTextures[curIdx].textures.push_back(LOADER.ExtractTexture(*texBmp, cur.posInTexture));
        }
        // Initialize OpenGL textures
        for(glArchivItem_Bitmap& bmp : terrainTextures[curIdx].textures)
            bmp.GetTexture();
        TerrainDesc::Triangle trianglePos = cur.GetRSUTriangle();
        Position texOrigin = cur.posInTexture.getOrigin();
        const glArchivItem_Bitmap& texture = terrainTextures[curIdx].textures[0];
        Triangle& rsuCoord = terrainTextures[curIdx].rsuCoords;
        PointF texSize(texture.GetTexSize());
        rsuCoord[0] = PointF(trianglePos.tip - texOrigin);
        rsuCoord[1] = PointF(trianglePos.left - texOrigin);
        rsuCoord[2] = PointF(trianglePos.right - texOrigin);
        // Normalize to texture size
        for(unsigned i = 0; i < 3; i++)
            rsuCoord[i] /= texSize;

        Triangle& usdCoord = terrainTextures[curIdx].usdCoords;
        trianglePos = cur.GetUSDTriangle();
        usdCoord[0] = PointF(trianglePos.left - texOrigin);
        usdCoord[1] = PointF(trianglePos.tip - texOrigin);
        usdCoord[2] = PointF(trianglePos.right - texOrigin);
        // Normalize to texture size
        for(unsigned i = 0; i < 3; i++)
            usdCoord[i] /= texSize;
    }
    for(DescIdx<EdgeDesc> curIdx : usedEdges)
    {
        const EdgeDesc& cur = desc.get(curIdx);
        const auto textureName = ResourceId::make(bfs::path(cur.texturePath));
        glArchivItem_Bitmap* texBmp = LOADER.GetImageN(textureName, 0);
        if(!texBmp)
            throw std::runtime_error("Invalid texture '" + cur.texturePath + "' for edge '" + cur.name + "'");
        edgeTextures[curIdx] = LOADER.ExtractTexture(*texBmp, cur.posInTexture);
        edgeTextures[curIdx]->GetTexture(); // Init texture
    }
    for(DescIdx<LandscapeDesc> curIdx : usedLandscapes)
    {
        const LandscapeDesc& cur = desc.get(curIdx);
        for(const auto i : helpers::enumRange<LandRoadType>())
        {
            const auto textureName = ResourceId::make(bfs::path(cur.roadTexDesc[i].texturePath));
            glArchivItem_Bitmap* texBmp = LOADER.GetImageN(textureName, 0);
            if(!texBmp)
                throw std::runtime_error("Invalid texture '" + cur.roadTexDesc[i].texturePath
                                         + "' for road in landscape '" + cur.name + "'");
            roadTextures[getFlatIndex(curIdx, i)] = LOADER.ExtractTexture(*texBmp, cur.roadTexDesc[i].posInTexture);
            roadTextures[getFlatIndex(curIdx, i)]->GetTexture(); // Init texture
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
    GetVertex(pt).pos = PointF(gwv.GetWorld().GetNodePos(pt));
}

void TerrainRenderer::UpdateVertexColor(const MapPoint pt, const GameWorldViewer& gwv)
{
    auto shadow = static_cast<float>(gwv.GetNode(pt).shadow);
    float clr = -1.f / (256.f * 256.f) * shadow * shadow + 1.f / 90.f * shadow + 0.38f;
    switch(gwv.GetVisibility(pt))
    {
        case Visibility::Invisible:
            // Unsichtbar -> schwarz
            GetVertex(pt).color = 0.0f;
            break;
        case Visibility::FogOfWar:
            // Fog of War -> abgedunkelt
            GetVertex(pt).color = clr / (TEXTURE_COLOR_DIVISOR * 2);
            break;
        case Visibility::Visible:
            // Normal sichtbar
            GetVertex(pt).color = clr / TEXTURE_COLOR_DIVISOR;
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
    vertex.borderPos[0] = (GetNeighbourVertexPos(pt, Direction::SouthWest) + GetVertexPos(pt)
                           + GetNeighbourVertexPos(pt, Direction::SouthEast))
                          / 3.0f;
    vertex.borderColor[0] = (GetColor(GetNeighbour(pt, Direction::SouthWest)) + GetColor(pt)
                             + GetColor(GetNeighbour(pt, Direction::SouthEast)))
                            / 3.0f;

    vertex.borderPos[1] =
      (GetNeighbourVertexPos(pt, Direction::East) + GetVertexPos(pt) + GetNeighbourVertexPos(pt, Direction::SouthEast))
      / 3.0f;
    vertex.borderColor[1] =
      (GetColor(GetNeighbour(pt, Direction::East)) + GetColor(pt) + GetColor(GetNeighbour(pt, Direction::SouthEast)))
      / 3.0f;
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

/// Get the edge type description index that t1 draws over t2. If none a falsy index is returned
static DescIdx<EdgeDesc> GetEdgeType(const TerrainDesc& t1, const TerrainDesc& t2)
{
    if(!t1.edgeType || t1.edgePriority <= t2.edgePriority)
        return {};
    else
        return t1.edgeType;
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
        const TerrainDesc& t3 = desc.get(terrain[GetVertexIdx(GetNeighbour(pt, Direction::East))][0]);
        const TerrainDesc& t4 = desc.get(terrain[GetVertexIdx(GetNeighbour(pt, Direction::SouthWest))][1]);

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

    if(SETTINGS.video.vbo)
    {
        // Create and fill the 3 VBOs for vertices, texCoords and colors
        vbo_vertices = ogl::VBO<Triangle>(ogl::Target::Array);
        vbo_vertices.fill(gl_vertices, ogl::Usage::Static);

        vbo_texcoords = ogl::VBO<Triangle>(ogl::Target::Array);
        vbo_texcoords.fill(gl_texcoords, ogl::Usage::Static);

        vbo_colors = ogl::VBO<ColorTriangle>(ogl::Target::Array);
        vbo_colors.fill(gl_colors, ogl::Usage::Static);

        // Unbind VBO to not interfere with other program parts
        vbo_colors.unbind();
    }
}

void TerrainRenderer::UpdateTrianglePos(const MapPoint pt, bool updateVBO)
{
    unsigned pos = GetTriangleIdx(pt);

    gl_vertices[pos][0] = GetVertexPos(pt);
    gl_vertices[pos][1] = GetNeighbourVertexPos(pt, Direction::SouthWest);
    gl_vertices[pos][2] = GetNeighbourVertexPos(pt, Direction::SouthEast);

    ++pos;

    gl_vertices[pos][0] = GetVertexPos(pt);
    gl_vertices[pos][1] = GetNeighbourVertexPos(pt, Direction::SouthEast);
    gl_vertices[pos][2] = GetNeighbourVertexPos(pt, Direction::East);

    if(updateVBO && vbo_vertices.isValid())
    {
        vbo_vertices.update(&gl_vertices[pos - 1], 2, pos - 1);
        vbo_vertices.unbind();
    }
}

void TerrainRenderer::UpdateTriangleColor(const MapPoint pt, bool updateVBO)
{
    unsigned pos = GetTriangleIdx(pt);

    Color& clr0 = gl_colors[pos][0];
    Color& clr1 = gl_colors[pos][1];
    Color& clr2 = gl_colors[pos][2];
    clr0.r = clr0.g = clr0.b = GetColor(pt);
    clr1.r = clr1.g = clr1.b = GetColor(GetNeighbour(pt, Direction::SouthWest));
    clr2.r = clr2.g = clr2.b = GetColor(GetNeighbour(pt, Direction::SouthEast));

    ++pos;

    Color& clr3 = gl_colors[pos][0];
    Color& clr4 = gl_colors[pos][1];
    Color& clr5 = gl_colors[pos][2];
    clr3.r = clr3.g = clr3.b = GetColor(pt);
    clr4.r = clr4.g = clr4.b = GetColor(GetNeighbour(pt, Direction::SouthEast));
    clr5.r = clr5.g = clr5.b = GetColor(GetNeighbour(pt, Direction::East));

    if(updateVBO && vbo_colors.isValid())
    {
        vbo_colors.update(&gl_colors[pos - 1], 2, pos - 1);
        vbo_colors.unbind();
    }
}

void TerrainRenderer::UpdateTriangleTerrain(const MapPoint pt, bool updateVBO)
{
    const unsigned nodeIdx = GetVertexIdx(pt);
    const DescIdx<TerrainDesc> t1 = terrain[nodeIdx][0];
    const DescIdx<TerrainDesc> t2 = terrain[nodeIdx][1];

    const unsigned triangleIdx = GetTriangleIdx(pt);
    gl_texcoords[triangleIdx] = terrainTextures[t1].rsuCoords;
    gl_texcoords[triangleIdx + 1] = terrainTextures[t2].usdCoords;

    if(updateVBO && vbo_texcoords.isValid())
    {
        vbo_texcoords.update(&gl_texcoords[triangleIdx - 1], 2, triangleIdx - 1);
        vbo_texcoords.unbind();
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
        gl_vertices[offset][1] = GetNeighbourVertexPos(pt, Direction::SouthEast);
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

        gl_vertices[offset][i ? 2 : 0] = GetNeighbourVertexPos(pt, Direction::SouthEast);
        gl_vertices[offset][1] = GetNeighbourVertexPos(pt, Direction::East);

        if(i == 0)
            gl_vertices[offset][2] = GetBorderPos(pt, 1);
        else
            gl_vertices[offset][0] = GetNeighbourBorderPos(pt, 0, Direction::East);

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

        gl_vertices[offset][i ? 2 : 0] = GetNeighbourVertexPos(pt, Direction::SouthWest);
        gl_vertices[offset][1] = GetNeighbourVertexPos(pt, Direction::SouthEast);

        if(i == 0)
            gl_vertices[offset][2] = GetBorderPos(pt, i);
        else
            gl_vertices[offset][0] = GetNeighbourBorderPos(pt, i, Direction::SouthWest);

        ++count_borders;
    }

    if(updateVBO && vbo_vertices.isValid())
    {
        vbo_vertices.update(&gl_vertices[first_offset], count_borders, first_offset);
        vbo_vertices.unbind();
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

        gl_colors[offset][i ? 0 : 2].r = gl_colors[offset][i ? 0 : 2].g = gl_colors[offset][i ? 0 : 2].b =
          GetColor(pt); //-V807
        gl_colors[offset][1].r = gl_colors[offset][1].g = gl_colors[offset][1].b =
          GetColor(GetNeighbour(pt, Direction::SouthEast)); //-V807
        gl_colors[offset][i ? 2 : 0].r = gl_colors[offset][i ? 2 : 0].g = gl_colors[offset][i ? 2 : 0].b =
          GetBorderColor(pt, i); //-V807

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
          GetColor(GetNeighbour(pt, Direction::SouthEast));
        gl_colors[offset][1].r = gl_colors[offset][1].g = gl_colors[offset][1].b =
          GetColor(GetNeighbour(pt, Direction::East));
        MapPoint pt2(pt.x + i, pt.y);
        if(pt2.x >= size_.x)
            pt2.x -= size_.x;
        gl_colors[offset][i ? 0 : 2].r = gl_colors[offset][i ? 0 : 2].g = gl_colors[offset][i ? 0 : 2].b =
          GetBorderColor(pt2, i ? 0 : 1);

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
          GetColor(GetNeighbour(pt, Direction::SouthWest));
        gl_colors[offset][1].r = gl_colors[offset][1].g = gl_colors[offset][1].b =
          GetColor(GetNeighbour(pt, Direction::SouthEast));

        if(i == 0)
            gl_colors[offset][2].r = gl_colors[offset][2].g = gl_colors[offset][2].b = GetBorderColor(pt, i); //-V807
        else
            gl_colors[offset][0].r = gl_colors[offset][0].g = gl_colors[offset][0].b =
              GetBorderColor(GetNeighbour(pt, Direction::SouthWest), i); //-V807

        ++count_borders;
    }

    if(updateVBO && vbo_colors.isValid())
    {
        vbo_colors.update(&gl_colors[first_offset], count_borders, first_offset);
        vbo_colors.unbind();
    }
}

void TerrainRenderer::UpdateBorderTriangleTerrain(const MapPoint pt, bool updateVBO)
{
    unsigned pos = GetVertexIdx(pt);

    // For updating the VBO count created borders and first offset
    unsigned count_borders = 0;
    unsigned first_offset = 0;

    // left to right border
    for(unsigned char i = 0; i < 2; ++i)
    {
        if(borders[pos].left_right[i])
        {
            unsigned offset = borders[pos].left_right_offset[i];

            if(!first_offset)
                first_offset = offset;

            const glArchivItem_Bitmap& texture = *edgeTextures[borders[pos].left_right[i]];
            Extent bmpSize = texture.GetSize();
            PointF texSize(texture.GetTexSize());

            gl_texcoords[offset][i ? 0 : 2] = PointF(0.0f, 0.0f);
            gl_texcoords[offset][1] = PointF(bmpSize.x / texSize.x, 0.0f);
            gl_texcoords[offset][i ? 2 : 0] = PointF(bmpSize.x / texSize.x / 2.f, bmpSize.y / texSize.y);

            ++count_borders;
        }
    }

    // right to left border
    for(unsigned char i = 0; i < 2; ++i)
    {
        if(borders[pos].right_left[i])
        {
            unsigned offset = borders[pos].right_left_offset[i];

            if(!first_offset)
                first_offset = offset;

            const glArchivItem_Bitmap& texture = *edgeTextures[borders[pos].right_left[i]];
            Extent bmpSize = texture.GetSize();
            PointF texSize(texture.GetTexSize());

            gl_texcoords[offset][i ? 2 : 0] = PointF(0.0f, 0.0f);
            gl_texcoords[offset][1] = PointF(bmpSize.x / texSize.x, 0.0f);
            gl_texcoords[offset][i ? 0 : 2] = PointF(bmpSize.x / texSize.x / 2.f, bmpSize.y / texSize.y);

            ++count_borders;
        }
    }

    // top to bottom border
    for(unsigned char i = 0; i < 2; ++i)
    {
        if(borders[pos].top_down[i])
        {
            unsigned offset = borders[pos].top_down_offset[i];

            if(!first_offset)
                first_offset = offset;

            const glArchivItem_Bitmap& texture = *edgeTextures[borders[pos].top_down[i]];
            Extent bmpSize = texture.GetSize();
            PointF texSize(texture.GetTexSize());

            gl_texcoords[offset][i ? 2 : 0] = PointF(0.0f, 0.0f);
            gl_texcoords[offset][1] = PointF(bmpSize.x / texSize.x, 0.0f);
            gl_texcoords[offset][i ? 0 : 2] = PointF(bmpSize.x / texSize.x / 2.f, bmpSize.y / texSize.y);

            ++count_borders;
        }
    }

    if(updateVBO && vbo_texcoords.isValid())
    {
        vbo_texcoords.update(&gl_texcoords[first_offset], count_borders, first_offset);
        vbo_texcoords.unbind();
    }
}

void TerrainRenderer::Draw(const Position& firstPt, const Position& lastPt, const GameWorldViewer& gwv,
                           unsigned* water) const
{
    RTTR_Assert(!gl_vertices.empty());
    RTTR_Assert(!borders.empty());

    // Sort textures into lists by type
    DescriptionVector<std::vector<MapTile>, TerrainDesc> sorted_textures(terrainTextures.size());
    DescriptionVector<std::vector<BorderTile>, EdgeDesc> sorted_borders(edgeTextures.size());
    PreparedRoads sorted_roads(roadTextures.size());

    Position lastOffset(0, 0);

    // Only draw visible area
    for(auto const y : helpers::range(firstPt.y, lastPt.y + 1))
    {
        DescIdx<TerrainDesc> lastTerrain;
        DescIdx<EdgeDesc> lastBorder;

        for(auto const x : helpers::range(firstPt.x, lastPt.x + 1))
        {
            Position posOffset;
            MapPoint tP = ConvertCoords(Position(x, y), &posOffset);

            auto t = terrain[GetVertexIdx(tP)][0];
            if(posOffset != lastOffset)
                lastTerrain = DescIdx<TerrainDesc>();

            if(t == lastTerrain && tP != MapPoint(0, 0))
                ++sorted_textures[t].back().count;
            else
            {
                MapTile tmp(GetTriangleIdx(tP), posOffset);
                sorted_textures[t].push_back(tmp);
                lastTerrain = t;
            }

            t = terrain[GetVertexIdx(tP)][1];

            if(t == lastTerrain)
                ++sorted_textures[t].back().count;
            else
            {
                MapTile tmp(GetTriangleIdx(tP) + 1, posOffset);
                sorted_textures[t].push_back(tmp);
            }

            lastTerrain = t;

            const Borders& curBorders = borders[GetVertexIdx(tP)];
            helpers::EnumArray<DescIdx<EdgeDesc>, Direction> tiles = {
              {curBorders.left_right[0], curBorders.left_right[1], curBorders.right_left[0], curBorders.right_left[1],
               curBorders.top_down[0], curBorders.top_down[1]}};

            // Offsets into gl_* arrays
            helpers::EnumArray<unsigned, Direction> offsets = {
              {curBorders.left_right_offset[0], curBorders.left_right_offset[1], curBorders.right_left_offset[0],
               curBorders.right_left_offset[1], curBorders.top_down_offset[0], curBorders.top_down_offset[1]}};

            for(const auto dir : helpers::EnumRange<Direction>{})
            {
                if(!tiles[dir])
                    continue;
                if(tiles[dir] == lastBorder)
                {
                    BorderTile& curTile = sorted_borders[lastBorder].back();
                    // Check that we did not wrap around the map and the expected offset matches
                    if(curTile.tileOffset + curTile.count == offsets[dir])
                    {
                        ++curTile.count;
                        continue;
                    }
                }
                lastBorder = tiles[dir];
                BorderTile tmp(offsets[dir], posOffset);
                sorted_borders[lastBorder].push_back(tmp);
            }

            PrepareWaysPoint(sorted_roads, gwv, tP, posOffset);

            lastOffset = posOffset;
        }
    }

    if(water)
    {
        const WorldDescription& desc = gwv.GetWorld().GetDescription();
        unsigned water_count = 0;
        for(const auto t : sorted_textures.indices())
        {
            if(desc.get(t).kind == TerrainKind::Water)
            {
                for(const MapTile& tile : sorted_textures[t])
                    water_count += tile.count;
            }
        }

        Position diff =
          lastPt - firstPt + Position(1, 1); // Number of points checked in X and Y, including(!) the last one
        // For each point there are 2 tiles added (USD, RSU) so we have 2 times the tiles as the number of points.
        // Calculate the percentage of water tiles
        *water = 100 * water_count / (2 * prodOfComponents(diff));
    }

    lastOffset = Position(0, 0);

    glEnableClientState(GL_COLOR_ARRAY);

    if(vbo_vertices.isValid())
    {
        vbo_vertices.bind();
        glVertexPointer(2, GL_FLOAT, 0, nullptr);

        vbo_texcoords.bind();
        glTexCoordPointer(2, GL_FLOAT, 0, nullptr);

        vbo_colors.bind();
        glColorPointer(3, GL_FLOAT, 0, nullptr);
    } else
    {
        glVertexPointer(2, GL_FLOAT, 0, &gl_vertices.front());
        glTexCoordPointer(2, GL_FLOAT, 0, &gl_texcoords.front());
        glColorPointer(3, GL_FLOAT, 0, &gl_colors.front());
    }

#if RTTR_OGL_GL4ES
    // Gl4ES behaves weird with GL_COMBINE. All textures are too bright and some shadows are missing. The function might
    // not be implemented in GL4ES at all.
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
#else
    // Modulate2x
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
    glTexEnvf(GL_TEXTURE_ENV, GL_RGB_SCALE, TEXTURE_COLOR_DIVISOR);
#endif

    // Disable alpha blending
    glDisable(GL_BLEND);

    glPushMatrix();
    for(const auto t : sorted_textures.indices())
    {
        if(sorted_textures[t].empty())
            continue;
        unsigned animationFrame;
        const unsigned numFrames = terrainTextures[t].textures.size();
        if(numFrames > 1)
            animationFrame = GAMECLIENT.GetGlobalAnimation(numFrames, 5 * numFrames, 16, 0); // We have 5/16 per frame
        else
            animationFrame = 0;

        VIDEODRIVER.BindTexture(terrainTextures[t].textures[animationFrame].GetTextureNoCreate());

        for(const auto& texture : sorted_textures[t])
        {
            if(texture.posOffset != lastOffset)
            {
                Position trans = texture.posOffset - lastOffset;
                glTranslatef(float(trans.x), float(trans.y), 0.0f);
                lastOffset = texture.posOffset;
            }

            RTTR_Assert(texture.tileOffset + texture.count <= size_.x * size_.y * 2u);
            glDrawArrays(GL_TRIANGLES, texture.tileOffset * 3,
                         texture.count * 3); // Arguments are in Elements. 1 triangle has 3 values
        }
    }
    glPopMatrix();

    glEnable(GL_BLEND);

    lastOffset = Position(0, 0);
    glPushMatrix();
    for(const auto i : sorted_borders.indices())
    {
        if(sorted_borders[i].empty())
            continue;
        VIDEODRIVER.BindTexture(edgeTextures[i]->GetTextureNoCreate());

        for(const auto& texture : sorted_borders[i])
        {
            if(texture.posOffset != lastOffset)
            {
                Position trans = texture.posOffset - lastOffset;
                glTranslatef(float(trans.x), float(trans.y), 0.0f);
                lastOffset = texture.posOffset;
            }
            RTTR_Assert(texture.tileOffset + texture.count <= gl_vertices.size());
            glDrawArrays(GL_TRIANGLES, texture.tileOffset * 3,
                         texture.count * 3); // Arguments are in elements. 1 triangle has 3 values
        }
    }
    glPopMatrix();

    // unbind VBO
    if(vbo_vertices.isValid())
        vbo_vertices.unbind();

    DrawWays(sorted_roads);

    glDisableClientState(GL_COLOR_ARRAY);
    // Switch back to modulation
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
}

MapPoint TerrainRenderer::ConvertCoords(const Position pt, Position* offset) const
{
    MapPoint ptOut = MakeMapPoint(pt, size_);
    RTTR_Assert(ptOut.x < size_.x);
    RTTR_Assert(ptOut.y < size_.y);
    if(offset)
    {
        // We need the offset by which we shifted the point in screen units
        *offset = (pt - ptOut) * Extent(TR_W, TR_H);
    }
    return ptOut;
}

void TerrainRenderer::PrepareWaysPoint(PreparedRoads& sorted_roads, const GameWorldViewer& gwViewer, MapPoint pt,
                                       const Position& offset) const
{
    const WorldDescription& desc = gwViewer.GetWorld().GetDescription();
    Position startPos = Position(Position::Truncate, GetVertexPos(pt)) + offset;

    Visibility visibility = gwViewer.GetVisibility(pt);

    int totalWidth = size_.x * TR_W;
    int totalHeight = size_.y * TR_H;

    // Wegtypen für die drei Richtungen
    for(const RoadDir dir : helpers::EnumRange<RoadDir>{})
    {
        const PointRoad type = gwViewer.GetVisibleRoad(pt, dir, visibility);
        if(type == PointRoad::None)
            continue;
        const Direction targetDir = toDirection(dir);
        MapPoint ta = gwViewer.GetNeighbour(pt, targetDir);

        Position endPos = Position(Position::Truncate, GetVertexPos(ta)) + offset;
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

        // The gfx road type is:
        // Boat for boat roads
        // else Mountain left or right is a mountain terrain
        // else Upgraded for Donkey roads
        // else Normal
        uint8_t gfxRoadType;
        const auto terrain = gwViewer.GetWorld().GetTerrain(pt, targetDir);
        const TerrainDesc& lTerrain = desc.get(terrain.left);
        if(type == PointRoad::Boat)
        {
            gfxRoadType = getFlatIndex(lTerrain.landscape, LandRoadType::Boat);
        } else
        {
            if(desc.get(terrain.left).kind == TerrainKind::Mountain)
                gfxRoadType = getFlatIndex(lTerrain.landscape, LandRoadType::Mountain);
            else
            {
                const TerrainDesc& rTerrain = desc.get(terrain.right);
                if(rTerrain.kind == TerrainKind::Mountain)
                    gfxRoadType = getFlatIndex(rTerrain.landscape, LandRoadType::Mountain);
                else
                    gfxRoadType =
                      getFlatIndex(lTerrain.landscape,
                                   ((type == PointRoad::Donkey) ? LandRoadType::Upgraded : LandRoadType::Normal));
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
    static constexpr helpers::EnumArray<std::array<Position, 4>, RoadDir> begin_end_coords = {{
      {{Position(0, -3), Position(0, 3), Position(3, 3), Position(3, -3)}},
      {{Position(2, -4), Position(-4, 2), Position(0, 6), Position(6, 0)}},
      {{Position(4, 2), Position(-2, -4), Position(-6, 0), Position(0, 6)}},
    }};

    size_t maxSize = 0;
    for(const std::vector<PreparedRoad>& sorted_road : sorted_roads)
        maxSize = std::max(maxSize, sorted_road.size());

    if(maxSize == 0)
        return;

    auto vertexData = std::unique_ptr<Tex2C3Ver2[]>(new Tex2C3Ver2[maxSize * 4]);
    // These should still be enabled
    RTTR_Assert(glIsEnabled(GL_VERTEX_ARRAY));
    RTTR_Assert(glIsEnabled(GL_TEXTURE_COORD_ARRAY));
    RTTR_Assert(glIsEnabled(GL_COLOR_ARRAY));
    glVertexPointer(2, GL_FLOAT, sizeof(Tex2C3Ver2), &vertexData[0].x);
    glTexCoordPointer(2, GL_FLOAT, sizeof(Tex2C3Ver2), &vertexData[0].tx);
    glColorPointer(3, GL_FLOAT, sizeof(Tex2C3Ver2), &vertexData[0].r);

    for(const auto itRoad : sorted_roads | boost::adaptors::indexed())
    {
        if(itRoad.value().empty())
            continue;
        Tex2C3Ver2* curVertexData = vertexData.get();
        const glArchivItem_Bitmap& texture = *roadTextures[itRoad.index()];
        PointF scaledTexSize = texture.GetSize() / PointF(texture.GetTexSize());

        for(const auto& it : itRoad.value())
        {
            curVertexData->tx = 0.0f;
            curVertexData->ty = 0.0f;
            curVertexData->r = curVertexData->g = curVertexData->b = it.color1;
            Position tmpP = it.pos + begin_end_coords[it.dir][0];
            curVertexData->x = GLfloat(tmpP.x);
            curVertexData->y = GLfloat(tmpP.y);

            curVertexData++;

            curVertexData->tx = 0.0f;
            curVertexData->ty = scaledTexSize.y;
            curVertexData->r = curVertexData->g = curVertexData->b = it.color1;
            tmpP = it.pos + begin_end_coords[it.dir][1];
            curVertexData->x = GLfloat(tmpP.x);
            curVertexData->y = GLfloat(tmpP.y);

            curVertexData++;

            curVertexData->tx = scaledTexSize.x;
            curVertexData->ty = scaledTexSize.y;
            curVertexData->r = curVertexData->g = curVertexData->b = it.color2;
            tmpP = it.pos2 + begin_end_coords[it.dir][2];
            curVertexData->x = GLfloat(tmpP.x);
            curVertexData->y = GLfloat(tmpP.y);

            curVertexData++;
            curVertexData->tx = scaledTexSize.x;
            curVertexData->ty = 0.0f;
            curVertexData->r = curVertexData->g = curVertexData->b = it.color2;
            tmpP = it.pos2 + begin_end_coords[it.dir][3];
            curVertexData->x = GLfloat(tmpP.x);
            curVertexData->y = GLfloat(tmpP.y);

            curVertexData++;
        }

        VIDEODRIVER.BindTexture(texture.GetTextureNoCreate());
        glDrawArrays(GL_QUADS, 0, itRoad.value().size() * 4);
    }
    // Note: No glDisableClientState as we did not enable it
}

void TerrainRenderer::AltitudeChanged(const MapPoint pt, const GameWorldViewer& gwv)
{
    // den selbst sowieso die Punkte darum updaten, da sich bei letzteren die Schattierung geändert haben könnte
    UpdateVertexPos(pt, gwv);
    UpdateVertexColor(pt, gwv);

    for(const MapPoint nb : gwv.GetNeighbours(pt))
        UpdateVertexColor(nb, gwv);

    // und für die Ränder
    UpdateBorderVertex(pt);

    for(const MapPoint nb : gwv.GetNeighbours(pt))
        UpdateBorderVertex(nb);

    // den selbst sowieso die Punkte darum updaten, da sich bei letzteren die Schattierung geändert haben könnte
    UpdateTrianglePos(pt, true);
    UpdateTriangleColor(pt, true);

    for(const MapPoint nb : gwv.GetNeighbours(pt))
    {
        UpdateTrianglePos(nb, true);
        UpdateTriangleColor(nb, true);
    }

    // Auch im zweiten Kreis drumherum die Dreiecke neu berechnen, da die durch die Schattenänderung der umliegenden
    // Punkte auch geändert werden könnten
    for(unsigned i = 0; i < 12; ++i)
        UpdateTriangleColor(gwv.GetWorld().GetNeighbour2(pt, i), true);

    // und für die Ränder
    UpdateBorderTrianglePos(pt, true);
    UpdateBorderTriangleColor(pt, true);

    for(const MapPoint nb : gwv.GetNeighbours(pt))
    {
        UpdateBorderTrianglePos(nb, true);
        UpdateBorderTriangleColor(nb, true);
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
    for(const MapPoint nb : gwv.GetNeighbours(pt))
        UpdateVertexColor(nb, gwv);

    // und für die Ränder
    UpdateBorderVertex(pt);
    for(const MapPoint nb : gwv.GetNeighbours(pt))
        UpdateBorderVertex(nb);

    // den selbst sowieso die Punkte darum updaten, da sich bei letzteren die Schattierung geändert haben könnte
    UpdateTriangleColor(pt, true);
    for(const MapPoint nb : gwv.GetNeighbours(pt))
        UpdateTriangleColor(nb, true);

    // und für die Ränder
    UpdateBorderTriangleColor(pt, true);
    for(const MapPoint nb : gwv.GetNeighbours(pt))
        UpdateBorderTriangleColor(nb, true);
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

    if(vbo_colors.isValid())
    {
        vbo_colors.update(gl_colors);
        vbo_colors.unbind();
    }
}

MapPoint TerrainRenderer::GetNeighbour(const MapPoint& pt, const Direction dir) const
{
    return MakeMapPoint(::GetNeighbour(Position(pt), dir), size_);
}
