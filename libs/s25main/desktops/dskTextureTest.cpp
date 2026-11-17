// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "dskTextureTest.h"
#include "Loader.h"
#include "RttrConfig.h"
#include "WindowManager.h"
#include "controls/ctrlComboBox.h"
#include "drivers/VideoDriverWrapper.h"
#include "dskMainMenu.h"
#include "files.h"
#include "ingameWindows/iwMsgbox.h"
#include "lua/GameDataLoader.h"
#include "ogl/FontStyle.h"
#include "ogl/glArchivItem_Bitmap.h"
#include <glad/glad.h>
#include <boost/filesystem/path.hpp>
#include <array>

namespace {
enum
{
    ID_txtTitle = dskMenuBase::ID_FIRST_FREE,
    ID_btBack,
    ID_cbTexture
};
}

dskTextureTest::dskTextureTest()
{
    addText(ID_txtTitle, DrawPoint(300, 20), "Test screen for textures", COLOR_ORANGE, FontStyle::CENTER, LargeFont);
    addComboBox(ID_cbTexture, DrawPoint(630, 50), Extent(100, 22), TextureColor::Green2, NormalFont, 100);
    Load();
    addTextButton(ID_btBack, DrawPoint(630, 565), Extent(150, 22), TextureColor::Red1, _("Back"), NormalFont);
}

dskTextureTest::~dskTextureTest() = default;

void dskTextureTest::Load()
{
    WorldDescription newDesc;
    GameDataLoader gdLoader(newDesc);
    if(!gdLoader.Load())
    {
        WINDOWMANAGER.showAfterSwitch(std::make_unique<iwMsgbox>(_("Error"), "Failed to load game data!", nullptr,
                                                                 MsgboxButton::Ok, MsgboxIcon::ExclamationRed));
        return;
    }
    desc = newDesc;

    auto* cb = getCtrl<ctrlComboBox>(ID_cbTexture);
    const unsigned selection = cb->getSelection().value_or(0);
    cb->deleteAllItems();
    for(const auto& t : desc.terrain)
        cb->addItem(t.name);
    cb->setSelection(selection);
    msgComboSelectItem(ID_cbTexture, selection);
}

void dskTextureTest::msgComboSelectItem(const unsigned ctrl_id, const unsigned selection)
{
    curTerrainIdx = desc.terrain.getIndex(getCtrl<ctrlComboBox>(ctrl_id)->getText(selection));
    if(!curTerrainIdx)
        return;
    const TerrainDesc& cur = desc.get(curTerrainIdx);
    LOADER.Load(RTTRCONFIG.ExpandPath(cur.texturePath));
    const auto textureName = ResourceId::make(bfs::path(cur.texturePath));
    glArchivItem_Bitmap* texBmp = LOADER.getImageN(textureName, 0);
    curTexture = LOADER.extractTexture(*texBmp, cur.posInTexture);
}

void dskTextureTest::msgButtonClick(const unsigned /*ctrl_id*/)
{
    WINDOWMANAGER.switchTo(std::make_unique<dskMainMenu>());
}

using PointF = Point<GLfloat>;
using Triangle = std::array<PointF, 3>;

void dskTextureTest::msgPaintAfter()
{
    dskMenuBase::msgPaintAfter();
    if(!curTexture)
        return;
    drawRectangle(Rect(0, 50, 800, 550), COLOR_BLACK);
    VIDEODRIVER.bindTexture(curTexture->getTexture());
    const TerrainDesc& cur = desc.get(curTerrainIdx);
    Position texOrigin = cur.posInTexture.getOrigin();
    PointF texSize(curTexture->getTexSize());

    TerrainDesc::Triangle trianglePos = cur.getRSUTriangle();
    std::array<Triangle, 2> texCoords;

    Triangle& rsuCoord = texCoords[0];
    rsuCoord[0] = PointF(trianglePos.tip - texOrigin);
    rsuCoord[1] = PointF(trianglePos.left - texOrigin);
    rsuCoord[2] = PointF(trianglePos.right - texOrigin);
    // Normalize to texture size
    for(unsigned i = 0; i < 3; i++)
        rsuCoord[i] /= texSize;

    Triangle& usdCoord = texCoords[1];
    trianglePos = cur.getUSDTriangle();
    usdCoord[0] = PointF(trianglePos.left - texOrigin);
    usdCoord[1] = PointF(trianglePos.tip - texOrigin);
    usdCoord[2] = PointF(trianglePos.right - texOrigin);
    // Normalize to texture size
    for(unsigned i = 0; i < 3; i++)
        usdCoord[i] /= texSize;

    std::array<Triangle, 2> vertexCoords;
    PointF triangleSize(448, 224);
    vertexCoords[0][0] = PointF(triangleSize.x / 2 + 10, 60);
    vertexCoords[0][1] = vertexCoords[0][0] + PointF(-triangleSize.x / 2, triangleSize.y);
    vertexCoords[0][2] = vertexCoords[0][0] + PointF(triangleSize.x / 2, triangleSize.y);

    vertexCoords[1][0] = PointF(10, triangleSize.y + 80);
    vertexCoords[1][1] = vertexCoords[1][0] + PointF(triangleSize.x / 2, triangleSize.y);
    vertexCoords[1][2] = vertexCoords[1][0] + PointF(triangleSize.x, 0);

    glVertexPointer(2, GL_FLOAT, 0, &vertexCoords.front());
    glTexCoordPointer(2, GL_FLOAT, 0, &texCoords.front());
    glColor3ub(0xFF, 0xFF, 0xFF);

    glDrawArrays(GL_TRIANGLES, 0, 2 * 3);
}

bool dskTextureTest::msgKeyDown(const KeyEvent& ke)
{
    if(ke.kt == KeyType::F5)
    {
        Load();
        return true;
    }
    return dskMenuBase::msgKeyDown(ke);
}
