// $Id: nofCharburner.cpp 9357 2014-04-25 15:35:25Z FloSoft $
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
#include "nofCharburner.h"

#include "GameWorld.h"
#include "noGranite.h"
#include "Loader.h"
#include "macros.h"
#include "GameClient.h"
#include "Ware.h"
#include "Random.h"
#include "SoundManager.h"
#include "GameWorld.h"
#include "GameInterface.h"
#include "noCharburnerPile.h"
#include "nobUsual.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

nofCharburner::nofCharburner(const unsigned short x, const unsigned short y, const unsigned char player, nobUsual* workplace)
    : nofFarmhand(JOB_CHARBURNER, x, y, player, workplace), harvest(false), wt(WT_WOOD)
{
}

nofCharburner::nofCharburner(SerializedGameData* sgd, const unsigned obj_id) : nofFarmhand(sgd, obj_id),
    harvest(sgd->PopBool()),
    wt(WareType(sgd->PopUnsignedChar()))
{
}

/// Malt den Arbeiter beim Arbeiten
void nofCharburner::DrawWorking(int x, int y)
{
    if(harvest)
    {
        unsigned short now_id = GameClient::inst().Interpolate(39, current_ev);

        // Schaufel-Sound
        if(now_id == 6 || now_id == 18 || now_id == 30)
        {
            SoundManager::inst().PlayNOSound(76, this, now_id / 12, 200);
            was_sounding = true;
        }

        unsigned draw_id;
        if(now_id < 36)
            draw_id = 9 + now_id % 12;
        else
            draw_id = 9 + 12 + (now_id - 36);


        LOADER.GetImageN("charburner_bobs", draw_id)->Draw(x, y, 0, 0, 0, 0, 0, 0, COLOR_WHITE, COLORS[gwg->GetPlayer(player)->color]);
    }
    else
        LOADER.GetImageN("charburner_bobs", 1 + GameClient::inst().Interpolate(18, current_ev) % 6)->Draw(x, y, 0, 0, 0, 0, 0, 0, COLOR_WHITE, COLORS[gwg->GetPlayer(player)->color]);

}

/// Fragt die abgeleitete Klasse um die ID in JOBS.BOB, wenn der Beruf Waren rausträgt (bzw rein)
unsigned short nofCharburner::GetCarryID() const
{
    return 1000;
}

/// Abgeleitete Klasse informieren, wenn sie anfängt zu arbeiten (Vorbereitungen)
void nofCharburner::WorkStarted()
{
}

/// Abgeleitete Klasse informieren, wenn fertig ist mit Arbeiten
void nofCharburner::WorkFinished()
{
    noBase* no = gwg->GetNO(x, y);

    // Is a charburner pile is already there?
    if(no->GetGOT() == GOT_CHARBURNERPILE)
    {
        // Is Pile already in the normal "coal harvest mode"?
        if(static_cast<noCharburnerPile*>(no)->GetState() == noCharburnerPile::STATE_HARVEST)
            // Then let's bring a coal to our house
            ware = GD_COAL;
        // One step further
        static_cast<noCharburnerPile*>(no)->NextStep();
        return;

    }

    // Point still good?
    if(GetPointQuality(x, y) != PQ_NOTPOSSIBLE)
    {
        // Delete previous elements
        // Only environt objects and signs are allowed to be removed by the worker!
        // Otherwise just do nothing
        NodalObjectType nop = no->GetType();

        if(nop == NOP_ENVIRONMENT || nop == NOP_NOTHING)
        {
            no = gwg->GetSpecObj<noBase>(x, y);
            if(no)
            {
                no->Destroy();
                delete no;
            }

            // Plant charburner pile
            gwg->SetNO(new noCharburnerPile(x, y), x, y);

            // BQ drumrum neu berechnen
            gwg->RecalcBQAroundPointBig(x, y);
        }
    }
}

/// Fragt abgeleitete Klasse, ob hier Platz bzw ob hier ein Baum etc steht, den z.B. der Holzfäller braucht
nofFarmhand::PointQuality nofCharburner::GetPointQuality(const unsigned short x, const unsigned short y)
{
    noBase* no = gwg->GetNO(x, y);

    // Is a charburner pile already here?
    if(no->GetGOT() == GOT_CHARBURNERPILE)
    {
        noCharburnerPile::State state = static_cast<noCharburnerPile*>(no)->GetState();
        // Can't it be harvested?
        if(state == noCharburnerPile::STATE_SMOLDERING)
            return PQ_NOTPOSSIBLE;

        // Wood stack which stell need resources?
        if(state == noCharburnerPile::STATE_WOOD)
        {
            // Does it need resources and I don't have them hen starting new work (state = STATE_WAITING1)?
            if(!workplace->WaresAvailable() && this->state == STATE_WAITING1)
                return PQ_NOTPOSSIBLE;
            else
                // Only second class, harvest all piles first before continue
                // to build others
                return PQ_CLASS2;
        }

        // All ok, work on this pile
        return PQ_CLASS1;
    }

    // Try to "plant" a new pile
    // Still enough wares when starting new work (state = STATE_WAITING1)?
    if(!workplace->WaresAvailable() && state == STATE_WAITING1)
        return PQ_NOTPOSSIBLE;

    // Der Platz muss frei sein
    noBase::BlockingManner bm = gwg->GetNO(x, y)->GetBM();

    if(bm != noBase::BM_NOTBLOCKING)
        return PQ_NOTPOSSIBLE;

    // Kein Grenzstein darf da stehen
    if(gwg->GetNode(x, y).boundary_stones[0])
        return PQ_NOTPOSSIBLE;





    for(unsigned char i = 0; i < 6; ++i)
    {
        // Don't set it next to buildings and other charburner piles and grain fields
        BlockingManner bm = gwg->GetNO(gwg->GetXA(x, y, i), gwg->GetYA(x, y, i))->GetBM();
        if(bm != BM_NOTBLOCKING)
            return PQ_NOTPOSSIBLE;
        // darf außerdem nicht neben einer Straße liegen
        for(unsigned char j = 0; j < 6; ++j)
        {
            if(gwg->GetPointRoad(gwg->GetXA(x, y, i), gwg->GetYA(x, y, i), j))
                return PQ_NOTPOSSIBLE;
        }
    }

    // Terrain untersuchen (nur auf Wiesen und Savanne und Steppe pflanzen
    unsigned char t, good_terrains = 0;

    for(unsigned char i = 0; i < 6; ++i)
    {
        t = gwg->GetTerrainAround(x, y, i);
        if(t == 1 || t == 3 || (t >= 8 && t <= 13))
            ++good_terrains;
    }

    if(good_terrains != 6)
        return PQ_NOTPOSSIBLE;

    return PQ_CLASS3;
}


void nofCharburner::Serialize(SerializedGameData* sgd) const
{
    Serialize_nofFarmhand(sgd);

    sgd->PushBool(harvest);
    sgd->PushUnsignedChar(static_cast<unsigned char>(wt));
}

/// Inform derived class about the start of the whole working process (at the beginning when walking out of the house)
void nofCharburner::WalkingStarted()
{
    noBase* nob = gwg->GetNO(dest_x, dest_y);
    if(nob->GetGOT() == GOT_CHARBURNERPILE)
        harvest = !(static_cast<noCharburnerPile*>(nob)->GetState() == noCharburnerPile::STATE_WOOD);
    else
        harvest = false;

    // Consume wares if we carry a ware
    if(!harvest)
    {
        workplace->ConsumeWares();
        // Dertermine ware which we should carry to the pile
        if(nob->GetGOT() != GOT_CHARBURNERPILE)
            wt = WT_WOOD;
        else
            wt = WareType(static_cast<noCharburnerPile*>(nob)->GetNeededWareType());
    }

}

/// Draws the figure while returning home / entering the building (often carrying wares)
void nofCharburner::DrawReturnStates(const int x, const int y)
{
    // Carry coal?
    if(ware == GD_COAL)
        DrawWalking(x, y, "charburner_bobs", 200);
    else
        // Draw normal walking otherwise
        DrawWalking(x, y);
}


/// Draws the charburner while walking
/// (overriding standard method of nofFarmhand)
void nofCharburner::DrawOtherStates(const int x, const int y)
{
    switch(state)
    {
        case STATE_WALKTOWORKPOINT:
        {
            // Carry ware?
            if(!harvest)
            {
                if(wt == WT_WOOD)
                    DrawWalking(x, y, "charburner_bobs", 102);
                else
                    DrawWalking(x, y, "charburner_bobs", 151);
            }
            else
                // Draw normal walking
                DrawWalking(x, y);
        } break;
        default: return;
    }
}

