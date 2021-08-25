// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "IngameWindow.h"

class IngameMinimap;
class GameWorldView;

/// Fenster für die Minimap
class iwMinimap final : public IngameWindow
{
    /// Fenster vergrößert?
    bool extended;

private:
    void Msg_ButtonClick(unsigned ctrl_id) override;

public:
    iwMinimap(IngameMinimap& minimap, GameWorldView& gwv);
    void Resize(const Extent& newSize) override;
};
