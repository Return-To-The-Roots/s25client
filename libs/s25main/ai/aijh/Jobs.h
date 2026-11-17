// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

//#define DEBUG_AI

#include "gameTypes/BuildingType.h"
#include "gameTypes/Direction.h"
#include "gameTypes/MapCoordinates.h"
#include <memory>
#include <vector>

namespace AIEvent {
class Base;
}

namespace AIJH {

class AIPlayerJH;
class PositionSearch;

enum class JobState
{
    Waiting,
    Start,
    ExecutingRoad1,
    ExecutingRoad2,
    ExecutingRoad2_2,
    Finished,
    Failed
};

enum class SearchMode
{
    None,
    Radius,
    Global
};

class AIJob
{
public:
    AIJob(AIPlayerJH& aijh);
    virtual ~AIJob() = default;
    virtual void executeJob() = 0;
    JobState getState() const { return state; }
    void setState(JobState s) { state = s; }

protected:
    AIPlayerJH& aijh;
    JobState state;
};

class JobWithTarget
{
public:
    JobWithTarget() : target(MapPoint::invalid()) {}
    MapPoint getTarget() const { return target; }
    void setTarget(MapPoint newTarget) { target = newTarget; }

protected:
    MapPoint target;
};

class BuildJob : public AIJob, public JobWithTarget
{
public:
    BuildJob(AIPlayerJH& aijh, BuildingType type, MapPoint around, SearchMode searchMode = SearchMode::Radius)
        : AIJob(aijh), type(type), around(around), searchMode(searchMode)
    {}

    void executeJob() override;
    BuildingType getType() const { return type; }
    MapPoint getAround() const { return around; }

private:
    BuildingType type;
    MapPoint around;
    SearchMode searchMode;
    std::vector<Direction> route;

    void tryToBuild();
    void buildMainRoad();
    void tryToBuildSecondaryRoad();
};

class ConnectJob : public AIJob, public JobWithTarget
{
public:
    ConnectJob(AIPlayerJH& aijh, MapPoint flagPos) : AIJob(aijh), flagPos(flagPos) {}
    void executeJob() override;
    MapPoint getFlag() const { return flagPos; }

private:
    MapPoint flagPos;
    std::vector<Direction> route;
};

class EventJob : public AIJob
{
public:
    EventJob(AIPlayerJH& aijh, std::unique_ptr<AIEvent::Base> ev);
    ~EventJob() override;
    void executeJob() override;
    const AIEvent::Base& getEvent() const { return *ev; }

private:
    std::unique_ptr<AIEvent::Base> ev;
};

class SearchJob : public AIJob
{
public:
    SearchJob(AIPlayerJH& aijh, PositionSearch* search) : AIJob(aijh), search(search) {}
    ~SearchJob() override;
    void executeJob() override;

private:
    PositionSearch* search;
};

} // namespace AIJH
