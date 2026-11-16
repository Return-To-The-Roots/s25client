// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "IngameWindow.h"
#include "variant.h"
#include "gameTypes/MapCoordinates.h"
#include <array>

class GameInterface;
class GameWorldView;
class ctrlBuildingIcon;
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
    using Params = boost_variant2<FlagType, SoldierCount>;

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

    /// Track which icon currently owns the radius preview (to ignore stale leaves)
    const ctrlBuildingIcon* activeHoveredIcon_;

public:
    iwAction(GameInterface& gi, GameWorldView& gwv, const Tabs& tabs, MapPoint selectedPt, const DrawPoint& mousePos,
             Params params, bool military_buildings);

    void Close() override;

    /// Gibt zurück, auf welchen Punkt es sich bezieht
    const MapPoint& getSelectedPt() const { return selectedPt; }

private:
    void msgGroupButtonClick(unsigned group_id, unsigned ctrl_id) override;
    void msgTabChange(unsigned ctrl_id, unsigned short tab_id) override;
    void msgGroupTabChange(unsigned group_id, unsigned ctrl_id, unsigned short tab_id) override;
    void msgPaintAfter() override;

    inline void msgButtonClickTabBuild(unsigned ctrl_id);
    inline void msgButtonClickTabCutRoad(unsigned ctrl_id);
    inline void msgButtonClickTabFlag(unsigned ctrl_id);
    inline void msgButtonClickTabAttack(unsigned ctrl_id);
    inline void msgButtonClickTabSeaAttack(unsigned ctrl_id);
    inline void msgButtonClickTabSetFlag(unsigned ctrl_id);
    inline void msgButtonClickTabWatch(unsigned ctrl_id);
    inline void msgButtonClickTabCheat(unsigned ctrl_id);

    void DisableMousePosResetOnClose();

    /// Fügt Angriffs-Steuerelemente für bestimmte Gruppe hinzu
    void AddAttackControls(ctrlGroup* group, unsigned attackers_count);
    void AddUpgradeRoad(ctrlGroup* group, unsigned& x, unsigned& width);
    bool DoUpgradeRoad();
};
