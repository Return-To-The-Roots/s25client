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

#include "rttrDefines.h" // IWYU pragma: keep
#include "RoadSegment.h"
#include "EventManager.h"
#include "GamePlayer.h"
#include "SerializedGameData.h"
#include "buildings/nobBaseWarehouse.h"
#include "figures/nofCarrier.h"
#include "random/Random.h"
#include "world/GameWorldGame.h"
#include "nodeObjs/noFlag.h"
#include "nodeObjs/noRoadNode.h"
#include "gameData/BuildingProperties.h"
#include "libutil/Log.h"
#include <stdexcept>

RoadSegment::RoadSegment(const RoadType rt, noRoadNode* const f1, noRoadNode* const f2, const std::vector<Direction>& route)
    : rt(rt), f1(f1), f2(f2), route(route)
{
    carriers_[0] = carriers_[1] = nullptr;
}

RoadSegment::RoadSegment(SerializedGameData& sgd, const unsigned obj_id)
    : GameObject(sgd, obj_id), rt(static_cast<RoadType>(sgd.PopUnsignedChar())), f1(sgd.PopObject<noRoadNode>(GOT_UNKNOWN)),
      f2(sgd.PopObject<noRoadNode>(GOT_UNKNOWN)), route(sgd.PopUnsignedShort())
{
    carriers_[0] = sgd.PopObject<nofCarrier>(GOT_NOF_CARRIER);
    carriers_[1] = sgd.PopObject<nofCarrier>(GOT_NOF_CARRIER);

    for(unsigned short i = 0; i < route.size(); ++i)
        route[i] = Direction(sgd.PopUnsignedChar());

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

void RoadSegment::Destroy_RoadSegment()
{
    // This can be the road segment from the flag to the building (always in this order!)
    // Those are not considered "roads" and therefore never registered
    RTTR_Assert(f1->GetGOT() == GOT_FLAG);
    if(f2->GetGOT() == GOT_FLAG)
        gwg->GetPlayer(f1->GetPlayer()).DeleteRoad(this);

    if(carriers_[0])
        carriers_[0]->LostWork();
    if(carriers_[1])
        carriers_[1]->LostWork();

    if(!route.empty())
    {
        // Straße durchgehen und alle Figuren sagen, dass sie die Arbeit verloren haben
        MapPoint pt = f1->GetPos();

        for(unsigned short i = 0; i < route.size() + 1; ++i)
        {
            // Figuren sammeln
            for(noBase* object : gwg->GetFigures(pt))
            {
                if(object->GetType() == NOP_FIGURE)
                {
                    if(static_cast<noFigure*>(object)->GetCurrentRoad() == this)
                    {
                        static_cast<noFigure*>(object)->Abrogate();
                        static_cast<noFigure*>(object)->StartWandering();
                    }
                }
            }

            gwg->RoadNodeAvailable(pt);

            if(i != route.size())
            {
                pt = gwg->GetNeighbour(pt, route[i]);
            }
        }

        route.clear();
    }
}

void RoadSegment::Serialize_RoadSegment(SerializedGameData& sgd) const
{
    Serialize_GameObject(sgd);

    sgd.PushUnsignedChar(static_cast<unsigned char>(rt));
    sgd.PushObject(f1, false);
    sgd.PushObject(f2, false);
    sgd.PushUnsignedShort(route.size());
    sgd.PushObject(carriers_[0], true);
    sgd.PushObject(carriers_[1], true);

    for(unsigned short i = 0; i < route.size(); ++i)
        sgd.PushUnsignedChar(route[i].toUInt());
}

/**
 *  zerteilt die Straße in 2 Teile.
 */
void RoadSegment::SplitRoad(noFlag* splitflag)
{
    // Flagge 1 _________ Diese Flagge _________ Flagge 2
    //         |       unterbrochener Weg       |

    // Alten Straßenverlauf merken, damit wir später allen Leuten darau Bescheid sagen können
    std::vector<Direction> old_route(route);

    // Stelle herausfinden, an der der Weg zerschnitten wird ( = Länge des ersten Teilstücks )
    unsigned length1, length2;
    MapPoint t = f1->GetPos();
    for(length1 = 0; length1 < route.size(); ++length1)
    {
        if(t == splitflag->GetPos())
            break;

        t = gwg->GetNeighbour(t, route[length1]);
    }

    length2 = this->route.size() - length1;

    std::vector<Direction> second_route(length2);
    for(unsigned i = 0; i < length2; ++i)
        second_route[i] = this->route[length1 + i];

    RoadSegment* second = new RoadSegment(rt, splitflag, f2, second_route);

    // Eselstraße? Dann prächtige Flagge, da sie ja wieder zwischen Eselstraßen ist
    if(rt == RT_DONKEY)
        splitflag->Upgrade();

    // 1. Teilstück von F1 bis zu dieser F erstellen ( 1. Teilstück ist dieser Weg dann! )

    route.resize(length1);
    // f1 = f1;
    f2 = splitflag;

    f1->SetRoute(route.front(), this);
    splitflag->SetRoute(route.back() + 3u, this);

    // 2. Teilstück von dieser F bis zu F2

    splitflag->SetRoute(second->route.front(), second);
    second->f2->SetRoute(second->route.back() + 3u, second);

    // Straße durchgehen und allen Figuren Bescheid sagen
    t = f1->GetPos();

    for(unsigned short i = 0; i < old_route.size() + 1; ++i)
    {
        const std::list<noBase*>& figures = gwg->GetFigures(t);
        for(std::list<noBase*>::const_iterator it = figures.begin(); it != figures.end(); ++it)
        {
            if((*it)->GetType() == NOP_FIGURE)
            {
                if(static_cast<noFigure*>(*it)->GetCurrentRoad() == this)
                    static_cast<noFigure*>(*it)->CorrectSplitData(second);
            }
        }

        if(i != old_route.size())
        {
            t = gwg->GetNeighbour(t, old_route[i]);
        }
    }

    gwg->GetPlayer(f1->GetPlayer()).AddRoad(second);

    for(unsigned char i = 0; i < 2; ++i)
    {
        if(carriers_[i])
            carriers_[i]->RoadSplitted(this, second);
        else if(i == 0)
            // Die Straße war vorher unbesetzt? Dann 2. Straßenteil zu den unoccupied rodes
            // (1. ist ja schon drin)
            gwg->GetPlayer(f1->GetPlayer()).FindCarrierForRoad(second);
    }
}

/**
 *  Überprüft ob es an den Flaggen noch Waren zu tragen gibt für den Träger.
 *  Nur bei Straßen mit 2 Flagge aufrufen, nicht bei Hauseingängen etc. !!
 */
bool RoadSegment::AreWareJobs(const bool flag, unsigned ct, const bool take_ware_immediately) const
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
        ct = 1 - ct;

        switch(carriers_[ct]->GetCarrierState())
        {
            default: break;
            case CARRS_FETCHWARE:
            case CARRS_CARRYWARE:
            case CARRS_WAITFORWARESPACE:
            case CARRS_GOBACKFROMFLAG:
            {
                // Läuft der in die Richtung, holt eine Ware bzw. ist schon fast da, braucht der hier nicht hinlaufen
                if(carriers_[ct]->GetRoadDir() == !flag)
                    return false;
            }
            break;
            case CARRS_CARRYWARETOBUILDING:
            case CARRS_LEAVEBUILDING:
            {
                // Wenn an die Flagge ein Gebäude angrenzt und der Träger da was reinträgt, kann der auch die Ware
                // gleich mitnehmen, der zweite muss hier also nicht kommen
                if((carriers_[ct]->GetCurrentRoad()->f1 == f1 && !flag) || (carriers_[ct]->GetCurrentRoad()->f1 == f2 && flag))
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
    // Wenn das eine Straße zu einer Gebäudetür ist, muss dem entsprechenden Gebäude Bescheid gesagt werden (momentan nur Lagerhäuser!)
    if(route.size() == 1)
    {
        if(f2->GetType() == NOP_BUILDING)
        {
            if(BuildingProperties::IsWareHouse(static_cast<noBuilding*>(f2)->GetBuildingType()))
                static_cast<nobBaseWarehouse*>(f2)->FetchWare();
            else
                LOG.write("RoadSegment::AddWareJob: WARNING: Ware in front of building at %i,%i (gf: %u)!\n") % f2->GetPos().x
                  % f2->GetPos().y % GetEvMgr().GetCurrentGF();
        } else
            LOG.write("RoadSegment::AddWareJob: WARNING: Ware in front of building site at %i,%i (gf: %u)!\n") % f2->GetPos().x
              % f2->GetPos().y % GetEvMgr().GetCurrentGF();
    }

    // Zufällig Esel oder Träger zuerst fragen, ob er Zeit hat
    unsigned char first = RANDOM.Rand(__FILE__, __LINE__, GetObjId(), 2);
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
    if(rt != RT_NORMAL)
        return;

    rt = RT_DONKEY;

    // Eselstraßen setzen
    MapPoint pt = f1->GetPos();
    for(unsigned short i = 0; i < route.size(); ++i)
    {
        gwg->SetPointRoad(pt, route[i], RT_DONKEY + 1);
        pt = gwg->GetNeighbour(pt, route[i]);
    }

    // Flaggen auf beiden Seiten upgraden
    RTTR_Assert(f1->GetGOT() == GOT_FLAG);
    RTTR_Assert(f2->GetGOT() == GOT_FLAG);

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
        carriers_[1] = gwg->GetPlayer(f1->GetPlayer()).OrderDonkey(this);
}

/**
 *  Ein Träger muss kündigen, aus welchen Gründen auch immer.
 */
void RoadSegment::CarrierAbrogated(nofCarrier* carrier)
{
    // Gucken, ob Träger und Esel gekündigt hat
    if(carrier->GetCarrierType() == nofCarrier::CT_NORMAL || carrier->GetCarrierType() == nofCarrier::CT_BOAT)
    {
        // Straße wieder unbesetzt, bzw. nur noch Esel
        this->carriers_[0] = nullptr;
        gwg->GetPlayer(f1->GetPlayer()).FindCarrierForRoad(this);
    } else
    {
        // Kein Esel mehr da, versuchen, neuen zu bestellen
        this->carriers_[1] = gwg->GetPlayer(f1->GetPlayer()).OrderDonkey(this);
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
