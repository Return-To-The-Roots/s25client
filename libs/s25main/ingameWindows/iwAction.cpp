// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "iwAction.h"
#include "GameInterface.h"
#include "GamePlayer.h"
#include "GlobalGameSettings.h"
#include "Loader.h"
#include "WindowManager.h"
#include "addons/const_addons.h"
#include "buildings/nobMilitary.h"
#include "controls/ctrlBuildingIcon.h"
#include "controls/ctrlGroup.h"
#include "controls/ctrlOptionGroup.h"
#include "controls/ctrlTab.h"
#include "drivers/VideoDriverWrapper.h"
#include "iwDemolishBuilding.h"
#include "iwMilitaryBuilding.h"
#include "iwObservate.h"
#include "network/GameClient.h"
#include "ogl/FontStyle.h"
#include "ogl/glFont.h"
#include "world/GameWorldBase.h"
#include "world/GameWorldView.h"
#include "world/GameWorldViewer.h"
#include "nodeObjs/noFlag.h"
#include "gameData/BuildingConsts.h"
#include "gameData/const_gui_ids.h"
#include <sstream>

// Tab - Flags
enum TabID
{
    TAB_BUILD = 1,
    TAB_SETFLAG,
    TAB_WATCH,
    TAB_FLAG,
    TAB_CUTROAD,
    TAB_ATTACK,
    TAB_SEAATTACK
};

iwAction::iwAction(GameInterface& gi, GameWorldView& gwv, const Tabs& tabs, MapPoint selectedPt,
                   const DrawPoint& mousePos, Params params, bool military_buildings)
    : IngameWindow(CGI_ACTION, mousePos, Extent(200, 254), _("Activity window"), LOADER.GetImageN("io", 1)), gi(gi),
      gwv(gwv), selectedPt(selectedPt), mousePosAtOpen_(mousePos)
{
    /*
        TAB_FLAG    1 = Land road
        TAB_FLAG    2 = Waterway
        TAB_FLAG    3 = Pull down flag
        TAB_FLAG    4 = Send geologist
        TAB_FLAG    5 = Send scout

        TAB_CUTROAD 1 = Cut Road

        TAB_BUILD   100-108, 200-212, 300-303, 400-403 = Buildings

        TAB_SETFLAG 1 = Erect flag

        TAB_WATCH   1 =
        TAB_WATCH   2 =
        TAB_WATCH   3 = zum HQ
        TAB_WATCH	4 = notify allies of location

        TAB_ATTACK  1 = Less soldiers
        TAB_ATTACK  2 = More soldiers
        TAB_ATTACK  3 = Option group: Better/Weaker
        TAB_ATTACK  4 = Angriff
        TAB_ATTACK  10-14 = Direktauswahl Anzahl
    */

    const GamePlayer& player = gwv.GetViewer().GetPlayer();

    /// Haupttab
    ctrlTab* main_tab = AddTabCtrl(0, DrawPoint(10, 20), 180);

    // Bau-main_tab
    if(tabs.build)
    {
        ctrlGroup* group = main_tab->AddTab(LOADER.GetImageN("io", 18), _("-> Build house"), TAB_BUILD);

        ctrlTab* build_tab = group->AddTabCtrl(1, DrawPoint(0, 45), 180);

        // Building tabs
        if(tabs.build_tabs == BuildTab::Mine) // mines
            build_tab->AddTab(LOADER.GetImageN("io", 76), _("-> Dig mines"), int(BuildTab::Mine));
        else
        {
            build_tab->AddTab(LOADER.GetImageN("io", 67), _("-> Build hut"), int(BuildTab::Hut));
            if(tabs.build_tabs >= BuildTab::House)
                build_tab->AddTab(LOADER.GetImageN("io", 68), _("-> Build house"), int(BuildTab::House));
            if(tabs.build_tabs >= BuildTab::Castle) // castle & harbor
                build_tab->AddTab(LOADER.GetImageN("io", 69), _("-> Build castle"), int(BuildTab::Castle));
        }

        // add building icons to TabCtrl. Note: None for BuildTab::Harbor (last)
        const helpers::EnumArray<std::vector<BuildingType>, BuildTab> building_icons = {
          // Linebreak
          {{BuildingType::Woodcutter, BuildingType::Forester, BuildingType::Quarry, BuildingType::Fishery,
            BuildingType::Hunter, BuildingType::Barracks, BuildingType::Guardhouse, BuildingType::LookoutTower,
            BuildingType::Well},
           {BuildingType::Sawmill, BuildingType::Slaughterhouse, BuildingType::Mill, BuildingType::Bakery,
            BuildingType::Ironsmelter, BuildingType::Metalworks, BuildingType::Armory, BuildingType::Mint,
            BuildingType::Shipyard, BuildingType::Brewery, BuildingType::Storehouse, BuildingType::Watchtower,
            BuildingType::Catapult},
           {BuildingType::Farm, BuildingType::PigFarm, BuildingType::DonkeyBreeder, BuildingType::Charburner,
            BuildingType::Fortress, BuildingType::HarborBuilding},
           {BuildingType::GoldMine, BuildingType::IronMine, BuildingType::CoalMine, BuildingType::GraniteMine}}};

        /// Flexible what-buildings-are-available handling
        helpers::EnumArray<bool, BuildingType> building_available;

        // First enable all buildings
        for(const auto bld : helpers::enumRange<BuildingType>())
            building_available[bld] = player.IsBuildingEnabled(bld);

        // Now deactivate those we don't want

        if(tabs.build_tabs != BuildTab::Harbor)
            building_available[BuildingType::HarborBuilding] = false;

        if(!military_buildings)
        {
            building_available[BuildingType::Barracks] = false;
            building_available[BuildingType::Guardhouse] = false;
            building_available[BuildingType::Watchtower] = false;
            building_available[BuildingType::Fortress] = false;
        }

        if(gwv.GetWorld().GetGGS().isEnabled(AddonId::CHANGE_GOLD_DEPOSITS))
        {
            building_available[BuildingType::GoldMine] = false;
            building_available[BuildingType::Mint] = false;
        }

        if(!player.CanBuildCatapult())
            building_available[BuildingType::Catapult] = false;

        if(!gwv.GetWorld().GetGGS().isEnabled(AddonId::CHARBURNER))
            building_available[BuildingType::Charburner] = false;

        constexpr helpers::EnumArray<unsigned, BuildTab> NUM_TABS = {1, 2, 3, 1, 3};

        for(unsigned char i = 0; i < NUM_TABS[tabs.build_tabs]; ++i)
        {
            unsigned char k = 0;
            const BuildTab bt = (tabs.build_tabs == BuildTab::Mine) ? BuildTab::Mine : BuildTab(i);

            for(const BuildingType bld : building_icons[bt])
            {
                if(!building_available[bld])
                    continue;

                // Baukosten im Tooltip mit anzeigen
                std::stringstream tooltip;
                tooltip << _(BUILDING_NAMES[bld]);

                tooltip << _("\nCosts: ");
                if(BUILDING_COSTS[bld].boards > 0)
                    tooltip << (int)BUILDING_COSTS[bld].boards << _(" boards");
                if(BUILDING_COSTS[bld].stones > 0)
                {
                    if(BUILDING_COSTS[bld].boards > 0)
                        tooltip << ", ";
                    tooltip << (int)BUILDING_COSTS[bld].stones << _(" stones");
                }

                DrawPoint iconPos((k % 5) * 36, (k / 5) * 36 + 45);
                build_tab->GetGroup(static_cast<int>(bt))
                  ->AddBuildingIcon(k, iconPos, bld, player.nation, 36, tooltip.str());

                ++k;
            }

            building_tab_heights[static_cast<int>(bt)] = (k / 5 + ((k % 5 != 0) ? 1 : 0)) * 36 + 150;
        }

        build_tab->SetSelection(0, true);
    }

    // Wenn es einen Flaggen-main_tab gibt, dann entsprechend die Buttons anordnen, wie sie gebraucht werden
    if(tabs.flag)
    {
        ctrlGroup* group = main_tab->AddTab(LOADER.GetImageN("io", 70), _("Erect flag"), TAB_FLAG);

        switch(boost::get<FlagType>(params))
        {
            case FlagType::Normal:
            {
                group->AddImageButton(1, DrawPoint(0, 45), Extent(45, 36), TextureColor::Grey,
                                      LOADER.GetImageN("io", 65), _("Build road"));
                group->AddImageButton(3, DrawPoint(45, 45), Extent(45, 36), TextureColor::Grey,
                                      LOADER.GetImageN("io", 118), _("Pull down flag"));
                group->AddImageButton(4, DrawPoint(90, 45), Extent(45, 36), TextureColor::Grey,
                                      LOADER.GetImageN("io", 20), _("Call in geologist"));
                group->AddImageButton(5, DrawPoint(135, 45), Extent(45, 36), TextureColor::Grey,
                                      LOADER.GetImageN("io", 96), _("Send out scout"));
            }
            break;
            case FlagType::WaterFlag:
            {
                group->AddImageButton(1, DrawPoint(0, 45), Extent(36, 36), TextureColor::Grey,
                                      LOADER.GetImageN("io", 65), _("Build road"));
                group->AddImageButton(2, DrawPoint(36, 45), Extent(36, 36), TextureColor::Grey,
                                      LOADER.GetImageN("io", 95), _("Build waterway"));
                group->AddImageButton(3, DrawPoint(72, 45), Extent(36, 36), TextureColor::Grey,
                                      LOADER.GetImageN("io", 118), _("Pull down flag"));
                group->AddImageButton(4, DrawPoint(108, 45), Extent(36, 36), TextureColor::Grey,
                                      LOADER.GetImageN("io", 20), _("Call in geologist"));
                group->AddImageButton(5, DrawPoint(144, 45), Extent(36, 36), TextureColor::Grey,
                                      LOADER.GetImageN("io", 96), _("Send out scout"));
            }
            break;
            case FlagType::HQ:
            {
                group->AddImageButton(1, DrawPoint(0, 45), Extent(180, 36), TextureColor::Grey,
                                      LOADER.GetImageN("io", 65), _("Build road"));
            }
            break;
            case FlagType::Storehouse:
            {
                group->AddImageButton(1, DrawPoint(0, 45), Extent(90, 36), TextureColor::Grey,
                                      LOADER.GetImageN("io", 65), _("Build road"));
                group->AddImageButton(3, DrawPoint(90, 45), Extent(90, 36), TextureColor::Grey,
                                      LOADER.GetImageN("io", 118), _("Demolish house"));
            }
            break;
        }
    }

    // Flagge Setzen-main_tab
    if(tabs.setflag)
    {
        ctrlGroup* group = main_tab->AddTab(LOADER.GetImageN("io", 45), _("Erect flag"), TAB_SETFLAG);

        unsigned nr = 70;
        if(boost::get<FlagType>(params) == FlagType::WaterFlag)
            nr = 94;

        // Straße aufwerten ggf anzeigen
        Extent btSize(180, 36);
        unsigned btPosX = 90;
        AddUpgradeRoad(group, btPosX, btSize.x);

        group->AddImageButton(1, DrawPoint(0, 45), btSize, TextureColor::Grey, LOADER.GetImageN("io", nr),
                              _("Erect flag"));
    }

    // Cut-main_tab
    if(tabs.cutroad)
    {
        ctrlGroup* group = main_tab->AddTab(LOADER.GetImageN("io", 19), _("Dig up road"), TAB_CUTROAD);

        // Straße aufwerten ggf anzeigen
        Extent btSize(180, 36);
        unsigned btPosX = 0;
        if(tabs.upgradeRoad)
            AddUpgradeRoad(group, btPosX, btSize.x);

        group->AddImageButton(1, DrawPoint(btPosX, 45), btSize, TextureColor::Grey, LOADER.GetImageN("io", 32),
                              _("Dig up road"));
    }

    if(tabs.attack)
    {
        ctrlGroup* group = main_tab->AddTab(LOADER.GetImageN("io", 98), _("Attack options"), TAB_ATTACK);
        available_soldiers_count = boost::get<SoldierCount>(params);
        AddAttackControls(group, available_soldiers_count);
        selected_soldiers_count = 1;
    }

    if(tabs.sea_attack)
    {
        ctrlGroup* group = main_tab->AddTab(LOADER.GetImageN("io", 177), _("Attack options"), TAB_SEAATTACK);

        selected_soldiers_count_sea = 1;
        available_soldiers_count_sea = gwv.GetViewer().GetNumSoldiersForSeaAttack(selectedPt);

        AddAttackControls(group, available_soldiers_count_sea);
    }

    // Beobachten-main_tab
    if(tabs.watch)
    {
        ctrlGroup* group = main_tab->AddTab(LOADER.GetImageN("io", 36), _("Display options"), TAB_WATCH);
        const Extent btSize(45, 36);
        DrawPoint curPos(0, 45);
        group->AddImageButton(1, curPos, btSize, TextureColor::Grey, LOADER.GetImageN("io", 108),
                              _("Observation window"));
        curPos.x += btSize.x;
        group->AddImageButton(2, curPos, btSize, TextureColor::Grey, LOADER.GetImageN("io", 179), _("House names"));
        curPos.x += btSize.x;
        group->AddImageButton(3, curPos, btSize, TextureColor::Grey, LOADER.GetImageN("io", 180),
                              _("Go to headquarters"));
        curPos.x += btSize.x;
        group->AddImageButton(4, curPos, btSize, TextureColor::Grey, LOADER.GetImageN("io", 107),
                              _("Notify allies of this location"));
    }

    main_tab->SetSelection(0, true);

    DrawPoint adjPos = GetPos();
    DrawPoint outerPt = GetPos() + GetSize();
    if(outerPt.x > static_cast<int>(VIDEODRIVER.GetRenderSize().x))
        adjPos.x = mousePos.x - GetSize().x - 40;
    if(outerPt.y > static_cast<int>(VIDEODRIVER.GetRenderSize().y))
        adjPos.y = mousePos.y - GetSize().y - 40;
    if(adjPos != GetPos())
        SetPos(adjPos);

    VIDEODRIVER.SetMousePos(GetDrawPos() + DrawPoint(20, 75));
}

void iwAction::AddUpgradeRoad(ctrlGroup* group, unsigned& /*x*/, unsigned& width)
{
    RTTR_Assert(group);

    if(gwv.GetWorld().GetGGS().isEnabled(AddonId::MANUAL_ROAD_ENLARGEMENT))
    {
        width = 90;
        group->AddImageButton(2, DrawPoint(90, 45), Extent(width, 36), TextureColor::Grey, LOADER.GetImageN("io", 44),
                              _("Upgrade to donkey road"));
    }
}

bool iwAction::DoUpgradeRoad()
{
    Direction flag_dir;
    const noFlag* flag = gwv.GetWorld().GetRoadFlag(selectedPt, flag_dir);
    if(flag)
        return GAMECLIENT.UpgradeRoad(flag->GetPos(), flag_dir);
    else
        return true;
}

/// Fügt Angriffs-Steuerelemente für bestimmte Gruppe hinzu
void iwAction::AddAttackControls(ctrlGroup* group, const unsigned attackers_count)
{
    // Verfügbare Soldatenzahl steht in params, wenns keine gibt, einfach Meldung anzeigen: "Angriff nicht möglich!"
    if(attackers_count == 0)
    {
        // Angriff nicht  möglich!
        group->AddText(1, DrawPoint(90, 56), _("Attack not possible."), COLOR_YELLOW, FontStyle::CENTER, NormalFont);
    } else
    {
        selected_soldiers_count = 1;

        // Minus und Plus - Button
        group->AddImageButton(1, DrawPoint(3, 49), Extent(26, 32), TextureColor::Grey, LOADER.GetImageN("io", 139),
                              _("Less attackers"));
        group->AddImageButton(2, DrawPoint(89, 49), Extent(26, 32), TextureColor::Grey, LOADER.GetImageN("io", 138),
                              _("More attackers"));

        // Starke/Schwache Soldaten
        ctrlOptionGroup* ogroup = group->AddOptionGroup(3, GroupSelectType::Illuminate);
        ogroup->AddImageButton(0, DrawPoint(146, 49), Extent(30, 33), TextureColor::Grey, LOADER.GetImageN("io", 31),
                               _("Weak attackers"));
        ogroup->AddImageButton(1, DrawPoint(117, 49), Extent(30, 33), TextureColor::Grey, LOADER.GetImageN("io", 30),
                               _("Strong attackers"));
        // standardmäßig starke Soldaten
        ogroup->SetSelection(1);

        // Schnellauswahl-Buttons
        unsigned buttons_count = (attackers_count > 3) ? 4 : attackers_count;
        unsigned short button_width = 112 / buttons_count;

        for(unsigned i = 0; i < buttons_count; ++i)
            group->AddImageButton(10 + i, DrawPoint(3 + i * button_width, 83), Extent(button_width, 32),
                                  TextureColor::Grey, LOADER.GetImageN("io", 204 + i), _("Number of attackers"));

        // Angriffsbutton
        group->AddImageButton(4, DrawPoint(117, 83), Extent(59, 32), TextureColor::Red1, LOADER.GetImageN("io", 25),
                              _("Attack!"));
    }
}

void iwAction::Close()
{
    if(ShouldBeClosed())
        return;
    IngameWindow::Close();
    if(mousePosAtOpen_.isValid())
        VIDEODRIVER.SetMousePos(mousePosAtOpen_);
}

void iwAction::Msg_Group_ButtonClick(const unsigned /*group_id*/, const unsigned ctrl_id)
{
    switch(GetCtrl<ctrlTab>(0)->GetCurrentTab())
    {
        default: break;

        case TAB_ATTACK:
        {
            Msg_ButtonClick_TabAttack(ctrl_id);
        }
        break;
        case TAB_SEAATTACK:
        {
            Msg_ButtonClick_TabSeaAttack(ctrl_id);
        }
        break;

        case TAB_FLAG:
        {
            Msg_ButtonClick_TabFlag(ctrl_id);
        }
        break;

        case TAB_BUILD:
        {
            Msg_ButtonClick_TabBuild(ctrl_id);
        }
        break;

        case TAB_SETFLAG:
        {
            Msg_ButtonClick_TabSetFlag(ctrl_id);
        }
        break;

        case TAB_CUTROAD:
        {
            Msg_ButtonClick_TabCutRoad(ctrl_id);
        }
        break;

        case TAB_WATCH:
        {
            Msg_ButtonClick_TabWatch(ctrl_id);
        }
        break;
    }
}

void iwAction::Msg_TabChange(const unsigned ctrl_id, const unsigned short tab_id)
{
    switch(ctrl_id)
    {
        case 0: // Haupttabs
        {
            unsigned short height = 0;
            switch(tab_id)
            {
                case TAB_FLAG:
                case TAB_CUTROAD:
                case TAB_SETFLAG:
                case TAB_WATCH: height = 138; break;
                case TAB_BUILD:
                {
                    height = building_tab_heights
                      [GetCtrl<ctrlTab>(0)->GetGroup(TAB_BUILD)->GetCtrl<ctrlTab>(1)->GetCurrentTab()];
                }
                break;
                case TAB_ATTACK:
                {
                    if(available_soldiers_count > 0)
                        height = 178;
                    else
                        height = 130;
                }
                break;
                case TAB_SEAATTACK:
                {
                    if(available_soldiers_count_sea > 0)
                        height = 178;
                    else
                        height = 130;
                }
                break;
            }

            SetHeight(height);
        }
        break;
    }
}

void iwAction::Msg_Group_TabChange(const unsigned /*group_id*/, const unsigned ctrl_id, const unsigned short tab_id)
{
    switch(ctrl_id)
    {
        case 1: // Gebäudetabs
        {
            SetHeight(building_tab_heights[tab_id]);
        }
        break;
    }
}

void iwAction::Msg_PaintAfter()
{
    IngameWindow::Msg_PaintAfter();
    auto* tab = GetCtrl<ctrlTab>(0);
    if(tab)
    {
        static boost::format fmt("%u/%u");
        // Anzeige Soldaten/mögliche Soldatenanzahl bei Angriffstab
        if(tab->GetCurrentTab() == TAB_ATTACK && available_soldiers_count > 0)
        {
            fmt % selected_soldiers_count % available_soldiers_count;
            LargeFont->Draw(GetDrawPos() + DrawPoint(67, 79), fmt.str(), FontStyle::CENTER, COLOR_YELLOW);
        } else if(tab->GetCurrentTab() == TAB_SEAATTACK && available_soldiers_count_sea > 0)
        {
            fmt % selected_soldiers_count_sea % available_soldiers_count_sea;
            LargeFont->Draw(GetDrawPos() + DrawPoint(67, 79), fmt.str(), FontStyle::CENTER, COLOR_YELLOW);
        }
    }
}

void iwAction::Msg_ButtonClick_TabAttack(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case 1: // 1 Soldat weniger
        {
            if(selected_soldiers_count > 1)
                --selected_soldiers_count;
        }
        break;
        case 2: // 1 Soldat mehr
        {
            if(selected_soldiers_count < available_soldiers_count)
                ++selected_soldiers_count;
        }
        break;
        case 10: // auf bestimmte Anzahl setzen
        case 11:
        case 12:
        case 13:
        {
            if(available_soldiers_count > 4)
                selected_soldiers_count = (ctrl_id - 9) * available_soldiers_count / 4;
            else
                selected_soldiers_count = ctrl_id - 9;
        }
        break;
        case 4: // Angriff!
        {
            auto* ogroup = GetCtrl<ctrlTab>(0)->GetGroup(TAB_ATTACK)->GetCtrl<ctrlOptionGroup>(3);
            if(GAMECLIENT.Attack(selectedPt, selected_soldiers_count, (ogroup->GetSelection() == 1)))
                Close();
        }
        break;
    }
}

void iwAction::Msg_ButtonClick_TabSeaAttack(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case 1: // 1 Soldat weniger
        {
            if(selected_soldiers_count_sea > 1)
                --selected_soldiers_count_sea;
        }
        break;
        case 2: // 1 Soldat mehr
        {
            if(selected_soldiers_count_sea < available_soldiers_count_sea)
                ++selected_soldiers_count_sea;
        }
        break;
        case 10: // auf bestimmte Anzahl setzen
        case 11:
        case 12:
        case 13:
        {
            if(available_soldiers_count_sea > 4)
                selected_soldiers_count_sea = (ctrl_id - 9) * available_soldiers_count_sea / 4;
            else
                selected_soldiers_count_sea = ctrl_id - 9;
        }
        break;
        case 4: // Angriff!
        {
            auto* ogroup = GetCtrl<ctrlTab>(0)->GetGroup(TAB_SEAATTACK)->GetCtrl<ctrlOptionGroup>(3);
            if(GAMECLIENT.SeaAttack(selectedPt, selected_soldiers_count_sea, (ogroup->GetSelection() == 1)))
                Close();
        }
        break;
    }
}

void iwAction::Msg_ButtonClick_TabFlag(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case 1: // Straße bauen
        {
            gi.GI_StartRoadBuilding(selectedPt, false);
            Close();
        }
        break;
        case 2: // Wasserstraße bauen
        {
            gi.GI_StartRoadBuilding(selectedPt, true);
            Close();
        }
        break;
        case 3: // Flagge abreißen
        {
            const GameWorldBase& world = gwv.GetWorld();
            NodalObjectType nop = (world.GetNO(world.GetNeighbour(selectedPt, Direction::NorthWest)))->GetType();
            // Haben wir ne Baustelle/Gebäude dran?
            if(nop == NodalObjectType::Building || nop == NodalObjectType::Buildingsite)
            {
                // Abreißen?
                const auto* building =
                  world.GetSpecObj<noBaseBuilding>(world.GetNeighbour(selectedPt, Direction::NorthWest));

                // Militärgebäude?
                if(building->GetGOT() == GO_Type::NobMilitary)
                {
                    // Darf das Gebäude abgerissen werden?
                    if(!static_cast<const nobMilitary*>(building)->IsDemolitionAllowed())
                    {
                        // Nein, dann Messagebox anzeigen
                        iwMilitaryBuilding::DemolitionNotAllowed(world.GetGGS());
                        break;
                    }
                }

                WINDOWMANAGER.Show(std::make_unique<iwDemolishBuilding>(gwv, building, true));
                DisableMousePosResetOnClose();
                Close();
            } else
            {
                if(GAMECLIENT.DestroyFlag(selectedPt))
                    Close();
            }
        }
        break;
        case 4: // Geologen rufen
        {
            if(GAMECLIENT.CallSpecialist(selectedPt, Job::Geologist))
                Close();
        }
        break;
        case 5: // Späher rufen
        {
            if(GAMECLIENT.CallSpecialist(selectedPt, Job::Scout))
                Close();
        }
        break;
    }
}

void iwAction::Msg_ButtonClick_TabBuild(const unsigned ctrl_id)
{
    // Klick auf Gebäudebauicon
    if(GAMECLIENT.SetBuildingSite(selectedPt, GetCtrl<ctrlTab>(0)
                                                ->GetGroup(TAB_BUILD)
                                                ->GetCtrl<ctrlTab>(1)
                                                ->GetCurrentGroup()
                                                ->GetCtrl<ctrlBuildingIcon>(ctrl_id)
                                                ->GetType()))
    {
        // Fenster schließen
        Close();
    }
}

void iwAction::Msg_ButtonClick_TabSetFlag(const unsigned ctrl_id)
{
    bool success = false;
    switch(ctrl_id)
    {
        case 1: // Flagge setzen
            success = GAMECLIENT.SetFlag(selectedPt);
            break;
        case 2: // Weg aufwerten
            success = DoUpgradeRoad();
            break;
    }

    if(success)
        Close();
}

void iwAction::Msg_ButtonClick_TabCutRoad(const unsigned ctrl_id)
{
    bool success = true;
    switch(ctrl_id)
    {
        case 1: // Straße abreißen
        {
            Direction flag_dir;
            const noFlag* flag = gwv.GetWorld().GetRoadFlag(selectedPt, flag_dir);
            if(flag)
                success = GAMECLIENT.DestroyRoad(flag->GetPos(), flag_dir);
        }
        break;
        case 2: // Straße aufwerten
            success = DoUpgradeRoad();
            break;
    }

    if(success)
        Close();
}

void iwAction::Msg_ButtonClick_TabWatch(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case 1:
            // TODO: bestimen, was an der position selected ist
            WINDOWMANAGER.Show(std::make_unique<iwObservate>(gwv, selectedPt));
            DisableMousePosResetOnClose();
            Close();
            break;
        case 2: // Häusernamen/Prozent anmachen
            gwv.ToggleShowNamesAndProductivity();
            Close();
            break;
        case 3: // zum HQ
            gwv.MoveToMapPt(gwv.GetViewer().GetPlayer().GetHQPos());
            DisableMousePosResetOnClose();
            Close();
            break;
        case 4:
            if(GAMECLIENT.NotifyAlliesOfLocation(selectedPt))
                Close();
            break;
    }
}

void iwAction::DisableMousePosResetOnClose()
{
    mousePosAtOpen_ = DrawPoint::Invalid();
}
