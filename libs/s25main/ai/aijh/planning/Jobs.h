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
    virtual void ExecuteJob() = 0;
    JobState GetState() const { return state; }
    void SetState(JobState s) { state = s; }

protected:
    AIPlayerJH& aijh;
    JobState state;
};

class JobWithTarget
{
public:
    JobWithTarget() : target(MapPoint::Invalid()) {}
    inline MapPoint GetTarget() const { return target; }
    void SetTarget(MapPoint newTarget) { target = newTarget; }

protected:
    MapPoint target;
};

class BuildJob : public AIJob, public JobWithTarget
{
public:
    BuildJob(AIPlayerJH& aijh, BuildingType type, MapPoint around, SearchMode searchMode = SearchMode::Radius)
        : AIJob(aijh),  priority(10000), type(type), around(around), searchMode(searchMode)
    {}

    void ExecuteJob() override;
    inline BuildingType GetType() const { return type; }
    inline MapPoint GetAround() const { return around; }
    unsigned priority;

private:
    BuildingType type;
    MapPoint around;
    SearchMode searchMode;
    std::vector<Direction> route;

    void TryToBuild();
    void BuildMainRoad();
    void TryToBuildSecondaryRoad();
};

struct CompareByPriority {
    bool operator()(const BuildJob& a, const BuildJob& b) const {
        // Ensure uniqueness in set: avoid two with same priority
        return a.priority > b.priority;
    }
};

class ConnectJob : public AIJob, public JobWithTarget
{
public:
    ConnectJob(AIPlayerJH& aijh, MapPoint flagPos) : AIJob(aijh), flagPos(flagPos) {}
    void ExecuteJob() override;
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
    void ExecuteJob() override;
    const AIEvent::Base& GetEvent() const { return *ev; }

private:
    std::unique_ptr<AIEvent::Base> ev;
};

class SearchJob : public AIJob
{
public:
    SearchJob(AIPlayerJH& aijh, PositionSearch* search) : AIJob(aijh), search(search) {}
    ~SearchJob() override;
    void ExecuteJob() override;

private:
    PositionSearch* search;
};

} // namespace AIJH
