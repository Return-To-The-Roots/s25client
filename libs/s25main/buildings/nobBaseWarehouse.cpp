// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nobBaseWarehouse.h"
#include "BurnedWarehouse.h"
#include "EventManager.h"
#include "FindWhConditions.h"
#include "GamePlayer.h"
#include "GlobalGameSettings.h"
#include "LeatherLoader.h"
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
    producinghelpers_event = getEvMgr().addEvent(this, PRODUCE_HELPERS_GF + RANDOM_RAND(PRODUCE_HELPERS_RANDOM_GF), 1);
    // Reserve nullen
    reserve_soldiers_available.fill(0);
    reserve_soldiers_available_with_armor.fill(0);
    reserve_soldiers_claimed_visual.fill(0);
    reserve_soldiers_claimed_real.fill(0);
}

nobBaseWarehouse::~nobBaseWarehouse() = default;

void nobBaseWarehouse::DestroyBuilding()
{
    // Den Waren und Figuren Bescheid sagen, die zu uns auf den Weg sind, dass wir nun nicht mehr existieren
    for(noFigure* dependentFigure : dependent_figures)
    {
        // Only send figures home that are not already at this position.
        if(dependentFigure->getPos() == getPos())
        {
            dependentFigure->setGoalTonullptr();
            dependentFigure->cutCurrentRoad();
            dependentFigure->startWandering();
        } else
            dependentFigure->goHome();
    }
    dependent_figures.clear();

    for(Ware* dependentWare : dependent_wares)
        wareNotNeeded(dependentWare);
    dependent_wares.clear();

    // ggf. Events abmelden
    getEvMgr().removeEvent(recruiting_event);
    getEvMgr().removeEvent(producinghelpers_event);
    getEvMgr().removeEvent(empty_event);
    getEvMgr().removeEvent(store_event);

    // Waiting Wares löschen
    for(auto& waitingWare : waiting_wares)
    {
        waitingWare->wareLost(player);
        waitingWare->destroy();
    }
    waiting_wares.clear();

    // restliche Warenbestände von der Inventur wieder abziehen
    for(const auto good : helpers::enumRange<GoodType>())
        world->getPlayer(player).decreaseInventoryWare(good, inventory[good]);

    // move soldiers from reserve to inventory.
    for(unsigned rank = 0; rank < world->getGGS().getMaxMilitaryRank(); ++rank)
    {
        if(reserve_soldiers_available[rank] > 0)
        {
            inventory.real.add(SOLDIER_JOBS[rank], reserve_soldiers_available[rank]);
            inventory.real.add(jobEnumToAmoredSoldierEnum(SOLDIER_JOBS[rank]),
                               reserve_soldiers_available_with_armor[rank]);
        }
    }
    // Objekt, das die flüchtenden Leute nach und nach ausspuckt, erzeugen
    world->addFigure(pos, std::make_unique<BurnedWarehouse>(pos, player, inventory.real.peopleOnly()));

    nobBaseMilitary::DestroyBuilding();
}

void nobBaseWarehouse::serialize(SerializedGameData& sgd) const
{
    nobBaseMilitary::serialize(sgd);

    sgd.pushObjectContainer(waiting_wares, true);
    sgd.pushBool(fetch_double_protection);
    sgd.pushObjectContainer(dependent_figures);
    sgd.pushObjectContainer(dependent_wares, true);
    sgd.pushEvent(producinghelpers_event);
    sgd.pushEvent(recruiting_event);
    sgd.pushEvent(empty_event);
    sgd.pushEvent(store_event);

    for(unsigned i = 0; i < 5; ++i)
    {
        // Nur das Reale, nicht das visuelle speichern, das wäre sinnlos!, beim Laden ist das visuelle = realem
        sgd.pushUnsignedInt(reserve_soldiers_available[i]);
        sgd.pushUnsignedInt(reserve_soldiers_available_with_armor[i]);
        sgd.pushUnsignedInt(reserve_soldiers_claimed_real[i]);
    }

    for(const auto i : helpers::enumRange<GoodType>())
    {
        sgd.pushUnsignedInt(inventory.visual[i]);
        sgd.pushUnsignedInt(inventory.real[i]);
        sgd.pushUnsignedChar(static_cast<uint8_t>(inventorySettings[i]));
    }
    for(const auto i : helpers::enumRange<Job>())
    {
        sgd.pushUnsignedInt(inventory.visual[i]);
        sgd.pushUnsignedInt(inventory.real[i]);
        sgd.pushUnsignedChar(static_cast<uint8_t>(inventorySettings[i]));
    }

    helpers::pushContainer(sgd, inventory.visual.armoredSoldiers);
    helpers::pushContainer(sgd, inventory.real.armoredSoldiers);
}

nobBaseWarehouse::nobBaseWarehouse(SerializedGameData& sgd, const unsigned obj_id) : nobBaseMilitary(sgd, obj_id)
{
    sgd.popObjectContainer(waiting_wares, GO_Type::Ware);
    fetch_double_protection = sgd.popBool();
    sgd.popObjectContainer(dependent_figures);
    sgd.popObjectContainer(dependent_wares, GO_Type::Ware);

    producinghelpers_event = sgd.popEvent();
    recruiting_event = sgd.popEvent();
    empty_event = sgd.popEvent();
    store_event = sgd.popEvent();

    reserve_soldiers_available_with_armor.fill(0);
    for(unsigned i = 0; i < 5; ++i)
    {
        reserve_soldiers_available[i] = sgd.popUnsignedInt();
        if(sgd.getGameDataVersion() >= 12)
            reserve_soldiers_available_with_armor[i] = sgd.popUnsignedInt();

        reserve_soldiers_claimed_visual[i] = reserve_soldiers_claimed_real[i] = sgd.popUnsignedInt();
    }

    for(const auto i : helpers::enumRange<GoodType>())
    {
        if(sgd.getGameDataVersion() < 11 && wineaddon::isWineAddonGoodType(i))
            continue;

        if(sgd.getGameDataVersion() < 12 && leatheraddon::isLeatherAddonGoodType(i))
            continue;

        inventory.visual.add(i, sgd.popUnsignedInt());
        inventory.real.add(i, sgd.popUnsignedInt());
        inventorySettings[i] = inventorySettingsVisual[i] = static_cast<InventorySetting>(sgd.popUnsignedChar());
    }
    for(const auto i : helpers::enumRange<Job>())
    {
        if(sgd.getGameDataVersion() < 11 && wineaddon::isWineAddonJobType(i))
            continue;

        if(sgd.getGameDataVersion() < 12 && leatheraddon::isLeatherAddonJobType(i))
            continue;

        inventory.visual.add(i, sgd.popUnsignedInt());
        inventory.real.add(i, sgd.popUnsignedInt());
        inventorySettings[i] = inventorySettingsVisual[i] = static_cast<InventorySetting>(sgd.popUnsignedChar());
    }
    if(sgd.getGameDataVersion() >= 12)
    {
        helpers::popContainer(sgd, inventory.visual.armoredSoldiers);
        helpers::popContainer(sgd, inventory.real.armoredSoldiers);
    }
}

void nobBaseWarehouse::clear()
{
    // Add reserve soldiers back
    for(unsigned i = 0; i < reserve_soldiers_available.size(); i++)
    {
        inventory.add(SOLDIER_JOBS[i], reserve_soldiers_available[i]);
        inventory.add(jobEnumToAmoredSoldierEnum(SOLDIER_JOBS[i]), reserve_soldiers_available_with_armor[i]);
    }
    reserve_soldiers_available.fill(0);
    reserve_soldiers_available_with_armor.fill(0);

    GamePlayer& owner = world->getPlayer(player);
    for(const auto i : helpers::enumRange<GoodType>())
        owner.decreaseInventoryWare(i, inventory[i]);

    for(const auto i : helpers::enumRange<Job>())
        owner.decreaseInventoryJob(i, inventory[i]);

    for(const auto i : helpers::enumRange<ArmoredSoldier>())
        owner.decreaseInventoryJob(i, inventory[i]);

    inventory.clear();

    for(auto& waitingWare : waiting_wares)
    {
        waitingWare->wareLost(player);
        waitingWare->destroy();
    }
    waiting_wares.clear();
}

void nobBaseWarehouse::orderCarrier(noRoadNode& goal, RoadSegment& workplace)
{
    RTTR_Assert(workplace.getCarrier(0) == nullptr);
    const bool isBoatRequired = workplace.getRoadType() == RoadType::Water;

    // We assume, that the caller already checked, if this is possible
    RTTR_Assert(inventory[Job::Helper]);
    if(isBoatRequired)
        RTTR_Assert(inventory[GoodType::Boat]);

    std::unique_ptr<noFigure> carrier = std::make_unique<nofCarrier>(
      isBoatRequired ? CarrierType::Boat : CarrierType::Normal, pos, player, &workplace, &goal);
    workplace.setCarrier(0, static_cast<nofCarrier*>(carrier.get()));

    if(!useFigureAtOnce(carrier, goal))
        addLeavingFigure(std::move(carrier));

    inventory.real.remove(Job::Helper);
    if(isBoatRequired)
        inventory.real.remove(GoodType::Boat);

    // Evtl. kein Gehilfe mehr, sodass das Rekrutieren gestoppt werden muss
    tryStopRecruiting();
}

bool nobBaseWarehouse::orderJob(const Job job, noRoadNode& goal, const bool allow_recruiting)
{
    // Maybe we have to recruit one
    if(!inventory[job])
    {
        if(!allow_recruiting || !tryRecruitJob(job))
            return false;
    }

    std::unique_ptr<noFigure> fig = JobFactory::createJob(job, pos, player, goal);
    if(isSoldier(fig->getJobType()))
    {
        if(inventory.real[jobEnumToAmoredSoldierEnum(job)] > 0)
        {
            inventory.real.remove(jobEnumToAmoredSoldierEnum(job));
            fig->setArmor(true);
        }
    }

    // Ziel Bescheid sagen, dass dortin ein neuer Arbeiter kommt (bei Flaggen als das anders machen)
    if(goal.getType() != NodalObjectType::Flag)
        checkedCast<noBaseBuilding>(goal).gotWorker(job, *fig);

    // Wenn Figur nicht sofort von abgeleiteter Klasse verwenet wird, fügen wir die zur Leave-Liste hinzu
    if(!useFigureAtOnce(fig, goal))
        addLeavingFigure(std::move(fig));

    inventory.real.remove(job);

    // Evtl. kein Gehilfe mehr da, sodass das Rekrutieren gestoppt werden muss
    tryStopRecruiting();

    return true;
}

nofCarrier* nobBaseWarehouse::orderDonkey(RoadSegment& road, noRoadNode& goal_flag)
{
    // Überhaupt ein Esel vorhanden?
    if(!inventory[Job::PackDonkey])
        return nullptr;

    auto donkey = std::make_unique<nofCarrier>(CarrierType::Donkey, pos, player, &road, &goal_flag);
    nofCarrier* donkeyRef = donkey.get();
    addLeavingFigure(std::move(donkey));
    inventory.real.remove(Job::PackDonkey);

    return donkeyRef;
}

void nobBaseWarehouse::handleBaseEvent(const unsigned id)
{
    switch(id)
    {
        case 0:
            leaving_event = nullptr;
            handleLeaveEvent();
            break;
        case 1:
            producinghelpers_event = nullptr;
            handleProduceHelperEvent();
            break;
        case 2:
            recruiting_event = nullptr;
            handleRecrutingEvent();
            break;
        case 3:
            empty_event = nullptr;
            handleSendoutEvent();
            break;
        case 4:
            store_event = nullptr;
            handleCollectEvent();
            break;
    }
}

void nobBaseWarehouse::handleCollectEvent()
{
    // Storing wares done?
    bool storingDone = false;
    // Is storing still wanted?
    bool storingWanted = false;

    // Untersuchen, welche Waren und Figuren eingelagert werden sollen
    for(const auto i : helpers::enumRange<GoodType>())
    {
        // Soll Ware eingeliefert werden?
        if(!getInventorySetting(i).isSet(EInventorySetting::Collect))
            continue;

        storingWanted = true;

        // Lagerhaus suchen, das diese Ware enthält
        nobBaseWarehouse* wh = world->getPlayer(player).findWarehouse(*this, FW::HasWareButNoCollect(i), false, false);
        // Gefunden?
        if(wh)
        {
            // Dann bestellen
            Ware* ware = wh->orderWare(i, *this);
            if(ware)
            {
                RTTR_Assert(isWareDependent(*ware));
                storingDone = true;
                break;
            }
        }
    }

    // Menschen "bestellen" wenn noch keine Ware bestellt wurde
    if(!storingDone)
    {
        for(const auto i : helpers::enumRange<Job>())
        {
            // Soll dieser Typ von Mensch bestellt werden?
            if(!getInventorySetting(i).isSet(EInventorySetting::Collect))
                continue;

            storingWanted = true;

            // Lagerhaus suchen, das diesen Job enthält
            nobBaseWarehouse* wh =
              world->getPlayer(player).findWarehouse(*this, FW::HasFigureButNoCollect(i, false), false, false);
            // Gefunden?
            if(wh)
            {
                // Dann bestellen
                if(wh->orderJob(i, *this, false))
                    break;
            }
        }
    }

    // Storing still wanted?
    // Then continue ordering new stuff
    if(storingWanted)
        store_event = getEvMgr().addEvent(this, STORE_INTERVAL, 4);
}

void nobBaseWarehouse::handleSendoutEvent()
{
    // Fight or something in front of the house? Try again later!
    if(!world->isRoadNodeForFigures(world->getNeighbour(pos, Direction::SouthEast)))
    {
        empty_event = getEvMgr().addEvent(this, empty_INTERVAL, 3);
        return;
    }

    std::vector<boost_variant2<GoodType, Job>> possibleTypes;
    // Waren und Figuren zum Auslagern zusammensuchen
    // Wenn keine Platz an Flagge, dann keine Waren raus
    if(getFlag()->hasSpaceForWare())
    {
        for(const auto i : helpers::enumRange<GoodType>())
        {
            if(getInventorySetting(i).isSet(EInventorySetting::Send) && inventory[i])
                possibleTypes.push_back(i);
        }
    }

    for(const auto i : helpers::enumRange<Job>())
    {
        // Figuren, die noch nicht implementiert sind, nicht nehmen!
        if(getInventorySetting(i).isSet(EInventorySetting::Send) && inventory[i])
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
        noBaseBuilding* wareGoal = world->getPlayer(player).findClientForWare(*ware);
        if(wareGoal != this)
        {
            ware->setGoal(wareGoal);

            // Ware zur Liste hinzufügen, damit sie dann rausgetragen wird
            waiting_wares.push_back(std::move(ware));

            addLeavingEvent();

            // Ware aus Inventar entfernen
            inventory.real.remove(goodType);

            // Evtl. kein Schwert/Schild/Bier mehr da, sodass das Rekrutieren gestoppt werden muss
            tryStopRecruiting();
        } else
            world->getPlayer(player).removeWare(*ware);
    } else
    {
        const auto jobType = get<Job>(selectedId);
        nobBaseWarehouse* wh =
          world->getPlayer(player).findWarehouse(*this, FW::AcceptsFigureButNoSend(jobType), true, false);
        if(wh != this)
        {
            auto fig = std::make_unique<nofPassiveWorker>(jobType, pos, player, nullptr);

            if(isSoldier(jobType) && inventory.real[jobEnumToAmoredSoldierEnum(jobType)] > 0)
            {
                fig->setArmor(true);
                inventory.real.remove(jobEnumToAmoredSoldierEnum(jobType));
            }

            // If no warehouse found, leave the figure with its default state (GotToGoal with no goal)
            // When it leaves the building it will start wandering.
            if(wh)
                fig->goHome(wh);

            addLeavingFigure(std::move(fig));

            // Person aus Inventar entfernen
            inventory.real.remove(jobType);

            // Evtl. kein Gehilfe mehr da, sodass das Rekrutieren gestoppt werden muss
            tryStopRecruiting();
        }
    }

    // Weitere Waren/Figuren zum Auslagern?
    if(areWaresToEmpty())
        // --> Nächstes Event
        empty_event = getEvMgr().addEvent(this, empty_INTERVAL, 3);
}

void nobBaseWarehouse::handleRecrutingEvent()
{
    // Wir wollen so viele der Soldaten rekrutieren,
    // wie in den military_settings angegeben.
    // Wird evtl gerundet, dann fair nach Zufall ;) ).

    unsigned max_recruits;
    max_recruits = std::min(inventory[GoodType::Sword], inventory[GoodType::ShieldRomans]);
    max_recruits = std::min(inventory[GoodType::Beer], max_recruits);
    max_recruits = std::min(inventory[Job::Helper], max_recruits);

    GamePlayer& owner = world->getPlayer(player);
    const unsigned recruiting_ratio = owner.getMilitarySetting(0);
    unsigned realRecruits = max_recruits * recruiting_ratio / MILITARY_SETTINGS_SCALE[0];
    // Wurde abgerundet?
    unsigned remainingRecruits = realRecruits * recruiting_ratio % MILITARY_SETTINGS_SCALE[0];
    if(remainingRecruits != 0 && unsigned(RANDOM_RAND(MILITARY_SETTINGS_SCALE[0] - 1)) < remainingRecruits)
        ++realRecruits;
    else if(realRecruits == 0)
        return; // Nothing to do

    inventory.add(Job::Private, realRecruits);
    owner.increaseInventoryJob(Job::Private, realRecruits);

    inventory.remove(Job::Helper, realRecruits);
    owner.decreaseInventoryJob(Job::Helper, realRecruits);

    inventory.remove(GoodType::Sword, realRecruits);
    owner.decreaseInventoryWare(GoodType::Sword, realRecruits);

    inventory.remove(GoodType::ShieldRomans, realRecruits);
    owner.decreaseInventoryWare(GoodType::ShieldRomans, realRecruits);

    inventory.remove(GoodType::Beer, realRecruits);
    owner.decreaseInventoryWare(GoodType::Beer, realRecruits);

    // Evtl. versuchen nächsten zu rekrutieren
    tryRecruiting();

    // If there were no soldiers before
    if(inventory[Job::Private] == realRecruits)
    {
        // Check reserve
        this->refreshReserve(0);
        // And check if we need the new ones (if any left) e.g. for military buildings
        if(inventory[Job::Private] > 0)
            owner.newSoldiersAvailable(inventory[Job::Private]);
    }
}

void nobBaseWarehouse::handleProduceHelperEvent()
{
    // Nur bei unter 100 Trägern, weitere "produzieren"
    if(inventory[Job::Helper] < 100)
    {
        inventory.add(Job::Helper);

        GamePlayer& owner = world->getPlayer(player);
        owner.increaseInventoryJob(Job::Helper, 1);

        if(inventory[Job::Helper] == 1)
        {
            // Wenn vorher keine Träger da waren, müssen alle unbesetzen Wege gucken, ob sie nen Weg hierher finden,
            // könnte ja sein, dass vorher nich genug Träger da waren
            owner.findCarrierForAllRoads();
            // evtl Träger mit Werkzeug kombiniert -> neuer Beruf
            owner.findWarehouseForAllJobs();
        }
    } else if(inventory[Job::Helper] > 100)
    {
        // Bei Überbevölkerung Träger vernichten
        inventory.remove(Job::Helper);

        world->getPlayer(player).decreaseInventoryJob(Job::Helper, 1);
    }

    producinghelpers_event = getEvMgr().addEvent(this, PRODUCE_HELPERS_GF + RANDOM_RAND(PRODUCE_HELPERS_RANDOM_GF), 1);

    // Evtl. genau der Gehilfe, der zum Rekrutieren notwendig ist
    tryRecruiting();

    // Evtl die Typen gleich wieder auslagern, falls erforderlich
    checkOuthousing(Job::Helper);
}

void nobBaseWarehouse::handleLeaveEvent()
{
#if RTTR_ENABLE_ASSERTS
    // Harbors have more queues. Ignore for now
    if(getGOT() != GO_Type::NobHarborbuilding)
    {
        Inventory should = inventory.real;
        for(auto& it : leave_house)
        {
            // Don't count warehouse workers
            if(!it->memberOfWarehouse())
            {
                if(it->getJobType() == Job::BoatCarrier)
                    should.add(Job::Helper);
                else
                    should.add(it->getJobType());
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
    if(!world->isRoadNodeForFigures(world->getNeighbour(pos, Direction::SouthEast)))
    {
        // there's a fight

        // try to find a defender
        const auto it = helpers::findIf(leave_house, [](const auto& sld) {
            return sld->getGOT() == GO_Type::NofAggressivedefender || sld->getGOT() == GO_Type::NofDefender;
        });
        // no defender found? enqueue next leaving event
        if(it == leave_house.end())
        {
            go_out = false;
            addLeavingEvent();
            return;
        }
        // Move defender to front to make him leave first
        leave_house.push_front(std::move(*it));
        leave_house.erase(it);
    }

    // Figures first, then wares
    if(!leave_house.empty())
    {
        noFigure& fig = world->addFigure(pos, std::move(leave_house.front()));
        leave_house.pop_front();

        // Init road walking for figures walking on roads
        if(fig.isWalkingOnRoad())
            fig.initializeRoadWalking(getRoute(Direction::SouthEast), 0, true);

        figureLeft(fig);
        fig.actAtFirst();
    } else
    {
        if(getFlag()->hasSpaceForWare())
        {
            // Send first ware
            auto ware = std::move(waiting_wares.front());
            waiting_wares.pop_front();
            inventory.visual.remove(convertShields(ware->type));
            ware->carry(*getFlag());
            world->addFigure(pos, std::make_unique<nofWarehouseWorker>(pos, player, std::move(ware), false))
              .walkToGoal();
        } else
        {
            // No space and no figures (checked above)
            go_out = false;
        }
    }

    // Nothing left waiting -> stop
    if(leave_house.empty() && waiting_wares.empty())
        go_out = false;

    if(go_out)
        leaving_event = getEvMgr().addEvent(this, LEAVE_INTERVAL + RANDOM_RAND(LEAVE_INTERVAL_RAND));
}

void nobBaseWarehouse::figureLeft(const noFigure& fig)
{
    // Adapt inventory for non-warehouse workers
    if(fig.memberOfWarehouse())
        return;
    removeFromVisualInventory(fig);

    if(fig.getGOT() == GO_Type::NofTradedonkey)
    {
        // Trade donkey carrying wares?
        const auto& carriedWare = static_cast<const nofTradeDonkey&>(fig).getCarriedWare();
        if(carriedWare)
            inventory.visual.remove(*carriedWare);
    }
}

Ware* nobBaseWarehouse::orderWare(const GoodType good, noBaseBuilding& goal)
{
    // Ware überhaupt hier vorhanden (Abfrage eigentlich nicht nötig, aber erstmal zur Sicherheit)
    if(!inventory[good])
    {
        LOG.write("nobBaseWarehouse::OrderWare: WARNING: No ware type %u in warehouse!\n")
          % static_cast<unsigned>(good);
        return nullptr;
    }

    auto ware = std::make_unique<Ware>(good, &goal, this);
    inventory.remove(good);

    // Copy pointer so functions below can take ownership
    Ware* wareRef = ware.get();

    // If we don't want to use the ware right away we add it to the waiting wares
    if(!useWareAtOnce(ware, goal))
        addWaitingWare(std::move(ware));
    RTTR_Assert(!ware);

    // Evtl. keine Waffen/Bier mehr da, sodass das Rekrutieren gestoppt werden muss
    tryStopRecruiting();

    return wareRef;
}

void nobBaseWarehouse::addWaitingWare(std::unique_ptr<Ware> ware)
{
    inventory.visual.add(convertShields(ware->type));
    ware->waitInWarehouse(*this);
    waiting_wares.push_back(std::move(ware));
    addLeavingEvent();
}

bool nobBaseWarehouse::freePlaceAtFlag()
{
    if(!waiting_wares.empty())
    {
        addLeavingEvent();
        return true;
    } else
    {
        // Evtl. war die Flagge voll und das Auslagern musste gestoppt werden
        // Weitere Waren/Figuren zum Auslagern und kein Event angemeldet?
        if(areWaresToEmpty() && !empty_event)
            // --> Nächstes Event
            empty_event = getEvMgr().addEvent(this, empty_INTERVAL, 3);

        return false;
    }
}

void nobBaseWarehouse::addWare(std::unique_ptr<Ware> ware)
{
    // Ware not dependent anymore (only if we had a goal)
    if(ware->getGoal())
    {
        RTTR_Assert(ware->getGoal() == this); // The goal should be here
        removeDependentWare(*ware);
    } else
        RTTR_Assert(!isWareDependent(*ware));

    // Die Schilde der verschiedenen Nation in eine "Schild-Sorte" (den der Römer) umwandeln!
    GoodType type = convertShields(ware->type);

    world->getPlayer(player).removeWare(*ware);

    inventory.add(type);

    checkUsesForNewWare(type);
}

void nobBaseWarehouse::checkUsesForNewWare(const GoodType gt)
{
    // Wenn es ein Werkzeug war, evtl neuen Job suchen, der jetzt erzeugt werden könnte..
    if(gt >= GoodType::Tongs && gt <= GoodType::Boat)
    {
        for(const auto job : helpers::EnumRange<Job>{})
        {
            if(JOB_CONSTS[job].tool == gt)
                world->getPlayer(player).findWarehouseForAllJobs(job);
        }
    }

    // Wars Baumaterial? Dann den Baustellen Bescheid sagen
    if(gt == GoodType::Boards || gt == GoodType::Stones)
        world->getPlayer(player).findMaterialForBuildingSites();

    // Evtl wurden Bier oder Waffen reingetragen --> versuchen zu rekrutieren
    tryRecruiting();

    // Evtl die Ware gleich wieder auslagern, falls erforderlich
    checkOuthousing(gt);
}

void nobBaseWarehouse::checkJobsForNewFigure(const Job job)
{
    // Evtl ging ein Gehilfe rein --> versuchen zu rekrutieren
    if(job == Job::Helper)
        tryRecruiting();

    if(isSoldier(job))
    {
        // Reserve prüfen
        refreshReserve(getSoldierRank(job));
        if(inventory[job] > 0)
        {
            // Truppen prüfen in allen Häusern
            world->getPlayer(player).newSoldiersAvailable(inventory[job]);
        }
    } else
    {
        if(job == Job::PackDonkey)
        {
            // Straße für Esel suchen
            noRoadNode* goal;
            if(RoadSegment* road = world->getPlayer(player).findRoadForDonkey(*this, &goal))
                road->gotDonkey(orderDonkey(*road, *goal));
        } else
        {
            // Evtl. Abnehmer für die Figur wieder finden
            GamePlayer& owner = world->getPlayer(player);
            owner.findWarehouseForAllJobs(job);
            // Wenns ein Träger war, auch Wege prüfen
            if(job == Job::Helper && inventory[Job::Helper] == 1)
            {
                // evtl als Träger auf Straßen schicken
                owner.findCarrierForAllRoads();
                // evtl Träger mit Werkzeug kombiniert -> neuer Beruf
                owner.findWarehouseForAllJobs();
            }
        }
    }

    // Evtl den Typen gleich wieder auslagern, falls erforderlich
    checkOuthousing(job);
}

void nobBaseWarehouse::addFigure(std::unique_ptr<noFigure> figure, const bool increaseVisualCounts)
{
    // Warenhausarbeiter werden nicht gezählt!
    if(!figure->memberOfWarehouse())
    {
        addToRealInventory(*figure);
        if(increaseVisualCounts)
            addToVisualInventory(*figure);
    }

    // Check if we were actually waiting for this figure or if it was just added (e.g. builder that constructed it) to
    // not confuse implementations of Remove...
    if(isDependentFigure(*figure))
        removeDependentFigure(*figure);

    checkJobsForNewFigure(figure->getJobType());
    getEvMgr().addToKillList(std::move(figure));
}

void nobBaseWarehouse::removeFromVisualInventory(const noFigure& figure)
{
    const auto job = figure.getJobType();
    if(job == Job::BoatCarrier)
    {
        // Remove helper and boat separately
        inventory.visual.remove(Job::Helper);
        inventory.visual.remove(GoodType::Boat);
    } else
    {
        inventory.visual.remove(job);
        if(isSoldier(job) && figure.hasArmor())
            inventory.visual.remove(jobEnumToAmoredSoldierEnum(job));
    }
}

static void addFigureToInventory(Inventory& inventory, const noFigure& figure)
{
    const auto job = figure.getJobType();
    if(job == Job::BoatCarrier)
    {
        inventory.add(Job::Helper);
        inventory.add(GoodType::Boat);
    } else
    {
        inventory.add(job);
        if(isSoldier(job) && figure.hasArmor())
            inventory.add(jobEnumToAmoredSoldierEnum(job));
    }
}

void nobBaseWarehouse::addToVisualInventory(const noFigure& figure)
{
    addFigureToInventory(inventory.visual, figure);
}

void nobBaseWarehouse::addToRealInventory(const noFigure& figure)
{
    addFigureToInventory(inventory.real, figure);
}

void nobBaseWarehouse::fetchWare()
{
    if(!fetch_double_protection)
        addLeavingFigure(std::make_unique<nofWarehouseWorker>(pos, player, nullptr, true));

    fetch_double_protection = false;
}

void nobBaseWarehouse::wareLost(Ware& ware)
{
    removeDependentWare(ware);
}

void nobBaseWarehouse::cancelWare(Ware*& ware)
{
    inventory.real.add(convertShields(ware->type));
    helpers::extractPtr(waiting_wares, ware);
    ware = nullptr;
}

void nobBaseWarehouse::cancelFigure(noFigure* figure)
{
    auto it = helpers::findPtr(leave_house, figure);
    RTTR_Assert(it != leave_house.end());

    // Figure aus den Waiting-Wares entfernen
    addFigure(std::move(*it), false);
    leave_house.erase(it);
}

void nobBaseWarehouse::takeWare(Ware* ware)
{
    // Ware zur Abhängigkeitsliste hinzufügen, damit sie benachrichtigt wird, wenn dieses Lagerhaus zerstört wird
    RTTR_Assert(!helpers::contains(dependent_wares, ware));
    dependent_wares.push_back(ware);
}

void nobBaseWarehouse::orderTroops(nobMilitary& goal, std::array<unsigned, NUM_SOLDIER_RANKS>& counts, unsigned& max)
{
    unsigned start, limit;
    int step;

    if(world->getPlayer(player).getMilitarySetting(1) >= MILITARY_SETTINGS_SCALE[1] / 2)
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
            auto soldier = std::make_unique<nofPassiveSoldier>(pos, player, &goal, &goal, i - 1);
            inventory.real.remove(curRank);
            if(inventory.real[jobEnumToAmoredSoldierEnum(curRank)] > 0)
            {
                inventory.real.remove(jobEnumToAmoredSoldierEnum(curRank));
                soldier->setArmor(true);
            }
            goal.gotWorker(curRank, *soldier);
            addLeavingFigure(std::move(soldier));
            --max;
            --counts[i - 1];
        }
    }
}

nofAggressiveDefender* nobBaseWarehouse::sendAggressiveDefender(nofAttacker& attacker)
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
    inventory.real.remove(SOLDIER_JOBS[rank - 1]);
    if(inventory.real[jobEnumToAmoredSoldierEnum(soldier->getJobType())] > 0)
    {
        soldier->setArmor(true);
        inventory.real.remove(jobEnumToAmoredSoldierEnum(soldier->getJobType()));
    }
    addLeavingFigure(std::move(soldier));

    troops_on_mission.push_back(&soldierRef);

    return &soldierRef;
}

void nobBaseWarehouse::soldierLost(nofSoldier* soldier)
{
    // Soldat konnte nicht (mehr) kommen --> rauswerfen
    RTTR_Assert(dynamic_cast<nofActiveSoldier*>(soldier));
    RTTR_Assert(helpers::contains(troops_on_mission, static_cast<nofActiveSoldier*>(soldier)));
    troops_on_mission.remove(static_cast<nofActiveSoldier*>(soldier));
}

void nobBaseWarehouse::addActiveSoldier(std::unique_ptr<nofActiveSoldier> soldier)
{
    inventory.add(soldier->getJobType());
    if(soldier->hasArmor())
        inventory.add(jobEnumToAmoredSoldierEnum(soldier->getJobType()));

    // Evtl. geht der Soldat wieder in die Reserve
    refreshReserve(soldier->getRank());

    // Truppen prüfen in allen Häusern
    world->getPlayer(player).regulateAllTroops();

    // Returned home
    if(soldier.get() == defender_)
        resetDefender();
    else
    {
        // Ggf. war er auf Mission
        RTTR_Assert(helpers::contains(troops_on_mission, soldier.get()));
        troops_on_mission.remove(soldier.get());
    }

    // und Soldat vernichten
    soldier->resetHome();
    getEvMgr().addToKillList(std::move(soldier));
}

std::unique_ptr<nofDefender> nobBaseWarehouse::provideDefender(nofAttacker& attacker)
{
    // Ränge zählen
    unsigned rankCount = 0;

    for(unsigned i = 0; i < SOLDIER_JOBS.size(); ++i)
    {
        if(inventory[SOLDIER_JOBS[i]] || reserve_soldiers_available[i])
            ++rankCount;
    }

    if(rankCount)
    {
        // Gewünschten Rang an Hand der Militäreinstellungen ausrechnen, je nachdem wie stark verteidigt werden soll
        unsigned rank = (rankCount - 1) * world->getPlayer(player).getMilitarySetting(1) / MILITARY_SETTINGS_SCALE[1];

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
                    auto defender = std::make_unique<nofDefender>(pos, player, *this, i, attacker);
                    inventory.real.remove(SOLDIER_JOBS[i]);
                    if(inventory.real[jobEnumToAmoredSoldierEnum(SOLDIER_JOBS[i])] > 0)
                    {
                        defender->setArmor(true);
                        inventory.real.remove(jobEnumToAmoredSoldierEnum(SOLDIER_JOBS[i]));
                    }
                    return defender;
                }
                ++r;
            } else if(reserve_soldiers_available[i])
            {
                if(r == rank)
                {
                    // diesen Soldaten wollen wir
                    auto defender = std::make_unique<nofDefender>(pos, player, *this, i, attacker);
                    --reserve_soldiers_available[i];
                    // bei der visuellen Warenanzahl wieder hinzufügen, da er dann wiederrum von der abgezogen wird,
                    // wenn er rausgeht und es so ins minus rutschen würde
                    inventory.visual.add(SOLDIER_JOBS[i]);
                    if(reserve_soldiers_available_with_armor[i] > 0)
                    {
                        defender->setArmor(true);
                        --reserve_soldiers_available_with_armor[i];
                        inventory.visual.add(jobEnumToAmoredSoldierEnum(SOLDIER_JOBS[i]));
                    }
                    return defender;
                }
                ++r;
            }
        }
    }

    // If no soldier was found take a soldier that is about to leave (to another building)
    for(auto it = leave_house.begin(); it != leave_house.end(); ++it)
    {
        if((*it)->getGOT() == GO_Type::NofPassivesoldier)
        {
            std::unique_ptr<nofSoldier> soldier = boost::static_pointer_cast<nofSoldier>(std::move(*it));

            leave_house.erase(it); // Only allowed in the loop as we return now
            soldier->abrogate();

            auto defender = std::make_unique<nofDefender>(pos, player, *this, soldier->getRank(), attacker);
            defender->setArmor(soldier->hasArmor());
            soldier->destroy();
            return defender;
        }
    }
    return nullptr;
}

bool nobBaseWarehouse::canRecruitSoldiers()
{
    // Minimal number of helpers required according to current setting, at least 1
    const unsigned needed_helpers = std::max(1, 100 - 10 * world->getPlayer(player).getMilitarySetting(0));
    return (inventory[Job::Helper] >= needed_helpers && inventory[GoodType::Sword] && inventory[GoodType::ShieldRomans]
            && inventory[GoodType::Beer]);
}

void nobBaseWarehouse::tryRecruiting()
{
    if(!recruiting_event && canRecruitSoldiers())
        recruiting_event = getEvMgr().addEvent(this, RECRUITE_GF + RANDOM_RAND(RECRUITE_RANDOM_GF), 2);
}

void nobBaseWarehouse::tryStopRecruiting()
{
    // Remove existing event if we have not enough recruits/wares
    if(recruiting_event && !canRecruitSoldiers())
        getEvMgr().removeEvent(recruiting_event);
}

const Inventory& nobBaseWarehouse::getInventory() const
{
    return inventory.visual;
}

void nobBaseWarehouse::addToInventory(const GoodsAndPeopleCounts& what, bool addToPlayer)
{
    addToInventory(static_cast<const GoodCounts&>(what), addToPlayer);
    addToInventory(static_cast<const PeopleCounts&>(what), addToPlayer);
}

void nobBaseWarehouse::addToInventory(const GoodCounts& goods, bool addToPlayer)
{
    GamePlayer& owner = world->getPlayer(player);
    for(const auto i : helpers::enumRange<GoodType>())
    {
        if(!goods[i])
            continue;
        // Can only add canonical shields (romans)
        RTTR_Assert(i == GoodType::ShieldRomans || convertShields(i) != GoodType::ShieldRomans);

        inventory.add(i, goods[i]);
        if(addToPlayer)
            owner.increaseInventoryWare(i, goods[i]);
        checkUsesForNewWare(i);
    }
}

void nobBaseWarehouse::addToInventory(const PeopleCounts& people, bool addToPlayer)
{
    GamePlayer& owner = world->getPlayer(player);
    for(const auto i : helpers::enumRange<Job>())
    {
        if(!people[i])
            continue;
        // Boatcarriers are added as carriers and boat individually
        RTTR_Assert(i != Job::BoatCarrier);

        inventory.add(i, people[i]);
        if(addToPlayer)
            owner.increaseInventoryJob(i, people[i]);

        if(isSoldier(i))
        {
            const auto armoredSoldier = jobEnumToAmoredSoldierEnum(i);
            if(people[armoredSoldier])
            {
                inventory.add(armoredSoldier, people[armoredSoldier]);
                if(addToPlayer)
                    owner.increaseInventoryJob(armoredSoldier, people[armoredSoldier]);
            }
        }
        checkJobsForNewFigure(i);
    }
}

bool nobBaseWarehouse::canRecruit(const Job job) const
{
    if(const auto& requiredTool = JOB_CONSTS[job].tool)
    {
        // Do we have a helper and a tool (if required)?
        return inventory[Job::Helper] > 0 && (*requiredTool == GoodType::Nothing || inventory[*requiredTool] > 0);
    } else // Cannot recruit
        return false;
}

bool nobBaseWarehouse::tryRecruitJob(const Job job)
{
    RTTR_Assert(!helpers::contains(SOLDIER_JOBS, job) && job != Job::PackDonkey);
    if(!canRecruit(job))
        return false;

    auto& owner = world->getPlayer(player);

    const GoodType requiredTool = *JOB_CONSTS[job].tool; // Validity checked in canRecruit
    if(requiredTool != GoodType::Nothing)
    {
        inventory.remove(requiredTool);
        owner.decreaseInventoryWare(requiredTool, 1);
    }

    inventory.remove(Job::Helper);
    owner.decreaseInventoryJob(Job::Helper, 1);

    inventory.add(job);
    owner.increaseInventoryJob(job, 1);
    return true;
}

InventorySetting nobBaseWarehouse::getInventorySettingVisual(const Job job) const
{
    return inventorySettingsVisual[(job == Job::BoatCarrier) ? Job::Helper : job];
}

InventorySetting nobBaseWarehouse::getInventorySettingVisual(const GoodType ware) const
{
    return inventorySettingsVisual[convertShields(ware)];
}

InventorySetting nobBaseWarehouse::getInventorySetting(const Job job) const
{
    return inventorySettings[(job == Job::BoatCarrier) ? Job::Helper : job];
}

InventorySetting nobBaseWarehouse::getInventorySetting(const GoodType ware) const
{
    return inventorySettings[convertShields(ware)];
}

void nobBaseWarehouse::setInventorySettingVisual(const boost_variant2<GoodType, Job>& what, InventorySetting state)
{
    state.makeValid();
    visit([this, state](auto type) { inventorySettingsVisual[type] = state; }, what);

    notifyListeners(1);
}

void nobBaseWarehouse::SetInventorySetting(const boost_variant2<GoodType, Job>& what, InventorySetting state)
{
    state.makeValid();
    InventorySetting& selectedSetting =
      visit([this](auto type) -> InventorySetting& { return inventorySettings[type]; }, what);

    InventorySetting oldState = selectedSetting;
    selectedSetting = state;

    // Bei anderen Spielern als dem lokalen, der das in Auftrag gegeben hat, müssen die visuellen ebenfalls
    // geändert werden oder auch bei Replays
    if(GAMECLIENT.isReplayModeOn() || GAMECLIENT.getPlayerId() != player)
        setInventorySettingVisual(what, state);

    if(holds_alternative<GoodType>(what) && oldState.isSet(EInventorySetting::Stop)
       && !state.isSet(EInventorySetting::Stop))
    {
        // Evtl gabs verlorene Waren, die jetzt in das HQ wieder reinkönnen
        world->getPlayer(player).findClientForLostWares();
    } // No else here!
    if(!oldState.isSet(EInventorySetting::Send) && state.isSet(EInventorySetting::Send))
    {
        // Sind Waren vorhanden, die ausgelagert werden müssen und ist noch kein Auslagerungsevent vorhanden --> neues
        // anmelden
        auto getWaresOrJobs = [this](auto type) { return inventory[type]; };
        if(!empty_event && visit(getWaresOrJobs, what))
            empty_event = getEvMgr().addEvent(this, empty_INTERVAL, 3);
    } else if(!oldState.isSet(EInventorySetting::Collect) && state.isSet(EInventorySetting::Collect))
    {
        // Sollen Waren eingelagert werden? Dann müssen wir neue bestellen
        if(!store_event)
            store_event = getEvMgr().addEvent(this, STORE_INTERVAL, 4);
    }
    notifyListeners(1);
}

void nobBaseWarehouse::SetAllInventorySettings(const bool isJob, const std::vector<InventorySetting>& states)
{
    bool isUnstopped = false;
    bool isCollectSet = false;

    const unsigned numElements = isJob ? inventorySettings.people.size() : inventorySettings.goods.size();
    RTTR_Assert(states.size() == numElements);
    InventorySetting* settings = isJob ? inventorySettings.people.begin() : inventorySettings.goods.begin();

    for(InventorySetting state : states)
    {
        state.makeValid();
        *(settings++) = state;
        isUnstopped |= !state.isSet(EInventorySetting::Stop);
        isCollectSet |= state.isSet(EInventorySetting::Collect);
    }

    // Evtl gabs verlorene Waren, die jetzt in das HQ wieder reinkönnen
    if(isUnstopped)
        world->getPlayer(player).findClientForLostWares();
    // no else!
    // Sind Waren vorhanden, die ausgelagert werden müssen und ist noch kein Auslagerungsevent vorhanden --> neues
    // anmelden
    if(areWaresToEmpty() && !empty_event)
        empty_event = getEvMgr().addEvent(this, empty_INTERVAL, 3);
    // Sollen Waren eingelagert werden? Dann müssen wir neue bestellen
    if(isCollectSet && !store_event)
        store_event = getEvMgr().addEvent(this, STORE_INTERVAL, 4);
}

bool nobBaseWarehouse::isWareDependent(const Ware& ware)
{
    return helpers::contains(dependent_wares, &ware);
}

bool nobBaseWarehouse::areWaresToEmpty() const
{
    // Prüfen, ob Warentyp ausgelagert werden soll und ob noch Waren davon vorhanden sind
    // Waren überprüfen
    for(const auto i : helpers::enumRange<GoodType>())
    {
        if(getInventorySetting(i).isSet(EInventorySetting::Send) && inventory[i])
            return true;
    }

    // Figuren überprüfen
    for(const auto i : helpers::enumRange<Job>())
    {
        if(getInventorySetting(i).isSet(EInventorySetting::Send) && inventory[i])
            return true;
    }

    return false;
}

bool nobBaseWarehouse::defendersAvailable() const
{
    const auto isNonZero = [](const unsigned ct) { return ct != 0; };
    if(helpers::containsIf(reserve_soldiers_available, isNonZero))
        return true;
    for(Job job : SOLDIER_JOBS)
    {
        if(inventory[job])
            return true;
    }

    return false;
}

void nobBaseWarehouse::setReserveVisual(const unsigned rank, const unsigned count)
{
    reserve_soldiers_claimed_visual[rank] = count;
}

void nobBaseWarehouse::setRealReserve(const unsigned rank, const unsigned count)
{
    reserve_soldiers_claimed_real[rank] = count;

    // Replay oder anderer Spieler? Dann die visuellen auch erhöhen
    if(GAMECLIENT.isReplayModeOn() || GAMECLIENT.getPlayerId() != player)
        reserve_soldiers_claimed_visual[rank] = count;

    // Geforderte Soldaten ggf. einbeziehen
    refreshReserve(rank);
}

void nobBaseWarehouse::refreshReserve(unsigned rank)
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

            const unsigned armor = std::min(add, inventory[jobEnumToAmoredSoldierEnum(SOLDIER_JOBS[rank])]);

            // Bei der Reserve hinzufügen
            reserve_soldiers_available[rank] += add;
            reserve_soldiers_available_with_armor[rank] += armor;
            // vom Warenbestand abziehen
            inventory.remove(SOLDIER_JOBS[rank], add);
            inventory.remove(jobEnumToAmoredSoldierEnum(SOLDIER_JOBS[rank]), armor);
        }
    } else if(reserve_soldiers_available[rank] > reserve_soldiers_claimed_real[rank])
    {
        // Zuviele, dann wieder welche freigeben
        unsigned subtract = reserve_soldiers_available[rank] - reserve_soldiers_claimed_real[rank];
        unsigned subtractArmor = 0u;
        if(reserve_soldiers_available_with_armor[rank] > reserve_soldiers_claimed_real[rank])
            subtractArmor = reserve_soldiers_available_with_armor[rank] - reserve_soldiers_claimed_real[rank];

        // Bei der Reserve abziehen
        reserve_soldiers_available[rank] -= subtract;
        reserve_soldiers_available_with_armor[rank] -= subtractArmor;
        // beim Warenbestand hinzufügen
        inventory.add(SOLDIER_JOBS[rank], subtract);
        inventory.add(jobEnumToAmoredSoldierEnum(SOLDIER_JOBS[rank]), subtractArmor);
        // if the rank is supposed to be send away, do it!
        checkOuthousing(SOLDIER_JOBS[rank]);
        // Ggf. Truppen in die Militärgebäude schicken
        world->getPlayer(player).regulateAllTroops();
    }
    // ansonsten ists gleich und alles ist in Ordnung!
}

void nobBaseWarehouse::checkOuthousing(const boost_variant2<GoodType, Job>& what)
{
    // Check if we need to send this ware or figure and register an event for this
    // If we already have an event, we don't need to do anything
    if(empty_event)
        return;

    const InventorySetting setting =
      visit(composeVisitor(
              [this](Job job) { // Bootsträger in Träger umwandeln, der evtl dann raus soll
                  return getInventorySetting((job == Job::BoatCarrier) ? Job::Helper : job);
              },
              [this](GoodType good) { return getInventorySetting(good); }),
            what);

    if(setting.isSet(EInventorySetting::Send))
        empty_event = getEvMgr().addEvent(this, empty_INTERVAL, 3);
}

bool nobBaseWarehouse::isDependentFigure(const noFigure& fig) const
{
    return helpers::contains(dependent_figures, &fig);
}

unsigned nobBaseWarehouse::getAvailableWaresForTrading(const GoodType gt) const
{
    // We need a helper as leader
    if(!inventory[Job::Helper])
        return 0;

    return std::min(inventory[gt], inventory[Job::PackDonkey]);
}

unsigned nobBaseWarehouse::getAvailableFiguresForTrading(const Job job) const
{
    // We need a helper as leader
    if(!inventory[Job::Helper])
        return 0;

    if(job == Job::Helper)
        return (inventory[Job::Helper] - 1) / 2; // need one as leader
    else
        return inventory[job];
}

void nobBaseWarehouse::startTradeCaravane(const boost_variant2<GoodType, Job>& what, const unsigned count,
                                          const TradeRoute& tr, nobBaseWarehouse* goal)
{
    auto tlOwned = std::make_unique<nofTradeLeader>(pos, player, tr, this->getPos(), goal->getPos());
    auto& tl = *tlOwned;
    addLeavingFigure(std::move(tlOwned));

    GamePlayer& owner = world->getPlayer(player);

    // Create the donkeys or other people
    const bool isSoldierType = holds_alternative<Job>(what) && isSoldier(get<Job>(what));
    nofTradeDonkey* last = nullptr;
    for(unsigned i = 0; i < count; ++i)
    {
        auto next = std::make_unique<nofTradeDonkey>(pos, player, what);
        if(isSoldierType)
        {
            auto const job = get<Job>(what);
            if(inventory.real[jobEnumToAmoredSoldierEnum(job)] > 0)
            {
                next->setArmor(true);
                inventory.real.remove(jobEnumToAmoredSoldierEnum(job), 1);
                owner.decreaseInventoryJob(jobEnumToAmoredSoldierEnum(job), 1);
            }
        }

        if(last)
            last->setSuccessor(next.get());
        else
            tl.setSuccessor(next.get());

        last = next.get();
        addLeavingFigure(std::move(next));
    }

    // Remove leader
    inventory.real.remove(Job::Helper);
    owner.decreaseInventoryJob(Job::Helper, 1);

    // Also diminish the count of donkeys
    visit(composeVisitor(
            [&](const Job job) {
                // remove the jobs
                inventory.real.remove(job, count);
                owner.decreaseInventoryJob(job, count);
            },
            [&](const GoodType gt) {
                // Diminish the goods in the warehouse
                inventory.real.remove(gt, count);
                owner.decreaseInventoryWare(gt, count);
                // now that we have removed the goods lets remove the donkeys
                inventory.real.remove(Job::PackDonkey, count);
                owner.decreaseInventoryJob(Job::PackDonkey, count);
            }),
          what);
}
