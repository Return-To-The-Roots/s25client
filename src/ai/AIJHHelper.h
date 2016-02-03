// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

#include "defines.h"
#include "gameTypes/MapTypes.h"
#include "gameTypes/BuildingTypes.h"
#include "AIEvents.h"
#include "ai/Resource.h"

#include <vector>

class AIPlayerJH;
class AIConstruction;
class GameWorldBase;
class AIInterface;
struct PositionSearch;
namespace gc { class GameCommand; }

namespace AIJH
{
    const unsigned RES_TYPE_COUNT = 9;
    const unsigned RES_RADIUS[RES_TYPE_COUNT] =
    {
        8, // Wood
        8, // Stones
        2, // Gold
        2, // Ironore
        2, // Coal
        2, // Granite
        3, // Plantspace
        5, // Borderland
        5 // Fish
    };

    struct Node
    {
        BuildingQuality bq;
        Resource res;
        bool owned;
        bool reachable;
        char failed_penalty;  // when a node was marked reachable, but building failed, this field is >0
        bool border;
        bool farmed;
    };

    enum JobStatus
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
            friend class iwAIDebug;
        public:
            Job(AIPlayerJH& aijh);
            virtual ~Job() { }
            virtual void ExecuteJob() { return; }
            JobStatus GetStatus() { return status; }
			void SetStatus(JobStatus s) {status=s;}
        protected:
            AIPlayerJH& aijh;
            JobStatus status;
    };

    class JobWithTarget{
    public:
        JobWithTarget(): target(0xFFFF, 0xFFFF){}
        inline MapPoint GetTarget() const { return target; }	
        void SetTargetX(MapCoord x) {target.x=x;}
        void SetTargetY(MapCoord y) {target.y=y;}
        void SetTarget(MapPoint newTarget) {target = newTarget;}
    protected:
        MapPoint target;
    };

    class BuildJob : public Job, public JobWithTarget
    {
            friend class iwAIDebug;
        public:
            BuildJob(AIPlayerJH& aijh, BuildingType type, MapPoint around, SearchMode searchMode = SEARCHMODE_RADIUS)
                : Job(aijh), type(type), around(around), searchMode(searchMode)
            {
                RTTR_Assert(type != BLD_NOTHING);
            }

            ~BuildJob() { }
            virtual void ExecuteJob();
            inline BuildingType GetType() const { return type; }            
            inline MapPoint GetAround() const { return around; }

        private:
            BuildingType type;
            MapPoint around;
            SearchMode searchMode;
            std::vector<unsigned char> route;

            void TryToBuild();
            void BuildMainRoad();
            void TryToBuildSecondaryRoad();
    };

    class ConnectJob : public Job, public JobWithTarget
    {
            friend class iwAIDebug;
        public:
            ConnectJob(AIPlayerJH& aijh, MapPoint flagPos)
                : Job(aijh), flagPos(flagPos) { }
            ~ConnectJob() { }
            virtual void ExecuteJob();
			MapPoint getFlag() const {return flagPos;}
        private:
            MapPoint flagPos;
            std::vector<unsigned char> route;
    };

    class EventJob : public Job
    {
            friend class iwAIDebug;
        public:
            EventJob(AIPlayerJH& aijh, AIEvent::Base* ev) : Job(aijh), ev(ev) { }
            ~EventJob() { delete ev; }
            void ExecuteJob();
            inline AIEvent::Base* GetEvent() const { return ev; }
        private:
            AIEvent::Base* ev;
    };


    class SearchJob : public Job
    {
            friend class iwAIDebug;
        public:
            SearchJob(AIPlayerJH& aijh, PositionSearch* search) : Job(aijh), search(search) { }
            ~SearchJob();
            void ExecuteJob();

        private:
            PositionSearch* search;
    };

}


#endif //!AIJHHELPER_H_INCLUDED
