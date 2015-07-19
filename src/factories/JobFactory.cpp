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

#include "JobFactory.h"

#include "noRoadNode.h"
#include "noBuildingSite.h"
#include "nobUsual.h"
#include "nobBaseMilitary.h"

//////////////////////////////////////////////////////////////////////////
// Job includes, maybe use register function in nof* classes?
//////////////////////////////////////////////////////////////////////////
#include "nofBuilder.h"
#include "nofPassiveWorker.h"
#include "nofPlaner.h"
#include "nofCarpenter.h"
#include "nofArmorer.h"
#include "nofStonemason.h"
#include "nofBrewer.h"
#include "nofButcher.h"
#include "nofMinter.h"
#include "nofButcher.h"
#include "nofIronfounder.h"
#include "nofMiller.h"
#include "nofMetalworker.h"
#include "nofBaker.h"
#include "nofWellguy.h"
#include "nofCatapultMan.h"
#include "nofGeologist.h"
#include "nofScout_LookoutTower.h"
#include "nofScout_Free.h"
#include "nofCatapultMan.h"
#include "nofMiner.h"
#include "nofFarmer.h"
#include "nofForester.h"
#include "nofWoodcutter.h"
#include "nofPigbreeder.h"
#include "nofDonkeybreeder.h"
#include "nofHunter.h"
#include "nofFisher.h"
#include "nofPassiveSoldier.h"
#include "nofCarrier.h"
#include "nofPassiveSoldier.h"
#include "nofShipWright.h"
#include "nofCharburner.h"

#include <stdexcept>
#include "helpers/converters.h"

noFigure* JobFactory::CreateJob(const Job job_id, const unsigned short x, const unsigned short y, const unsigned char player, noRoadNode* const goal){
    switch(job_id)
    {
    case JOB_BUILDER:
        if(!goal)
            return new nofBuilder(x, y, player, goal);
        else if(goal->GetGOT() == GOT_NOB_HARBORBUILDING)
            return new nofPassiveWorker(JOB_BUILDER, x, y, player, goal);
        else return new nofBuilder(x, y, player, goal);
    case JOB_PLANER:
        return new nofPlaner(x, y, player, static_cast<noBuildingSite*>(goal));
    case JOB_CARPENTER:
        return new nofCarpenter(x, y, player, static_cast<nobUsual*>(goal));
    case JOB_ARMORER:
        return new nofArmorer(x, y, player, static_cast<nobUsual*>(goal));
    case JOB_STONEMASON:
        return new nofStonemason(x, y, player, static_cast<nobUsual*>(goal));
    case JOB_BREWER:
        return new nofBrewer(x, y, player, static_cast<nobUsual*>(goal));
    case JOB_MINTER:
        return new nofMinter(x, y, player, static_cast<nobUsual*>(goal));
    case JOB_BUTCHER:
        return new nofButcher(x, y, player, static_cast<nobUsual*>(goal));
    case JOB_IRONFOUNDER:
        return new nofIronfounder(x, y, player, static_cast<nobUsual*>(goal));
    case JOB_MILLER:
        return new nofMiller(x, y, player, static_cast<nobUsual*>(goal));
    case JOB_METALWORKER:
        return new nofMetalworker(x, y, player, static_cast<nobUsual*>(goal));
    case JOB_BAKER:
        return new nofBaker(x, y, player, static_cast<nobUsual*>(goal));
    case JOB_HELPER:
        // Wenn goal = 0 oder Lagerhaus, dann Auslagern anscheinend und mann kann irgendeinen Typ nehmen
        if(!goal)
            return new nofWellguy(x, y, player, NULL);
        if(goal->GetGOT() == GOT_NOB_STOREHOUSE || goal->GetGOT() == GOT_NOB_HARBORBUILDING || goal->GetGOT() == GOT_NOB_HQ)
            return new nofWellguy(x, y, player, static_cast<nobUsual*>(goal));
        if(goal->GetGOT() == GOT_NOB_USUAL){
            nobUsual* goalBld = static_cast<nobUsual*>(goal);
            if(goalBld->GetBuildingType() == BLD_WELL)
                return new nofWellguy(x, y, player, goalBld);
            else if(goalBld->GetBuildingType() == BLD_CATAPULT)
                return new nofCatapultMan(x, y, player, goalBld);
        }
        throw std::runtime_error("Invalid goal type: " + helpers::toString(goal->GetGOT()) + " for job " + helpers::toString(job_id));
    case JOB_GEOLOGIST:
        return new nofGeologist(x, y, player, static_cast<noFlag*>(goal));
    case JOB_SCOUT:
        // Im Spähturm arbeitet ein anderer Spähter-Typ
        // Wenn goal = 0 oder Lagerhaus, dann Auslagern anscheinend und mann kann irgendeinen Typ nehmen
        if(!goal)
            return new nofScout_LookoutTower(x, y, player, NULL);
        if(goal->GetGOT() == GOT_NOB_HARBORBUILDING || goal->GetGOT() == GOT_NOB_STOREHOUSE || goal->GetGOT() == GOT_NOB_HQ)
            return new nofPassiveWorker(JOB_SCOUT, x, y, player, goal);
        // Spähturm / Lagerhaus?
        if(goal->GetGOT() == GOT_NOB_USUAL || goal->GetGOT() == GOT_NOB_HARBORBUILDING)
            return new nofScout_LookoutTower(x, y, player, static_cast<nobUsual*>(goal));
        if(goal->GetGOT() == GOT_FLAG)
            return new nofScout_Free(x, y, player, goal);
        throw std::runtime_error("Invalid goal type: " + helpers::toString(goal->GetGOT()) + " for job " + helpers::toString(job_id));
    case JOB_MINER:
        return new nofMiner(x, y, player, static_cast<nobUsual*>(goal));
    case JOB_FARMER:
        return new nofFarmer(x, y, player, static_cast<nobUsual*>(goal));
    case JOB_FORESTER:
        return new nofForester(x, y, player, static_cast<nobUsual*>(goal));
    case JOB_WOODCUTTER:
        return new nofWoodcutter(x, y, player, static_cast<nobUsual*>(goal));
    case JOB_PIGBREEDER:
        return new nofPigbreeder(x, y, player, static_cast<nobUsual*>(goal));
    case JOB_DONKEYBREEDER:
        return new nofDonkeybreeder(x, y, player, static_cast<nobUsual*>(goal) );
    case JOB_HUNTER:
        return new nofHunter(x, y, player, static_cast<nobUsual*>(goal));
    case JOB_FISHER:
        return new nofFisher(x, y, player, static_cast<nobUsual*>(goal));
    case JOB_PRIVATE:
    case JOB_PRIVATEFIRSTCLASS:
    case JOB_SERGEANT:
    case JOB_OFFICER:
    case JOB_GENERAL:
        return new nofPassiveSoldier(x, y, player, static_cast<nobBaseMilitary*>(goal), static_cast<nobBaseMilitary*>(goal), job_id - JOB_PRIVATE);
    case JOB_PACKDONKEY:
        return new nofCarrier(nofCarrier::CT_DONKEY, x, y, player, 0, goal);
    case JOB_SHIPWRIGHT:
        return new nofShipWright(x, y, player, static_cast<nobUsual*>(goal));
    case JOB_CHARBURNER:
        return new nofCharburner(x, y, player, static_cast<nobUsual*>(goal));
    }
    throw std::runtime_error("Invalid job type " + helpers::toString(job_id));
}