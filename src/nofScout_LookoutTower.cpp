// $Id: nofScout_LookoutTower.cpp 9357 2014-04-25 15:35:25Z FloSoft $
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
#include "nofScout_LookoutTower.h"

#include "GameWorld.h"
#include "MilitaryConsts.h"
#include "nobUsual.h"
#include "GameClient.h"
#include "nobHarborBuilding.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

nofScout_LookoutTower::nofScout_LookoutTower(const unsigned short x, const unsigned short y, const unsigned char player, nobUsual* workplace)
    : nofBuildingWorker(JOB_SCOUT, x, y, player, workplace)
{
}

nofScout_LookoutTower::nofScout_LookoutTower(SerializedGameData* sgd, const unsigned obj_id)
    : nofBuildingWorker(sgd, obj_id)
{
}


void nofScout_LookoutTower::Serialize_nofScout_LookoutTower(SerializedGameData* sgd) const
{
    Serialize_nofBuildingWorker(sgd);
}


void nofScout_LookoutTower::WalkedDerived()
{
}

void nofScout_LookoutTower::DrawWorking(int x, int y)
{
}

void nofScout_LookoutTower::HandleDerivedEvent(const unsigned int id)
{
}

void nofScout_LookoutTower::WorkAborted()
{
    // Im enstprechenden Radius alles neu berechnen
    gwg->RecalcVisibilitiesAroundPoint(x, y, VISUALRANGE_LOOKOUTTOWER, player, workplace);
}

void nofScout_LookoutTower::WorkplaceReached()
{

    // Im enstprechenden Radius alles sichtbar machen
    gwg->SetVisibilitiesAroundPoint(x, y, VISUALRANGE_LOOKOUTTOWER, player);

    // Und Post versenden
    if(GameClient::inst().GetPlayerID() == this->player)
        GameClient::inst().SendPostMessage(new ImagePostMsgWithLocation(
                                               _("Lookout-tower occupied"), PMC_MILITARY, x, y, workplace->GetBuildingType(), workplace->GetNation()));

}
