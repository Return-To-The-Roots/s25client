// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "iwAction.h"
#include "Cheats.h"
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
#include <boost/format.hpp>
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
    TAB_SEAATTACK,
    TAB_CHEAT
};

iwAction::iwAction(GameInterface& gi, GameWorldView& gwv, const Tabs& tabs, MapPoint selectedPt,
                   const DrawPoint& mousePos, Params params, bool military_buildings)
    : IngameWindow(CGI_ACTION, mousePos, Extent(200, 254), _("Activity window"), LOADER.getImageN("io", 1)), gi(gi),
      gwv(gwv), selectedPt(selectedPt), mousePosAtOpen_(mousePos), activeHoveredIcon_(nullptr)
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

        TAB_CHEAT   1 = Place cheat building
    */

    const GamePlayer& player = gwv.getViewer().getPlayer();

    /// Haupttab
    ctrlTab* main_tab = addTabCtrl(0, DrawPoint(10, 20), 180);

    // Bau-main_tab
    if(tabs.build)
    {
        ctrlGroup* group = main_tab->addTab(LOADER.getImageN("io", 18), _("-> Build house"), TAB_BUILD);

        ctrlTab* build_tab = group->addTabCtrl(1, DrawPoint(0, 45), 180);

        // Building tabs
        if(tabs.build_tabs == BuildTab::Mine) // mines
            build_tab->addTab(LOADER.getImageN("io", 76), _("-> Dig mines"), int(BuildTab::Mine));
        else
        {
            build_tab->addTab(LOADER.getImageN("io", 67), _("-> Build hut"), int(BuildTab::Hut));
            if(tabs.build_tabs >= BuildTab::House)
                build_tab->addTab(LOADER.getImageN("io", 68), _("-> Build house"), int(BuildTab::House));
            if(tabs.build_tabs >= BuildTab::Castle) // castle & harbor
                build_tab->addTab(LOADER.getImageN("io", 69), _("-> Build castle"), int(BuildTab::Castle));
        }

        // add building icons to TabCtrl. Note: None for BuildTab::Harbor (last)
        const helpers::EnumArray<std::vector<BuildingType>, BuildTab> building_icons = {
          // Linebreak
          {{BuildingType::Woodcutter, BuildingType::Forester, BuildingType::Quarry, BuildingType::Fishery,
            BuildingType::Hunter, BuildingType::Barracks, BuildingType::Guardhouse, BuildingType::LookoutTower,
            BuildingType::Well, BuildingType::Skinner},
           {BuildingType::Sawmill, BuildingType::Slaughterhouse, BuildingType::Mill, BuildingType::Bakery,
            BuildingType::Ironsmelter, BuildingType::Metalworks, BuildingType::Armory, BuildingType::Mint,
            BuildingType::Shipyard, BuildingType::Brewery, BuildingType::Winery, BuildingType::Tannery,
            BuildingType::LeatherWorks, BuildingType::Storehouse, BuildingType::Watchtower, BuildingType::Catapult},
           {BuildingType::Farm, BuildingType::PigFarm, BuildingType::DonkeyBreeder, BuildingType::Charburner,
            BuildingType::Vineyard, BuildingType::Temple, BuildingType::Fortress, BuildingType::HarborBuilding},
           {BuildingType::GoldMine, BuildingType::IronMine, BuildingType::CoalMine, BuildingType::GraniteMine}}};

        /// Flexible what-buildings-are-available handling
        helpers::EnumArray<bool, BuildingType> buildingAvailable;

        // First enable all buildings
        for(const auto bld : helpers::enumRange<BuildingType>())
            buildingAvailable[bld] = player.isBuildingEnabled(bld);

        // Now deactivate those we don't want

        if(tabs.build_tabs != BuildTab::Harbor)
            buildingAvailable[BuildingType::HarborBuilding] = false;

        if(!military_buildings)
        {
            buildingAvailable[BuildingType::Barracks] = false;
            buildingAvailable[BuildingType::Guardhouse] = false;
            buildingAvailable[BuildingType::Watchtower] = false;
            buildingAvailable[BuildingType::Fortress] = false;
        }

        if(gwv.getWorld().getGGS().isEnabled(AddonId::CHANGE_GOLD_DEPOSITS))
        {
            buildingAvailable[BuildingType::GoldMine] = false;
            buildingAvailable[BuildingType::Mint] = false;
        }

        if(!player.canBuildCatapult())
            buildingAvailable[BuildingType::Catapult] = false;

        if(!gwv.getWorld().getGGS().isEnabled(AddonId::CHARBURNER))
            buildingAvailable[BuildingType::Charburner] = false;

        if(!gwv.getWorld().getGGS().isEnabled(AddonId::WINE))
        {
            buildingAvailable[BuildingType::Vineyard] = false;
            buildingAvailable[BuildingType::Winery] = false;
            buildingAvailable[BuildingType::Temple] = false;
        }

        if(!gwv.getWorld().getGGS().isEnabled(AddonId::LEATHER))
        {
            buildingAvailable[BuildingType::Skinner] = false;
            buildingAvailable[BuildingType::Tannery] = false;
            buildingAvailable[BuildingType::LeatherWorks] = false;
        }

        const bool showBuildingRadius = gwv.getWorld().getGGS().isEnabled(AddonId::BUILDING_RADIUS);
        constexpr helpers::EnumArray<unsigned, BuildTab> NUM_TABS = {1, 2, 3, 1, 3};

        for(unsigned char i = 0; i < NUM_TABS[tabs.build_tabs]; ++i)
        {
            unsigned char k = 0;
            const BuildTab bt = (tabs.build_tabs == BuildTab::Mine) ? BuildTab::Mine : BuildTab(i);

            for(const BuildingType bld : building_icons[bt])
            {
                if(!buildingAvailable[bld])
                    continue;

                // Baukosten im Tooltip mit anzeigen
                std::stringstream tooltip;
                tooltip << _(BUILDING_NAMES[bld]);

                // Show radius if any
                const unsigned radius = showBuildingRadius ? gwv.getBuildingRadius(bld) : 0;
                if(radius > 0)
                    tooltip << boost::format(_("\nRange: %1% tiles")) % radius;

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
                ctrlBuildingIcon* icon = build_tab->getGroup(static_cast<int>(bt))
                                           ->addBuildingIcon(k, iconPos, bld, player.nation, 36, tooltip.str());

                if(radius > 0)
                {
                    icon->setOnHoverChanged([this, icon, radius](bool hovered) noexcept {
                        if(hovered && !shouldBeClosed())
                        {
                            this->activeHoveredIcon_ = icon;
                            this->gwv.showRadiusPreview(this->selectedPt, radius);
                        } else if(this->activeHoveredIcon_ == icon)
                        {
                            this->activeHoveredIcon_ = nullptr;
                            this->gwv.hideRadiusPreview();
                        }
                        // else: stale leave from a previously-hovered icon, ignore
                    });
                }

                ++k;
            }

            building_tab_heights[static_cast<int>(bt)] = (k / 5 + ((k % 5 != 0) ? 1 : 0)) * 36 + 150;
        }

        build_tab->setSelection(0, true);
    }

    // Wenn es einen Flaggen-main_tab gibt, dann entsprechend die Buttons anordnen, wie sie gebraucht werden
    if(tabs.flag)
    {
        ctrlGroup* group = main_tab->addTab(LOADER.getImageN("io", 70), _("Erect flag"), TAB_FLAG);

        switch(get<FlagType>(params))
        {
            case FlagType::Normal:
            {
                group->addImageButton(1, DrawPoint(0, 45), Extent(45, 36), TextureColor::Grey,
                                      LOADER.getImageN("io", 65), _("Build road"));
                group->addImageButton(3, DrawPoint(45, 45), Extent(45, 36), TextureColor::Grey,
                                      LOADER.getImageN("io", 118), _("Pull down flag"));
                group->addImageButton(4, DrawPoint(90, 45), Extent(45, 36), TextureColor::Grey,
                                      LOADER.getImageN("io", 20), _("Call in geologist"));
                group->addImageButton(5, DrawPoint(135, 45), Extent(45, 36), TextureColor::Grey,
                                      LOADER.getImageN("io", 96), _("Send out scout"));
            }
            break;
            case FlagType::WaterFlag:
            {
                group->addImageButton(1, DrawPoint(0, 45), Extent(36, 36), TextureColor::Grey,
                                      LOADER.getImageN("io", 65), _("Build road"));
                group->addImageButton(2, DrawPoint(36, 45), Extent(36, 36), TextureColor::Grey,
                                      LOADER.getImageN("io", 95), _("Build waterway"));
                group->addImageButton(3, DrawPoint(72, 45), Extent(36, 36), TextureColor::Grey,
                                      LOADER.getImageN("io", 118), _("Pull down flag"));
                group->addImageButton(4, DrawPoint(108, 45), Extent(36, 36), TextureColor::Grey,
                                      LOADER.getImageN("io", 20), _("Call in geologist"));
                group->addImageButton(5, DrawPoint(144, 45), Extent(36, 36), TextureColor::Grey,
                                      LOADER.getImageN("io", 96), _("Send out scout"));
            }
            break;
            case FlagType::HQ:
            {
                group->addImageButton(1, DrawPoint(0, 45), Extent(180, 36), TextureColor::Grey,
                                      LOADER.getImageN("io", 65), _("Build road"));
            }
            break;
            case FlagType::Storehouse:
            {
                group->addImageButton(1, DrawPoint(0, 45), Extent(90, 36), TextureColor::Grey,
                                      LOADER.getImageN("io", 65), _("Build road"));
                group->addImageButton(3, DrawPoint(90, 45), Extent(90, 36), TextureColor::Grey,
                                      LOADER.getImageN("io", 118), _("Demolish house"));
            }
            break;
        }
    }

    // Flagge Setzen-main_tab
    if(tabs.setflag)
    {
        ctrlGroup* group = main_tab->addTab(LOADER.getImageN("io", 45), _("Erect flag"), TAB_SETFLAG);

        unsigned nr = 70;
        if(get<FlagType>(params) == FlagType::WaterFlag)
            nr = 94;

        // Straße aufwerten ggf anzeigen
        Extent btSize(180, 36);
        unsigned btPosX = 90;
        addUpgradeRoad(group, btPosX, btSize.x);

        group->addImageButton(1, DrawPoint(0, 45), btSize, TextureColor::Grey, LOADER.getImageN("io", nr),
                              _("Erect flag"));
    }

    // Cut-main_tab
    if(tabs.cutroad)
    {
        ctrlGroup* group = main_tab->addTab(LOADER.getImageN("io", 19), _("Dig up road"), TAB_CUTROAD);

        // Straße aufwerten ggf anzeigen
        Extent btSize(180, 36);
        unsigned btPosX = 0;
        if(tabs.upgradeRoad)
            addUpgradeRoad(group, btPosX, btSize.x);

        group->addImageButton(1, DrawPoint(btPosX, 45), btSize, TextureColor::Grey, LOADER.getImageN("io", 32),
                              _("Dig up road"));
    }

    if(tabs.attack)
    {
        ctrlGroup* group = main_tab->addTab(LOADER.getImageN("io", 98), _("Attack options"), TAB_ATTACK);
        available_soldiers_count = get<SoldierCount>(params);
        addAttackControls(group, available_soldiers_count);
        selected_soldiers_count = 1;
    }

    if(tabs.sea_attack)
    {
        ctrlGroup* group = main_tab->addTab(LOADER.getImageN("io", 177), _("Attack options"), TAB_SEAATTACK);

        selected_soldiers_count_sea = 1;
        available_soldiers_count_sea = gwv.getViewer().getNumSoldiersForSeaAttack(selectedPt);

        addAttackControls(group, available_soldiers_count_sea);
    }

    // Beobachten-main_tab
    if(tabs.watch)
    {
        if(gi.GI_GetCheats().canPlaceCheatBuilding(selectedPt))
        {
            constexpr auto buildImgId = 18;
            auto* buildImg = LOADER.getImageN("io", buildImgId);
            ctrlGroup* group = main_tab->addTab(buildImg, _("Build headquarters"), TAB_CHEAT);
            group->addImageButton(1, DrawPoint{0, 45}, Extent{180, 36}, TextureColor::Grey, buildImg,
                                  _("Build headquarters"));
        }

        ctrlGroup* group = main_tab->addTab(LOADER.getImageN("io", 36), _("Display options"), TAB_WATCH);
        const Extent btSize(45, 36);
        DrawPoint curPos(0, 45);
        group->addImageButton(1, curPos, btSize, TextureColor::Grey, LOADER.getImageN("io", 108),
                              _("Observation window"));
        curPos.x += btSize.x;
        group->addImageButton(2, curPos, btSize, TextureColor::Grey, LOADER.getImageN("io", 179), _("House names"));
        curPos.x += btSize.x;
        group->addImageButton(3, curPos, btSize, TextureColor::Grey, LOADER.getImageN("io", 180),
                              _("Go to headquarters"));
        curPos.x += btSize.x;
        group->addImageButton(4, curPos, btSize, TextureColor::Grey, LOADER.getImageN("io", 107),
                              _("Notify allies of this location"));
    }

    main_tab->setSelection(0, true);

    DrawPoint adjPos = getPos();
    DrawPoint outerPt = getPos() + getSize();
    if(outerPt.x > static_cast<int>(VIDEODRIVER.getRenderSize().x))
        adjPos.x = mousePos.x - getSize().x - 40;
    if(outerPt.y > static_cast<int>(VIDEODRIVER.getRenderSize().y))
        adjPos.y = mousePos.y - getSize().y - 40;
    if(adjPos != getPos())
        setPos(adjPos);

    VIDEODRIVER.setMousePos(getDrawPos() + DrawPoint(20, 75));
}

void iwAction::addUpgradeRoad(ctrlGroup* group, unsigned& /*x*/, unsigned& width)
{
    RTTR_Assert(group);

    if(gwv.getWorld().getGGS().isEnabled(AddonId::MANUAL_ROAD_ENLARGEMENT))
    {
        width = 90;
        group->addImageButton(2, DrawPoint(90, 45), Extent(width, 36), TextureColor::Grey, LOADER.getImageN("io", 44),
                              _("Upgrade to donkey road"));
    }
}

bool iwAction::doUpgradeRoad()
{
    Direction flagDir;
    const noFlag* flag = gwv.getWorld().getRoadFlag(selectedPt, flagDir);
    if(flag)
        return GAMECLIENT.UpgradeRoad(flag->getPos(), flagDir);
    else
        return true;
}

/// Fügt Angriffs-Steuerelemente für bestimmte Gruppe hinzu
void iwAction::addAttackControls(ctrlGroup* group, const unsigned attackersCount)
{
    // Verfügbare Soldatenzahl steht in params, wenns keine gibt, einfach Meldung anzeigen: "Angriff nicht möglich!"
    if(attackersCount == 0)
    {
        // Angriff nicht  möglich!
        group->addText(1, DrawPoint(90, 56), _("Attack not possible."), COLOR_YELLOW, FontStyle::CENTER, NormalFont);
    } else
    {
        selected_soldiers_count = 1;

        // Minus und Plus - Button
        group->addImageButton(1, DrawPoint(3, 49), Extent(26, 32), TextureColor::Grey, LOADER.getImageN("io", 139),
                              _("Less attackers"));
        group->addImageButton(2, DrawPoint(89, 49), Extent(26, 32), TextureColor::Grey, LOADER.getImageN("io", 138),
                              _("More attackers"));

        // Starke/Schwache Soldaten
        ctrlOptionGroup* ogroup = group->addOptionGroup(3, GroupSelectType::Illuminate);
        ogroup->addImageButton(0, DrawPoint(146, 49), Extent(30, 33), TextureColor::Grey, LOADER.getImageN("io", 31),
                               _("Weak attackers"));
        ogroup->addImageButton(1, DrawPoint(117, 49), Extent(30, 33), TextureColor::Grey, LOADER.getImageN("io", 30),
                               _("Strong attackers"));
        // standardmäßig starke Soldaten
        ogroup->setSelection(1);

        // Schnellauswahl-Buttons
        unsigned buttonsCount = (attackersCount > 3) ? 4 : attackersCount;
        unsigned short buttonWidth = 112 / buttonsCount;

        for(unsigned i = 0; i < buttonsCount; ++i)
            group->addImageButton(10 + i, DrawPoint(3 + i * buttonWidth, 83), Extent(buttonWidth, 32),
                                  TextureColor::Grey, LOADER.getImageN("io", 204 + i), _("Number of attackers"));

        // Angriffsbutton
        group->addImageButton(4, DrawPoint(117, 83), Extent(59, 32), TextureColor::Red1, LOADER.getImageN("io", 25),
                              _("Attack!"));
    }
}

void iwAction::close()
{
    if(shouldBeClosed())
        return;
    activeHoveredIcon_ = nullptr;
    gwv.hideRadiusPreview();
    IngameWindow::close();
    if(mousePosAtOpen_.isValid())
        VIDEODRIVER.setMousePos(mousePosAtOpen_);
}

void iwAction::msgGroupButtonClick(const unsigned /*group_id*/, const unsigned ctrl_id)
{
    switch(getCtrl<ctrlTab>(0)->getCurrentTab())
    {
        default: break;

        case TAB_ATTACK:
        {
            msgButtonClickTabAttack(ctrl_id);
        }
        break;
        case TAB_SEAATTACK:
        {
            msgButtonClickTabSeaAttack(ctrl_id);
        }
        break;

        case TAB_FLAG:
        {
            msgButtonClickTabFlag(ctrl_id);
        }
        break;

        case TAB_BUILD:
        {
            msgButtonClickTabBuild(ctrl_id);
        }
        break;

        case TAB_SETFLAG:
        {
            msgButtonClickTabSetFlag(ctrl_id);
        }
        break;

        case TAB_CUTROAD:
        {
            msgButtonClickTabCutRoad(ctrl_id);
        }
        break;

        case TAB_WATCH:
        {
            msgButtonClickTabWatch(ctrl_id);
        }
        break;

        case TAB_CHEAT:
        {
            msgButtonClickTabCheat(ctrl_id);
        }
        break;
    }
}

void iwAction::msgTabChange(const unsigned ctrl_id, const unsigned short tabId)
{
    switch(ctrl_id)
    {
        case 0: // Haupttabs
        {
            unsigned short height = 0;
            switch(tabId)
            {
                case TAB_FLAG:
                case TAB_CUTROAD:
                case TAB_SETFLAG:
                case TAB_CHEAT:
                case TAB_WATCH: height = 138; break;
                case TAB_BUILD:
                {
                    height = building_tab_heights
                      [getCtrl<ctrlTab>(0)->getGroup(TAB_BUILD)->getCtrl<ctrlTab>(1)->getCurrentTab()];
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

            setHeight(height);
        }
        break;
    }
}

void iwAction::msgGroupTabChange(const unsigned /*group_id*/, const unsigned ctrl_id, const unsigned short tabId)
{
    switch(ctrl_id)
    {
        case 1: // Gebäudetabs
        {
            setHeight(building_tab_heights[tabId]);
        }
        break;
    }
}

void iwAction::msgPaintAfter()
{
    IngameWindow::msgPaintAfter();

    auto* tab = getCtrl<ctrlTab>(0);
    if(tab)
    {
        static boost::format fmt("%u/%u");
        // Anzeige Soldaten/mögliche Soldatenanzahl bei Angriffstab
        if(tab->getCurrentTab() == TAB_ATTACK && available_soldiers_count > 0)
        {
            fmt % selected_soldiers_count % available_soldiers_count;
            LargeFont->draw(getDrawPos() + DrawPoint(67, 79), fmt.str(), FontStyle::CENTER, COLOR_YELLOW);
        } else if(tab->getCurrentTab() == TAB_SEAATTACK && available_soldiers_count_sea > 0)
        {
            fmt % selected_soldiers_count_sea % available_soldiers_count_sea;
            LargeFont->draw(getDrawPos() + DrawPoint(67, 79), fmt.str(), FontStyle::CENTER, COLOR_YELLOW);
        }
    }
}

void iwAction::msgButtonClickTabAttack(const unsigned ctrl_id)
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
            auto* ogroup = getCtrl<ctrlTab>(0)->getGroup(TAB_ATTACK)->getCtrl<ctrlOptionGroup>(3);
            if(GAMECLIENT.Attack(selectedPt, selected_soldiers_count, (ogroup->getSelection() == 1)))
                close();
        }
        break;
    }
}

void iwAction::msgButtonClickTabSeaAttack(const unsigned ctrl_id)
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
            auto* ogroup = getCtrl<ctrlTab>(0)->getGroup(TAB_SEAATTACK)->getCtrl<ctrlOptionGroup>(3);
            if(GAMECLIENT.SeaAttack(selectedPt, selected_soldiers_count_sea, (ogroup->getSelection() == 1)))
                close();
        }
        break;
    }
}

void iwAction::msgButtonClickTabFlag(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case 1: // Straße bauen
        {
            gi.GI_StartRoadBuilding(selectedPt, false);
            close();
        }
        break;
        case 2: // Wasserstraße bauen
        {
            gi.GI_StartRoadBuilding(selectedPt, true);
            close();
        }
        break;
        case 3: // Flagge abreißen
        {
            const GameWorldBase& world = gwv.getWorld();
            NodalObjectType nop = (world.getNO(world.getNeighbour(selectedPt, Direction::NorthWest)))->getType();
            // Haben wir ne Baustelle/Gebäude dran?
            if(nop == NodalObjectType::Building || nop == NodalObjectType::Buildingsite)
            {
                // Abreißen?
                const auto* building =
                  world.getSpecObj<noBaseBuilding>(world.getNeighbour(selectedPt, Direction::NorthWest));

                // Militärgebäude?
                if(building->getGOT() == GO_Type::NobMilitary)
                {
                    // Darf das Gebäude abgerissen werden?
                    if(!static_cast<const nobMilitary*>(building)->isDemolitionAllowed())
                    {
                        // Nein, dann Messagebox anzeigen
                        iwMilitaryBuilding::demolitionNotAllowed(world.getGGS());
                        break;
                    }
                }

                WINDOWMANAGER.show(std::make_unique<iwDemolishBuilding>(gwv, building, true));
                disableMousePosResetOnClose();
                close();
            } else
            {
                if(GAMECLIENT.DestroyFlag(selectedPt))
                    close();
            }
        }
        break;
        case 4: // Geologen rufen
        {
            if(GAMECLIENT.CallSpecialist(selectedPt, Job::Geologist))
                close();
        }
        break;
        case 5: // Späher rufen
        {
            if(GAMECLIENT.CallSpecialist(selectedPt, Job::Scout))
                close();
        }
        break;
    }
}

void iwAction::msgButtonClickTabBuild(const unsigned ctrl_id)
{
    // Klick auf Gebäudebauicon
    if(GAMECLIENT.SetBuildingSite(selectedPt, getCtrl<ctrlTab>(0)
                                                ->getGroup(TAB_BUILD)
                                                ->getCtrl<ctrlTab>(1)
                                                ->getCurrentGroup()
                                                ->getCtrl<ctrlBuildingIcon>(ctrl_id)
                                                ->getType()))
    {
        // Fenster schließen
        close();
    }
}

void iwAction::msgButtonClickTabSetFlag(const unsigned ctrl_id)
{
    bool success = false;
    switch(ctrl_id)
    {
        case 1: // Flagge setzen
            success = GAMECLIENT.SetFlag(selectedPt);
            break;
        case 2: // Weg aufwerten
            success = doUpgradeRoad();
            break;
    }

    if(success)
        close();
}

void iwAction::msgButtonClickTabCutRoad(const unsigned ctrl_id)
{
    bool success = true;
    switch(ctrl_id)
    {
        case 1: // Straße abreißen
        {
            Direction flagDir;
            const noFlag* flag = gwv.getWorld().getRoadFlag(selectedPt, flagDir);
            if(flag)
                success = GAMECLIENT.DestroyRoad(flag->getPos(), flagDir);
        }
        break;
        case 2: // Straße aufwerten
            success = doUpgradeRoad();
            break;
    }

    if(success)
        close();
}

void iwAction::msgButtonClickTabWatch(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case 1:
            // TODO: bestimen, was an der position selected ist
            WINDOWMANAGER.show(std::make_unique<iwObservate>(gwv, selectedPt));
            disableMousePosResetOnClose();
            close();
            break;
        case 2: // Häusernamen/Prozent anmachen
            gwv.toggleShowNamesAndProductivity();
            close();
            break;
        case 3: // zum HQ
            gwv.moveToMapPt(gwv.getViewer().getPlayer().getHQPos());
            disableMousePosResetOnClose();
            close();
            break;
        case 4:
            if(GAMECLIENT.NotifyAlliesOfLocation(selectedPt))
                close();
            break;
    }
}

void iwAction::msgButtonClickTabCheat(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case 1:
            gi.GI_GetCheats().placeCheatBuilding(selectedPt, gwv.getViewer().getPlayer());
            close();
            break;
    }
}

void iwAction::disableMousePosResetOnClose()
{
    mousePosAtOpen_ = DrawPoint::invalid();
}
