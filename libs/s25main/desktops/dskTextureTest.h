// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "desktops/dskMenuBase.h"
#include "gameData/WorldDescription.h"
#include <memory>

class dskTextureTest : public dskMenuBase
{
public:
    dskTextureTest();
    ~dskTextureTest();

    void Load();

    void Msg_ComboSelectItem(unsigned ctrl_id, unsigned selection) override;
    void Msg_ButtonClick(unsigned ctrl_id) override;
    void Msg_PaintAfter() override;
    bool Msg_KeyDown(const KeyEvent& ke) override;

private:
    WorldDescription desc;
    std::unique_ptr<glArchivItem_Bitmap> curTexture;
    DescIdx<TerrainDesc> curTerrainIdx;
};
