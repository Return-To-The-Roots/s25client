// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nofBuilder.h"
#include "EventManager.h"
#include "GameEvent.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "SoundManager.h"
#include "buildings/noBuildingSite.h"
#include "buildings/nobBaseWarehouse.h"
#include "factories/BuildingFactory.h"
#include "network/GameClient.h"
#include "notifications/BuildingNote.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "ogl/glSmartBitmap.h"
#include "random/Random.h"
#include "world/GameWorld.h"
#include "gameData/BuildingConsts.h"
#include "gameData/BuildingProperties.h"

nofBuilder::nofBuilder(const MapPoint pos, const unsigned char player, noBuildingSite* building_site)
    : noFigure(Job::Builder, pos, player, building_site), state(BuilderState::FigureWork), building_site(building_site),
      building_steps_available(0)
{
    // Sind wir schon an unsere Baustelle gleich hingesetzt worden (bei Häfen)?
    if(building_site)
    {
        if(pos == building_site->getPos())
        {
            // Dann gleich mit dem Bauprozess beginnen
            fs = FigureState::Job;
            goalReached();
        }
    }
}

void nofBuilder::Serialize(SerializedGameData& sgd) const
{
    noFigure::Serialize(sgd);

    sgd.pushEnum<uint8_t>(state);
    sgd.pushObject(building_site, true);
    helpers::pushPoint(sgd, offsetSite);
    helpers::pushPoint(sgd, nextOffsetSite);
    sgd.PushUnsignedChar(building_steps_available);
}

nofBuilder::nofBuilder(SerializedGameData& sgd, const unsigned obj_id)
    : noFigure(sgd, obj_id), state(sgd.pop<BuilderState>()),
      building_site(sgd.popObject<noBuildingSite>(GO_Type::Buildingsite)),
      offsetSite(helpers::popPoint<Point<int16_t>>(sgd)), nextOffsetSite(helpers::popPoint<Point<int16_t>>(sgd)),
      building_steps_available(sgd.PopUnsignedChar())
{}

void nofBuilder::goalReached()
{
    goal_ = nullptr;
    // an der Baustelle normal anfangen zu arbeiten
    state = BuilderState::WaitingFreewalk;

    // Sind jetzt an der Baustelle
    offsetSite = Point<short>(0, 0);

    // Anfangen um die Baustelle herumzulaufen
    startFreewalk();
}

void nofBuilder::walked() {}

void nofBuilder::abrogateWorkplace()
{
    if(building_site)
    {
        state = BuilderState::FigureWork;
        building_site->abrogate();
        building_site = nullptr;
    }
}

void nofBuilder::lostWork()
{
    building_site = nullptr;

    if(state == BuilderState::FigureWork)
        goHome();
    else
    {
        // Event abmelden
        getEvMgr().removeEvent(current_ev);

        startWandering();
        wander();
        state = BuilderState::FigureWork;
    }
}

void nofBuilder::handleDerivedEvent(const unsigned id)
{
    RTTR_Assert(id == 1u && current_ev->id == id);
    switch(state)
    {
        case BuilderState::WaitingFreewalk:
        {
            // Platz einnehmen
            offsetSite = nextOffsetSite;

            // Ware aufnehmen, falls es eine gibt
            if(chooseWare())
                state = BuilderState::BuildFreewalk;

            // Weiter drumrumlaufen
            startFreewalk();
        }
        break;
        case BuilderState::BuildFreewalk:
        {
            // Platz einnehmen
            offsetSite = nextOffsetSite;

            // Gibts noch was zu bauen?
            if(building_steps_available)
            {
                // dann mal schön bauen
                current_ev = getEvMgr().addEvent(this, 40, 1);
                state = BuilderState::Build;
            } else if(building_site->isBuildingComplete())
            {
                // fertig mit Bauen!
                current_ev = nullptr;

                // Baustelle abreißen und Gebäude hinsetzen

                // Gebäudetyp merken und das Volk des Gebäudes
                BuildingType building_type = building_site->getBuildingType();
                Nation building_nation = building_site->getNation();

                state = BuilderState::FigureWork;

                // Baustelle abmelden
                GamePlayer& owner = world->getPlayer(player);
                owner.removeBuildingSite(building_site);
                if(world->isHarborBuildingSiteFromSea(building_site))
                    world->removeHarborBuildingSiteFromSea(building_site);

                // Remove buildingsite, but don't destroy!
                world->setNO(building_site->getPos(), nullptr);
                deletePtr(building_site);

                noBuilding* bld = BuildingFactory::createBuilding(*world, building_type, pos, player, building_nation);
                world->getNotifications().publish(BuildingNote(BuildingNote::Constructed, player, pos, building_type));

                // Special handling for warehouses
                if(BuildingProperties::isWareHouse(building_type))
                {
                    auto* wh = static_cast<nobBaseWarehouse*>(bld);
                    // Mich dort gleich einquartieren und nicht erst zurücklaufen
                    wh->addFigure(world->RemoveFigure(pos, *this));

                    // Evtl Träger aus dem HQ wieder verwenden
                    owner.findCarrierForAllRoads();
                    owner.findWarehouseForAllJobs(Job::Helper);

                    // Evtl gabs verlorene Waren, die jetzt in das WH wieder reinkönnen
                    owner.findClientForLostWares();
                    return;
                }

                // Nach Hause laufen bzw. auch rumirren
                rs_pos = 0;
                rs_dir = true;
                cur_rs = world->getSpecObj<noRoadNode>(pos)->getRoute(Direction::SouthEast);

                goHome();
                startWalking(Direction::SouthEast);
            } else
            {
                // Brauchen neues Material

                // Ware aufnehmen, falls es eine gibt
                if(!chooseWare())
                    state = BuilderState::WaitingFreewalk;

                // Weiter drumrumlaufen
                startFreewalk();
            }
        }
        break;
        case BuilderState::Build:
        {
            // Sounds abmelden
            world->getSoundMgr().stopSounds(*this);

            // ein Bauschritt weniger, Haus um eins höher
            --building_steps_available;
            ++building_site->build_progress;
            // Fertig mit dem Bauschritt, dann an nächste Position gehen
            state = BuilderState::BuildFreewalk;
            startFreewalk();
        }
        break;
        default: break;
    }
}

// Länge, die der Bauarbeiter in einem Free-Walk zurücklegt (in Pixeln)
const std::array<short, 2> FREEWALK_LENGTH = {22, 11};          // waagerecht
const std::array<short, 2> FREEWALK_LENGTH_SLANTWISE = {14, 7}; // schräg

void nofBuilder::startFreewalk()
{
    std::vector<Direction> possible_directions;

    unsigned char waiting_walk = ((state == BuilderState::WaitingFreewalk) ? 0 : 1);

    // Wohin kann der Bauarbeiter noch laufen?

    // Nach links
    if(offsetSite.x - FREEWALK_LENGTH[waiting_walk] >= LEFT_MAX)
        possible_directions.push_back(Direction::West);
    // Nach rechts
    if(offsetSite.x + FREEWALK_LENGTH[waiting_walk] <= RIGHT_MAX)
        possible_directions.push_back(Direction::East);
    // Nach links/oben
    if(offsetSite.x - FREEWALK_LENGTH_SLANTWISE[waiting_walk] >= LEFT_MAX
       && offsetSite.y - FREEWALK_LENGTH_SLANTWISE[waiting_walk] >= UP_MAX)
        possible_directions.push_back(Direction::NorthWest);
    // Nach links/unten
    if(offsetSite.x - FREEWALK_LENGTH_SLANTWISE[waiting_walk] >= LEFT_MAX
       && offsetSite.y + FREEWALK_LENGTH_SLANTWISE[waiting_walk] <= DOWN_MAX)
        possible_directions.push_back(Direction::SouthWest);
    // Nach rechts/oben
    if(offsetSite.x + FREEWALK_LENGTH_SLANTWISE[waiting_walk] <= RIGHT_MAX
       && offsetSite.y - FREEWALK_LENGTH_SLANTWISE[waiting_walk] >= UP_MAX)
        possible_directions.push_back(Direction::NorthEast);
    // Nach rechts/unten
    if(offsetSite.x + FREEWALK_LENGTH_SLANTWISE[waiting_walk] <= RIGHT_MAX
       && offsetSite.y + FREEWALK_LENGTH_SLANTWISE[waiting_walk] <= DOWN_MAX)
        possible_directions.push_back(Direction::SouthEast);

    RTTR_Assert(!possible_directions.empty());
    // Zufällige Richtung von diesen auswählen
    faceDir(RANDOM_ELEMENT(possible_directions));

    // Und dort auch hinlaufen
    current_ev = getEvMgr().addEvent(this, (state == BuilderState::WaitingFreewalk) ? 24 : 17, 1);

    // Zukünftigen Platz berechnen
    nextOffsetSite = offsetSite;

    switch(getCurMoveDir())
    {
        case Direction::West: nextOffsetSite.x -= FREEWALK_LENGTH[waiting_walk]; break;
        case Direction::NorthWest:
            nextOffsetSite.x -= FREEWALK_LENGTH_SLANTWISE[waiting_walk];
            nextOffsetSite.y -= FREEWALK_LENGTH_SLANTWISE[waiting_walk];
            break;
        case Direction::NorthEast:
            nextOffsetSite.x += FREEWALK_LENGTH_SLANTWISE[waiting_walk];
            nextOffsetSite.y -= FREEWALK_LENGTH_SLANTWISE[waiting_walk];
            break;
        case Direction::East: nextOffsetSite.x += FREEWALK_LENGTH[waiting_walk]; break;
        case Direction::SouthEast:
            nextOffsetSite.x += FREEWALK_LENGTH_SLANTWISE[waiting_walk];
            nextOffsetSite.y += FREEWALK_LENGTH_SLANTWISE[waiting_walk];
            break;
        case Direction::SouthWest:
            nextOffsetSite.x -= FREEWALK_LENGTH_SLANTWISE[waiting_walk];
            nextOffsetSite.y += FREEWALK_LENGTH_SLANTWISE[waiting_walk];
            break;
    }
}

void nofBuilder::draw(DrawPoint drawPt)
{
    switch(state)
    {
        case BuilderState::FigureWork:
        {
            drawWalkingBobJobs(drawPt, Job::Builder);
        }
        break;
        case BuilderState::BuildFreewalk:
        case BuilderState::WaitingFreewalk:
        {
            // Interpolieren und Door-Point von Baustelle draufaddieren
            drawPt.x += GAMECLIENT.interpolate(offsetSite.x, nextOffsetSite.x, current_ev);
            drawPt.y += GAMECLIENT.interpolate(offsetSite.y, nextOffsetSite.y, current_ev);
            drawPt += building_site->getDoorPoint();

            LOADER
              .getBobSprite(building_site->getNation(), Job::Builder, getCurMoveDir(),
                            GAMECLIENT.interpolate(12, current_ev) % 8u)
              .draw(drawPt, COLOR_WHITE, world->getPlayer(player).color);
        }
        break;
        case BuilderState::Build:
        {
            const unsigned index = GAMECLIENT.interpolate(28, current_ev);
            unsigned texture;
            unsigned soundId = 0;

            // Je nachdem, wie weit der Bauarbeiter links bzw rechts oder in der Mitte steht, so wird auch die Animation
            // angezeigt
            if(std::abs(offsetSite.x) > 5)
            {
                // With hammer
                if(index < 12 || index > 19)
                {
                    // standing
                    if(offsetSite.x < 0)
                        texture = 353; // From left
                    else
                        texture = 279; // From right
                    texture += index % 4;
                    soundId = 78;
                } else
                {
                    // er kniet
                    texture = 283 + index % 4;
                    soundId = 72;
                }
            } else
            {
                // in der Mitte mit "Händen"
                texture = 287 + (index / 2) % 4;
            }
            drawPt += building_site->getDoorPoint() + DrawPoint(offsetSite);
            LOADER.getPlayerImage("rom_bobs", texture)
              ->drawFull(drawPt, COLOR_WHITE, world->getPlayer(building_site->getPlayer()).color);
            if(soundId && index % 4 == 2)
                world->getSoundMgr().playNOSound(soundId, *this, index, 160 - rand() % 60);
        }
        break;
    }
}

bool nofBuilder::chooseWare()
{
    // Brauch ich ein Brett(Rohbau und wenn kein Stein benötigt wird) oder Stein?
    const BuildingCost costs = BUILDING_COSTS[building_site->getBuildingType()];
    if(building_site->getBuildProgress(false) < costs.boards * 8 || !costs.stones)
    {
        // Brett
        if(building_site->boards)
        {
            // ein Brett weniger liegt da
            --building_site->boards;
            ++building_site->used_boards;
            // wir können 8 Bauschritt ausführen
            building_steps_available = 8;

            return true;
        }
    } else
    {
        // Stein
        if(building_site->stones)
        {
            // ein Stein weniger liegt da
            --building_site->stones;
            ++building_site->used_stones;
            // wir können 8 Bauschritt ausführen
            building_steps_available = 8;

            return true;
        }
    }

    return false;
}
