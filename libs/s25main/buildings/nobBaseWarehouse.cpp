// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#include "commonDefines.h"
#include "nobBaseWarehouse.h"
#include "BurnedWarehouse.h"
#include "EventManager.h"
#include "FindWhConditions.h"
#include "GamePlayer.h"
#include "GlobalGameSettings.h"
#include "SerializedGameData.h"
#include "Ware.h"
#include "factories/JobFactory.h"
#include "figures/nofAggressiveDefender.h"
#include "figures/nofCarrier.h"
#include "figures/nofDefender.h"
#include "figures/nofPassiveSoldier.h"
#include "figures/nofPassiveWorker.h"
#include "figures/nofTradeDonkey.h"
#include "figures/nofTradeLeader.h"
#include "figures/nofWarehouseWorker.h"
#include "helpers/containerUtils.h"
#include "network/GameClient.h"
#include "nobMilitary.h"
#include "random/Random.h"
#include "variant.h"
#include "world/GameWorldGame.h"
#include "nodeObjs/noFlag.h"
#include "gameData/JobConsts.h"
#include "gameData/SettingTypeConv.h"
#include "gameData/ShieldConsts.h"
#include "s25util/Log.h"
#include <algorithm>

/// Intervall für Ausleerung (in gf)
const unsigned empty_INTERVAL = 25;
/// Intervall für Einlieferung
const unsigned STORE_INTERVAL = 80;
/// Dauer für das Erstellen von Trägern
const unsigned PRODUCE_HELPERS_GF = 150;
const unsigned PRODUCE_HELPERS_RANDOM_GF = 20;
/// Dauer für das Rekrutierung von Soldaten
const unsigned RECRUITE_GF = 200;
const unsigned RECRUITE_RANDOM_GF = 200;
const unsigned LEAVE_INTERVAL = 20;
const unsigned LEAVE_INTERVAL_RAND = 10;

nobBaseWarehouse::nobBaseWarehouse(const BuildingType type, const MapPoint pos, const unsigned char player, const Nation nation)
    : nobBaseMilitary(type, pos, player, nation), fetch_double_protection(false), recruiting_event(nullptr), empty_event(nullptr),
      store_event(nullptr)
{
    producinghelpers_event =
      GetEvMgr().AddEvent(this, PRODUCE_HELPERS_GF + RANDOM.Rand(__FILE__, __LINE__, GetObjId(), PRODUCE_HELPERS_RANDOM_GF), 1);
    // Reserve nullen
    reserve_soldiers_available.fill(0);
    reserve_soldiers_claimed_visual.fill(0);
    reserve_soldiers_claimed_real.fill(0);
}

nobBaseWarehouse::~nobBaseWarehouse()
{
    // Waiting Wares löschen
    for(auto& waiting_ware : waiting_wares)
        delete waiting_ware;
}

void nobBaseWarehouse::DestroyBuilding()
{
    // Den Waren und Figuren Bescheid sagen, die zu uns auf den Weg sind, dass wir nun nicht mehr existieren
    for(auto& dependent_figure : dependent_figures)
        dependent_figure->GoHome();
    dependent_figures.clear();
    for(auto& dependent_ware : dependent_wares)
        WareNotNeeded(dependent_ware);
    dependent_wares.clear();

    // ggf. Events abmelden
    GetEvMgr().RemoveEvent(recruiting_event);
    GetEvMgr().RemoveEvent(producinghelpers_event);
    GetEvMgr().RemoveEvent(empty_event);
    GetEvMgr().RemoveEvent(store_event);

    // Waiting Wares löschen
    for(auto& waiting_ware : waiting_wares)
    {
        waiting_ware->WareLost(player);
        delete waiting_ware;
    }
    waiting_wares.clear();

    // restliche Warenbestände von der Inventur wieder abziehen
    for(unsigned i = 0; i < NUM_WARE_TYPES; ++i)
        gwg->GetPlayer(player).DecreaseInventoryWare(GoodType(i), inventory[GoodType(i)]);

    // move soldiers from reserve to inventory.
    for(unsigned rank = 0; rank < gwg->GetGGS().GetMaxMilitaryRank(); ++rank)
    {
        if(reserve_soldiers_available[rank] > 0)
            inventory.real.Add(SOLDIER_JOBS[rank], reserve_soldiers_available[rank]);
    }

    // Objekt, das die flüchtenden Leute nach und nach ausspuckt, erzeugen
    gwg->AddFigure(pos, new BurnedWarehouse(pos, player, inventory.real.people));

    nobBaseMilitary::DestroyBuilding();
}

void nobBaseWarehouse::Serialize_nobBaseWarehouse(SerializedGameData& sgd) const
{
    Serialize_nobBaseMilitary(sgd);

    sgd.PushObjectContainer(waiting_wares, true);
    sgd.PushBool(fetch_double_protection);
    sgd.PushObjectContainer(dependent_figures, false);
    sgd.PushObjectContainer(dependent_wares, true);
    sgd.PushEvent(producinghelpers_event);
    sgd.PushEvent(recruiting_event);
    sgd.PushEvent(empty_event);
    sgd.PushEvent(store_event);

    for(unsigned i = 0; i < 5; ++i)
    {
        // Nur das Reale, nicht das visuelle speichern, das wäre sinnlos!, beim Laden ist das visuelle = realem
        sgd.PushUnsignedInt(reserve_soldiers_available[i]);
        sgd.PushUnsignedInt(reserve_soldiers_claimed_real[i]);
    }

    for(unsigned i = 0; i < NUM_WARE_TYPES; ++i)
    {
        sgd.PushUnsignedInt(inventory.visual.goods[i]);
        sgd.PushUnsignedInt(inventory.real.goods[i]);
        sgd.PushUnsignedChar(inventorySettings.wares[i].ToUnsignedChar());
    }
    for(unsigned i = 0; i < NUM_JOB_TYPES; ++i)
    {
        sgd.PushUnsignedInt(inventory.visual.people[i]);
        sgd.PushUnsignedInt(inventory.real.people[i]);
        sgd.PushUnsignedChar(inventorySettings.figures[i].ToUnsignedChar());
    }
}

nobBaseWarehouse::nobBaseWarehouse(SerializedGameData& sgd, const unsigned obj_id) : nobBaseMilitary(sgd, obj_id)
{
    sgd.PopObjectContainer(waiting_wares, GOT_WARE);
    fetch_double_protection = sgd.PopBool();
    sgd.PopObjectContainer(dependent_figures, GOT_UNKNOWN);
    sgd.PopObjectContainer(dependent_wares, GOT_WARE);

    producinghelpers_event = sgd.PopEvent();
    recruiting_event = sgd.PopEvent();
    empty_event = sgd.PopEvent();
    store_event = sgd.PopEvent();

    for(unsigned i = 0; i < 5; ++i)
    {
        reserve_soldiers_available[i] = sgd.PopUnsignedInt();
        reserve_soldiers_claimed_visual[i] = reserve_soldiers_claimed_real[i] = sgd.PopUnsignedInt();
    }

    for(unsigned i = 0; i < NUM_WARE_TYPES; ++i)
    {
        inventory.visual.goods[i] = sgd.PopUnsignedInt();
        inventory.real.goods[i] = sgd.PopUnsignedInt();
        inventorySettings.wares[i] = inventorySettingsVisual.wares[i] = static_cast<InventorySetting>(sgd.PopUnsignedChar());
    }
    for(unsigned i = 0; i < NUM_JOB_TYPES; ++i)
    {
        inventory.visual.people[i] = sgd.PopUnsignedInt();
        inventory.real.people[i] = sgd.PopUnsignedInt();
        inventorySettings.figures[i] = inventorySettingsVisual.figures[i] = static_cast<InventorySetting>(sgd.PopUnsignedChar());
    }
}

void nobBaseWarehouse::Clear()
{
    // Add reserve soldiers back
    for(unsigned i = 0; i < reserve_soldiers_available.size(); i++)
        inventory.Add(SOLDIER_JOBS[i], reserve_soldiers_available[i]);
    reserve_soldiers_available.fill(0);

    GamePlayer& owner = gwg->GetPlayer(player);
    for(unsigned i = 0; i < NUM_WARE_TYPES; ++i)
        owner.DecreaseInventoryWare(GoodType(i), inventory[GoodType(i)]);

    for(unsigned i = 0; i < NUM_JOB_TYPES; ++i)
        owner.DecreaseInventoryJob(Job(i), inventory[Job(i)]);

    inventory.clear();

    for(auto& waiting_ware : waiting_wares)
    {
        waiting_ware->WareLost(player);
        waiting_ware->Destroy();
        delete waiting_ware;
    }

    waiting_wares.clear();
}

void nobBaseWarehouse::OrderCarrier(noRoadNode& goal, RoadSegment& workplace)
{
    RTTR_Assert(workplace.getCarrier(0) == nullptr);
    const bool isBoatRequired = workplace.GetRoadType() == RoadSegment::RT_BOAT;

    // We assume, that the caller already checked, if this is possible
    RTTR_Assert(inventory[JOB_HELPER]);
    if(isBoatRequired)
        RTTR_Assert(inventory[GD_BOAT]);

    auto* carrier = new nofCarrier(isBoatRequired ? nofCarrier::CT_BOAT : nofCarrier::CT_NORMAL, pos, player, &workplace, &goal);
    workplace.setCarrier(0, carrier);

    if(!UseFigureAtOnce(carrier, goal))
        AddLeavingFigure(carrier);

    inventory.real.Remove(JOB_HELPER);
    if(isBoatRequired)
        inventory.real.Remove(GD_BOAT);

    // Evtl. kein Gehilfe mehr, sodass das Rekrutieren gestoppt werden muss
    TryStopRecruiting();
}

bool nobBaseWarehouse::OrderJob(const Job job, noRoadNode* const goal, const bool allow_recruiting)
{
    RTTR_Assert(goal);
    // Maybe we have to recruit one
    if(!inventory[job])
    {
        if(!allow_recruiting || !TryRecruitJob(job))
            return false;
    }

    noFigure* fig = JobFactory::CreateJob(job, pos, player, goal);
    // Wenn Figur nicht sofort von abgeleiteter Klasse verwenet wird, fügen wir die zur Leave-Liste hinzu
    if(!UseFigureAtOnce(fig, *goal))
        AddLeavingFigure(fig);

    // Ziel Bescheid sagen, dass dortin ein neuer Arbeiter kommt (bei Flaggen als das anders machen)
    if(goal->GetType() != NOP_FLAG)
    {
        RTTR_Assert(dynamic_cast<noBaseBuilding*>(goal));
        static_cast<noBaseBuilding*>(goal)->GotWorker(job, fig);
    }

    inventory.real.Remove(job);

    // Evtl. kein Gehilfe mehr da, sodass das Rekrutieren gestoppt werden muss
    TryStopRecruiting();

    return true;
}

nofCarrier* nobBaseWarehouse::OrderDonkey(RoadSegment* road, noRoadNode* const goal_flag)
{
    // Überhaupt ein Esel vorhanden?
    if(!inventory[JOB_PACKDONKEY])
        return nullptr;

    auto* donkey = new nofCarrier(nofCarrier::CT_DONKEY, pos, player, road, goal_flag);
    AddLeavingFigure(donkey);
    inventory.real.Remove(JOB_PACKDONKEY);

    return donkey;
}

void nobBaseWarehouse::HandleBaseEvent(const unsigned id)
{
    switch(id)
    {
        case 0:
            leaving_event = nullptr;
            HandleLeaveEvent();
            break;
        case 1:
            producinghelpers_event = nullptr;
            HandleProduceHelperEvent();
            break;
        case 2:
            recruiting_event = nullptr;
            HandleRecrutingEvent();
            break;
        case 3:
            empty_event = nullptr;
            HandleSendoutEvent();
            break;
        case 4:
            store_event = nullptr;
            HandleCollectEvent();
            break;
    }
}

void nobBaseWarehouse::HandleCollectEvent()
{
    // Storing wares done?
    bool storing_done = false;
    // Is storing still wanted?
    bool storing_wanted = false;

    // Untersuchen, welche Waren und Figuren eingelagert werden sollen
    for(unsigned i = 0; i < NUM_WARE_TYPES; ++i)
    {
        // Soll Ware eingeliefert werden?
        if(!GetInventorySetting(GoodType(i)).IsSet(EInventorySetting::COLLECT))
            continue;

        storing_wanted = true;

        // Lagerhaus suchen, das diese Ware enthält
        nobBaseWarehouse* wh = gwg->GetPlayer(player).FindWarehouse(*this, FW::HasWareButNoCollect(GoodType(i)), false, false);
        // Gefunden?
        if(wh)
        {
            // Dann bestellen
            Ware* ware = wh->OrderWare(GoodType(i), this);
            if(ware)
            {
                RTTR_Assert(IsWareDependent(ware));
                storing_done = true;
                break;
            }
        }
    }

    // Menschen "bestellen" wenn noch keine Ware bestellt wurde
    if(!storing_done)
    {
        for(unsigned i = 0; i < NUM_JOB_TYPES; ++i)
        {
            // Soll dieser Typ von Mensch bestellt werden?
            if(!GetInventorySetting(Job(i)).IsSet(EInventorySetting::COLLECT))
                continue;

            storing_wanted = true;

            // Lagerhaus suchen, das diesen Job enthält
            nobBaseWarehouse* wh = gwg->GetPlayer(player).FindWarehouse(*this, FW::HasFigureButNoCollect(Job(i), false), false, false);
            // Gefunden?
            if(wh)
            {
                // Dann bestellen
                if(wh->OrderJob(Job(i), this, false))
                    break;
            }
        }
    }

    // Storing still wanted?
    // Then continue ordering new stuff
    if(storing_wanted)
        store_event = GetEvMgr().AddEvent(this, STORE_INTERVAL, 4);
}

void nobBaseWarehouse::HandleSendoutEvent()
{
    // Fight or something in front of the house? Try again later!
    if(!gwg->IsRoadNodeForFigures(gwg->GetNeighbour(pos, Direction::SOUTHEAST)))
    {
        empty_event = GetEvMgr().AddEvent(this, empty_INTERVAL, 3);
        return;
    }

    std::vector<unsigned> possibleIds;
    // Waren und Figuren zum Auslagern zusammensuchen
    // Wenn keine Platz an Flagge, dann keine Waren raus
    if(GetFlag()->IsSpaceForWare())
    {
        for(unsigned i = 0; i < NUM_WARE_TYPES; ++i)
        {
            if(GetInventorySetting(GoodType(i)).IsSet(EInventorySetting::SEND) && inventory[GoodType(i)])
                possibleIds.push_back(i);
        }
    }

    for(unsigned i = 0; i < NUM_JOB_TYPES; ++i)
    {
        // Figuren, die noch nicht implementiert sind, nicht nehmen!
        if(GetInventorySetting(Job(i)).IsSet(EInventorySetting::SEND) && inventory[Job(i)])
            possibleIds.push_back(NUM_WARE_TYPES + i);
    }

    // Gibts überhaupt welche?
    if(possibleIds.empty())
        // ansonsten gleich tschüss
        return;

    // Eine ID zufällig auswählen
    unsigned selectedId = possibleIds[RANDOM.Rand(__FILE__, __LINE__, GetObjId(), possibleIds.size())];

    if(selectedId < NUM_WARE_TYPES)
    {
        // Ware
        auto* ware = new Ware(GoodType(selectedId), nullptr, this);
        noBaseBuilding* wareGoal = gwg->GetPlayer(player).FindClientForWare(ware);
        if(wareGoal != this)
        {
            ware->SetGoal(wareGoal);

            // Ware zur Liste hinzufügen, damit sie dann rausgetragen wird
            waiting_wares.push_back(ware);

            AddLeavingEvent();

            // Ware aus Inventar entfernen
            inventory.real.Remove(GoodType(selectedId));

            // Evtl. kein Schwert/Schild/Bier mehr da, sodass das Rekrutieren gestoppt werden muss
            TryStopRecruiting();
        } else
        {
            gwg->GetPlayer(player).RemoveWare(ware);
            deletePtr(ware);
        }
    } else
    {
        // Figur
        selectedId -= NUM_WARE_TYPES;

        nobBaseWarehouse* wh = gwg->GetPlayer(player).FindWarehouse(*this, FW::AcceptsFigureButNoSend(Job(selectedId)), true, false);
        if(wh != this)
        {
            auto* fig = new nofPassiveWorker(Job(selectedId), pos, player, nullptr);

            if(wh)
                fig->GoHome(wh);
            else
                fig->StartWandering();

            AddLeavingFigure(fig);

            // Person aus Inventar entfernen
            inventory.real.Remove(Job(selectedId));

            // Evtl. kein Gehilfe mehr da, sodass das Rekrutieren gestoppt werden muss
            TryStopRecruiting();
        }
    }

    // Weitere Waren/Figuren zum Auslagern?
    if(AreWaresToEmpty())
        // --> Nächstes Event
        empty_event = GetEvMgr().AddEvent(this, empty_INTERVAL, 3);
}

void nobBaseWarehouse::HandleRecrutingEvent()
{
    // Wir wollen so viele der Soldaten rekrutieren,
    // wie in den military_settings angegeben.
    // Wird evtl gerundet, dann fair nach Zufall ;) ).

    unsigned max_recruits;
    max_recruits = std::min(inventory[GD_SWORD], inventory[GD_SHIELDROMANS]);
    max_recruits = std::min(inventory[GD_BEER], max_recruits);
    max_recruits = std::min(inventory[JOB_HELPER], max_recruits);

    GamePlayer& owner = gwg->GetPlayer(player);
    const unsigned recruiting_ratio = owner.GetMilitarySetting(0);
    unsigned real_recruits = max_recruits * recruiting_ratio / MILITARY_SETTINGS_SCALE[0];
    // Wurde abgerundet?
    unsigned remainingRecruits = real_recruits * recruiting_ratio % MILITARY_SETTINGS_SCALE[0];
    if(remainingRecruits != 0 && unsigned(RANDOM.Rand(__FILE__, __LINE__, GetObjId(), MILITARY_SETTINGS_SCALE[0] - 1)) < remainingRecruits)
        ++real_recruits;
    else if(real_recruits == 0)
        return; // Nothing to do

    inventory.Add(JOB_PRIVATE, real_recruits);
    owner.IncreaseInventoryJob(JOB_PRIVATE, real_recruits);

    inventory.Remove(JOB_HELPER, real_recruits);
    owner.DecreaseInventoryJob(JOB_HELPER, real_recruits);

    inventory.Remove(GD_SWORD, real_recruits);
    owner.DecreaseInventoryWare(GD_SWORD, real_recruits);

    inventory.Remove(GD_SHIELDROMANS, real_recruits);
    owner.DecreaseInventoryWare(GD_SHIELDROMANS, real_recruits);

    inventory.Remove(GD_BEER, real_recruits);
    owner.DecreaseInventoryWare(GD_BEER, real_recruits);

    // Evtl. versuchen nächsten zu rekrutieren
    TryRecruiting();

    // If there were no soliders before
    if(inventory[JOB_PRIVATE] == real_recruits)
    {
        // Check reserve
        this->RefreshReserve(0);
        // And check if we need the new ones (if any left) e.g. for military buildings
        if(inventory[JOB_PRIVATE] > 0)
            owner.NewSoldiersAvailable(inventory[JOB_PRIVATE]);
    }
}

void nobBaseWarehouse::HandleProduceHelperEvent()
{
    // Nur bei unter 100 Trägern, weitere "produzieren"
    if(inventory[JOB_HELPER] < 100)
    {
        inventory.Add(JOB_HELPER);

        GamePlayer& owner = gwg->GetPlayer(player);
        owner.IncreaseInventoryJob(JOB_HELPER, 1);

        if(inventory[JOB_HELPER] == 1)
        {
            // Wenn vorher keine Träger da waren, müssen alle unbesetzen Wege gucken, ob sie nen Weg hierher finden, könnte ja sein, dass
            // vorher nich genug Träger da waren
            owner.FindCarrierForAllRoads();
            // evtl Träger mit Werkzeug kombiniert -> neuer Beruf
            owner.FindWarehouseForAllJobs();
        }
    } else if(inventory[JOB_HELPER] > 100)
    {
        // Bei Überbevölkerung Träger vernichten
        inventory.Remove(JOB_HELPER);

        gwg->GetPlayer(player).DecreaseInventoryJob(JOB_HELPER, 1);
    }

    producinghelpers_event =
      GetEvMgr().AddEvent(this, PRODUCE_HELPERS_GF + RANDOM.Rand(__FILE__, __LINE__, GetObjId(), PRODUCE_HELPERS_RANDOM_GF), 1);

    // Evtl. genau der Gehilfe, der zum Rekrutieren notwendig ist
    TryRecruiting();

    // Evtl die Typen gleich wieder auslagern, falls erforderlich
    CheckOuthousing(true, JOB_HELPER);
}

void nobBaseWarehouse::HandleLeaveEvent()
{
#if RTTR_ENABLE_ASSERTS
    // Harbors have more queues. Ignore for now
    if(GetGOT() != GOT_NOB_HARBORBUILDING)
    {
        Inventory should = inventory.real;
        for(auto& it : leave_house)
        {
            // Don't count warehouse workers
            if(!it->MemberOfWarehouse())
            {
                if(it->GetJobType() == JOB_BOATCARRIER)
                    should.Add(JOB_HELPER);
                else
                    should.Add(it->GetJobType());
            }
        }
        RTTR_Assert(should.people == inventory.visual.people);
    }
#endif

    // Falls eine Bestellung storniert wurde
    if(leave_house.empty() && waiting_wares.empty())
    {
        go_out = false;
        return;
    }

    // Fight or something in front of the house and we are not defending?
    if(!gwg->IsRoadNodeForFigures(gwg->GetNeighbour(pos, Direction::SOUTHEAST)))
    {
        // there's a fight

        // try to find a defender
        const auto it = std::find_if(leave_house.begin(), leave_house.end(), [](const auto* sld) {
            return sld->GetGOT() == GOT_NOF_AGGRESSIVEDEFENDER || sld->GetGOT() == GOT_NOF_DEFENDER;
        });
        // no defender found? trigger next leaving event :)
        if(it == leave_house.end())
        {
            go_out = false;
            AddLeavingEvent();
            return;
        }
        // and make him leave the house first
        // remove defender from list, insert him again in front of all others
        leave_house.push_front(*it);
        leave_house.erase(it);
    }

    // Figuren kommen zuerst raus
    if(!leave_house.empty())
    {
        noFigure* fig = leave_house.front();

        gwg->AddFigure(pos, fig);

        // Init road walking for figures walking on roads
        if(fig->IsWalkingOnRoad())
            fig->InitializeRoadWalking(routes[4], 0, true);

        fig->ActAtFirst();
        // Bei Lagerhausarbeitern das nicht abziehen!
        if(!fig->MemberOfWarehouse())
        {
            // War das ein Boot-Träger?
            if(fig->GetJobType() == JOB_BOATCARRIER)
            {
                // Remove helper and boat separately
                inventory.visual.Remove(JOB_HELPER);
                inventory.visual.Remove(GD_BOAT);
            } else
                inventory.visual.Remove(fig->GetJobType());

            if(fig->GetGOT() == GOT_NOF_TRADEDONKEY)
            {
                // Trade donkey carrying wares?
                GoodType carriedWare = static_cast<nofTradeDonkey*>(fig)->GetCarriedWare();
                if(carriedWare != GD_NOTHING)
                    inventory.visual.Remove(carriedWare);
            }
        }

        leave_house.pop_front();
    } else
    {
        // Ist noch Platz an der Flagge?
        if(GetFlag()->GetNumWares() < 8)
        {
            // Dann Ware raustragen lassen
            Ware* ware = waiting_wares.front();
            auto* worker = new nofWarehouseWorker(pos, player, ware, false);
            gwg->AddFigure(pos, worker);
            inventory.visual.Remove(ConvertShields(ware->type));
            worker->WalkToGoal();
            ware->Carry(GetFlag());
            waiting_wares.pop_front();
        } else
        {
            // Kein Platz mehr für Waren --> keiner brauch mehr rauszukommen, und Figuren gibts ja auch keine mehr
            go_out = false;
        }
    }

    // Wenn keine Figuren und Waren mehr da sind (bzw die Flagge vorm Haus voll ist), brauch auch keiner mehr rauszukommen
    if(leave_house.empty() && waiting_wares.empty())
        go_out = false;

    if(go_out)
        leaving_event = GetEvMgr().AddEvent(this, LEAVE_INTERVAL + RANDOM.Rand(__FILE__, __LINE__, GetObjId(), LEAVE_INTERVAL_RAND));
}

/// Abgeleitete kann eine gerade erzeugte Ware ggf. sofort verwenden
/// (muss in dem Fall true zurückgeben)
bool nobBaseWarehouse::UseWareAtOnce(Ware* /*ware*/, noBaseBuilding& /*goal*/)
{
    return false;
}

/// Dasselbe für Menschen
bool nobBaseWarehouse::UseFigureAtOnce(noFigure* /*fig*/, noRoadNode& /*goal*/)
{
    return false;
}

Ware* nobBaseWarehouse::OrderWare(const GoodType good, noBaseBuilding* const goal)
{
    RTTR_Assert(goal);
    // Ware überhaupt hier vorhanden (Abfrage eigentlich nicht nötig, aber erstmal zur Sicherheit)
    if(!inventory[good])
    {
        LOG.write("nobBaseWarehouse::OrderWare: WARNING: No ware type %u in warehouse!\n") % static_cast<unsigned>(good);
        return nullptr;
    }

    auto* ware = new Ware(good, goal, this);
    inventory.Remove(good);

    // Abgeleitete Klasse fragen, ob die irgend etwas besonderes mit dieser Ware anfangen will
    if(!UseWareAtOnce(ware, *goal))
    {
        // Add to wating ware, but use copy of pointer, as AddWaitingWare takes ownership
        Ware* tmpWare = ware;
        AddWaitingWare(tmpWare);
    }

    // Evtl. keine Waffen/Bier mehr da, sodass das Rekrutieren gestoppt werden muss
    TryStopRecruiting();

    return ware;
}

void nobBaseWarehouse::AddWaitingWare(Ware*& ware)
{
    waiting_wares.push_back(ware);
    ware->WaitInWarehouse(this);
    // Wenn gerade keiner rausgeht, muss neues Event angemeldet werden
    AddLeavingEvent();
    // Die visuelle Warenanzahl wieder erhöhen
    inventory.visual.Add(ConvertShields(ware->type));
    ware = nullptr; // Take ownership
}

bool nobBaseWarehouse::FreePlaceAtFlag()
{
    if(!waiting_wares.empty())
    {
        AddLeavingEvent();
        return true;
    } else
    {
        // Evtl. war die Flagge voll und das Auslagern musste gestoppt werden
        // Weitere Waren/Figuren zum Auslagern und kein Event angemeldet?
        if(AreWaresToEmpty() && !empty_event)
            // --> Nächstes Event
            empty_event = GetEvMgr().AddEvent(this, empty_INTERVAL, 3);

        return false;
    }
}

void nobBaseWarehouse::AddWare(Ware*& ware)
{
    // Ware not dependent anymore (only if we had a goal)
    if(ware->GetGoal())
    {
        RTTR_Assert(ware->GetGoal() == this); // The goal should be here
        RemoveDependentWare(ware);
    } else
        RTTR_Assert(!IsWareDependent(ware));

    // Die Schilde der verschiedenen Nation in eine "Schild-Sorte" (den der Römer) umwandeln!
    GoodType type = ConvertShields(ware->type);

    gwg->GetPlayer(player).RemoveWare(ware);
    deletePtr(ware);

    inventory.Add(type);

    CheckUsesForNewWare(type);
}

/// Prüft verschiedene Verwendungszwecke für eine neuangekommende Ware
void nobBaseWarehouse::CheckUsesForNewWare(const GoodType gt)
{
    // Wenn es ein Werkzeug war, evtl neuen Job suchen, der jetzt erzeugt werden könnte..
    if(gt >= GD_TONGS && gt <= GD_BOAT)
    {
        for(unsigned i = 0; i < NUM_JOB_TYPES; ++i)
        {
            if(JOB_CONSTS[i].tool == gt)
                gwg->GetPlayer(player).FindWarehouseForAllJobs(Job(i));
        }
    }

    // Wars Baumaterial? Dann den Baustellen Bescheid sagen
    if(gt == GD_BOARDS || gt == GD_STONES)
        gwg->GetPlayer(player).FindMaterialForBuildingSites();

    // Evtl wurden Bier oder Waffen reingetragen --> versuchen zu rekrutieren
    TryRecruiting();

    // Evtl die Ware gleich wieder auslagern, falls erforderlich
    CheckOuthousing(false, gt);
}

/// Prüft verschiedene Sachen, falls ein neuer Mensch das Haus betreten hat
void nobBaseWarehouse::CheckJobsForNewFigure(const Job job)
{
    // Evtl ging ein Gehilfe rein --> versuchen zu rekrutieren
    if(job == JOB_HELPER)
        TryRecruiting();

    if(job >= JOB_PRIVATE && job <= JOB_GENERAL)
    {
        // Reserve prüfen
        RefreshReserve(job - JOB_PRIVATE);
        if(inventory[job] > 0)
        {
            // Truppen prüfen in allen Häusern
            gwg->GetPlayer(player).NewSoldiersAvailable(inventory[job]);
        }
    } else
    {
        if(job == JOB_PACKDONKEY)
        {
            // Straße für Esel suchen
            noRoadNode* goal;
            if(RoadSegment* road = gwg->GetPlayer(player).FindRoadForDonkey(this, &goal))
                road->GotDonkey(OrderDonkey(road, goal));
        } else
        {
            // Evtl. Abnehmer für die Figur wieder finden
            GamePlayer& owner = gwg->GetPlayer(player);
            owner.FindWarehouseForAllJobs(job);
            // Wenns ein Träger war, auch Wege prüfen
            if(job == JOB_HELPER && inventory[JOB_HELPER] == 1)
            {
                // evtl als Träger auf Straßen schicken
                owner.FindCarrierForAllRoads();
                // evtl Träger mit Werkzeug kombiniert -> neuer Beruf
                owner.FindWarehouseForAllJobs();
            }
        }
    }

    // Evtl den Typen gleich wieder auslagern, falls erforderlich
    CheckOuthousing(true, job);
}

void nobBaseWarehouse::AddFigure(noFigure* figure, const bool increase_visual_counts)
{
    // Warenhausarbeiter werden nicht gezählt!
    if(!figure->MemberOfWarehouse())
    {
        // War das ein Boot-Träger?
        if(figure->GetJobType() == JOB_BOATCARRIER)
        {
            if(increase_visual_counts)
            {
                inventory.Add(JOB_HELPER);
                inventory.Add(GD_BOAT);
            } else
            {
                inventory.real.Add(JOB_HELPER);
                inventory.real.Add(GD_BOAT);
            }
        } else
        {
            if(increase_visual_counts)
                inventory.Add(figure->GetJobType());
            else
                inventory.real.Add(figure->GetJobType());
        }
    }

    // Check if we were actually waiting for this figure or if it was just added (e.g. builder that constructed it) to not confuse
    // implementations of Remove...
    if(IsDependentFigure(figure))
        RemoveDependentFigure(figure);
    GetEvMgr().AddToKillList(figure);

    CheckJobsForNewFigure(figure->GetJobType());
}

void nobBaseWarehouse::FetchWare()
{
    if(!fetch_double_protection)
        AddLeavingFigure(new nofWarehouseWorker(pos, player, nullptr, true));

    fetch_double_protection = false;
}

void nobBaseWarehouse::WareLost(Ware* ware)
{
    RemoveDependentWare(ware);
}

void nobBaseWarehouse::CancelWare(Ware* ware)
{
    // Ware aus den Waiting-Wares entfernen
    RTTR_Assert(helpers::contains(waiting_wares, ware));
    waiting_wares.remove(ware);
    // Anzahl davon wieder hochsetzen
    inventory.real.Add(ConvertShields(ware->type));
}

/// Bestellte Figur, die sich noch inder Warteschlange befindet, kommt nicht mehr und will rausgehauen werden
void nobBaseWarehouse::CancelFigure(noFigure* figure)
{
    auto it = std::find(leave_house.begin(), leave_house.end(), figure);
    RTTR_Assert(it != leave_house.end()); // TODO: Is this true in all cases? If yes, remove the check below

    // Figure aus den Waiting-Wares entfernen
    if(it != leave_house.end())
        leave_house.erase(it);

    AddFigure(figure, false);
}

void nobBaseWarehouse::TakeWare(Ware* ware)
{
    // Ware zur Abhängigkeitsliste hinzufügen, damit sie benachrichtigt wird, wenn dieses Lagerhaus zerstört wird
    RTTR_Assert(!helpers::contains(dependent_wares, ware));
    dependent_wares.push_back(ware);
}

void nobBaseWarehouse::OrderTroops(nobMilitary* goal, unsigned count, bool ignoresettingsendweakfirst)
{
    // Soldaten durchgehen und count rausschicken

    // Ränge durchgehen, absteigend, starke zuerst
    if(gwg->GetPlayer(player).GetMilitarySetting(1) >= MILITARY_SETTINGS_SCALE[1] / 2 && !ignoresettingsendweakfirst)
    {
        for(unsigned i = SOLDIER_JOBS.size(); i && count; --i)
        {
            const Job curRank = SOLDIER_JOBS[i - 1];
            // Vertreter der Ränge ggf rausschicken
            while(inventory[curRank] && count)
            {
                nofSoldier* soldier = new nofPassiveSoldier(pos, player, goal, goal, i - 1);
                inventory.real.Remove(curRank);
                AddLeavingFigure(soldier);
                goal->GotWorker(curRank, soldier);
                --count;
            }
        }
    }
    // Ränge durchgehen, aufsteigend, schwache zuerst
    else
    {
        for(unsigned i = 1; i <= SOLDIER_JOBS.size() && count; ++i)
        {
            const Job curRank = SOLDIER_JOBS[i - 1];
            // Vertreter der Ränge ggf rausschicken
            while(inventory[curRank] && count)
            {
                nofSoldier* soldier = new nofPassiveSoldier(pos, player, goal, goal, i - 1);
                inventory.real.Remove(curRank);
                AddLeavingFigure(soldier);
                goal->GotWorker(curRank, soldier);
                --count;
            }
        }
    }
}

nofAggressiveDefender* nobBaseWarehouse::SendAggressiveDefender(nofAttacker* attacker)
{
    // Sind noch Soldaten da?
    unsigned char rank;
    for(rank = SOLDIER_JOBS.size(); rank > 0; --rank) //-V1029
    {
        if(inventory[SOLDIER_JOBS[rank - 1]])
            break;
    }

    // Wenn kein Soldat mehr da ist --> 0 zurückgeben
    if(!rank)
        return nullptr;

    // Dann den Stärksten rausschicken
    auto* soldier = new nofAggressiveDefender(pos, player, this, rank - 1, attacker);
    inventory.real.Remove(SOLDIER_JOBS[rank - 1]);
    AddLeavingFigure(soldier);

    troops_on_mission.push_back(soldier);

    return soldier;
}

void nobBaseWarehouse::SoldierLost(nofSoldier* soldier)
{
    // Soldat konnte nicht (mehr) kommen --> rauswerfen
    RTTR_Assert(dynamic_cast<nofActiveSoldier*>(soldier));
    RTTR_Assert(helpers::contains(troops_on_mission, static_cast<nofActiveSoldier*>(soldier)));
    troops_on_mission.remove(static_cast<nofActiveSoldier*>(soldier));
}

void nobBaseWarehouse::AddActiveSoldier(nofActiveSoldier* soldier)
{
    // Add soldier. If he is still in the leave-queue, then don't add him to the visual settings again
    if(helpers::contains(leave_house, soldier))
        inventory.real.Add(soldier->GetJobType());
    else
        inventory.Add(SOLDIER_JOBS[soldier->GetRank()]);

    // Evtl. geht der Soldat wieder in die Reserve
    RefreshReserve(soldier->GetRank());

    // Truppen prüfen in allen Häusern
    gwg->GetPlayer(player).RegulateAllTroops();

    // Returned home
    if(soldier == defender_)
        NoDefender();
    else
    {
        // Ggf. war er auf Mission
        RTTR_Assert(helpers::contains(troops_on_mission, soldier));
        troops_on_mission.remove(soldier);
    }

    // und Soldat vernichten
    soldier->ResetHome();
    GetEvMgr().AddToKillList(soldier);
}

nofDefender* nobBaseWarehouse::ProvideDefender(nofAttacker* const attacker)
{
    // Ränge zählen
    unsigned rank_count = 0;

    for(unsigned i = 0; i < SOLDIER_JOBS.size(); ++i)
    {
        if(inventory[SOLDIER_JOBS[i]] || reserve_soldiers_available[i])
            ++rank_count;
    }

    if(rank_count)
    {
        // Gewünschten Rang an Hand der Militäreinstellungen ausrechnen, je nachdem wie stark verteidigt werden soll
        unsigned rank = (rank_count - 1) * gwg->GetPlayer(player).GetMilitarySetting(1) / MILITARY_SETTINGS_SCALE[1];

        // Gewünschten Rang suchen
        unsigned r = 0;
        for(unsigned i = 0; i < SOLDIER_JOBS.size(); ++i)
        {
            // andere Soldaten bevorzugen
            if(inventory[SOLDIER_JOBS[i]])
            {
                if(r == rank)
                {
                    // diesen Soldaten wollen wir
                    inventory.real.Remove(SOLDIER_JOBS[i]);
                    auto* soldier = new nofDefender(pos, player, this, i, attacker);
                    return soldier;
                }
                ++r;
            }
            // Reserve
            else if(reserve_soldiers_available[i])
            {
                if(r == rank)
                {
                    // diesen Soldaten wollen wir
                    --reserve_soldiers_available[i];
                    // bei der visuellen Warenanzahl wieder hinzufügen, da er dann wiederrum von der abgezogen wird, wenn
                    // er rausgeht und es so ins minus rutschen würde
                    inventory.visual.Add(SOLDIER_JOBS[i]);
                    auto* soldier = new nofDefender(pos, player, this, i, attacker);
                    return soldier;
                }
                ++r;
            }
        }
    }

    // Kein Soldat gefunden, als letzten Hoffnung die Soldaten nehmen, die ggf in der Warteschlange noch hängen
    for(auto it = leave_house.begin(); it != leave_house.end(); ++it)
    {
        nofSoldier* soldier;
        // Soldat?
        if((*it)->GetGOT() == GOT_NOF_AGGRESSIVEDEFENDER)
        {
            auto* aggDefender = static_cast<nofAggressiveDefender*>(*it);
            aggDefender->NeedForHomeDefence();
            soldier = aggDefender;
        } else if((*it)->GetGOT() == GOT_NOF_PASSIVESOLDIER)
            soldier = static_cast<nofPassiveSoldier*>(*it);
        else
            continue;

        leave_house.erase(it); // Only allowed in the loop as we return now
        soldier->Abrogate();

        auto* defender = new nofDefender(pos, player, this, soldier->GetRank(), attacker);
        soldier->Destroy();
        delete soldier;
        return defender;
    }

    return nullptr;
}

bool nobBaseWarehouse::AreRecruitingConditionsComply()
{
    // Mindestanzahl der Gehilfen die vorhanden sein müssen anhand der 1. Militäreinstellung ausrechnen
    unsigned needed_helpers = 100 - 10 * gwg->GetPlayer(player).GetMilitarySetting(0);

    // einer muss natürlich mindestens vorhanden sein!
    if(!needed_helpers)
        needed_helpers = 1;

    // Wenn alle Bedingungen erfüllt sind, Event anmelden
    return (inventory[JOB_HELPER] >= needed_helpers && inventory[GD_SWORD] && inventory[GD_SHIELDROMANS] && inventory[GD_BEER]);
}

void nobBaseWarehouse::TryRecruiting()
{
    // Wenn noch kein Event angemeldet wurde und alle Bedingungen erfüllt sind, kann ein neues angemeldet werden
    if(!recruiting_event)
    {
        if(AreRecruitingConditionsComply())
            recruiting_event = GetEvMgr().AddEvent(this, RECRUITE_GF + RANDOM.Rand(__FILE__, __LINE__, GetObjId(), RECRUITE_RANDOM_GF), 2);
    }
}

void nobBaseWarehouse::TryStopRecruiting()
{
    // Wenn ein Event angemeldet wurde und die Bedingungen nicht mehr erfüllt sind, muss es wieder vernichtet werden
    if(recruiting_event)
    {
        if(!AreRecruitingConditionsComply())
        {
            GetEvMgr().RemoveEvent(recruiting_event);
            recruiting_event = nullptr;
        }
    }
}

const Inventory& nobBaseWarehouse::GetInventory() const
{
    return inventory.visual;
}

void nobBaseWarehouse::AddGoods(const Inventory& goods, bool addToPlayer)
{
    GamePlayer& owner = gwg->GetPlayer(player);
    for(unsigned i = 0; i < NUM_WARE_TYPES; ++i)
    {
        if(!goods.goods[i])
            continue;
        // Can only add canonical shields (romans)
        RTTR_Assert(GoodType(i) == GD_SHIELDROMANS || ConvertShields(GoodType(i)) != GD_SHIELDROMANS);

        inventory.Add(GoodType(i), goods.goods[i]);
        if(addToPlayer)
            owner.IncreaseInventoryWare(GoodType(i), goods.goods[i]);
        CheckUsesForNewWare(GoodType(i));
    }

    for(unsigned i = 0; i < NUM_JOB_TYPES; ++i)
    {
        if(!goods.people[i])
            continue;
        // Boatcarriers are added as carriers and boat individually
        RTTR_Assert(Job(i) != JOB_BOATCARRIER);

        inventory.Add(Job(i), goods.people[i]);
        if(addToPlayer)
            owner.IncreaseInventoryJob(Job(i), goods.people[i]);
        CheckJobsForNewFigure(Job(i));
    }
}

void nobBaseWarehouse::AddToInventory()
{
    GamePlayer& owner = gwg->GetPlayer(player);
    for(unsigned i = 0; i < NUM_WARE_TYPES; ++i)
        owner.IncreaseInventoryWare(GoodType(i), inventory[GoodType(i)]);

    for(unsigned i = 0; i < NUM_JOB_TYPES; ++i)
        owner.IncreaseInventoryJob(Job(i), inventory[Job(i)]);
}

bool nobBaseWarehouse::CanRecruit(const Job job) const
{
    if(JOB_CONSTS[job].tool == GD_INVALID)
        return false;

    // Do we have a helper and a tool (if required)?
    return inventory[JOB_HELPER] > 0 && (JOB_CONSTS[job].tool == GD_NOTHING || inventory[JOB_CONSTS[job].tool] > 0);
}

bool nobBaseWarehouse::TryRecruitJob(const Job job)
{
    RTTR_Assert(!helpers::contains(SOLDIER_JOBS, job) && job != JOB_PACKDONKEY);
    if(!CanRecruit(job))
        return false;

    // All ok, recruit him
    if(JOB_CONSTS[job].tool != GD_NOTHING)
    {
        inventory.Remove(JOB_CONSTS[job].tool);
        gwg->GetPlayer(player).DecreaseInventoryWare(JOB_CONSTS[job].tool, 1);
    }

    inventory.Remove(JOB_HELPER);
    gwg->GetPlayer(player).DecreaseInventoryJob(JOB_HELPER, 1);

    inventory.Add(job);
    gwg->GetPlayer(player).IncreaseInventoryJob(job, 1);
    return true;
}

InventorySetting nobBaseWarehouse::GetInventorySettingVisual(const Job job) const
{
    return inventorySettingsVisual.figures[(job == JOB_BOATCARRIER) ? JOB_HELPER : job];
}

InventorySetting nobBaseWarehouse::GetInventorySettingVisual(const GoodType ware) const
{
    return inventorySettingsVisual.wares[ConvertShields(ware)];
}

InventorySetting nobBaseWarehouse::GetInventorySetting(const Job job) const
{
    return inventorySettings.figures[(job == JOB_BOATCARRIER) ? JOB_HELPER : job];
}

InventorySetting nobBaseWarehouse::GetInventorySetting(const GoodType ware) const
{
    return inventorySettings.wares[ConvertShields(ware)];
}

/// Verändert Ein/Auslagerungseinstellungen (visuell)
void nobBaseWarehouse::SetInventorySettingVisual(const bool isJob, const unsigned char type, InventorySetting state)
{
    state.MakeValid();
    if(isJob)
        inventorySettingsVisual.figures[type] = state;
    else
        inventorySettingsVisual.wares[type] = state;

    NotifyListeners(1);
}

/// Verändert Ein/Auslagerungseinstellungen (real)
void nobBaseWarehouse::SetInventorySetting(const bool isJob, const unsigned char type, InventorySetting state)
{
    state.MakeValid();
    InventorySetting oldState;
    if(isJob)
    {
        oldState = inventorySettings.figures[type];
        inventorySettings.figures[type] = state;
    } else
    {
        oldState = inventorySettings.wares[type];
        inventorySettings.wares[type] = state;
    }

    /// Bei anderen Spielern als dem lokalen, der das in Auftrag gegeben hat, müssen die visuellen ebenfalls
    /// geändert werden oder auch bei Replays
    if(GAMECLIENT.IsReplayModeOn() || GAMECLIENT.GetPlayerId() != player)
        SetInventorySettingVisual(isJob, type, state);

    if(!isJob && oldState.IsSet(EInventorySetting::STOP) && !state.IsSet(EInventorySetting::STOP))
    {
        // Evtl gabs verlorene Waren, die jetzt in das HQ wieder reinkönnen
        gwg->GetPlayer(player).FindClientForLostWares();
    } // No else here!
    if(!oldState.IsSet(EInventorySetting::SEND) && state.IsSet(EInventorySetting::SEND))
    {
        // Sind Waren vorhanden, die ausgelagert werden müssen und ist noch kein Auslagerungsevent vorhanden --> neues anmelden
        if(!empty_event && (isJob ? inventory[Job(type)] : inventory[GoodType(type)]))
            empty_event = GetEvMgr().AddEvent(this, empty_INTERVAL, 3);
    } else if(!oldState.IsSet(EInventorySetting::COLLECT) && state.IsSet(EInventorySetting::COLLECT))
    {
        // Sollen Waren eingelagert werden? Dann müssen wir neue bestellen
        if(!store_event)
            store_event = GetEvMgr().AddEvent(this, STORE_INTERVAL, 4);
    }
    NotifyListeners(1);
}

/// Verändert alle Ein/Auslagerungseinstellungen einer Kategorie (also Waren oder Figuren)(real)
void nobBaseWarehouse::SetAllInventorySettings(const bool isJob, const std::vector<InventorySetting>& states)
{
    bool isUnstopped = false;
    bool isCollectSet = false;

    const unsigned numElements = isJob ? inventorySettings.figures.size() : inventorySettings.wares.size();
    RTTR_Assert(states.size() == numElements);

    for(unsigned i = 0; i < numElements; i++)
    {
        InventorySetting state = states[i];
        state.MakeValid();
        if(isJob)
            inventorySettings.figures[i] = state;
        else
            inventorySettings.wares[i] = state;
        isUnstopped |= !state.IsSet(EInventorySetting::STOP);
        isCollectSet |= state.IsSet(EInventorySetting::COLLECT);
    }

    // Evtl gabs verlorene Waren, die jetzt in das HQ wieder reinkönnen
    if(isUnstopped)
        gwg->GetPlayer(player).FindClientForLostWares();
    // no else!
    // Sind Waren vorhanden, die ausgelagert werden müssen und ist noch kein Auslagerungsevent vorhanden --> neues anmelden
    if(AreWaresToEmpty() && !empty_event)
        empty_event = GetEvMgr().AddEvent(this, empty_INTERVAL, 3);
    // Sollen Waren eingelagert werden? Dann müssen wir neue bestellen
    if(isCollectSet && !store_event)
        store_event = GetEvMgr().AddEvent(this, STORE_INTERVAL, 4);
}

bool nobBaseWarehouse::IsWareDependent(Ware* ware)
{
    return helpers::contains(dependent_wares, ware);
}

bool nobBaseWarehouse::AreWaresToEmpty() const
{
    // Prüfen, ob Warentyp ausgelagert werden soll und ob noch Waren davon vorhanden sind
    // Waren überprüfen
    for(unsigned i = 0; i < NUM_WARE_TYPES; ++i)
    {
        if(GetInventorySetting(GoodType(i)).IsSet(EInventorySetting::SEND) && inventory[GoodType(i)])
            return true;
    }

    // Figuren überprüfen
    for(unsigned i = 0; i < NUM_JOB_TYPES; ++i)
    {
        if(GetInventorySetting(Job(i)).IsSet(EInventorySetting::SEND) && inventory[Job(i)])
            return true;
    }

    return false;
}

bool nobBaseWarehouse::DefendersAvailable() const
{
    const auto isNonZero = [](const unsigned ct) { return ct != 0; };
    if(helpers::contains_if(reserve_soldiers_available, isNonZero))
        return true;
    for(Job job : SOLDIER_JOBS)
    {
        if(inventory[job])
            return true;
    }

    return false;
}

unsigned nobBaseWarehouse::IncreaseReserveVisual(unsigned rank)
{
    return ++reserve_soldiers_claimed_visual[rank];
}

unsigned nobBaseWarehouse::DecreaseReserveVisual(unsigned rank)
{
    if(reserve_soldiers_claimed_visual[rank])
        --reserve_soldiers_claimed_visual[rank];

    return reserve_soldiers_claimed_visual[rank];
}

void nobBaseWarehouse::SetRealReserve(const unsigned rank, const unsigned count)
{
    reserve_soldiers_claimed_real[rank] = count;

    // Replay oder anderer Spieler? Dann die visuellen auch erhöhen
    if(GAMECLIENT.IsReplayModeOn() || GAMECLIENT.GetPlayerId() != player)
        reserve_soldiers_claimed_visual[rank] = count;

    // Geforderte Soldaten ggf. einbeziehen
    RefreshReserve(rank);
}

void nobBaseWarehouse::RefreshReserve(unsigned rank)
{
    // Zuviele oder zuwenig Soldaten einkassiert?
    if(reserve_soldiers_available[rank] < reserve_soldiers_claimed_real[rank])
    {
        // Zuwenig --> gucken,ob wir noch mehr einkassieren können
        if(inventory[SOLDIER_JOBS[rank]])
        {
            // ja, dann nehmen wir mal noch soviele wie nötig und möglich
            unsigned add = std::min(inventory[SOLDIER_JOBS[rank]],                                           // möglich
                                    reserve_soldiers_claimed_real[rank] - reserve_soldiers_available[rank]); // nötig

            // Bei der Reserve hinzufügen
            reserve_soldiers_available[rank] += add;
            // vom Warenbestand abziehen
            inventory.Remove(SOLDIER_JOBS[rank], add);
        }
    } else if(reserve_soldiers_available[rank] > reserve_soldiers_claimed_real[rank])
    {
        // Zuviele, dann wieder welche freigeben
        unsigned subtract = reserve_soldiers_available[rank] - reserve_soldiers_claimed_real[rank];

        // Bei der Reserve abziehen
        reserve_soldiers_available[rank] -= subtract;
        // beim Warenbestand hinzufügen
        inventory.Add(SOLDIER_JOBS[rank], subtract);
        // if the rank is supposed to be send away, do it!
        CheckOuthousing(true, SOLDIER_JOBS[rank]);
        // Ggf. Truppen in die Militärgebäude schicken
        gwg->GetPlayer(player).RegulateAllTroops();
    }
    // ansonsten ists gleich und alles ist in Ordnung!
}

void nobBaseWarehouse::CheckOuthousing(const bool isJob, unsigned job_ware_id)
{
    // Check if we need to send this ware or figure and register an event for this
    // If we already have an event, we don't need to do anything
    if(empty_event)
        return;

    // Bootsträger in Träger umwandeln, der evtl dann raus soll
    if(isJob && job_ware_id == JOB_BOATCARRIER)
        job_ware_id = JOB_HELPER;

    const InventorySetting setting = isJob ? GetInventorySetting(Job(job_ware_id)) : GetInventorySetting(GoodType(job_ware_id));
    if(setting.IsSet(EInventorySetting::SEND))
        empty_event = GetEvMgr().AddEvent(this, empty_INTERVAL, 3);
}

/// For debug only
bool nobBaseWarehouse::IsDependentFigure(noFigure* fig) const
{
    return helpers::contains(dependent_figures, fig);
}

/// Available goods of a specific type that can be used for trading
unsigned nobBaseWarehouse::GetAvailableWaresForTrading(const GoodType gt) const
{
    // We need a helper as leader
    if(!inventory[JOB_HELPER])
        return 0;

    return std::min(inventory[gt], inventory[JOB_PACKDONKEY]);
}

/// Available figures of a speciefic type that can be used for trading
unsigned nobBaseWarehouse::GetAvailableFiguresForTrading(const Job job) const
{
    // We need a helper as leader
    if(!inventory[JOB_HELPER])
        return 0;

    if(job == JOB_HELPER)
        return (inventory[JOB_HELPER] - 1) / 2; // need one as leader
    else
        return std::min(inventory[job], inventory[JOB_HELPER] - 1);
}

/// Starts a trade caravane from this warehouse
void nobBaseWarehouse::StartTradeCaravane(const boost::variant<GoodType, Job>& what, const unsigned count, const TradeRoute& tr,
                                          nobBaseWarehouse* goal)
{
    auto* tl = new nofTradeLeader(pos, player, tr, this->GetPos(), goal->GetPos());
    AddLeavingFigure(tl);

    // Create the donkeys or other people
    nofTradeDonkey* last = nullptr;
    for(unsigned i = 0; i < count; ++i)
    {
        auto* next = new nofTradeDonkey(pos, player, what);

        if(last)
            last->SetSuccessor(next);
        else
            tl->SetSuccessor(next);

        last = next;
        AddLeavingFigure(next);
    }

    GamePlayer& owner = gwg->GetPlayer(player);
    // Remove leader
    inventory.real.Remove(JOB_HELPER);
    owner.DecreaseInventoryJob(JOB_HELPER, 1);

    // Also diminish the count of donkeys
    boost::apply_visitor(composeVisitor(
                           [&](const Job job) {
                               // remove the jobs
                               inventory.real.Remove(job, count);
                               owner.DecreaseInventoryJob(job, count);
                           },
                           [&](const GoodType gt) {
                               // Diminish the goods in the warehouse
                               inventory.real.Remove(gt, count);
                               owner.DecreaseInventoryWare(gt, count);
                               // now that we have removed the goods lets remove the donkeys
                               inventory.real.Remove(JOB_PACKDONKEY, count);
                               owner.DecreaseInventoryJob(JOB_PACKDONKEY, count);
                           }),
                         what);
}
