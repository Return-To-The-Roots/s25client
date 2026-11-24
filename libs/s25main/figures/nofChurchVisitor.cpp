// Copyright (C) 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nofChurchVisitor.h"
#include "EventManager.h"
#include "FindWhConditions.h"
#include "GamePlayer.h"
#include "SerializedGameData.h"
#include "buildings/noBaseBuilding.h"
#include "buildings/nobBaseWarehouse.h"
#include "buildings/nobChurch.h"
#include "buildings/nobMilitary.h"
#include "figures/nofPassiveSoldier.h"
#include "world/GameWorld.h"
#include "gameData/MilitaryConsts.h"

nofChurchVisitor::nofChurchVisitor(const Job job, const MapPoint pos, const unsigned char player,
                                   nobChurch* const church, noBaseBuilding* const origin, const uint8_t soldierHp,
                                   const bool hasArmor, const bool hasHealing)
    : noFigure(job, pos, player, church), state_(State::GoingToChurch), church_(church), origin_(origin),
      soldierHp_(soldierHp)
{
    hasArmor_ = hasArmor;
    hasHealing_ = hasHealing;
}

nofChurchVisitor::nofChurchVisitor(SerializedGameData& sgd, const unsigned obj_id)
    : noFigure(sgd, obj_id), state_(sgd.pop<State>()), church_(sgd.popObject<nobChurch>()),
      origin_(sgd.popObject<noBaseBuilding>()), soldierHp_(sgd.popUnsignedChar())
{}

void nofChurchVisitor::serialize(SerializedGameData& sgd) const
{
    noFigure::serialize(sgd);

    sgd.pushEnum<uint8_t>(state_);
    sgd.pushObject(church_);
    sgd.pushObject(origin_);
    sgd.pushUnsignedChar(soldierHp_);
}

void nofChurchVisitor::destroy()
{
    church_ = nullptr;
    origin_ = nullptr;
    noFigure::destroy();
}

void nofChurchVisitor::draw(DrawPoint drawPt)
{
    if(state_ != State::AttendingService)
        drawWalking(drawPt);
}

void nofChurchVisitor::grantHealing()
{
    RTTR_Assert(isSoldier());
    hasHealing_ = true;
    soldierHp_ = static_cast<uint8_t>(getMaxSoldierHitpoints(getSoldierRank(job_), true));
}

void nofChurchVisitor::notifyOriginLost()
{
    if(!origin_)
        return;

    if(nobBaseWarehouse::isStorehouseGOT(origin_->getGOT()))
        static_cast<nobBaseWarehouse*>(origin_)->churchVisitorLost(*this);
    else if(origin_->getGOT() == GO_Type::NobMilitary)
        static_cast<nobMilitary*>(origin_)->churchVisitorLost(*this);
}

void nofChurchVisitor::abrogateWorkplace()
{
    if(church_)
    {
        church_->visitorLost(*this);
        church_ = nullptr;
    }
    if(origin_)
    {
        notifyOriginLost();
        origin_ = nullptr;
    }
    goal_ = nullptr;
}

void nofChurchVisitor::churchDestroyed()
{
    church_ = nullptr;
    if(state_ == State::ReturningHome)
        return;

    if(state_ == State::AttendingService)
    {
        getEvMgr().removeEvent(current_ev);
        notifyOriginLost();
        origin_ = nullptr;
        goal_ = nullptr;
        state_ = State::ReturningHome;
        startWandering();
        wander();
        return;
    }

    // GoingToChurch: check if still queued inside origin's leave_house vs already on roads
    if(cur_rs == nullptr && !current_ev)
    {
        if(origin_ && nobBaseWarehouse::isStorehouseGOT(origin_->getGOT()))
        {
            auto* wh = static_cast<nobBaseWarehouse*>(origin_);
            wh->churchVisitorLost(*this);
            origin_ = nullptr;
            // Cancel right back into warehouse once popped by handleLeaveEvent()
            fs = FigureState::GoHome;
            goal_ = wh;
            state_ = State::ReturningHome;
            return;
        }
        notifyOriginLost();
        origin_ = nullptr;
        goal_ = nullptr;
        state_ = State::ReturningHome;
        startWandering();
        return;
    }

    notifyOriginLost();
    origin_ = nullptr;
    goal_ = nullptr;
    state_ = State::ReturningHome;
    goHome(nullptr);
}

void nofChurchVisitor::originDestroyed()
{
    origin_ = nullptr;
    if(state_ != State::ReturningHome)
        return;

    goal_ = nullptr;
    if(cur_rs != nullptr)
        goHome(nullptr);
    else if(!current_ev)
    {
        startWandering();
        wander();
    }
}

void nofChurchVisitor::startReturnOrGoHome()
{
    state_ = State::ReturningHome;
    auto* curRoadNode = world->getSpecObj<noRoadNode>(pos);

    if(origin_ && curRoadNode)
    {
        const RoadPathDirection dir = world->findHumanPathOnRoads(*curRoadNode, *origin_);
        if(dir != RoadPathDirection::None && dir != RoadPathDirection::Ship)
        {
            fs = FigureState::GotToGoal;
            goal_ = origin_;
            cur_rs = nullptr;
            rs_pos = 0;
            rs_dir = false;
            walkToGoal();
            return;
        }
    }

    notifyOriginLost();
    origin_ = nullptr;

    nobBaseWarehouse* wh = nullptr;
    if(curRoadNode)
        wh = world->getPlayer(player).findWarehouse(*curRoadNode, FW::AcceptsFigure(job_), true, false);
    if(wh)
    {
        cur_rs = nullptr;
        rs_pos = 0;
        rs_dir = false;
        goHome(wh);
        walkToGoal();
    } else
    {
        goal_ = nullptr;
        startWandering();
        wander();
    }
}

void nofChurchVisitor::goalReached()
{
    if(state_ == State::GoingToChurch)
    {
        if(!church_)
        {
            startReturnOrGoHome();
            return;
        }
        state_ = State::AttendingService;
        church_->visitorArrived(*this);
        return;
    }

    if(state_ == State::ReturningHome && origin_)
    {
        church_ = nullptr;
        noBaseBuilding* const dest = origin_;
        origin_ = nullptr;

        if(dest->getGOT() == GO_Type::NobMilitary)
        {
            auto* mil = static_cast<nobMilitary*>(dest);
            auto soldier = std::make_unique<nofPassiveSoldier>(pos, player, nullptr, mil, getSoldierRank(job_));
            soldier->setArmor(hasArmor_);
            soldier->setHealing(hasHealing_);
            soldier->setHitpoints(soldierHp_);
            mil->returnFromChurch(*this, std::move(soldier));
            getEvMgr().addToKillList(world->removeFigure(pos, *this));
        } else if(nobBaseWarehouse::isStorehouseGOT(dest->getGOT()))
        {
            auto* wh = static_cast<nobBaseWarehouse*>(dest);
            wh->returnFromChurch(*this);
            wh->addFigure(world->removeFigure(pos, *this));
        } else
        {
            startWandering();
            wander();
        }
    }
}

void nofChurchVisitor::departFromChurch()
{
    church_ = nullptr;
    startReturnOrGoHome();
}

void nofChurchVisitor::handleDerivedEvent(const unsigned /*id*/) {}

void nofChurchVisitor::walked() {}
