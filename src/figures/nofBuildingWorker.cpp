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

///////////////////////////////////////////////////////////////////////////////
// Header


#include "defines.h" // IWYU pragma: keep
#include "nofBuildingWorker.h"
#include "ai/AIEvents.h"
#include "buildings/nobUsual.h"
#include "buildings/nobBaseWarehouse.h"
#include "Loader.h"
#include "nodeObjs/noFlag.h"
#include "Ware.h"
#include "GameClient.h"
#include "GameClientPlayer.h"
#include "EventManager.h"
#include "SoundManager.h"
#include "PostMsg.h"
#include "SerializedGameData.h"
#include "gameData/GameConsts.h"
#include "gameData/ShieldConsts.h"
#include "gameData/JobConsts.h"

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

nofBuildingWorker::nofBuildingWorker(const Job job, const MapPoint pos, const unsigned char player, nobUsual* workplace)
    : noFigure(job, pos, player, workplace), state(STATE_FIGUREWORK), workplace(workplace), ware(GD_NOTHING), not_working(0), since_not_working(0xFFFFFFFF), was_sounding(false), outOfRessourcesMsgSent(false)
{
    RTTR_Assert(dynamic_cast<nobUsual*>(static_cast<GameObject*>(workplace))); // Assume we have at least a GameObject and check if it is a valid workplace
}

nofBuildingWorker::nofBuildingWorker(const Job job, const MapPoint pos, const unsigned char player, nobBaseWarehouse* goalWh)
    : noFigure(job, pos, player, goalWh), state(STATE_FIGUREWORK), workplace(NULL), ware(GD_NOTHING), not_working(0), since_not_working(0xFFFFFFFF), was_sounding(false), outOfRessourcesMsgSent(false)
{}

void nofBuildingWorker::Serialize_nofBuildingWorker(SerializedGameData& sgd) const
{
    Serialize_noFigure(sgd);

    sgd.PushUnsignedChar(static_cast<unsigned char>(state));

    if(fs != FS_GOHOME && fs != FS_WANDER)
    {
        sgd.PushObject(workplace, false);
        sgd.PushUnsignedChar(static_cast<unsigned char>(ware));
        sgd.PushUnsignedShort(not_working);
        sgd.PushUnsignedInt(since_not_working);
        sgd.PushBool(was_sounding);
    }
    sgd.PushBool(outOfRessourcesMsgSent);
}

nofBuildingWorker::nofBuildingWorker(SerializedGameData& sgd, const unsigned obj_id) : noFigure(sgd, obj_id),
    state(State(sgd.PopUnsignedChar()))
{
    if(fs != FS_GOHOME && fs != FS_WANDER)
    {
        workplace = sgd.PopObject<nobUsual>(GOT_UNKNOWN);
        ware = GoodType(sgd.PopUnsignedChar());
        not_working = sgd.PopUnsignedShort();
        since_not_working = sgd.PopUnsignedInt();
        was_sounding = sgd.PopBool();
    }
    else
    {
        workplace = 0;
        ware = GD_NOTHING;
        not_working = 0;
        since_not_working = 0xFFFFFFFF;
        was_sounding = false;
    }
    outOfRessourcesMsgSent = sgd.PopBool();
}


void nofBuildingWorker::AbrogateWorkplace()
{
    if(workplace)
    {
        workplace->WorkerLost();
        workplace = NULL;
    }
}

void nofBuildingWorker::Draw(int x, int y)
{
    switch(state)
    {
        case STATE_FIGUREWORK:

        case STATE_HUNTER_CHASING:
        case STATE_HUNTER_WALKINGTOCADAVER:
        case STATE_HUNTER_FINDINGSHOOTINGPOINT:
        {
            DrawWalking(x, y);
        } break;
        case STATE_WORK:
        case STATE_HUNTER_SHOOTING:
        case STATE_HUNTER_EVISCERATING:
        case STATE_CATAPULT_TARGETBUILDING:
        case STATE_CATAPULT_BACKOFF:
            DrawWorking(x, y); break;
        case STATE_CARRYOUTWARE:
        {
            unsigned short id = GetCarryID();

            // Über 100 bedeutet aus der carrier.bob nehmen, ansonsten aus der jobs.bob!
            if(id >= 100)
                DrawWalking(x, y, LOADER.GetBobN("carrier"), GetCarryID() - 100, JOB_CONSTS[job_].fat);
            else
                DrawWalking(x, y, LOADER.GetBobN("jobs"), GetCarryID(), JOB_CONSTS[job_].fat);
        } break;
        case STATE_WALKINGHOME:
        case STATE_ENTERBUILDING:
        {
            DrawReturnStates(x, y);

        } break;
        default:
            DrawOtherStates(x, y);
            break;
    }
}


void nofBuildingWorker::Walked()
{
    switch(state)
    {
        case STATE_ENTERBUILDING:
        {
            // Hab ich noch ne Ware in der Hand?

            if(ware != GD_NOTHING)
            {
                // dann war draußen kein Platz --> ist jetzt evtl Platz?
                state = STATE_WAITFORWARESPACE;
                if(workplace->GetFlag()->GetWareCount() < 8)
                    FreePlaceAtFlag();
                // Ab jetzt warten, d.h. nicht mehr arbeiten --> schlecht für die Produktivität
                StartNotWorking();
            }
            else
            {
                // Anfangen zu Arbeiten
			    TryToWork();
            }

        } break;
        case STATE_CARRYOUTWARE:
        {
            // Alles weitere übernimmt nofBuildingWorker
            WorkingReady();
        } break;
        default:
            WalkedDerived();
    }
}

void nofBuildingWorker::WorkingReady()
{
    // wir arbeiten nicht mehr
    workplace->is_working = false;

    // Trage ich eine Ware?
    if(ware != GD_NOTHING)
    {
        noFlag* flag = workplace->GetFlag();
        // Ist noch Platz an der Fahne?
        if(flag->GetWareCount() < 8)
        {
            // Ware erzeugen
            Ware* real_ware = new Ware(ware, 0, flag);
            // Inventur entsprechend erhöhen, dabei Schilder unterscheiden!
            GoodType ware_type = ConvertShields(real_ware->type);
            gwg->GetPlayer(player).IncreaseInventoryWare(ware_type, 1);
            // Abnehmer für Ware finden
            real_ware->SetGoal(gwg->GetPlayer(player).FindClientForWare(real_ware));
            // Ware soll ihren weiteren Weg berechnen
            real_ware->RecalcRoute();
            // Ware ablegen
            flag->AddWare(real_ware);
            real_ware->WaitAtFlag(flag);
            // Warenstatistik erhöhen
            GAMECLIENT.GetPlayer(this->player).IncreaseMerchandiseStatistic(ware);
            // Tragen nun keine Ware mehr
            ware = GD_NOTHING;
        }
    }

    // Wieder reingehen
    StartWalking(1);
    state = STATE_ENTERBUILDING;
}

void nofBuildingWorker::TryToWork()
{
    // Wurde die Produktion eingestellt?
    if(workplace->IsProductionDisabled())
    {
        state = STATE_WAITINGFORWARES_OR_PRODUCTIONSTOPPED;
        // Nun arbeite ich nich mehr
        StartNotWorking();
    }
    // Falls man auf Waren wartet, kann man dann anfangen zu arbeiten
    else if(AreWaresAvailable())
    {
        if(ReadyForWork())
        {
            state = STATE_WAITING1;
            current_ev = em->AddEvent(this, (GetGOT() == GOT_NOF_CATAPULTMAN) ? CATAPULT_WAIT1_LENGTH : JOB_CONSTS[job_].wait1_length, 1);
            StopNotWorking();
        }else
        {
            state = STATE_WAITINGFORWARES_OR_PRODUCTIONSTOPPED;
        }
    }
    else
    {
        state = STATE_WAITINGFORWARES_OR_PRODUCTIONSTOPPED;
        // Nun arbeite ich nich mehr
        StartNotWorking();
    }
}

bool nofBuildingWorker::AreWaresAvailable()
{
    return workplace->WaresAvailable();
}

bool nofBuildingWorker::ReadyForWork()
{
    return true;
}

void nofBuildingWorker::GotWareOrProductionAllowed()
{
    // Falls man auf Waren wartet, kann man dann anfangen zu arbeiten
    if(state == STATE_WAITINGFORWARES_OR_PRODUCTIONSTOPPED)
    {
        // anfangen zu arbeiten
		TryToWork();
    }
}

void nofBuildingWorker::GoalReached()
{
    // Tür zumachen (ist immer bis zu Erstbesetzung offen)
    workplace->CloseDoor();
    // Gebäude Bescheid sagen, dass ich da bin
    workplace->WorkerArrived();

    WorkplaceReached();

    // ggf. anfangen zu arbeiten
    TryToWork();
}

bool nofBuildingWorker::FreePlaceAtFlag()
{
    // Hinaus gehen, um Ware abzulegen, falls wir auf einen freien Platz warten
    if(state == STATE_WAITFORWARESPACE)
    {
        StartWalking(4);
        state = STATE_CARRYOUTWARE;
        return true;
    }
    else
        return false;
}
void nofBuildingWorker::LostWork()
{
    switch(state)
    {
        default:
            break;
        case STATE_FIGUREWORK:
        {
            // Auf Wegen nach Hause gehen
            GoHome();
        } break;
        case STATE_WAITING1:
        case STATE_WAITING2:
        case STATE_WORK:
        case STATE_WAITINGFORWARES_OR_PRODUCTIONSTOPPED:
        case STATE_WAITFORWARESPACE:
        case STATE_HUNTER_SHOOTING:
        case STATE_HUNTER_EVISCERATING:
        case STATE_CATAPULT_TARGETBUILDING:
        case STATE_CATAPULT_BACKOFF:
        {
            // Bisheriges Event abmelden, da die Arbeit unterbrochen wird
            em->RemoveEvent(current_ev);

            // Bescheid sagen, dass Arbeit abgebrochen wurde
            WorkAborted();

            // Rumirren
            StartWandering();
            Wander();


            // Evtl. Sounds löschen
            SOUNDMANAGER.WorkingFinished(this);

            state = STATE_FIGUREWORK;

        } break;
        case STATE_ENTERBUILDING:
        case STATE_CARRYOUTWARE:
        case STATE_WALKTOWORKPOINT:
        case STATE_WALKINGHOME:
        case STATE_HUNTER_CHASING:
        case STATE_HUNTER_FINDINGSHOOTINGPOINT:
        case STATE_HUNTER_WALKINGTOCADAVER:
        {
            // Bescheid sagen, dass Arbeit abgebrochen wurde
            WorkAborted();

            // Rumirren
            // Bei diesen States läuft man schon, darf also nicht noch zusätzlich Wander aufrufen, da man dann ja im Laufen nochmal losläuft!
            StartWandering();

            // Evtl. Sounds löschen
            SOUNDMANAGER.WorkingFinished(this);

            state = STATE_FIGUREWORK;
        } break;

    }

    workplace = NULL;
}

namespace{
    struct NodeHasResource
    {
        const GameWorldGame& gwg;
        const unsigned char res;
        NodeHasResource(const GameWorldGame& gwg, const unsigned char res):gwg(gwg), res(res){}

        bool operator()(const MapPoint pt)
        {
            return gwg.IsResourcesOnNode(pt, res);
        }
    };
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  verbraucht einen Rohstoff einer Mine oder eines Brunnens
 *  an einer (umliegenden) Stelle.
 *
 *  @author OLiver
 */
bool nofBuildingWorker::GetResources(unsigned char type)
{
    //this makes granite mines work everywhere
    const GlobalGameSettings& settings = GAMECLIENT.GetGGS();
    if (type == 0 && settings.isEnabled(AddonId::INEXHAUSTIBLE_GRANITEMINES))
        return true;
    // in Map-Resource-Koordinaten konvertieren
    type = RESOURCES_MINE_TO_MAP[type];

    MapPoint mP(0, 0);
    bool found = false;

    // Alle Punkte durchgehen, bis man einen findet, wo man graben kann
    if(gwg->IsResourcesOnNode(pos, type))
    {
        mP = pos;
        found = true;
    }else
    {
        std::vector<MapPoint> pts = gwg->GetPointsInRadius<1>(pos, MINER_RADIUS, Identity<MapPoint>(), NodeHasResource(*gwg, type));
        if(!pts.empty())
        {
            mP = pts.front();
            found = true;
        }
    }

    if(found)
    {
        // Minen / Brunnen unerschöpflich?
        if( (type == 4 && settings.isEnabled(AddonId::EXHAUSTIBLE_WELLS)) || (type != 4 && !settings.isEnabled(AddonId::INEXHAUSTIBLE_MINES)) )
            gwg->ReduceResource(mP);
        return true;
    }

    // Post verschicken, keine Rohstoffe mehr da
    if (!outOfRessourcesMsgSent)
    {
        if(GAMECLIENT.GetPlayerID() == this->player)
        {
            std::string error;
            if(workplace->GetBuildingType() == BLD_WELL)
                error = _("This well has dried out");
            else
                error = _("This mine is exhausted");

            GAMECLIENT.SendPostMessage(new ImagePostMsgWithLocation(error, PMC_GENERAL, pos, workplace->GetBuildingType(), workplace->GetNation()));
        }

        outOfRessourcesMsgSent = true;
        // Produktivitätsanzeige auf 0 setzen
        workplace->SetProductivityToZero();

        // KI-Event erzeugen
        GAMECLIENT.SendAIEvent(new AIEvent::Building(AIEvent::NoMoreResourcesReachable, workplace->GetPos(), workplace->GetBuildingType()), player);
    }

    return false;
}

void nofBuildingWorker::StartNotWorking()
{
    // Wenn noch kein Zeitpunkt festgesetzt wurde, jetzt merken
    if(since_not_working == 0xFFFFFFFF)
        since_not_working = GAMECLIENT.GetGFNumber();
}

void nofBuildingWorker::StopNotWorking()
{
    // Falls wir vorher nicht gearbeitet haben, diese Zeit merken für die Produktivität
    if(since_not_working != 0xFFFFFFFF)
    {
        not_working += static_cast<unsigned short>(GAMECLIENT.GetGFNumber() - since_not_working);
        since_not_working = 0xFFFFFFFF;
    }
}

unsigned short nofBuildingWorker::CalcProductivity()
{
    if (outOfRessourcesMsgSent)
        return 0;
    // Gucken, ob bis jetzt gearbeitet wurde/wird oder nicht, je nachdem noch was dazuzählen
    if(since_not_working != 0xFFFFFFFF)
    {
        // Es wurde bis jetzt nicht mehr gearbeitet, das also noch dazuzählen
        not_working += static_cast<unsigned short>(GAMECLIENT.GetGFNumber() - since_not_working);
        // Zähler zurücksetzen
        since_not_working = GAMECLIENT.GetGFNumber();
    }

    // Produktivität ausrechnen
    unsigned short productivity = (400 - not_working) / 4;

    // Zähler zurücksetzen
    not_working = 0;

    return productivity;
}

void nofBuildingWorker::ProductionStopped()
{
    // Wenn ich gerade warte und schon ein Arbeitsevent angemeldet habe, muss das wieder abgemeldet werden
    if(state == STATE_WAITING1)
    {
        em->RemoveEvent(current_ev);
        current_ev = 0;
        state = STATE_WAITINGFORWARES_OR_PRODUCTIONSTOPPED;
        StartNotWorking();
    }
}

void nofBuildingWorker::WorkAborted()
{
}

void nofBuildingWorker::WorkplaceReached()
{
}

/// Zeichnen der Figur in sonstigen Arbeitslagen
void nofBuildingWorker::DrawOtherStates(const int  /*x*/, const int  /*y*/)
{
}


/// Zeichnet Figur beim Hereinlaufen/nach Hause laufen mit evtl. getragenen Waren
void nofBuildingWorker::DrawReturnStates(const int x, const int y)
{
    // Beim Nachhausegehen (Landarbeiter) und beim Reingehen kann entweder eine Ware getragen werden oder nicht
    if(ware != GD_NOTHING)
        DrawWalking(x, y, LOADER.GetBobN("jobs"), GetCarryID(), JOB_CONSTS[job_].fat);
    else
        DrawWalking(x, y);
}

