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
    : nobBaseWarehouse(BLD_HEADQUARTERS, pos, player, nation), isTent_(isTent)
{
    // StartWaren setzen
    switch(gwg->GetGGS().startWares)
    {
        // sehr wenig
        case 0:
            inventory.visual.goods[GD_BEER] = 0;
            inventory.visual.goods[GD_TONGS] = 1;
            inventory.visual.goods[GD_HAMMER] = 4;
            inventory.visual.goods[GD_AXE] = 1;
            inventory.visual.goods[GD_SAW] = 0;
            inventory.visual.goods[GD_PICKAXE] = 0;
            inventory.visual.goods[GD_SHOVEL] = 1;
            inventory.visual.goods[GD_CRUCIBLE] = 1;
            inventory.visual.goods[GD_RODANDLINE] = 1; //??
            inventory.visual.goods[GD_SCYTHE] = 2;     //??
            inventory.visual.goods[GD_WATEREMPTY] = 0;
            inventory.visual.goods[GD_WATER] = 0;
            inventory.visual.goods[GD_CLEAVER] = 0;
            inventory.visual.goods[GD_ROLLINGPIN] = 1;
            inventory.visual.goods[GD_BOW] = 0;
            inventory.visual.goods[GD_BOAT] = 0;
            inventory.visual.goods[GD_SWORD] = 0;
            inventory.visual.goods[GD_IRON] = 0;
            inventory.visual.goods[GD_FLOUR] = 0;
            inventory.visual.goods[GD_FISH] = 1;
            inventory.visual.goods[GD_BREAD] = 2;
            inventory.visual.goods[GD_SHIELDROMANS] = 0;
            inventory.visual.goods[GD_WOOD] = 6;
            inventory.visual.goods[GD_BOARDS] = 11;
            inventory.visual.goods[GD_STONES] = 17;
            inventory.visual.goods[GD_SHIELDVIKINGS] = 0;
            inventory.visual.goods[GD_SHIELDAFRICANS] = 0;
            inventory.visual.goods[GD_GRAIN] = 0;
            inventory.visual.goods[GD_COINS] = 0;
            inventory.visual.goods[GD_GOLD] = 0;
            inventory.visual.goods[GD_IRONORE] = 4;
            inventory.visual.goods[GD_COAL] = 4;
            inventory.visual.goods[GD_MEAT] = 0;
            inventory.visual.goods[GD_HAM] = 0;
            inventory.visual.goods[GD_SHIELDJAPANESE] = 0;

            inventory.visual.people[JOB_HELPER] = 13;
            inventory.visual.people[JOB_WOODCUTTER] = 2;
            inventory.visual.people[JOB_FISHER] = 0;
            inventory.visual.people[JOB_FORESTER] = 1;
            inventory.visual.people[JOB_CARPENTER] = 1;
            inventory.visual.people[JOB_STONEMASON] = 1;
            inventory.visual.people[JOB_HUNTER] = 1;
            inventory.visual.people[JOB_FARMER] = 0;
            inventory.visual.people[JOB_MILLER] = 0;
            inventory.visual.people[JOB_BAKER] = 0;
            inventory.visual.people[JOB_BUTCHER] = 0;
            inventory.visual.people[JOB_MINER] = 2;
            inventory.visual.people[JOB_BREWER] = 0;
            inventory.visual.people[JOB_PIGBREEDER] = 0;
            inventory.visual.people[JOB_DONKEYBREEDER] = 0;
            inventory.visual.people[JOB_IRONFOUNDER] = 0;
            inventory.visual.people[JOB_MINTER] = 0;
            inventory.visual.people[JOB_METALWORKER] = 0;
            inventory.visual.people[JOB_ARMORER] = 1;
            inventory.visual.people[JOB_BUILDER] = 2;
            inventory.visual.people[JOB_PLANER] = 1;
            inventory.visual.people[JOB_PRIVATE] = 13;
            inventory.visual.people[JOB_PRIVATEFIRSTCLASS] = 0;
            inventory.visual.people[JOB_SERGEANT] = 0;
            inventory.visual.people[JOB_OFFICER] = 0;
            inventory.visual.people[JOB_GENERAL] = 0;
            inventory.visual.people[JOB_GEOLOGIST] = 2;
            inventory.visual.people[JOB_SHIPWRIGHT] = 0;
            inventory.visual.people[JOB_SCOUT] = 1;
            inventory.visual.people[JOB_PACKDONKEY] = 2;
            break;

        // Wenig
        case 1:

            inventory.visual.goods[GD_BEER] = 0;
            inventory.visual.goods[GD_TONGS] = 0;
            inventory.visual.goods[GD_HAMMER] = 8;
            inventory.visual.goods[GD_AXE] = 3;
            inventory.visual.goods[GD_SAW] = 1;
            inventory.visual.goods[GD_PICKAXE] = 1;
            inventory.visual.goods[GD_SHOVEL] = 2;
            inventory.visual.goods[GD_CRUCIBLE] = 2;
            inventory.visual.goods[GD_RODANDLINE] = 3;
            inventory.visual.goods[GD_SCYTHE] = 4;
            inventory.visual.goods[GD_WATEREMPTY] = 0;
            inventory.visual.goods[GD_WATER] = 0;
            inventory.visual.goods[GD_CLEAVER] = 1;
            inventory.visual.goods[GD_ROLLINGPIN] = 1;
            inventory.visual.goods[GD_BOW] = 1;
            inventory.visual.goods[GD_BOAT] = 6;
            inventory.visual.goods[GD_SWORD] = 0;
            inventory.visual.goods[GD_IRON] = 0;
            inventory.visual.goods[GD_FLOUR] = 0;
            inventory.visual.goods[GD_FISH] = 2;
            inventory.visual.goods[GD_BREAD] = 4;
            inventory.visual.goods[GD_SHIELDROMANS] = 0;
            inventory.visual.goods[GD_WOOD] = 12;
            inventory.visual.goods[GD_BOARDS] = 22;
            inventory.visual.goods[GD_STONES] = 34;
            inventory.visual.goods[GD_SHIELDVIKINGS] = 0;
            inventory.visual.goods[GD_SHIELDAFRICANS] = 0;
            inventory.visual.goods[GD_GRAIN] = 0;
            inventory.visual.goods[GD_COINS] = 0;
            inventory.visual.goods[GD_GOLD] = 0;
            inventory.visual.goods[GD_IRONORE] = 8;
            inventory.visual.goods[GD_COAL] = 8;
            inventory.visual.goods[GD_MEAT] = 3;
            inventory.visual.goods[GD_HAM] = 0;
            inventory.visual.goods[GD_SHIELDJAPANESE] = 0;

            inventory.visual.people[JOB_HELPER] = 26;
            inventory.visual.people[JOB_WOODCUTTER] = 4;
            inventory.visual.people[JOB_FISHER] = 0;
            inventory.visual.people[JOB_FORESTER] = 2;
            inventory.visual.people[JOB_CARPENTER] = 2;
            inventory.visual.people[JOB_STONEMASON] = 2;
            inventory.visual.people[JOB_HUNTER] = 1;
            inventory.visual.people[JOB_FARMER] = 0;
            inventory.visual.people[JOB_MILLER] = 0;
            inventory.visual.people[JOB_BAKER] = 0;
            inventory.visual.people[JOB_BUTCHER] = 0;
            inventory.visual.people[JOB_MINER] = 5;
            inventory.visual.people[JOB_BREWER] = 0;
            inventory.visual.people[JOB_PIGBREEDER] = 0;
            inventory.visual.people[JOB_DONKEYBREEDER] = 0;
            inventory.visual.people[JOB_IRONFOUNDER] = 0;
            inventory.visual.people[JOB_MINTER] = 0;
            inventory.visual.people[JOB_METALWORKER] = 1;
            inventory.visual.people[JOB_ARMORER] = 2;
            inventory.visual.people[JOB_BUILDER] = 5;
            inventory.visual.people[JOB_PLANER] = 3;
            inventory.visual.people[JOB_PRIVATE] = 26;
            inventory.visual.people[JOB_PRIVATEFIRSTCLASS] = 0;
            inventory.visual.people[JOB_SERGEANT] = 0;
            inventory.visual.people[JOB_OFFICER] = 0;
            inventory.visual.people[JOB_GENERAL] = 0;
            inventory.visual.people[JOB_GEOLOGIST] = 3;
            inventory.visual.people[JOB_SHIPWRIGHT] = 0;
            inventory.visual.people[JOB_SCOUT] = 1;
            inventory.visual.people[JOB_PACKDONKEY] = 4;
            break;

        // Mittel
        case 2:

            inventory.visual.goods[GD_BEER] = 6;
            inventory.visual.goods[GD_TONGS] = 0;
            inventory.visual.goods[GD_HAMMER] = 16;
            inventory.visual.goods[GD_AXE] = 6;
            inventory.visual.goods[GD_SAW] = 2;
            inventory.visual.goods[GD_PICKAXE] = 2;
            inventory.visual.goods[GD_SHOVEL] = 4;
            inventory.visual.goods[GD_CRUCIBLE] = 4;
            inventory.visual.goods[GD_RODANDLINE] = 6;
            inventory.visual.goods[GD_SCYTHE] = 8;
            inventory.visual.goods[GD_WATEREMPTY] = 0;
            inventory.visual.goods[GD_WATER] = 0;
            inventory.visual.goods[GD_CLEAVER] = 2;
            inventory.visual.goods[GD_ROLLINGPIN] = 2;
            inventory.visual.goods[GD_BOW] = 2;
            inventory.visual.goods[GD_BOAT] = 12;
            inventory.visual.goods[GD_SWORD] = 6;
            inventory.visual.goods[GD_IRON] = 0;
            inventory.visual.goods[GD_FLOUR] = 0;
            inventory.visual.goods[GD_FISH] = 4;
            inventory.visual.goods[GD_BREAD] = 8;
            inventory.visual.goods[GD_SHIELDROMANS] = 6;
            inventory.visual.goods[GD_WOOD] = 24;
            inventory.visual.goods[GD_BOARDS] = 44;
            inventory.visual.goods[GD_STONES] = 68;
            inventory.visual.goods[GD_SHIELDVIKINGS] = 0;
            inventory.visual.goods[GD_SHIELDAFRICANS] = 0;
            inventory.visual.goods[GD_GRAIN] = 0;
            inventory.visual.goods[GD_COINS] = 0;
            inventory.visual.goods[GD_GOLD] = 0;
            inventory.visual.goods[GD_IRONORE] = 16;
            inventory.visual.goods[GD_COAL] = 16;
            inventory.visual.goods[GD_MEAT] = 6;
            inventory.visual.goods[GD_HAM] = 0;
            inventory.visual.goods[GD_SHIELDJAPANESE] = 0;

            inventory.visual.people[JOB_HELPER] = 52;
            inventory.visual.people[JOB_WOODCUTTER] = 8;
            inventory.visual.people[JOB_FISHER] = 0;
            inventory.visual.people[JOB_FORESTER] = 4;
            inventory.visual.people[JOB_CARPENTER] = 4;
            inventory.visual.people[JOB_STONEMASON] = 4;
            inventory.visual.people[JOB_HUNTER] = 2;
            inventory.visual.people[JOB_FARMER] = 0;
            inventory.visual.people[JOB_MILLER] = 0;
            inventory.visual.people[JOB_BAKER] = 0;
            inventory.visual.people[JOB_BUTCHER] = 0;
            inventory.visual.people[JOB_MINER] = 10;
            inventory.visual.people[JOB_BREWER] = 0;
            inventory.visual.people[JOB_PIGBREEDER] = 0;
            inventory.visual.people[JOB_DONKEYBREEDER] = 0;
            inventory.visual.people[JOB_IRONFOUNDER] = 0;
            inventory.visual.people[JOB_MINTER] = 0;
            inventory.visual.people[JOB_METALWORKER] = 2;
            inventory.visual.people[JOB_ARMORER] = 4;
            inventory.visual.people[JOB_BUILDER] = 10;
            inventory.visual.people[JOB_PLANER] = 6;
            inventory.visual.people[JOB_PRIVATE] = 46;
            inventory.visual.people[JOB_PRIVATEFIRSTCLASS] = 0;
            inventory.visual.people[JOB_SERGEANT] = 0;
            inventory.visual.people[JOB_OFFICER] = 0;
            inventory.visual.people[JOB_GENERAL] = 0;
            inventory.visual.people[JOB_GEOLOGIST] = 6;
            inventory.visual.people[JOB_SHIPWRIGHT] = 0;
            inventory.visual.people[JOB_SCOUT] = 2;
            inventory.visual.people[JOB_PACKDONKEY] = 8;
            break;

        // Viel
        case 3:
            inventory.visual.goods[GD_BEER] = 12;
            inventory.visual.goods[GD_TONGS] = 0;
            inventory.visual.goods[GD_HAMMER] = 32;
            inventory.visual.goods[GD_AXE] = 12;
            inventory.visual.goods[GD_SAW] = 4;
            inventory.visual.goods[GD_PICKAXE] = 4;
            inventory.visual.goods[GD_SHOVEL] = 8;
            inventory.visual.goods[GD_CRUCIBLE] = 8;
            inventory.visual.goods[GD_RODANDLINE] = 12;
            inventory.visual.goods[GD_SCYTHE] = 16;
            inventory.visual.goods[GD_WATEREMPTY] = 0;
            inventory.visual.goods[GD_WATER] = 0;
            inventory.visual.goods[GD_CLEAVER] = 4;
            inventory.visual.goods[GD_ROLLINGPIN] = 4;
            inventory.visual.goods[GD_BOW] = 4;
            inventory.visual.goods[GD_BOAT] = 24;
            inventory.visual.goods[GD_SWORD] = 12;
            inventory.visual.goods[GD_IRON] = 0;
            inventory.visual.goods[GD_FLOUR] = 0;
            inventory.visual.goods[GD_FISH] = 8;
            inventory.visual.goods[GD_BREAD] = 16;
            inventory.visual.goods[GD_SHIELDROMANS] = 12;
            inventory.visual.goods[GD_WOOD] = 48;
            inventory.visual.goods[GD_BOARDS] = 88;
            inventory.visual.goods[GD_STONES] = 136;
            inventory.visual.goods[GD_SHIELDVIKINGS] = 0;
            inventory.visual.goods[GD_SHIELDAFRICANS] = 0;
            inventory.visual.goods[GD_GRAIN] = 0;
            inventory.visual.goods[GD_COINS] = 0;
            inventory.visual.goods[GD_GOLD] = 0;
            inventory.visual.goods[GD_IRONORE] = 32;
            inventory.visual.goods[GD_COAL] = 32;
            inventory.visual.goods[GD_MEAT] = 12;
            inventory.visual.goods[GD_HAM] = 0;
            inventory.visual.goods[GD_SHIELDJAPANESE] = 0;

            inventory.visual.people[JOB_HELPER] = 104;
            inventory.visual.people[JOB_WOODCUTTER] = 16;
            inventory.visual.people[JOB_FISHER] = 0;
            inventory.visual.people[JOB_FORESTER] = 8;
            inventory.visual.people[JOB_CARPENTER] = 8;
            inventory.visual.people[JOB_STONEMASON] = 8;
            inventory.visual.people[JOB_HUNTER] = 4;
            inventory.visual.people[JOB_FARMER] = 0;
            inventory.visual.people[JOB_MILLER] = 0;
            inventory.visual.people[JOB_BAKER] = 0;
            inventory.visual.people[JOB_BUTCHER] = 0;
            inventory.visual.people[JOB_MINER] = 20;
            inventory.visual.people[JOB_BREWER] = 0;
            inventory.visual.people[JOB_PIGBREEDER] = 0;
            inventory.visual.people[JOB_DONKEYBREEDER] = 0;
            inventory.visual.people[JOB_IRONFOUNDER] = 0;
            inventory.visual.people[JOB_MINTER] = 0;
            inventory.visual.people[JOB_METALWORKER] = 4;
            inventory.visual.people[JOB_ARMORER] = 8;
            inventory.visual.people[JOB_BUILDER] = 20;
            inventory.visual.people[JOB_PLANER] = 12;
            inventory.visual.people[JOB_PRIVATE] = 92;
            inventory.visual.people[JOB_PRIVATEFIRSTCLASS] = 0;
            inventory.visual.people[JOB_SERGEANT] = 0;
            inventory.visual.people[JOB_OFFICER] = 0;
            inventory.visual.people[JOB_GENERAL] = 0;
            inventory.visual.people[JOB_GEOLOGIST] = 12;
            inventory.visual.people[JOB_SHIPWRIGHT] = 0;
            inventory.visual.people[JOB_SCOUT] = 4;
            inventory.visual.people[JOB_PACKDONKEY] = 16;
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
        LOADER.building_cache[nation][BLD_HEADQUARTERS][1].draw(drawPt);
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
