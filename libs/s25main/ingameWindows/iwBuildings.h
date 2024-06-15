// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "IngameWindow.h"
#include "gameTypes/BuildingType.h"
#include <list>
#include <vector>

class GameCommandFactory;
class GameWorldView;

/// Fenster, welches die Anzahl aller Gebäude und der Baustellen auflistet
class iwBuildings : public IngameWindow
{
    GameWorldView& gwv;
    GameCommandFactory& gcFactory;

public:
    iwBuildings(GameWorldView& gwv, GameCommandFactory& gcFactory);

private:
    /// Anzahlen der Gebäude zeichnen
    void Msg_PaintAfter() override;

    void Msg_ButtonClick(unsigned ctrl_id) override;
    template<class T_Window, class T_Building>
    void GoToFirstMatching(BuildingType bldType, const std::list<T_Building*>& blds);

    void setBuildingOrder();
    std::vector<BuildingType> bts;
};
