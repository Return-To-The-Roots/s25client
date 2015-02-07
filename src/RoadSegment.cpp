// $Id: RoadSegment.cpp 9599 2015-02-07 11:08:22Z marcus $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "main.h"
#include "RoadSegment.h"

#include "nofCarrier.h"
#include "noRoadNode.h"
#include "noFlag.h"
#include "nobBaseWarehouse.h"

#include "SerializedGameData.h"
#include "Random.h"
#include "GameWorld.h"
#include "GameClient.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
RoadSegment::RoadSegment(const RoadType rt,
                         noRoadNode* const f1,
                         noRoadNode* const f2,
                         const std::vector<unsigned char>& route)
    : rt(rt), f1(f1), f2(f2), route(route)
{
    carrier[0] = carrier[1] = NULL;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
RoadSegment::RoadSegment(SerializedGameData* sgd, const unsigned int obj_id)
    : GameObject(sgd, obj_id),
      rt(static_cast<RoadType>(sgd->PopUnsignedChar())),
      f1(sgd->PopObject<noRoadNode>(GOT_UNKNOWN)),
      f2(sgd->PopObject<noRoadNode>(GOT_UNKNOWN)),
      route(sgd->PopUnsignedShort())
{
    carrier[0] = sgd->PopObject<nofCarrier>(GOT_NOF_CARRIER);
    carrier[1] = sgd->PopObject<nofCarrier>(GOT_NOF_CARRIER);

    for(unsigned short i = 0; i < route.size(); ++i)
        route[i] = sgd->PopUnsignedChar();

    // tell the noRoadNodes about our existance
    f1->routes[route[0]] = this;
    f2->routes[(route[route.size() - 1] + 3) % 6] = this;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void RoadSegment::Destroy_RoadSegment()
{
    assert(f1);

    if(f1)
        gwg->GetPlayer(f1->GetPlayer())->DeleteRoad(this);

    if(carrier[0])
        carrier[0]->LostWork();
    if(carrier[1])
        carrier[1]->LostWork();

    if(route.size())
    {
        // Straße durchgehen und alle Figuren sagen, dass sie die Arbeit verloren haben
        unsigned short x = f1->GetX();
        unsigned short y = f1->GetY();

        for(unsigned short i = 0; i < route.size() + 1; ++i)
        {
            // Figuren sammeln, Achtung, einige können (... ? was?)
            list<noBase*> objects;
            gwg->GetDynamicObjectsFrom(x, y, objects);

            for(list<noBase*>::iterator it = objects.begin(); it.valid(); ++it)
            {
                if((*it)->GetType() == NOP_FIGURE)
                {
                    if(static_cast<noFigure*>(*it)->GetCurrentRoad() == this)
                    {
                        static_cast<noFigure*>(*it)->Abrogate();
                        static_cast<noFigure*>(*it)->StartWandering();
                    }
                }
            }

            gwg->RoadNodeAvailable(x, y);

            if(i != route.size())
            {
                int tx = x, ty = y;
                x = gwg->GetXA(tx, ty, route[i]);
                y = gwg->GetYA(tx, ty, route[i]);
            }
        }

        route.clear();
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void RoadSegment::Serialize_RoadSegment(SerializedGameData* sgd) const
{
    Serialize_GameObject(sgd);

    sgd->PushUnsignedChar(static_cast<unsigned char>(rt));
    sgd->PushObject(f1, false);
    sgd->PushObject(f2, false);
    sgd->PushUnsignedShort(route.size());
    sgd->PushObject(carrier[0], true);
    sgd->PushObject(carrier[1], true);

    for(unsigned short i = 0; i < route.size(); ++i)
        sgd->PushUnsignedChar(route[i]);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  zerteilt die Straße in 2 Teile.
 *
 *  @author OLiver
 */
void RoadSegment::SplitRoad(noFlag* splitflag)
{
    // Flagge 1 _________ Diese Flagge _________ Flagge 2
    //         |       unterbrochener Weg       |

    // Alten Straßenverlauf merken, damit wir später allen Leuten darau Bescheid sagen können
    std::vector<unsigned char> old_route(route);

    // Stelle herausfinden, an der der Weg zerschnitten wird ( = Länge des ersten Teilstücks )
    unsigned int length1, length2;
    unsigned short tx = f1->GetX(), ty = f1->GetY();
    for(length1 = 0; length1 < route.size(); ++length1)
    {
        if(tx == splitflag->GetX() && ty == splitflag->GetY())
            break;

        gwg->GetPointA(tx, ty, route[length1]);
    }

    length2 = this->route.size() - length1;

    std::vector<unsigned char> second_route(length2);
    for(unsigned int i = 0; i < length2; ++i)
        second_route[i] = this->route[length1 + i];

    RoadSegment* second = new RoadSegment(rt, splitflag, f2, second_route);

    // Eselstraße? Dann prächtige Flagge, da sie ja wieder zwischen Eselstraßen ist
    if(rt == RT_DONKEY)
        splitflag->Upgrade();

    // 1. Teilstück von F1 bis zu dieser F erstellen ( 1. Teilstück ist dieser Weg dann! )

    route.resize(length1);
    //f1 = f1;
    f2 = splitflag;

    f1->routes[route.front()] = this;
    splitflag->routes[(route.back() + 3) % 6] = this;

    // 2. Teilstück von dieser F bis zu F2

    splitflag->routes[second->route.front()] = second;
    second->f2->routes[(second->route.back() + 3) % 6] = second;

    // Straße durchgehen und allen Figuren Bescheid sagen
    tx = f1->GetX();
    ty = f1->GetY();

    for(unsigned short i = 0; i < old_route.size() + 1; ++i)
    {
        for(list<noBase*>::iterator it = gwg->GetFigures(tx, ty).begin(); it.valid(); ++it)
        {
            if(static_cast<noFigure*>(*it)->GetType() == NOP_FIGURE)
            {
                if(static_cast<noFigure*>(*it)->GetCurrentRoad() == this)
                    static_cast<noFigure*>(*it)->CorrectSplitData(second);
            }
        }

        if(i != old_route.size())
        {
            int tx2 = tx, ty2 = ty;
            tx = gwg->GetXA(tx2, ty2, old_route[i]);
            ty = gwg->GetYA(tx2, ty2, old_route[i]);
        }
    }

    gwg->GetPlayer(f1->GetPlayer())->AddRoad(second);

    for(unsigned char i = 0; i < 2; ++i)
    {
        if(carrier[i])
            carrier[i]->RoadSplitted(this, second);
        else if(i == 0)
            // Die Straße war vorher unbesetzt? Dann 2. Straßenteil zu den unoccupied rodes
            // (1. ist ja schon drin)
            gwg->GetPlayer(f1->GetPlayer())->FindCarrierForRoad(second);
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Überprüft ob es an den Flaggen noch Waren zu tragen gibt für den Träger.
 *  Nur bei Straßen mit 2 Flagge aufrufen, nicht bei Hauseingängen etc. !!
 *
 *  @author OLiver
 */
bool RoadSegment::AreWareJobs(const bool flag, unsigned ct, const bool take_ware_immediately) const
{
    unsigned int jobs_count;

    // Anzahl der Waren, die getragen werden wollen, ermitteln
    if(flag)
        jobs_count = static_cast<noFlag*>(f2)->GetWaresCountForRoad((route.back() + 3) % 6);
    else
        jobs_count = static_cast<noFlag*>(f1)->GetWaresCountForRoad(route.front());

    // Nur eine Ware da --> evtl läuft schon ein anderer Träger/Esel hin, nur wo Esel und Träger da sind
    // Wenn der Träger nun natürlich schon da ist, kann er die mitnehmen
    if(jobs_count == 1 && carrier[0] && carrier[1] && !take_ware_immediately)
    {
        // anderen Esel ermitteln
        ct = 1 - ct;

        switch(carrier[ct]->GetCarrierState())
        {
            default: break;
            case CARRS_FETCHWARE:
            case CARRS_CARRYWARE:
            case CARRS_WAITFORWARESPACE:
            case CARRS_GOBACKFROMFLAG:
            {
                // Läuft der in die Richtung, holt eine Ware bzw. ist schon fast da, braucht der hier nicht hinlaufen
                if(carrier[ct]->GetRoadDir() == !flag)
                    return false;
            } break;
            case CARRS_CARRYWARETOBUILDING:
            case CARRS_LEAVEBUILDING:
            {
                // Wenn an die Flagge ein Gebäude angrenzt und der Träger da was reinträgt, kann der auch die Ware
                // gleich mitnehmen, der zweite muss hier also nicht kommen
                if((carrier[ct]->GetCurrentRoad()->f1 == f1 && flag == false) || (carrier[ct]->GetCurrentRoad()->f1 == f2 && flag == true))
                    return false;
            } break;

        }
    }

    return (jobs_count > 0);
}
///////////////////////////////////////////////////////////////////////////////
/**
 *  Eine Ware sagt Bescheid, dass sie über dem Weg getragen werden will.
 *
 *  rn ist die Flagge, von der sie kommt
 *
 *  @author OLiver
 */
void RoadSegment::AddWareJob(const noRoadNode* rn)
{
    // Wenn das eine Straße zu einer Gebäudetür ist, muss dem entsprechenden Gebäude Bescheid gesagt werden (momentan nur Lagerhäuser!)
    if(route.size() == 1)
    {
        if(f2->GetType() == NOP_BUILDING)
        {
            if(static_cast<noBuilding*>(f2)->GetBuildingType() == BLD_HEADQUARTERS ||
                    static_cast<noBuilding*>(f2)->GetBuildingType() == BLD_STOREHOUSE ||
                    static_cast<noBuilding*>(f2)->GetBuildingType() == BLD_HARBORBUILDING)
                static_cast<nobBaseWarehouse*>(f2)->FetchWare();
            else
                LOG.lprintf("RoadSegment::AddWareJob: WARNING: Ware in front of building at %i,%i (gf: %u)!\n", f2->GetX(), f2->GetY(), GAMECLIENT.GetGFNumber());
        }
        else
			LOG.lprintf("RoadSegment::AddWareJob: WARNING: Ware in front of building site at %i,%i (gf: %u)!\n",f2->GetX(), f2->GetY(), GAMECLIENT.GetGFNumber());
    }

    // Zufällig Esel oder Träger zuerst fragen, ob er Zeit hat
    unsigned char first = Random::inst().Rand(__FILE__, __LINE__, this->obj_id, 2);
    for(unsigned char i = 0; i < 2; ++i)
    {
        if(carrier[(i + first) % 2])
        {
            if(carrier[(i + first) % 2]->AddWareJob(rn))
                // Ja, hat Zeit, dann brauchen wir den anderen nicht zu fragen
                break;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Eine Ware will nicht mehr befördert werden.
 *
 *  @author OLiver
 */
void RoadSegment::WareJobRemoved(const noFigure* const exception)
{
    // Allen Trägern Bescheid sagen
    for(unsigned char i = 0; i < 2; ++i)
    {
        if(carrier[i] && carrier[i] != exception)
            carrier[i]->RemoveWareJob();
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Baut die Straße zu einer Eselstraße aus.
 *
 *  @author OLiver
 */
void RoadSegment::UpgradeDonkeyRoad()
{
    // Nur normale Straße können Eselstraßen werden
    if(rt != RT_NORMAL)
        return;

    rt = RT_DONKEY;

    // Eselstraßen setzen
    int x = f1->GetX(), y = f1->GetY();
    for(unsigned short i = 0; i < route.size(); ++i)
    {
        gwg->SetPointRoad(x, y, route[i], RT_DONKEY + 1);
        int tx = x, ty = y;
        x = gwg->GetXA(tx, ty, route[i]);
        y = gwg->GetYA(tx, ty, route[i]);
    }

    // Flaggen auf beiden Seiten upgraden
    assert(f1->GetGOT() == GOT_FLAG);
    assert(f2->GetGOT() == GOT_FLAG);

    static_cast<noFlag*>(f1)->Upgrade();
    static_cast<noFlag*>(f2)->Upgrade();

    // Esel rufen (falls es einen gibt)
    TryGetDonkey();
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Soll versuchen einen Esel zu bekommen.
 *
 *  @author OLiver
 */
void RoadSegment::TryGetDonkey()
{
    // Nur rufen, falls es eine Eselstraße ist, noch kein Esel da ist, aber schon ein Träger da ist
    if(NeedDonkey())
        carrier[1] = gwg->GetPlayer(f1->GetPlayer())->OrderDonkey(this);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Ein Träger muss kündigen, aus welchen Gründen auch immer.
 *
 *  @author OLiver
 */
void RoadSegment::CarrierAbrogated(nofCarrier* carrier)
{
    // Gucken, ob Träger und Esel gekündigt hat
    if(carrier->GetCarrierType() == nofCarrier::CT_NORMAL ||
            carrier->GetCarrierType() == nofCarrier::CT_BOAT)
    {
        // Straße wieder unbesetzt, bzw. nur noch Esel
        this->carrier[0] = NULL;
        gwg->GetPlayer(f1->GetPlayer())->FindCarrierForRoad(this);
    }
    else
    {
        // Kein Esel mehr da, versuchen, neuen zu bestellen
        this->carrier[1] = gwg->GetPlayer(f1->GetPlayer())->OrderDonkey(this);
    }
}
////////////////////////////////////////////////////////////////////////////////////////
/**
 * Return flag at the other end of the road
 *
 * @author PoC
 */
noFlag* RoadSegment::GetOtherFlag(const noFlag* flag)
{
    //is it a valid flag?
    assert((flag->GetX() == f1->GetX() && flag->GetY() == f1->GetY()) || (flag->GetX() == f2->GetX() && flag->GetY() == f2->GetY()));
    if(flag->GetX() == f1->GetX() && flag->GetY() == f1->GetY())
        return gwg->GetSpecObj<noFlag>(f2->GetX(), f2->GetY());
    if(flag->GetX() == f2->GetX() && flag->GetY() == f2->GetY())
        return gwg->GetSpecObj<noFlag>(f1->GetX(), f1->GetY());
    //shouldnt get here or at least catch the assertion fail
    return NULL;
}
////////////////////////////////////////////////////////////////////////////////////////
/**
 * Return last road direction to flag at the other end of the road
 *
 * @author PoC
 */
unsigned char RoadSegment::GetOtherFlagDir(const noFlag* flag)
{
    //is it a valid flag?
    assert((flag->GetX() == f1->GetX() && flag->GetY() == f1->GetY()) || (flag->GetX() == f2->GetX() && flag->GetY() == f2->GetY()));
    if(flag->GetX() == f1->GetX() && flag->GetY() == f1->GetY())
        return route[route.size() - 1];
    if(flag->GetX() == f2->GetX() && flag->GetY() == f2->GetY())
        return (route[0] + 3) % 6;;
    //shouldnt get here or at least catch the assertion fail
    return 255;
}
