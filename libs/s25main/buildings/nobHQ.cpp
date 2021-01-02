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

#include "nobHQ.h"
#include "GamePlayer.h"
#include "GlobalGameSettings.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "network/GameClient.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "world/GameWorldGame.h"
#include "gameData/MilitaryConsts.h"
#include <numeric>

nobHQ::nobHQ(const MapPoint pos, const unsigned char player, const Nation nation, const bool isTent)
    : nobBaseWarehouse(BuildingType::Headquarters, pos, player, nation), isTent_(isTent)
{
    // StartWaren setzen
    switch(gwg->GetGGS().startWares)
    {
        case StartWares::VLow:
            inventory.visual.goods[GoodType::Beer] = 0;
            inventory.visual.goods[GoodType::Tongs] = 1;
            inventory.visual.goods[GoodType::Hammer] = 4;
            inventory.visual.goods[GoodType::Axe] = 1;
            inventory.visual.goods[GoodType::Saw] = 0;
            inventory.visual.goods[GoodType::PickAxe] = 0;
            inventory.visual.goods[GoodType::Shovel] = 1;
            inventory.visual.goods[GoodType::Crucible] = 1;
            inventory.visual.goods[GoodType::RodAndLine] = 1; //??
            inventory.visual.goods[GoodType::Scythe] = 2;     //??
            inventory.visual.goods[GoodType::WaterEmpty] = 0;
            inventory.visual.goods[GoodType::Water] = 0;
            inventory.visual.goods[GoodType::Cleaver] = 0;
            inventory.visual.goods[GoodType::Rollingpin] = 1;
            inventory.visual.goods[GoodType::Bow] = 0;
            inventory.visual.goods[GoodType::Boat] = 0;
            inventory.visual.goods[GoodType::Sword] = 0;
            inventory.visual.goods[GoodType::Iron] = 0;
            inventory.visual.goods[GoodType::Flour] = 0;
            inventory.visual.goods[GoodType::Fish] = 1;
            inventory.visual.goods[GoodType::Bread] = 2;
            inventory.visual.goods[GoodType::ShieldRomans] = 0;
            inventory.visual.goods[GoodType::Wood] = 6;
            inventory.visual.goods[GoodType::Boards] = 11;
            inventory.visual.goods[GoodType::Stones] = 17;
            inventory.visual.goods[GoodType::ShieldVikings] = 0;
            inventory.visual.goods[GoodType::ShieldAfricans] = 0;
            inventory.visual.goods[GoodType::Grain] = 0;
            inventory.visual.goods[GoodType::Coins] = 0;
            inventory.visual.goods[GoodType::Gold] = 0;
            inventory.visual.goods[GoodType::IronOre] = 4;
            inventory.visual.goods[GoodType::Coal] = 4;
            inventory.visual.goods[GoodType::Meat] = 0;
            inventory.visual.goods[GoodType::Ham] = 0;
            inventory.visual.goods[GoodType::ShieldJapanese] = 0;

            inventory.visual.people[Job::Helper] = 13;
            inventory.visual.people[Job::Woodcutter] = 2;
            inventory.visual.people[Job::Fisher] = 0;
            inventory.visual.people[Job::Forester] = 1;
            inventory.visual.people[Job::Carpenter] = 1;
            inventory.visual.people[Job::Stonemason] = 1;
            inventory.visual.people[Job::Hunter] = 1;
            inventory.visual.people[Job::Farmer] = 0;
            inventory.visual.people[Job::Miller] = 0;
            inventory.visual.people[Job::Baker] = 0;
            inventory.visual.people[Job::Butcher] = 0;
            inventory.visual.people[Job::Miner] = 2;
            inventory.visual.people[Job::Brewer] = 0;
            inventory.visual.people[Job::PigBreeder] = 0;
            inventory.visual.people[Job::DonkeyBreeder] = 0;
            inventory.visual.people[Job::IronFounder] = 0;
            inventory.visual.people[Job::Minter] = 0;
            inventory.visual.people[Job::Metalworker] = 0;
            inventory.visual.people[Job::Armorer] = 1;
            inventory.visual.people[Job::Builder] = 2;
            inventory.visual.people[Job::Planer] = 1;
            inventory.visual.people[Job::Private] = 13;
            inventory.visual.people[Job::PrivateFirstClass] = 0;
            inventory.visual.people[Job::Sergeant] = 0;
            inventory.visual.people[Job::Officer] = 0;
            inventory.visual.people[Job::General] = 0;
            inventory.visual.people[Job::Geologist] = 2;
            inventory.visual.people[Job::Shipwright] = 0;
            inventory.visual.people[Job::Scout] = 1;
            inventory.visual.people[Job::PackDonkey] = 2;
            break;

        case StartWares::Low:

            inventory.visual.goods[GoodType::Beer] = 0;
            inventory.visual.goods[GoodType::Tongs] = 0;
            inventory.visual.goods[GoodType::Hammer] = 8;
            inventory.visual.goods[GoodType::Axe] = 3;
            inventory.visual.goods[GoodType::Saw] = 1;
            inventory.visual.goods[GoodType::PickAxe] = 1;
            inventory.visual.goods[GoodType::Shovel] = 2;
            inventory.visual.goods[GoodType::Crucible] = 2;
            inventory.visual.goods[GoodType::RodAndLine] = 3;
            inventory.visual.goods[GoodType::Scythe] = 4;
            inventory.visual.goods[GoodType::WaterEmpty] = 0;
            inventory.visual.goods[GoodType::Water] = 0;
            inventory.visual.goods[GoodType::Cleaver] = 1;
            inventory.visual.goods[GoodType::Rollingpin] = 1;
            inventory.visual.goods[GoodType::Bow] = 1;
            inventory.visual.goods[GoodType::Boat] = 6;
            inventory.visual.goods[GoodType::Sword] = 0;
            inventory.visual.goods[GoodType::Iron] = 0;
            inventory.visual.goods[GoodType::Flour] = 0;
            inventory.visual.goods[GoodType::Fish] = 2;
            inventory.visual.goods[GoodType::Bread] = 4;
            inventory.visual.goods[GoodType::ShieldRomans] = 0;
            inventory.visual.goods[GoodType::Wood] = 12;
            inventory.visual.goods[GoodType::Boards] = 22;
            inventory.visual.goods[GoodType::Stones] = 34;
            inventory.visual.goods[GoodType::ShieldVikings] = 0;
            inventory.visual.goods[GoodType::ShieldAfricans] = 0;
            inventory.visual.goods[GoodType::Grain] = 0;
            inventory.visual.goods[GoodType::Coins] = 0;
            inventory.visual.goods[GoodType::Gold] = 0;
            inventory.visual.goods[GoodType::IronOre] = 8;
            inventory.visual.goods[GoodType::Coal] = 8;
            inventory.visual.goods[GoodType::Meat] = 3;
            inventory.visual.goods[GoodType::Ham] = 0;
            inventory.visual.goods[GoodType::ShieldJapanese] = 0;

            inventory.visual.people[Job::Helper] = 26;
            inventory.visual.people[Job::Woodcutter] = 4;
            inventory.visual.people[Job::Fisher] = 0;
            inventory.visual.people[Job::Forester] = 2;
            inventory.visual.people[Job::Carpenter] = 2;
            inventory.visual.people[Job::Stonemason] = 2;
            inventory.visual.people[Job::Hunter] = 1;
            inventory.visual.people[Job::Farmer] = 0;
            inventory.visual.people[Job::Miller] = 0;
            inventory.visual.people[Job::Baker] = 0;
            inventory.visual.people[Job::Butcher] = 0;
            inventory.visual.people[Job::Miner] = 5;
            inventory.visual.people[Job::Brewer] = 0;
            inventory.visual.people[Job::PigBreeder] = 0;
            inventory.visual.people[Job::DonkeyBreeder] = 0;
            inventory.visual.people[Job::IronFounder] = 0;
            inventory.visual.people[Job::Minter] = 0;
            inventory.visual.people[Job::Metalworker] = 1;
            inventory.visual.people[Job::Armorer] = 2;
            inventory.visual.people[Job::Builder] = 5;
            inventory.visual.people[Job::Planer] = 3;
            inventory.visual.people[Job::Private] = 26;
            inventory.visual.people[Job::PrivateFirstClass] = 0;
            inventory.visual.people[Job::Sergeant] = 0;
            inventory.visual.people[Job::Officer] = 0;
            inventory.visual.people[Job::General] = 0;
            inventory.visual.people[Job::Geologist] = 3;
            inventory.visual.people[Job::Shipwright] = 0;
            inventory.visual.people[Job::Scout] = 1;
            inventory.visual.people[Job::PackDonkey] = 4;
            break;

        case StartWares::Normal:

            inventory.visual.goods[GoodType::Beer] = 6;
            inventory.visual.goods[GoodType::Tongs] = 0;
            inventory.visual.goods[GoodType::Hammer] = 16;
            inventory.visual.goods[GoodType::Axe] = 6;
            inventory.visual.goods[GoodType::Saw] = 2;
            inventory.visual.goods[GoodType::PickAxe] = 2;
            inventory.visual.goods[GoodType::Shovel] = 4;
            inventory.visual.goods[GoodType::Crucible] = 4;
            inventory.visual.goods[GoodType::RodAndLine] = 6;
            inventory.visual.goods[GoodType::Scythe] = 8;
            inventory.visual.goods[GoodType::WaterEmpty] = 0;
            inventory.visual.goods[GoodType::Water] = 0;
            inventory.visual.goods[GoodType::Cleaver] = 2;
            inventory.visual.goods[GoodType::Rollingpin] = 2;
            inventory.visual.goods[GoodType::Bow] = 2;
            inventory.visual.goods[GoodType::Boat] = 12;
            inventory.visual.goods[GoodType::Sword] = 6;
            inventory.visual.goods[GoodType::Iron] = 0;
            inventory.visual.goods[GoodType::Flour] = 0;
            inventory.visual.goods[GoodType::Fish] = 4;
            inventory.visual.goods[GoodType::Bread] = 8;
            inventory.visual.goods[GoodType::ShieldRomans] = 6;
            inventory.visual.goods[GoodType::Wood] = 24;
            inventory.visual.goods[GoodType::Boards] = 44;
            inventory.visual.goods[GoodType::Stones] = 68;
            inventory.visual.goods[GoodType::ShieldVikings] = 0;
            inventory.visual.goods[GoodType::ShieldAfricans] = 0;
            inventory.visual.goods[GoodType::Grain] = 0;
            inventory.visual.goods[GoodType::Coins] = 0;
            inventory.visual.goods[GoodType::Gold] = 0;
            inventory.visual.goods[GoodType::IronOre] = 16;
            inventory.visual.goods[GoodType::Coal] = 16;
            inventory.visual.goods[GoodType::Meat] = 6;
            inventory.visual.goods[GoodType::Ham] = 0;
            inventory.visual.goods[GoodType::ShieldJapanese] = 0;

            inventory.visual.people[Job::Helper] = 52;
            inventory.visual.people[Job::Woodcutter] = 8;
            inventory.visual.people[Job::Fisher] = 0;
            inventory.visual.people[Job::Forester] = 4;
            inventory.visual.people[Job::Carpenter] = 4;
            inventory.visual.people[Job::Stonemason] = 4;
            inventory.visual.people[Job::Hunter] = 2;
            inventory.visual.people[Job::Farmer] = 0;
            inventory.visual.people[Job::Miller] = 0;
            inventory.visual.people[Job::Baker] = 0;
            inventory.visual.people[Job::Butcher] = 0;
            inventory.visual.people[Job::Miner] = 10;
            inventory.visual.people[Job::Brewer] = 0;
            inventory.visual.people[Job::PigBreeder] = 0;
            inventory.visual.people[Job::DonkeyBreeder] = 0;
            inventory.visual.people[Job::IronFounder] = 0;
            inventory.visual.people[Job::Minter] = 0;
            inventory.visual.people[Job::Metalworker] = 2;
            inventory.visual.people[Job::Armorer] = 4;
            inventory.visual.people[Job::Builder] = 10;
            inventory.visual.people[Job::Planer] = 6;
            inventory.visual.people[Job::Private] = 46;
            inventory.visual.people[Job::PrivateFirstClass] = 0;
            inventory.visual.people[Job::Sergeant] = 0;
            inventory.visual.people[Job::Officer] = 0;
            inventory.visual.people[Job::General] = 0;
            inventory.visual.people[Job::Geologist] = 6;
            inventory.visual.people[Job::Shipwright] = 0;
            inventory.visual.people[Job::Scout] = 2;
            inventory.visual.people[Job::PackDonkey] = 8;
            break;

        case StartWares::ALot:
            inventory.visual.goods[GoodType::Beer] = 12;
            inventory.visual.goods[GoodType::Tongs] = 0;
            inventory.visual.goods[GoodType::Hammer] = 32;
            inventory.visual.goods[GoodType::Axe] = 12;
            inventory.visual.goods[GoodType::Saw] = 4;
            inventory.visual.goods[GoodType::PickAxe] = 4;
            inventory.visual.goods[GoodType::Shovel] = 8;
            inventory.visual.goods[GoodType::Crucible] = 8;
            inventory.visual.goods[GoodType::RodAndLine] = 12;
            inventory.visual.goods[GoodType::Scythe] = 16;
            inventory.visual.goods[GoodType::WaterEmpty] = 0;
            inventory.visual.goods[GoodType::Water] = 0;
            inventory.visual.goods[GoodType::Cleaver] = 4;
            inventory.visual.goods[GoodType::Rollingpin] = 4;
            inventory.visual.goods[GoodType::Bow] = 4;
            inventory.visual.goods[GoodType::Boat] = 24;
            inventory.visual.goods[GoodType::Sword] = 12;
            inventory.visual.goods[GoodType::Iron] = 0;
            inventory.visual.goods[GoodType::Flour] = 0;
            inventory.visual.goods[GoodType::Fish] = 8;
            inventory.visual.goods[GoodType::Bread] = 16;
            inventory.visual.goods[GoodType::ShieldRomans] = 12;
            inventory.visual.goods[GoodType::Wood] = 48;
            inventory.visual.goods[GoodType::Boards] = 88;
            inventory.visual.goods[GoodType::Stones] = 136;
            inventory.visual.goods[GoodType::ShieldVikings] = 0;
            inventory.visual.goods[GoodType::ShieldAfricans] = 0;
            inventory.visual.goods[GoodType::Grain] = 0;
            inventory.visual.goods[GoodType::Coins] = 0;
            inventory.visual.goods[GoodType::Gold] = 0;
            inventory.visual.goods[GoodType::IronOre] = 32;
            inventory.visual.goods[GoodType::Coal] = 32;
            inventory.visual.goods[GoodType::Meat] = 12;
            inventory.visual.goods[GoodType::Ham] = 0;
            inventory.visual.goods[GoodType::ShieldJapanese] = 0;

            inventory.visual.people[Job::Helper] = 104;
            inventory.visual.people[Job::Woodcutter] = 16;
            inventory.visual.people[Job::Fisher] = 0;
            inventory.visual.people[Job::Forester] = 8;
            inventory.visual.people[Job::Carpenter] = 8;
            inventory.visual.people[Job::Stonemason] = 8;
            inventory.visual.people[Job::Hunter] = 4;
            inventory.visual.people[Job::Farmer] = 0;
            inventory.visual.people[Job::Miller] = 0;
            inventory.visual.people[Job::Baker] = 0;
            inventory.visual.people[Job::Butcher] = 0;
            inventory.visual.people[Job::Miner] = 20;
            inventory.visual.people[Job::Brewer] = 0;
            inventory.visual.people[Job::PigBreeder] = 0;
            inventory.visual.people[Job::DonkeyBreeder] = 0;
            inventory.visual.people[Job::IronFounder] = 0;
            inventory.visual.people[Job::Minter] = 0;
            inventory.visual.people[Job::Metalworker] = 4;
            inventory.visual.people[Job::Armorer] = 8;
            inventory.visual.people[Job::Builder] = 20;
            inventory.visual.people[Job::Planer] = 12;
            inventory.visual.people[Job::Private] = 92;
            inventory.visual.people[Job::PrivateFirstClass] = 0;
            inventory.visual.people[Job::Sergeant] = 0;
            inventory.visual.people[Job::Officer] = 0;
            inventory.visual.people[Job::General] = 0;
            inventory.visual.people[Job::Geologist] = 12;
            inventory.visual.people[Job::Shipwright] = 0;
            inventory.visual.people[Job::Scout] = 4;
            inventory.visual.people[Job::PackDonkey] = 16;
            break;
    }

    inventory.real = inventory.visual;

    // Aktuellen Warenbestand zur aktuellen Inventur dazu addieren
    AddToInventory();

    // Take 1 as the reserve per rank
    for(unsigned i = 0; i <= gwg->GetGGS().GetMaxMilitaryRank(); ++i)
    {
        reserve_soldiers_claimed_visual[i] = reserve_soldiers_claimed_real[i] = 1;
        RefreshReserve(i);
    }

    // Evtl. liegen am Anfang Waffen im HQ, sodass rekrutiert werden muss
    TryRecruiting();

    // ins Militärquadrat einfügen
    gwg->GetMilitarySquares().Add(this);
    gwg->RecalcTerritory(*this, TerritoryChangeReason::Build);
}

void nobHQ::DestroyBuilding()
{
    nobBaseWarehouse::DestroyBuilding();
    // Wieder aus dem Militärquadrat rauswerfen
    gwg->GetMilitarySquares().Remove(this);
    // Recalc territory. AFTER calling base destroy as otherwise figures might get stuck here
    gwg->RecalcTerritory(*this, TerritoryChangeReason::Destroyed);
}

void nobHQ::Serialize_nobHQ(SerializedGameData& sgd) const
{
    Serialize_nobBaseWarehouse(sgd);
    sgd.PushBool(isTent_);
}

nobHQ::nobHQ(SerializedGameData& sgd, const unsigned obj_id) : nobBaseWarehouse(sgd, obj_id), isTent_(sgd.PopBool())
{
    gwg->GetMilitarySquares().Add(this);
}

void nobHQ::Draw(DrawPoint drawPt)
{
    if(isTent_)
        LOADER.building_cache[nation][BuildingType::Headquarters].skeleton.draw(drawPt);
    else
    {
        DrawBaseBuilding(drawPt);

        // Draw at most 4 flags
        const unsigned numSoldiers =
          std::accumulate(reserve_soldiers_available.begin(), reserve_soldiers_available.end(), GetNumSoldiers());
        DrawPoint flagsPos = drawPt + TROOPS_FLAG_HQ_OFFSET[nation];
        for(auto i = std::min<unsigned>(numSoldiers, 4); i; --i)
        {
            glArchivItem_Bitmap_Player* bitmap =
              LOADER.GetMapPlayerImage(3162 + GAMECLIENT.GetGlobalAnimation(8, 80, 40, GetX() * GetY() * i));
            if(bitmap)
                bitmap->DrawFull(flagsPos + DrawPoint(0, (i - 1) * 3), COLOR_WHITE, gwg->GetPlayer(player).color);
        }
    }
}

void nobHQ::HandleEvent(const unsigned id)
{
    HandleBaseEvent(id);
}
