// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nofPassiveSoldier.h"
#include "EventManager.h"
#include "GamePlayer.h"
#include "SerializedGameData.h"
#include "buildings/nobMilitary.h"
#include "helpers/containerUtils.h"
#include "helpers/pointerContainerUtils.h"
#include "random/Random.h"
#include "world/GameWorld.h"
#include "gameData/MilitaryConsts.h"

nofPassiveSoldier::nofPassiveSoldier(const nofSoldier& soldier) : nofSoldier(soldier), healing_event(nullptr)
{
    // Soldat von einer Mission nach Hause gekommen --> ggf heilen!
    heal();
    // Laufevent nullen, laufen ja nicht mehr
    current_ev = nullptr;
}

nofPassiveSoldier::nofPassiveSoldier(const MapPoint pos, const unsigned char player, nobBaseMilitary* const goal,
                                     nobMilitary* const home, const unsigned char rank)
    : nofSoldier(pos, player, goal, home, rank), healing_event(nullptr)
{}

nofPassiveSoldier::~nofPassiveSoldier() = default;

void nofPassiveSoldier::destroy()
{
    getEvMgr().removeEvent(healing_event);
    nofSoldier::destroy();
}

void nofPassiveSoldier::serialize(SerializedGameData& sgd) const
{
    nofSoldier::serialize(sgd);

    sgd.pushEvent(healing_event);
}

nofPassiveSoldier::nofPassiveSoldier(SerializedGameData& sgd, const unsigned obj_id)
    : nofSoldier(sgd, obj_id), healing_event(sgd.popEvent())
{}

void nofPassiveSoldier::draw(DrawPoint drawPt)
{
    // Soldat normal laufend zeichnen
    drawWalkingBobJobs(drawPt, job_);
}

void nofPassiveSoldier::handleDerivedEvent(const unsigned id)
{
    switch(id)
    {
        // "Heilungs-Event"
        case 1:
        {
            healing_event = nullptr;

            // Sind wir noch im Haus?
            if(fs == FigureState::Job)
            {
                // Dann uns heilen, wenn wir nicht schon gesund sind
                if(hitpoints < getMaxHitpoints())
                {
                    ++hitpoints;

                    // Sind wir immer noch nicht gesund? Dann neues Event anmelden!
                    if(hitpoints < getMaxHitpoints())
                        healing_event =
                          getEvMgr().addEvent(this, CONVALESCE_TIME + RANDOM_RAND(CONVALESCE_TIME_RANDOM), 1);
                }
            }
        }
        break;
    }
}

void nofPassiveSoldier::heal()
{
    // Schon ein Event angemeldet?
    // Dann muss dieses entfernt werden, wahrscheinlich war er zuvor draußen gewesen
    if(healing_event)
    {
        getEvMgr().removeEvent(healing_event);
        healing_event = nullptr;
    }

    // Ist er verletzt?
    // Dann muss er geheilt werden
    if(hitpoints < getMaxHitpoints())
        healing_event = getEvMgr().addEvent(this, CONVALESCE_TIME + RANDOM_RAND(CONVALESCE_TIME_RANDOM), 1);
}

void nofPassiveSoldier::goalReached()
{
    static_cast<nobMilitary*>(homeBld)->addPassiveSoldier(world->removeFigure(pos, *this));
}

void nofPassiveSoldier::leaveBuilding()
{
    // Nach Hause in ein Lagerhaus gehen
    rs_dir = true;
    rs_pos = 1;
    cur_rs = homeBld->getRoute(Direction::SouthEast);
    goHome();

    homeBld = nullptr;
}

void nofPassiveSoldier::upgrade()
{
    // We must not be in the buildings list while upgrading. This would destroy the ordered list
    RTTR_Assert(!homeBld || !static_cast<nobMilitary*>(homeBld)->isInTroops(*this));
    // Einen Rang höher
    job_ = Job(unsigned(job_) + 1);

    // wieder heilen bzw. Hitpoints anpasen
    GamePlayer& owner = world->getPlayer(player);
    hitpoints = getMaxHitpoints();

    // Inventur entsprechend erhöhen und verringern
    owner.increaseInventoryJob(job_, 1);
    owner.decreaseInventoryJob(Job(unsigned(job_) - 1), 1);
    if(hasArmor_)
    {
        owner.increaseInventoryJob(jobEnumToAmoredSoldierEnum(job_), 1);
        owner.decreaseInventoryJob(jobEnumToAmoredSoldierEnum(Job(unsigned(job_) - 1)), 1);
    }
    if(hasHealing_)
    {
        owner.increaseInventoryJob(jobEnumToHealedSoldierEnum(job_), 1);
        owner.decreaseInventoryJob(jobEnumToHealedSoldierEnum(Job(unsigned(job_) - 1)), 1);
    }
}

void nofPassiveSoldier::walked()
{
    throw std::logic_error("Passive soldiers shall not walk");
}

void nofPassiveSoldier::notNeeded()
{
    homeBld = nullptr;
    goHome();
}

nobMilitary* nofPassiveSoldier::getHome() const
{
    return checkedCast<nobMilitary*>(homeBld);
}
