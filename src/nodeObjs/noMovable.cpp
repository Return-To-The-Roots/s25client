// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

#include "defines.h" // IWYU pragma: keep
#include "noMovable.h"

#include "GameClient.h"
#include "SerializedGameData.h"
#include "EventManager.h"
#include "world/GameWorldGame.h"
#include "GameEvent.h"
#include "gameData/MapConsts.h"
#include "Log.h"

noMovable::noMovable(const NodalObjectType nop, const MapPoint pos)
    : noCoordBase(nop, pos), curMoveDir(4), ascent(0), current_ev(0), pause_walked_gf(0), pause_event_length(0), moving(false)
{
}

void noMovable::Serialize_noMovable(SerializedGameData& sgd) const
{
    Serialize_noCoordBase(sgd);

    sgd.PushUnsignedChar(curMoveDir);
    sgd.PushUnsignedChar(ascent);
    sgd.PushObject(current_ev, true);
    sgd.PushUnsignedInt(pause_walked_gf);
    sgd.PushUnsignedInt(pause_event_length);
    sgd.PushBool(moving);
}

noMovable::noMovable(SerializedGameData& sgd, const unsigned obj_id) : noCoordBase(sgd, obj_id),
    curMoveDir(sgd.PopUnsignedChar()),
    ascent(sgd.PopUnsignedChar()),
    current_ev(sgd.PopEvent()),
    pause_walked_gf(sgd.PopUnsignedInt()),
    pause_event_length(sgd.PopUnsignedInt())
    , moving(sgd.PopBool())
{
}

void noMovable::Walk()
{
    moving = false;
	
	if ((curMoveDir != 1) && (curMoveDir != 2))
	{
		gwg->RemoveFigure(this, pos);
		
		pos = gwg->GetNeighbour(pos, curMoveDir);
		
		gwg->AddFigure(this, pos);
	} else
	{
		pos = gwg->GetNeighbour(pos, curMoveDir);
	}
}

void noMovable::FaceDir(unsigned char newDir)
{
    RTTR_Assert(newDir < 6);
    curMoveDir = newDir;
}

void noMovable::StartMoving(const unsigned char newDir, unsigned gf_length)
{
    RTTR_Assert(!moving);
    RTTR_Assert(newDir < 6);

    // Ist das Wesen stehengeblieben mitten aufm Weg?
    if(pause_walked_gf)
    {
        // Das Laufevent fortführen
        RTTR_Assert(newDir == curMoveDir);
        // Avoid setting an event for current gf by increasing the length
        if(pause_walked_gf == pause_event_length)
            pause_event_length++;
        current_ev = GetEvMgr().AddEvent(this, pause_event_length, 0, pause_walked_gf);
        pause_walked_gf = 0;
        moving = true;
        return;
    }

    // Steigung ermitteln, muss entsprechend langsamer (hoch) bzw. schneller (runter) laufen
    // runter natürlich nich so viel schneller werden wie langsamer hoch
    switch(int(gwg->GetNeighbourNode(pos, newDir).altitude) - int(gwg->GetNode(pos).altitude))
    {
        default: ascent = 3; break; // gerade
        case 1: ascent = 4; gf_length+=(gf_length/2); break; // leicht hoch
        case 2: case 3: ascent = 5; gf_length*=2;  break; // mittelsteil hoch
        case 4: case 5: ascent = 6; gf_length*=3;  break; // steil hoch
        case -1: ascent = 2; break; // leicht runter
        case -2: case -3: ascent = 1;  break; // mittelsteil runter
        case -4: case -5: ascent = 0; break; // steil runter
    }

    current_ev = GetEvMgr().AddEvent(this, gf_length);
    this->curMoveDir = newDir;
    moving = true;

    // Wenn wir nach oben gehen, muss vom oberen Punkt dann aus gezeichnet werden im GameWorld
    if(newDir == 1 || newDir == 2)
    {
        gwg->RemoveFigure(this, pos);
        gwg->AddFigure(this, gwg->GetNeighbour(pos, newDir));
    }
}

Point<int> noMovable::CalcRelative(const Point<int>& curPt, const Point<int>& nextPt) const
{
    if(current_ev)
    {
        RTTR_Assert(current_ev->length > 0);
        if(current_ev->length == 0)
        {
            LOG.lprintf("WARNING: Bug detected (GF: %u). Please report this with the savegame and replay. noMovable::CalcRelative: current_ev->gf_length = 0!\n", GetEvMgr().GetCurrentGF());
            return Point<int>(0, 0);
        }
    }

    RTTR_Assert(current_ev || pause_walked_gf);

    // Wenn wir mittem aufm Weg stehen geblieben sind, die gemerkten Werte jeweils nehmen
    unsigned gf_diff = current_ev ? (GetEvMgr().GetCurrentGF() - current_ev->startGF) : pause_walked_gf;
    unsigned evLength = current_ev ? current_ev->length : pause_event_length;
    unsigned frame_time = current_ev ? GetEvMgr().GetCurrentGF() : 0;

    // Convert to real world time
    const unsigned gfLength = GetEvMgr().GetCurrentGF();
    // Time since the start of the event
    unsigned curTimePassed = gf_diff * gfLength + frame_time;
    // Duration of the event
    unsigned duration = evLength * gfLength;

    // We are in that event
    RTTR_Assert(curTimePassed <= duration);
    // We need to convert to int
    RTTR_Assert(curTimePassed <= static_cast<unsigned>(std::numeric_limits<int>::max()));
    RTTR_Assert(duration <= static_cast<unsigned>(std::numeric_limits<int>::max()));

    if(curMoveDir != 1 && curMoveDir != 2)
    {
        return ((nextPt - curPt) * static_cast<int>(curTimePassed)) / static_cast<int>(duration);
    }
    else
    {
        return ((nextPt - curPt) * static_cast<int>(duration - curTimePassed)) / static_cast<int>(duration);
    }
}

/// Interpoliert fürs Laufen zwischen zwei Kartenpunkten
Point<int> noMovable::CalcWalkingRelative() const
{
    Point<int> curPt  = gwg->GetNodePos(pos);
    Point<int> nextPt = gwg->GetNodePos(gwg->GetNeighbour(pos, curMoveDir));

    // Gehen wir über einen Kartenrand (horizontale Richung?)
    const int mapWidth = gwg->GetWidth() * TR_W;
    if(std::abs(curPt.x - nextPt.x) >= mapWidth / 2)
    {
        // So we need to get closer to nextPt
        if(curPt.x > nextPt.x)
            curPt.x -= mapWidth;
        else
            curPt.x += mapWidth;
    }
    // Und dasselbe für vertikale Richtung
    const int mapHeight = gwg->GetHeight() * TR_H;
    if(std::abs(curPt.y - nextPt.y) >= mapHeight / 2)
    {
        if(curPt.y > nextPt.y)
            curPt.y -= mapHeight;
        else
            curPt.y += mapHeight;
    }

    // Wenn sie runterlaufen, muss es andersrum sein, da die Tiere dann immer vom OBEREN Punkt aus gezeichnet werden
    if(curMoveDir == 1 || curMoveDir == 2)
    {
        using std::swap;
        swap(curPt, nextPt);
    }

    return CalcRelative(curPt, nextPt);
}


void noMovable::PauseWalking()
{
    // Frames festhalten, bis zu denen wir gekommen sind
    pause_walked_gf = GetEvMgr().GetCurrentGF() - current_ev->startGF;
    // Länge merken
    pause_event_length = current_ev->length;
    // Event abmelden
    GetEvMgr().RemoveEvent(current_ev);
    moving = false;

    // Achtung, evtl wird er gleich in dem gf gestoppt, wo er auch losgelaufen war
    // in dem Fall ist es wie, als ob er normal an einem bestimmten Knotenpunkt stoppt, bloß dass er
    // schon losgelaufen war, als der andere ihn stoppt, daher muss Verschieben wegen dem Zeichnen im GameWorld
    // rückgängig gemacht werden!
    if(pause_walked_gf == 0)
    {
        // Wenn wir nach oben gehen, muss vom oberen Punkt dann aus gezeichnet werden im GameWorld
        // --> rückgängig!
        if(curMoveDir == 1 || curMoveDir == 2)
        {
            gwg->RemoveFigure(this, gwg->GetNeighbour(pos, curMoveDir));
            gwg->AddFigure(this, pos);
        }
    }
}

/// Gibt zurück, ob sich das angegebene Objekt zwischen zwei Punkten bewegt
bool noMovable::IsMoving() const
{
    return current_ev && (current_ev->id == 0);
}

/// Gibt die Position zurück, wo wir uns hinbewegen (selbe Position, wenn Schiff steht)
MapPoint noMovable::GetDestinationForCurrentMove() const
{
    // Bewegt sich das Ding gerade?
    if(IsMoving())
        // Dann unsere Zielrichtung zur Berechnung verwenden
        return MapPoint(gwg->GetNeighbour(pos, curMoveDir));

    return pos;
}

