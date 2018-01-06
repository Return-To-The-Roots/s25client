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

#pragma once

#ifndef dskTextureTest_h__
#define dskTextureTest_h__

#include "desktops/dskMenuBase.h"
#include "helpers/Deleter.h"
#include "gameData/WorldDescription.h"
#include <boost/interprocess/smart_ptr/unique_ptr.hpp>

class glArchivItem_Bitmap;

class dskTextureTest : public dskMenuBase
{
public:
    dskTextureTest();

    void Load();

    void Msg_ComboSelectItem(const unsigned ctrl_id, const int selection) override;
    void Msg_ButtonClick(const unsigned ctrl_id) override;
    void Msg_PaintAfter() override;
    bool Msg_KeyDown(const KeyEvent& ke) override;

private:
    WorldDescription desc;
    boost::interprocess::unique_ptr<glArchivItem_Bitmap, Deleter<glArchivItem_Bitmap> > curTexture;
    DescIdx<TerrainDesc> curTerrainIdx;
};

#endif // dskTextureTest_h__
