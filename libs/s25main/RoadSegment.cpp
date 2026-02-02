// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "RoadSegment.h"
#include "EventManager.h"
#include "GamePlayer.h"
#include "SerializedGameData.h"
#include "buildings/nobBaseWarehouse.h"
#include "figures/nofCarrier.h"
#include "random/Random.h"
#include "world/GameWorld.h"
#include "nodeObjs/noFlag.h"
#include "nodeObjs/noRoadNode.h"
#include "gameData/BuildingProperties.h"
#include "s25util/Log.h"
#include <utility>

RoadSegment::RoadSegment(const RoadType rt, noRoadNode* const f1, noRoadNode* const f2, std::vector<Direction> route)
    : rt(rt), f1(f1), f2(f2), route(std::move(route))
{
    carriers_[0] = carriers_[1] = nullptr;
}

RoadSegment::RoadSegment(SerializedGameData& sgd, const unsigned obj_id)
    : GameObject(sgd, obj_id), rt(sgd.Pop<RoadType>()), f1(sgd.PopObject<noRoadNode>()), f2(sgd.PopObject<noRoadNode>())
{
    if(sgd.GetGameDataVersion() < 7)
        route.resize(sgd.PopUnsignedShort());
    else
        helpers::popContainer(sgd, route);

    carriers_[0] = sgd.PopObject<nofCarrier>(GO_Type::NofCarrier);
    carriers_[1] = sgd.PopObject<nofCarrier>(GO_Type::NofCarrier);

    if(sgd.GetGameDataVersion() < 7)
        helpers::popContainer(sgd, route, true);

    // tell the noRoadNodes about our existance
    f1->SetRoute(route.front(), this);
    f2->SetRoute(route.back() + 3u, this);
}

bool RoadSegment::GetNodeID(const noRoadNode& rn) const
{
    RTTR_Assert(&rn == f1 || &rn == f2);
    // Return true if it is the 2nd node
    return (&rn == f2);
}

void RoadSegment::Destroy()
{
    // This can be the road segment from the flag to the building (always in this order!)
    // Those are not considered "roads" and therefore never registered
    RTTR_Assert(f1->GetGOT() == GO_Type::Flag);
    if(f2->GetGOT() == GO_Type::Flag)
        world->GetPlayer(f1->GetPlayer()).DeleteRoad(this);

    if(carriers_[0])
        carriers_[0]->LostWork();
    if(carriers_[1])
        carriers_[1]->LostWork();

    if(!route.empty())
    {
        // Tell all characters on this road they lost their jobs
        MapPoint pt = f1->GetPos();

        for(unsigned short i = 0; i < route.size() + 1; ++i)
        {
            // Figuren sammeln
            for(noBase& object : world->GetFigures(pt))
            {
                if(object.GetType() == NodalObjectType::Figure)
                {
                    auto& figure = static_cast<noFigure&>(object);
                    if(figure.GetCurrentRoad() == this)
                    {
                        figure.Abrogate();
                        figure.StartWandering();
                    }
                }
            }

            world->RoadNodeAvailable(pt);

            if(i != route.size())
            {
                pt = world->GetNeighbour(pt, route[i]);
            }
        }

        route.clear();
    }
}

void RoadSegment::Serialize(SerializedGameData& sgd) const
{
    sgd.PushEnum<uint8_t>(rt);
    sgd.PushObject(f1);
    sgd.PushObject(f2);
    helpers::pushContainer(sgd, route);
    sgd.PushObject(carriers_[0], true);
    sgd.PushObject(carriers_[1], true);
}

/**
 *  zerteilt die Straße in 2 Teile.
 */
void RoadSegment::SplitRoad(noFlag* splitflag)
{
    // Flag 1 _________ This flag _________ Flag 2
    //         |          broken road           |

    // Store the old route of the road so that we can tell everyone on it later
    std::vector<Direction> old_route(route);

    // find the place where the road is cut ( = length of the first section )
    unsigned length1, length2;
    MapPoint t = f1->GetPos();
    for(length1 = 0; length1 < route.size(); ++length1)
    {
        if(t == splitflag->GetPos())
            break;

        t = world->GetNeighbour(t, route[length1]);
    }

    length2 = this->route.size() - length1;

    std::vector<Direction> second_route(length2);
    for(unsigned i = 0; i < length2; ++i)
        second_route[i] = this->route[length1 + i];

    auto* second = new RoadSegment(rt, splitflag, f2, second_route);

    // donkey road? Then upgrade flag, since it will be between donkey roads
    if(rt == RoadType::Donkey)
        splitflag->Upgrade();

    // create 1st section from F1 to this F (1st section is this road!)
    route.resize(length1);
    // f1 = f1;
    f2 = splitflag;

    f1->SetRoute(route.front(), this);
    splitflag->SetRoute(route.back() + 3u, this);

    // 2nd section from this F to F2

    splitflag->SetRoute(second->route.front(), second);
    second->f2->SetRoute(second->route.back() + 3u, second);

    // Notify all characters on the road
    t = f1->GetPos();

    for(unsigned short i = 0; i < old_route.size() + 1; ++i)
    {
        for(noBase& object : world->GetFigures(t))
        {
            if(object.GetType() == NodalObjectType::Figure)
            {
                auto& figure = static_cast<noFigure&>(object);
                if(figure.GetCurrentRoad() == this)
                    figure.CorrectSplitData(second);
            }
        }

        if(i != old_route.size())
        {
            t = world->GetNeighbour(t, old_route[i]);
        }
    }

    world->GetPlayer(f1->GetPlayer()).AddRoad(second);

    for(unsigned char i = 0; i < 2; ++i)
    {
        if(carriers_[i])
            carriers_[i]->RoadSplitted(this, second);
        else if(i == 0)
            // If road was unoccupied before then add 2nd part to the unoccupied roads
            // (1st is already included)
            world->GetPlayer(f1->GetPlayer()).FindCarrierForRoad(*second);
    }
}

/**
 *  Überprüft ob es an den Flaggen noch Waren zu tragen gibt für den Träger.
 *  Nur bei Straßen mit 2 Flagge aufrufen, nicht bei Hauseingängen etc. !!
 */
bool RoadSegment::AreWareJobs(const bool flag, CarrierType ct, const bool take_ware_immediately) const
{
    unsigned jobs_count;

    // Anzahl der Waren, die getragen werden wollen, ermitteln
    if(flag)
        jobs_count = static_cast<noFlag*>(f2)->GetNumWaresForRoad((route.back() + 3u));
    else
        jobs_count = static_cast<noFlag*>(f1)->GetNumWaresForRoad(route.front());

    // Nur eine Ware da --> evtl läuft schon ein anderer Träger/Esel hin, nur wo Esel und Träger da sind
    // Wenn der Träger nun natürlich schon da ist, kann er die mitnehmen
    if(jobs_count == 1 && carriers_[0] && carriers_[1] && !take_ware_immediately)
    {
        // anderen Esel ermitteln
        unsigned otherCarrier = (ct == CarrierType::Donkey) ? 0 : 1;

        switch(carriers_[otherCarrier]->GetCarrierState())
        {
            default: break;
            case CarrierState::FetchWare:
            case CarrierState::CarryWare:
            case CarrierState::WaitForWareSpace:
            case CarrierState::GoBackFromFlag:
            {
                // Läuft der in die Richtung, holt eine Ware bzw. ist schon fast da, braucht der hier nicht hinlaufen
                if(carriers_[otherCarrier]->GetRoadDir() == !flag)
                    return false;
            }
            break;
            case CarrierState::CarryWareToBuilding:
            case CarrierState::LeaveBuilding:
            {
                // Wenn an die Flagge ein Gebäude angrenzt und der Träger da was reinträgt, kann der auch die Ware
                // gleich mitnehmen, der zweite muss hier also nicht kommen
                if((carriers_[otherCarrier]->GetCurrentRoad()->f1 == f1 && !flag)
                   || (carriers_[otherCarrier]->GetCurrentRoad()->f1 == f2 && flag))
                    return false;
            }
            break;
        }
    }

    return (jobs_count > 0);
}
/**
 *  Eine Ware sagt Bescheid, dass sie über dem Weg getragen werden will.
 *
 *  rn ist die Flagge, von der sie kommt
 */
void RoadSegment::AddWareJob(const noRoadNode* rn)
{
    // Wenn das eine Straße zu einer Gebäudetür ist, muss dem entsprechenden Gebäude Bescheid gesagt werden (momentan
    // nur Lagerhäuser!)
    if(route.size() == 1)
    {
        if(f2->GetType() == NodalObjectType::Building)
        {
            if(BuildingProperties::IsWareHouse(static_cast<noBuilding*>(f2)->GetBuildingType()))
                static_cast<nobBaseWarehouse*>(f2)->FetchWare();
            else
                LOG.write("RoadSegment::AddWareJob: WARNING: Ware in front of building at %i,%i (gf: %u)!\n")
                  % f2->GetPos().x % f2->GetPos().y % GetEvMgr().GetCurrentGF();
        } else
            LOG.write("RoadSegment::AddWareJob: WARNING: Ware in front of building site at %i,%i (gf: %u)!\n")
              % f2->GetPos().x % f2->GetPos().y % GetEvMgr().GetCurrentGF();
    }

    // Zufällig Esel oder Träger zuerst fragen, ob er Zeit hat
    unsigned char first = RANDOM_RAND(2);
    for(unsigned char i = 0; i < 2; ++i)
    {
        if(carriers_[(i + first) % 2])
        {
            if(carriers_[(i + first) % 2]->AddWareJob(rn))
                // Ja, hat Zeit, dann brauchen wir den anderen nicht zu fragen
                break;
        }
    }
}

/**
 *  Eine Ware will nicht mehr befördert werden.
 */
void RoadSegment::WareJobRemoved(const noFigure* const exception)
{
    // Allen Trägern Bescheid sagen
    for(unsigned char i = 0; i < 2; ++i)
    {
        if(carriers_[i] && carriers_[i] != exception)
            carriers_[i]->RemoveWareJob();
    }
}

/**
 *  Baut die Straße zu einer Eselstraße aus.
 */
void RoadSegment::UpgradeDonkeyRoad()
{
    // Nur normale Straße können Eselstraßen werden
    if(rt != RoadType::Normal)
        return;

    rt = RoadType::Donkey;

    // Eselstraßen setzen
    MapPoint pt = f1->GetPos();
    for(auto i : route)
    {
        world->SetPointRoad(pt, i, PointRoad::Donkey);
        pt = world->GetNeighbour(pt, i);
    }

    // Flaggen auf beiden Seiten upgraden
    RTTR_Assert(f1->GetGOT() == GO_Type::Flag);
    RTTR_Assert(f2->GetGOT() == GO_Type::Flag);

    static_cast<noFlag*>(f1)->Upgrade();
    static_cast<noFlag*>(f2)->Upgrade();

    // Esel rufen (falls es einen gibt)
    TryGetDonkey();
}

/**
 *  Soll versuchen einen Esel zu bekommen.
 */
void RoadSegment::TryGetDonkey()
{
    // Nur rufen, falls es eine Eselstraße ist, noch kein Esel da ist, aber schon ein Träger da ist
    if(NeedDonkey())
        carriers_[1] = world->GetPlayer(f1->GetPlayer()).OrderDonkey(*this);
}

/**
 *  Ein Träger muss kündigen, aus welchen Gründen auch immer.
 */
void RoadSegment::CarrierAbrogated(nofCarrier* carrier)
{
    // Gucken, ob Träger und Esel gekündigt hat
    if(carrier->GetCarrierType() == CarrierType::Normal || carrier->GetCarrierType() == CarrierType::Boat)
    {
        // Straße wieder unbesetzt, bzw. nur noch Esel
        this->carriers_[0] = nullptr;
        world->GetPlayer(f1->GetPlayer()).FindCarrierForRoad(*this);
    } else
    {
        // Kein Esel mehr da, versuchen, neuen zu bestellen
        this->carriers_[1] = world->GetPlayer(f1->GetPlayer()).OrderDonkey(*this);
    }
}
/**
 * Return flag at the other end of the road
 */
const noFlag& RoadSegment::GetOtherFlag(const noFlag& flag) const
{
    if(GetNodeID(flag))
        return dynamic_cast<noFlag&>(*f1);
    else
        return dynamic_cast<noFlag&>(*f2);
}
/**
 * Return last road direction to flag at the other end of the road
 */
Direction RoadSegment::GetOtherFlagDir(const noFlag& flag) const
{
    if(GetNodeID(flag))
        return (route.front() + 3u);
    else
        return route.back();
}
