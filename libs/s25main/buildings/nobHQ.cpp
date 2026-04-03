// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nobHQ.h"
#include "GamePlayer.h"
#include "GlobalGameSettings.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "network/GameClient.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "world/GameWorld.h"
#include "gameData/MilitaryConsts.h"
#include <numeric>

nobHQ::nobHQ(const MapPoint pos, const unsigned char player, const Nation nation, const bool isTent)
    : nobBaseWarehouse(BuildingType::Headquarters, pos, player, nation), isTent_(isTent)
{
    // Take 1 as the reserve per rank
    for(unsigned i = 0; i <= world->GetGGS().GetMaxMilitaryRank(); ++i)
    {
        reserve_soldiers_claimed_visual[i] = reserve_soldiers_claimed_real[i] = 1;
        RefreshReserve(i);
    }

    // Evtl. liegen am Anfang Waffen im HQ, sodass rekrutiert werden muss
    TryRecruiting();

    // ins Militärquadrat einfügen
    world->GetMilitarySquares().Add(this);
    world->RecalcTerritory(*this, TerritoryChangeReason::Build);
}

GoodsAndPeopleCounts nobHQ::getStartInventory(StartWares setting)
{
    GoodsAndPeopleCounts inventory;
    switch(setting)
    {
        case StartWares::VLow:
            inventory[GoodType::Beer] = 0;
            inventory[GoodType::Tongs] = 1;
            inventory[GoodType::Hammer] = 4;
            inventory[GoodType::Axe] = 1;
            inventory[GoodType::Saw] = 0;
            inventory[GoodType::PickAxe] = 0;
            inventory[GoodType::Shovel] = 1;
            inventory[GoodType::Crucible] = 1;
            inventory[GoodType::RodAndLine] = 1; //??
            inventory[GoodType::Scythe] = 2;     //??
            inventory[GoodType::WaterEmpty] = 0;
            inventory[GoodType::Water] = 0;
            inventory[GoodType::Cleaver] = 0;
            inventory[GoodType::Rollingpin] = 1;
            inventory[GoodType::Bow] = 0;
            inventory[GoodType::Boat] = 0;
            inventory[GoodType::Sword] = 0;
            inventory[GoodType::Iron] = 0;
            inventory[GoodType::Flour] = 0;
            inventory[GoodType::Fish] = 1;
            inventory[GoodType::Bread] = 2;
            inventory[GoodType::ShieldRomans] = 0;
            inventory[GoodType::Wood] = 6;
            inventory[GoodType::Boards] = 11;
            inventory[GoodType::Stones] = 17;
            inventory[GoodType::ShieldVikings] = 0;
            inventory[GoodType::ShieldAfricans] = 0;
            inventory[GoodType::Grain] = 0;
            inventory[GoodType::Coins] = 0;
            inventory[GoodType::Gold] = 0;
            inventory[GoodType::IronOre] = 4;
            inventory[GoodType::Coal] = 4;
            inventory[GoodType::Meat] = 0;
            inventory[GoodType::Ham] = 0;
            inventory[GoodType::ShieldJapanese] = 0;

            inventory[Job::Helper] = 13;
            inventory[Job::Woodcutter] = 2;
            inventory[Job::Fisher] = 0;
            inventory[Job::Forester] = 1;
            inventory[Job::Carpenter] = 1;
            inventory[Job::Stonemason] = 1;
            inventory[Job::Hunter] = 1;
            inventory[Job::Farmer] = 0;
            inventory[Job::Miller] = 0;
            inventory[Job::Baker] = 0;
            inventory[Job::Butcher] = 0;
            inventory[Job::Miner] = 2;
            inventory[Job::Brewer] = 0;
            inventory[Job::PigBreeder] = 0;
            inventory[Job::DonkeyBreeder] = 0;
            inventory[Job::IronFounder] = 0;
            inventory[Job::Minter] = 0;
            inventory[Job::Metalworker] = 0;
            inventory[Job::Armorer] = 1;
            inventory[Job::Builder] = 2;
            inventory[Job::Planer] = 1;
            inventory[Job::Private] = 13;
            inventory[Job::PrivateFirstClass] = 0;
            inventory[Job::Sergeant] = 0;
            inventory[Job::Officer] = 0;
            inventory[Job::General] = 0;
            inventory[Job::Geologist] = 2;
            inventory[Job::Shipwright] = 0;
            inventory[Job::Scout] = 1;
            inventory[Job::PackDonkey] = 2;
            break;

        case StartWares::Low:

            inventory[GoodType::Beer] = 0;
            inventory[GoodType::Tongs] = 0;
            inventory[GoodType::Hammer] = 8;
            inventory[GoodType::Axe] = 3;
            inventory[GoodType::Saw] = 1;
            inventory[GoodType::PickAxe] = 1;
            inventory[GoodType::Shovel] = 2;
            inventory[GoodType::Crucible] = 2;
            inventory[GoodType::RodAndLine] = 3;
            inventory[GoodType::Scythe] = 4;
            inventory[GoodType::WaterEmpty] = 0;
            inventory[GoodType::Water] = 0;
            inventory[GoodType::Cleaver] = 1;
            inventory[GoodType::Rollingpin] = 1;
            inventory[GoodType::Bow] = 1;
            inventory[GoodType::Boat] = 6;
            inventory[GoodType::Sword] = 0;
            inventory[GoodType::Iron] = 0;
            inventory[GoodType::Flour] = 0;
            inventory[GoodType::Fish] = 2;
            inventory[GoodType::Bread] = 4;
            inventory[GoodType::ShieldRomans] = 0;
            inventory[GoodType::Wood] = 12;
            inventory[GoodType::Boards] = 22;
            inventory[GoodType::Stones] = 34;
            inventory[GoodType::ShieldVikings] = 0;
            inventory[GoodType::ShieldAfricans] = 0;
            inventory[GoodType::Grain] = 0;
            inventory[GoodType::Coins] = 0;
            inventory[GoodType::Gold] = 0;
            inventory[GoodType::IronOre] = 8;
            inventory[GoodType::Coal] = 8;
            inventory[GoodType::Meat] = 3;
            inventory[GoodType::Ham] = 0;
            inventory[GoodType::ShieldJapanese] = 0;

            inventory[Job::Helper] = 26;
            inventory[Job::Woodcutter] = 4;
            inventory[Job::Fisher] = 0;
            inventory[Job::Forester] = 2;
            inventory[Job::Carpenter] = 2;
            inventory[Job::Stonemason] = 2;
            inventory[Job::Hunter] = 1;
            inventory[Job::Farmer] = 0;
            inventory[Job::Miller] = 0;
            inventory[Job::Baker] = 0;
            inventory[Job::Butcher] = 0;
            inventory[Job::Miner] = 5;
            inventory[Job::Brewer] = 0;
            inventory[Job::PigBreeder] = 0;
            inventory[Job::DonkeyBreeder] = 0;
            inventory[Job::IronFounder] = 0;
            inventory[Job::Minter] = 0;
            inventory[Job::Metalworker] = 1;
            inventory[Job::Armorer] = 2;
            inventory[Job::Builder] = 5;
            inventory[Job::Planer] = 3;
            inventory[Job::Private] = 26;
            inventory[Job::PrivateFirstClass] = 0;
            inventory[Job::Sergeant] = 0;
            inventory[Job::Officer] = 0;
            inventory[Job::General] = 0;
            inventory[Job::Geologist] = 3;
            inventory[Job::Shipwright] = 0;
            inventory[Job::Scout] = 1;
            inventory[Job::PackDonkey] = 4;
            break;

        case StartWares::Normal:

            inventory[GoodType::Beer] = 6;
            inventory[GoodType::Tongs] = 0;
            inventory[GoodType::Hammer] = 16;
            inventory[GoodType::Axe] = 6;
            inventory[GoodType::Saw] = 2;
            inventory[GoodType::PickAxe] = 2;
            inventory[GoodType::Shovel] = 4;
            inventory[GoodType::Crucible] = 4;
            inventory[GoodType::RodAndLine] = 6;
            inventory[GoodType::Scythe] = 8;
            inventory[GoodType::WaterEmpty] = 0;
            inventory[GoodType::Water] = 0;
            inventory[GoodType::Cleaver] = 2;
            inventory[GoodType::Rollingpin] = 2;
            inventory[GoodType::Bow] = 2;
            inventory[GoodType::Boat] = 12;
            inventory[GoodType::Sword] = 6;
            inventory[GoodType::Iron] = 0;
            inventory[GoodType::Flour] = 0;
            inventory[GoodType::Fish] = 4;
            inventory[GoodType::Bread] = 8;
            inventory[GoodType::ShieldRomans] = 6;
            inventory[GoodType::Wood] = 24;
            inventory[GoodType::Boards] = 44;
            inventory[GoodType::Stones] = 68;
            inventory[GoodType::ShieldVikings] = 0;
            inventory[GoodType::ShieldAfricans] = 0;
            inventory[GoodType::Grain] = 0;
            inventory[GoodType::Coins] = 0;
            inventory[GoodType::Gold] = 0;
            inventory[GoodType::IronOre] = 16;
            inventory[GoodType::Coal] = 16;
            inventory[GoodType::Meat] = 6;
            inventory[GoodType::Ham] = 0;
            inventory[GoodType::ShieldJapanese] = 0;

            inventory[Job::Helper] = 52;
            inventory[Job::Woodcutter] = 8;
            inventory[Job::Fisher] = 0;
            inventory[Job::Forester] = 4;
            inventory[Job::Carpenter] = 4;
            inventory[Job::Stonemason] = 4;
            inventory[Job::Hunter] = 2;
            inventory[Job::Farmer] = 0;
            inventory[Job::Miller] = 0;
            inventory[Job::Baker] = 0;
            inventory[Job::Butcher] = 0;
            inventory[Job::Miner] = 10;
            inventory[Job::Brewer] = 0;
            inventory[Job::PigBreeder] = 0;
            inventory[Job::DonkeyBreeder] = 0;
            inventory[Job::IronFounder] = 0;
            inventory[Job::Minter] = 0;
            inventory[Job::Metalworker] = 2;
            inventory[Job::Armorer] = 4;
            inventory[Job::Builder] = 10;
            inventory[Job::Planer] = 6;
            inventory[Job::Private] = 46;
            inventory[Job::PrivateFirstClass] = 0;
            inventory[Job::Sergeant] = 0;
            inventory[Job::Officer] = 0;
            inventory[Job::General] = 0;
            inventory[Job::Geologist] = 6;
            inventory[Job::Shipwright] = 0;
            inventory[Job::Scout] = 2;
            inventory[Job::PackDonkey] = 8;
            break;

        case StartWares::ALot:
            inventory[GoodType::Beer] = 12;
            inventory[GoodType::Tongs] = 0;
            inventory[GoodType::Hammer] = 32;
            inventory[GoodType::Axe] = 12;
            inventory[GoodType::Saw] = 4;
            inventory[GoodType::PickAxe] = 4;
            inventory[GoodType::Shovel] = 8;
            inventory[GoodType::Crucible] = 8;
            inventory[GoodType::RodAndLine] = 12;
            inventory[GoodType::Scythe] = 16;
            inventory[GoodType::WaterEmpty] = 0;
            inventory[GoodType::Water] = 0;
            inventory[GoodType::Cleaver] = 4;
            inventory[GoodType::Rollingpin] = 4;
            inventory[GoodType::Bow] = 4;
            inventory[GoodType::Boat] = 24;
            inventory[GoodType::Sword] = 12;
            inventory[GoodType::Iron] = 0;
            inventory[GoodType::Flour] = 0;
            inventory[GoodType::Fish] = 8;
            inventory[GoodType::Bread] = 16;
            inventory[GoodType::ShieldRomans] = 12;
            inventory[GoodType::Wood] = 48;
            inventory[GoodType::Boards] = 88;
            inventory[GoodType::Stones] = 136;
            inventory[GoodType::ShieldVikings] = 0;
            inventory[GoodType::ShieldAfricans] = 0;
            inventory[GoodType::Grain] = 0;
            inventory[GoodType::Coins] = 0;
            inventory[GoodType::Gold] = 0;
            inventory[GoodType::IronOre] = 32;
            inventory[GoodType::Coal] = 32;
            inventory[GoodType::Meat] = 12;
            inventory[GoodType::Ham] = 0;
            inventory[GoodType::ShieldJapanese] = 0;

            inventory[Job::Helper] = 104;
            inventory[Job::Woodcutter] = 16;
            inventory[Job::Fisher] = 0;
            inventory[Job::Forester] = 8;
            inventory[Job::Carpenter] = 8;
            inventory[Job::Stonemason] = 8;
            inventory[Job::Hunter] = 4;
            inventory[Job::Farmer] = 0;
            inventory[Job::Miller] = 0;
            inventory[Job::Baker] = 0;
            inventory[Job::Butcher] = 0;
            inventory[Job::Miner] = 20;
            inventory[Job::Brewer] = 0;
            inventory[Job::PigBreeder] = 0;
            inventory[Job::DonkeyBreeder] = 0;
            inventory[Job::IronFounder] = 0;
            inventory[Job::Minter] = 0;
            inventory[Job::Metalworker] = 4;
            inventory[Job::Armorer] = 8;
            inventory[Job::Builder] = 20;
            inventory[Job::Planer] = 12;
            inventory[Job::Private] = 92;
            inventory[Job::PrivateFirstClass] = 0;
            inventory[Job::Sergeant] = 0;
            inventory[Job::Officer] = 0;
            inventory[Job::General] = 0;
            inventory[Job::Geologist] = 12;
            inventory[Job::Shipwright] = 0;
            inventory[Job::Scout] = 4;
            inventory[Job::PackDonkey] = 16;
            break;
    }
    return inventory;
}

void nobHQ::addStartWares()
{
    AddToInventory(getStartInventory(world->GetGGS().startWares), true);
}

void nobHQ::DestroyBuilding()
{
    nobBaseWarehouse::DestroyBuilding();
    // Wieder aus dem Militärquadrat rauswerfen
    world->GetMilitarySquares().Remove(this);
    // Recalc territory. AFTER calling base destroy as otherwise figures might get stuck here
    world->RecalcTerritory(*this, TerritoryChangeReason::Destroyed);
}

void nobHQ::Serialize(SerializedGameData& sgd) const
{
    nobBaseWarehouse::Serialize(sgd);
    sgd.PushBool(isTent_);
}

nobHQ::nobHQ(SerializedGameData& sgd, const unsigned obj_id) : nobBaseWarehouse(sgd, obj_id), isTent_(sgd.PopBool())
{
    world->GetMilitarySquares().Add(this);
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
                bitmap->drawForPlayer(flagsPos + DrawPoint(0, (i - 1) * 3), world->GetPlayer(player).color);
        }
    }
}

void nobHQ::HandleEvent(const unsigned id)
{
    HandleBaseEvent(id);
}
