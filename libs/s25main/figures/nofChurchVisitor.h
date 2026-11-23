// Copyright (C) 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "figures/noFigure.h"

class SerializedGameData;
class noBaseBuilding;
class nobChurch;

/// Worker or soldier walking from a warehouse or military building to attend church service and return home
class nofChurchVisitor : public noFigure
{
public:
    enum class State : uint8_t
    {
        GoingToChurch,
        AttendingService,
        ReturningHome,
    };
    friend constexpr auto maxEnumValue(State) { return State::ReturningHome; }

private:
    State state_;
    nobChurch* church_;
    noBaseBuilding* origin_;
    uint8_t soldierHp_;

    void goalReached() override;
    void walked() override;
    void handleDerivedEvent(unsigned id) override;
    void abrogateWorkplace() override;

    void notifyOriginLost();
    void startReturnOrGoHome();

public:
    nofChurchVisitor(Job job, MapPoint pos, unsigned char player, nobChurch* church, noBaseBuilding* origin,
                     uint8_t soldierHp, bool hasArmor, bool hasHealing);
    nofChurchVisitor(SerializedGameData& sgd, unsigned obj_id);

    void destroy() override;
    void serialize(SerializedGameData& sgd) const override;

    GO_Type getGOT() const final { return GO_Type::NofChurchvisitor; }

    void draw(DrawPoint drawPt) override;

    bool isAttendingService() const { return state_ == State::AttendingService; }
    /// Grants permanent +50% HP church blessing buff to a soldier visitor
    void grantHealing();
    /// Called by the church when the 100-frame service ends and visitors depart together
    void departFromChurch();
    /// Called when the target church is destroyed (or gathering times out) while visitor is heading to or attending it
    void churchDestroyed();
    /// Called when the origin building is destroyed or captured while this visitor is out
    void originDestroyed();
};
