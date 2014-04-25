// $Id: nobHQ.cpp 9357 2014-04-25 15:35:25Z FloSoft $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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


#include "main.h"
#include "nobHQ.h"
#include "GameWorld.h"
#include "Loader.h"
#include "noExtension.h"
#include "MilitaryConsts.h"
#include "GameClient.h"
#include "GameClientPlayer.h"
#include "GlobalGameSettings.h"
#include "Ware.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

nobHQ::nobHQ(const unsigned short x, const unsigned short y, const unsigned char player, const Nation nation)
    : nobBaseWarehouse(BLD_HEADQUARTERS, x, y, player, nation)
{

    // ins Militärquadrat einfügen
    gwg->GetMilitarySquare(x, y).push_back(this);
    gwg->RecalcTerritory(this, GetMilitaryRadius(), false, true);


    // StartWaren setzen ( provisorisch )
    switch(GameClient::inst().GetGGS().start_wares)
    {
            //sehr wenig

        case 0:
            goods.goods[GD_BEER] = 0;
            goods.goods[GD_TONGS] = 1;
            goods.goods[GD_HAMMER] = 4;
            goods.goods[GD_AXE] = 1;
            goods.goods[GD_SAW] = 0;
            goods.goods[GD_PICKAXE] = 0;
            goods.goods[GD_SHOVEL] = 1;
            goods.goods[GD_CRUCIBLE] = 1;
            goods.goods[GD_RODANDLINE] = 1;//??
            goods.goods[GD_SCYTHE] = 2;//??
            goods.goods[GD_WATEREMPTY] = 0;
            goods.goods[GD_WATER] = 0;
            goods.goods[GD_CLEAVER] = 0;
            goods.goods[GD_ROLLINGPIN] = 1;
            goods.goods[GD_BOW] = 0;
            goods.goods[GD_BOAT] = 0;
            goods.goods[GD_SWORD] = 0;
            goods.goods[GD_IRON] = 0;
            goods.goods[GD_FLOUR] = 0;
            goods.goods[GD_FISH] = 1;
            goods.goods[GD_BREAD] = 2;
            goods.goods[GD_SHIELDROMANS] = 0;
            goods.goods[GD_WOOD] = 6;
            goods.goods[GD_BOARDS] = 11;
            goods.goods[GD_STONES] = 17;
            goods.goods[GD_SHIELDVIKINGS] = 0;
            goods.goods[GD_SHIELDAFRICANS] = 0;
            goods.goods[GD_GRAIN] = 0;
            goods.goods[GD_COINS] = 0;
            goods.goods[GD_GOLD] = 0;
            goods.goods[GD_IRONORE] = 4;
            goods.goods[GD_COAL] = 4;
            goods.goods[GD_MEAT] = 0;
            goods.goods[GD_HAM] = 0;
            goods.goods[GD_SHIELDJAPANESE] = 0;

            goods.people[JOB_HELPER] = 13;
            goods.people[JOB_WOODCUTTER] = 2;
            goods.people[JOB_FISHER] = 0;
            goods.people[JOB_FORESTER] = 1;
            goods.people[JOB_CARPENTER] = 1;
            goods.people[JOB_STONEMASON] = 1;
            goods.people[JOB_HUNTER] = 1;
            goods.people[JOB_FARMER] = 0;
            goods.people[JOB_MILLER] = 0;
            goods.people[JOB_BAKER] = 0;
            goods.people[JOB_BUTCHER] = 0;
            goods.people[JOB_MINER] = 2;
            goods.people[JOB_BREWER] = 0;
            goods.people[JOB_PIGBREEDER] = 0;
            goods.people[JOB_DONKEYBREEDER] = 0;
            goods.people[JOB_IRONFOUNDER] = 0;
            goods.people[JOB_MINTER] = 0;
            goods.people[JOB_METALWORKER] = 0;
            goods.people[JOB_ARMORER] = 1;
            goods.people[JOB_BUILDER] = 2;
            goods.people[JOB_PLANER] = 1;
            goods.people[JOB_PRIVATE] = 13;
            goods.people[JOB_PRIVATEFIRSTCLASS] = 0;
            goods.people[JOB_SERGEANT] = 0;
            goods.people[JOB_OFFICER] = 0;
            goods.people[JOB_GENERAL] = 0;
            goods.people[JOB_GEOLOGIST] = 2;
            goods.people[JOB_SHIPWRIGHT] = 0;
            goods.people[JOB_SCOUT] = 1;
            goods.people[JOB_PACKDONKEY] = 2;
            break;

            // Wenig
        case 1:

            goods.goods[GD_BEER] = 0;
            goods.goods[GD_TONGS] = 0;
            goods.goods[GD_HAMMER] = 8;
            goods.goods[GD_AXE] = 3;
            goods.goods[GD_SAW] = 1;
            goods.goods[GD_PICKAXE] = 1;
            goods.goods[GD_SHOVEL] = 2;
            goods.goods[GD_CRUCIBLE] = 2;
            goods.goods[GD_RODANDLINE] = 3;
            goods.goods[GD_SCYTHE] = 4;
            goods.goods[GD_WATEREMPTY] = 0;
            goods.goods[GD_WATER] = 0;
            goods.goods[GD_CLEAVER] = 1;
            goods.goods[GD_ROLLINGPIN] = 1;
            goods.goods[GD_BOW] = 1;
            goods.goods[GD_BOAT] = 6;
            goods.goods[GD_SWORD] = 0;
            goods.goods[GD_IRON] = 0;
            goods.goods[GD_FLOUR] = 0;
            goods.goods[GD_FISH] = 2;
            goods.goods[GD_BREAD] = 4;
            goods.goods[GD_SHIELDROMANS] = 0;
            goods.goods[GD_WOOD] = 12;
            goods.goods[GD_BOARDS] = 22;
            goods.goods[GD_STONES] = 34;
            goods.goods[GD_SHIELDVIKINGS] = 0;
            goods.goods[GD_SHIELDAFRICANS] = 0;
            goods.goods[GD_GRAIN] = 0;
            goods.goods[GD_COINS] = 0;
            goods.goods[GD_GOLD] = 0;
            goods.goods[GD_IRONORE] = 8;
            goods.goods[GD_COAL] = 8;
            goods.goods[GD_MEAT] = 3;
            goods.goods[GD_HAM] = 0;
            goods.goods[GD_SHIELDJAPANESE] = 0;

            goods.people[JOB_HELPER] = 26;
            goods.people[JOB_WOODCUTTER] = 4;
            goods.people[JOB_FISHER] = 0;
            goods.people[JOB_FORESTER] = 2;
            goods.people[JOB_CARPENTER] = 2;
            goods.people[JOB_STONEMASON] = 2;
            goods.people[JOB_HUNTER] = 1;
            goods.people[JOB_FARMER] = 0;
            goods.people[JOB_MILLER] = 0;
            goods.people[JOB_BAKER] = 0;
            goods.people[JOB_BUTCHER] = 0;
            goods.people[JOB_MINER] = 5;
            goods.people[JOB_BREWER] = 0;
            goods.people[JOB_PIGBREEDER] = 0;
            goods.people[JOB_DONKEYBREEDER] = 0;
            goods.people[JOB_IRONFOUNDER] = 0;
            goods.people[JOB_MINTER] = 0;
            goods.people[JOB_METALWORKER] = 1;
            goods.people[JOB_ARMORER] = 2;
            goods.people[JOB_BUILDER] = 5;
            goods.people[JOB_PLANER] = 3;
            goods.people[JOB_PRIVATE] = 26;
            goods.people[JOB_PRIVATEFIRSTCLASS] = 0;
            goods.people[JOB_SERGEANT] = 0;
            goods.people[JOB_OFFICER] = 0;
            goods.people[JOB_GENERAL] = 0;
            goods.people[JOB_GEOLOGIST] = 3;
            goods.people[JOB_SHIPWRIGHT] = 0;
            goods.people[JOB_SCOUT] = 1;
            goods.people[JOB_PACKDONKEY] = 4;
            break;

            // Mittel
        case 2:

            goods.goods[GD_BEER] = 6;
            goods.goods[GD_TONGS] = 0;
            goods.goods[GD_HAMMER] = 16;
            goods.goods[GD_AXE] = 6;
            goods.goods[GD_SAW] = 2;
            goods.goods[GD_PICKAXE] = 2;
            goods.goods[GD_SHOVEL] = 4;
            goods.goods[GD_CRUCIBLE] = 4;
            goods.goods[GD_RODANDLINE] = 6;
            goods.goods[GD_SCYTHE] = 8;
            goods.goods[GD_WATEREMPTY] = 0;
            goods.goods[GD_WATER] = 0;
            goods.goods[GD_CLEAVER] = 2;
            goods.goods[GD_ROLLINGPIN] = 2;
            goods.goods[GD_BOW] = 2;
            goods.goods[GD_BOAT] = 12;
            goods.goods[GD_SWORD] = 6;
            goods.goods[GD_IRON] = 0;
            goods.goods[GD_FLOUR] = 0;
            goods.goods[GD_FISH] = 4;
            goods.goods[GD_BREAD] = 8;
            goods.goods[GD_SHIELDROMANS] = 6;
            goods.goods[GD_WOOD] = 24;
            goods.goods[GD_BOARDS] = 44;
            goods.goods[GD_STONES] = 68;
            goods.goods[GD_SHIELDVIKINGS] = 0;
            goods.goods[GD_SHIELDAFRICANS] = 0;
            goods.goods[GD_GRAIN] = 0;
            goods.goods[GD_COINS] = 0;
            goods.goods[GD_GOLD] = 0;
            goods.goods[GD_IRONORE] = 16;
            goods.goods[GD_COAL] = 16;
            goods.goods[GD_MEAT] = 6;
            goods.goods[GD_HAM] = 0;
            goods.goods[GD_SHIELDJAPANESE] = 0;

            goods.people[JOB_HELPER] = 52;
            goods.people[JOB_WOODCUTTER] = 8;
            goods.people[JOB_FISHER] = 0;
            goods.people[JOB_FORESTER] = 4;
            goods.people[JOB_CARPENTER] = 4;
            goods.people[JOB_STONEMASON] = 4;
            goods.people[JOB_HUNTER] = 2;
            goods.people[JOB_FARMER] = 0;
            goods.people[JOB_MILLER] = 0;
            goods.people[JOB_BAKER] = 0;
            goods.people[JOB_BUTCHER] = 0;
            goods.people[JOB_MINER] = 10;
            goods.people[JOB_BREWER] = 0;
            goods.people[JOB_PIGBREEDER] = 0;
            goods.people[JOB_DONKEYBREEDER] = 0;
            goods.people[JOB_IRONFOUNDER] = 0;
            goods.people[JOB_MINTER] = 0;
            goods.people[JOB_METALWORKER] = 2;
            goods.people[JOB_ARMORER] = 4;
            goods.people[JOB_BUILDER] = 10;
            goods.people[JOB_PLANER] = 6;
            goods.people[JOB_PRIVATE] = 46;
            goods.people[JOB_PRIVATEFIRSTCLASS] = 0;
            goods.people[JOB_SERGEANT] = 0;
            goods.people[JOB_OFFICER] = 0;
            goods.people[JOB_GENERAL] = 0;
            goods.people[JOB_GEOLOGIST] = 6;
            goods.people[JOB_SHIPWRIGHT] = 0;
            goods.people[JOB_SCOUT] = 2;
            goods.people[JOB_PACKDONKEY] = 8;
            break;

            // Viel
        case 3:
            goods.goods[GD_BEER] = 12;
            goods.goods[GD_TONGS] = 0;
            goods.goods[GD_HAMMER] = 32;
            goods.goods[GD_AXE] = 12;
            goods.goods[GD_SAW] = 4;
            goods.goods[GD_PICKAXE] = 4;
            goods.goods[GD_SHOVEL] = 8;
            goods.goods[GD_CRUCIBLE] = 8;
            goods.goods[GD_RODANDLINE] = 12;
            goods.goods[GD_SCYTHE] = 16;
            goods.goods[GD_WATEREMPTY] = 0;
            goods.goods[GD_WATER] = 0;
            goods.goods[GD_CLEAVER] = 4;
            goods.goods[GD_ROLLINGPIN] = 4;
            goods.goods[GD_BOW] = 4;
            goods.goods[GD_BOAT] = 24;
            goods.goods[GD_SWORD] = 12;
            goods.goods[GD_IRON] = 0;
            goods.goods[GD_FLOUR] = 0;
            goods.goods[GD_FISH] = 8;
            goods.goods[GD_BREAD] = 16;
            goods.goods[GD_SHIELDROMANS] = 12;
            goods.goods[GD_WOOD] = 48;
            goods.goods[GD_BOARDS] = 88;
            goods.goods[GD_STONES] = 136;
            goods.goods[GD_SHIELDVIKINGS] = 0;
            goods.goods[GD_SHIELDAFRICANS] = 0;
            goods.goods[GD_GRAIN] = 0;
            goods.goods[GD_COINS] = 0;
            goods.goods[GD_GOLD] = 0;
            goods.goods[GD_IRONORE] = 32;
            goods.goods[GD_COAL] = 32;
            goods.goods[GD_MEAT] = 12;
            goods.goods[GD_HAM] = 0;
            goods.goods[GD_SHIELDJAPANESE] = 0;

            goods.people[JOB_HELPER] = 104;
            goods.people[JOB_WOODCUTTER] = 16;
            goods.people[JOB_FISHER] = 0;
            goods.people[JOB_FORESTER] = 8;
            goods.people[JOB_CARPENTER] = 8;
            goods.people[JOB_STONEMASON] = 8;
            goods.people[JOB_HUNTER] = 4;
            goods.people[JOB_FARMER] = 0;
            goods.people[JOB_MILLER] = 0;
            goods.people[JOB_BAKER] = 0;
            goods.people[JOB_BUTCHER] = 0;
            goods.people[JOB_MINER] = 20;
            goods.people[JOB_BREWER] = 0;
            goods.people[JOB_PIGBREEDER] = 0;
            goods.people[JOB_DONKEYBREEDER] = 0;
            goods.people[JOB_IRONFOUNDER] = 0;
            goods.people[JOB_MINTER] = 0;
            goods.people[JOB_METALWORKER] = 4;
            goods.people[JOB_ARMORER] = 8;
            goods.people[JOB_BUILDER] = 20;
            goods.people[JOB_PLANER] = 12;
            goods.people[JOB_PRIVATE] = 92;
            goods.people[JOB_PRIVATEFIRSTCLASS] = 0;
            goods.people[JOB_SERGEANT] = 0;
            goods.people[JOB_OFFICER] = 0;
            goods.people[JOB_GENERAL] = 0;
            goods.people[JOB_GEOLOGIST] = 12;
            goods.people[JOB_SHIPWRIGHT] = 0;
            goods.people[JOB_SCOUT] = 4;
            goods.people[JOB_PACKDONKEY] = 16;
            break;
    }

    real_goods = goods;

    // Aktuellen Warenbestand zur aktuellen Inventur dazu addieren
    AddToInventory();

    // Evtl. liegen am Anfang Waffen im HQ, sodass rekrutiert werden muss
    TryRecruiting();
}

void nobHQ::Destroy_nobHQ()
{
    Destroy_nobBaseWarehouse();

    // Land drumherum neu berechnen (nur wenn es schon besetzt wurde!)
    // Nach dem BaseDestroy erst, da in diesem erst das Feuer gesetzt, die Straße gelöscht wird usw.
    gwg->RecalcTerritory(this, MILITARY_RADIUS[GetSize()], true, false);

    // Wieder aus dem Militärquadrat rauswerfen
    gwg->GetMilitarySquare(x, y).erase(this);
}

void nobHQ::Serialize_nobHQ(SerializedGameData* sgd) const
{
    Serialize_nobBaseWarehouse(sgd);
}

nobHQ::nobHQ(SerializedGameData* sgd, const unsigned obj_id) : nobBaseWarehouse(sgd, obj_id)
{
    // ins Militärquadrat einfügen
    gwg->GetMilitarySquare(x, y).push_back(this);

    // Startpos setzen
    GameClient::inst().GetPlayer(player)->hqx = this->x;
    GameClient::inst().GetPlayer(player)->hqy = this->y;
}

void nobHQ::Draw(int x, int y)
{
    // Gebäude an sich zeichnen
    DrawBaseBuilding(x, y);


    // 4 Fähnchen zeichnen
    for(unsigned i = min<unsigned>(GetSoldiersCount() +
                                   reserve_soldiers_available[0] + reserve_soldiers_available[1] + reserve_soldiers_available[2] + reserve_soldiers_available[3] + reserve_soldiers_available[4]
                                   , 4); i; --i)
    {
        glArchivItem_Bitmap* bitmap = LOADER.GetMapImageN(3162 + GAMECLIENT.GetGlobalAnimation(8, 80, 40, GetX() * GetY() * i));
        if(bitmap)
            bitmap->Draw(x + TROOPS_FLAGS_HQ[nation][0], y + TROOPS_FLAGS_HQ[nation][1] + (i - 1) * 3, 0, 0, 0, 0, 0, 0, COLOR_WHITE, COLORS[GAMECLIENT.GetPlayer(player)->color]);
    }
}


void nobHQ::HandleEvent(const unsigned int id)
{
    /*switch(id)
    {
    default:*/
    HandleBaseEvent(id);
    /*break;
    }*/

}
