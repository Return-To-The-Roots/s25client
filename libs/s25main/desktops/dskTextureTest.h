// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
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

    void msgComboSelectItem(unsigned ctrl_id, unsigned selection) override;
    void msgButtonClick(unsigned ctrl_id) override;
    void msgPaintAfter() override;
    bool msgKeyDown(const KeyEvent& ke) override;

private:
    WorldDescription desc;
    std::unique_ptr<glArchivItem_Bitmap> curTexture;
    DescIdx<TerrainDesc> curTerrainIdx;
};
