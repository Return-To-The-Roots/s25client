// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "nofCharburner.h"

#include "GamePlayer.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "SoundManager.h"
#include "buildings/nobUsual.h"
#include "network/GameClient.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "world/GameWorldGame.h"
#include "nodeObjs/noCharburnerPile.h"
#include "gameData/TerrainDesc.h"
#include <stdexcept>

nofCharburner::nofCharburner(const MapPoint pos, const unsigned char player, nobUsual* workplace)
    : nofFarmhand(Job::CharBurner, pos, player, workplace), harvest(false), wt(WT_WOOD)
{}

nofCharburner::nofCharburner(SerializedGameData& sgd, const unsigned obj_id)
    : nofFarmhand(sgd, obj_id), harvest(sgd.PopBool()), wt(WareType(sgd.PopUnsignedChar()))
{}

/// Malt den Arbeiter beim Arbeiten
void nofCharburner::DrawWorking(DrawPoint drawPt)
{
    if(harvest)
    {
        unsigned short now_id = GAMECLIENT.Interpolate(39, current_ev);

        // Schaufel-Sound
        if(now_id == 6 || now_id == 18 || now_id == 30)
        {
            SOUNDMANAGER.PlayNOSound(76, this, now_id / 12, 200);
            was_sounding = true;
        }

        unsigned draw_id;
        if(now_id < 36)
            draw_id = 9 + now_id % 12;
        else
            draw_id = 9 + 12 + (now_id - 36);

        LOADER.GetPlayerImage("charburner_bobs", draw_id)->DrawFull(drawPt, COLOR_WHITE, gwg->GetPlayer(player).color);
    } else
        LOADER.GetPlayerImage("charburner_bobs", 1 + GAMECLIENT.Interpolate(18, current_ev) % 6)
          ->DrawFull(drawPt, COLOR_WHITE, gwg->GetPlayer(player).color);
}

unsigned short nofCharburner::GetCarryID() const
{
    throw std::logic_error("Must not be called. Handled by custom DrawWalkingWithWare");
}

/// Abgeleitete Klasse informieren, wenn sie anfängt zu arbeiten (Vorbereitungen)
void nofCharburner::WorkStarted() {}

/// Abgeleitete Klasse informieren, wenn fertig ist mit Arbeiten
void nofCharburner::WorkFinished()
{
    noBase* no = gwg->GetNO(pos);

    // Is a charburner pile is already there?
    if(no->GetGOT() == GOT_CHARBURNERPILE)
    {
        // Is Pile already in the normal "coal harvest mode"?
        if(static_cast<noCharburnerPile*>(no)->GetState() == noCharburnerPile::State::Harvest)
            // Then let's bring a coal to our house
            ware = GoodType::Coal;
        // One step further
        static_cast<noCharburnerPile*>(no)->NextStep();
        return;
    }

    // Point still good?
    if(GetPointQuality(pos) != PQ_NOTPOSSIBLE)
    {
        // Delete previous elements
        // Only environt objects and signs are allowed to be removed by the worker!
        // Otherwise just do nothing
        NodalObjectType noType = no->GetType();

        if(noType == NodalObjectType::Environment || noType == NodalObjectType::Nothing)
        {
            gwg->DestroyNO(pos, false);
            // Plant charburner pile
            gwg->SetNO(pos, new noCharburnerPile(pos));

            // BQ drumrum neu berechnen
            gwg->RecalcBQAroundPointBig(pos);
        }
    }
}

/// Fragt abgeleitete Klasse, ob hier Platz bzw ob hier ein Baum etc steht, den z.B. der Holzfäller braucht
nofFarmhand::PointQuality nofCharburner::GetPointQuality(const MapPoint pt) const
{
    noBase* no = gwg->GetNO(pt);

    // Is a charburner pile already here?
    if(no->GetGOT() == GOT_CHARBURNERPILE)
    {
        noCharburnerPile::State pileState = static_cast<noCharburnerPile*>(no)->GetState();
        // Can't it be harvested?
        if(pileState == noCharburnerPile::State::Smoldering)
            return PQ_NOTPOSSIBLE;

        // Wood stack which stell need resources?
        if(pileState == noCharburnerPile::State::Wood)
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
    BlockingManner bm = gwg->GetNO(pt)->GetBM();

    if(bm != BlockingManner::None)
        return PQ_NOTPOSSIBLE;

    // Kein Grenzstein darf da stehen
    if(gwg->GetNode(pt).boundary_stones[BorderStonePos::OnPoint])
        return PQ_NOTPOSSIBLE;

    for(const auto dir : helpers::EnumRange<Direction>{})
    {
        // Don't set it next to buildings and other charburner piles and grain fields
        BlockingManner bm = gwg->GetNO(gwg->GetNeighbour(pt, dir))->GetBM();
        if(bm != BlockingManner::None)
            return PQ_NOTPOSSIBLE;
        // darf außerdem nicht neben einer Straße liegen
        for(const auto dir2 : helpers::EnumRange<Direction>{})
        {
            if(gwg->GetPointRoad(gwg->GetNeighbour(pt, dir), dir2) != PointRoad::None)
                return PQ_NOTPOSSIBLE;
        }
    }

    // Terrain untersuchen (need walkable land)
    if(gwg->IsOfTerrain(pt,
                        [](const auto& desc) { return desc.Is(ETerrain::Walkable) && desc.kind == TerrainKind::LAND; }))
        return PQ_CLASS3;
    else
        return PQ_NOTPOSSIBLE;
}

void nofCharburner::Serialize(SerializedGameData& sgd) const
{
    Serialize_nofFarmhand(sgd);

    sgd.PushBool(harvest);
    sgd.PushUnsignedChar(static_cast<unsigned char>(wt));
}

/// Inform derived class about the start of the whole working process (at the beginning when walking out of the house)
void nofCharburner::WalkingStarted()
{
    noBase* nob = gwg->GetNO(dest);
    if(nob->GetGOT() == GOT_CHARBURNERPILE)
        harvest = !(static_cast<noCharburnerPile*>(nob)->GetState() == noCharburnerPile::State::Wood);
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
void nofCharburner::DrawWalkingWithWare(DrawPoint drawPt)
{
    DrawWalking(drawPt, "charburner_bobs", 200);
}

/// Draws the charburner while walking
/// (overriding standard method of nofFarmhand)
void nofCharburner::DrawOtherStates(DrawPoint drawPt)
{
    switch(state)
    {
        case STATE_WALKTOWORKPOINT:
        {
            // Carry ware?
            if(!harvest)
            {
                if(wt == WT_WOOD)
                    DrawWalking(drawPt, "charburner_bobs", 102);
                else
                    DrawWalking(drawPt, "charburner_bobs", 151);
            } else
                // Draw normal walking
                DrawWalking(drawPt);
        }
        break;
        default: return;
    }
}

bool nofCharburner::AreWaresAvailable() const
{
    // Charburner doesn't need wares for harvesting!
    // -> Wares are considered when calling GetPointQuality!
    return true;
}
