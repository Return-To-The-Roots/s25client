// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.
#ifndef AIJHHELPER_H_INCLUDED
#define AIJHHELPER_H_INCLUDED

#pragma once

//#define DEBUG_AI

#include "gameTypes/BuildingType.h"
#include "gameTypes/Direction.h"
#include "gameTypes/MapCoordinates.h"
#include <vector>

namespace AIEvent {
class Base;
}

namespace AIJH {

class AIPlayerJH;
class PositionSearch;

enum JobState
{
    JOB_WAITING,
    JOB_EXECUTING_START,
    JOB_EXECUTING_ROAD1,
    JOB_EXECUTING_ROAD2,
    JOB_EXECUTING_ROAD2_2,
    JOB_FINISHED,
    JOB_FAILED
};

enum SearchMode
{
    SEARCHMODE_NONE,
    SEARCHMODE_RADIUS,
    SEARCHMODE_GLOBAL
};

class Job
{
public:
    Job(AIPlayerJH& aijh);
    virtual ~Job() {}
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

class BuildJob : public Job, public JobWithTarget
{
public:
    BuildJob(AIPlayerJH& aijh, BuildingType type, MapPoint around, SearchMode searchMode = SEARCHMODE_RADIUS)
        : Job(aijh), type(type), around(around), searchMode(searchMode)
    {
        RTTR_Assert(type != BLD_NOTHING);
    }

    ~BuildJob() override {}
    void ExecuteJob() override;
    inline BuildingType GetType() const { return type; }
    inline MapPoint GetAround() const { return around; }

private:
    BuildingType type;
    MapPoint around;
    SearchMode searchMode;
    std::vector<Direction> route;

    void TryToBuild();
    void BuildMainRoad();
    void TryToBuildSecondaryRoad();
};

class ConnectJob : public Job, public JobWithTarget
{
public:
    ConnectJob(AIPlayerJH& aijh, MapPoint flagPos) : Job(aijh), flagPos(flagPos) {}
    ~ConnectJob() override {}
    void ExecuteJob() override;
    MapPoint getFlag() const { return flagPos; }

private:
    MapPoint flagPos;
    std::vector<Direction> route;
};

class EventJob : public Job
{
public:
    EventJob(AIPlayerJH& aijh, AIEvent::Base* ev) : Job(aijh), ev(ev) {}
    ~EventJob() override;
    void ExecuteJob() override;
    AIEvent::Base* GetEvent() const { return ev; }

private:
    AIEvent::Base* ev;
};

class SearchJob : public Job
{
public:
    SearchJob(AIPlayerJH& aijh, PositionSearch* search) : Job(aijh), search(search) {}
    ~SearchJob() override;
    void ExecuteJob() override;

private:
    PositionSearch* search;
};

} // namespace AIJH

#endif //! AIJHHELPER_H_INCLUDED
