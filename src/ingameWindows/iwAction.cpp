// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "defines.h" // IWYU pragma: keep
#include "iwAction.h"

#include "GameInterface.h"
#include "iwDemolishBuilding.h"
#include "iwMilitaryBuilding.h"
#include "iwObservate.h"
#include "world/GameWorldView.h"
#include "Loader.h"
#include "drivers/VideoDriverWrapper.h"
#include "GameClient.h"
#include "WindowManager.h"
#include "controls/ctrlBuildingIcon.h"
#include "controls/ctrlGroup.h"
#include "controls/ctrlOptionGroup.h"
#include "controls/ctrlTab.h"
#include "nodeObjs/noFlag.h"
#include "buildings/nobMilitary.h"
#include "ogl/glArchivItem_Font.h"
#include "gameData/const_gui_ids.h"

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

// Tab - Flags
enum TabID
{
    TAB_BUILD = 1,
    TAB_SETFLAG ,
    TAB_WATCH ,
    TAB_FLAG,
    TAB_CUTROAD,
    TAB_ATTACK,
    TAB_SEAATTACK
};

iwAction::iwAction(GameInterface& gi, GameWorldView& gwv, const Tabs& tabs, MapPoint selectedPt, int mouse_x, int mouse_y, unsigned int params, bool military_buildings)
    : IngameWindow(CGI_ACTION, mouse_x, mouse_y, 200, 254, _("Activity window"), LOADER.GetImageN("io", 1)),
      gi(gi), gwv(gwv), selectedPt(selectedPt), mousePosAtOpen_(mouse_x, mouse_y)
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

    /// Haupttab
    ctrlTab* main_tab = AddTabCtrl(0, 10, 20, 180);

    // Bau-main_tab
    if(tabs.build)
    {
        ctrlGroup* group =  main_tab->AddTab(LOADER.GetImageN("io", 18), _("-> Build house"), TAB_BUILD);

        ctrlTab* build_tab = group->AddTabCtrl(1, 0, 45, 180);

        // Building tabs
        if(tabs.build_tabs == Tabs::BT_MINE) //mines
            build_tab->AddTab(LOADER.GetImageN("io", 76), _("-> Dig mines"), Tabs::BT_MINE);
        else
        {
            build_tab->AddTab(LOADER.GetImageN("io", 67), _("-> Build hut"), Tabs::BT_HUT);
            if(tabs.build_tabs >= Tabs::BT_HOUSE)
                build_tab->AddTab(LOADER.GetImageN("io", 68), _("-> Build house"), Tabs::BT_HOUSE);
            if(tabs.build_tabs >= Tabs::BT_CASTLE) //castle & harbor
                build_tab->AddTab(LOADER.GetImageN("io", 69), _("-> Build castle"), Tabs::BT_CASTLE);
        }

        // add building icons to TabCtrl
        const unsigned char building_count_max = 14;
        const unsigned building_count[4] = { 9, 13, 6, 4 };
        const BuildingType building_icons[4][building_count_max] =
        {
            { /* 0 */
                /* 0 */ BLD_WOODCUTTER,
                /* 1 */ BLD_FORESTER,
                /* 2 */ BLD_QUARRY,
                /* 3 */ BLD_FISHERY,
                /* 4 */ BLD_HUNTER,
                /* 5 */ BLD_BARRACKS,
                /* 6 */ BLD_GUARDHOUSE,
                /* 7 */ BLD_LOOKOUTTOWER,
                /* 8 */ BLD_WELL
            },
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
                /* 12 */ BLD_CATAPULT
            },
            { /* 2 */
                /* 0 */ BLD_FARM,
                /* 1 */ BLD_PIGFARM,
                /* 2 */ BLD_DONKEYBREEDER,
                /* 3 */ BLD_CHARBURNER,
                /* 4 */ BLD_FORTRESS,
                /* 5 */ BLD_HARBORBUILDING
            },
            { /* 3 */
                /* 0 */ BLD_GOLDMINE,
                /* 1 */ BLD_IRONMINE,
                /* 2 */ BLD_COALMINE,
                /* 3 */ BLD_GRANITEMINE
            }
        };

        const unsigned TABS_COUNT[5] = {1, 2, 3, 1, 3};

        /// Flexible what-buildings-are-available handling
        bool building_available[4][building_count_max] ;

        // First enable all buildings
        for (unsigned char i = 0; i < 4; ++i)
        {
            for(unsigned char j = 0; j < building_count_max; ++j)
            {
                if (j < building_count[i])
                {
                    building_available[i][j] = GAMECLIENT.GetLocalPlayer().IsBuildingEnabled(building_icons[i][j]);
                }
                else
                {
                    building_available[i][j] = false;
                }
            }
        }

        // Now deactivate those we don't want

        // Harbor
        if (tabs.build_tabs != Tabs::BT_HARBOR)
            building_available[2][5] = false;

        // Military buildings
        if (!military_buildings)
        {
            building_available[0][5] = false;
            building_available[0][6] = false;
            building_available[1][11] = false;
            building_available[2][4] = false;
        }

        // Mint and Goldmine
        if(GAMECLIENT.GetGGS().isEnabled(AddonId::CHANGE_GOLD_DEPOSITS))
        {
            building_available[1][7] = false;
            building_available[3][0] = false;
        }

        // Catapult
        if (!GAMECLIENT.GetLocalPlayer().CanBuildCatapult()) //-V807
            building_available[1][12] = false;

        // Charburner
        if(!GAMECLIENT.GetGGS().isEnabled(AddonId::CHARBURNER))
            building_available[2][3] = false;

        for(unsigned char i = 0; i < TABS_COUNT[tabs.build_tabs]; ++i)
        {
            unsigned char k = 0;
            Tabs::BuildTab bt = (tabs.build_tabs == Tabs::BT_MINE) ? Tabs::BT_MINE : Tabs::BuildTab(i);

            for(unsigned char j = 0; j < building_count_max; ++j)
            {
                if (!building_available[bt][j])
                    continue;

                // Baukosten im Tooltip mit anzeigen
                std::stringstream tooltip;
                tooltip << _(BUILDING_NAMES[building_icons[bt][j]]);

                tooltip << _("\nCosts: ");
                if(BUILDING_COSTS[GAMECLIENT.GetLocalPlayer().nation][building_icons[bt][j]].boards > 0)
                    tooltip << (int)BUILDING_COSTS[GAMECLIENT.GetLocalPlayer().nation][building_icons[bt][j]].boards << _(" boards");
                if(BUILDING_COSTS[GAMECLIENT.GetLocalPlayer().nation][building_icons[bt][j]].stones > 0)
                {
                    if(BUILDING_COSTS[GAMECLIENT.GetLocalPlayer().nation][building_icons[bt][j]].boards > 0)
                        tooltip << ", ";
                    tooltip << (int)BUILDING_COSTS[GAMECLIENT.GetLocalPlayer().nation][building_icons[bt][j]].stones << _(" stones");
                }

                build_tab->GetGroup(bt)->AddBuildingIcon(j, (k % 5) * 36, (k / 5) * 36 + 45, building_icons[bt][j], GAMECLIENT.GetLocalPlayer().nation, 36, tooltip.str());

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
                group->AddImageButton(1,  0, 45, 45, 36, TC_GREY, LOADER.GetImageN("io",  65), _("Build road"));
                group->AddImageButton(3,  45, 45, 45, 36, TC_GREY, LOADER.GetImageN("io", 118), _("Pull down flag"));
                group->AddImageButton(4, 90, 45, 45, 36, TC_GREY, LOADER.GetImageN("io",  20), _("Call in geologist"));
                group->AddImageButton(5, 135, 45, 45, 36, TC_GREY, LOADER.GetImageN("io",  96), _("Send out scout"));
            } break;
            case AWFT_WATERFLAG: // Water flag
            {
                group->AddImageButton(1,  0, 45, 36, 36, TC_GREY, LOADER.GetImageN("io",  65), _("Build road"));
                group->AddImageButton(2,  36, 45, 36, 36, TC_GREY, LOADER.GetImageN("io",  95), _("Build waterway"));
                group->AddImageButton(3,  72, 45, 36, 36, TC_GREY, LOADER.GetImageN("io", 118), _("Pull down flag"));
                group->AddImageButton(4, 108, 45, 36, 36, TC_GREY, LOADER.GetImageN("io",  20), _("Call in geologist"));
                group->AddImageButton(5, 144, 45, 36, 36, TC_GREY, LOADER.GetImageN("io",  96), _("Send out scout"));
            } break;
            case AWFT_HQ: // HQ
            {
                group->AddImageButton(1, 0, 45, 180, 36, TC_GREY, LOADER.GetImageN("io", 65), _("Build road"));
            } break;
            case AWFT_STOREHOUSE: // Storehouse
            {
                group->AddImageButton(1, 0, 45, 90, 36, TC_GREY, LOADER.GetImageN("io", 65), _("Build road"));
                group->AddImageButton(3, 90, 45, 90, 36, TC_GREY, LOADER.GetImageN("io", 118), _("Demolish house"));
            } break;
        }
    }

    // Flagge Setzen-main_tab
    if(tabs.setflag)
    {
        ctrlGroup* group = main_tab->AddTab(LOADER.GetImageN("io", 45), _("Erect flag"), TAB_SETFLAG);

        unsigned int nr = 70;
        if(params == AWFT_WATERFLAG)
            nr = 94;

        // Straße aufwerten ggf anzeigen
        unsigned int btWidth = 180, btPosX = 90;
        AddUpgradeRoad(group, btPosX, btWidth);

        group->AddImageButton(1, 0, 45, btWidth, 36, TC_GREY, LOADER.GetImageN("io", nr), _("Erect flag"));
    }

    // Cut-main_tab
    if(tabs.cutroad)
    {
        ctrlGroup* group = main_tab->AddTab(LOADER.GetImageN("io", 19), _("Dig up road"), TAB_CUTROAD);

        // Straße aufwerten ggf anzeigen
        unsigned int btWidth = 180, btPosX = 0;
        if(!tabs.setflag)
            AddUpgradeRoad(group, btPosX, btWidth);

        group->AddImageButton(1, btPosX, 45, btWidth, 36, TC_GREY, LOADER.GetImageN("io", 32), _("Dig up road"));
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
        available_soldiers_count_sea = gwv.GetViewer().GetAvailableSoldiersForSeaAttackCount(GAMECLIENT.GetPlayerID(), selectedPt);

        AddAttackControls(group, available_soldiers_count_sea);
    }

    // Beobachten-main_tab
    if(tabs.watch)
    {
        ctrlGroup* group = main_tab->AddTab(LOADER.GetImageN("io", 36), _("Display options"), TAB_WATCH);

        group->AddImageButton(1, 0, 45,  45, 36, TC_GREY, LOADER.GetImageN("io", 108), _("Observation window"));
        group->AddImageButton(2,  45, 45,  45, 36, TC_GREY, LOADER.GetImageN("io", 179), _("House names"));
        group->AddImageButton(3, 90, 45,  45, 36, TC_GREY, LOADER.GetImageN("io", 180), _("Go to headquarters"));
		group->AddImageButton(4, 135, 45,  45, 36, TC_GREY, LOADER.GetImageN("io", 107), _("Notify allies of this location"));
    }

    main_tab->SetSelection(0, true);

    if(x_ + GetWidth() > VIDEODRIVER.GetScreenWidth())
        x_ = mouse_x - GetWidth() - 40;
    if(y_ + GetHeight() > VIDEODRIVER.GetScreenHeight())
        y_ = mouse_y - GetHeight() - 40;

    VIDEODRIVER.SetMousePos(GetX() + 20, GetY() + 75);
}

void iwAction::AddUpgradeRoad(ctrlGroup* group, unsigned int&  /*x*/, unsigned int& width)
{
    RTTR_Assert(group);

    if(GAMECLIENT.GetGGS().isEnabled(AddonId::MANUAL_ROAD_ENLARGEMENT))
    {
        unsigned char flag_dir = 0;
        noFlag* flag = gwv.GetViewer().GetRoadFlag(selectedPt, flag_dir);
        if(flag && flag->routes[flag_dir]->GetRoadType() == RoadSegment::RT_NORMAL)
        {
            width = 90;
            group->AddImageButton(2, 90, 45, width, 36, TC_GREY, LOADER.GetImageN("io", 44), _("Upgrade to donkey road"));
        }
    }
}

void iwAction::DoUpgradeRoad()
{
    unsigned char flag_dir = 0;
    noFlag* flag = gwv.GetViewer().GetRoadFlag(selectedPt, flag_dir);
    if(flag)
        GAMECLIENT.UpgradeRoad(flag->GetPos(), flag_dir);
}

/// Fügt Angriffs-Steuerelemente für bestimmte Gruppe hinzu
void iwAction::AddAttackControls(ctrlGroup* group, const unsigned attackers_count)
{
    // Verfügbare Soldatenzahl steht in params, wenns keine gibt, einfach Meldung anzeigen: "Angriff nicht möglich!"
    if(attackers_count == 0)
    {
        // Angriff nicht  möglich!
        group->AddText(1, 90, 56, _("Attack not possible."), COLOR_YELLOW, glArchivItem_Font::DF_CENTER, NormalFont);
    }
    else
    {
        selected_soldiers_count = 1;

        // Minus und Plus - Button
        group->AddImageButton(1, 3, 49, 26, 32, TC_GREY, LOADER.GetImageN("io", 139), _("Less attackers"));
        group->AddImageButton(2, 89, 49, 26, 32, TC_GREY, LOADER.GetImageN("io", 138), _("More attackers"));

        // Starke/Schwache Soldaten
        ctrlOptionGroup* ogroup = group->AddOptionGroup(3, ctrlOptionGroup::ILLUMINATE);
        ogroup->AddImageButton(0, 146, 49, 30, 33, TC_GREY, LOADER.GetImageN("io", 31), _("Weak attackers"));
        ogroup->AddImageButton(1, 117, 49, 30, 33, TC_GREY, LOADER.GetImageN("io", 30), _("Strong attackers"));
        // standardmäßig starke Soldaten
        ogroup->SetSelection(1);

        // Schnellauswahl-Buttons
        unsigned int buttons_count = (attackers_count > 3) ? 4 : attackers_count;
        unsigned short button_width = 112 / buttons_count;

        for(unsigned i = 0; i < buttons_count; ++i)
            group->AddImageButton(10 + i, 3 + i * button_width, 83, button_width, 32, TC_GREY, LOADER.GetImageN("io", 204 + i), _("Number of attackers"));

        // Angriffsbutton
        group->AddImageButton(4, 117, 83, 59, 32, TC_RED1, LOADER.GetImageN("io", 25), _("Attack!"));
    }
}


iwAction::~iwAction()
{
    VIDEODRIVER.SetMousePos(mousePosAtOpen_.x, mousePosAtOpen_.y);
    gi.GI_WindowClosed(this);
}

void iwAction::Msg_Group_ButtonClick(const unsigned int  /*group_id*/, const unsigned int ctrl_id)
{
    switch(GetCtrl<ctrlTab>(0)->GetCurrentTab())
    {
        default:
            break;

        case TAB_ATTACK:
        {
            Msg_ButtonClick_TabAttack(ctrl_id);
        } break;
        case TAB_SEAATTACK:
        {
            Msg_ButtonClick_TabSeaAttack(ctrl_id);
        } break;


        case TAB_FLAG:
        {
            Msg_ButtonClick_TabFlag(ctrl_id);
        } break;

        case TAB_BUILD:
        {
            Msg_ButtonClick_TabBuild(ctrl_id);
        } break;

        case TAB_SETFLAG:
        {
            Msg_ButtonClick_TabSetFlag(ctrl_id);
        } break;

        case TAB_CUTROAD:
        {
            Msg_ButtonClick_TabCutRoad(ctrl_id);
        } break;

        case TAB_WATCH:
        {
            Msg_ButtonClick_TabWatch(ctrl_id);
        } break;
    }
}


void iwAction::Msg_TabChange(const unsigned int ctrl_id, const unsigned short tab_id)
{
    switch(ctrl_id)
    {
        case 0: // Haupttabs
        {
            unsigned short height = 0;
            switch(tab_id)
            {
                case TAB_FLAG:    height = 138; break;
                case TAB_CUTROAD: height = 138; break;
                case TAB_BUILD:
                {
                    // unterschiedliche Höhe, je nachdem welches Gebäude
                    switch(GetCtrl<ctrlTab>(0)->GetGroup(TAB_BUILD)->GetCtrl<ctrlTab>(1)->GetCurrentTab())
                    {
                        case Tabs::BT_HUT:    height = 222; break;
                        case Tabs::BT_HOUSE:  height = 258; break;
                        case Tabs::BT_CASTLE: height = 186; break;
                        case Tabs::BT_MINE:   height = 186; break;
                    }
                } break;
                case TAB_SETFLAG: height = 138; break;
                case TAB_ATTACK:
                {
                    if(available_soldiers_count > 0)
                        height = 178;
                    else
                        height = 130;
                } break;
                case TAB_SEAATTACK:
                {
                    if(available_soldiers_count_sea > 0)
                        height = 178;
                    else
                        height = 130;
                } break;
                case TAB_WATCH:   height = 138; break;
            }


            SetIwHeight(height);
        } break;
    }

}

void iwAction::Msg_Group_TabChange(const unsigned  /*group_id*/, const unsigned int ctrl_id, const unsigned short tab_id)
{
    switch(ctrl_id)
    {
        case 1: // Gebäudetabs
        {
            SetIwHeight(building_tab_heights[tab_id]);
        } break;
    }
}


void iwAction::Msg_PaintAfter()
{
    ctrlTab* tab = GetCtrl<ctrlTab>(0);
    if(tab)
    {
        // Anzeige Soldaten/mögliche Soldatenanzahl bei Angriffstab
        if(tab->GetCurrentTab() == TAB_ATTACK && available_soldiers_count > 0)
        {
            char str[32];
            sprintf(str, "%u/%u", selected_soldiers_count, available_soldiers_count);
            LargeFont->Draw(GetX() + 67, GetY() + 79, str, glArchivItem_Font::DF_CENTER, COLOR_YELLOW);
        }
        else if(tab->GetCurrentTab() == TAB_SEAATTACK && available_soldiers_count_sea > 0)
        {
            char str[32];
            sprintf(str, "%u/%u", selected_soldiers_count_sea, available_soldiers_count_sea);
            LargeFont->Draw(GetX() + 67, GetY() + 79, str, glArchivItem_Font::DF_CENTER, COLOR_YELLOW);
        }
    }
}



void iwAction::Msg_ButtonClick_TabAttack(const unsigned int ctrl_id)
{
    switch(ctrl_id)
    {
        case 1: // 1 Soldat weniger
        {
            if(selected_soldiers_count > 1)
                --selected_soldiers_count;
        } break;
        case 2: // 1 Soldat mehr
        {
            if(selected_soldiers_count < available_soldiers_count)
                ++selected_soldiers_count;
        } break;
        case 10: // auf bestimmte Anzahl setzen
        case 11:
        case 12:
        case 13:
        {
            if(available_soldiers_count > 4)
                selected_soldiers_count = (ctrl_id - 9) * available_soldiers_count / 4;
            else
                selected_soldiers_count = ctrl_id - 9;
        } break;
        case 4: // Angriff!
        {
            ctrlOptionGroup* ogroup = GetCtrl<ctrlTab>(0)->GetGroup(TAB_ATTACK)->GetCtrl<ctrlOptionGroup>(3);

            GAMECLIENT.Attack(selectedPt, selected_soldiers_count, (ogroup->GetSelection() == 1));

            Close();
        } break;
    }
}


void iwAction::Msg_ButtonClick_TabSeaAttack(const unsigned int ctrl_id)
{
    switch(ctrl_id)
    {
        case 1: // 1 Soldat weniger
        {
            if(selected_soldiers_count_sea > 1)
                --selected_soldiers_count_sea;
        } break;
        case 2: // 1 Soldat mehr
        {
            if(selected_soldiers_count_sea < available_soldiers_count_sea)
                ++selected_soldiers_count_sea;
        } break;
        case 10: // auf bestimmte Anzahl setzen
        case 11:
        case 12:
        case 13:
        {
            if(available_soldiers_count_sea > 4)
                selected_soldiers_count_sea = (ctrl_id - 9) * available_soldiers_count_sea / 4;
            else
                selected_soldiers_count_sea = ctrl_id - 9;
        } break;
        case 4: // Angriff!
        {
            ctrlOptionGroup* ogroup = GetCtrl<ctrlTab>(0)->GetGroup(TAB_SEAATTACK)->GetCtrl<ctrlOptionGroup>(3);

            GAMECLIENT.SeaAttack(selectedPt, selected_soldiers_count_sea, (ogroup->GetSelection() == 1));

            Close();
        } break;
    }
}


void iwAction::Msg_ButtonClick_TabFlag(const unsigned int ctrl_id)
{
    switch(ctrl_id)
    {
        case 1: // Straße bauen
        {
            gi.GI_SetRoadBuildMode(RM_NORMAL);
            Close();
        } break;
        case 2: // Wasserstraße bauen
        {
            gi.GI_SetRoadBuildMode(RM_BOAT);
            Close();
        } break;
        case 3: // Flagge abreißen
        {
            NodalObjectType nop = (gwv.GetViewer().GetNO(gwv.GetViewer().GetNeighbour(selectedPt, 1)))->GetType() ;
            // Haben wir ne Baustelle/Gebäude dran?
            if(nop == NOP_BUILDING || nop == NOP_BUILDINGSITE)
            {
                // Abreißen?
                Close();
                noBaseBuilding* building = gwv.GetViewer().GetSpecObj<noBaseBuilding>(gwv.GetViewer().GetNeighbour(selectedPt, 1));

                // Militärgebäude?
                if(building->GetGOT() == GOT_NOB_MILITARY)
                {
                    // Darf das Gebäude abgerissen werden?
                    if(!static_cast<nobMilitary*>(building)->IsDemolitionAllowed())
                    {
                        // Nein, dann Messagebox anzeigen
                        iwMilitaryBuilding::DemolitionNotAllowed();
                        break;
                    }

                }

                WINDOWMANAGER.Show(new iwDemolishBuilding(gwv, building, true));
            }
            else
            {
                GAMECLIENT.DestroyFlag(selectedPt);
                Close();
            }
        } break;
        case 4: // Geologen rufen
        {
            GAMECLIENT.CallGeologist(selectedPt);
            Close();
        } break;
        case 5: // Späher rufen
        {
            GAMECLIENT.CallScout(selectedPt);
            Close();
        } break;
    }
}

void iwAction::Msg_ButtonClick_TabBuild(const unsigned int ctrl_id)
{
    // Klick auf Gebäudebauicon
    GAMECLIENT.SetBuildingSite(selectedPt,
                     GetCtrl<ctrlTab>(0)->GetGroup(TAB_BUILD)->GetCtrl<ctrlTab>(1)->GetCurrentGroup()->
                     GetCtrl<ctrlBuildingIcon>(ctrl_id)->GetType());

    // Fenster schließen
    Close();
}

void iwAction::Msg_ButtonClick_TabSetFlag(const unsigned int ctrl_id)
{
    switch(ctrl_id)
    {
        case 1: // Flagge setzen
        {
            GAMECLIENT.SetFlag(selectedPt);
        } break;
        case 2: // Weg aufwerten
        {
            DoUpgradeRoad();
        } break;
    }

    Close();
}

void iwAction::Msg_ButtonClick_TabCutRoad(const unsigned int ctrl_id)
{
    switch(ctrl_id)
    {
        case 1: // Straße abreißen
        {
            unsigned char flag_dir = 0;
            noFlag* flag = gwv.GetViewer().GetRoadFlag(selectedPt, flag_dir);
            if(flag)
                GAMECLIENT.DestroyRoad(flag->GetPos(), flag_dir);
        } break;
        case 2: // Straße aufwerten
        {
            DoUpgradeRoad();
        } break;
    }

    Close();
}

void iwAction::Msg_ButtonClick_TabWatch(const unsigned int ctrl_id)
{
    switch(ctrl_id)
    {
        case 1:
            // TODO: bestimen, was an der position selected ist
            WINDOWMANAGER.Show(new iwObservate(gwv, selectedPt));
            break;
        case 2: // Häusernamen/Prozent anmachen
            gwv.ToggleShowNamesAndProductivity();
            break;
        case 3: // zum HQ
            gwv.MoveToMapPt(GAMECLIENT.GetLocalPlayer().hqPos);
            break;
		case 4:
			GAMECLIENT.NotifyAlliesOfLocation(selectedPt);
            break;
    }

    Close();
}
