// $Id: nobStorehouse.cpp 9357 2014-04-25 15:35:25Z FloSoft $
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
#include "nobStorehouse.h"
#include "GameWorld.h"
#include "Loader.h"
#include "noExtension.h"
#include "MilitaryConsts.h"
#include "GameClient.h"
#include "GameClientPlayer.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

nobStorehouse::nobStorehouse(const unsigned short x, const unsigned short y, const unsigned char player, const Nation nation)
    : nobBaseWarehouse(BLD_STOREHOUSE, x, y, player, nation)
{
    // Alle Waren 0, außer 100 Träger
    goods.clear();
    real_goods.clear();

    // Aktuellen Warenbestand zur aktuellen Inventur dazu addieren
    AddToInventory();

    // Der Wirtschaftsverwaltung Bescheid sagen
    gwg->GetPlayer(player)->AddWarehouse(this);

    // Post versenden
    if(GameClient::inst().GetPlayerID() == this->player)
        GameClient::inst().SendPostMessage(new ImagePostMsgWithLocation(
                                               _("New storehouse finished"), PMC_GENERAL, x, y, BLD_STOREHOUSE, nation));
}



void nobStorehouse::Destroy_nobStorehouse()
{
    // Der Wirtschaftsverwaltung Bescheid sagen
    gwg->GetPlayer(player)->RemoveWarehouse(this);

    Destroy_nobBaseWarehouse();
}

void nobStorehouse::Serialize_nobStorehouse(SerializedGameData* sgd) const
{
    Serialize_nobBaseWarehouse(sgd);
}

nobStorehouse::nobStorehouse(SerializedGameData* sgd, const unsigned obj_id) : nobBaseWarehouse(sgd, obj_id)
{
}


void nobStorehouse::Draw(int x, int y)
{
    // Gebäude an sich zeichnen
    DrawBaseBuilding(x, y);
}


void nobStorehouse::HandleEvent(const unsigned int id)
{
    HandleBaseEvent(id);
}
