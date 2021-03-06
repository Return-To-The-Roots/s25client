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

#include "buildings/noBuildingSite.h"
#include "FOWObjects.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "Ware.h"
#include "figures/nofBuilder.h"
#include "figures/nofPlaner.h"
#include "helpers/containerUtils.h"
#include "helpers/toString.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "ogl/glSmartBitmap.h"
#include "world/GameWorld.h"
#include "gameData/BuildingConsts.h"
#include "gameData/MilitaryConsts.h"
#include "s25util/colors.h"
#include <stdexcept>

noBuildingSite::noBuildingSite(const BuildingType type, const MapPoint pos, const unsigned char player)
    : noBaseBuilding(NodalObjectType::Buildingsite, type, pos, player), state(BuildingSiteState::Building),
      planer(nullptr), builder(nullptr), boards(0), stones(0), used_boards(0), used_stones(0), build_progress(0)
{
    // Überprüfen, ob die Baustelle erst noch planiert werden muss (nur bei mittleren/großen Gebäuden)
    if(GetSize() == BuildingQuality::House || GetSize() == BuildingQuality::Castle
       || GetSize() == BuildingQuality::Harbor)
    {
        // Höhe auf dem Punkt, wo die Baustelle steht
        int altitude = world->GetNode(pos).altitude;

        for(const auto dir : helpers::EnumRange<Direction>{})
        {
            // Richtung 4 wird nicht planiert (Flagge)
            if(dir != Direction::SouthEast)
            {
                // Gibt es da Differenzen?
                if(altitude - world->GetNeighbourNode(pos, dir).altitude != 0)
                    state = BuildingSiteState::Planing;
            }
        }
    }

    // Wir hätten gerne einen Planierer/Bauarbeiter...
    world->GetPlayer(player).AddJobWanted((state == BuildingSiteState::Planing) ? Job::Planer : Job::Builder, this);

    // Bauwaren anfordern
    OrderConstructionMaterial();

    // Baustelle in den Index eintragen, damit die Wirtschaft auch Bescheid weiß
    world->GetPlayer(player).AddBuildingSite(this);
}

/// Konstruktor für Hafenbaustellen vom Schiff aus
noBuildingSite::noBuildingSite(const MapPoint pos, const unsigned char player)
    : noBaseBuilding(NodalObjectType::Buildingsite, BuildingType::HarborBuilding, pos, player),
      state(BuildingSiteState::Building), planer(nullptr), boards(BUILDING_COSTS[BuildingType::HarborBuilding].boards),
      stones(BUILDING_COSTS[BuildingType::HarborBuilding].stones), used_boards(0), used_stones(0), build_progress(0)
{
    GamePlayer& owner = world->GetPlayer(player);
    // Baustelle in den Index eintragen, damit die Wirtschaft auch Bescheid weiß
    owner.AddBuildingSite(this);
    // Bauarbeiter auch auf der Karte auftragen
    builder = &world->AddFigure(pos, std::make_unique<nofBuilder>(pos, player, this));

    // Baumaterialien in der Inventur verbuchen
    owner.DecreaseInventoryWare(GoodType::Boards, boards);
    owner.DecreaseInventoryWare(GoodType::Stones, stones);
}

noBuildingSite::~noBuildingSite() = default;

void noBuildingSite::Destroy()
{
    // Bauarbeiter/Planierer Bescheid sagen
    if(builder)
    {
        builder->LostWork();
        builder = nullptr;
    } else if(planer)
    {
        planer->LostWork();
        planer = nullptr;
    } else
        world->GetPlayer(player).JobNotWanted(this);

    RTTR_Assert(!builder);
    RTTR_Assert(!planer);

    // Bestellte Waren Bescheid sagen
    for(auto& ordered_board : ordered_boards)
        WareNotNeeded(ordered_board);
    ordered_boards.clear();
    for(auto& ordered_stone : ordered_stones)
        WareNotNeeded(ordered_stone);
    ordered_stones.clear();

    // und Feld wird leer
    world->SetNO(pos, nullptr);

    // Baustelle wieder aus der Liste entfernen - dont forget about expedition harbor status
    bool expeditionharbor = IsHarborBuildingSiteFromSea();
    world->GetPlayer(player).RemoveBuildingSite(this);

    noBaseBuilding::Destroy();

    // Hafenbaustelle?
    if(expeditionharbor)
    {
        world->RemoveHarborBuildingSiteFromSea(this);
        // Land neu berechnen nach zerstören weil da schon straßen etc entfernt werden
        world->RecalcTerritory(*this, TerritoryChangeReason::Destroyed);
    }
    world->RecalcBQAroundPointBig(pos);
}

void noBuildingSite::Serialize(SerializedGameData& sgd) const
{
    noBaseBuilding::Serialize(sgd);

    sgd.PushEnum<uint8_t>(state);
    sgd.PushObject(planer, true);
    sgd.PushObject(builder, true);
    sgd.PushUnsignedChar(boards);
    sgd.PushUnsignedChar(stones);
    sgd.PushUnsignedChar(used_boards);
    sgd.PushUnsignedChar(used_stones);
    sgd.PushUnsignedChar(build_progress);
    sgd.PushObjectContainer(ordered_boards, true);
    sgd.PushObjectContainer(ordered_stones, true);
}

noBuildingSite::noBuildingSite(SerializedGameData& sgd, const unsigned obj_id)
    : noBaseBuilding(sgd, obj_id), state(sgd.Pop<BuildingSiteState>()),
      planer(sgd.PopObject<nofPlaner>(GO_Type::NofPlaner)), builder(sgd.PopObject<nofBuilder>(GO_Type::NofBuilder)),
      boards(sgd.PopUnsignedChar()), stones(sgd.PopUnsignedChar()), used_boards(sgd.PopUnsignedChar()),
      used_stones(sgd.PopUnsignedChar()), build_progress(sgd.PopUnsignedChar())
{
    sgd.PopObjectContainer(ordered_boards, GO_Type::Ware);
    sgd.PopObjectContainer(ordered_stones, GO_Type::Ware);
}

void noBuildingSite::OrderConstructionMaterial()
{
    // Bei Planieren keine Waren bestellen
    if(state == BuildingSiteState::Planing)
        return;

    // Bretter
    GamePlayer& owner = world->GetPlayer(player);
    for(int i = used_boards + boards + ordered_boards.size(); i < BUILDING_COSTS[bldType_].boards; ++i)
    {
        Ware* w = owner.OrderWare(GoodType::Boards, this);
        if(!w)
            break;
        RTTR_Assert(helpers::contains(ordered_boards, w));
    }
    // Steine
    for(int i = used_stones + stones + ordered_stones.size(); i < BUILDING_COSTS[bldType_].stones; ++i)
    {
        Ware* w = owner.OrderWare(GoodType::Stones, this);
        if(!w)
            break;
        RTTR_Assert(helpers::contains(ordered_stones, w));
    }
}

unsigned noBuildingSite::GetMilitaryRadius() const
{
    /// Note: This actually only applies to harbor buildings made from expeditions. We rely on the calling functions to
    /// only take those into account
    return bldType_ == BuildingType::HarborBuilding ? HARBOR_RADIUS : 0;
}

void noBuildingSite::Draw(DrawPoint drawPt)
{
    if(state == BuildingSiteState::Planing)
    {
        // Baustellenschild mit Schatten zeichnen
        LOADER.GetNationImage(world->GetPlayer(player).nation, 450)->DrawFull(drawPt);
        LOADER.GetNationImage(world->GetPlayer(player).nation, 451)->DrawFull(drawPt, COLOR_SHADOW);
    } else
    {
        // Baustellenstein und -schatten zeichnen
        LOADER.GetNationImage(world->GetPlayer(player).nation, 455)->DrawFull(drawPt);
        LOADER.GetNationImage(world->GetPlayer(player).nation, 456)->DrawFull(drawPt, COLOR_SHADOW);

        // Waren auf der Baustelle

        // Bretter
        DrawPoint doorPos = drawPt + DrawPoint(GetDoorPointX(), GetDoorPointY());
        for(unsigned char i = 0; i < boards; ++i)
            LOADER.GetMapImageN(WARE_STACK_TEX_MAP_OFFSET + rttr::enum_cast(GoodType::Boards))
              ->DrawFull(doorPos - DrawPoint(5, 10 + i * 4));
        // Steine
        for(unsigned char i = 0; i < stones; ++i)
            LOADER.GetMapImageN(WARE_STACK_TEX_MAP_OFFSET + rttr::enum_cast(GoodType::Stones))
              ->DrawFull(doorPos + DrawPoint(8, -12 - i * 4));

        // bis dahin gebautes Haus zeichnen

        // Rohbau

        // ausrechnen, wie weit er ist
        unsigned progressRaw, progressBld;
        unsigned maxProgressRaw, maxProgressBld;

        if(BUILDING_COSTS[bldType_].stones)
        {
            // Haus besteht aus Steinen und Brettern
            maxProgressRaw = BUILDING_COSTS[bldType_].boards * 8;
            maxProgressBld = BUILDING_COSTS[bldType_].stones * 8;
        } else
        {
            // Haus besteht nur aus Brettern, dann 50:50
            maxProgressBld = maxProgressRaw = BUILDING_COSTS[bldType_].boards * 4;
        }
        progressRaw = std::min<unsigned>(build_progress, maxProgressRaw);
        progressBld = ((build_progress > maxProgressRaw) ? (build_progress - maxProgressRaw) : 0);

        LOADER.building_cache[nation][bldType_].skeleton.drawPercent(drawPt, progressRaw * 100 / maxProgressRaw);
        LOADER.building_cache[nation][bldType_].building.drawPercent(drawPt, progressBld * 100 / maxProgressBld);
    }
}

/// Erzeugt von ihnen selbst ein FOW Objekt als visuelle "Erinnerung" für den Fog of War
std::unique_ptr<FOWObject> noBuildingSite::CreateFOWObject() const
{
    return std::make_unique<fowBuildingSite>(state == BuildingSiteState::Planing, bldType_, nation, build_progress);
}

void noBuildingSite::GotWorker(Job /*job*/, noFigure& worker)
{
    // Aha, wir haben nen Planierer/Bauarbeiter bekommen
    if(state == BuildingSiteState::Planing)
    {
        RTTR_Assert(worker.GetGOT() == GO_Type::NofPlaner);
        planer = static_cast<nofPlaner*>(&worker);
    } else
    {
        RTTR_Assert(worker.GetGOT() == GO_Type::NofBuilder);
        builder = static_cast<nofBuilder*>(&worker);
    }
}

void noBuildingSite::Abrogate()
{
    planer = nullptr;
    builder = nullptr;

    world->GetPlayer(player).AddJobWanted((state == BuildingSiteState::Planing) ? Job::Planer : Job::Builder, this);
}

unsigned noBuildingSite::CalcDistributionPoints(const GoodType goodtype)
{
    // Beim Planieren brauchen wir noch gar nichts
    if(state == BuildingSiteState::Planing)
        return 0;

    // We only need boards and stones.
    if(goodtype != GoodType::Boards && goodtype != GoodType::Stones)
        return 0;

    const unsigned curBoards = ordered_boards.size() + boards + used_boards;
    const unsigned curStones = ordered_stones.size() + stones + used_stones;
    RTTR_Assert(curBoards <= BUILDING_COSTS[this->bldType_].boards);
    RTTR_Assert(curStones <= BUILDING_COSTS[this->bldType_].stones);

    // Wenn wir schon genug Baumaterial haben, brauchen wir nichts mehr
    if((goodtype == GoodType::Boards && curBoards == BUILDING_COSTS[this->bldType_].boards)
       || (goodtype == GoodType::Stones && curStones == BUILDING_COSTS[this->bldType_].stones))
        return 0;

    // 10000 als Basis wählen, damit man auch noch was abziehen kann
    constexpr unsigned basePoints = 10000;
    unsigned points = basePoints;

    // Baumaterial mit einberechnen (wer noch am wenigsten braucht, soll mehr Punkte kriegen, da ja möglichst
    // zuerst Gebäude fertiggestellt werden sollten)
    points -= (BUILDING_COSTS[bldType_].boards - curBoards) * 20;
    points -= (BUILDING_COSTS[bldType_].stones - curStones) * 20;

    // Baupriorität mit einberechnen (niedriger = höhere Priorität, daher - !)
    const unsigned buildingSitePrio = world->GetPlayer(player).GetBuidingSitePriority(this) * 30;

    if(points > buildingSitePrio)
        points -= buildingSitePrio;
    else
        points = 0;

    RTTR_Assert(points <= basePoints); // Underflow detection. Should never happen...

    return points;
}

void noBuildingSite::AddWare(std::unique_ptr<Ware> ware)
{
    RTTR_Assert(state == BuildingSiteState::Building);

    if(ware->type == GoodType::Boards)
    {
        RTTR_Assert(helpers::contains(ordered_boards, ware.get()));
        ordered_boards.remove(ware.get());
        ++boards;
    } else if(ware->type == GoodType::Stones)
    {
        RTTR_Assert(helpers::contains(ordered_stones, ware.get()));
        ordered_stones.remove(ware.get());
        ++stones;
    } else
        throw std::logic_error("Wrong ware type " + helpers::toString(ware->type));

    // Inventur entsprechend verringern
    world->GetPlayer(player).DecreaseInventoryWare(ware->type, 1);
    world->GetPlayer(player).RemoveWare(*ware);
}

void noBuildingSite::WareLost(Ware& ware)
{
    RTTR_Assert(state == BuildingSiteState::Building);

    if(ware.type == GoodType::Boards)
    {
        RTTR_Assert(helpers::contains(ordered_boards, &ware));
        ordered_boards.remove(&ware);
    } else if(ware.type == GoodType::Stones)
    {
        RTTR_Assert(helpers::contains(ordered_stones, &ware));
        ordered_stones.remove(&ware);
    } else
        throw std::logic_error("Wrong ware type lost " + helpers::toString(ware.type));

    OrderConstructionMaterial();
}

void noBuildingSite::TakeWare(Ware* ware)
{
    RTTR_Assert(state == BuildingSiteState::Building);

    // Ware in die Bestellliste aufnehmen
    if(ware->type == GoodType::Boards)
    {
        RTTR_Assert(!helpers::contains(ordered_boards, ware));
        ordered_boards.push_back(ware);
    } else if(ware->type == GoodType::Stones)
    {
        RTTR_Assert(!helpers::contains(ordered_stones, ware));
        ordered_stones.push_back(ware);
    } else
        throw std::logic_error("Wrong ware type " + helpers::toString(ware->type));
}

bool noBuildingSite::IsBuildingComplete()
{
    return (build_progress == BUILDING_COSTS[bldType_].boards * 8 + BUILDING_COSTS[bldType_].stones * 8);
}

unsigned char noBuildingSite::GetBuildProgress(bool percent) const
{
    if(!percent)
        return build_progress;

    unsigned costs = BUILDING_COSTS[bldType_].boards * 8 + BUILDING_COSTS[bldType_].stones * 8;
    unsigned progress = (((unsigned)build_progress) * 100) / costs;

    return (unsigned char)progress;
}

/// Aufgerufen, wenn Planierung beendet wurde
void noBuildingSite::PlaningFinished()
{
    /// Normale Baustelle
    state = BuildingSiteState::Building;
    planer = nullptr;

    // Wir hätten gerne einen Bauarbeiter...
    world->GetPlayer(player).AddJobWanted(Job::Builder, this);

    // Bauwaren anfordern
    OrderConstructionMaterial();
}

/// Gibt zurück, ob eine bestimmte Baustellen eine Baustelle ist, die vom Schiff aus errichtet wurde
bool noBuildingSite::IsHarborBuildingSiteFromSea() const
{
    if(this->bldType_ == BuildingType::HarborBuilding)
        return world->IsHarborBuildingSiteFromSea(this);
    else
        return false;
}
