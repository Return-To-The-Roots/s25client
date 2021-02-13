// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#include "GamePlayer.h"
#include "EventManager.h"
#include "FindWhConditions.h"
#include "GameInterface.h"
#include "GlobalGameSettings.h"
#include "RoadSegment.h"
#include "SerializedGameData.h"
#include "TradePathCache.h"
#include "Ware.h"
#include "addons/const_addons.h"
#include "buildings/noBuildingSite.h"
#include "buildings/nobHarborBuilding.h"
#include "buildings/nobMilitary.h"
#include "buildings/nobUsual.h"
#include "figures/nofCarrier.h"
#include "figures/nofFlagWorker.h"
#include "helpers/containerUtils.h"
#include "helpers/mathFuncs.h"
#include "lua/LuaInterfaceGame.h"
#include "notifications/ToolNote.h"
#include "pathfinding/RoadPathFinder.h"
#include "postSystem/DiplomacyPostQuestion.h"
#include "postSystem/PostManager.h"
#include "random/Random.h"
#include "variant.h"
#include "world/GameWorldGame.h"
#include "world/TradeRoute.h"
#include "nodeObjs/noFlag.h"
#include "nodeObjs/noShip.h"
#include "gameTypes/BuildingCount.h"
#include "gameTypes/GoodTypes.h"
#include "gameTypes/JobTypes.h"
#include "gameTypes/PactTypes.h"
#include "gameTypes/VisualSettings.h"
#include "gameData/BuildingConsts.h"
#include "gameData/BuildingProperties.h"
#include "gameData/GoodConsts.h"
#include "gameData/SettingTypeConv.h"
#include "gameData/ShieldConsts.h"
#include "gameData/ToolConsts.h"
#include "s25util/Log.h"
#include <limits>

GamePlayer::GamePlayer(unsigned playerId, const PlayerInfo& playerInfo, GameWorldGame& gwg)
    : GamePlayerInfo(playerId, playerInfo), gwg(gwg), hqPos(MapPoint::Invalid()), emergency(false)
{
    std::fill(building_enabled.begin(), building_enabled.end(), true);

    LoadStandardDistribution();
    useCustomBuildOrder_ = false;
    build_order = GetStandardBuildOrder();
    transportPrio = STD_TRANSPORT_PRIO;
    LoadStandardMilitarySettings();
    LoadStandardToolSettings();

    // Inventur nullen
    global_inventory.clear();

    // Statistiken mit 0en füllen
    memset(&statistic, 0, sizeof(statistic));
    memset(&statisticCurrentData, 0, sizeof(statisticCurrentData));
    memset(&statisticCurrentMerchandiseData, 0, sizeof(statisticCurrentMerchandiseData));

    RecalcDistribution();
}

void GamePlayer::LoadStandardToolSettings()
{
    // metalwork tool request

    // manually
    for(unsigned i = 0; i < NUM_TOOLS; ++i)
    {
        tools_ordered[i] = 0;
        tools_ordered_delta[i] = 0;
    }

    // percentage (tool-settings-window-slider, in 10th percent)
    toolsSettings_[0] = 1;
    toolsSettings_[1] = 4;
    toolsSettings_[2] = 2;
    toolsSettings_[3] = 5;
    toolsSettings_[4] = 7;
    toolsSettings_[5] = 1;
    toolsSettings_[6] = 3;
    toolsSettings_[7] = 1;
    toolsSettings_[8] = 2;
    toolsSettings_[9] = 1;
    toolsSettings_[10] = 2;
    toolsSettings_[11] = 1;
}

void GamePlayer::LoadStandardMilitarySettings()
{
    // military settings (military-window-slider, in 10th percent)
    militarySettings_[0] = MILITARY_SETTINGS_SCALE[0]; //-V525
    militarySettings_[1] = 3;
    militarySettings_[2] = MILITARY_SETTINGS_SCALE[2];
    militarySettings_[3] = 3;
    militarySettings_[4] = 0;
    militarySettings_[5] = 1;
    militarySettings_[6] = MILITARY_SETTINGS_SCALE[6];
    militarySettings_[7] = MILITARY_SETTINGS_SCALE[7];
}

BuildOrders GamePlayer::GetStandardBuildOrder()
{
    BuildOrders ordering;

    // Baureihenfolge füllen
    unsigned curPrio = 0;
    for(const auto bld : helpers::enumRange<BuildingType>())
    {
        if(bld == BuildingType::Headquarters || !BuildingProperties::IsValid(bld))
            continue;

        RTTR_Assert(curPrio < ordering.size());
        ordering[curPrio] = bld;
        ++curPrio;
    }
    RTTR_Assert(curPrio == ordering.size());
    return ordering;
}

void GamePlayer::LoadStandardDistribution()
{
    // Verteilung mit Standardwerten füllen bei Waren mit nur einem Ziel (wie z.B. Mehl, Holz...)
    distribution[GoodType::Flour].client_buildings.push_back(BuildingType::Bakery);
    distribution[GoodType::Gold].client_buildings.push_back(BuildingType::Mint);
    distribution[GoodType::IronOre].client_buildings.push_back(BuildingType::Ironsmelter);
    distribution[GoodType::Ham].client_buildings.push_back(BuildingType::Slaughterhouse);
    distribution[GoodType::Stones].client_buildings.push_back(
      BuildingType::Headquarters); // BuildingType::Headquarters = Baustellen!
    distribution[GoodType::Stones].client_buildings.push_back(BuildingType::Catapult);

    // Waren mit mehreren möglichen Zielen erstmal nullen, kann dann im Fenster eingestellt werden
    for(const auto i : helpers::enumRange<GoodType>())
    {
        std::fill(distribution[i].percent_buildings.begin(), distribution[i].percent_buildings.end(), 0);
        distribution[i].selected_goal = 0;
    }

    // Standardverteilung der Waren
    for(const DistributionMapping& mapping : distributionMap)
    {
        distribution[std::get<0>(mapping)].percent_buildings[std::get<1>(mapping)] = std::get<2>(mapping);
    }
}

GamePlayer::~GamePlayer() = default;

void GamePlayer::Serialize(SerializedGameData& sgd) const
{
    // PlayerStatus speichern, ehemalig
    sgd.PushEnum<uint8_t>(ps);

    // Nur richtige Spieler serialisieren
    if(!(ps == PlayerState::Occupied || ps == PlayerState::AI))
        return;

    sgd.PushBool(isDefeated);

    buildings.Serialize(sgd);

    sgd.PushObjectContainer(roads, true);

    sgd.PushUnsignedInt(jobs_wanted.size());
    for(JobNeeded job : jobs_wanted)
    {
        sgd.PushEnum<uint8_t>(job.job);
        sgd.PushObject(job.workplace);
    }

    sgd.PushObjectContainer(ware_list, true);
    sgd.PushObjectContainer(flagworkers);
    sgd.PushObjectContainer(ships, true);

    helpers::pushContainer(sgd, shouldSendDefenderList);

    sgd.PushMapPoint(hqPos);

    for(const Distribution& dist : distribution)
    {
        for(uint8_t p : dist.percent_buildings)
            sgd.PushUnsignedChar(p);
        sgd.PushUnsignedInt(dist.client_buildings.size());
        for(BuildingType bld : dist.client_buildings)
            sgd.PushEnum<uint8_t>(bld);
        sgd.PushUnsignedInt(unsigned(dist.goals.size()));
        for(BuildingType goal : dist.goals)
            sgd.PushEnum<uint8_t>(goal);
        sgd.PushUnsignedInt(dist.selected_goal);
    }

    sgd.PushBool(useCustomBuildOrder_);

    for(BuildingType i : build_order)
        sgd.PushEnum<uint8_t>(i);

    sgd.PushRawData(transportPrio.data(), transportPrio.size());

    for(unsigned char militarySetting : militarySettings_)
        sgd.PushUnsignedChar(militarySetting);

    for(unsigned char toolsSetting : toolsSettings_)
        sgd.PushUnsignedChar(toolsSetting);

    // qx:tools
    for(unsigned i = 0; i < NUM_TOOLS; ++i)
        sgd.PushUnsignedChar(tools_ordered[i]);

    for(const auto i : helpers::enumRange<GoodType>())
        sgd.PushUnsignedInt(global_inventory[i]);
    for(const auto i : helpers::enumRange<Job>())
        sgd.PushUnsignedInt(global_inventory[i]);

    // für Statistik
    for(const auto i : helpers::enumRange<StatisticTime>())
    {
        // normale Statistik
        for(const auto j : helpers::enumRange<StatisticType>())
            for(unsigned k = 0; k < NUM_STAT_STEPS; ++k)
                sgd.PushUnsignedInt(statistic[i].data[j][k]);

        // Warenstatistik
        for(unsigned j = 0; j < NUM_STAT_MERCHANDISE_TYPES; ++j)
            for(unsigned k = 0; k < NUM_STAT_STEPS; ++k)
                sgd.PushUnsignedShort(statistic[i].merchandiseData[j][k]);

        sgd.PushUnsignedShort(statistic[i].currentIndex);
        sgd.PushUnsignedShort(statistic[i].counter);
    }
    for(const auto i : helpers::enumRange<StatisticType>())
        sgd.PushUnsignedInt(statisticCurrentData[i]);

    for(unsigned i = 0; i < NUM_STAT_MERCHANDISE_TYPES; ++i)
        sgd.PushUnsignedShort(statisticCurrentMerchandiseData[i]);

    // Serialize Pacts:
    for(unsigned i = 0; i < MAX_PLAYERS; ++i)
    {
        for(const auto u : helpers::enumRange<PactType>())
            pacts[i][u].Serialize(sgd);
    }

    sgd.PushBool(emergency);
}

void GamePlayer::Deserialize(SerializedGameData& sgd)
{
    std::fill(building_enabled.begin(), building_enabled.end(), true);

    // Ehemaligen PS auslesen
    auto origin_ps = sgd.Pop<PlayerState>();
    // Nur richtige Spieler serialisieren
    if(!(origin_ps == PlayerState::Occupied || origin_ps == PlayerState::AI))
        return;

    isDefeated = sgd.PopBool();
    buildings.Deserialize(sgd);

    sgd.PopObjectContainer(roads, GO_Type::Roadsegment);

    unsigned list_size = sgd.PopUnsignedInt();
    for(unsigned i = 0; i < list_size; ++i)
    {
        JobNeeded nj;
        nj.job = sgd.Pop<Job>();
        nj.workplace = sgd.PopObject<noRoadNode>();
        jobs_wanted.push_back(nj);
    }

    if(sgd.GetGameDataVersion() < 2)
        buildings.Deserialize2(sgd);

    sgd.PopObjectContainer(ware_list, GO_Type::Ware);
    sgd.PopObjectContainer(flagworkers);
    sgd.PopObjectContainer(ships, GO_Type::Ship);

    sgd.PopContainer(shouldSendDefenderList);

    hqPos = sgd.PopMapPoint();

    for(Distribution& dist : distribution)
    {
        for(uint8_t& p : dist.percent_buildings)
            p = sgd.PopUnsignedChar();
        dist.client_buildings.resize(sgd.PopUnsignedInt());
        for(BuildingType& bld : dist.client_buildings)
            bld = sgd.Pop<BuildingType>();
        dist.goals.resize(sgd.PopUnsignedInt());
        for(BuildingType& goal : dist.goals)
            goal = sgd.Pop<BuildingType>();
        dist.selected_goal = sgd.PopUnsignedInt();
    }

    useCustomBuildOrder_ = sgd.PopBool();

    for(auto& i : build_order)
        i = sgd.Pop<BuildingType>();

    sgd.PopRawData(transportPrio.data(), transportPrio.size());

    for(uint8_t& militarySetting : militarySettings_)
        militarySetting = sgd.PopUnsignedChar();

// False positive, see https://github.com/Return-To-The-Roots/s25client/issues/1327
#if defined(__GNUC__) && __GNUC__ == 10
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wstringop-overflow"
#endif
    for(uint8_t& toolsSetting : toolsSettings_)
        toolsSetting = sgd.PopUnsignedChar();
#if defined(__GNUC__) && __GNUC__ == 10
#    pragma GCC diagnostic pop
#endif

    // qx:tools
    for(unsigned i = 0; i < NUM_TOOLS; ++i)
        tools_ordered[i] = sgd.PopUnsignedChar();
    for(unsigned i = 0; i < NUM_TOOLS; ++i)
        tools_ordered_delta[i] = 0;

    for(const auto i : helpers::enumRange<GoodType>())
        global_inventory[i] = sgd.PopUnsignedInt();
    for(const auto i : helpers::enumRange<Job>())
        global_inventory[i] = sgd.PopUnsignedInt();

    // Visuelle Einstellungen festlegen

    // für Statistik
    for(const auto i : helpers::enumRange<StatisticTime>())
    {
        // normale Statistik
        for(const auto j : helpers::enumRange<StatisticType>())
            for(unsigned k = 0; k < NUM_STAT_STEPS; ++k)
                statistic[i].data[j][k] = sgd.PopUnsignedInt();

        // Warenstatistik
        for(unsigned j = 0; j < NUM_STAT_MERCHANDISE_TYPES; ++j)
            for(unsigned k = 0; k < NUM_STAT_STEPS; ++k)
                statistic[i].merchandiseData[j][k] = sgd.PopUnsignedShort();

        statistic[i].currentIndex = sgd.PopUnsignedShort();
        statistic[i].counter = sgd.PopUnsignedShort();
    }
    for(const auto i : helpers::enumRange<StatisticType>())
        statisticCurrentData[i] = sgd.PopUnsignedInt();

    for(unsigned i = 0; i < NUM_STAT_MERCHANDISE_TYPES; ++i)
        statisticCurrentMerchandiseData[i] = sgd.PopUnsignedShort();

    // Deserialize Pacts:
    for(unsigned i = 0; i < MAX_PLAYERS; ++i)
    {
        for(const auto u : helpers::enumRange<PactType>())
            pacts[i][u] = GamePlayer::Pact(sgd);
    }

    emergency = sgd.PopBool();
}

template<class T_IsWarehouseGood>
nobBaseWarehouse* GamePlayer::FindWarehouse(const noRoadNode& start, const T_IsWarehouseGood& isWarehouseGood,
                                            bool to_wh, bool use_boat_roads, unsigned* length,
                                            const RoadSegment* forbidden) const
{
    nobBaseWarehouse* best = nullptr;

    unsigned best_length = std::numeric_limits<unsigned>::max();

    for(nobBaseWarehouse* wh : buildings.GetStorehouses())
    {
        // Lagerhaus geeignet?
        RTTR_Assert(wh);
        if(!isWarehouseGood(*wh))
            continue;

        if(start.GetPos() == wh->GetPos())
        {
            // We are already there -> Take it
            if(length)
                *length = 0;
            return wh;
        }

        // now check if there is at least a chance that the next wh is closer than current best because pathfinding
        // takes time
        if(gwg.CalcDistance(start.GetPos(), wh->GetPos()) > best_length)
            continue;
        // Bei der erlaubten Benutzung von Bootsstraßen Waren-Pathfinding benutzen wenns zu nem Lagerhaus gehn soll
        // start <-> ziel tauschen bei der wegfindung
        unsigned tlength;
        if(gwg.GetRoadPathFinder().FindPath(to_wh ? start : *wh, to_wh ? *wh : start, use_boat_roads, best_length,
                                            forbidden, &tlength))
        {
            if(tlength < best_length || !best)
            {
                best_length = tlength;
                best = wh;
            }
        }
    }

    if(length)
        *length = best_length;

    return best;
}

void GamePlayer::AddBuildingSite(noBuildingSite* bldSite)
{
    RTTR_Assert(bldSite->GetPlayer() == GetPlayerId());
    buildings.Add(bldSite);
}

void GamePlayer::RemoveBuildingSite(noBuildingSite* bldSite)
{
    RTTR_Assert(bldSite->GetPlayer() == GetPlayerId());
    buildings.Remove(bldSite);
}

void GamePlayer::AddBuilding(noBuilding* bld, BuildingType bldType)
{
    RTTR_Assert(bld->GetPlayer() == GetPlayerId());
    buildings.Add(bld, bldType);
    ChangeStatisticValue(StatisticType::Buildings, 1);

    // Order a worker if needed
    const auto& description = BLD_WORK_DESC[bldType];
    if(description.job && description.job != Job::Private)
    {
        AddJobWanted(*description.job, bld);
    }

    if(bldType == BuildingType::HarborBuilding)
    {
        // Schiff durchgehen und denen Bescheid sagen
        for(noShip* ship : ships)
            ship->NewHarborBuilt(static_cast<nobHarborBuilding*>(bld));
    } else if(bldType == BuildingType::Headquarters)
        hqPos = bld->GetPos();
    else if(BuildingProperties::IsMilitary(bldType))
    {
        auto* milBld = static_cast<nobMilitary*>(bld);
        // New built? -> Calculate frontier distance
        if(milBld->IsNewBuilt())
            milBld->LookForEnemyBuildings();
    }
}

void GamePlayer::RemoveBuilding(noBuilding* bld, BuildingType bldType)
{
    RTTR_Assert(bld->GetPlayer() == GetPlayerId());
    buildings.Remove(bld, bldType);
    ChangeStatisticValue(StatisticType::Buildings, -1);
    if(bldType == BuildingType::HarborBuilding)
    { // Schiffen Bescheid sagen
        for(auto& ship : ships)
            ship->HarborDestroyed(static_cast<nobHarborBuilding*>(bld));
    } else if(bldType == BuildingType::Headquarters)
    {
        hqPos = MapPoint::Invalid();
        for(const noBaseBuilding* bld : buildings.GetStorehouses())
        {
            if(bld->GetBuildingType() == BuildingType::Headquarters)
            {
                hqPos = bld->GetPos();
                break;
            }
        }
    }
    if(BuildingProperties::IsWareHouse(bldType) || BuildingProperties::IsMilitary(bldType))
        TestDefeat();
}

void GamePlayer::NewRoadConnection(RoadSegment* rs)
{
    // Zu den Straßen hinzufgen, da's ja ne neue ist
    roads.push_back(rs);

    // Alle Straßen müssen nun gucken, ob sie einen Weg zu einem Warehouse finden
    FindCarrierForAllRoads();

    // Alle Straßen müssen gucken, ob sie einen Esel bekommen können
    for(RoadSegment* rs : roads)
        rs->TryGetDonkey();

    // Alle Arbeitsplätze müssen nun gucken, ob sie einen Weg zu einem Lagerhaus mit entsprechender Arbeitskraft finden
    FindWarehouseForAllJobs();

    // Alle Baustellen müssen nun gucken, ob sie ihr benötigtes Baumaterial bekommen (evtl war vorher die Straße zum
    // Lagerhaus unterbrochen
    FindMaterialForBuildingSites();

    // Alle Lost-Wares müssen gucken, ob sie ein Lagerhaus finden
    FindClientForLostWares();

    // Alle Militärgebäude müssen ihre Truppen überprüfen und können nun ggf. neue bestellen
    // und müssen prüfen, ob sie evtl Gold bekommen
    for(nobMilitary* mil : buildings.GetMilitaryBuildings())
    {
        mil->RegulateTroops();
        mil->SearchCoins();
    }
}

void GamePlayer::AddRoad(RoadSegment* rs)
{
    roads.push_back(rs);
}

void GamePlayer::DeleteRoad(RoadSegment* rs)
{
    RTTR_Assert(helpers::contains(roads, rs));
    roads.remove(rs);
}

void GamePlayer::FindClientForLostWares()
{
    // Alle Lost-Wares müssen gucken, ob sie ein Lagerhaus finden
    for(Ware* ware : ware_list)
    {
        if(ware->IsLostWare())
        {
            if(ware->FindRouteToWarehouse() && ware->IsWaitingAtFlag())
                ware->CallCarrier();
        }
    }
}

void GamePlayer::RoadDestroyed()
{
    // Alle Waren, die an Flagge liegen und in Lagerhäusern, müssen gucken, ob sie ihr Ziel noch erreichen können, jetzt
    // wo eine Straße fehlt
    for(auto it = ware_list.begin(); it != ware_list.end();)
    {
        Ware* ware = *it;
        if(ware->IsWaitingAtFlag()) // Liegt die Flagge an einer Flagge, muss ihr Weg neu berechnet werden
        {
            RoadPathDirection last_next_dir = ware->GetNextDir();
            ware->RecalcRoute();
            // special case: ware was lost some time ago and the new goal is at this flag and not a warehouse,hq,harbor
            // and the "flip-route" picked so a carrier would pick up the ware carry it away from goal then back and
            // drop  it off at the goal was just destroyed?
            // -> try to pick another flip route or tell the goal about failure.
            noRoadNode& wareLocation = *ware->GetLocation();
            noBaseBuilding* wareGoal = ware->GetGoal();
            if(wareGoal && ware->GetNextDir() == RoadPathDirection::NorthWest
               && wareLocation.GetPos() == wareGoal->GetFlag()->GetPos()
               && ((wareGoal->GetBuildingType() != BuildingType::Storehouse
                    && wareGoal->GetBuildingType() != BuildingType::Headquarters
                    && wareGoal->GetBuildingType() != BuildingType::HarborBuilding)
                   || wareGoal->GetType() == NodalObjectType::Buildingsite))
            {
                Direction newWareDir = Direction::NorthWest;
                for(auto dir : helpers::EnumRange<Direction>{})
                {
                    dir += 2u; // Need to skip Direction::NorthWest and we used to start with an offset of 2. TODO:
                               // Increase gameDataVersion and just skip NW
                    if(wareLocation.GetRoute(dir))
                    {
                        newWareDir = dir;
                        break;
                    }
                }
                if(newWareDir != Direction::NorthWest)
                {
                    ware->SetNextDir(toRoadPathDirection(newWareDir));
                } else // no route to goal -> notify goal, try to send ware to a warehouse
                {
                    ware->NotifyGoalAboutLostWare();
                    ware->FindRouteToWarehouse();
                }
            }
            // end of special case

            // notify carriers/flags about news if there are any
            if(ware->GetNextDir() != last_next_dir)
            {
                // notify current flag that transport in the old direction might not longer be required
                ware->RemoveWareJobForDir(last_next_dir);
                if(ware->GetNextDir() != RoadPathDirection::None)
                    ware->CallCarrier();
            }
        } else if(ware->IsWaitingInWarehouse())
        {
            if(!ware->IsRouteToGoal())
            {
                Ware* ware = *it;

                // Ware aus der Warteliste des Lagerhauses entfernen
                static_cast<nobBaseWarehouse*>(ware->GetLocation())->CancelWare(ware);
                // Das Ziel wird nun nich mehr beliefert
                ware->NotifyGoalAboutLostWare();
                // Ware aus der Liste raus
                it = ware_list.erase(it);
                // And trash it
                deletePtr(ware);
                continue;
            }
        } else if(ware->IsWaitingForShip())
        {
            // Weg neu berechnen
            ware->RecalcRoute();
        }

        ++it;
    }

    // Alle Häfen müssen ihre Figuren den Weg überprüfen lassen
    for(nobHarborBuilding* hb : buildings.GetHarbors())
    {
        hb->ExamineShipRouteOfPeople();
    }
}

bool GamePlayer::FindCarrierForRoad(RoadSegment* rs) const
{
    RTTR_Assert(rs->GetF1() != nullptr && rs->GetF2() != nullptr);
    std::array<unsigned, 2> length;
    std::array<nobBaseWarehouse*, 2> best;

    // Braucht der ein Boot?
    if(rs->GetRoadType() == RoadType::Water)
    {
        // dann braucht man Träger UND Boot
        best[0] = FindWarehouse(*rs->GetF1(), FW::HasWareAndFigure(GoodType::Boat, Job::Helper, false), false, false,
                                &length[0], rs);
        // 2. Flagge des Weges
        best[1] = FindWarehouse(*rs->GetF2(), FW::HasWareAndFigure(GoodType::Boat, Job::Helper, false), false, false,
                                &length[1], rs);
    } else
    {
        // 1. Flagge des Weges
        best[0] = FindWarehouse(*rs->GetF1(), FW::HasFigure(Job::Helper, false), false, false, &length[0], rs);
        // 2. Flagge des Weges
        best[1] = FindWarehouse(*rs->GetF2(), FW::HasFigure(Job::Helper, false), false, false, &length[1], rs);
    }

    // überhaupt nen Weg gefunden?
    // Welche Flagge benutzen?
    if(best[0] && (!best[1] || length[0] < length[1]))
        best[0]->OrderCarrier(*rs->GetF1(), *rs);
    else if(best[1])
        best[1]->OrderCarrier(*rs->GetF2(), *rs);
    else
        return false;
    return true;
}

bool GamePlayer::IsWarehouseValid(nobBaseWarehouse* wh) const
{
    return helpers::contains(buildings.GetStorehouses(), wh);
}

void GamePlayer::RecalcDistribution()
{
    GoodType lastWare = GoodType::Nothing;
    for(const DistributionMapping& mapping : distributionMap)
    {
        if(lastWare == std::get<0>(mapping))
            continue;
        lastWare = std::get<0>(mapping);
        RecalcDistributionOfWare(std::get<0>(mapping));
    }
}

void GamePlayer::RecalcDistributionOfWare(const GoodType ware)
{
    // Punktesystem zur Verteilung, in der Liste alle Gebäude sammeln, die die Ware wollen
    distribution[ware].client_buildings.clear();

    // 1. Anteile der einzelnen Waren ausrechnen

    /// Mapping of buildings that want the current ware to its percentage
    using BldEntry = std::pair<BuildingType, uint8_t>;
    std::vector<BldEntry> bldPercentageMap;

    unsigned goal_count = 0;

    for(const auto bld : helpers::enumRange<BuildingType>())
    {
        uint8_t percentForCurBld = distribution[ware].percent_buildings[bld];
        if(percentForCurBld)
        {
            distribution[ware].client_buildings.push_back(bld);
            goal_count += percentForCurBld;
            bldPercentageMap.emplace_back(bld, percentForCurBld);
        }
    }

    // TODO: evtl noch die counts miteinander kürzen (ggt berechnen)

    // Array für die Gebäudtypen erstellen

    std::vector<BuildingType>& wareGoals = distribution[ware].goals;
    wareGoals.clear();
    wareGoals.reserve(goal_count);

    // just drop them in the list, the distribution will be handled by going through this list using a prime as step
    // (see GameClientPlayer::FindClientForWare)
    for(const BldEntry& bldEntry : bldPercentageMap)
    {
        for(unsigned char i = 0; i < bldEntry.second; ++i)
            wareGoals.push_back(bldEntry.first);
    }

    distribution[ware].selected_goal = 0;
}

void GamePlayer::FindCarrierForAllRoads()
{
    for(RoadSegment* rs : roads)
    {
        if(!rs->hasCarrier(0))
            FindCarrierForRoad(rs);
    }
}

void GamePlayer::FindMaterialForBuildingSites()
{
    for(noBuildingSite* bldSite : buildings.GetBuildingSites())
        bldSite->OrderConstructionMaterial();
}

void GamePlayer::AddJobWanted(const Job job, noRoadNode* workplace)
{
    // Und gleich suchen
    if(!FindWarehouseForJob(job, workplace))
    {
        JobNeeded jn = {job, workplace};
        jobs_wanted.push_back(jn);
    }
}

void GamePlayer::JobNotWanted(noRoadNode* workplace, bool all)
{
    for(auto it = jobs_wanted.begin(); it != jobs_wanted.end();)
    {
        if(it->workplace == workplace)
        {
            it = jobs_wanted.erase(it);
            if(!all)
                return;
        } else
        {
            ++it;
        }
    }
}

void GamePlayer::OneJobNotWanted(const Job job, noRoadNode* workplace)
{
    const auto it = helpers::find_if(
      jobs_wanted, [workplace, job](const auto& it) { return it.workplace == workplace && it.job == job; });
    if(it != jobs_wanted.end())
        jobs_wanted.erase(it);
}

void GamePlayer::SendPostMessage(std::unique_ptr<PostMsg> msg)
{
    gwg.GetPostMgr().SendMsg(GetPlayerId(), std::move(msg));
}

unsigned GamePlayer::GetToolsOrderedVisual(unsigned toolIdx) const
{
    RTTR_Assert(toolIdx < tools_ordered.size());
    return std::max(0, int(tools_ordered[toolIdx] + tools_ordered_delta[toolIdx]));
}

unsigned GamePlayer::GetToolsOrdered(unsigned toolIdx) const
{
    RTTR_Assert(toolIdx < tools_ordered.size());
    return tools_ordered[toolIdx];
}

bool GamePlayer::ChangeToolOrderVisual(unsigned toolIdx, int changeAmount) const
{
    if(std::abs(changeAmount) > 100)
        return false;
    int newOrderAmount = int(GetToolsOrderedVisual(toolIdx)) + changeAmount;
    if(newOrderAmount < 0 || newOrderAmount > 100)
        return false;
    tools_ordered_delta[toolIdx] += changeAmount;
    return true;
}

unsigned GamePlayer::GetToolPriority(unsigned toolIdx) const
{
    RTTR_Assert(toolIdx < toolsSettings_.size());
    return toolsSettings_[toolIdx];
}

void GamePlayer::ToolOrderProcessed(unsigned toolIdx)
{
    RTTR_Assert(toolIdx < tools_ordered.size());
    if(tools_ordered[toolIdx])
    {
        --tools_ordered[toolIdx];
        gwg.GetNotifications().publish(ToolNote(ToolNote::OrderCompleted, GetPlayerId()));
    }
}

bool GamePlayer::FindWarehouseForJob(const Job job, noRoadNode* goal) const
{
    nobBaseWarehouse* wh = FindWarehouse(*goal, FW::HasFigure(job, true), false, false);

    if(wh)
    {
        // Es wurde ein Lagerhaus gefunden, wo es den geforderten Beruf gibt, also den Typen zur Arbeit rufen
        wh->OrderJob(job, goal, true);
        return true;
    }

    return false;
}

void GamePlayer::FindWarehouseForAllJobs()
{
    for(auto it = jobs_wanted.begin(); it != jobs_wanted.end();)
    {
        if(FindWarehouseForJob(it->job, it->workplace))
            it = jobs_wanted.erase(it);
        else
            ++it;
    }
}

void GamePlayer::FindWarehouseForAllJobs(const Job job)
{
    for(auto it = jobs_wanted.begin(); it != jobs_wanted.end();)
    {
        if(it->job == job)
        {
            if(FindWarehouseForJob(it->job, it->workplace))
                it = jobs_wanted.erase(it);
            else
                ++it;
        } else
            ++it;
    }
}

Ware* GamePlayer::OrderWare(const GoodType ware, noBaseBuilding* goal)
{
    /// Gibt es ein Lagerhaus mit dieser Ware?
    nobBaseWarehouse* wh = FindWarehouse(*goal, FW::HasMinWares(ware, 1), false, true);

    if(wh)
    {
        // Prüfe ob Notfallprogramm aktiv
        if(!emergency)
            return wh->OrderWare(ware, goal);
        else
        {
            // Wenn Notfallprogramm aktiv nur an Holzfäller und Sägewerke Bretter/Steine liefern
            if((ware != GoodType::Boards && ware != GoodType::Stones)
               || goal->GetBuildingType() == BuildingType::Woodcutter
               || goal->GetBuildingType() == BuildingType::Sawmill)
                return wh->OrderWare(ware, goal);
            else
                return nullptr;
        }
    } else // no warehouse can deliver the ware -> check all our wares for lost wares that might match the order
    {
        unsigned bestLength = std::numeric_limits<unsigned>::max();
        Ware* bestWare = nullptr;
        for(Ware* curWare : ware_list)
        {
            if(curWare->IsLostWare() && curWare->type == ware)
            {
                // got a lost ware with a road to goal -> find best
                unsigned curLength = curWare->CheckNewGoalForLostWare(*goal);
                if(curLength < bestLength)
                {
                    bestLength = curLength;
                    bestWare = curWare;
                }
            }
        }
        if(bestWare)
        {
            bestWare->SetNewGoalForLostWare(goal);
            return bestWare;
        }
    }
    return nullptr;
}

nofCarrier* GamePlayer::OrderDonkey(RoadSegment* road) const
{
    std::array<unsigned, 2> length;
    std::array<nobBaseWarehouse*, 2> best;

    // 1. Flagge des Weges
    best[0] = FindWarehouse(*road->GetF1(), FW::HasFigure(Job::PackDonkey, false), false, false, &length[0], road);
    // 2. Flagge des Weges
    best[1] = FindWarehouse(*road->GetF2(), FW::HasFigure(Job::PackDonkey, false), false, false, &length[1], road);

    // überhaupt nen Weg gefunden?
    // Welche Flagge benutzen?
    if(best[0] && (!best[1] || length[0] < length[1]))
        return best[0]->OrderDonkey(road, road->GetF1());
    else if(best[1])
        return best[1]->OrderDonkey(road, road->GetF2());
    else
        return nullptr;
}

RoadSegment* GamePlayer::FindRoadForDonkey(noRoadNode* start, noRoadNode** goal)
{
    // Bisher höchste Trägerproduktivität und die entsprechende Straße dazu
    unsigned best_productivity = 0;
    RoadSegment* best_road = nullptr;
    // Beste Flagge dieser Straße
    *goal = nullptr;

    for(RoadSegment* roadSeg : roads)
    {
        // Braucht die Straße einen Esel?
        if(roadSeg->NeedDonkey())
        {
            // Beste Flagge von diesem Weg, und beste Wegstrecke
            noRoadNode* current_best_goal = nullptr;
            // Weg zu beiden Flaggen berechnen
            unsigned length1, length2;
            bool isF1Reachable = gwg.FindHumanPathOnRoads(*start, *roadSeg->GetF1(), &length1, nullptr, roadSeg)
                                 != RoadPathDirection::None;
            bool isF2Reachable = gwg.FindHumanPathOnRoads(*start, *roadSeg->GetF2(), &length2, nullptr, roadSeg)
                                 != RoadPathDirection::None;

            // Wenn man zu einer Flagge nich kommt, die jeweils andere nehmen
            if(!isF1Reachable)
                current_best_goal = (isF2Reachable) ? roadSeg->GetF2() : nullptr;
            else if(!isF2Reachable)
                current_best_goal = roadSeg->GetF1();
            else
            {
                // ansonsten die kürzeste von beiden
                current_best_goal = (length1 < length2) ? roadSeg->GetF1() : roadSeg->GetF2();
            }

            // Kein Weg führt hin, nächste Straße bitte
            if(!current_best_goal)
                continue;

            // Jeweiligen Weg bestimmen
            unsigned current_best_way = (roadSeg->GetF1() == current_best_goal) ? length1 : length2;

            // Produktivität ausrechnen, *10 die Produktivität + die Wegstrecke, damit die
            // auch noch mit einberechnet wird
            unsigned current_productivity = 10 * roadSeg->getCarrier(0)->GetProductivity() + current_best_way;

            // Besser als der bisher beste?
            if(current_productivity > best_productivity)
            {
                // Dann wird der vom Thron gestoßen
                best_productivity = current_productivity;
                best_road = roadSeg;
                *goal = current_best_goal;
            }
        }
    }

    return best_road;
}

struct ClientForWare
{
    noBaseBuilding* bld;
    unsigned estimate; // points minus half the optimal distance
    unsigned points;

    ClientForWare(noBaseBuilding* bld, unsigned estimate, unsigned points)
        : bld(bld), estimate(estimate), points(points)
    {}

    bool operator<(const ClientForWare& b) const
    {
        // use estimate, points and object id (as tie breaker) for sorting
        if(estimate != b.estimate)
            return estimate > b.estimate;
        else if(points != b.points)
            return points > b.points;
        else
            return bld->GetObjId() > b.bld->GetObjId();
    }
};

noBaseBuilding* GamePlayer::FindClientForWare(Ware* ware)
{
    // Wenn es eine Goldmünze ist, wird das Ziel auf eine andere Art und Weise berechnet
    if(ware->type == GoodType::Coins)
        return FindClientForCoin(ware);

    // Warentyp herausfinden
    GoodType gt = ware->type;
    // All food is considered fish in the distribution table
    Distribution& wareDistribution =
      (gt == GoodType::Bread || gt == GoodType::Meat) ? distribution[GoodType::Fish] : distribution[gt];

    std::vector<ClientForWare> possibleClients;

    noRoadNode* start = ware->GetLocation();

    // Bretter und Steine können evtl. auch Häfen für Expeditionen gebrauchen
    if(gt == GoodType::Stones || gt == GoodType::Boards)
    {
        for(nobHarborBuilding* harbor : buildings.GetHarbors())
        {
            unsigned points = harbor->CalcDistributionPoints(gt);
            if(!points)
                continue;

            points += 10 * 30; // Verteilung existiert nicht, Expeditionen haben allerdings hohe Priorität
            unsigned distance = gwg.CalcDistance(start->GetPos(), harbor->GetPos()) / 2;
            possibleClients.push_back(ClientForWare(harbor, points > distance ? points - distance : 0, points));
        }
    }

    for(const auto bldType : wareDistribution.client_buildings)
    {
        // BuildingType::Headquarters sind Baustellen!!, da HQs ja sowieso nicht gebaut werden können
        if(bldType == BuildingType::Headquarters)
        {
            // Bei Baustellen die Extraliste abfragen
            for(noBuildingSite* bldSite : buildings.GetBuildingSites())
            {
                unsigned points = bldSite->CalcDistributionPoints(ware->GetLocation(), gt);
                if(!points)
                    continue;

                points += wareDistribution.percent_buildings[BuildingType::Headquarters] * 30;
                unsigned distance = gwg.CalcDistance(start->GetPos(), bldSite->GetPos()) / 2;
                possibleClients.push_back(ClientForWare(bldSite, points > distance ? points - distance : 0, points));
            }
        } else
        {
            // Für übrige Gebäude
            for(nobUsual* bld : buildings.GetBuildings(bldType))
            {
                unsigned points = bld->CalcDistributionPoints(ware->GetLocation(), gt);
                if(!points)
                    continue; // Ware not needed

                if(!wareDistribution.goals.empty())
                {
                    if(bld->GetBuildingType()
                       == static_cast<BuildingType>(wareDistribution.goals[wareDistribution.selected_goal]))
                        points += 300;
                    else if(points >= 300) // avoid overflows (async!)
                        points -= 300;
                    else
                        points = 0;
                }

                unsigned distance = gwg.CalcDistance(start->GetPos(), bld->GetPos()) / 2;
                possibleClients.push_back(ClientForWare(bld, points > distance ? points - distance : 0, points));
            }
        }
    }

    // sort our clients, highest score first
    std::sort(possibleClients.begin(), possibleClients.end());

    noBaseBuilding* lastBld = nullptr;
    noBaseBuilding* bestBld = nullptr;
    unsigned best_points = 0;
    for(auto& possibleClient : possibleClients)
    {
        unsigned path_length;

        // If our estimate is worse (or equal) best_points, the real value cannot be better.
        // As our list is sorted, further entries cannot be better either, so stop searching.
        if(possibleClient.estimate <= best_points)
            break;

        // get rid of double building entries. TODO: why are there double entries!?
        if(possibleClient.bld == lastBld)
            continue;

        lastBld = possibleClient.bld;

        // Just to be sure no underflow happens...
        if(possibleClient.points < best_points + 1)
            continue;

        // Find path ONLY if it may be better. Pathfinding is limited to the worst path score that would lead to a
        // better score. This eliminates the worst case scenario where all nodes in a split road network would be hit by
        // the pathfinding only to conclude that there is no possible path.
        if(gwg.FindPathForWareOnRoads(*start, *possibleClient.bld, &path_length, nullptr,
                                      (possibleClient.points - best_points) * 2 - 1)
           != RoadPathDirection::None)
        {
            unsigned score = possibleClient.points - (path_length / 2);

            // As we have limited our pathfinding to take a maximum of (points - best_points) * 2 - 1 steps,
            // path_length / 2 can at most be points - best_points - 1, so the score will be greater than best_points.
            // :)
            RTTR_Assert(score > best_points);

            best_points = score;
            bestBld = possibleClient.bld;
        }
    }

    if(bestBld && !wareDistribution.goals.empty())
        wareDistribution.selected_goal =
          (wareDistribution.selected_goal + 907) % unsigned(wareDistribution.goals.size());

    // Wenn kein Abnehmer gefunden wurde, muss es halt in ein Lagerhaus
    if(!bestBld)
        bestBld = FindWarehouseForWare(*ware);

    return bestBld;
}

nobBaseWarehouse* GamePlayer::FindWarehouseForWare(const Ware& ware) const
{
    // Check whs that collect this ware
    nobBaseWarehouse* wh = FindWarehouse(*ware.GetLocation(), FW::CollectsWare(ware.type), true, true);
    // If there is none, check those that accept it
    if(!wh)
    {
        // First find the ones, that do not send it right away (IMPORTANT: This avoids sending a ware to the wh that is
        // sending the ware out)
        wh = FindWarehouse(*ware.GetLocation(), FW::AcceptsWareButNoSend(ware.type), true, true);
        // The others only if this fails
        if(!wh)
            wh = FindWarehouse(*ware.GetLocation(), FW::AcceptsWare(ware.type), true, true);
    }
    return wh;
}

nobBaseMilitary* GamePlayer::FindClientForCoin(Ware* ware) const
{
    nobBaseMilitary* bb = nullptr;
    unsigned best_points = 0, points;

    // Militärgebäude durchgehen
    for(nobMilitary* milBld : buildings.GetMilitaryBuildings())
    {
        unsigned way_points;

        points = milBld->CalcCoinsPoints();
        // Wenn 0, will er gar keine Münzen (Goldzufuhr gestoppt)
        if(points)
        {
            // Weg dorthin berechnen
            if(gwg.FindPathForWareOnRoads(*ware->GetLocation(), *milBld, &way_points) != RoadPathDirection::None)
            {
                // Die Wegpunkte noch davon abziehen
                points -= way_points;
                // Besser als der bisher Beste?
                if(points > best_points)
                {
                    best_points = points;
                    bb = milBld;
                }
            }
        }
    }

    // Wenn kein Abnehmer gefunden wurde, muss es halt in ein Lagerhaus
    if(!bb)
        bb = FindWarehouseForWare(*ware);

    return bb;
}

unsigned GamePlayer::GetBuidingSitePriority(const noBuildingSite* building_site)
{
    if(useCustomBuildOrder_)
    {
        // Spezielle Reihenfolge

        // Typ in der Reihenfolge suchen und Position als Priorität zurückgeben
        for(unsigned i = 0; i < build_order.size(); ++i)
        {
            if(building_site->GetBuildingType() == build_order[i])
                return i;
        }
    } else
    {
        // Reihenfolge der Bauaufträge, also was zuerst in Auftrag gegeben wurde, wird zuerst gebaut
        unsigned i = 0;
        for(noBuildingSite* bldSite : buildings.GetBuildingSites())
        {
            if(building_site == bldSite)
                return i;
            i++;
        }
    }

    LOG.write("GameClientPlayer::GetBuidingSitePriority: ERROR: Buildingsite or type of it not found in the list!\n");
    RTTR_Assert(false);
    // We may want to multiply this value so don't return the absolute max value
    return std::numeric_limits<unsigned>::max() / 1000;
}

void GamePlayer::ConvertTransportData(const TransportOrders& transport_data)
{
    for(const auto ware : helpers::EnumRange<GoodType>{})
        transportPrio[ware] = GetTransportPrioFromOrdering(transport_data, ware);
}

bool GamePlayer::IsAlly(const unsigned char playerId) const
{
    // Der Spieler ist ja auch zu sich selber verbündet
    if(GetPlayerId() == playerId)
        return true;
    else
        return GetPactState(PactType::TreatyOfAlliance, playerId) == PactState::Accepted;
}

bool GamePlayer::IsAttackable(const unsigned char playerId) const
{
    // Verbündete dürfen nicht angegriffen werden
    if(IsAlly(playerId))
        return false;
    else
        // Ansonsten darf bei bestehendem Nichtangriffspakt ebenfalls nicht angegriffen werden
        return GetPactState(PactType::NonAgressionPact, playerId) != PactState::Accepted;
}

void GamePlayer::OrderTroops(nobMilitary* goal, unsigned count, bool ignoresettingsendweakfirst) const
{
    // Solange Lagerhäuser nach Soldaten absuchen, bis entweder keins mehr übrig ist oder alle Soldaten bestellt sind
    nobBaseWarehouse* wh;
    do
    {
        wh = FindWarehouse(*goal, FW::HasMinSoldiers(1), false, false);
        if(wh)
        {
            unsigned order_count = std::min(wh->GetNumSoldiers(), count);
            count -= order_count;
            wh->OrderTroops(goal, order_count, ignoresettingsendweakfirst);
        }
    } while(count && wh);
}

void GamePlayer::RegulateAllTroops()
{
    for(nobMilitary* milBld : buildings.GetMilitaryBuildings())
        milBld->RegulateTroops();
}

/// Prüft von allen Militärgebäuden die Fahnen neu
void GamePlayer::RecalcMilitaryFlags()
{
    for(nobMilitary* milBld : buildings.GetMilitaryBuildings())
        milBld->LookForEnemyBuildings(nullptr);
}

/// Sucht für Soldaten ein neues Militärgebäude, als Argument wird Referenz auf die
/// entsprechende Soldatenanzahl im Lagerhaus verlangt
void GamePlayer::NewSoldiersAvailable(const unsigned& soldier_count)
{
    RTTR_Assert(soldier_count > 0);
    // solange laufen lassen, bis soldier_count = 0, d.h. der Soldat irgendwohin geschickt wurde
    // Zuerst nach unbesetzten Militärgebäude schauen
    for(nobMilitary* milBld : buildings.GetMilitaryBuildings())
    {
        if(milBld->IsNewBuilt())
        {
            milBld->RegulateTroops();
            // Used that soldier? Go out
            if(!soldier_count)
                return;
        }
    }

    // Als nächstes Gebäude in Grenznähe
    for(nobMilitary* milBld : buildings.GetMilitaryBuildings())
    {
        if(milBld->GetFrontierDistance() == FrontierDistance::Near)
        {
            milBld->RegulateTroops();
            // Used that soldier? Go out
            if(!soldier_count)
                return;
        }
    }

    // Und den Rest ggf.
    for(nobMilitary* milBld : buildings.GetMilitaryBuildings())
    {
        // already checked? -> skip
        if(milBld->GetFrontierDistance() == FrontierDistance::Near || milBld->IsNewBuilt())
            continue;
        milBld->RegulateTroops();
        if(!soldier_count) // used the soldier?
            return;
    }
}

void GamePlayer::CallFlagWorker(const MapPoint pt, const Job job)
{
    auto* flag = gwg.GetSpecObj<noFlag>(pt);
    if(!flag)
        return;
    /// Find wh with given job type (e.g. geologist, scout, ...)
    nobBaseWarehouse* wh = FindWarehouse(*flag, FW::HasFigure(job, true), false, false);

    /// Wenns eins gibt, dann rufen
    if(wh)
        wh->OrderJob(job, flag, true);
}

bool GamePlayer::IsFlagWorker(nofFlagWorker* flagworker)
{
    return helpers::contains(flagworkers, flagworker);
}

void GamePlayer::FlagDestroyed(noFlag* flag)
{
    // Alle durchgehen und ggf. sagen, dass sie keine Flagge mehr haben, wenn das ihre Flagge war, die zerstört wurde
    for(auto it = flagworkers.begin(); it != flagworkers.end();)
    {
        if((*it)->GetFlag() == flag)
        {
            (*it)->LostWork();
            it = flagworkers.erase(it);
        } else
            ++it;
    }
}

void GamePlayer::RefreshDefenderList()
{
    shouldSendDefenderList.clear();
    // Add as many true values as set in the settings, the rest will be false
    for(unsigned i = 0; i < MILITARY_SETTINGS_SCALE[2]; ++i)
        shouldSendDefenderList.push_back(i < militarySettings_[2]);
    // und ordentlich schütteln
    RANDOM_SHUFFLE2(shouldSendDefenderList, 0);
}

void GamePlayer::ChangeMilitarySettings(const MilitarySettings& military_settings)
{
    for(unsigned i = 0; i < military_settings.size(); ++i)
    {
        // Sicherstellen, dass im validen Bereich
        RTTR_Assert(military_settings[i] <= MILITARY_SETTINGS_SCALE[i]);
        this->militarySettings_[i] = military_settings[i];
    }
    /// Truppen müssen neu kalkuliert werden
    RegulateAllTroops();
    /// Die Verteidigungsliste muss erneuert werden
    RefreshDefenderList();
}

/// Setzt neue Werkzeugeinstellungen
void GamePlayer::ChangeToolsSettings(const ToolSettings& tools_settings,
                                     const std::array<int8_t, NUM_TOOLS>& orderChanges)
{
    const bool settingsChanged = toolsSettings_ != tools_settings;
    toolsSettings_ = tools_settings;
    if(settingsChanged)
        gwg.GetNotifications().publish(ToolNote(ToolNote::SettingsChanged, GetPlayerId()));

    for(unsigned i = 0; i < NUM_TOOLS; ++i)
    {
        tools_ordered[i] = helpers::clamp(tools_ordered[i] + orderChanges[i], 0, 100);
        tools_ordered_delta[i] -= orderChanges[i];

        if(orderChanges[i] != 0)
        {
            LOG.write(">> Committing an order of %d for tool #%d(%s)\n", LogTarget::File) % (int)orderChanges[i] % i
              % _(WARE_NAMES[TOOLS[i]]);
            gwg.GetNotifications().publish(ToolNote(ToolNote::OrderPlaced, GetPlayerId()));
        }
    }
}

/// Setzt neue Verteilungseinstellungen
void GamePlayer::ChangeDistribution(const Distributions& distribution_settings)
{
    unsigned idx = 0;
    for(const DistributionMapping& mapping : distributionMap)
    {
        distribution[std::get<0>(mapping)].percent_buildings[std::get<1>(mapping)] = distribution_settings[idx++];
    }

    RecalcDistribution();
}

/// Setzt neue Baureihenfolge-Einstellungen
void GamePlayer::ChangeBuildOrder(bool useCustomBuildOrder, const BuildOrders& order_data)
{
    this->useCustomBuildOrder_ = useCustomBuildOrder;
    this->build_order = order_data;
}

bool GamePlayer::ShouldSendDefender()
{
    // Wenn wir schon am Ende sind, muss die Verteidgungsliste erneuert werden
    if(shouldSendDefenderList.empty())
        RefreshDefenderList();

    bool result = shouldSendDefenderList.back();
    shouldSendDefenderList.pop_back();
    return result;
}

void GamePlayer::TestDefeat()
{
    // Nicht schon besiegt?
    // Keine Militärgebäude, keine Lagerhäuser (HQ,Häfen) -> kein Land --> verloren
    if(!isDefeated && buildings.GetMilitaryBuildings().empty() && buildings.GetStorehouses().empty())
        Surrender();
}

void GamePlayer::Surrender()
{
    if(isDefeated)
        return;

    isDefeated = true;

    // GUI Bescheid sagen
    if(gwg.GetGameInterface())
        gwg.GetGameInterface()->GI_PlayerDefeated(GetPlayerId());
}

void GamePlayer::SetStatisticValue(StatisticType type, unsigned value)
{
    statisticCurrentData[type] = value;
}

void GamePlayer::ChangeStatisticValue(StatisticType type, int change)
{
    assert(statisticCurrentData[type] + change >= 0);
    statisticCurrentData[type] += change;
}

void GamePlayer::IncreaseMerchandiseStatistic(GoodType type)
{
    // Einsortieren...
    switch(type)
    {
        case GoodType::Wood: statisticCurrentMerchandiseData[0]++; break;
        case GoodType::Boards: statisticCurrentMerchandiseData[1]++; break;
        case GoodType::Stones: statisticCurrentMerchandiseData[2]++; break;
        case GoodType::Fish:
        case GoodType::Bread:
        case GoodType::Meat: statisticCurrentMerchandiseData[3]++; break;
        case GoodType::Water: statisticCurrentMerchandiseData[4]++; break;
        case GoodType::Beer: statisticCurrentMerchandiseData[5]++; break;
        case GoodType::Coal: statisticCurrentMerchandiseData[6]++; break;
        case GoodType::IronOre: statisticCurrentMerchandiseData[7]++; break;
        case GoodType::Gold: statisticCurrentMerchandiseData[8]++; break;
        case GoodType::Iron: statisticCurrentMerchandiseData[9]++; break;
        case GoodType::Coins: statisticCurrentMerchandiseData[10]++; break;
        case GoodType::Tongs:
        case GoodType::Axe:
        case GoodType::Saw:
        case GoodType::PickAxe:
        case GoodType::Hammer:
        case GoodType::Shovel:
        case GoodType::Crucible:
        case GoodType::RodAndLine:
        case GoodType::Scythe:
        case GoodType::Cleaver:
        case GoodType::Rollingpin:
        case GoodType::Bow: statisticCurrentMerchandiseData[11]++; break;
        case GoodType::ShieldVikings:
        case GoodType::ShieldAfricans:
        case GoodType::ShieldRomans:
        case GoodType::ShieldJapanese:
        case GoodType::Sword: statisticCurrentMerchandiseData[12]++; break;
        case GoodType::Boat: statisticCurrentMerchandiseData[13]++; break;
        default: break;
    }
}

/// Calculates current statistics
void GamePlayer::CalcStatistics()
{
    // Waren aus der Inventur zählen
    statisticCurrentData[StatisticType::Merchandise] = 0;
    for(const auto i : helpers::enumRange<GoodType>())
        statisticCurrentData[StatisticType::Merchandise] += global_inventory[i];

    // Bevölkerung aus der Inventur zählen
    statisticCurrentData[StatisticType::Inhabitants] = 0;
    for(const auto i : helpers::enumRange<Job>())
        statisticCurrentData[StatisticType::Inhabitants] += global_inventory[i];

    // Militär aus der Inventur zählen
    statisticCurrentData[StatisticType::Military] =
      global_inventory.people[Job::Private] + global_inventory.people[Job::PrivateFirstClass] * 2
      + global_inventory.people[Job::Sergeant] * 3 + global_inventory.people[Job::Officer] * 4
      + global_inventory.people[Job::General] * 5;

    // Produktivität berechnen
    statisticCurrentData[StatisticType::Productivity] = buildings.CalcAverageProductivity();

    // Total points for tournament games
    statisticCurrentData[StatisticType::Tournament] =
      statisticCurrentData[StatisticType::Military] + 3 * statisticCurrentData[StatisticType::Vanquished];
}

void GamePlayer::StatisticStep()
{
    CalcStatistics();

    // 15-min-Statistik ein Feld weiterschieben
    for(const auto i : helpers::enumRange<StatisticType>())
    {
        statistic[StatisticTime::T15Minutes].data[i][incrStatIndex(statistic[StatisticTime::T15Minutes].currentIndex)] =
          statisticCurrentData[i];
    }
    for(unsigned i = 0; i < NUM_STAT_MERCHANDISE_TYPES; ++i)
    {
        statistic[StatisticTime::T15Minutes]
          .merchandiseData[i][incrStatIndex(statistic[StatisticTime::T15Minutes].currentIndex)] =
          statisticCurrentMerchandiseData[i];
    }
    statistic[StatisticTime::T15Minutes].currentIndex =
      incrStatIndex(statistic[StatisticTime::T15Minutes].currentIndex);

    statistic[StatisticTime::T15Minutes].counter++;

    // Prüfen ob 4mal 15-min-Statistik weitergeschoben wurde, wenn ja: 1-h-Statistik weiterschieben
    // und aktuellen Wert der 15min-Statistik benutzen
    // gleiches für die 4h und 16h Statistik
    for(const auto t : helpers::enumRange<StatisticTime>())
    {
        if(t == StatisticTime(helpers::MaxEnumValue_v<StatisticTime>))
            break;
        const auto nextT = StatisticTime(rttr::enum_cast(t) + 1);
        if(statistic[t].counter == 4)
        {
            statistic[t].counter = 0;
            for(const auto i : helpers::enumRange<StatisticType>())
            {
                statistic[nextT].data[i][incrStatIndex(statistic[nextT].currentIndex)] = statisticCurrentData[i];
            }

            // Summe für den Zeitraum berechnen (immer 4 Zeitschritte der jeweils kleineren Statistik)
            for(unsigned i = 0; i < NUM_STAT_MERCHANDISE_TYPES; ++i)
            {
                statistic[nextT].merchandiseData[i][incrStatIndex(statistic[nextT].currentIndex)] =
                  statisticCurrentMerchandiseData[i]
                  + statistic[t].merchandiseData[i][decrStatIndex(statistic[t].currentIndex, 1)]
                  + statistic[t].merchandiseData[i][decrStatIndex(statistic[t].currentIndex, 2)]
                  + statistic[t].merchandiseData[i][decrStatIndex(statistic[t].currentIndex, 3)];
            }

            statistic[nextT].currentIndex = incrStatIndex(statistic[nextT].currentIndex);
            statistic[nextT].counter++;
        }
    }

    // Warenstatistikzähler nullen
    statisticCurrentMerchandiseData.fill(0);
}

GamePlayer::Pact::Pact(SerializedGameData& sgd)
    : duration(sgd.PopUnsignedInt()), start(sgd.PopUnsignedInt()), accepted(sgd.PopBool()), want_cancel(sgd.PopBool())
{}

void GamePlayer::Pact::Serialize(SerializedGameData& sgd) const
{
    sgd.PushUnsignedInt(duration);
    sgd.PushUnsignedInt(start);
    sgd.PushBool(accepted);
    sgd.PushBool(want_cancel);
}

void GamePlayer::PactChanged(const PactType pt)
{
    // Recheck military flags as the border (to an enemy) might have changed
    RecalcMilitaryFlags();

    // Ggf. den GUI Bescheid sagen, um Sichtbarkeiten etc. neu zu berechnen
    if(pt == PactType::TreatyOfAlliance)
    {
        if(gwg.GetGameInterface())
            gwg.GetGameInterface()->GI_TreatyOfAllianceChanged(GetPlayerId());
    }
}

void GamePlayer::SuggestPact(const unsigned char targetPlayerId, const PactType pt, const unsigned duration)
{
    // Don't try to make pact with self
    if(targetPlayerId == GetPlayerId())
        return;

    if(!pacts[targetPlayerId][pt].accepted && duration > 0)
    {
        pacts[targetPlayerId][pt].accepted = false;
        pacts[targetPlayerId][pt].duration = duration;
        pacts[targetPlayerId][pt].start = gwg.GetEvMgr().GetCurrentGF();
        GamePlayer targetPlayer = gwg.GetPlayer(targetPlayerId);
        if(targetPlayer.isHuman())
            targetPlayer.SendPostMessage(std::make_unique<DiplomacyPostQuestion>(
              gwg.GetEvMgr().GetCurrentGF(), pt, pacts[targetPlayerId][pt].start, *this, duration));
        else if(gwg.HasLua())
            gwg.GetLua().EventSuggestPact(pt, GetPlayerId(), targetPlayerId, duration);
    }
}

void GamePlayer::AcceptPact(const unsigned id, const PactType pt, const unsigned char targetPlayer)
{
    if(!pacts[targetPlayer][pt].accepted && pacts[targetPlayer][pt].duration > 0 && pacts[targetPlayer][pt].start == id)
    {
        MakePact(pt, targetPlayer, pacts[targetPlayer][pt].duration);
        gwg.GetPlayer(targetPlayer).MakePact(pt, GetPlayerId(), pacts[targetPlayer][pt].duration);
        PactChanged(pt);
        gwg.GetPlayer(targetPlayer).PactChanged(pt);
        if(gwg.HasLua())
            gwg.GetLua().EventPactCreated(pt, GetPlayerId(), targetPlayer, pacts[targetPlayer][pt].duration);
    }
}

/// Bündnis (real, d.h. spielentscheidend) abschließen
void GamePlayer::MakePact(const PactType pt, const unsigned char other_player, const unsigned duration)
{
    pacts[other_player][pt].accepted = true;
    pacts[other_player][pt].start = gwg.GetEvMgr().GetCurrentGF();
    pacts[other_player][pt].duration = duration;
    pacts[other_player][pt].want_cancel = false;

    SendPostMessage(std::make_unique<PostMsg>(gwg.GetEvMgr().GetCurrentGF(), pt, gwg.GetPlayer(other_player), true));
}

/// Zeigt an, ob ein Pakt besteht
PactState GamePlayer::GetPactState(const PactType pt, const unsigned char other_player) const
{
    // Prüfen, ob Bündnis in Kraft ist
    if(pacts[other_player][pt].duration)
    {
        if(!pacts[other_player][pt].accepted)
            return PactState::InProgress;

        if(pacts[other_player][pt].duration == DURATION_INFINITE
           || gwg.GetEvMgr().GetCurrentGF() < pacts[other_player][pt].start + pacts[other_player][pt].duration)
            return PactState::Accepted;
    }

    return PactState::None;
}

/// all allied players get a letter with the location
void GamePlayer::NotifyAlliesOfLocation(const MapPoint pt)
{
    for(unsigned i = 0; i < gwg.GetNumPlayers(); ++i)
    {
        if(i != GetPlayerId() && IsAlly(i))
            gwg.GetPlayer(i).SendPostMessage(std::make_unique<PostMsg>(
              gwg.GetEvMgr().GetCurrentGF(), _("Your ally wishes to notify you of this location"),
              PostCategory::Diplomacy, pt));
    }
}

/// Gibt die verbleibende Dauer zurück, die ein Bündnis noch laufen wird (DURATION_INFINITE = für immer)
unsigned GamePlayer::GetRemainingPactTime(const PactType pt, const unsigned char other_player) const
{
    if(pacts[other_player][pt].duration)
    {
        if(pacts[other_player][pt].accepted)
        {
            if(pacts[other_player][pt].duration == DURATION_INFINITE)
                return DURATION_INFINITE;
            else if(gwg.GetEvMgr().GetCurrentGF() <= pacts[other_player][pt].start + pacts[other_player][pt].duration)
                return ((pacts[other_player][pt].start + pacts[other_player][pt].duration)
                        - gwg.GetEvMgr().GetCurrentGF());
        }
    }

    return 0;
}

/// Gibt Einverständnis, dass dieser Spieler den Pakt auflösen will
/// Falls dieser Spieler einen Bündnisvorschlag gemacht hat, wird dieser dagegen zurückgenommen
void GamePlayer::CancelPact(const PactType pt, const unsigned char otherPlayerIdx)
{
    // Don't try to cancel pact with self
    if(otherPlayerIdx == GetPlayerId())
        return;

    // Besteht bereits ein Bündnis?
    if(pacts[otherPlayerIdx][pt].accepted)
    {
        // Vermerken, dass der Spieler das Bündnis auflösen will
        pacts[otherPlayerIdx][pt].want_cancel = true;

        // Will der andere Spieler das Bündnis auch auflösen?
        GamePlayer& otherPlayer = gwg.GetPlayer(otherPlayerIdx);
        if(otherPlayer.pacts[GetPlayerId()][pt].want_cancel)
        {
            // Dann wird das Bündnis aufgelöst
            pacts[otherPlayerIdx][pt].accepted = false;
            pacts[otherPlayerIdx][pt].duration = 0;
            pacts[otherPlayerIdx][pt].want_cancel = false;

            otherPlayer.pacts[GetPlayerId()][pt].accepted = false;
            otherPlayer.pacts[GetPlayerId()][pt].duration = 0;
            otherPlayer.pacts[GetPlayerId()][pt].want_cancel = false;

            // Den Spielern eine Informationsnachricht schicken
            gwg.GetPlayer(otherPlayerIdx)
              .SendPostMessage(std::make_unique<PostMsg>(gwg.GetEvMgr().GetCurrentGF(), pt, *this, false));
            SendPostMessage(
              std::make_unique<PostMsg>(gwg.GetEvMgr().GetCurrentGF(), pt, gwg.GetPlayer(otherPlayerIdx), false));
            PactChanged(pt);
            otherPlayer.PactChanged(pt);
            if(gwg.HasLua())
                gwg.GetLua().EventPactCanceled(pt, GetPlayerId(), otherPlayerIdx);
        } else
        {
            // Ansonsten den anderen Spieler fragen, ob der das auch so sieht
            if(otherPlayer.isHuman())
                otherPlayer.SendPostMessage(std::make_unique<DiplomacyPostQuestion>(
                  gwg.GetEvMgr().GetCurrentGF(), pt, pacts[otherPlayerIdx][pt].start, *this));
            else if(!gwg.HasLua() || gwg.GetLua().EventCancelPactRequest(pt, GetPlayerId(), otherPlayerIdx))
            {
                // AI accepts cancels, if there is no lua-interace
                pacts[otherPlayerIdx][pt].accepted = false;
                pacts[otherPlayerIdx][pt].duration = 0;
                pacts[otherPlayerIdx][pt].want_cancel = false;

                otherPlayer.pacts[GetPlayerId()][pt].accepted = false;
                otherPlayer.pacts[GetPlayerId()][pt].duration = 0;
                otherPlayer.pacts[GetPlayerId()][pt].want_cancel = false;

                if(gwg.HasLua())
                    gwg.GetLua().EventPactCanceled(pt, GetPlayerId(), otherPlayerIdx);
            }
        }
    } else
    {
        // Es besteht kein Bündnis, also unseren Bündnisvorschlag wieder zurücknehmen
        pacts[otherPlayerIdx][pt].duration = 0;
    }
}

void GamePlayer::MakeStartPacts()
{
    // Reset pacts
    for(unsigned i = 0; i < gwg.GetNumPlayers(); ++i)
    {
        for(const auto z : helpers::enumRange<PactType>())
            pacts[i][z] = Pact();
    }

    // No team -> No pacts
    if(team == Team::None)
        return;
    RTTR_Assert(isTeam(team));

    // Create ally- and non-aggression-pact for all players of same team
    for(unsigned i = 0; i < gwg.GetNumPlayers(); ++i)
    {
        if(team != gwg.GetPlayer(i).team)
            continue;
        for(const auto z : helpers::enumRange<PactType>())
        {
            pacts[i][z].duration = DURATION_INFINITE;
            pacts[i][z].start = 0;
            pacts[i][z].accepted = true;
            pacts[i][z].want_cancel = false;
        }
    }
}

bool GamePlayer::IsWareRegistred(Ware* ware)
{
    return (helpers::contains(ware_list, ware));
}

bool GamePlayer::IsWareDependent(Ware* ware)
{
    for(nobBaseWarehouse* wh : buildings.GetStorehouses())
    {
        if(wh->IsWareDependent(ware))
            return true;
    }

    return false;
}

void GamePlayer::IncreaseInventoryWare(const GoodType ware, const unsigned count)
{
    global_inventory.Add(ConvertShields(ware), count);
}

void GamePlayer::DecreaseInventoryWare(const GoodType ware, const unsigned count)
{
    global_inventory.Remove(ConvertShields(ware), count);
}

/// Registriert ein Schiff beim Einwohnermeldeamt
void GamePlayer::RegisterShip(noShip* ship)
{
    ships.push_back(ship);
    // Evtl bekommt das Schiffchen gleich was zu tun?
    GetJobForShip(ship);
}

struct ShipForHarbor
{
    noShip* ship;
    uint32_t estimate;

    ShipForHarbor(noShip* ship, uint32_t estimate) : ship(ship), estimate(estimate) {}

    bool operator<(const ShipForHarbor& b) const
    {
        return (estimate < b.estimate) || (estimate == b.estimate && ship->GetObjId() < b.ship->GetObjId());
    }
};

/// Schiff für Hafen bestellen
bool GamePlayer::OrderShip(nobHarborBuilding& hb)
{
    std::vector<ShipForHarbor> sfh;

    // we need more ships than those that are already on their way? limit search to idle ships
    if(GetShipsToHarbor(hb) < hb.GetNumNeededShips())
    {
        for(noShip* ship : ships)
        {
            if(ship->IsIdling() && gwg.IsHarborAtSea(gwg.GetHarborPointID(hb.GetPos()), ship->GetSeaID()))
                sfh.push_back(ShipForHarbor(ship, gwg.CalcDistance(hb.GetPos(), ship->GetPos())));
        }
    } else
    {
        for(noShip* ship : ships)
        {
            if((ship->IsIdling() && gwg.IsHarborAtSea(gwg.GetHarborPointID(hb.GetPos()), ship->GetSeaID()))
               || ship->IsGoingToHarbor(hb))
            {
                sfh.push_back(ShipForHarbor(ship, gwg.CalcDistance(hb.GetPos(), ship->GetPos())));
            }
        }
    }

    std::sort(sfh.begin(), sfh.end());

    noShip* best_ship = nullptr;
    uint32_t best_distance = std::numeric_limits<uint32_t>::max();
    std::vector<Direction> best_route;

    for(auto& it : sfh)
    {
        uint32_t distance;
        std::vector<Direction> route;

        // the estimate (air-line distance) for this and all other ships in the list is already worse than what we
        // found? disregard the rest
        if(it.estimate >= best_distance)
            break;

        noShip* ship = it.ship;

        MapPoint dest = gwg.GetCoastalPoint(hb.GetHarborPosID(), ship->GetSeaID());

        // ship already there?
        if(ship->GetPos() == dest)
        {
            hb.ShipArrived(ship);
            return (true);
        }

        if(gwg.FindShipPathToHarbor(ship->GetPos(), hb.GetHarborPosID(), ship->GetSeaID(), &route, &distance))
        {
            if(distance < best_distance)
            {
                best_ship = ship;
                best_distance = distance;
                best_route = route;
            }
        }
    }

    // only order ships not already on their way
    if(best_ship && best_ship->IsIdling())
    {
        best_ship->GoToHarbor(hb, best_route);

        return (true);
    }

    return (false);
}

/// Meldet das Schiff wieder ab
void GamePlayer::RemoveShip(noShip* ship)
{
    for(unsigned i = 0; i < ships.size(); ++i)
    {
        if(ships[i] == ship)
        {
            ships.erase(ships.begin() + i);
            return;
        }
    }
}

/// Versucht, für ein untätiges Schiff eine Arbeit zu suchen
void GamePlayer::GetJobForShip(noShip* ship)
{
    // Evtl. steht irgendwo eine Expedition an und das Schiff kann diese übernehmen
    nobHarborBuilding* best = nullptr;
    int best_points = 0;
    std::vector<Direction> best_route;

    // Beste Weglänge, die ein Schiff zurücklegen muss, welches gerade nichts zu tun hat
    for(nobHarborBuilding* harbor : buildings.GetHarbors())
    {
        // Braucht der Hafen noch Schiffe?
        if(harbor->GetNumNeededShips() == 0)
            continue;

        // Anzahl der Schiffe ermitteln, die diesen Hafen bereits anfahren
        unsigned ships_coming = GetShipsToHarbor(*harbor);

        // Evtl. kommen schon genug?
        if(harbor->GetNumNeededShips() <= ships_coming)
            continue;

        // liegen wir am gleichen Meer?
        if(gwg.IsHarborAtSea(harbor->GetHarborPosID(), ship->GetSeaID()))
        {
            const MapPoint coastPt = gwg.GetCoastalPoint(harbor->GetHarborPosID(), ship->GetSeaID());

            // Evtl. sind wir schon da?
            if(ship->GetPos() == coastPt)
            {
                harbor->ShipArrived(ship);
                return;
            }

            unsigned length;
            std::vector<Direction> route;

            if(gwg.FindShipPathToHarbor(ship->GetPos(), harbor->GetHarborPosID(), ship->GetSeaID(), &route, &length))
            {
                // Punkte ausrechnen
                int points = harbor->GetNeedForShip(ships_coming) - length;
                if(points > best_points || !best)
                {
                    best = harbor;
                    best_points = points;
                    best_route = route;
                }
            }
        }
    }

    // Einen Hafen gefunden?
    if(best)
        // Dann bekommt das gleich der Hafen
        ship->GoToHarbor(*best, best_route);
}

/// Gibt die ID eines Schiffes zurück
unsigned GamePlayer::GetShipID(const noShip* const ship) const
{
    for(unsigned i = 0; i < ships.size(); ++i)
        if(ships[i] == ship)
            return i;

    return 0xFFFFFFFF;
}

/// Gibt ein Schiff anhand der ID zurück bzw. nullptr, wenn keines mit der ID existiert
noShip* GamePlayer::GetShipByID(const unsigned ship_id) const
{
    if(ship_id >= ships.size())
        return nullptr;
    else
        return ships[ship_id];
}

/// Gibt eine Liste mit allen Häfen dieses Spieler zurück, die an ein bestimmtes Meer angrenzen
void GamePlayer::GetHarborsAtSea(std::vector<nobHarborBuilding*>& harbor_buildings, const unsigned short seaId) const
{
    for(nobHarborBuilding* harbor : buildings.GetHarbors())
    {
        if(helpers::contains(harbor_buildings, harbor))
            continue;

        if(gwg.IsHarborAtSea(harbor->GetHarborPosID(), seaId))
            harbor_buildings.push_back(harbor);
    }
}

/// Gibt die Anzahl der Schiffe, die einen bestimmten Hafen ansteuern, zurück
unsigned GamePlayer::GetShipsToHarbor(const nobHarborBuilding& hb) const
{
    unsigned count = 0;
    for(const auto* ship : ships)
    {
        if(ship->IsGoingToHarbor(hb))
            ++count;
    }

    return count;
}

/// Sucht einen Hafen in der Nähe, wo dieses Schiff seine Waren abladen kann
/// gibt true zurück, falls erfolgreich
bool GamePlayer::FindHarborForUnloading(noShip* ship, const MapPoint start, unsigned* goal_harborId,
                                        std::vector<Direction>* route, nobHarborBuilding* exception)
{
    nobHarborBuilding* best = nullptr;
    unsigned best_distance = 0xffffffff;

    for(nobHarborBuilding* hb : buildings.GetHarbors())
    {
        // Bestimmten Hafen ausschließen
        if(hb == exception)
            continue;

        // Prüfen, ob Hafen an das Meer, wo sich das Schiff gerade befindet, angrenzt
        if(!gwg.IsHarborAtSea(hb->GetHarborPosID(), ship->GetSeaID()))
            continue;

        // Distanz ermitteln zwischen Schiff und Hafen, Schiff kann natürlich auch über Kartenränder fahren
        unsigned distance = gwg.CalcDistance(ship->GetPos(), hb->GetPos());

        // Kürzerer Weg als bisher bestes Ziel?
        if(distance < best_distance)
        {
            best_distance = distance;
            best = hb;
        }
    }

    // Hafen gefunden?
    if(best)
    {
        // Weg dorthin suchen
        route->clear();
        *goal_harborId = best->GetHarborPosID();
        const MapPoint coastPt = gwg.GetCoastalPoint(best->GetHarborPosID(), ship->GetSeaID());
        if(start == coastPt
           || gwg.FindShipPathToHarbor(start, best->GetHarborPosID(), ship->GetSeaID(), route, nullptr))
            return true;
    }

    return false;
}

void GamePlayer::TestForEmergencyProgramm()
{
    // we are already defeated, do not even think about an emergency program - it's too late :-(
    if(isDefeated)
        return;

    // In Lagern vorhandene Bretter und Steine zählen
    unsigned boards = 0;
    unsigned stones = 0;
    for(nobBaseWarehouse* wh : buildings.GetStorehouses())
    {
        boards += wh->GetInventory().goods[GoodType::Boards];
        stones += wh->GetInventory().goods[GoodType::Stones];
    }

    // Emergency happens, if we have less than 10 boards or stones...
    bool isNewEmergency = boards <= 10 || stones <= 10;
    // ...and no woddcutter or sawmill
    isNewEmergency &=
      buildings.GetBuildings(BuildingType::Woodcutter).empty() || buildings.GetBuildings(BuildingType::Sawmill).empty();

    // Wenn nötig, Notfallprogramm auslösen
    if(isNewEmergency)
    {
        if(!emergency)
        {
            emergency = true;
            SendPostMessage(std::make_unique<PostMsg>(
              gwg.GetEvMgr().GetCurrentGF(), _("The emergency program has been activated."), PostCategory::Economy));
        }
    } else
    {
        // Sobald Notfall vorbei, Notfallprogramm beenden, evtl. Baustellen wieder mit Kram versorgen
        if(emergency)
        {
            emergency = false;
            SendPostMessage(std::make_unique<PostMsg>(
              gwg.GetEvMgr().GetCurrentGF(), _("The emergency program has been deactivated."), PostCategory::Economy));
            FindMaterialForBuildingSites();
        }
    }
}

/// Testet die Bündnisse, ob sie nicht schon abgelaufen sind
void GamePlayer::TestPacts()
{
    for(unsigned i = 0; i < gwg.GetNumPlayers(); ++i)
    {
        if(i == GetPlayerId())
            continue;

        for(const auto pact : helpers::enumRange<PactType>())
        {
            // Pact not running
            if(pacts[i][pact].duration == 0)
                continue;
            if(GetPactState(pact, i) == PactState::None)
            {
                // Pact was running but is expired -> Cancel for both players
                pacts[i][pact].duration = 0;
                GamePlayer& otherPlayer = gwg.GetPlayer(i);
                RTTR_Assert(otherPlayer.pacts[GetPlayerId()][pact].duration);
                otherPlayer.pacts[GetPlayerId()][pact].duration = 0;
                // And notify
                PactChanged(pact);
                otherPlayer.PactChanged(pact);
            }
        }
    }
}

bool GamePlayer::CanBuildCatapult() const
{
    // Wenn AddonId::LIMIT_CATAPULTS nicht aktiv ist, bauen immer erlaubt
    if(!gwg.GetGGS().isEnabled(AddonId::LIMIT_CATAPULTS)) //-V807
        return true;

    BuildingCount bc = buildings.GetBuildingNums();

    unsigned max = 0;
    // proportional?
    if(gwg.GetGGS().getSelection(AddonId::LIMIT_CATAPULTS) == 1)
    {
        max = int(bc.buildings[BuildingType::Barracks] * 0.125 + bc.buildings[BuildingType::Guardhouse] * 0.25
                  + bc.buildings[BuildingType::Watchtower] * 0.5 + bc.buildings[BuildingType::Fortress]
                  + 0.111); // to avoid rounding errors
    } else if(gwg.GetGGS().getSelection(AddonId::LIMIT_CATAPULTS) < 8)
    {
        const std::array<unsigned, 6> limits = {{0, 3, 5, 10, 20, 30}};
        max = limits[gwg.GetGGS().getSelection(AddonId::LIMIT_CATAPULTS) - 2];
    }

    return bc.buildings[BuildingType::Catapult] + bc.buildingSites[BuildingType::Catapult] < max;
}

/// A ship has discovered new hostile territory --> determines if this is new
/// i.e. there is a sufficient distance to older locations
/// Returns true if yes and false if not
bool GamePlayer::ShipDiscoveredHostileTerritory(const MapPoint location)
{
    // Prüfen, ob Abstand zu bisherigen Punkten nicht zu klein
    for(const auto& enemies_discovered_by_ship : enemies_discovered_by_ships)
    {
        if(gwg.CalcDistance(enemies_discovered_by_ship, location) < 30)
            return false;
    }

    // Nein? Dann haben wir ein neues Territorium gefunden
    enemies_discovered_by_ships.push_back(location);

    return true;
}

/// For debug only
bool GamePlayer::IsDependentFigure(noFigure* fig)
{
    for(const nobBaseWarehouse* wh : buildings.GetStorehouses())
    {
        if(wh->IsDependentFigure(fig))
            return true;
    }
    return false;
}

std::vector<nobBaseWarehouse*> GamePlayer::GetWarehousesForTrading(const nobBaseWarehouse& goalWh) const
{
    std::vector<nobBaseWarehouse*> result;

    // Don't try to trade with us!
    if(goalWh.GetPlayer() == GetPlayerId())
        return result;

    const MapPoint goalFlagPos = goalWh.GetFlag()->GetPos();

    for(nobBaseWarehouse* wh : buildings.GetStorehouses())
    {
        // Is there a trade path from this warehouse to wh? (flag to flag)
        if(TradePathCache::inst().PathExists(gwg, wh->GetFlag()->GetPos(), goalFlagPos, GetPlayerId()))
            result.push_back(wh);
    }

    return result;
}

struct WarehouseDistanceComparator
{
    // Reference warehouse position, to which we want to calc the distance
    const MapPoint refWareHousePos_;
    /// GameWorld
    const GameWorldGame& gwg_;

    WarehouseDistanceComparator(const nobBaseWarehouse& refWareHouse, const GameWorldGame& gwg)
        : refWareHousePos_(refWareHouse.GetPos()), gwg_(gwg)
    {}

    bool operator()(nobBaseWarehouse* const wh1, nobBaseWarehouse* const wh2) const
    {
        unsigned dist1 = gwg_.CalcDistance(wh1->GetPos(), refWareHousePos_);
        unsigned dist2 = gwg_.CalcDistance(wh2->GetPos(), refWareHousePos_);
        return (dist1 < dist2) || (dist1 == dist2 && wh1->GetObjId() < wh2->GetObjId());
    }
};

/// Send wares to warehouse wh
void GamePlayer::Trade(nobBaseWarehouse* goalWh, const boost::variant<GoodType, Job>& what, unsigned count) const
{
    if(!gwg.GetGGS().isEnabled(AddonId::TRADE))
        return;

    if(count == 0)
        return;

    // Don't try to trade with us!
    if(goalWh->GetPlayer() == GetPlayerId())
        return;

    // No trades with enemies
    if(!IsAlly(goalWh->GetPlayer()))
        return;

    const MapPoint goalFlagPos = goalWh->GetFlag()->GetPos();

    std::vector<nobBaseWarehouse*> whs(buildings.GetStorehouses().begin(), buildings.GetStorehouses().end());
    std::sort(whs.begin(), whs.end(), WarehouseDistanceComparator(*goalWh, gwg));
    for(nobBaseWarehouse* wh : whs)
    {
        // Get available wares
        const unsigned available =
          boost::apply_visitor(composeVisitor([wh](GoodType gt) { return wh->GetAvailableWaresForTrading(gt); },
                                              [wh](Job job) { return wh->GetAvailableFiguresForTrading(job); }),
                               what);
        if(available == 0)
            continue;

        const unsigned actualCount = std::min(available, count);

        // Find a trade path from flag to flag
        TradeRoute tr(gwg, GetPlayerId(), wh->GetFlag()->GetPos(), goalFlagPos);

        // Found a path?
        if(tr.IsValid())
        {
            // Add to cache for future searches
            TradePathCache::inst().AddEntry(gwg, tr.GetTradePath(), GetPlayerId());

            wh->StartTradeCaravane(what, actualCount, tr, goalWh);
            count -= available;
            if(count == 0)
                return;
        }
    }
}

void GamePlayer::FillVisualSettings(VisualSettings& visualSettings) const
{
    Distributions& visDistribution = visualSettings.distribution;
    unsigned visIdx = 0;
    for(const DistributionMapping& mapping : distributionMap)
    {
        visDistribution[visIdx++] = distribution[std::get<0>(mapping)].percent_buildings[std::get<1>(mapping)];
    }

    visualSettings.useCustomBuildOrder = useCustomBuildOrder_;
    visualSettings.build_order = build_order;

    visualSettings.transport_order = GetOrderingFromTransportPrio(transportPrio);

    visualSettings.military_settings = militarySettings_;
    visualSettings.tools_settings = toolsSettings_;
}

#define INSTANTIATE_FINDWH(Cond)                                                                                \
    template nobBaseWarehouse* GamePlayer::FindWarehouse(const noRoadNode&, const Cond&, bool, bool, unsigned*, \
                                                         const RoadSegment*) const

INSTANTIATE_FINDWH(FW::HasMinWares);
INSTANTIATE_FINDWH(FW::HasFigure);
INSTANTIATE_FINDWH(FW::HasWareAndFigure);
INSTANTIATE_FINDWH(FW::HasMinSoldiers);
INSTANTIATE_FINDWH(FW::AcceptsWare);
INSTANTIATE_FINDWH(FW::AcceptsFigure);
INSTANTIATE_FINDWH(FW::CollectsWare);
INSTANTIATE_FINDWH(FW::CollectsFigure);
INSTANTIATE_FINDWH(FW::HasWareButNoCollect);
INSTANTIATE_FINDWH(FW::HasFigureButNoCollect);
INSTANTIATE_FINDWH(FW::AcceptsFigureButNoSend);
INSTANTIATE_FINDWH(FW::NoCondition);

#undef INSTANTIATE_FINDWH
