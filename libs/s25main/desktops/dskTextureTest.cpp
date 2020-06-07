// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
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
#include "s25util/strAlgos.h"
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
    AddText(ID_txtTitle, DrawPoint(300, 20), "Test screen for textures", COLOR_ORANGE, FontStyle::CENTER, LargeFont);
    AddComboBox(ID_cbTexture, DrawPoint(630, 50), Extent(100, 22), TC_GREEN2, NormalFont, 100);
    Load();
    AddTextButton(ID_btBack, DrawPoint(630, 565), Extent(150, 22), TC_RED1, _("Back"), NormalFont);
}

dskTextureTest::~dskTextureTest() = default;

void dskTextureTest::Load()
{
    WorldDescription newDesc;
    GameDataLoader gdLoader(newDesc);
    if(!gdLoader.Load())
    {
        WINDOWMANAGER.ShowAfterSwitch(
          std::make_unique<iwMsgbox>(_("Error"), "Failed to load game data!", nullptr, MSB_OK, MSB_EXCLAMATIONRED));
        return;
    }
    desc = newDesc;

    auto* cb = GetCtrl<ctrlComboBox>(ID_cbTexture);
    int selection = cb->GetSelection();
    if(selection < 0)
        selection = 0;
    cb->DeleteAllItems();
    LOADER.ClearOverrideFolders();
    LOADER.AddOverrideFolder(s25::folders::gameLstsGlobal);
    for(DescIdx<TerrainDesc> i(0); i.value < desc.terrain.size(); i.value++)
        cb->AddString(desc.get(i).name);
    cb->SetSelection(selection);
    Msg_ComboSelectItem(ID_cbTexture, selection);
}

void dskTextureTest::Msg_ComboSelectItem(const unsigned ctrl_id, const int selection)
{
    curTerrainIdx = desc.terrain.getIndex(GetCtrl<ctrlComboBox>(ctrl_id)->GetText(selection));
    if(!curTerrainIdx)
        return;
    const TerrainDesc& cur = desc.get(curTerrainIdx);
    LOADER.Load(RTTRCONFIG.ExpandPath(cur.texturePath));
    std::string textureName = s25util::toLower(bfs::path(cur.texturePath).stem().string());
    glArchivItem_Bitmap* texBmp = LOADER.GetImageN(textureName, 0);
    curTexture = LOADER.ExtractTexture(*texBmp, cur.posInTexture);
}

void dskTextureTest::Msg_ButtonClick(const unsigned /*ctrl_id*/)
{
    WINDOWMANAGER.Switch(std::make_unique<dskMainMenu>());
}

using PointF = Point<GLfloat>;
using Triangle = std::array<PointF, 3>;

void dskTextureTest::Msg_PaintAfter()
{
    dskMenuBase::Msg_PaintAfter();
    if(!curTexture)
        return;
    DrawRectangle(Rect(0, 50, 800, 550), COLOR_BLACK);
    VIDEODRIVER.BindTexture(curTexture->GetTexture());
    const TerrainDesc& cur = desc.get(curTerrainIdx);
    Position texOrigin = cur.posInTexture.getOrigin();
    PointF texSize(curTexture->GetTexSize());

    TerrainDesc::Triangle trianglePos = cur.GetRSUTriangle();
    std::array<Triangle, 2> texCoords;

    Triangle& rsuCoord = texCoords[0];
    rsuCoord[0] = PointF(trianglePos.tip - texOrigin);
    rsuCoord[1] = PointF(trianglePos.left - texOrigin);
    rsuCoord[2] = PointF(trianglePos.right - texOrigin);
    // Normalize to texture size
    for(unsigned i = 0; i < 3; i++)
        rsuCoord[i] /= texSize;

    Triangle& usdCoord = texCoords[1];
    trianglePos = cur.GetUSDTriangle();
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

bool dskTextureTest::Msg_KeyDown(const KeyEvent& ke)
{
    if(ke.kt == KT_F5)
    {
        Load();
        return true;
    }
    return dskMenuBase::Msg_KeyDown(ke);
}
