// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nobBaseWarehouse.h"
#include "BurnedWarehouse.h"
#include "EventManager.h"
#include "FindWhConditions.h"
#include "GamePlayer.h"
#include "GlobalGameSettings.h"
#include "SerializedGameData.h"
#include "Ware.h"
#include "WineLoader.h"
#include "commonDefines.h"
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
#include "helpers/pointerContainerUtils.h"
#include "network/GameClient.h"
#include "nobMilitary.h"
#include "random/Random.h"
#include "variant.h"
#include "world/GameWorld.h"
#include "nodeObjs/noFlag.h"
#include "gameData/JobConsts.h"
#include "gameData/SettingTypeConv.h"
#include "gameData/ShieldConsts.h"
#include "s25util/Log.h"
#include <boost/pointer_cast.hpp>
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

nobBaseWarehouse::nobBaseWarehouse(const BuildingType type, const MapPoint pos, const unsigned char player,
                                   const Nation nation)
    : nobBaseMilitary(type, pos, player, nation), fetch_double_protection(false), recruiting_event(nullptr),
      empty_event(nullptr), store_event(nullptr)
{
    producinghelpers_event = GetEvMgr().AddEvent(this, PRODUCE_HELPERS_GF + RANDOM_RAND(PRODUCE_HELPERS_RANDOM_GF), 1);
    // Reserve nullen
    reserve_soldiers_available.fill(0);
    reserve_soldiers_claimed_visual.fill(0);
    reserve_soldiers_claimed_real.fill(0);
}

nobBaseWarehouse::~nobBaseWarehouse() = default;

void nobBaseWarehouse::DestroyBuilding()
{
    // Den Waren und Figuren Bescheid sagen, die zu uns auf den Weg sind, dass wir nun nicht mehr existieren
    for(noFigure* dependent_figure : dependent_figures)
    {
        // Only send figures home that are not already at this position.
        if(dependent_figure->GetPos() == GetPos())
        {
            dependent_figure->SetGoalTonullptr();
            dependent_figure->CutCurrentRoad();
            dependent_figure->StartWandering();
        } else
            dependent_figure->GoHome();
    }
    dependent_figures.clear();

    for(Ware* dependent_ware : dependent_wares)
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
        waiting_ware->Destroy();
    }
    waiting_wares.clear();

    // restliche Warenbestände von der Inventur wieder abziehen
    for(const auto good : helpers::enumRange<GoodType>())
        world->GetPlayer(player).DecreaseInventoryWare(good, inventory[good]);

    // move soldiers from reserve to inventory.
    for(unsigned rank = 0; rank < world->GetGGS().GetMaxMilitaryRank(); ++rank)
    {
        if(reserve_soldiers_available[rank] > 0)
            inventory.real.Add(SOLDIER_JOBS[rank], reserve_soldiers_available[rank]);
    }

    // Objekt, das die flüchtenden Leute nach und nach ausspuckt, erzeugen
    world->AddFigure(pos, std::make_unique<BurnedWarehouse>(pos, player, inventory.real.people));

    nobBaseMilitary::DestroyBuilding();
}

void nobBaseWarehouse::Serialize(SerializedGameData& sgd) const
{
    nobBaseMilitary::Serialize(sgd);

    sgd.PushObjectContainer(waiting_wares, true);
    sgd.PushBool(fetch_double_protection);
    sgd.PushObjectContainer(dependent_figures);
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

    for(const auto i : helpers::enumRange<GoodType>())
    {
        sgd.PushUnsignedInt(inventory.visual[i]);
        sgd.PushUnsignedInt(inventory.real[i]);
        sgd.PushUnsignedChar(static_cast<uint8_t>(inventorySettings[i]));
    }
    for(const auto i : helpers::enumRange<Job>())
    {
        sgd.PushUnsignedInt(inventory.visual[i]);
        sgd.PushUnsignedInt(inventory.real[i]);
        sgd.PushUnsignedChar(static_cast<uint8_t>(inventorySettings[i]));
    }
}

nobBaseWarehouse::nobBaseWarehouse(SerializedGameData& sgd, const unsigned obj_id) : nobBaseMilitary(sgd, obj_id)
{
    sgd.PopObjectContainer(waiting_wares, GO_Type::Ware);
    fetch_double_protection = sgd.PopBool();
    sgd.PopObjectContainer(dependent_figures);
    sgd.PopObjectContainer(dependent_wares, GO_Type::Ware);

    producinghelpers_event = sgd.PopEvent();
    recruiting_event = sgd.PopEvent();
    empty_event = sgd.PopEvent();
    store_event = sgd.PopEvent();

    for(unsigned i = 0; i < 5; ++i)
    {
        reserve_soldiers_available[i] = sgd.PopUnsignedInt();
        reserve_soldiers_claimed_visual[i] = reserve_soldiers_claimed_real[i] = sgd.PopUnsignedInt();
    }

    for(const auto i : helpers::enumRange<GoodType>())
    {
        if(sgd.GetGameDataVersion() < 11 && wineaddon::isWineAddonGoodType(i))
            continue;
        inventory.visual[i] = sgd.PopUnsignedInt();
        inventory.real[i] = sgd.PopUnsignedInt();
        inventorySettings[i] = inventorySettingsVisual[i] = static_cast<InventorySetting>(sgd.PopUnsignedChar());
    }
    for(const auto i : helpers::enumRange<Job>())
    {
        if(sgd.GetGameDataVersion() < 11 && wineaddon::isWineAddonJobType(i))
            continue;
        inventory.visual[i] = sgd.PopUnsignedInt();
        inventory.real[i] = sgd.PopUnsignedInt();
        inventorySettings[i] = inventorySettingsVisual[i] = static_cast<InventorySetting>(sgd.PopUnsignedChar());
    }
}

void nobBaseWarehouse::Clear()
{
    // Add reserve soldiers back
    for(unsigned i = 0; i < reserve_soldiers_available.size(); i++)
        inventory.Add(SOLDIER_JOBS[i], reserve_soldiers_available[i]);
    reserve_soldiers_available.fill(0);

    GamePlayer& owner = world->GetPlayer(player);
    for(const auto i : helpers::enumRange<GoodType>())
        owner.DecreaseInventoryWare(i, inventory[i]);

    for(const auto i : helpers::enumRange<Job>())
        owner.DecreaseInventoryJob(i, inventory[i]);

    inventory.clear();

    for(auto& waiting_ware : waiting_wares)
    {
        waiting_ware->WareLost(player);
        waiting_ware->Destroy();
    }
    waiting_wares.clear();
}

void nobBaseWarehouse::OrderCarrier(noRoadNode& goal, RoadSegment& workplace)
{
    RTTR_Assert(workplace.getCarrier(0) == nullptr);
    const bool isBoatRequired = workplace.GetRoadType() == RoadType::Water;

    // We assume, that the caller already checked, if this is possible
    RTTR_Assert(inventory[Job::Helper]);
    if(isBoatRequired)
        RTTR_Assert(inventory[GoodType::Boat]);

    std::unique_ptr<noFigure> carrier = std::make_unique<nofCarrier>(
      isBoatRequired ? CarrierType::Boat : CarrierType::Normal, pos, player, &workplace, &goal);
    workplace.setCarrier(0, static_cast<nofCarrier*>(carrier.get()));

    if(!UseFigureAtOnce(carrier, goal))
        AddLeavingFigure(std::move(carrier));

    inventory.real.Remove(Job::Helper);
    if(isBoatRequired)
        inventory.real.Remove(GoodType::Boat);

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

    std::unique_ptr<noFigure> fig = JobFactory::CreateJob(job, pos, player, *goal);
    // Ziel Bescheid sagen, dass dortin ein neuer Arbeiter kommt (bei Flaggen als das anders machen)
    if(goal->GetType() != NodalObjectType::Flag)
        checkedCast<noBaseBuilding*>(goal)->GotWorker(job, *fig);

    // Wenn Figur nicht sofort von abgeleiteter Klasse verwenet wird, fügen wir die zur Leave-Liste hinzu
    if(!UseFigureAtOnce(fig, *goal))
        AddLeavingFigure(std::move(fig));

    inventory.real.Remove(job);

    // Evtl. kein Gehilfe mehr da, sodass das Rekrutieren gestoppt werden muss
    TryStopRecruiting();

    return true;
}

nofCarrier* nobBaseWarehouse::OrderDonkey(RoadSegment* road, noRoadNode* const goal_flag)
{
    // Überhaupt ein Esel vorhanden?
    if(!inventory[Job::PackDonkey])
        return nullptr;

    auto donkey = std::make_unique<nofCarrier>(CarrierType::Donkey, pos, player, road, goal_flag);
    nofCarrier* donkeyRef = donkey.get();
    AddLeavingFigure(std::move(donkey));
    inventory.real.Remove(Job::PackDonkey);

    return donkeyRef;
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
    for(const auto i : helpers::enumRange<GoodType>())
    {
        // Soll Ware eingeliefert werden?
        if(!GetInventorySetting(i).IsSet(EInventorySetting::Collect))
            continue;

        storing_wanted = true;

        // Lagerhaus suchen, das diese Ware enthält
        nobBaseWarehouse* wh = world->GetPlayer(player).FindWarehouse(*this, FW::HasWareButNoCollect(i), false, false);
        // Gefunden?
        if(wh)
        {
            // Dann bestellen
            Ware* ware = wh->OrderWare(i, this);
            if(ware)
            {
                RTTR_Assert(IsWareDependent(*ware));
                storing_done = true;
                break;
            }
        }
    }

    // Menschen "bestellen" wenn noch keine Ware bestellt wurde
    if(!storing_done)
    {
        for(const auto i : helpers::enumRange<Job>())
        {
            // Soll dieser Typ von Mensch bestellt werden?
            if(!GetInventorySetting(i).IsSet(EInventorySetting::Collect))
                continue;

            storing_wanted = true;

            // Lagerhaus suchen, das diesen Job enthält
            nobBaseWarehouse* wh =
              world->GetPlayer(player).FindWarehouse(*this, FW::HasFigureButNoCollect(i, false), false, false);
            // Gefunden?
            if(wh)
            {
                // Dann bestellen
                if(wh->OrderJob(i, this, false))
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
    if(!world->IsRoadNodeForFigures(world->GetNeighbour(pos, Direction::SouthEast)))
    {
        empty_event = GetEvMgr().AddEvent(this, empty_INTERVAL, 3);
        return;
    }

    std::vector<boost_variant2<GoodType, Job>> possibleTypes;
    // Waren und Figuren zum Auslagern zusammensuchen
    // Wenn keine Platz an Flagge, dann keine Waren raus
    if(GetFlag()->HasSpaceForWare())
    {
        for(const auto i : helpers::enumRange<GoodType>())
        {
            if(GetInventorySetting(i).IsSet(EInventorySetting::Send) && inventory[i])
                possibleTypes.push_back(i);
        }
    }

    for(const auto i : helpers::enumRange<Job>())
    {
        // Figuren, die noch nicht implementiert sind, nicht nehmen!
        if(GetInventorySetting(i).IsSet(EInventorySetting::Send) && inventory[i])
            possibleTypes.push_back(i);
    }

    // Gibts überhaupt welche?
    if(possibleTypes.empty())
        // ansonsten gleich tschüss
        return;

    // Eine ID zufällig auswählen
    const auto selectedId = RANDOM_ELEMENT(possibleTypes);

    if(holds_alternative<GoodType>(selectedId))
    {
        // Ware
        const auto goodType = get<GoodType>(selectedId);
        auto ware = std::make_unique<Ware>(goodType, nullptr, this);
        noBaseBuilding* wareGoal = world->GetPlayer(player).FindClientForWare(*ware);
        if(wareGoal != this)
        {
            ware->SetGoal(wareGoal);

            // Ware zur Liste hinzufügen, damit sie dann rausgetragen wird
            waiting_wares.push_back(std::move(ware));

            AddLeavingEvent();

            // Ware aus Inventar entfernen
            inventory.real.Remove(goodType);

            // Evtl. kein Schwert/Schild/Bier mehr da, sodass das Rekrutieren gestoppt werden muss
            TryStopRecruiting();
        } else
            world->GetPlayer(player).RemoveWare(*ware);
    } else
    {
        const auto jobType = get<Job>(selectedId);
        nobBaseWarehouse* wh =
          world->GetPlayer(player).FindWarehouse(*this, FW::AcceptsFigureButNoSend(jobType), true, false);
        if(wh != this)
        {
            auto fig = std::make_unique<nofPassiveWorker>(jobType, pos, player, nullptr);

            if(wh)
                fig->GoHome(wh);
            else
                fig->StartWandering();

            AddLeavingFigure(std::move(fig));

            // Person aus Inventar entfernen
            inventory.real.Remove(jobType);

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
    max_recruits = std::min(inventory[GoodType::Sword], inventory[GoodType::ShieldRomans]);
    max_recruits = std::min(inventory[GoodType::Beer], max_recruits);
    max_recruits = std::min(inventory[Job::Helper], max_recruits);

    GamePlayer& owner = world->GetPlayer(player);
    const unsigned recruiting_ratio = owner.GetMilitarySetting(0);
    unsigned real_recruits = max_recruits * recruiting_ratio / MILITARY_SETTINGS_SCALE[0];
    // Wurde abgerundet?
    unsigned remainingRecruits = real_recruits * recruiting_ratio % MILITARY_SETTINGS_SCALE[0];
    if(remainingRecruits != 0 && unsigned(RANDOM_RAND(MILITARY_SETTINGS_SCALE[0] - 1)) < remainingRecruits)
        ++real_recruits;
    else if(real_recruits == 0)
        return; // Nothing to do

    inventory.Add(Job::Private, real_recruits);
    owner.IncreaseInventoryJob(Job::Private, real_recruits);

    inventory.Remove(Job::Helper, real_recruits);
    owner.DecreaseInventoryJob(Job::Helper, real_recruits);

    inventory.Remove(GoodType::Sword, real_recruits);
    owner.DecreaseInventoryWare(GoodType::Sword, real_recruits);

    inventory.Remove(GoodType::ShieldRomans, real_recruits);
    owner.DecreaseInventoryWare(GoodType::ShieldRomans, real_recruits);

    inventory.Remove(GoodType::Beer, real_recruits);
    owner.DecreaseInventoryWare(GoodType::Beer, real_recruits);

    // Evtl. versuchen nächsten zu rekrutieren
    TryRecruiting();

    // If there were no soliders before
    if(inventory[Job::Private] == real_recruits)
    {
        // Check reserve
        this->RefreshReserve(0);
        // And check if we need the new ones (if any left) e.g. for military buildings
        if(inventory[Job::Private] > 0)
            owner.NewSoldiersAvailable(inventory[Job::Private]);
    }
}

void nobBaseWarehouse::HandleProduceHelperEvent()
{
    // Nur bei unter 100 Trägern, weitere "produzieren"
    if(inventory[Job::Helper] < 100)
    {
        inventory.Add(Job::Helper);

        GamePlayer& owner = world->GetPlayer(player);
        owner.IncreaseInventoryJob(Job::Helper, 1);

        if(inventory[Job::Helper] == 1)
        {
            // Wenn vorher keine Träger da waren, müssen alle unbesetzen Wege gucken, ob sie nen Weg hierher finden,
            // könnte ja sein, dass vorher nich genug Träger da waren
            owner.FindCarrierForAllRoads();
            // evtl Träger mit Werkzeug kombiniert -> neuer Beruf
            owner.FindWarehouseForAllJobs();
        }
    } else if(inventory[Job::Helper] > 100)
    {
        // Bei Überbevölkerung Träger vernichten
        inventory.Remove(Job::Helper);

        world->GetPlayer(player).DecreaseInventoryJob(Job::Helper, 1);
    }

    producinghelpers_event = GetEvMgr().AddEvent(this, PRODUCE_HELPERS_GF + RANDOM_RAND(PRODUCE_HELPERS_RANDOM_GF), 1);

    // Evtl. genau der Gehilfe, der zum Rekrutieren notwendig ist
    TryRecruiting();

    // Evtl die Typen gleich wieder auslagern, falls erforderlich
    CheckOuthousing(Job::Helper);
}

void nobBaseWarehouse::HandleLeaveEvent()
{
#if RTTR_ENABLE_ASSERTS
    // Harbors have more queues. Ignore for now
    if(GetGOT() != GO_Type::NobHarborbuilding)
    {
        Inventory should = inventory.real;
        for(auto& it : leave_house)
        {
            // Don't count warehouse workers
            if(!it->MemberOfWarehouse())
            {
                if(it->GetJobType() == Job::BoatCarrier)
                    should.Add(Job::Helper);
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
    if(!world->IsRoadNodeForFigures(world->GetNeighbour(pos, Direction::SouthEast)))
    {
        // there's a fight

        // try to find a defender
        const auto it = helpers::find_if(leave_house, [](const auto& sld) {
            return sld->GetGOT() == GO_Type::NofAggressivedefender || sld->GetGOT() == GO_Type::NofDefender;
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
        leave_house.push_front(std::move(*it));
        leave_house.erase(it);
    }

    // Figuren kommen zuerst raus
    if(!leave_house.empty())
    {
        noFigure& fig = world->AddFigure(pos, std::move(leave_house.front()));
        leave_house.pop_front();

        // Init road walking for figures walking on roads
        if(fig.IsWalkingOnRoad())
            fig.InitializeRoadWalking(GetRoute(Direction::SouthEast), 0, true);

        fig.ActAtFirst();
        // Bei Lagerhausarbeitern das nicht abziehen!
        if(!fig.MemberOfWarehouse())
        {
            // War das ein Boot-Träger?
            if(fig.GetJobType() == Job::BoatCarrier)
            {
                // Remove helper and boat separately
                inventory.visual.Remove(Job::Helper);
                inventory.visual.Remove(GoodType::Boat);
            } else
                inventory.visual.Remove(fig.GetJobType());

            if(fig.GetGOT() == GO_Type::NofTradedonkey)
            {
                // Trade donkey carrying wares?
                const auto& carriedWare = static_cast<nofTradeDonkey&>(fig).GetCarriedWare();
                if(carriedWare)
                    inventory.visual.Remove(*carriedWare);
            }
        }
    } else
    {
        if(GetFlag()->HasSpaceForWare())
        {
            // Dann Ware raustragen lassen
            auto ware = std::move(waiting_wares.front());
            waiting_wares.pop_front();
            inventory.visual.Remove(ConvertShields(ware->type));
            ware->Carry(GetFlag());
            world->AddFigure(pos, std::make_unique<nofWarehouseWorker>(pos, player, std::move(ware), false))
              .WalkToGoal();
        } else
        {
            // Kein Platz mehr für Waren --> keiner brauch mehr rauszukommen, und Figuren gibts ja auch keine mehr
            go_out = false;
        }
    }

    // Wenn keine Figuren und Waren mehr da sind (bzw die Flagge vorm Haus voll ist), brauch auch keiner mehr
    // rauszukommen
    if(leave_house.empty() && waiting_wares.empty())
        go_out = false;

    if(go_out)
        leaving_event = GetEvMgr().AddEvent(this, LEAVE_INTERVAL + RANDOM_RAND(LEAVE_INTERVAL_RAND));
}

/// Abgeleitete kann eine gerade erzeugte Ware ggf. sofort verwenden
/// (muss in dem Fall true zurückgeben)
bool nobBaseWarehouse::UseWareAtOnce(std::unique_ptr<Ware>& /*ware*/, noBaseBuilding& /*goal*/)
{
    return false;
}

/// Dasselbe für Menschen
bool nobBaseWarehouse::UseFigureAtOnce(std::unique_ptr<noFigure>& /*fig*/, noRoadNode& /*goal*/)
{
    return false;
}

Ware* nobBaseWarehouse::OrderWare(const GoodType good, noBaseBuilding* const goal)
{
    RTTR_Assert(goal);
    // Ware überhaupt hier vorhanden (Abfrage eigentlich nicht nötig, aber erstmal zur Sicherheit)
    if(!inventory[good])
    {
        LOG.write("nobBaseWarehouse::OrderWare: WARNING: No ware type %u in warehouse!\n")
          % static_cast<unsigned>(good);
        return nullptr;
    }

    auto ware = std::make_unique<Ware>(good, goal, this);
    inventory.Remove(good);

    // Copy pointer so functions below can take ownership
    Ware* wareRef = ware.get();

    // If we don't want to use the ware right away we add it to the waiting wares
    if(!UseWareAtOnce(ware, *goal))
        AddWaitingWare(std::move(ware));
    RTTR_Assert(!ware);

    // Evtl. keine Waffen/Bier mehr da, sodass das Rekrutieren gestoppt werden muss
    TryStopRecruiting();

    return wareRef;
}

void nobBaseWarehouse::AddWaitingWare(std::unique_ptr<Ware> ware)
{
    inventory.visual.Add(ConvertShields(ware->type));
    ware->WaitInWarehouse(this);
    waiting_wares.push_back(std::move(ware));
    AddLeavingEvent();
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

void nobBaseWarehouse::AddWare(std::unique_ptr<Ware> ware)
{
    // Ware not dependent anymore (only if we had a goal)
    if(ware->GetGoal())
    {
        RTTR_Assert(ware->GetGoal() == this); // The goal should be here
        RemoveDependentWare(*ware);
    } else
        RTTR_Assert(!IsWareDependent(*ware));

    // Die Schilde der verschiedenen Nation in eine "Schild-Sorte" (den der Römer) umwandeln!
    GoodType type = ConvertShields(ware->type);

    world->GetPlayer(player).RemoveWare(*ware);

    inventory.Add(type);

    CheckUsesForNewWare(type);
}

/// Prüft verschiedene Verwendungszwecke für eine neuangekommende Ware
void nobBaseWarehouse::CheckUsesForNewWare(const GoodType gt)
{
    // Wenn es ein Werkzeug war, evtl neuen Job suchen, der jetzt erzeugt werden könnte..
    if(gt >= GoodType::Tongs && gt <= GoodType::Boat)
    {
        for(const auto job : helpers::EnumRange<Job>{})
        {
            if(JOB_CONSTS[job].tool == gt)
                world->GetPlayer(player).FindWarehouseForAllJobs(job);
        }
    }

    // Wars Baumaterial? Dann den Baustellen Bescheid sagen
    if(gt == GoodType::Boards || gt == GoodType::Stones)
        world->GetPlayer(player).FindMaterialForBuildingSites();

    // Evtl wurden Bier oder Waffen reingetragen --> versuchen zu rekrutieren
    TryRecruiting();

    // Evtl die Ware gleich wieder auslagern, falls erforderlich
    CheckOuthousing(gt);
}

/// Prüft verschiedene Sachen, falls ein neuer Mensch das Haus betreten hat
void nobBaseWarehouse::CheckJobsForNewFigure(const Job job)
{
    // Evtl ging ein Gehilfe rein --> versuchen zu rekrutieren
    if(job == Job::Helper)
        TryRecruiting();

    if(isSoldierJob(job))
    {
        // Reserve prüfen
        RefreshReserve(getSoldierRank(job));
        if(inventory[job] > 0)
        {
            // Truppen prüfen in allen Häusern
            world->GetPlayer(player).NewSoldiersAvailable(inventory[job]);
        }
    } else
    {
        if(job == Job::PackDonkey)
        {
            // Straße für Esel suchen
            noRoadNode* goal;
            if(RoadSegment* road = world->GetPlayer(player).FindRoadForDonkey(this, &goal))
                road->GotDonkey(OrderDonkey(road, goal));
        } else
        {
            // Evtl. Abnehmer für die Figur wieder finden
            GamePlayer& owner = world->GetPlayer(player);
            owner.FindWarehouseForAllJobs(job);
            // Wenns ein Träger war, auch Wege prüfen
            if(job == Job::Helper && inventory[Job::Helper] == 1)
            {
                // evtl als Träger auf Straßen schicken
                owner.FindCarrierForAllRoads();
                // evtl Träger mit Werkzeug kombiniert -> neuer Beruf
                owner.FindWarehouseForAllJobs();
            }
        }
    }

    // Evtl den Typen gleich wieder auslagern, falls erforderlich
    CheckOuthousing(job);
}

void nobBaseWarehouse::AddFigure(std::unique_ptr<noFigure> figure, const bool increase_visual_counts)
{
    // Warenhausarbeiter werden nicht gezählt!
    if(!figure->MemberOfWarehouse())
    {
        // War das ein Boot-Träger?
        if(figure->GetJobType() == Job::BoatCarrier)
        {
            if(increase_visual_counts)
            {
                inventory.Add(Job::Helper);
                inventory.Add(GoodType::Boat);
            } else
            {
                inventory.real.Add(Job::Helper);
                inventory.real.Add(GoodType::Boat);
            }
        } else
        {
            if(increase_visual_counts)
                inventory.Add(figure->GetJobType());
            else
                inventory.real.Add(figure->GetJobType());
        }
    }

    // Check if we were actually waiting for this figure or if it was just added (e.g. builder that constructed it) to
    // not confuse implementations of Remove...
    if(IsDependentFigure(*figure))
        RemoveDependentFigure(*figure);

    CheckJobsForNewFigure(figure->GetJobType());
    GetEvMgr().AddToKillList(std::move(figure));
}

void nobBaseWarehouse::FetchWare()
{
    if(!fetch_double_protection)
        AddLeavingFigure(std::make_unique<nofWarehouseWorker>(pos, player, nullptr, true));

    fetch_double_protection = false;
}

void nobBaseWarehouse::WareLost(Ware& ware)
{
    RemoveDependentWare(ware);
}

void nobBaseWarehouse::CancelWare(Ware*& ware)
{
    inventory.real.Add(ConvertShields(ware->type));
    helpers::extractPtr(waiting_wares, ware);
    ware = nullptr;
}

/// Bestellte Figur, die sich noch inder Warteschlange befindet, kommt nicht mehr und will rausgehauen werden
void nobBaseWarehouse::CancelFigure(noFigure* figure)
{
    auto it = helpers::findPtr(leave_house, figure);
    RTTR_Assert(it != leave_house.end());

    // Figure aus den Waiting-Wares entfernen
    AddFigure(std::move(*it), false);
    leave_house.erase(it);
}

void nobBaseWarehouse::TakeWare(Ware* ware)
{
    // Ware zur Abhängigkeitsliste hinzufügen, damit sie benachrichtigt wird, wenn dieses Lagerhaus zerstört wird
    RTTR_Assert(!helpers::contains(dependent_wares, ware));
    dependent_wares.push_back(ware);
}

void nobBaseWarehouse::OrderTroops(nobMilitary* goal, std::array<unsigned, NUM_SOLDIER_RANKS>& counts, unsigned& max)
{
    unsigned start, limit;
    int step;

    if(world->GetPlayer(player).GetMilitarySetting(1) >= MILITARY_SETTINGS_SCALE[1] / 2)
    {
        // Ränge durchgehen, absteigend, starke zuerst
        start = SOLDIER_JOBS.size();
        step = -1;
        limit = 0;
    } else
    {
        // Ränge durchgehen, aufsteigend, schwache zuerst
        start = 1;
        step = 1;
        limit = SOLDIER_JOBS.size() + 1;
    }

    for(unsigned i = start; i != limit && max; i += step)
    {
        const Job curRank = SOLDIER_JOBS[i - 1];
        // Vertreter der Ränge ggf rausschicken
        while(inventory[curRank] && max && counts[i - 1])
        {
            auto soldier = std::make_unique<nofPassiveSoldier>(pos, player, goal, goal, i - 1);
            inventory.real.Remove(curRank);
            goal->GotWorker(curRank, *soldier);
            AddLeavingFigure(std::move(soldier));
            --max;
            --counts[i - 1];
        }
    }
}

nofAggressiveDefender* nobBaseWarehouse::SendAggressiveDefender(nofAttacker& attacker)
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
    auto soldier = std::make_unique<nofAggressiveDefender>(pos, player, *this, rank - 1, attacker);
    nofAggressiveDefender& soldierRef = *soldier;
    inventory.real.Remove(SOLDIER_JOBS[rank - 1]);
    AddLeavingFigure(std::move(soldier));

    troops_on_mission.push_back(&soldierRef);

    return &soldierRef;
}

void nobBaseWarehouse::SoldierLost(nofSoldier* soldier)
{
    // Soldat konnte nicht (mehr) kommen --> rauswerfen
    RTTR_Assert(dynamic_cast<nofActiveSoldier*>(soldier));
    RTTR_Assert(helpers::contains(troops_on_mission, static_cast<nofActiveSoldier*>(soldier)));
    troops_on_mission.remove(static_cast<nofActiveSoldier*>(soldier));
}

void nobBaseWarehouse::AddActiveSoldier(std::unique_ptr<nofActiveSoldier> soldier)
{
    // Add soldier. If he is still in the leave-queue, then don't add him to the visual settings again
    if(helpers::contains(leave_house, soldier))
        inventory.real.Add(soldier->GetJobType());
    else
        inventory.Add(SOLDIER_JOBS[soldier->GetRank()]);

    // Evtl. geht der Soldat wieder in die Reserve
    RefreshReserve(soldier->GetRank());

    // Truppen prüfen in allen Häusern
    world->GetPlayer(player).RegulateAllTroops();

    // Returned home
    if(soldier.get() == defender_)
        NoDefender();
    else
    {
        // Ggf. war er auf Mission
        RTTR_Assert(helpers::contains(troops_on_mission, soldier.get()));
        troops_on_mission.remove(soldier.get());
    }

    // und Soldat vernichten
    soldier->ResetHome();
    GetEvMgr().AddToKillList(std::move(soldier));
}

std::unique_ptr<nofDefender> nobBaseWarehouse::ProvideDefender(nofAttacker& attacker)
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
        unsigned rank = (rank_count - 1) * world->GetPlayer(player).GetMilitarySetting(1) / MILITARY_SETTINGS_SCALE[1];

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
                    return std::make_unique<nofDefender>(pos, player, *this, i, attacker);
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
                    // bei der visuellen Warenanzahl wieder hinzufügen, da er dann wiederrum von der abgezogen wird,
                    // wenn er rausgeht und es so ins minus rutschen würde
                    inventory.visual.Add(SOLDIER_JOBS[i]);
                    return std::make_unique<nofDefender>(pos, player, *this, i, attacker);
                }
                ++r;
            }
        }
    }

    // Kein Soldat gefunden, als letzten Hoffnung die Soldaten nehmen, die ggf in der Warteschlange noch hängen
    for(auto it = leave_house.begin(); it != leave_house.end(); ++it)
    {
        std::unique_ptr<nofSoldier> soldier;
        // Soldat?
        if((*it)->GetGOT() == GO_Type::NofAggressivedefender)
        {
            soldier = boost::static_pointer_cast<nofSoldier>(std::move(*it));
            static_cast<nofAggressiveDefender&>(*soldier).NeedForHomeDefence();
        } else if((*it)->GetGOT() == GO_Type::NofPassivesoldier)
            soldier = boost::static_pointer_cast<nofSoldier>(std::move(*it));
        else
            continue;

        leave_house.erase(it); // Only allowed in the loop as we return now
        soldier->Abrogate();

        auto defender = std::make_unique<nofDefender>(pos, player, *this, soldier->GetRank(), attacker);
        soldier->Destroy();
        return defender;
    }

    return nullptr;
}

bool nobBaseWarehouse::AreRecruitingConditionsComply()
{
    // Mindestanzahl der Gehilfen die vorhanden sein müssen anhand der 1. Militäreinstellung ausrechnen
    unsigned needed_helpers = 100 - 10 * world->GetPlayer(player).GetMilitarySetting(0);

    // einer muss natürlich mindestens vorhanden sein!
    if(!needed_helpers)
        needed_helpers = 1;

    // Wenn alle Bedingungen erfüllt sind, Event anmelden
    return (inventory[Job::Helper] >= needed_helpers && inventory[GoodType::Sword] && inventory[GoodType::ShieldRomans]
            && inventory[GoodType::Beer]);
}

void nobBaseWarehouse::TryRecruiting()
{
    // Wenn noch kein Event angemeldet wurde und alle Bedingungen erfüllt sind, kann ein neues angemeldet werden
    if(!recruiting_event)
    {
        if(AreRecruitingConditionsComply())
            recruiting_event = GetEvMgr().AddEvent(this, RECRUITE_GF + RANDOM_RAND(RECRUITE_RANDOM_GF), 2);
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
    GamePlayer& owner = world->GetPlayer(player);
    for(const auto i : helpers::enumRange<GoodType>())
    {
        if(!goods[i])
            continue;
        // Can only add canonical shields (romans)
        RTTR_Assert(i == GoodType::ShieldRomans || ConvertShields(i) != GoodType::ShieldRomans);

        inventory.Add(i, goods.goods[i]);
        if(addToPlayer)
            owner.IncreaseInventoryWare(i, goods.goods[i]);
        CheckUsesForNewWare(i);
    }

    for(const auto i : helpers::enumRange<Job>())
    {
        if(!goods.people[i])
            continue;
        // Boatcarriers are added as carriers and boat individually
        RTTR_Assert(i != Job::BoatCarrier);

        inventory.Add(i, goods.people[i]);
        if(addToPlayer)
            owner.IncreaseInventoryJob(i, goods.people[i]);
        CheckJobsForNewFigure(i);
    }
}

void nobBaseWarehouse::AddToInventory()
{
    GamePlayer& owner = world->GetPlayer(player);
    for(const auto i : helpers::enumRange<GoodType>())
        owner.IncreaseInventoryWare(i, inventory[i]);

    for(const auto i : helpers::enumRange<Job>())
        owner.IncreaseInventoryJob(i, inventory[i]);
}

bool nobBaseWarehouse::CanRecruit(const Job job) const
{
    if(const GoodType* requiredTool = JOB_CONSTS[job].tool.get_ptr())
    {
        // Do we have a helper and a tool (if required)?
        return inventory[Job::Helper] > 0 && (*requiredTool == GoodType::Nothing || inventory[*requiredTool] > 0);
    } else // Cannot recruit
        return false;
}

bool nobBaseWarehouse::TryRecruitJob(const Job job)
{
    RTTR_Assert(!helpers::contains(SOLDIER_JOBS, job) && job != Job::PackDonkey);
    if(!CanRecruit(job))
        return false;

    auto& owner = world->GetPlayer(player);

    const GoodType requiredTool = JOB_CONSTS[job].tool.get(); // Validity checked in CanRecruit
    if(requiredTool != GoodType::Nothing)
    {
        inventory.Remove(requiredTool);
        owner.DecreaseInventoryWare(requiredTool, 1);
    }

    inventory.Remove(Job::Helper);
    owner.DecreaseInventoryJob(Job::Helper, 1);

    inventory.Add(job);
    owner.IncreaseInventoryJob(job, 1);
    return true;
}

InventorySetting nobBaseWarehouse::GetInventorySettingVisual(const Job job) const
{
    return inventorySettingsVisual[(job == Job::BoatCarrier) ? Job::Helper : job];
}

InventorySetting nobBaseWarehouse::GetInventorySettingVisual(const GoodType ware) const
{
    return inventorySettingsVisual[ConvertShields(ware)];
}

InventorySetting nobBaseWarehouse::GetInventorySetting(const Job job) const
{
    return inventorySettings[(job == Job::BoatCarrier) ? Job::Helper : job];
}

InventorySetting nobBaseWarehouse::GetInventorySetting(const GoodType ware) const
{
    return inventorySettings[ConvertShields(ware)];
}

/// Verändert Ein/Auslagerungseinstellungen (visuell)
void nobBaseWarehouse::SetInventorySettingVisual(const boost_variant2<GoodType, Job>& what, InventorySetting state)
{
    state.MakeValid();
    visit([this, state](auto type) { inventorySettingsVisual[type] = state; }, what);

    NotifyListeners(1);
}

/// Verändert Ein/Auslagerungseinstellungen (real)
void nobBaseWarehouse::SetInventorySetting(const boost_variant2<GoodType, Job>& what, InventorySetting state)
{
    state.MakeValid();
    InventorySetting& selectedSetting =
      visit([this](auto type) -> InventorySetting& { return inventorySettings[type]; }, what);

    InventorySetting oldState = selectedSetting;
    selectedSetting = state;

    /// Bei anderen Spielern als dem lokalen, der das in Auftrag gegeben hat, müssen die visuellen ebenfalls
    /// geändert werden oder auch bei Replays
    if(GAMECLIENT.IsReplayModeOn() || GAMECLIENT.GetPlayerId() != player)
        SetInventorySettingVisual(what, state);

    if(holds_alternative<GoodType>(what) && oldState.IsSet(EInventorySetting::Stop)
       && !state.IsSet(EInventorySetting::Stop))
    {
        // Evtl gabs verlorene Waren, die jetzt in das HQ wieder reinkönnen
        world->GetPlayer(player).FindClientForLostWares();
    } // No else here!
    if(!oldState.IsSet(EInventorySetting::Send) && state.IsSet(EInventorySetting::Send))
    {
        // Sind Waren vorhanden, die ausgelagert werden müssen und ist noch kein Auslagerungsevent vorhanden --> neues
        // anmelden
        auto getWaresOrJobs = [this](auto type) { return inventory[type]; };
        if(!empty_event && visit(getWaresOrJobs, what))
            empty_event = GetEvMgr().AddEvent(this, empty_INTERVAL, 3);
    } else if(!oldState.IsSet(EInventorySetting::Collect) && state.IsSet(EInventorySetting::Collect))
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

    const unsigned numElements = isJob ? inventorySettings.people.size() : inventorySettings.goods.size();
    RTTR_Assert(states.size() == numElements);
    InventorySetting* settings = isJob ? inventorySettings.people.begin() : inventorySettings.goods.begin();

    for(InventorySetting state : states)
    {
        state.MakeValid();
        *(settings++) = state;
        isUnstopped |= !state.IsSet(EInventorySetting::Stop);
        isCollectSet |= state.IsSet(EInventorySetting::Collect);
    }

    // Evtl gabs verlorene Waren, die jetzt in das HQ wieder reinkönnen
    if(isUnstopped)
        world->GetPlayer(player).FindClientForLostWares();
    // no else!
    // Sind Waren vorhanden, die ausgelagert werden müssen und ist noch kein Auslagerungsevent vorhanden --> neues
    // anmelden
    if(AreWaresToEmpty() && !empty_event)
        empty_event = GetEvMgr().AddEvent(this, empty_INTERVAL, 3);
    // Sollen Waren eingelagert werden? Dann müssen wir neue bestellen
    if(isCollectSet && !store_event)
        store_event = GetEvMgr().AddEvent(this, STORE_INTERVAL, 4);
}

bool nobBaseWarehouse::IsWareDependent(const Ware& ware)
{
    return helpers::contains(dependent_wares, &ware);
}

bool nobBaseWarehouse::AreWaresToEmpty() const
{
    // Prüfen, ob Warentyp ausgelagert werden soll und ob noch Waren davon vorhanden sind
    // Waren überprüfen
    for(const auto i : helpers::enumRange<GoodType>())
    {
        if(GetInventorySetting(i).IsSet(EInventorySetting::Send) && inventory[i])
            return true;
    }

    // Figuren überprüfen
    for(const auto i : helpers::enumRange<Job>())
    {
        if(GetInventorySetting(i).IsSet(EInventorySetting::Send) && inventory[i])
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

void nobBaseWarehouse::SetReserveVisual(const unsigned rank, const unsigned count)
{
    reserve_soldiers_claimed_visual[rank] = count;
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
        CheckOuthousing(SOLDIER_JOBS[rank]);
        // Ggf. Truppen in die Militärgebäude schicken
        world->GetPlayer(player).RegulateAllTroops();
    }
    // ansonsten ists gleich und alles ist in Ordnung!
}

void nobBaseWarehouse::CheckOuthousing(const boost_variant2<GoodType, Job>& what)
{
    // Check if we need to send this ware or figure and register an event for this
    // If we already have an event, we don't need to do anything
    if(empty_event)
        return;

    const InventorySetting setting =
      visit(composeVisitor(
              [this](Job job) { // Bootsträger in Träger umwandeln, der evtl dann raus soll
                  return GetInventorySetting((job == Job::BoatCarrier) ? Job::Helper : job);
              },
              [this](GoodType good) { return GetInventorySetting(good); }),
            what);

    if(setting.IsSet(EInventorySetting::Send))
        empty_event = GetEvMgr().AddEvent(this, empty_INTERVAL, 3);
}

/// For debug only
bool nobBaseWarehouse::IsDependentFigure(const noFigure& fig) const
{
    return helpers::contains(dependent_figures, &fig);
}

/// Available goods of a specific type that can be used for trading
unsigned nobBaseWarehouse::GetAvailableWaresForTrading(const GoodType gt) const
{
    // We need a helper as leader
    if(!inventory[Job::Helper])
        return 0;

    return std::min(inventory[gt], inventory[Job::PackDonkey]);
}

/// Available figures of a speciefic type that can be used for trading
unsigned nobBaseWarehouse::GetAvailableFiguresForTrading(const Job job) const
{
    // We need a helper as leader
    if(!inventory[Job::Helper])
        return 0;

    if(job == Job::Helper)
        return (inventory[Job::Helper] - 1) / 2; // need one as leader
    else
        return std::min(inventory[job], inventory[Job::Helper] - 1);
}

/// Starts a trade caravane from this warehouse
void nobBaseWarehouse::StartTradeCaravane(const boost_variant2<GoodType, Job>& what, const unsigned count,
                                          const TradeRoute& tr, nobBaseWarehouse* goal)
{
    auto tlOwned = std::make_unique<nofTradeLeader>(pos, player, tr, this->GetPos(), goal->GetPos());
    auto& tl = *tlOwned;
    AddLeavingFigure(std::move(tlOwned));

    // Create the donkeys or other people
    nofTradeDonkey* last = nullptr;
    for(unsigned i = 0; i < count; ++i)
    {
        auto next = std::make_unique<nofTradeDonkey>(pos, player, what);

        if(last)
            last->SetSuccessor(next.get());
        else
            tl.SetSuccessor(next.get());

        last = next.get();
        AddLeavingFigure(std::move(next));
    }

    GamePlayer& owner = world->GetPlayer(player);
    // Remove leader
    inventory.real.Remove(Job::Helper);
    owner.DecreaseInventoryJob(Job::Helper, 1);

    // Also diminish the count of donkeys
    visit(composeVisitor(
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
                inventory.real.Remove(Job::PackDonkey, count);
                owner.DecreaseInventoryJob(Job::PackDonkey, count);
            }),
          what);
}
