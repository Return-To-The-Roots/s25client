// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nobHarborBuilding.h"
#include "EventManager.h"
#include "GamePlayer.h"
#include "GlobalGameSettings.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "Ware.h"
#include "figures/noFigure.h"
#include "figures/nofAttacker.h"
#include "figures/nofDefender.h"
#include "helpers/EnumArray.h"
#include "helpers/containerUtils.h"
#include "helpers/pointerContainerUtils.h"
#include "network/GameClient.h"
#include "nobMilitary.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "ogl/glSmartBitmap.h"
#include "pathfinding/RoadPathFinder.h"
#include "postSystem/PostMsgWithBuilding.h"
#include "random/Random.h"
#include "world/GameWorld.h"
#include "nodeObjs/noShip.h"
#include "gameData/BuildingConsts.h"
#include "gameData/MilitaryConsts.h"
#include "gameData/ShieldConsts.h"
#include <limits>

nobHarborBuilding::ExpeditionInfo::ExpeditionInfo(SerializedGameData& sgd)
    : boards(sgd.popUnsignedInt()), stones(sgd.popUnsignedInt()), active(sgd.popBool()), builder(sgd.popBool())
{}

void nobHarborBuilding::ExpeditionInfo::serialize(SerializedGameData& sgd) const
{
    sgd.pushUnsignedInt(boards);
    sgd.pushUnsignedInt(stones);
    sgd.pushBool(active);
    sgd.pushBool(builder);
}

nobHarborBuilding::ExplorationExpeditionInfo::ExplorationExpeditionInfo(SerializedGameData& sgd)
    : active(sgd.popBool()), scouts(sgd.popUnsignedInt())
{}

void nobHarborBuilding::ExplorationExpeditionInfo::serialize(SerializedGameData& sgd) const
{
    sgd.pushBool(active);
    sgd.pushUnsignedInt(scouts);
}

nobHarborBuilding::nobHarborBuilding(const MapPoint pos, const unsigned char player, const Nation nation)
    : nobBaseWarehouse(BuildingType::HarborBuilding, pos, player, nation), orderware_ev(nullptr)
{
    // ins Militärquadrat einfügen
    world->getMilitarySquares().add(this);
    world->recalcTerritory(*this, TerritoryChangeReason::Build);

    // Take 1 as the reserve per rank
    for(unsigned i = 0; i <= world->getGGS().getMaxMilitaryRank(); ++i)
    {
        reserve_soldiers_claimed_visual[i] = reserve_soldiers_claimed_real[i] = 1;
        refreshReserve(i);
    }

    /// Die Meere herausfinden, an die dieser Hafen grenzt
    for(const auto dir : helpers::EnumRange<Direction>{})
        seaIds[dir] = world->getSeaFromCoastalPoint(world->getNeighbour(pos, dir));

    // Post versenden
    sendPostMessage(player,
                    std::make_unique<PostMsgWithBuilding>(getEvMgr().getCurrentGF(), _("New harbor building finished"),
                                                          PostCategory::Economy, *this));
}

void nobHarborBuilding::DestroyBuilding()
{
    getEvMgr().removeEvent(orderware_ev);

    // Der Wirtschaftsverwaltung Bescheid sagen
    GamePlayer& owner = world->getPlayer(player);

    // Baumaterialien in der Inventur verbuchen
    if(expedition.active)
    {
        owner.decreaseInventoryWare(GoodType::Boards, expedition.boards);
        owner.decreaseInventoryWare(GoodType::Stones, expedition.stones);

        // Und Bauarbeiter (später) rausschicken
        if(expedition.builder)
            inventory.add(Job::Builder);
        else
            owner.oneJobNotWanted(Job::Builder, this);
    }
    // cancel order for scouts
    if(exploration_expedition.active)
    {
        inventory.real.add(Job::Scout, exploration_expedition.scouts);
        for(unsigned i = exploration_expedition.scouts; i < world->getGGS().getNumScoutsExpedition(); i++)
            owner.oneJobNotWanted(Job::Scout, this);
    }
    // cancel all jobs wanted for this building
    owner.jobNotWanted(this, true);
    // Waiting Wares löschen
    for(auto& waresForShip : wares_for_ships)
    {
        waresForShip->wareLost(player);
        waresForShip->destroy();
    }
    wares_for_ships.clear();

    // Leute, die noch aufs Schiff warten, rausschicken
    for(auto& figuresForShip : figures_for_ships)
    {
        noFigure& figure = world->addFigure(pos, std::move(figuresForShip.fig));

        figure.abrogate();
        figure.startWandering();
        figure.startWalking(RANDOM_ENUM(Direction));
    }
    figures_for_ships.clear();

    for(auto& soldiersForShip : soldiers_for_ships)
    {
        nofAttacker& soldier = world->addFigure(pos, std::move(soldiersForShip.attacker));

        soldier.cancelSeaAttack();
        RTTR_Assert(!soldier.getAttackedGoal());
        RTTR_Assert(!soldier.getHomeBld());
        RTTR_Assert(!soldier.getGoal());
        soldier.startWandering();
        soldier.startWalking(RANDOM_ENUM(Direction));
    }
    soldiers_for_ships.clear();

    nobBaseWarehouse::DestroyBuilding();

    world->getMilitarySquares().remove(this);
    // Recalc territory. AFTER calling base destroy as otherwise figures might get stuck here
    world->recalcTerritory(*this, TerritoryChangeReason::Destroyed);
}

void nobHarborBuilding::serialize(SerializedGameData& sgd) const
{
    nobBaseWarehouse::serialize(sgd);
    expedition.serialize(sgd);
    exploration_expedition.serialize(sgd);
    sgd.pushEvent(orderware_ev);
    helpers::pushContainer(sgd, seaIds);
    sgd.pushObjectContainer(wares_for_ships, true);
    sgd.pushUnsignedInt(figures_for_ships.size());
    for(const auto& figuresForShip : figures_for_ships)
    {
        helpers::pushPoint(sgd, figuresForShip.dest);
        sgd.pushObject(figuresForShip.fig);
    }
    sgd.pushUnsignedInt(soldiers_for_ships.size());
    for(const auto& soldiersForShip : soldiers_for_ships)
    {
        helpers::pushPoint(sgd, soldiersForShip.dest);
        sgd.pushObject(soldiersForShip.attacker, true);
    }
}

nobHarborBuilding::nobHarborBuilding(SerializedGameData& sgd, const unsigned obj_id)
    : nobBaseWarehouse(sgd, obj_id), expedition(sgd), exploration_expedition(sgd), orderware_ev(sgd.popEvent())
{
    // ins Militärquadrat einfügen
    world->getMilitarySquares().add(this);

    helpers::popContainer(sgd, seaIds);

    sgd.popObjectContainer(wares_for_ships, GO_Type::Ware);

    unsigned count = sgd.popUnsignedInt();
    for(unsigned i = 0; i < count; ++i)
    {
        const MapPoint dest = sgd.popMapPoint();
        auto* fig = sgd.popObject<noFigure>();
        figures_for_ships.emplace_back(FigureForShip{std::unique_ptr<noFigure>(fig), dest});
    }

    count = sgd.popUnsignedInt();
    for(unsigned i = 0; i < count; ++i)
    {
        const MapPoint dest = sgd.popMapPoint();
        auto* fig = sgd.popObject<nofAttacker>(GO_Type::NofAttacker);
        soldiers_for_ships.emplace_back(SoldierForShip{std::unique_ptr<nofAttacker>(fig), dest});
    }
}

// Relative Position des Bauarbeiters
constexpr helpers::EnumArray<Position, Nation> BUILDER_POS = {
  {Position(-20, 18), Position(-28, 17), Position(-20, 15), Position(-38, 17), Position(-38, 17)}};
/// Relative Position der Brettertürme
constexpr helpers::EnumArray<Position, Nation> BOARDS_POS = {
  {Position(-75, -5), Position(-60, -5), Position(-55, -5), Position(-65, -5), Position(-65, -5)}};
/// Relative Position der Steintürme
constexpr helpers::EnumArray<Position, Nation> STONES_POS = {
  {Position(-65, 10), Position(-52, 10), Position(-42, 10), Position(-52, 10), Position(-52, 10)}};
/// Relative Postion der inneren Hafenfeuer
constexpr helpers::EnumArray<Position, Nation> FIRE_POS = {
  {Position(36, -51), Position(0, 0), Position(0, 0), Position(5, -80), Position(0, 0)}};
/// Relative Postion der äußeren Hafenfeuer
constexpr helpers::EnumArray<Position, Nation> EXTRAFIRE_POS = {
  {Position(0, 0), Position(0, 0), Position(8, -115), Position(0, 0), Position(0, 0)}};

void nobHarborBuilding::draw(DrawPoint drawPt)
{
    // Gebäude an sich zeichnen
    drawBaseBuilding(drawPt);

    // Hafenfeuer zeichnen // TODO auch für nicht-römer machen
    if(nation == Nation::Romans || nation == Nation::Japanese || nation == Nation::Babylonians)
    {
        LOADER.getNationImage(nation, 500 + 5 * GAMECLIENT.getGlobalAnimation(8, 2, 1, getObjId() + getX() + getY()))
          ->drawFull(drawPt + FIRE_POS[nation]);
    } else if(nation == Nation::Africans || nation == Nation::Vikings)
    {
        LOADER.getMapTexture(740 + GAMECLIENT.getGlobalAnimation(8, 5, 2, getObjId() + getX() + getY()))
          ->drawFull(drawPt + FIRE_POS[nation]);
    }

    if(nation == Nation::Romans)
    {
        // Zusätzliches Feuer
        LOADER.getMapTexture(740 + GAMECLIENT.getGlobalAnimation(8, 5, 2, getObjId() + getX() + getY()))
          ->drawFull(drawPt + EXTRAFIRE_POS[nation]);
    }

    // Läuft gerade eine Expedition?
    if(expedition.active)
    {
        // Waren für die Expedition zeichnen

        // Bretter
        DrawPoint boardsPos = drawPt + BOARDS_POS[nation];
        for(unsigned char i = 0; i < expedition.boards; ++i)
            LOADER.getWareStackTex(GoodType::Boards)->drawFull(boardsPos - DrawPoint(0, i * 4));
        DrawPoint stonesPos = drawPt + STONES_POS[nation];
        // Steine
        for(unsigned char i = 0; i < expedition.stones; ++i)
            LOADER.getWareStackTex(GoodType::Stones)->drawFull(stonesPos - DrawPoint(0, i * 4));

        // Und den Bauarbeiter, falls er schon da ist
        if(expedition.builder)
        {
            unsigned id = GAMECLIENT.getGlobalAnimation(1000, 7, 1, getX() + getY());

            const int WALKING_DISTANCE = 30;

            // Wegstrecke, die er von einem Punkt vom anderen schon gelaufen ist
            int walking_distance = (id % 500) * WALKING_DISTANCE / 500;
            // Id vom laufen
            unsigned walking_id = (id / 32) % 8;

            DrawPoint builderPos = drawPt + BUILDER_POS[nation];
            if(id < 500)
                LOADER.getBobSprite(nation, Job::Builder, Direction::West, walking_id)
                  .draw(builderPos - DrawPoint(walking_distance, 0), COLOR_WHITE, world->getPlayer(player).color);
            else
                LOADER.getBobSprite(nation, Job::Builder, Direction::East, walking_id)
                  .draw(builderPos + DrawPoint(walking_distance - WALKING_DISTANCE, 0), COLOR_WHITE,
                        world->getPlayer(player).color);
        }
    }
}

void nobHarborBuilding::handleEvent(const unsigned id)
{
    switch(id)
    {
        case 10:
            // Waren-Bestell-Event
            this->orderware_ev = nullptr;
            // Mal wieder schauen, ob es Waren für unsere Expedition gibt
            orderExpeditionWares();
            break;
        default: handleBaseEvent(id); break;
    }
}

/// Startet eine Expedition
void nobHarborBuilding::startExpedition()
{
    // Schon eine Expedition gestartet?
    if(expedition.active)
        return;

    // Initialisierung
    expedition.active = true;

    // In unseren Warenbestand gucken und die erforderlichen Bretter und Steine sowie den
    // Bauarbeiter holen, falls vorhanden
    expedition.boards =
      std::min(unsigned(BUILDING_COSTS[BuildingType::HarborBuilding].boards), inventory[GoodType::Boards]);
    expedition.stones =
      std::min(unsigned(BUILDING_COSTS[BuildingType::HarborBuilding].stones), inventory[GoodType::Stones]);
    inventory.remove(GoodType::Boards, expedition.boards);
    inventory.remove(GoodType::Stones, expedition.stones);

    if(inventory[Job::Builder])
    {
        expedition.builder = true;
        inventory.remove(Job::Builder);
    } else
    {
        bool convert = true;
        expedition.builder = false;
        // got a builder in ANY storehouse?
        GamePlayer& owner = world->getPlayer(player);
        for(const nobBaseWarehouse* wh : owner.getBuildingRegister().getStorehouses()) //-V807
        {
            if(wh->getNumRealFigures(Job::Builder))
            {
                convert = false;
                break;
            }
        }
        if(convert && inventory[GoodType::Hammer]
           && inventory[Job::Helper] > 1) // maybe have a hammer & helper to create our own builder?
        {
            inventory.remove(GoodType::Hammer);
            owner.decreaseInventoryWare(GoodType::Hammer, 1);
            inventory.remove(Job::Helper);
            owner.decreaseInventoryJob(Job::Helper, 1);

            owner.increaseInventoryJob(Job::Builder, 1);
            expedition.builder = true;
        }
        // not in harbor, and didnt have to or couldnt convert so order a builder
        if(!expedition.builder)
            owner.addJobWanted(Job::Builder, this);
    }

    // Ggf. Waren bestellen, die noch fehlen
    orderExpeditionWares();

    // Ggf. ist jetzt alles benötigte schon da
    // Dann Schiff rufen
    checkExpeditionReady();
}

void nobHarborBuilding::stopExpedition()
{
    if(!expedition.active)
        return;

    // Dann diese stoppen
    expedition.active = false;

    // Waren zurücktransferieren
    inventory.add(GoodType::Boards, expedition.boards);
    inventory.add(GoodType::Stones, expedition.stones);

    if(expedition.builder)
    {
        inventory.add(Job::Builder);
        // Evtl. Abnehmer für die Figur wieder finden
        world->getPlayer(player).findWarehouseForAllJobs(Job::Builder);
    } else // todo falls noch nicht da - unterscheiden ob unterwegs oder nur bestellt - falls bestellt stornieren sonst
           // informieren damit kein ersatz geschickt wird falls was nicht klappt aufm weg
    {
        world->getPlayer(player).oneJobNotWanted(Job::Builder, this);
    }
}

/// Startet eine Erkundungs-Expedition oder stoppt sie, wenn bereits eine stattfindet
void nobHarborBuilding::startExplorationExpedition()
{
    // Schon eine Expedition gestartet?
    if(exploration_expedition.active)
        return;

    // Initialisierung
    exploration_expedition.active = true;
    exploration_expedition.scouts = 0;

    // Look for missing scouts
    const unsigned numScoutsRequired = world->getGGS().getNumScoutsExpedition();
    if(inventory[Job::Scout] < numScoutsRequired)
    {
        unsigned missing = numScoutsRequired - inventory[Job::Scout];
        // got scouts in ANY storehouse?
        GamePlayer& owner = world->getPlayer(player);
        for(const nobBaseWarehouse* wh : owner.getBuildingRegister().getStorehouses()) //-V807
        {
            const unsigned numScouts = wh->getNumRealFigures(Job::Scout);
            if(numScouts >= missing)
            {
                missing = 0;
                break;
            } else if(numScouts > 0)
                missing -= numScouts;
        }
        // Recruit missing ones if possible
        while(missing > 0 && tryRecruitJob(Job::Scout))
            missing--;
        // Order scouts, we still requires
        for(unsigned i = inventory[Job::Scout]; i < numScoutsRequired; ++i)
            owner.addJobWanted(Job::Scout, this);
    }
    if(inventory[Job::Scout])
    {
        exploration_expedition.scouts = std::min(inventory[Job::Scout], numScoutsRequired);
        inventory.real.remove(Job::Scout, exploration_expedition.scouts);
    }

    checkExplorationExpeditionReady();
}

void nobHarborBuilding::stopExplorationExpedition()
{
    if(!exploration_expedition.active)
        return;
    // Dann diese stoppen
    exploration_expedition.active = false;
    // cancel order for scouts
    for(unsigned i = exploration_expedition.scouts; i < world->getGGS().getNumScoutsExpedition(); i++)
    {
        world->getPlayer(player).oneJobNotWanted(Job::Scout, this);
    }
    // Erkunder zurücktransferieren
    if(exploration_expedition.scouts)
    {
        inventory.real.add(Job::Scout, exploration_expedition.scouts);
        exploration_expedition.scouts = 0;
        // Evtl. Abnehmer für die Figur wieder finden
        world->getPlayer(player).findWarehouseForAllJobs(Job::Scout);
    }
}

/// Bestellt die zusätzlichen erforderlichen Waren für eine Expedition
void nobHarborBuilding::orderExpeditionWares()
{
    RTTR_Assert(!isBeingDestroyedNow()); // Wares should already be canceled!
    if(this->isBeingDestroyedNow())      // don't order new stuff if we are about to be destroyed
        return;

    if(!expedition.active) // expedition no longer active?
        return;
    // Waren in der Bestellungsliste mit beachten
    unsigned boards = 0, stones = 0;
    for(const auto* ware : dependent_wares)
    {
        RTTR_Assert(ware);
        if(ware->type == GoodType::Boards)
            ++boards;
        else if(ware->type == GoodType::Stones)
            ++stones;
    }

    // Prüfen, ob jeweils noch weitere Waren bestellt werden müssen
    unsigned todoBoards = 0;
    if(boards + expedition.boards < BUILDING_COSTS[BuildingType::HarborBuilding].boards)
    {
        todoBoards = BUILDING_COSTS[BuildingType::HarborBuilding].boards - (boards + expedition.boards);
        Ware* ware;
        do
        {
            ware = world->getPlayer(player).orderWare(GoodType::Boards, *this);
            if(ware)
            {
                RTTR_Assert(isWareDependent(*ware));
                --todoBoards;
            }
        } while(ware && todoBoards);
    }

    unsigned todoStones = 0;
    if(stones + expedition.stones < BUILDING_COSTS[BuildingType::HarborBuilding].stones)
    {
        todoStones = BUILDING_COSTS[BuildingType::HarborBuilding].stones - (stones + expedition.stones);
        Ware* ware;
        do
        {
            ware = world->getPlayer(player).orderWare(GoodType::Stones, *this);
            if(ware)
            {
                RTTR_Assert(isWareDependent(*ware));
                --todoStones;
            }
        } while(ware && todoStones);
    }

    // Wenn immer noch nicht alles da ist, später noch einmal bestellen
    if(!orderware_ev)
        orderware_ev = getEvMgr().addEvent(this, 210, 10);
}

/// Eine bestellte Ware konnte doch nicht kommen
void nobHarborBuilding::wareLost(Ware& ware)
{
    RTTR_Assert(!isBeingDestroyedNow());
    // ggf. neue Waren für Expedition bestellen
    if(expedition.active && (ware.type == GoodType::Boards || ware.type == GoodType::Stones))
        orderExpeditionWares();
    nobBaseWarehouse::wareLost(ware);
}

/// Schiff ist angekommen
void nobHarborBuilding::shipArrived(noShip& ship)
{
    // get a new job - priority is given according to this list: attack,expedition,exploration,transport
    // any attackers ready?
    if(!soldiers_for_ships.empty())
    {
        // load all soldiers that share the same target as the first soldier in the list
        std::vector<std::unique_ptr<nofAttacker>> attackers;
        MapPoint ship_dest = soldiers_for_ships.begin()->dest;

        for(auto it = soldiers_for_ships.begin(); it != soldiers_for_ships.end();)
        {
            if(it->dest == ship_dest)
            {
                removeFromVisualInventory(*it->attacker);
                attackers.push_back(std::move(it->attacker));
                it = soldiers_for_ships.erase(it);
            } else
                ++it;
        }

        ship.prepareSeaAttack(getHarborPosID(), ship_dest, std::move(attackers));
        return;
    }
    // Expedition ready?
    if(expedition.active && expedition.builder
       && expedition.boards == BUILDING_COSTS[BuildingType::HarborBuilding].boards
       && expedition.stones == BUILDING_COSTS[BuildingType::HarborBuilding].stones)
    {
        // Aufräumen am Hafen
        expedition.active = false;
        // Expedition starten
        ship.startExpedition(getHarborPosID());
        return;
    }
    // Exploration-Expedition ready?
    if(isExplorationExpeditionReady())
    {
        // Aufräumen am Hafen
        exploration_expedition.active = false;
        // Expedition starten
        ship.startExplorationExpedition(getHarborPosID());
        inventory.visual.remove(Job::Scout, exploration_expedition.scouts);
        return;
    }

    // Gibt es Waren oder Figuren, die ein Schiff von hier aus nutzen wollen?
    if(!wares_for_ships.empty() || !figures_for_ships.empty())
    {
        // Das Ziel wird nach der ersten Figur bzw. ersten Ware gewählt
        // actually since the wares might not yet have informed the harbor that their target harbor was destroyed we
        // pick the first figure/ware with a valid target instead
        MapPoint dest;
        bool gotdest = false;
        for(const auto& figureForShip : figures_for_ships)
        {
            noBase* nb = world->getNO(figureForShip.dest);
            if(nb->getGOT() == GO_Type::NobHarborbuilding
               && world->getNode(figureForShip.dest).owner
                    == player + 1) // target is a harbor and owned by the same player
            {
                dest = figureForShip.dest;
                gotdest = true;
                break;
            }
        }
        for(const auto& wareForShip : wares_for_ships)
        {
            noBase* nb = world->getNO(wareForShip->getNextHarbor());
            if(nb->getGOT() == GO_Type::NobHarborbuilding
               && world->getNode(wareForShip->getNextHarbor()).owner == player + 1)
            {
                dest = wareForShip->getNextHarbor();
                gotdest = true;
                break;
            }
        }
        if(gotdest)
        {
            std::list<std::unique_ptr<noFigure>> figures;

            // Figuren auswählen, die zu diesem Ziel wollen
            for(auto it = figures_for_ships.begin(); it != figures_for_ships.end() && figures.size() < SHIP_CAPACITY;)
            {
                if(it->dest == dest)
                {
                    it->fig->startShipJourney();
                    removeFromVisualInventory(*it->fig);
                    figures.push_back(std::move(it->fig));
                    it = figures_for_ships.erase(it);
                } else
                    ++it;
            }

            // Und noch die Waren auswählen
            std::list<std::unique_ptr<Ware>> wares;
            for(auto it = wares_for_ships.begin();
                it != wares_for_ships.end() && figures.size() + wares.size() < SHIP_CAPACITY;)
            {
                if((*it)->getNextHarbor() == dest)
                {
                    (*it)->startShipJourney();
                    inventory.visual.remove(convertShields((*it)->type));
                    wares.push_back(std::move(*it));
                    it = wares_for_ships.erase(it);
                } else
                    ++it;
            }

            // Und das Schiff starten lassen
            ship.prepareTransport(getHarborPosID(), dest, std::move(figures), std::move(wares));
        }
    }
}

void nobHarborBuilding::addWare(std::unique_ptr<Ware> ware)
{
    if(ware->getGoal() && ware->getGoal() != this)
    {
        // This is not the goal but we have one -> Get new route
        ware->recalcRoute();

        // Go by ship next?
        if(ware->getNextDir() == RoadPathDirection::Ship)
        {
            addWareForShip(std::move(ware));
            return;
        }
        if(ware->getNextDir() != RoadPathDirection::None)
        {
            // Travel on roads -> Carry out
            RTTR_Assert(ware->getGoal() != this);
            addWaitingWare(std::move(ware));
            return;
        }
        // No next dir means the ware reached its goal, i.e. us,
        // or there is no valid, reachable goal
        // In both cases we take it as we initially would have.
        if(ware->getGoal() && ware->getGoal() != this)
        {
            // We have a goal but it isn't this and there is no next dir
            // This can only happen when the ware was redirected to a warehouse while being carried,
            // see Ware::FindRouteToWarehouse called by Ware::RecalcRoute

            // TODO(Replay) When Ware::FindRouteToWarehouse recalculates the route when called from RecalcRoute
            // and next_dir is None then goal will be NULL or us.
            // So the condition of this branch can never be true and the branch can be replaced by:
            // RTTR_Assert(!ware->GetGoal() || ware->GetGoal() == this)
            RTTR_Assert(ware->isCarried());
            // Explicitly calculate the route which now should set it.
            ware->recalcRoute();
            RTTR_Assert(ware->getGoal());
            RTTR_Assert(ware->getNextDir() != RoadPathDirection::None);
            if(ware->getNextDir() == RoadPathDirection::Ship)
                addWareForShip(std::move(ware));
            else
                addWaitingWare(std::move(ware));
            return;
        }
    }
    // When ware should be transported to any other goal we returned above, so now we need to take it

    // Do we need the ware for an expedition?
    if(expedition.active)
    {
        if((ware->type == GoodType::Boards && expedition.boards < BUILDING_COSTS[BuildingType::HarborBuilding].boards)
           || (ware->type == GoodType::Stones
               && expedition.stones < BUILDING_COSTS[BuildingType::HarborBuilding].stones))
        {
            // Don't wait for it any longer if it had a goal, i.e. us.
            // Without a goal it was a "lost" ware.
            if(ware->getGoal())
                removeDependentWare(*ware);
            // Add to expedition
            world->getPlayer(player).removeWare(*ware);
            if(ware->type == GoodType::Boards)
                ++expedition.boards;
            else
                ++expedition.stones;

            // Could be ready now
            checkExpeditionReady();
            return;
        }
    }

    nobBaseWarehouse::addWare(std::move(ware));
}

/// Eine Figur geht ins Lagerhaus
void nobHarborBuilding::addFigure(std::unique_ptr<noFigure> figure, const bool increaseVisualCounts)
{
    // Brauchen wir einen Bauarbeiter für die Expedition?
    if(figure->getJobType() == Job::Builder && expedition.active && !expedition.builder)
    {
        // Make sure the figure came from outside and was not already here waiting for a ship
        if(isDependentFigure(*figure))
            nobBaseWarehouse::removeDependentFigure(*figure);
        getEvMgr().addToKillList(std::move(figure));

        expedition.builder = true;
        // Ggf. ist jetzt alles benötigte da
        checkExpeditionReady();
    }
    // Brauchen wir einen Spähter für die Expedition?
    else if(figure->getJobType() == Job::Scout && exploration_expedition.active && !isExplorationExpeditionReady())
    {
        // Make sure the figure came from outside and was not already here waiting for a ship
        if(isDependentFigure(*figure))
            nobBaseWarehouse::removeDependentFigure(*figure);
        getEvMgr().addToKillList(std::move(figure));

        ++exploration_expedition.scouts;
        inventory.visual.add(Job::Scout);
        // Ggf. ist jetzt alles benötigte da
        checkExplorationExpeditionReady();
    } else
        // ansonsten weiterdelegieren
        nobBaseWarehouse::addFigure(std::move(figure), increaseVisualCounts);
}

/// Gibt zurück, ob Expedition vollständig ist
bool nobHarborBuilding::isExpeditionReady() const
{
    if(!expedition.active)
        return false;
    // Alles da?
    if(expedition.boards < BUILDING_COSTS[BuildingType::HarborBuilding].boards)
        return false;
    if(expedition.stones < BUILDING_COSTS[BuildingType::HarborBuilding].stones)
        return false;
    if(!expedition.builder)
        return false;

    return true;
}

/// Gibt zurück, ob Expedition vollständig ist
bool nobHarborBuilding::isExplorationExpeditionReady() const
{
    if(!exploration_expedition.active)
        return false;
    // Alles da?
    if(exploration_expedition.scouts < world->getGGS().getNumScoutsExpedition())
        return false;

    return true;
}

/// Prüft, ob eine Expedition von den Waren her vollständig ist und ruft ggf. das Schiff
void nobHarborBuilding::checkExpeditionReady()
{
    // Alles da?
    // Dann bestellen wir mal das Schiff
    if(isExpeditionReady())
        orderShip();
}

/// Prüft, ob eine Expedition von den Spähern her vollständig ist und ruft ggf. das Schiff
void nobHarborBuilding::checkExplorationExpeditionReady()
{
    // Alles da?
    // Dann bestellen wir mal das Schiff
    if(isExplorationExpeditionReady())
        orderShip();
}

/// Schiff konnte nicht mehr kommen
void nobHarborBuilding::shipLost(noShip* /*ship*/)
{
    // Neues Schiff bestellen
    orderShip();
}

/// Gibt die Hafenplatz-ID zurück, auf der der Hafen steht
HarborId nobHarborBuilding::getHarborPosID() const
{
    return world->getHarborPointID(pos);
}

/// Abfangen, wenn ein Mann nicht mehr kommen kann --> könnte ein Bauarbeiter sein und
/// wenn wir einen benötigen, müssen wir einen neuen bestellen
void nobHarborBuilding::removeDependentFigure(noFigure& figure)
{
    nobBaseWarehouse::removeDependentFigure(figure);
    // Ist das ein Bauarbeiter und brauchen wir noch einen
    if(figure.getJobType() == Job::Builder && expedition.active && !expedition.builder)
    {
        // Alle Figuren durchkommen, die noch hierher kommen wollen und gucken, ob ein
        // Bauarbeiter dabei ist
        for(noFigure* dependentFigure : dependent_figures)
        {
            if(dependentFigure->getJobType() == Job::Builder)
                // Brauchen keinen bestellen, also raus
                return;
        }

        // Keinen gefunden, also müssen wir noch einen bestellen
        world->getPlayer(player).addJobWanted(Job::Builder, this);
    }

    // Ist das ein Erkunder und brauchen wir noch welche?
    else if(figure.getJobType() == Job::Scout && exploration_expedition.active)
    {
        unsigned scouts_coming = 0;
        for(noFigure* dependentFigure : dependent_figures)
        {
            if(dependentFigure->getJobType() == Job::Scout)
                ++scouts_coming;
        }

        // Wenn nicht genug Erkunder mehr kommen, müssen wir einen neuen bestellen
        if(exploration_expedition.scouts + scouts_coming < world->getGGS().getNumScoutsExpedition())
            world->getPlayer(player).addJobWanted(Job::Scout, this);
    }
}

/// Gibt eine Liste mit möglichen Verbindungen zurück
std::vector<nobHarborBuilding::ShipConnection> nobHarborBuilding::getShipConnections() const
{
    std::vector<ShipConnection> connections;

    // Is the harbor being destroyed right now? Could happen due to pathfinding for wares that get notified about this
    // buildings destruction
    if(isBeingDestroyedNow())
        return connections;

    // Should already be handled by the above check, but keep the runtime check for now (TODO: remove runtime check)
    RTTR_Assert(world->getGOT(pos) == GO_Type::NobHarborbuilding);

    // Is there any harbor building at all? (could be destroyed)?
    if(world->getGOT(pos) != GO_Type::NobHarborbuilding)
        return connections;

    const auto getSharedSea = [&seaIds = this->seaIds, world = this->world](const nobHarborBuilding& harbor) {
        for(SeaId seaId : seaIds)
        {
            if(seaId && world->isHarborAtSea(harbor.getHarborPosID(), seaId))
                return seaId;
        }
        return SeaId::invalidValue();
    };
    for(auto* harborBuilding : world->getPlayer(player).getBuildingRegister().getHarbors())
    {
        if(harborBuilding == this)
            continue;
        SeaId seaId = getSharedSea(*harborBuilding);
        if(seaId)
        {
            ShipConnection sc;
            sc.dest = harborBuilding;
            // Use twice the distance as cost (ship might need to arrive first) and a fixed value to represent
            // loading&unloading
            sc.way_costs = 2 * world->getHarborDistance(getHarborPosID(), harborBuilding->getHarborPosID(), seaId) + 10;
            connections.push_back(sc);
        }
    }
    return connections;
}

/// Fügt einen Mensch hinzu, der mit dem Schiff irgendwo hin fahren will
void nobHarborBuilding::addFigureForShip(std::unique_ptr<noFigure> fig, MapPoint dest)
{
    RTTR_Assert(!world->hasFigureAt(fig->getPos(), *fig)); // Figure is in the harbor, so it cannot be outside
    addToVisualInventory(*fig);
    figures_for_ships.emplace_back(FigureForShip{std::move(fig), dest});
    orderShip();
}

/// Fügt eine Ware hinzu, die mit dem Schiff verschickt werden soll
void nobHarborBuilding::addWareForShip(std::unique_ptr<Ware> ware)
{
    // Anzahl visuell erhöhen
    inventory.visual.add(convertShields(ware->type));
    ware->waitForShip(this);
    wares_for_ships.emplace_back(std::move(ware));
    orderShip();
    // Take ownership
    ware = nullptr;
}

/// Gibt Anzahl der Schiffe zurück, die noch für ausstehende Aufgaben benötigt werden
unsigned nobHarborBuilding::getNumNeededShips() const
{
    unsigned count = 0;

    // Expedition -> 1 Schiff
    if(isExpeditionReady())
        ++count;
    // Erkundungs-Expedition -> noch ein Schiff
    if(isExplorationExpeditionReady())
        ++count;
    // Evtl. Waren und Figuren -> noch ein Schiff pro Ziel
    if(!figures_for_ships.empty() || !wares_for_ships.empty())
    {
        // Die verschiedenen Zielhäfen -> Für jeden Hafen ein Schiff ordern
        std::vector<MapPoint> destinations;

        for(const auto& figuresForShip : figures_for_ships)
        {
            if(!helpers::contains(destinations, figuresForShip.dest))
            {
                destinations.push_back(figuresForShip.dest);
                ++count;
            }
        }

        for(const auto& waresForShip : wares_for_ships)
        {
            if(!helpers::contains(destinations, waresForShip->getNextHarbor()))
            {
                destinations.push_back(waresForShip->getNextHarbor());
                ++count;
            }
        }
    }
    // Evtl. Angreifer, die noch verschifft werden müssen
    if(!soldiers_for_ships.empty())
    {
        // Die verschiedenen Zielhäfen -> Für jeden Hafen ein Schiff ordern
        std::vector<MapPoint> different_dests;
        for(const auto& soldiersForShip : soldiers_for_ships)
        {
            if(!helpers::contains(different_dests, soldiersForShip.dest))
            {
                different_dests.push_back(soldiersForShip.dest);
                ++count;
            }
        }
    }

    return count;
}

/// Gibt die Wichtigkeit an, dass ein Schiff kommen muss (0 -> keine Bedürftigkeit)
int nobHarborBuilding::getNeedForShip(unsigned shipsComing) const
{
    int points = 0;

    // Expedition -> 1 Schiff
    if(isExpeditionReady())
    {
        if(shipsComing == 0)
            points += 100;
        else
            --shipsComing;
    }
    if(isExplorationExpeditionReady())
    {
        if(shipsComing == 0)
            points += 100;
        else
            --shipsComing;
    }
    if(!figures_for_ships.empty() || !wares_for_ships.empty())
    {
        if(shipsComing)
            --shipsComing;
        else
            points += (figures_for_ships.size() + wares_for_ships.size()) * 5;
    }

    if(!soldiers_for_ships.empty() && shipsComing == 0)
        points += (soldiers_for_ships.size() * 10);

    return points;
}

// try to order any ship that might be needed and is not ordered yet
void nobHarborBuilding::orderShip()
{
    unsigned needed = getNumNeededShips();
    GamePlayer& owner = world->getPlayer(player);

    // Order (possibly) remaining ships
    for(unsigned ordered = owner.getShipsToHarbor(*this); ordered < needed; ++ordered)
        owner.orderShip(*this);
}

/// Abgeleitete kann eine gerade erzeugte Ware ggf. sofort verwenden
/// (muss in dem Fall true zurückgeben)
bool nobHarborBuilding::useWareAtOnce(std::unique_ptr<Ware>& ware, noBaseBuilding& goal)
{
    // If the ware is coming to us, there is nothing to do
    if(&goal == this)
        return false;
    // Evtl. muss die Ware gleich das Schiff nehmen -> dann zum Schiffsreservoir hinzufügen
    // Assert: This is a ware that got ordered. There MUST be a path to the goal
    //         Otherwise the ware will notify the goal which will order a new ware resulting in an infinite loop
    RTTR_Assert(world->getRoadPathFinder().pathExists(*this, goal, true));
    ware->recalcRoute(); // Also sets nextHarbor!
    RTTR_Assert(ware->getNextDir() != RoadPathDirection::None);
    if(ware->getNextDir() == RoadPathDirection::Ship)
    {
        // Dann fügen wir die mal bei uns hinzu
        addWareForShip(std::move(ware));
        return true;
    } else
        return false;
}

/// Dasselbe für Menschen
bool nobHarborBuilding::useFigureAtOnce(std::unique_ptr<noFigure>& fig, noRoadNode& goal)
{
    // If the figure needs to take a ship, add it to the ships inventory

    // If the goal is this building don't handle it
    if(this == &goal)
        return false;

    MapPoint next_harbor;
    if(world->findHumanPathOnRoads(*this, goal, nullptr, &next_harbor) == RoadPathDirection::Ship)
    {
        // Reduce figure count because figures don't go through the house leaving process
        // And therefore the visual count reducement
        removeFromVisualInventory(*fig);
        // Dann fügen wir die mal bei uns hinzu
        addFigureForShip(std::move(fig), next_harbor);
        return true;
    }

    return false;
}

/// Erhält die Waren von einem Schiff und nimmt diese in den Warenbestand auf
void nobHarborBuilding::receiveGoodsFromShip(std::list<std::unique_ptr<noFigure>>& figures,
                                             std::list<std::unique_ptr<Ware>>& wares)
{
    // Menschen zur Ausgehliste hinzufügen
    for(auto& figure : figures)
    {
        figure->arrivedByShip(pos);

        // Wenn es kein Ziel mehr hat, sprich keinen weiteren Weg, kann es direkt hier gelagert werden
        if(figure->getGoal() == this)
        {
            figure->setGoalTonullptr();
            addFigure(std::move(figure), true);
        } else if(!figure->getGoal())
        {
            addDependentFigure(*figure); // No goal? We take it
            addFigure(std::move(figure), true);
        } else
        {
            RoadPathDirection nextDir;
            MapPoint next_harbor = figure->examineRouteBeforeShipping(nextDir); //-V821

            if(nextDir == RoadPathDirection::SouthEast)
            {
                addToVisualInventory(*figure);
                addLeavingFigure(std::move(figure));
            } else if(nextDir == RoadPathDirection::Ship)
            {
                addFigureForShip(std::move(figure), next_harbor);
            } else
            {
                // No or invalid path -> Store here
                RTTR_Assert(nextDir == RoadPathDirection::None);
                figure->setGoalTonullptr();
                addDependentFigure(*figure);
                addFigure(std::move(figure), true);
            }
        }
    }
    figures.clear();

    // Waren zur Warteliste hinzufügen
    for(auto& ware : wares)
    {
        ware->shipJorneyEnded(this);
        addWare(std::move(ware));
    }
    wares.clear();
}

nofAggressiveDefender* nobHarborBuilding::sendAggressiveDefender(nofAttacker& attacker)
{
    // Don't sent out last soldier
    unsigned numSoldiers = 0;
    for(const Job rankJob : SOLDIER_JOBS)
    {
        numSoldiers += inventory[rankJob];
        if(numSoldiers > 1)
            break;
    }
    if(numSoldiers <= 1)
        return nullptr;
    return nobBaseWarehouse::sendAggressiveDefender(attacker);
}

/// Storniert die Bestellung für eine bestimmte Ware, die mit einem Schiff transportiert werden soll
std::unique_ptr<Ware> nobHarborBuilding::cancelWareForShip(Ware* ware)
{
    // Ware zur Inventur hinzufügen
    inventory.real.add(convertShields(ware->type));
    return helpers::extractPtr(wares_for_ships, ware);
}

/// Bestellte Figur, die sich noch inder Warteschlange befindet, kommt nicht mehr und will rausgehauen werden
void nobHarborBuilding::cancelFigure(noFigure* figure)
{
    const auto it =
      helpers::findIf(figures_for_ships, [figure](const FigureForShip& it) { return it.fig.get() == figure; });

    // Figur ggf. aus der List entfernen
    if(it != figures_for_ships.end())
    {
        // Dann zu unserem Inventar hinzufügen
        addFigure(std::move(it->fig), false);
        figures_for_ships.erase(it);
    }
    // An Basisklasse weiterdelegieren
    else
        nobBaseWarehouse::cancelFigure(figure);
}

/// Gibt verfügbare Angreifer zurück
std::vector<nobHarborBuilding::SeaAttackerBuilding> nobHarborBuilding::getAttackerBuildingsForSeaIdAttack()
{
    std::vector<nobHarborBuilding::SeaAttackerBuilding> buildings;
    sortedMilitaryBlds all_buildings = world->lookForMilitaryBuildings(pos, 3);
    // Und zählen
    for(nobBaseMilitary* allBuilding : all_buildings)
    {
        if(allBuilding->getGOT() != GO_Type::NobMilitary)
            continue;

        // Liegt er auch im groben Raster und handelt es sich um den gleichen Besitzer?
        if(allBuilding->getPlayer() != player
           || world->calcDistance(allBuilding->getPos(), pos) > BASE_ATTACKING_DISTANCE)
            continue;
        // Gebäude suchen, vielleicht schon vorhanden? Dann können wir uns den pathfinding Aufwand sparen!
        if(helpers::contains(buildings, static_cast<nobMilitary*>(allBuilding)))
        {
            // Dann zum nächsten test
            continue;
        }
        // Weg vom Hafen zum Militärgebäude berechnen
        if(!world->findHumanPath(allBuilding->getPos(), pos, MAX_ATTACKING_RUN_DISTANCE))
            continue;
        // neues Gebäude mit weg und allem -> in die Liste!
        SeaAttackerBuilding sab = {static_cast<nobMilitary*>(allBuilding), this, 0};
        buildings.push_back(sab);
    }
    return buildings;
}

std::vector<nobHarborBuilding::SeaAttackerBuilding>
nobHarborBuilding::getAttackerBuildingsForSeaAttack(const std::vector<HarborId>& defenderHarbors)
{
    std::vector<nobHarborBuilding::SeaAttackerBuilding> buildings;
    sortedMilitaryBlds all_buildings = world->lookForMilitaryBuildings(pos, 3);
    // Und zählen
    for(nobBaseMilitary* allBuilding : all_buildings)
    {
        if(allBuilding->getGOT() != GO_Type::NobMilitary)
            continue;

        // Liegt er auch im groben Raster und handelt es sich um den gleichen Besitzer?
        if(allBuilding->getPlayer() != player
           || world->calcDistance(allBuilding->getPos(), pos) > BASE_ATTACKING_DISTANCE)
            continue;

        // Weg vom Hafen zum Militärgebäude berechnen
        if(!world->findHumanPath(allBuilding->getPos(), pos, MAX_ATTACKING_RUN_DISTANCE))
            continue;

        // Entfernung zwischen Hafen und möglichen Zielhafenpunkt ausrechnen
        auto minDistance = std::numeric_limits<unsigned>::max();
        for(HarborId defenderHarbor : defenderHarbors)
        {
            minDistance = std::min(minDistance, world->getMinHarborDistance(getHarborPosID(), defenderHarbor));
        }

        // Gebäude suchen, vielleicht schon vorhanden?
        auto it2 = helpers::find(buildings, static_cast<nobMilitary*>(allBuilding));
        // Noch nicht vorhanden?
        if(it2 == buildings.end())
        {
            // Dann neu hinzufügen
            SeaAttackerBuilding sab = {static_cast<nobMilitary*>(allBuilding), this, minDistance};
            buildings.push_back(sab);
        }
        // Oder vorhanden und jetzige Distanz ist kleiner?
        else if(minDistance < it2->distance)
        {
            // Dann Distanz und betreffenden Hafen aktualisieren
            it2->distance = minDistance;
            it2->harbor = this;
        }
    }
    return buildings;
}

/// Fügt einen Schiffs-Angreifer zum Hafen hinzu
void nobHarborBuilding::addSeaAttacker(std::unique_ptr<nofAttacker> attacker)
{
    unsigned bestDistance = 0xffffffff;
    HarborId bestHarborPoint;
    RTTR_Assert(attacker->getAttackedGoal());
    std::vector<HarborId> harborPoints =
      world->getHarborPointsAroundMilitaryBuilding(attacker->getAttackedGoal()->getPos());
    for(HarborId harborPoint : harborPoints)
    {
        unsigned tmp_distance = world->getMinHarborDistance(this->getHarborPosID(), harborPoint);
        if(tmp_distance < bestDistance)
        {
            bestDistance = tmp_distance;
            bestHarborPoint = harborPoint;
        }
    }

    // no harbor to target (should not happen) or no target (might happen very very rarely not sure)
    if(!bestHarborPoint)
    {
        // notify target about noShow, notify home that soldier wont return, add to inventory
        attacker->seaAttackFailedBeforeLaunch(); // set state, remove target & home
        RTTR_Assert(!attacker->getAttackedGoal());
        RTTR_Assert(!attacker->getHomeBld());
        RTTR_Assert(!attacker->getGoal());
        addFigure(std::move(attacker), true);
        return;
    }

    inventory.visual.add(attacker->getJobType());
    if(attacker->hasArmor())
        inventory.visual.add(jobEnumToAmoredSoldierEnum(attacker->getJobType()));
    soldiers_for_ships.emplace_back(SoldierForShip{std::move(attacker), world->getHarborPoint(bestHarborPoint)});

    orderShip();
}

void nobHarborBuilding::cancelSeaAttacker(nofAttacker* attacker)
{
    const auto it =
      helpers::findIf(soldiers_for_ships, [attacker](const auto& it) { return it.attacker.get() == attacker; });

    RTTR_Assert(it != soldiers_for_ships.end());
    if(!attacker->getGoal())
    {
        // No goal? We take it
        addDependentFigure(*attacker);
        addFigure(std::move(it->attacker), false);
    } else
        addLeavingFigure(std::move(it->attacker)); // Just let him leave so he can go home
    soldiers_for_ships.erase(it);
}

unsigned nobHarborBuilding::calcDistributionPoints(const GoodType type) const
{
    // Ist überhaupt eine Expedition im Gang und ein entsprechender Warentyp
    if(!expedition.active || (type != GoodType::Boards && type != GoodType::Stones))
        return 0;

    unsigned ordered_boards = 0, ordered_stones = 0;
    // Ermitteln, wiviele Bretter und Steine auf dem Weg zum Lagerhaus sind
    for(auto* dependentWare : dependent_wares)
    {
        if(dependentWare->type == GoodType::Boards)
            ++ordered_boards;
        else if(dependentWare->type == GoodType::Stones)
            ++ordered_stones;
    }

    // 10000 als Basis wählen, damit man auch noch was abziehen kann
    unsigned short points = 10000;

    // Ermitteln, ob wir noch Bretter oder Steine brauchen
    if(expedition.boards + ordered_boards >= BUILDING_COSTS[BuildingType::HarborBuilding].boards
       && type == GoodType::Boards)
        return 0;
    if(expedition.stones + ordered_stones >= BUILDING_COSTS[BuildingType::HarborBuilding].stones
       && type == GoodType::Stones)
        return 0;

    // Schon bestellte Sachen wirken sich positiv aus, da wir ja so eher eine Expedition bereit haben
    if(type == GoodType::Boards)
        points += (expedition.boards + ordered_boards) * 30;
    else if(type == GoodType::Stones)
        points += (expedition.stones + ordered_stones) * 30;

    return points;
}

/// A ware changed its route and doesn't want to use the ship anymore
void nobHarborBuilding::wareDontWantToTravelByShip(Ware* ware)
{
    // Maybe this building is already destroyed
    if(world->getGOT(pos) != GO_Type::NobHarborbuilding)
        return;

    // Move to waiting_wares
    waiting_wares.push_back(helpers::extractPtr(wares_for_ships, ware));
    ware->waitInWarehouse(*this);
    // Carry out. If it would want to go back to this building, then this will be handled by the carrier
    addLeavingEvent();
}

/// Stellt Verteidiger zur Verfügung
std::unique_ptr<nofDefender> nobHarborBuilding::provideDefender(nofAttacker& attacker)
{
    // Versuchen, zunächst auf konventionelle Weise Angreifer zu bekoommen
    std::unique_ptr<nofDefender> defender = nobBaseWarehouse::provideDefender(attacker);
    // Wenn das nicht geklappt hat und noch Soldaten in der Warteschlange für den Seeangriff sind
    // zweigen wir einfach diese ab
    if(!defender && !soldiers_for_ships.empty())
    {
        std::unique_ptr<nofAttacker> defender_attacker = std::move(soldiers_for_ships.begin()->attacker);
        soldiers_for_ships.pop_front();
        defender = std::make_unique<nofDefender>(pos, player, *this, defender_attacker->getRank(), attacker);
        defender->setArmor(defender_attacker->hasArmor());
        defender_attacker->cancelSeaAttack();
        defender_attacker->destroy();
    }

    return defender;
}

/// People waiting for a ship have to examine their route if a road was destroyed
void nobHarborBuilding::examineShipRouteOfPeople()
{
    for(auto it = figures_for_ships.begin(); it != figures_for_ships.end();)
    {
        auto& fig = it->fig;
        RoadPathDirection nextDir;
        it->dest = fig->examineRouteBeforeShipping(nextDir);

        if(nextDir == RoadPathDirection::None)
        {
            // No route found!
            // I.E. insert the worker in this harbor
            addDependentFigure(*fig);
            addFigure(std::move(fig), false);
            it = figures_for_ships.erase(it);
        } else if(nextDir != RoadPathDirection::Ship)
        {
            // Figure want to continue walking to its goal but not on ship anymore
            this->addLeavingFigure(std::move(fig));
            it = figures_for_ships.erase(it);
        } else
            // Otherwise figure want to travel by ship, do nothing!
            ++it;
    }
}

bool nobHarborBuilding::isBeingDestroyedNow() const
{
    // check if this harbor is in the known harbors. if not, it is probably being destroyed right now.
    return !helpers::contains(world->getPlayer(player).getBuildingRegister().getHarbors(), this);
}
