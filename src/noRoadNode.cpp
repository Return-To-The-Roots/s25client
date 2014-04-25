// $Id: noRoadNode.cpp 9357 2014-04-25 15:35:25Z FloSoft $
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
#include "noRoadNode.h"

#include "RoadSegment.h"
#include "GameWorld.h"
#include "GameClient.h"
#include "GameClientPlayer.h"
#include "SerializedGameData.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


noRoadNode::noRoadNode(const NodalObjectType nop, const unsigned short x, const unsigned short y, const unsigned char player)
    : noCoordBase(nop, x, y),
      player(player)
{
    for(unsigned i = 0; i < 6; ++i)
        routes[i] = 0;
    coord_id = gwg->MakeCoordID(x, y);
    last_visit = 0;
}

noRoadNode::~noRoadNode()
{
}

void noRoadNode::Destroy_noRoadNode()
{
    DestroyAllRoads();

    Destroy_noCoordBase();
}

void noRoadNode::Serialize_noRoadNode(SerializedGameData* sgd) const
{
    Serialize_noCoordBase(sgd);

    sgd->PushUnsignedChar(player);

    // the trick only seems to work for flags
    if (this->GetGOT() == GOT_FLAG)
    {
        // this is a trick:
        // -> initialize routes for flag with NULL
        // -> RoadSegment will set these later
        for (unsigned i = 0; i < 6; ++i)
        {
            sgd->PushObject(NULL, true);
        }
    }
    else
    {
        for (unsigned i = 0; i < 6; ++i)
        {
            sgd->PushObject(routes[i], true);
        }
    }
}

noRoadNode::noRoadNode(SerializedGameData* sgd, const unsigned obj_id) : noCoordBase(sgd, obj_id),
    player(sgd->PopUnsignedChar())
{
    for (unsigned i = 0; i < 6; ++i)
    {
        routes[i] = sgd->PopObject<RoadSegment>(GOT_ROADSEGMENT);
    }

    coord_id = gwg->MakeCoordID(x, y);
    last_visit = 0;
}

void noRoadNode::UpgradeRoad(const unsigned char dir)
{
    if(routes[dir])
        routes[dir]->UpgradeDonkeyRoad();
}

void noRoadNode::DestroyRoad(const unsigned char dir)
{
    if(routes[dir])
    {
        int tx = routes[dir]->GetF1()->GetX(), ty = routes[dir]->GetF1()->GetY();
        for(unsigned z = 0; z < routes[dir]->GetLength(); ++z)
        {
            gwg->SetPointRoad(tx, ty, routes[dir]->GetRoute(z), 0);
            int ttx = tx, tty = ty;
            tx = gwg->GetXA(ttx, tty, routes[dir]->GetRoute(z));
            ty = gwg->GetYA(ttx, tty, routes[dir]->GetRoute(z));
            gwg->CalcRoad(ttx, tty, player);
        }

        noRoadNode* oflag;

        if(routes[dir]->GetF1() == this)
            oflag = routes[dir]->GetF2();
        else
            oflag = routes[dir]->GetF1();

#ifndef NDEBUG
        bool found = false;
#endif
        for(unsigned z = 0; z < 6; ++z)
        {
            if(oflag->routes[z] == routes[dir])
            {
                oflag->routes[z] = NULL;
#ifndef NDEBUG
                found = true;
#endif
                break;
            }
        }

#ifndef NDEBUG
        assert(found);
#endif

        RoadSegment* tmp = routes[dir];
        routes[dir] = 0;

        tmp->Destroy();
        delete tmp;

        // Spieler Bescheid sagen
        gwg->GetPlayer(player)->RoadDestroyed();
    }
}

/// Vernichtet Alle Straße um diesen Knoten
void noRoadNode::DestroyAllRoads()
{
    // Alle Straßen um mich herum zerstören
    for(unsigned char i = 0; i < 6; ++i)
        DestroyRoad(i);
}
