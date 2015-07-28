﻿// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

#include "JobFactory.h"

#include "nodeObjs/noRoadNode.h"
#include "buildings/noBuildingSite.h"
#include "buildings/nobUsual.h"
#include "buildings/nobBaseMilitary.h"

//////////////////////////////////////////////////////////////////////////
// Job includes, maybe use register function in nof* classes?
//////////////////////////////////////////////////////////////////////////
#include "figures/nofBuilder.h"
#include "figures/nofPassiveWorker.h"
#include "figures/nofPlaner.h"
#include "figures/nofCarpenter.h"
#include "figures/nofArmorer.h"
#include "figures/nofStonemason.h"
#include "figures/nofBrewer.h"
#include "figures/nofButcher.h"
#include "figures/nofMinter.h"
#include "figures/nofButcher.h"
#include "figures/nofIronfounder.h"
#include "figures/nofMiller.h"
#include "figures/nofMetalworker.h"
#include "figures/nofBaker.h"
#include "figures/nofWellguy.h"
#include "figures/nofCatapultMan.h"
#include "figures/nofGeologist.h"
#include "figures/nofScout_LookoutTower.h"
#include "figures/nofScout_Free.h"
#include "figures/nofCatapultMan.h"
#include "figures/nofMiner.h"
#include "figures/nofFarmer.h"
#include "figures/nofForester.h"
#include "figures/nofWoodcutter.h"
#include "figures/nofPigbreeder.h"
#include "figures/nofDonkeybreeder.h"
#include "figures/nofHunter.h"
#include "figures/nofFisher.h"
#include "figures/nofPassiveSoldier.h"
#include "figures/nofCarrier.h"
#include "figures/nofPassiveSoldier.h"
#include "figures/nofShipWright.h"
#include "figures/nofCharburner.h"

#include <stdexcept>
#include "helpers/converters.h"

noFigure* JobFactory::CreateJob(const Job job_id, const MapPoint pt, const unsigned char player, noRoadNode* const goal){
    switch(job_id)
    {
    case JOB_BUILDER:
        if(!goal)
            return new nofBuilder(pt, player, goal);
        else if(goal->GetGOT() == GOT_NOB_HARBORBUILDING)
            return new nofPassiveWorker(JOB_BUILDER, pt, player, goal);
        else return new nofBuilder(pt, player, goal);
    case JOB_PLANER:
        return new nofPlaner(pt, player, static_cast<noBuildingSite*>(goal));
    case JOB_CARPENTER:
        return new nofCarpenter(pt, player, static_cast<nobUsual*>(goal));
    case JOB_ARMORER:
        return new nofArmorer(pt, player, static_cast<nobUsual*>(goal));
    case JOB_STONEMASON:
        return new nofStonemason(pt, player, static_cast<nobUsual*>(goal));
    case JOB_BREWER:
        return new nofBrewer(pt, player, static_cast<nobUsual*>(goal));
    case JOB_MINTER:
        return new nofMinter(pt, player, static_cast<nobUsual*>(goal));
    case JOB_BUTCHER:
        return new nofButcher(pt, player, static_cast<nobUsual*>(goal));
    case JOB_IRONFOUNDER:
        return new nofIronfounder(pt, player, static_cast<nobUsual*>(goal));
    case JOB_MILLER:
        return new nofMiller(pt, player, static_cast<nobUsual*>(goal));
    case JOB_METALWORKER:
        return new nofMetalworker(pt, player, static_cast<nobUsual*>(goal));
    case JOB_BAKER:
        return new nofBaker(pt, player, static_cast<nobUsual*>(goal));
    case JOB_HELPER:
        // Wenn goal = 0 oder Lagerhaus, dann Auslagern anscheinend und mann kann irgendeinen Typ nehmen
        if(!goal)
            return new nofWellguy(pt, player, NULL);
        if(goal->GetGOT() == GOT_NOB_STOREHOUSE || goal->GetGOT() == GOT_NOB_HARBORBUILDING || goal->GetGOT() == GOT_NOB_HQ)
            return new nofWellguy(pt, player, static_cast<nobUsual*>(goal));
        if(goal->GetGOT() == GOT_NOB_USUAL){
            nobUsual* goalBld = static_cast<nobUsual*>(goal);
            if(goalBld->GetBuildingType() == BLD_WELL)
                return new nofWellguy(pt, player, goalBld);
            else if(goalBld->GetBuildingType() == BLD_CATAPULT)
                return new nofCatapultMan(pt, player, goalBld);
        }
        throw std::runtime_error("Invalid goal type: " + helpers::toString(goal->GetGOT()) + " for job " + helpers::toString(job_id));
    case JOB_GEOLOGIST:
        return new nofGeologist(pt, player, static_cast<noFlag*>(goal));
    case JOB_SCOUT:
        // Im Spähturm arbeitet ein anderer Spähter-Typ
        // Wenn goal = 0 oder Lagerhaus, dann Auslagern anscheinend und mann kann irgendeinen Typ nehmen
        if(!goal)
            return new nofScout_LookoutTower(pt, player, NULL);
        if(goal->GetGOT() == GOT_NOB_HARBORBUILDING || goal->GetGOT() == GOT_NOB_STOREHOUSE || goal->GetGOT() == GOT_NOB_HQ)
            return new nofPassiveWorker(JOB_SCOUT, pt, player, goal);
        // Spähturm / Lagerhaus?
        if(goal->GetGOT() == GOT_NOB_USUAL || goal->GetGOT() == GOT_NOB_HARBORBUILDING)
            return new nofScout_LookoutTower(pt, player, static_cast<nobUsual*>(goal));
        if(goal->GetGOT() == GOT_FLAG)
            return new nofScout_Free(pt, player, goal);
        throw std::runtime_error("Invalid goal type: " + helpers::toString(goal->GetGOT()) + " for job " + helpers::toString(job_id));
    case JOB_MINER:
        return new nofMiner(pt, player, static_cast<nobUsual*>(goal));
    case JOB_FARMER:
        return new nofFarmer(pt, player, static_cast<nobUsual*>(goal));
    case JOB_FORESTER:
        return new nofForester(pt, player, static_cast<nobUsual*>(goal));
    case JOB_WOODCUTTER:
        return new nofWoodcutter(pt, player, static_cast<nobUsual*>(goal));
    case JOB_PIGBREEDER:
        return new nofPigbreeder(pt, player, static_cast<nobUsual*>(goal));
    case JOB_DONKEYBREEDER:
        return new nofDonkeybreeder(pt, player, static_cast<nobUsual*>(goal) );
    case JOB_HUNTER:
        return new nofHunter(pt, player, static_cast<nobUsual*>(goal));
    case JOB_FISHER:
        return new nofFisher(pt, player, static_cast<nobUsual*>(goal));
    case JOB_PRIVATE:
    case JOB_PRIVATEFIRSTCLASS:
    case JOB_SERGEANT:
    case JOB_OFFICER:
    case JOB_GENERAL:
        return new nofPassiveSoldier(pt, player, static_cast<nobBaseMilitary*>(goal), static_cast<nobBaseMilitary*>(goal), job_id - JOB_PRIVATE);
    case JOB_PACKDONKEY:
        return new nofCarrier(nofCarrier::CT_DONKEY, pt, player, 0, goal);
    case JOB_SHIPWRIGHT:
        return new nofShipWright(pt, player, static_cast<nobUsual*>(goal));
    case JOB_CHARBURNER:
        return new nofCharburner(pt, player, static_cast<nobUsual*>(goal));
    case JOB_NOTHING:
        throw std::runtime_error("Cannot create a nothing job");
    }
    throw std::runtime_error("Invalid job type " + helpers::toString(job_id));
}
