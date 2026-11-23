// Copyright (C) 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "ChurchLoader.h"
#include "buildings/nobUsual.h"
#include <cstdint>
#include <list>

class GameEvent;
class SerializedGameData;
class nofChurchVisitor;

/// Church ("Dome") building producing Blessings from Fish + Wine + Bread and hosting periodic services
class nobChurch : public nobUsual
{
public:
    enum class ServiceState : uint8_t
    {
        Idle,
        Gathering,
        InService,
    };
    friend constexpr auto maxEnumValue(ServiceState) { return ServiceState::InService; }

private:
    static constexpr unsigned CHURCH_EV_SERVICE = 2;
    static constexpr unsigned CHURCH_EV_SERVICE_END = 3;
    static constexpr unsigned CHURCH_EV_GATHER_TIMEOUT = 4;

    uint8_t numBlessings_ = 0;
    ServiceState serviceState_ = ServiceState::Idle;
    const GameEvent* serviceEv_ = nullptr;
    const GameEvent* serviceEndEv_ = nullptr;
    std::list<nofChurchVisitor*> visitors_;

    friend class SerializedGameData;
    friend class BuildingFactory;
    nobChurch(MapPoint pos, unsigned char player, Nation nation);
    nobChurch(SerializedGameData& sgd, unsigned obj_id);

    void DestroyBuilding() override;
    bool allVisitorsInside() const;
    void startService();
    void beginServiceInside();
    void finishService();

public:
    ~nobChurch() override;

    void serialize(SerializedGameData& sgd) const override;
    GO_Type getGOT() const final { return GO_Type::NobChurch; }

    void draw(DrawPoint drawPt) override;
    void handleEvent(unsigned id) override;

    uint8_t getNumBlessings() const { return numBlessings_; }
    bool isBlessingStorageFull() const { return numBlessings_ >= churchaddon::MAX_CHURCH_BLESSINGS; }
    ServiceState getServiceState() const { return serviceState_; }
    bool isInService() const { return serviceState_ == ServiceState::InService; }
    bool isServiceActive() const { return serviceState_ != ServiceState::Idle; }
    unsigned getNumActiveVisitors() const { return visitors_.size(); }
    unsigned getNumSoldiersInside() const;
    unsigned getNumSoldiersEnRoute() const;

    void addBlessing();
    void visitorArrived(nofChurchVisitor& visitor);
    void visitorLost(nofChurchVisitor& visitor);
};
