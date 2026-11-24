// Copyright (C) 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nobChurch.h"
#include "EventManager.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "SoundManager.h"
#include "buildings/nobBaseWarehouse.h"
#include "buildings/nobMilitary.h"
#include "figures/nofBuildingWorker.h"
#include "figures/nofChurchVisitor.h"
#include "helpers/containerUtils.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "world/GameWorld.h"
#include <algorithm>
#include <vector>

nobChurch::nobChurch(const MapPoint pos, const unsigned char player, const Nation nation)
    : nobUsual(BuildingType::Church, pos, player, nation)
{
    productivity = 0;
    lastProductivities.fill(0);
    serviceEv_ = getEvMgr().addEvent(this, churchaddon::CHURCH_FIRST_SERVICE_DELAY, CHURCH_EV_SERVICE);
}

nobChurch::nobChurch(SerializedGameData& sgd, const unsigned obj_id)
    : nobUsual(sgd, obj_id), numBlessings_(sgd.popUnsignedChar()), serviceState_(sgd.pop<ServiceState>()),
      serviceEv_(sgd.popEvent()), serviceEndEv_(sgd.popEvent())
{
    sgd.popObjectContainer(visitors_, GO_Type::NofChurchvisitor);
}

nobChurch::~nobChurch() = default;

void nobChurch::serialize(SerializedGameData& sgd) const
{
    nobUsual::serialize(sgd);

    sgd.pushUnsignedChar(numBlessings_);
    sgd.pushEnum<uint8_t>(serviceState_);
    sgd.pushEvent(serviceEv_);
    sgd.pushEvent(serviceEndEv_);
    sgd.pushObjectContainer(visitors_, true);
}

void nobChurch::DestroyBuilding()
{
    serviceState_ = ServiceState::Idle;
    auto visitors = std::move(visitors_);
    visitors_.clear();
    for(auto* visitor : visitors)
        visitor->churchDestroyed();

    getEvMgr().removeEvent(serviceEv_);
    getEvMgr().removeEvent(serviceEndEv_);
    world->getSoundMgr().stopSounds(*this);

    nobUsual::DestroyBuilding();
}

void nobChurch::draw(DrawPoint drawPt)
{
    nobUsual::draw(drawPt);
    if(serviceState_ == ServiceState::InService)
    {
        if(auto* bmp = LOADER.getImageN("church", static_cast<unsigned>(churchaddon::SharedSprite::Dove)))
            bmp->drawFull(drawPt + DrawPoint(0, churchaddon::getDoveOffsetY(nation)));
    }
}

void nobChurch::handleEvent(const unsigned id)
{
    if(id == CHURCH_EV_SERVICE)
    {
        serviceEv_ = nullptr;
        if(serviceState_ == ServiceState::Idle)
            startService();
    } else if(id == CHURCH_EV_GATHER_TIMEOUT)
    {
        serviceEndEv_ = nullptr;
        if(serviceState_ == ServiceState::Gathering)
        {
            std::vector<nofChurchVisitor*> stragglers;
            for(auto it = visitors_.begin(); it != visitors_.end();)
            {
                if(!(*it)->isAttendingService())
                {
                    stragglers.push_back(*it);
                    it = visitors_.erase(it);
                } else
                    ++it;
            }
            for(auto* straggler : stragglers)
                straggler->churchDestroyed();

            if(!visitors_.empty())
                beginServiceInside();
            else
                finishService();
        }
    } else if(id == CHURCH_EV_SERVICE_END)
    {
        serviceEndEv_ = nullptr;
        if(serviceState_ == ServiceState::InService)
            finishService();
    } else if(id == 1)
    {
        // Keep Church productivity tied to soldiers blessed in the last service rather than worker busy time
        const unsigned short savedProductivity = productivity;
        nobUsual::handleEvent(id);
        productivity = savedProductivity;
        lastProductivities.fill(savedProductivity);
    } else
        nobUsual::handleEvent(id);
}

void nobChurch::addBlessing()
{
    if(numBlessings_ < churchaddon::MAX_CHURCH_BLESSINGS)
        ++numBlessings_;
}

unsigned nobChurch::getNumSoldiersInside() const
{
    return static_cast<unsigned>(std::count_if(visitors_.begin(), visitors_.end(), [](const auto* v) {
        return v->isSoldier() && v->isAttendingService();
    }));
}

unsigned nobChurch::getNumSoldiersEnRoute() const
{
    return static_cast<unsigned>(std::count_if(visitors_.begin(), visitors_.end(), [](const auto* v) {
        return v->isSoldier() && !v->isAttendingService();
    }));
}

bool nobChurch::allVisitorsInside() const
{
    return std::all_of(visitors_.begin(), visitors_.end(), [](const auto* v) { return v->isAttendingService(); });
}

void nobChurch::visitorArrived(nofChurchVisitor& visitor)
{
    if(serviceState_ != ServiceState::Gathering)
    {
        visitors_.remove(&visitor);
        visitor.departFromChurch();
        return;
    }
    if(allVisitorsInside())
        beginServiceInside();
}

void nobChurch::visitorLost(nofChurchVisitor& visitor)
{
    RTTR_Assert(helpers::contains(visitors_, &visitor));
    visitors_.remove(&visitor);
    if(serviceState_ == ServiceState::Gathering)
    {
        if(visitors_.empty())
            finishService();
        else if(allVisitorsInside())
            beginServiceInside();
    }
}

void nobChurch::startService()
{
    world->getSoundMgr().playChurchServiceSound(*this);

    GamePlayer& owner = world->getPlayer(player);

    for(nobBaseWarehouse* wh : owner.getBuildingRegister().getStorehouses())
    {
        const RoadPathDirection dir = world->findHumanPathOnRoads(*wh, *this);
        if(dir != RoadPathDirection::None && dir != RoadPathDirection::Ship)
        {
            auto newVisitors = wh->sendChurchVisitors(*this);
            visitors_.insert(visitors_.end(), newVisitors.begin(), newVisitors.end());
        }
    }

    for(nobMilitary* mil : owner.getBuildingRegister().getMilitaryBuildings())
    {
        const RoadPathDirection dir = world->findHumanPathOnRoads(*mil, *this);
        if(dir != RoadPathDirection::None && dir != RoadPathDirection::Ship)
        {
            auto newVisitors = mil->sendChurchVisitors(*this);
            visitors_.insert(visitors_.end(), newVisitors.begin(), newVisitors.end());
        }
    }

    if(visitors_.empty())
    {
        beginServiceInside();
        return;
    }

    serviceState_ = ServiceState::Gathering;
    getEvMgr().removeEvent(serviceEndEv_);
    serviceEndEv_ = getEvMgr().addEvent(this, churchaddon::CHURCH_MAX_GATHER_WAIT, CHURCH_EV_GATHER_TIMEOUT);
}

void nobChurch::beginServiceInside()
{
    serviceState_ = ServiceState::InService;
    getEvMgr().removeEvent(serviceEndEv_);
    serviceEndEv_ = getEvMgr().addEvent(this, churchaddon::CHURCH_SERVICE_DURATION, CHURCH_EV_SERVICE_END);
    world->getSoundMgr().playChurchServiceSound(*this);
}

void nobChurch::finishService()
{
    unsigned blessedCount = 0;
    if(serviceState_ == ServiceState::InService)
    {
        GamePlayer& owner = world->getPlayer(player);
        for(auto* visitor : visitors_)
        {
            if(visitor->isSoldier() && !visitor->hasHealing() && numBlessings_ > 0)
            {
                --numBlessings_;
                visitor->grantHealing();
                owner.increaseInventoryJob(jobEnumToHealedSoldierEnum(visitor->getJobType()), 1);
                ++blessedCount;
            }
        }
        if(blessedCount > 0 && worker)
            worker->gotWareOrProductionAllowed();
    }

    const unsigned short serviceProductivity =
      static_cast<unsigned short>(std::min<unsigned>(100u, (blessedCount * 100u) / churchaddon::MAX_CHURCH_BLESSINGS));
    productivity = serviceProductivity;
    lastProductivities.fill(serviceProductivity);

    serviceState_ = ServiceState::Idle;
    getEvMgr().removeEvent(serviceEndEv_);

    auto departing = std::move(visitors_);
    visitors_.clear();
    for(auto* visitor : departing)
        visitor->departFromChurch();

    getEvMgr().removeEvent(serviceEv_);
    serviceEv_ = getEvMgr().addEvent(this, churchaddon::CHURCH_SERVICE_INTERVAL, CHURCH_EV_SERVICE);
}
