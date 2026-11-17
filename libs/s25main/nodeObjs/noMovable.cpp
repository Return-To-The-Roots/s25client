// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "noMovable.h"

#include "EventManager.h"
#include "GameEvent.h"
#include "SerializedGameData.h"
#include "enum_cast.hpp"
#include "network/GameClient.h"
#include "world/GameWorld.h"
#include "gameData/MapConsts.h"
#include "s25util/Log.h"

EventState::EventState(SerializedGameData& sgd) : elapsed(sgd.PopUnsignedInt()), length(sgd.PopUnsignedInt()) {}

noMovable::noMovable(const NodalObjectType nop, const MapPoint pos)
    : noCoordBase(nop, pos), curMoveDir(Direction::SouthEast), ascent(0), moving(false), current_ev(nullptr)
{}

void noMovable::serialize(SerializedGameData& sgd) const
{
    noCoordBase::serialize(sgd);

    sgd.pushEnum<uint8_t>(curMoveDir);
    sgd.PushUnsignedChar(ascent);
    sgd.pushEvent(current_ev);
    sgd.PushUnsignedInt(pauseEv.elapsed);
    sgd.PushUnsignedInt(pauseEv.length);
    sgd.PushBool(moving);
}

noMovable::noMovable(SerializedGameData& sgd, const unsigned obj_id)
    : noCoordBase(sgd, obj_id), curMoveDir(sgd.pop<Direction>()), ascent(sgd.PopUnsignedChar())
{
    current_ev = sgd.popEvent();
    pauseEv = EventState(sgd);
    moving = sgd.PopBool();
}

void noMovable::walk()
{
    moving = false;
    const MapPoint oldPos = pos;
    pos = world->getNeighbour(pos, curMoveDir);
    world->addFigure(pos, world->removeFigure(oldPos, *this));
}

void noMovable::faceDir(Direction newDir)
{
    curMoveDir = newDir;
}

void noMovable::startMoving(const Direction dir, unsigned gf_length)
{
    RTTR_Assert(!moving);

    // Ist das Wesen stehengeblieben mitten aufm Weg?
    if(isStoppedBetweenNodes())
    {
        // Das Laufevent fortführen
        RTTR_Assert(dir == curMoveDir);
        // Avoid setting an event for current gf by increasing the length
        if(pauseEv.length == pauseEv.elapsed)
            pauseEv.length++;
        current_ev = getEvMgr().addEvent(this, pauseEv.length, 0, pauseEv.elapsed);
        pauseEv = EventState();
        moving = true;
        return;
    }

    // Steigung ermitteln, muss entsprechend langsamer (hoch) bzw. schneller (runter) laufen
    // runter natürlich nich so viel schneller werden wie langsamer hoch
    switch(int(world->getNeighbourNode(pos, dir).altitude) - int(world->getNode(pos).altitude))
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

    current_ev = getEvMgr().addEvent(this, gf_length);
    this->curMoveDir = dir;
    moving = true;
}

DrawPoint noMovable::calcRelative(DrawPoint curPt, DrawPoint nextPt) const
{
    if(current_ev)
    {
        RTTR_Assert(current_ev->length > 0);
        if(current_ev->length == 0)
        {
            LOG.write("WARNING: Bug detected (GF: %u). Please report this with the savegame and replay. "
                      "noMovable::CalcRelative: "
                      "current_ev->gf_length = 0!\n")
              % getEvMgr().getCurrentGF();
            return Position(0, 0);
        }
    }

    RTTR_Assert(isMoving() || isStoppedBetweenNodes());

    using milliseconds_i32_t = std::chrono::duration<int32_t, std::milli>;

    // Wenn wir mittem aufm Weg stehen geblieben sind, die gemerkten Werte jeweils nehmen
    EventState curState;
    milliseconds_i32_t frame_time;
    if(current_ev)
    {
        curState.elapsed = getEvMgr().getCurrentGF() - current_ev->startGF;
        curState.length = current_ev->length;
        frame_time = GAMECLIENT.getFrameTime();
    } else
    {
        curState = pauseEv;
        frame_time = milliseconds_i32_t::zero();
    }

    // Convert to real world time
    const milliseconds_i32_t gfLength = GAMECLIENT.getGFLength();
    // Time since the start of the event
    milliseconds_i32_t curTimePassed = curState.elapsed * gfLength + frame_time;
    // Duration of the event
    milliseconds_i32_t duration = curState.length * gfLength;

    // We are in that event
    RTTR_Assert(curTimePassed <= duration);

    // Check for map border crossing
    const Position mapDrawSize = world->getSize() * Position(TR_W, TR_H);
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
DrawPoint noMovable::calcWalkingRelative() const
{
    Position curPt = world->getNodePos(pos);
    Position nextPt = world->getNodePos(world->getNeighbour(pos, curMoveDir));

    return calcRelative(curPt, nextPt);
}

void noMovable::pauseWalking()
{
    RTTR_Assert(isMoving());
    // Frames festhalten, bis zu denen wir gekommen sind
    pauseEv = EventState(getEvMgr().getCurrentGF() - current_ev->startGF, current_ev->length);
    // Event abmelden
    getEvMgr().removeEvent(current_ev);
    moving = false;
}

/// Gibt zurück, ob sich das angegebene Objekt zwischen zwei Punkten bewegt
bool noMovable::isMoving() const
{
    return current_ev && (current_ev->id == 0);
}

/// Gibt die Position zurück, wo wir uns hinbewegen (selbe Position, wenn Schiff steht)
MapPoint noMovable::getDestinationForCurrentMove() const
{
    // Bewegt sich das Ding gerade?
    if(isMoving())
        // Dann unsere Zielrichtung zur Berechnung verwenden
        return world->getNeighbour(pos, curMoveDir);

    return pos;
}
