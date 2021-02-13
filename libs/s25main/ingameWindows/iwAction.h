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

#include "IngameWindow.h"
#include "gameTypes/MapCoordinates.h"
#include <boost/variant.hpp>
#include <array>

class GameInterface;
class GameWorldView;
class ctrlGroup;

class iwAction : public IngameWindow
{
public:
    using SoldierCount = unsigned;
    /// Konstanten für ActionWindow-Flag-Tab - Typen
    enum class FlagType
    {
        Normal,
        HQ,         /// von der HQ-Flagge kann nur eine Straße gebaut werden
        Storehouse, /// von einer Lagerhaus-Flagge kann nur eine Straße gebaut werden oder die Flagge abgerissen
                    /// werden
        WaterFlag   /// Flagge mit Anker drauf (Wasserstraße kann gebaut werden)
    };
    using Params = boost::variant<FlagType, SoldierCount>;

    enum class BuildTab
    {
        Hut,
        House,
        Castle,
        Mine,
        Harbor
    };
    friend constexpr auto maxEnumValue(BuildTab) { return BuildTab::Harbor; }

    /// Struktur mit den Tabs, die angestellt werden sollen
    class Tabs
    {
    public:
        /// Haupttabs
        bool build, setflag, watch, flag, cutroad, upgradeRoad, attack, sea_attack;
        /// Gebäude-Bau-Tabs
        BuildTab build_tabs;

        Tabs()
            : build(false), setflag(false), watch(false), flag(false), cutroad(false), upgradeRoad(false),
              attack(false), sea_attack(false), build_tabs(BuildTab::Hut)
        {}
    };

private:
    GameInterface& gi;
    GameWorldView& gwv;

    MapPoint selectedPt;
    DrawPoint mousePosAtOpen_;

    /// Anzahl gewählter Soldaten für den Angriff und die Maximalanzahl
    unsigned selected_soldiers_count;
    unsigned available_soldiers_count;
    /// Dasselbe für Schiffsangriffe
    unsigned selected_soldiers_count_sea;
    unsigned available_soldiers_count_sea;
    /// Die einzelnen Höhen für die einzelnen Tabs im Bautab
    std::array<unsigned short, 4> building_tab_heights;

public:
    iwAction(GameInterface& gi, GameWorldView& gwv, const Tabs& tabs, MapPoint selectedPt, const DrawPoint& mousePos,
             Params params, bool military_buildings);
    ~iwAction() override;

    /// Gibt zurück, auf welchen Punkt es sich bezieht
    const MapPoint& GetSelectedPt() const { return selectedPt; }

private:
    void Msg_Group_ButtonClick(unsigned group_id, unsigned ctrl_id) override;
    void Msg_TabChange(unsigned ctrl_id, unsigned short tab_id) override;
    void Msg_Group_TabChange(unsigned group_id, unsigned ctrl_id, unsigned short tab_id) override;
    void Msg_PaintAfter() override;

    inline void Msg_ButtonClick_TabBuild(unsigned ctrl_id);
    inline void Msg_ButtonClick_TabCutRoad(unsigned ctrl_id);
    inline void Msg_ButtonClick_TabFlag(unsigned ctrl_id);
    inline void Msg_ButtonClick_TabAttack(unsigned ctrl_id);
    inline void Msg_ButtonClick_TabSeaAttack(unsigned ctrl_id);
    inline void Msg_ButtonClick_TabSetFlag(unsigned ctrl_id);
    inline void Msg_ButtonClick_TabWatch(unsigned ctrl_id);

    void DisableMousePosResetOnClose();

    /// Fügt Angriffs-Steuerelemente für bestimmte Gruppe hinzu
    void AddAttackControls(ctrlGroup* group, unsigned attackers_count);
    void AddUpgradeRoad(ctrlGroup* group, unsigned& x, unsigned& width);
    bool DoUpgradeRoad();
};
