// $Id: nofBuildingWorker.cpp 9567 2015-01-03 19:34:57Z marcus $
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
#include "nofBuildingWorker.h"
#include "nobUsual.h"
#include "nobBaseWarehouse.h"
#include "Loader.h"
#include "noFlag.h"
#include "Ware.h"
#include "GameClient.h"
#include "GameClientPlayer.h"
#include "EventManager.h"
#include "nofWoodcutter.h"
#include "nofHunter.h"
#include "nofArmorer.h"
#include "SoundManager.h"
#include "SerializedGameData.h"
#include "AIEventManager.h"



///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

nofBuildingWorker::nofBuildingWorker(const Job job, const unsigned short x, const unsigned short y, const unsigned char player, nobUsual* workplace)
    : noFigure(job, x, y, player, workplace), state(STATE_FIGUREWORK), workplace(workplace), ware(GD_NOTHING), not_working(0), since_not_working(0xFFFFFFFF), was_sounding(false), OutOfRessourcesMsgSent(false)
{
}

void nofBuildingWorker::Serialize_nofBuildingWorker(SerializedGameData* sgd) const
{
    Serialize_noFigure(sgd);

    sgd->PushUnsignedChar(static_cast<unsigned char>(state));

    if(fs != FS_GOHOME && fs != FS_WANDER)
    {
        sgd->PushObject(workplace, false);
        sgd->PushUnsignedChar(static_cast<unsigned char>(ware));
        sgd->PushUnsignedShort(not_working);
        sgd->PushUnsignedInt(since_not_working);
        sgd->PushBool(was_sounding);
    }
    sgd->PushBool(OutOfRessourcesMsgSent);
}

nofBuildingWorker::nofBuildingWorker(SerializedGameData* sgd, const unsigned obj_id) : noFigure(sgd, obj_id),
    state(State(sgd->PopUnsignedChar()))
{
    if(fs != FS_GOHOME && fs != FS_WANDER)
    {
        workplace = sgd->PopObject<nobUsual>(GOT_UNKNOWN);
        ware = GoodType(sgd->PopUnsignedChar());
        not_working = sgd->PopUnsignedShort();
        since_not_working = sgd->PopUnsignedInt();
        was_sounding = sgd->PopBool();
    }
    else
    {
        workplace = 0;
        ware = GD_NOTHING;
        not_working = 0;
        since_not_working = 0xFFFFFFFF;
        was_sounding = false;
    }
    OutOfRessourcesMsgSent = sgd->PopBool();
}


void nofBuildingWorker::AbrogateWorkplace()
{
    if(workplace)
        workplace->WorkerLost();
    workplace = 0;
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
                DrawWalking(x, y, LOADER.GetBobN("carrier"), GetCarryID() - 100, JOB_CONSTS[job].fat);
            else
                DrawWalking(x, y, LOADER.GetBobN("jobs"), GetCarryID(), JOB_CONSTS[job].fat);
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
				if(job!=JOB_ARMORER)
					TryToWork();
				else
					dynamic_cast<nofArmorer*>(this)->TryToWork();
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
            GoodType ware_type = (real_ware->type == GD_SHIELDVIKINGS || real_ware->type == GD_SHIELDAFRICANS ||
                                  real_ware->type == GD_SHIELDJAPANESE) ? GD_SHIELDROMANS : real_ware->type;
            gwg->GetPlayer(player)->IncreaseInventoryWare(ware_type, 1);
            // Abnehmer für Ware finden
            real_ware->goal = gwg->GetPlayer(player)->FindClientForWare(real_ware);
            // Ware soll ihren weiteren Weg berechnen
            real_ware->RecalcRoute();
            // Ware ablegen
            flag->AddWare(real_ware);
            real_ware->LieAtFlag(flag);
            // Warenstatistik erhöhen
            GameClient::inst().GetPlayer(this->player)->IncreaseMerchandiseStatistic(ware);
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
    // (bei Bergwerken müssen zusätzlich noch Rohstoffvorkommen vorhanden sein!)
    // (bei Brunnen muss ebenfalls auf Wasser geprüft werden!)
    // Spähturm-Erkunder arbeiten nie!
    // Charburner doesn't need wares for harvesting!
    // -> Wares are considered when calling GetPointQuality!
    else if( (workplace->WaresAvailable() || job == JOB_CHARBURNER) &&
             (job != JOB_MINER || GetResources(workplace->GetBuildingType() - BLD_GRANITEMINE)) &&
             (job != JOB_HELPER || GetResources(4)) &&
             job != JOB_SCOUT)
    {
        state = STATE_WAITING1;
        current_ev = em->AddEvent(this, (GetGOT() == GOT_NOF_CATAPULTMAN) ? CATAPULT_WAIT1_LENGTH : JOB_CONSTS[job].wait1_length, 1);
        StopNotWorking();

    }
    else
    {
        state = STATE_WAITINGFORWARES_OR_PRODUCTIONSTOPPED;
        // Nun arbeite ich nich mehr
        StartNotWorking();
    }
}



void nofBuildingWorker::GotWareOrProductionAllowed()
{
    // Falls man auf Waren wartet, kann man dann anfangen zu arbeiten
    if(state == STATE_WAITINGFORWARES_OR_PRODUCTIONSTOPPED)
        // anfangen zu arbeiten
		if(job!=JOB_ARMORER)
			TryToWork();
		else
			dynamic_cast<nofArmorer*>(this)->TryToWork();
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
            if(state != STATE_WAITINGFORWARES_OR_PRODUCTIONSTOPPED && state != STATE_WAITFORWARESPACE)
            {
                em->RemoveEvent(current_ev);
                current_ev = 0;
            }

            // Bescheid sagen, dass Arbeit abgebrochen wurde
            WorkAborted();

            // Rumirren
            StartWandering();
            Wander();


            // Evtl. Sounds löschen
            SoundManager::inst().WorkingFinished(this);

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
            SoundManager::inst().WorkingFinished(this);

            state = STATE_FIGUREWORK;
        } break;

    }

    workplace = 0;
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
    if (type == 0 && GameClient::inst().GetGGS().isEnabled(ADDON_INEXHAUSTIBLE_GRANITEMINES))
        return true;
    // in Map-Resource-Koordinaten konvertieren
    type = RESOURCES_MINE_TO_MAP[type];

    MapCoord mx = 0, my = 0;
    bool found = false;

    // Alle Punkte durchgehen, bis man einen findet, wo man graben kann
    if(GetResourcesOfNode(x, y, type))
    {
        mx = x;
        my = y;
        found = true;
    }

    for(MapCoord tx = gwg->GetXA(x, y, 0), r = 1; !found && r <= MINER_RADIUS; tx = gwg->GetXA(tx, y, 0), ++r)
    {
        MapCoord tx2 = tx, ty2 = y;
        for(unsigned int i = 2; !found && i < 8; ++i)
        {
            for(MapCoord r2 = 0; !found && r2 < r; gwg->GetPointA(tx2, ty2, i % 6), ++r2)
            {
                if(GetResourcesOfNode(tx2, ty2, type))
                {
                    mx = tx2;
                    my = ty2;
                    found = true;
                }
            }
        }
    }

    if(found)
    {
        // Minen / Brunnen unerschöpflich?
        if( (type == 4 && GameClient::inst().GetGGS().isEnabled(ADDON_EXHAUSTIBLE_WELLS)) ||
                (type != 4 && !GameClient::inst().GetGGS().isEnabled(ADDON_INEXHAUSTIBLE_MINES)) )
            --gwg->GetNode(mx, my).resources;
        return true;
    }

    // Hoffe das passt auch, Post verschicken, keine Rohstoffe mehr da
    if (!OutOfRessourcesMsgSent)
    {
        if(GameClient::inst().GetPlayerID() == this->player)
        {
            std::string error = _("This mine is exhausted");
            if(workplace->GetBuildingType() == BLD_WELL)
                error = _("This well is dried out");

            GameClient::inst().SendPostMessage(
                new ImagePostMsgWithLocation(_(error), PMC_GENERAL, x, y,
                                             workplace->GetBuildingType(), workplace->GetNation())
            );
        }

        OutOfRessourcesMsgSent = true;
        // Produktivitätsanzeige auf 0 setzen
        workplace->SetProductivityToZero();

        // KI-Event erzeugen
        GAMECLIENT.SendAIEvent(
            new AIEvent::Building(AIEvent::NoMoreResourcesReachable, workplace->GetX(), workplace->GetY(),
                                  workplace->GetBuildingType()), player
        );
    }

    // Hoffe das passt...
    return false;
}

bool nofBuildingWorker::GetResourcesOfNode(const unsigned short x, const unsigned short y, const unsigned char type)
{
    // Evtl über die Grenzen gelesen?
    if(x >= gwg->GetWidth() || y >= gwg->GetHeight())
        return false;

    unsigned char resources = gwg->GetNode(x, y).resources;

    // wasser?
    if(type == 4)
        return (resources > 0x20 && resources < 0x28);

    // Gibts Ressourcen von dem Typ an diesem Punkt?
    return (resources > 0x40 + type * 8 && resources < 0x48 + type * 8);
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
    if (OutOfRessourcesMsgSent)
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
void nofBuildingWorker::DrawOtherStates(const int x, const int y)
{
}


/// Zeichnet Figur beim Hereinlaufen/nach Hause laufen mit evtl. getragenen Waren
void nofBuildingWorker::DrawReturnStates(const int x, const int y)
{
    // Beim Nachhausegehen (Landarbeiter) und beim Reingehen kann entweder eine Ware getragen werden oder nicht
    if(ware != GD_NOTHING)
        DrawWalking(x, y, LOADER.GetBobN("jobs"), GetCarryID(), JOB_CONSTS[job].fat);
    else
        DrawWalking(x, y);
}

