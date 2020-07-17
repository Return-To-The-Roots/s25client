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

#include "noMovable.h"

#include "EventManager.h"
#include "GameEvent.h"
#include "SerializedGameData.h"
#include "network/GameClient.h"
#include "world/GameWorldGame.h"
#include "gameData/MapConsts.h"
#include "s25util/Log.h"

EventState::EventState(SerializedGameData& sgd) : elapsed(sgd.PopUnsignedInt()), length(sgd.PopUnsignedInt()) {}

noMovable::noMovable(const NodalObjectType nop, const MapPoint pos)
    : noCoordBase(nop, pos), curMoveDir(4), ascent(0), moving(false), current_ev(nullptr)
{}

void noMovable::Serialize(SerializedGameData& sgd) const
{
    noCoordBase::Serialize(sgd);

    sgd.PushUnsignedChar(curMoveDir.toUInt());
    sgd.PushUnsignedChar(ascent);
    sgd.PushEvent(current_ev);
    sgd.PushUnsignedInt(pauseEv.elapsed);
    sgd.PushUnsignedInt(pauseEv.length);
    sgd.PushBool(moving);
}

noMovable::noMovable(SerializedGameData& sgd, const unsigned obj_id)
    : noCoordBase(sgd, obj_id), curMoveDir(sgd.PopUnsignedChar()), ascent(sgd.PopUnsignedChar())
{
    current_ev = sgd.PopEvent();
    pauseEv = EventState(sgd);
    moving = sgd.PopBool();
}

void noMovable::Walk()
{
    moving = false;
    gwg->RemoveFigure(pos, this);
    pos = gwg->GetNeighbour(pos, curMoveDir);
    gwg->AddFigure(pos, this);
}

void noMovable::FaceDir(Direction newDir)
{
    curMoveDir = newDir;
}

void noMovable::StartMoving(const Direction dir, unsigned gf_length)
{
    RTTR_Assert(!moving);

    // Ist das Wesen stehengeblieben mitten aufm Weg?
    if(IsStoppedBetweenNodes())
    {
        // Das Laufevent fortführen
        RTTR_Assert(dir == curMoveDir);
        // Avoid setting an event for current gf by increasing the length
        if(pauseEv.length == pauseEv.elapsed)
            pauseEv.length++;
        current_ev = GetEvMgr().AddEvent(this, pauseEv.length, 0, pauseEv.elapsed);
        pauseEv = EventState();
        moving = true;
        return;
    }

    // Steigung ermitteln, muss entsprechend langsamer (hoch) bzw. schneller (runter) laufen
    // runter natürlich nich so viel schneller werden wie langsamer hoch
    switch(int(gwg->GetNeighbourNode(pos, dir).altitude) - int(gwg->GetNode(pos).altitude))
    {
        default: ascent = 3; break; // gerade
        case 1:
            ascent = 4;
            gf_length += (gf_length / 2);
            break; // leicht hoch
        case 2:
        case 3:
            ascent = 5;
            gf_length *= 2;
            break; // mittelsteil hoch
        case 4:
        case 5:
            ascent = 6;
            gf_length *= 3;
            break;                  // steil hoch
        case -1: ascent = 2; break; // leicht runter
        case -2:
        case -3: ascent = 1; break; // mittelsteil runter
        case -4:
        case -5: ascent = 0; break; // steil runter
    }

    current_ev = GetEvMgr().AddEvent(this, gf_length);
    this->curMoveDir = dir;
    moving = true;
}

DrawPoint noMovable::CalcRelative(DrawPoint curPt, DrawPoint nextPt) const
{
    if(current_ev)
    {
        RTTR_Assert(current_ev->length > 0);
        if(current_ev->length == 0)
        {
            LOG.write("WARNING: Bug detected (GF: %u). Please report this with the savegame and replay. noMovable::CalcRelative: "
                      "current_ev->gf_length = 0!\n")
              % GetEvMgr().GetCurrentGF();
            return Position(0, 0);
        }
    }

    RTTR_Assert(IsMoving() || IsStoppedBetweenNodes());

    using milliseconds_i32_t = std::chrono::duration<int32_t, std::milli>;

    // Wenn wir mittem aufm Weg stehen geblieben sind, die gemerkten Werte jeweils nehmen
    EventState curState;
    milliseconds_i32_t frame_time;
    if(current_ev)
    {
        curState.elapsed = GetEvMgr().GetCurrentGF() - current_ev->startGF;
        curState.length = current_ev->length;
        frame_time = GAMECLIENT.GetFrameTime();
    } else
    {
        curState = pauseEv;
        frame_time = milliseconds_i32_t::zero();
    }

    // Convert to real world time
    const milliseconds_i32_t gfLength = GAMECLIENT.GetGFLength();
    // Time since the start of the event
    milliseconds_i32_t curTimePassed = curState.elapsed * gfLength + frame_time;
    // Duration of the event
    milliseconds_i32_t duration = curState.length * gfLength;

    // We are in that event
    RTTR_Assert(curTimePassed <= duration);

    // Check for map border crossing
    const Position mapDrawSize = gwg->GetSize() * Position(TR_W, TR_H);
    if(std::abs(nextPt.x - curPt.x) >= mapDrawSize.x / 2)
    {
        // So we need to get closer to nextPt
        if(curPt.x > nextPt.x)
            curPt.x -= mapDrawSize.x;
        else
            curPt.x += mapDrawSize.x;
    }
    if(std::abs(curPt.y - nextPt.y) >= mapDrawSize.y / 2)
    {
        if(curPt.y > nextPt.y)
            curPt.y -= mapDrawSize.y;
        else
            curPt.y += mapDrawSize.y;
    }

    return ((nextPt - curPt) * curTimePassed.count()) / duration.count();
}

/// Interpoliert fürs Laufen zwischen zwei Kartenpunkten
DrawPoint noMovable::CalcWalkingRelative() const
{
    Position curPt = gwg->GetNodePos(pos);
    Position nextPt = gwg->GetNodePos(gwg->GetNeighbour(pos, curMoveDir));

    return CalcRelative(curPt, nextPt);
}

void noMovable::PauseWalking()
{
    RTTR_Assert(IsMoving());
    // Frames festhalten, bis zu denen wir gekommen sind
    pauseEv = EventState(GetEvMgr().GetCurrentGF() - current_ev->startGF, current_ev->length);
    // Event abmelden
    GetEvMgr().RemoveEvent(current_ev);
    moving = false;
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
        return gwg->GetNeighbour(pos, curMoveDir);

    return pos;
}
