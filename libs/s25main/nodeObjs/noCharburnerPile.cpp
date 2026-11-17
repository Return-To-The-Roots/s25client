// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "noCharburnerPile.h"

#include "EventManager.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "network/GameClient.h"
#include "noEnvObject.h"
#include "noFire.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "world/GameWorld.h"

/// Length of the smoldering
const unsigned SMOLDERING_LENGTH = 3000;
/// if nothing happens in this amount of GF the pile will catch fire and burn down (removes inactive piles)
/// it takes about ~4000 gf to remove the cover & harvest the coal from a priority pile - so after this delay there are
/// ~8k gf remaining for the charburner to start working on the started pile again
/// -> this should only run out if there is either a very long time without resources or the building was
/// destroyed/stopped
const unsigned SELFDESTRUCT_DELAY = 12000;

/// Work steps for the construction of the wood pile and the cover
const std::array<unsigned short, 2> CONSTRUCTION_WORKING_STEPS = {6, 6};
/// Work steps for one graphical step during the remove of the cover
const unsigned short REMOVECOVER_WORK_STEPS = 1;
/// Work steps for one graphical step during the "harvest"
const unsigned short HARVEST_WORK_STEPS = 1;

noCharburnerPile::noCharburnerPile(const MapPoint pos)
    : noCoordBase(NodalObjectType::CharburnerPile, pos), state(State::Wood), step(0), sub_step(1), event(nullptr)
{}

noCharburnerPile::~noCharburnerPile() = default;

void noCharburnerPile::destroy()
{
    getEvMgr().removeEvent(event);

    // Bauplätze drumrum neu berechnen
    world->recalcBQAroundPointBig(pos);

    noCoordBase::destroy();
}

void noCharburnerPile::serialize(SerializedGameData& sgd) const
{
    noCoordBase::serialize(sgd);

    sgd.pushEnum<uint8_t>(state);
    sgd.pushUnsignedShort(step);
    sgd.pushUnsignedShort(sub_step);
    sgd.pushEvent(event);
}

noCharburnerPile::noCharburnerPile(SerializedGameData& sgd, const unsigned obj_id)
    : noCoordBase(sgd, obj_id), state(sgd.pop<State>()), step(sgd.popUnsignedShort()), sub_step(sgd.popUnsignedShort()),
      event(sgd.popEvent())
{}

void noCharburnerPile::draw(DrawPoint drawPt)
{
    switch(state)
    {
        case State::Wood:
        {
            // Draw sand on which the wood stack is constructed
            LOADER.getImageN("charburner_bobs", 25)->drawFull(drawPt);

            glArchivItem_Bitmap* image;
            if(step == 0)
                image = LOADER.getImageN("charburner_bobs", 26);
            else
            {
                image = LOADER.getImageN("charburner_bobs", 28);

                // Draw wood pile beneath the cover
                LOADER.getImageN("charburner_bobs", 26)->drawFull(drawPt);
            }

            unsigned short progress = sub_step * 100 / CONSTRUCTION_WORKING_STEPS[step];
            if(progress != 0)
                image->drawPercent(drawPt, progress);
        }
            return;
        case State::Smoldering:
        {
            LOADER
              .getImageN("charburner_bobs",
                         27 + GAMECLIENT.getGlobalAnimation(2, 10, 1, getObjId() + this->pos.x * 10 + this->pos.y * 10))
              ->drawFull(drawPt);

            // Dann Qualm zeichnen
            unsigned globalAnimation = GAMECLIENT.getGlobalAnimation(8, 5, 2, (this->pos.x + this->pos.y) * 100);
            LOADER.getMapTexture(692 + 1 * 8 + globalAnimation)
              ->drawFull(drawPt + DrawPoint(21, -11), 0x99EEEEEE); //-V525
            LOADER.getMapTexture(692 + 2 * 8 + globalAnimation)->drawFull(drawPt - DrawPoint(2, 06), 0x99EEEEEE);
            LOADER.getMapTexture(692 + 1 * 8 + globalAnimation)->drawFull(drawPt - DrawPoint(25, 11), 0x99EEEEEE);
            LOADER.getMapTexture(692 + 3 * 8 + globalAnimation)->drawFull(drawPt - DrawPoint(2, 35), 0x99EEEEEE);
        }
            return;
        case State::RemoveCover:
        {
            LOADER.getImageN("charburner_bobs", 28 + step)->drawFull(drawPt);
        }
            return;
        case State::Harvest:
        {
            LOADER.getImageN("charburner_bobs", 34 + step)->drawFull(drawPt);
        }
            return;
        default: return;
    }
}

void noCharburnerPile::handleEvent(const unsigned /*id*/)
{
    // Smoldering is over
    // Pile is ready for the remove of the cover
    if(state == State::Smoldering)
    {
        state = State::RemoveCover;
        // start a selfdestruct timer
        event = getEvMgr().addEvent(this, SELFDESTRUCT_DELAY, 0);
    } else
    {
        // selfdestruct!
        event = nullptr;
        world->setNO(pos, new noFire(pos, false), true);
        world->recalcBQAroundPoint(pos);
        getEvMgr().addToKillList(this);
    }
}

/// Charburner has worked on it --> Goto next step
void noCharburnerPile::nextStep()
{
    // reset selfdestruct timer
    if(event)
        getEvMgr().removeEvent(event);
    event = getEvMgr().addEvent(this, SELFDESTRUCT_DELAY, 0);

    // execute step
    switch(state)
    {
        default: return;
        case State::Wood:
        {
            ++sub_step;
            if(sub_step == CONSTRUCTION_WORKING_STEPS[step])
            {
                ++step;
                sub_step = 0;

                // Reached new state?
                if(step == 2)
                {
                    step = 0;
                    state = State::Smoldering;
                    getEvMgr().removeEvent(event);
                    event = getEvMgr().addEvent(this, SMOLDERING_LENGTH, 0);
                }
            }
        }
            return;
        case State::RemoveCover:
        {
            ++sub_step;
            if(sub_step == REMOVECOVER_WORK_STEPS)
            {
                ++step;
                sub_step = 0;

                // Reached new state?
                if(step == 6)
                {
                    state = State::Harvest;
                    step = 0;
                }
            }
        }
            return;
        case State::Harvest:
        {
            ++sub_step;
            if(sub_step == HARVEST_WORK_STEPS)
            {
                ++step;
                sub_step = 0;

                // Reached new state?
                if(step == 6)
                {
                    // Add an empty pile as environmental object
                    world->setNO(pos, new noEnvObject(pos, 40, 6), true);
                    getEvMgr().addToKillList(this);

                    // BQ drumrum neu berechnen
                    world->recalcBQAroundPoint(pos);
                }
            }
        }
            return;
    }
}

noCharburnerPile::WareType noCharburnerPile::getNeededWareType() const
{
    if(sub_step % 2 == 0)
        return WareType::Wood;
    else
        return WareType::Grain;
}
