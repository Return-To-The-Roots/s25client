// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "JobFactory.h"
#include "buildings/noBuildingSite.h"
#include "buildings/nobBaseMilitary.h"
#include "buildings/nobBaseWarehouse.h"
#include "buildings/nobUsual.h"
#include "nodeObjs/noRoadNode.h"

//////////////////////////////////////////////////////////////////////////
// Job includes, maybe use register function in nof* classes?
//////////////////////////////////////////////////////////////////////////
#include "figures/nofArmorer.h"
#include "figures/nofBaker.h"
#include "figures/nofBrewer.h"
#include "figures/nofBuilder.h"
#include "figures/nofButcher.h"
#include "figures/nofCarpenter.h"
#include "figures/nofCarrier.h"
#include "figures/nofCatapultMan.h"
#include "figures/nofCharburner.h"
#include "figures/nofDonkeybreeder.h"
#include "figures/nofFarmer.h"
#include "figures/nofFisher.h"
#include "figures/nofForester.h"
#include "figures/nofGeologist.h"
#include "figures/nofHunter.h"
#include "figures/nofIronfounder.h"
#include "figures/nofMetalworker.h"
#include "figures/nofMiller.h"
#include "figures/nofMiner.h"
#include "figures/nofMinter.h"
#include "figures/nofPassiveSoldier.h"
#include "figures/nofPassiveWorker.h"
#include "figures/nofPigbreeder.h"
#include "figures/nofPlaner.h"
#include "figures/nofScout_Free.h"
#include "figures/nofScout_LookoutTower.h"
#include "figures/nofShipWright.h"
#include "figures/nofStonemason.h"
#include "figures/nofTempleServant.h"
#include "figures/nofVintner.h"
#include "figures/nofWellguy.h"
#include "figures/nofWinegrower.h"
#include "figures/nofWoodcutter.h"
#include "helpers/toString.h"
#include "nodeObjs/noFlag.h"
#include <stdexcept>

namespace {
nobUsual* staticCastAndAssertCorrectType(noRoadNode* const goal)
{
    RTTR_Assert(dynamic_cast<nobUsual*>(goal));
    return static_cast<nobUsual*>(goal);
}
} // namespace

std::unique_ptr<noFigure> JobFactory::CreateJob(const Job job_id, const MapPoint pt, const unsigned char player,
                                                noRoadNode* const goal)
{
    switch(job_id)
    {
        case Job::Builder:
            if(!goal)
                return std::make_unique<nofBuilder>(pt, player, nullptr);
            else if(goal->GetGOT() != GO_Type::Buildingsite)
                return std::make_unique<nofPassiveWorker>(Job::Builder, pt, player, goal);
            else
                return std::make_unique<nofBuilder>(pt, player, static_cast<noBuildingSite*>(goal));
        case Job::Planer:
            RTTR_Assert(dynamic_cast<noBuildingSite*>(goal));
            return std::make_unique<nofPlaner>(pt, player, static_cast<noBuildingSite*>(goal));
        case Job::Carpenter: return std::make_unique<nofCarpenter>(pt, player, staticCastAndAssertCorrectType(goal));
        case Job::Armorer: return std::make_unique<nofArmorer>(pt, player, staticCastAndAssertCorrectType(goal));
        case Job::Stonemason: return std::make_unique<nofStonemason>(pt, player, staticCastAndAssertCorrectType(goal));
        case Job::Brewer: return std::make_unique<nofBrewer>(pt, player, staticCastAndAssertCorrectType(goal));
        case Job::Minter: return std::make_unique<nofMinter>(pt, player, staticCastAndAssertCorrectType(goal));
        case Job::Butcher: return std::make_unique<nofButcher>(pt, player, staticCastAndAssertCorrectType(goal));
        case Job::IronFounder:
            return std::make_unique<nofIronfounder>(pt, player, staticCastAndAssertCorrectType(goal));
        case Job::Miller: return std::make_unique<nofMiller>(pt, player, staticCastAndAssertCorrectType(goal));
        case Job::Metalworker:
            return std::make_unique<nofMetalworker>(pt, player, staticCastAndAssertCorrectType(goal));
        case Job::Baker: return std::make_unique<nofBaker>(pt, player, staticCastAndAssertCorrectType(goal));
        case Job::Helper:
            if(goal && goal->GetGOT() == GO_Type::NobUsual)
            {
                auto* goalBld = staticCastAndAssertCorrectType(goal);
                if(goalBld->GetBuildingType() == BuildingType::Well)
                    return std::make_unique<nofWellguy>(pt, player, goalBld);
                else if(goalBld->GetBuildingType() == BuildingType::Catapult)
                    return std::make_unique<nofCatapultMan>(pt, player, goalBld);
            }
            throw std::runtime_error("Invalid goal type: " + (goal ? helpers::toString(goal->GetGOT()) : "NULL")
                                     + " for job " + helpers::toString(job_id));
        case Job::Geologist:
            RTTR_Assert(dynamic_cast<noFlag*>(goal));
            return std::make_unique<nofGeologist>(pt, player, static_cast<noFlag*>(goal));
        case Job::Scout:
            // Different scout for lookout towers and free scouts
            if(goal->GetGOT() == GO_Type::NobUsual)
            {
                return std::make_unique<nofScout_LookoutTower>(pt, player, staticCastAndAssertCorrectType(goal));
            } else if(goal->GetGOT() == GO_Type::Flag)
                return std::make_unique<nofScout_Free>(pt, player, goal);
            throw std::runtime_error("Invalid goal type: " + helpers::toString(goal->GetGOT()) + " for job "
                                     + helpers::toString(job_id));
        case Job::Miner: return std::make_unique<nofMiner>(pt, player, staticCastAndAssertCorrectType(goal));
        case Job::Farmer: return std::make_unique<nofFarmer>(pt, player, staticCastAndAssertCorrectType(goal));
        case Job::Forester: return std::make_unique<nofForester>(pt, player, staticCastAndAssertCorrectType(goal));
        case Job::Woodcutter: return std::make_unique<nofWoodcutter>(pt, player, staticCastAndAssertCorrectType(goal));
        case Job::PigBreeder: return std::make_unique<nofPigbreeder>(pt, player, staticCastAndAssertCorrectType(goal));
        case Job::DonkeyBreeder:
            return std::make_unique<nofDonkeybreeder>(pt, player, staticCastAndAssertCorrectType(goal));
        case Job::Hunter: return std::make_unique<nofHunter>(pt, player, staticCastAndAssertCorrectType(goal));
        case Job::Fisher: return std::make_unique<nofFisher>(pt, player, staticCastAndAssertCorrectType(goal));
        case Job::Private:
        case Job::PrivateFirstClass:
        case Job::Sergeant:
        case Job::Officer:
        case Job::General:
            // TODO: Is this ever called? If yes, then why is the home here set to nullptr?
            RTTR_Assert(dynamic_cast<nobBaseMilitary*>(goal));
            return std::make_unique<nofPassiveSoldier>(pt, player, static_cast<nobBaseMilitary*>(goal), nullptr,
                                                       getSoldierRank(job_id));
        case Job::PackDonkey: return std::make_unique<nofCarrier>(CarrierType::Donkey, pt, player, nullptr, goal);
        case Job::Shipwright: return std::make_unique<nofShipWright>(pt, player, staticCastAndAssertCorrectType(goal));
        case Job::CharBurner: return std::make_unique<nofCharburner>(pt, player, staticCastAndAssertCorrectType(goal));
        case Job::Winegrower: return std::make_unique<nofWinegrower>(pt, player, staticCastAndAssertCorrectType(goal));
        case Job::Vintner: return std::make_unique<nofVintner>(pt, player, staticCastAndAssertCorrectType(goal));
        case Job::TempleServant:
            RTTR_Assert(dynamic_cast<nobUsual*>(goal));
            return std::make_unique<nofTempleServant>(pt, player, staticCastAndAssertCorrectType(goal));
        case Job::BoatCarrier:
            throw std::logic_error("Cannot create a boat carrier job (try creating Job::Helper).");
            break;
    }
    throw std::logic_error("Invalid job type " + helpers::toString(job_id));
}
