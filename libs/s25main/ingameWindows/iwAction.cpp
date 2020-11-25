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
                   const DrawPoint& mousePos, unsigned params, bool military_buildings)
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
        if(tabs.build_tabs == Tabs::BT_MINE) // mines
            build_tab->AddTab(LOADER.GetImageN("io", 76), _("-> Dig mines"), Tabs::BT_MINE);
        else
        {
            build_tab->AddTab(LOADER.GetImageN("io", 67), _("-> Build hut"), Tabs::BT_HUT);
            if(tabs.build_tabs >= Tabs::BT_HOUSE)
                build_tab->AddTab(LOADER.GetImageN("io", 68), _("-> Build house"), Tabs::BT_HOUSE);
            if(tabs.build_tabs >= Tabs::BT_CASTLE) // castle & harbor
                build_tab->AddTab(LOADER.GetImageN("io", 69), _("-> Build castle"), Tabs::BT_CASTLE);
        }

        // add building icons to TabCtrl
        const unsigned char building_count_max = 13;
        const std::array<unsigned, 4> building_count = {9, 13, 6, 4};
        const helpers::MultiArray<BuildingType, building_count.size(), building_count_max> building_icons = {
          // Linebreak
          {{/* 0 */
            /* 0 */ BLD_WOODCUTTER,
            /* 1 */ BLD_FORESTER,
            /* 2 */ BLD_QUARRY,
            /* 3 */ BLD_FISHERY,
            /* 4 */ BLD_HUNTER,
            /* 5 */ BLD_BARRACKS,
            /* 6 */ BLD_GUARDHOUSE,
            /* 7 */ BLD_LOOKOUTTOWER,
            /* 8 */ BLD_WELL},
           {/* 1 */
            /*  0 */ BLD_SAWMILL,
            /*  1 */ BLD_SLAUGHTERHOUSE,
            /*  2 */ BLD_MILL,
            /*  3 */ BLD_BAKERY,
            /*  4 */ BLD_IRONSMELTER,
            /*  5 */ BLD_METALWORKS,
            /*  6 */ BLD_ARMORY,
            /*  7 */ BLD_MINT,
            /*  8 */ BLD_SHIPYARD,
            /*  9 */ BLD_BREWERY,
            /* 10 */ BLD_STOREHOUSE,
            /* 11 */ BLD_WATCHTOWER,
            /* 12 */ BLD_CATAPULT},
           {/* 2 */
            /* 0 */ BLD_FARM,
            /* 1 */ BLD_PIGFARM,
            /* 2 */ BLD_DONKEYBREEDER,
            /* 3 */ BLD_CHARBURNER,
            /* 4 */ BLD_FORTRESS,
            /* 5 */ BLD_HARBORBUILDING},
           {/* 3 */
            /* 0 */ BLD_GOLDMINE,
            /* 1 */ BLD_IRONMINE,
            /* 2 */ BLD_COALMINE,
            /* 3 */ BLD_GRANITEMINE}}};

        /// Flexible what-buildings-are-available handling
        helpers::MultiArray<bool, 4, building_count_max> building_available;

        // First enable all buildings
        for(unsigned char i = 0; i < 4; ++i)
        {
            for(unsigned char j = 0; j < building_count_max; ++j)
            {
                if(j < building_count[i])
                {
                    building_available[i][j] = player.IsBuildingEnabled(building_icons[i][j]);
                } else
                {
                    building_available[i][j] = false;
                }
            }
        }

        // Now deactivate those we don't want

        // Harbor
        if(tabs.build_tabs != Tabs::BT_HARBOR)
            building_available[2][5] = false;

        // Military buildings
        if(!military_buildings)
        {
            building_available[0][5] = false;
            building_available[0][6] = false;
            building_available[1][11] = false;
            building_available[2][4] = false;
        }

        // Mint and Goldmine
        if(gwv.GetWorld().GetGGS().isEnabled(AddonId::CHANGE_GOLD_DEPOSITS))
        {
            building_available[1][7] = false;
            building_available[3][0] = false;
        }

        // Catapult
        if(!player.CanBuildCatapult()) //-V807
            building_available[1][12] = false;

        // Charburner
        if(!gwv.GetWorld().GetGGS().isEnabled(AddonId::CHARBURNER))
            building_available[2][3] = false;

        const std::array<unsigned, 5> NUM_TABS = {1, 2, 3, 1, 3};

        for(unsigned char i = 0; i < NUM_TABS[tabs.build_tabs]; ++i)
        {
            unsigned char k = 0;
            Tabs::BuildTab bt = (tabs.build_tabs == Tabs::BT_MINE) ? Tabs::BT_MINE : Tabs::BuildTab(i);

            for(unsigned char j = 0; j < building_count_max; ++j)
            {
                if(!building_available[bt][j])
                    continue;

                // Baukosten im Tooltip mit anzeigen
                std::stringstream tooltip;
                tooltip << _(BUILDING_NAMES[building_icons[bt][j]]);

                tooltip << _("\nCosts: ");
                if(BUILDING_COSTS[player.nation][building_icons[bt][j]].boards > 0)
                    tooltip << (int)BUILDING_COSTS[player.nation][building_icons[bt][j]].boards << _(" boards");
                if(BUILDING_COSTS[player.nation][building_icons[bt][j]].stones > 0)
                {
                    if(BUILDING_COSTS[player.nation][building_icons[bt][j]].boards > 0)
                        tooltip << ", ";
                    tooltip << (int)BUILDING_COSTS[player.nation][building_icons[bt][j]].stones << _(" stones");
                }

                DrawPoint iconPos((k % 5) * 36, (k / 5) * 36 + 45);
                build_tab->GetGroup(bt)->AddBuildingIcon(j, iconPos, building_icons[bt][j], player.nation, 36,
                                                         tooltip.str());

                ++k;
            }

            building_tab_heights[bt] = (k / 5 + ((k % 5 != 0) ? 1 : 0)) * 36 + 150;
        }

        build_tab->SetSelection(0, true);
    }

    // Wenn es einen Flaggen-main_tab gibt, dann entsprechend die Buttons anordnen, wie sie gebraucht werden
    if(tabs.flag)
    {
        ctrlGroup* group = main_tab->AddTab(LOADER.GetImageN("io", 70), _("Erect flag"), TAB_FLAG);

        switch(params)
        {
            case AWFT_NORMAL: // normal Flag
            {
                group->AddImageButton(1, DrawPoint(0, 45), Extent(45, 36), TC_GREY, LOADER.GetImageN("io", 65),
                                      _("Build road"));
                group->AddImageButton(3, DrawPoint(45, 45), Extent(45, 36), TC_GREY, LOADER.GetImageN("io", 118),
                                      _("Pull down flag"));
                group->AddImageButton(4, DrawPoint(90, 45), Extent(45, 36), TC_GREY, LOADER.GetImageN("io", 20),
                                      _("Call in geologist"));
                group->AddImageButton(5, DrawPoint(135, 45), Extent(45, 36), TC_GREY, LOADER.GetImageN("io", 96),
                                      _("Send out scout"));
            }
            break;
            case AWFT_WATERFLAG: // Water flag
            {
                group->AddImageButton(1, DrawPoint(0, 45), Extent(36, 36), TC_GREY, LOADER.GetImageN("io", 65),
                                      _("Build road"));
                group->AddImageButton(2, DrawPoint(36, 45), Extent(36, 36), TC_GREY, LOADER.GetImageN("io", 95),
                                      _("Build waterway"));
                group->AddImageButton(3, DrawPoint(72, 45), Extent(36, 36), TC_GREY, LOADER.GetImageN("io", 118),
                                      _("Pull down flag"));
                group->AddImageButton(4, DrawPoint(108, 45), Extent(36, 36), TC_GREY, LOADER.GetImageN("io", 20),
                                      _("Call in geologist"));
                group->AddImageButton(5, DrawPoint(144, 45), Extent(36, 36), TC_GREY, LOADER.GetImageN("io", 96),
                                      _("Send out scout"));
            }
            break;
            case AWFT_HQ: // HQ
            {
                group->AddImageButton(1, DrawPoint(0, 45), Extent(180, 36), TC_GREY, LOADER.GetImageN("io", 65),
                                      _("Build road"));
            }
            break;
            case AWFT_STOREHOUSE: // Storehouse
            {
                group->AddImageButton(1, DrawPoint(0, 45), Extent(90, 36), TC_GREY, LOADER.GetImageN("io", 65),
                                      _("Build road"));
                group->AddImageButton(3, DrawPoint(90, 45), Extent(90, 36), TC_GREY, LOADER.GetImageN("io", 118),
                                      _("Demolish house"));
            }
            break;
        }
    }

    // Flagge Setzen-main_tab
    if(tabs.setflag)
    {
        ctrlGroup* group = main_tab->AddTab(LOADER.GetImageN("io", 45), _("Erect flag"), TAB_SETFLAG);

        unsigned nr = 70;
        if(params == AWFT_WATERFLAG)
            nr = 94;

        // Straße aufwerten ggf anzeigen
        Extent btSize(180, 36);
        unsigned btPosX = 90;
        AddUpgradeRoad(group, btPosX, btSize.x);

        group->AddImageButton(1, DrawPoint(0, 45), btSize, TC_GREY, LOADER.GetImageN("io", nr), _("Erect flag"));
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

        group->AddImageButton(1, DrawPoint(btPosX, 45), btSize, TC_GREY, LOADER.GetImageN("io", 32), _("Dig up road"));
    }

    if(tabs.attack)
    {
        ctrlGroup* group = main_tab->AddTab(LOADER.GetImageN("io", 98), _("Attack options"), TAB_ATTACK);
        available_soldiers_count = params;
        AddAttackControls(group, params);
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
        group->AddImageButton(1, curPos, btSize, TC_GREY, LOADER.GetImageN("io", 108), _("Observation window"));
        curPos.x += btSize.x;
        group->AddImageButton(2, curPos, btSize, TC_GREY, LOADER.GetImageN("io", 179), _("House names"));
        curPos.x += btSize.x;
        group->AddImageButton(3, curPos, btSize, TC_GREY, LOADER.GetImageN("io", 180), _("Go to headquarters"));
        curPos.x += btSize.x;
        group->AddImageButton(4, curPos, btSize, TC_GREY, LOADER.GetImageN("io", 107),
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
        group->AddImageButton(2, DrawPoint(90, 45), Extent(width, 36), TC_GREY, LOADER.GetImageN("io", 44),
                              _("Upgrade to donkey road"));
    }
}

void iwAction::DoUpgradeRoad()
{
    Direction flag_dir;
    const noFlag* flag = gwv.GetWorld().GetRoadFlag(selectedPt, flag_dir);
    if(flag)
        GAMECLIENT.UpgradeRoad(flag->GetPos(), flag_dir);
}

/// Fügt Angriffs-Steuerelemente für bestimmte Gruppe hinzu
void iwAction::AddAttackControls(ctrlGroup* group, const unsigned attackers_count)
{
    // Verfügbare Soldatenzahl steht in params, wenns keine gibt, einfach Meldung anzeigen: "Angriff nicht möglich!" (Genauso im Peaceful Mode)
    if(attackers_count == 0 || gwv.GetWorld().GetGGS().getSelection(AddonId::PEACEFUL))
    {
        // Angriff nicht  möglich!
        group->AddText(1, DrawPoint(90, 56), _("Attack not possible."), COLOR_YELLOW, FontStyle::CENTER, NormalFont);
    } else
    {
        selected_soldiers_count = 1;

        // Minus und Plus - Button
        group->AddImageButton(1, DrawPoint(3, 49), Extent(26, 32), TC_GREY, LOADER.GetImageN("io", 139),
                              _("Less attackers"));
        group->AddImageButton(2, DrawPoint(89, 49), Extent(26, 32), TC_GREY, LOADER.GetImageN("io", 138),
                              _("More attackers"));

        // Starke/Schwache Soldaten
        ctrlOptionGroup* ogroup = group->AddOptionGroup(3, ctrlOptionGroup::ILLUMINATE);
        ogroup->AddImageButton(0, DrawPoint(146, 49), Extent(30, 33), TC_GREY, LOADER.GetImageN("io", 31),
                               _("Weak attackers"));
        ogroup->AddImageButton(1, DrawPoint(117, 49), Extent(30, 33), TC_GREY, LOADER.GetImageN("io", 30),
                               _("Strong attackers"));
        // standardmäßig starke Soldaten
        ogroup->SetSelection(1);

        // Schnellauswahl-Buttons
        unsigned buttons_count = (attackers_count > 3) ? 4 : attackers_count;
        unsigned short button_width = 112 / buttons_count;

        for(unsigned i = 0; i < buttons_count; ++i)
            group->AddImageButton(10 + i, DrawPoint(3 + i * button_width, 83), Extent(button_width, 32), TC_GREY,
                                  LOADER.GetImageN("io", 204 + i), _("Number of attackers"));

        // Angriffsbutton
        group->AddImageButton(4, DrawPoint(117, 83), Extent(59, 32), TC_RED1, LOADER.GetImageN("io", 25), _("Attack!"));
    }
}

iwAction::~iwAction()
{
    if(mousePosAtOpen_.isValid())
        VIDEODRIVER.SetMousePos(mousePosAtOpen_);
    gi.GI_WindowClosed(this);
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
                    if(available_soldiers_count > 0 && !gwv.GetWorld().GetGGS().getSelection(AddonId::PEACEFUL))
                        height = 178;
                    else
                        height = 130;
                }
                break;
                case TAB_SEAATTACK:
                {
                    if(available_soldiers_count_sea > 0 && !gwv.GetWorld().GetGGS().getSelection(AddonId::PEACEFUL))
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
        if(tab->GetCurrentTab() == TAB_ATTACK && available_soldiers_count > 0
           && !gwv.GetWorld().GetGGS().getSelection(AddonId::PEACEFUL))
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
            GAMECLIENT.Attack(selectedPt, selected_soldiers_count, (ogroup->GetSelection() == 1));
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
            GAMECLIENT.SeaAttack(selectedPt, selected_soldiers_count_sea, (ogroup->GetSelection() == 1));
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
            NodalObjectType nop = (world.GetNO(world.GetNeighbour(selectedPt, Direction::NORTHWEST)))->GetType();
            // Haben wir ne Baustelle/Gebäude dran?
            if(nop == NOP_BUILDING || nop == NOP_BUILDINGSITE)
            {
                // Abreißen?
                const auto* building =
                  world.GetSpecObj<noBaseBuilding>(world.GetNeighbour(selectedPt, Direction::NORTHWEST));

                // Militärgebäude?
                if(building->GetGOT() == GOT_NOB_MILITARY)
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
                GAMECLIENT.DestroyFlag(selectedPt);
                Close();
            }
        }
        break;
        case 4: // Geologen rufen
        {
            GAMECLIENT.CallSpecialist(selectedPt, JOB_GEOLOGIST);
            Close();
        }
        break;
        case 5: // Späher rufen
        {
            GAMECLIENT.CallSpecialist(selectedPt, JOB_SCOUT);
            Close();
        }
        break;
    }
}

void iwAction::Msg_ButtonClick_TabBuild(const unsigned ctrl_id)
{
    // Klick auf Gebäudebauicon
    GAMECLIENT.SetBuildingSite(selectedPt, GetCtrl<ctrlTab>(0)
                                             ->GetGroup(TAB_BUILD)
                                             ->GetCtrl<ctrlTab>(1)
                                             ->GetCurrentGroup()
                                             ->GetCtrl<ctrlBuildingIcon>(ctrl_id)
                                             ->GetType());

    // Fenster schließen
    Close();
}

void iwAction::Msg_ButtonClick_TabSetFlag(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case 1: // Flagge setzen
            GAMECLIENT.SetFlag(selectedPt);
            break;
        case 2: // Weg aufwerten
            DoUpgradeRoad();
            break;
    }

    Close();
}

void iwAction::Msg_ButtonClick_TabCutRoad(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case 1: // Straße abreißen
        {
            Direction flag_dir;
            const noFlag* flag = gwv.GetWorld().GetRoadFlag(selectedPt, flag_dir);
            if(flag)
                GAMECLIENT.DestroyRoad(flag->GetPos(), flag_dir);
        }
        break;
        case 2: // Straße aufwerten
            DoUpgradeRoad();
            break;
    }

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
            break;
        case 2: // Häusernamen/Prozent anmachen
            gwv.ToggleShowNamesAndProductivity();
            break;
        case 3: // zum HQ
            gwv.MoveToMapPt(gwv.GetViewer().GetPlayer().GetHQPos());
            DisableMousePosResetOnClose();
            break;
        case 4: GAMECLIENT.NotifyAlliesOfLocation(selectedPt); break;
    }

    Close();
}

void iwAction::DisableMousePosResetOnClose()
{
    mousePosAtOpen_ = DrawPoint::Invalid();
}
