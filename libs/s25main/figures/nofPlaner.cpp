// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nofPlaner.h"
#include "EventManager.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "SoundManager.h"
#include "buildings/noBuildingSite.h"
#include "network/GameClient.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "random/Random.h"
#include "world/GameWorld.h"
#include "gameData/JobConsts.h"

nofPlaner::nofPlaner(const MapPoint pos, const unsigned char player, noBuildingSite* building_site)
    : noFigure(Job::Planer, pos, player, building_site), building_site(building_site), state(PlanerState::FigureWork),
      pd(PlaningDir::NotWorking)
{}

void nofPlaner::serialize(SerializedGameData& sgd) const
{
    noFigure::serialize(sgd);

    sgd.pushEnum<uint8_t>(state);
    sgd.pushObject(building_site, true);
    sgd.pushEnum<uint8_t>(pd);
}

nofPlaner::nofPlaner(SerializedGameData& sgd, const unsigned obj_id) : noFigure(sgd, obj_id)
{
    state = sgd.pop<PlanerState>();
    building_site = sgd.popObject<noBuildingSite>(GO_Type::Buildingsite);
    pd = sgd.pop<PlaningDir>();
}

void nofPlaner::goalReached()
{
    state = PlanerState::Walking;

    // Zufällig Uhrzeigersinn oder dagegen
    pd = (RANDOM_RAND(2) == 0) ? (PlaningDir::Clockwise) : (PlaningDir::Counterclockwise);

    // Je nachdem erst nach rechts oder links gehen
    startWalking((pd == PlaningDir::Clockwise) ? Direction::SouthWest : Direction::East);
}

void nofPlaner::walked()
{
    /// Zur Baustelle zurückgelaufen? (=fertig)
    if(pos == building_site->getPos())
    {
        // Baustelle Bescheid sagen
        building_site->planingFinished();

        state = PlanerState::FigureWork;

        // Nach Hause laufen bzw. auch rumirren
        rs_pos = 0;
        rs_dir = true;
        cur_rs = world->getSpecObj<noRoadNode>(pos)->getRoute(Direction::SouthEast);
        building_site = nullptr;

        goHome();
        startWalking(Direction::SouthEast);
    } else
    {
        /// Anfangen zu arbeiten
        current_ev = getEvMgr().addEvent(this, JOB_CONSTS[Job::Planer].work_length, 1);
        state = PlanerState::Planing;
    }
}

void nofPlaner::abrogateWorkplace()
{
    if(building_site)
    {
        state = PlanerState::FigureWork;
        building_site->abrogate();
        building_site = nullptr;
    }
}

void nofPlaner::abrogate()
{
    state = PlanerState::FigureWork;
    noFigure::abrogate();
    RTTR_Assert(!building_site);
}

void nofPlaner::lostWork()
{
    building_site = nullptr;

    if(state == PlanerState::FigureWork)
        goHome();
    else
    {
        // Event ggf. abmelden
        if(state == PlanerState::Planing)
        {
            getEvMgr().removeEvent(current_ev);
            /// Sounds abmelden
            world->getSoundMgr().stopSounds(*this);
        }

        startWandering();
    }
}

void nofPlaner::draw(DrawPoint drawPt)
{
    switch(state)
    {
        case PlanerState::FigureWork:
        case PlanerState::Walking:
        {
            drawWalkingBobJobs(drawPt, Job::Planer);
        }
        break;
        case PlanerState::Planing:
        {
            // 41

            /// Animation des Planierers
            unsigned nowId = GAMECLIENT.interpolate(69, current_ev);

            // spezielle Animation am Ende
            const std::array<unsigned, 21> ANIMATION = {273, 273, 273, 273, 273, 274, 274, 275, 276, 276, 276,
                                                        276, 276, 276, 276, 276, 276, 276, 277, 277, 278};

            unsigned bobId;
            if(nowId < 20)
                bobId = 253 + nowId;
            else if(nowId < 41)
                bobId = ANIMATION[nowId - 20];
            else if(nowId < 55)
                bobId = 253 + nowId - 41;
            else
                bobId = 253 + nowId - 55;
            LOADER.getPlayerImage("rom_bobs", bobId)
              ->drawFull(drawPt, COLOR_WHITE, world->getPlayer(building_site->getPlayer()).color);

            // Schaufel-Sound
            if(nowId == 5 || nowId == 46 || nowId == 60)
                world->getSoundMgr().playNOSound(76, *this, nowId, 200);
            // Tret-Sound
            else if(nowId == 20 || nowId == 28)
                world->getSoundMgr().playNOSound(66, *this, nowId, 200);
        }
        break;
    }
}

void nofPlaner::handleDerivedEvent(const unsigned id)
{
    if(id == 1)
    {
        // Planieren (falls Baustelle noch existiert)
        if(building_site)
            world->changeAltitude(pos, world->getNode(building_site->getPos()).altitude);
        /// Sounds abmelden
        world->getSoundMgr().stopSounds(*this);

        state = PlanerState::Walking;

        // Planierung fertig --> weiterlaufen
        Direction curDir = getCurMoveDir();

        // Das erste Mal gelaufen?
        if((pd == PlaningDir::Clockwise && curDir == Direction::SouthWest)
           || (pd == PlaningDir::Counterclockwise && curDir == Direction::East))
            startWalking(Direction::NorthWest);
        // Fertig -> zur Baustelle zurücklaufen
        else if(pd == PlaningDir::Clockwise && curDir == Direction::SouthEast)
            startWalking(Direction::West);
        else if(pd == PlaningDir::Counterclockwise && curDir == Direction::SouthEast)
            startWalking(Direction::NorthEast);

        // In nächste Richtung gehen
        else if(pd == PlaningDir::Clockwise)
            startWalking(curDir + 1u);
        else
            startWalking(curDir - 1u);
    }
}
