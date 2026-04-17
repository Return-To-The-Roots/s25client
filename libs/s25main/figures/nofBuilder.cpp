// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nofBuilder.h"
#include "BuildingEventLogger.h"
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
    // Were we placed directly at our construction site already (for harbors)?
    if(building_site)
    {
        if(pos == building_site->GetPos())
        {
            // Then start the building process right away
            fs = FigureState::Job;
            GoalReached();
        }
    }
}

void nofBuilder::Serialize(SerializedGameData& sgd) const
{
    noFigure::Serialize(sgd);

    sgd.PushEnum<uint8_t>(state);
    sgd.PushObject(building_site, true);
    helpers::pushPoint(sgd, offsetSite);
    helpers::pushPoint(sgd, nextOffsetSite);
    sgd.PushUnsignedChar(building_steps_available);
}

nofBuilder::nofBuilder(SerializedGameData& sgd, const unsigned obj_id)
    : noFigure(sgd, obj_id), state(sgd.Pop<BuilderState>()),
      building_site(sgd.PopObject<noBuildingSite>(GO_Type::Buildingsite)),
      offsetSite(helpers::popPoint<Point<int16_t>>(sgd)), nextOffsetSite(helpers::popPoint<Point<int16_t>>(sgd)),
      building_steps_available(sgd.PopUnsignedChar())
{}

void nofBuilder::GoalReached()
{
    goal_ = nullptr;
    // Start working normally at the construction site
    state = BuilderState::WaitingFreewalk;

    // We are now at the construction site
    offsetSite = Point<short>(0, 0);

    // Start walking around the construction site
    StartFreewalk();
}

void nofBuilder::Walked() {}

void nofBuilder::AbrogateWorkplace()
{
    if(building_site)
    {
        state = BuilderState::FigureWork;
        building_site->Abrogate();
        building_site = nullptr;
    }
}

void nofBuilder::LostWork()
{
    building_site = nullptr;

    if(state == BuilderState::FigureWork)
        GoHome();
    else
    {
        // Unregister event
        GetEvMgr().RemoveEvent(current_ev);

        StartWandering();
        Wander();
        state = BuilderState::FigureWork;
    }
}

void nofBuilder::HandleDerivedEvent(const unsigned id)
{
    RTTR_Assert(id == 1u && current_ev->id == id);
    switch(state)
    {
        case BuilderState::WaitingFreewalk:
        {
            // Take up the spot
            offsetSite = nextOffsetSite;

            // Pick up material if available
            if(ChooseWare())
                state = BuilderState::BuildFreewalk;

            // Keep walking around it
            StartFreewalk();
        }
        break;
        case BuilderState::BuildFreewalk:
        {
            // Take up the spot
            offsetSite = nextOffsetSite;

            // Is there still something to build?
            if(building_steps_available)
            {
                // Then keep building
                current_ev = GetEvMgr().AddEvent(this, 40, 1);
                state = BuilderState::Build;
            } else if(building_site->IsBuildingComplete())
            {
                // Done building!
                current_ev = nullptr;

                // Demolish the construction site and place the building

                // Remember the building type and its nation
                BuildingType building_type = building_site->GetBuildingType();
                Nation building_nation = building_site->GetNation();

                state = BuilderState::FigureWork;

                // Unregister the construction site
                GamePlayer& owner = world->GetPlayer(player);
                BuildingEventLogger::MarkConstructionSiteConstructed(building_site);
                owner.RemoveBuildingSite(building_site);
                if(world->IsHarborBuildingSiteFromSea(building_site))
                    world->RemoveHarborBuildingSiteFromSea(building_site);

                const unsigned completionGF = world->GetEvMgr().GetCurrentGF();
                const unsigned startFrame = building_site->GetBuildStartingFrame();
                building_site->SetBuildCompleteFrame(completionGF);

                // Remove buildingsite, but don't destroy!
                world->SetNO(building_site->GetPos(), nullptr);
                deletePtr(building_site);

                noBuilding* bld = BuildingFactory::CreateBuilding(*world, building_type, pos, player, building_nation);
                bld->SetBuildStartingFrame(startFrame);
                bld->SetBuildCompleteFrame(completionGF);
                BuildingEventLogger::LogBuildingConstructed(completionGF, player, building_type, bld->GetObjId(), pos.x,
                                                            pos.y);
                if(BuildingProperties::IsWareHouse(building_type))
                    BuildingEventLogger::LogBuildingInhabited(completionGF, player, building_type, bld->GetObjId(),
                                                              pos.x, pos.y);
                world->GetNotifications().publish(BuildingNote(BuildingNote::Constructed, player, pos, building_type));

                // Special handling for warehouses
                if(BuildingProperties::IsWareHouse(building_type))
                {
                    auto* wh = static_cast<nobBaseWarehouse*>(bld);
                    // Move in there immediately instead of walking back first
                    wh->AddFigure(world->RemoveFigure(pos, *this));

                    // Possibly reuse carriers from the HQ
                    owner.FindCarrierForAllRoads();
                    owner.FindWarehouseForAllJobs(Job::Helper);

                    // Possibly there were lost wares that can now go back into the warehouse
                    owner.FindClientForLostWares();
                    return;
                }

                // Walk home or keep wandering
                rs_pos = 0;
                rs_dir = true;
                cur_rs = world->GetSpecObj<noRoadNode>(pos)->GetRoute(Direction::SouthEast);

                GoHome();
                StartWalking(Direction::SouthEast);
            } else
            {
                // Need new material

                // Pick up material if available
                if(!ChooseWare())
                    state = BuilderState::WaitingFreewalk;

                // Keep walking around it
                StartFreewalk();
            }
        }
        break;
        case BuilderState::Build:
        {
            // Stop sounds
            world->GetSoundMgr().stopSounds(*this);

            // One building step less, house one level higher
            --building_steps_available;
            ++building_site->build_progress;
            // Finished with the building step, then move to the next position
            state = BuilderState::BuildFreewalk;
            StartFreewalk();
        }
        break;
        default: break;
    }
}

// Distance the builder covers during a free walk (in pixels)
const std::array<short, 2> FREEWALK_LENGTH = {22, 11};          // horizontal
const std::array<short, 2> FREEWALK_LENGTH_SLANTWISE = {14, 7}; // diagonal

void nofBuilder::StartFreewalk()
{
    std::vector<Direction> possible_directions;

    unsigned char waiting_walk = ((state == BuilderState::WaitingFreewalk) ? 0 : 1);

    // Where else can the builder walk?

    // To the left
    if(offsetSite.x - FREEWALK_LENGTH[waiting_walk] >= LEFT_MAX)
        possible_directions.push_back(Direction::West);
    // To the right
    if(offsetSite.x + FREEWALK_LENGTH[waiting_walk] <= RIGHT_MAX)
        possible_directions.push_back(Direction::East);
    // To the upper left
    if(offsetSite.x - FREEWALK_LENGTH_SLANTWISE[waiting_walk] >= LEFT_MAX
       && offsetSite.y - FREEWALK_LENGTH_SLANTWISE[waiting_walk] >= UP_MAX)
        possible_directions.push_back(Direction::NorthWest);
    // To the lower left
    if(offsetSite.x - FREEWALK_LENGTH_SLANTWISE[waiting_walk] >= LEFT_MAX
       && offsetSite.y + FREEWALK_LENGTH_SLANTWISE[waiting_walk] <= DOWN_MAX)
        possible_directions.push_back(Direction::SouthWest);
    // To the upper right
    if(offsetSite.x + FREEWALK_LENGTH_SLANTWISE[waiting_walk] <= RIGHT_MAX
       && offsetSite.y - FREEWALK_LENGTH_SLANTWISE[waiting_walk] >= UP_MAX)
        possible_directions.push_back(Direction::NorthEast);
    // To the lower right
    if(offsetSite.x + FREEWALK_LENGTH_SLANTWISE[waiting_walk] <= RIGHT_MAX
       && offsetSite.y + FREEWALK_LENGTH_SLANTWISE[waiting_walk] <= DOWN_MAX)
        possible_directions.push_back(Direction::SouthEast);

    RTTR_Assert(!possible_directions.empty());
    // Pick a random direction from these
    FaceDir(RANDOM_ELEMENT(possible_directions));

    // And walk there
    current_ev = GetEvMgr().AddEvent(this, (state == BuilderState::WaitingFreewalk) ? 24 : 17, 1);

    // Calculate the future spot
    nextOffsetSite = offsetSite;

    switch(GetCurMoveDir())
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

void nofBuilder::Draw(DrawPoint drawPt)
{
    switch(state)
    {
        case BuilderState::FigureWork:
        {
            DrawWalkingBobJobs(drawPt, Job::Builder);
        }
        break;
        case BuilderState::BuildFreewalk:
        case BuilderState::WaitingFreewalk:
        {
            // Interpolate and add the construction site's door point
            drawPt.x += GAMECLIENT.Interpolate(offsetSite.x, nextOffsetSite.x, current_ev);
            drawPt.y += GAMECLIENT.Interpolate(offsetSite.y, nextOffsetSite.y, current_ev);
            drawPt += building_site->GetDoorPoint();

            LOADER
              .getBobSprite(building_site->GetNation(), Job::Builder, GetCurMoveDir(),
                            GAMECLIENT.Interpolate(12, current_ev) % 8u)
              .draw(drawPt, COLOR_WHITE, world->GetPlayer(player).color);
        }
        break;
        case BuilderState::Build:
        {
            const unsigned index = GAMECLIENT.Interpolate(28, current_ev);
            unsigned texture;
            unsigned soundId = 0;

            // Depending on how far left/right or centered the builder stands, the animation is chosen accordingly
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
                    // He kneels
                    texture = 283 + index % 4;
                    soundId = 72;
                }
            } else
            {
                // In the middle with "hands"
                texture = 287 + (index / 2) % 4;
            }
            drawPt += building_site->GetDoorPoint() + DrawPoint(offsetSite);
            LOADER.GetPlayerImage("rom_bobs", texture)
              ->DrawFull(drawPt, COLOR_WHITE, world->GetPlayer(building_site->GetPlayer()).color);
            if(soundId && index % 4 == 2)
                world->GetSoundMgr().playNOSound(soundId, *this, index, 160 - rand() % 60);
        }
        break;
    }
}

bool nofBuilder::ChooseWare()
{
    // Do I need a board (shell stage or when no stone is required) or stone?
    const BuildingCost costs = BUILDING_COSTS[building_site->GetBuildingType()];
    if(building_site->GetBuildProgress(false) < costs.boards * 8 || !costs.stones)
    {
        // Board
        if(building_site->boards)
        {
            // One fewer board is lying there
            --building_site->boards;
            ++building_site->used_boards;
            // We can execute 8 construction steps
            building_steps_available = 8;

            return true;
        }
    } else
    {
        // Stone
        if(building_site->stones)
        {
            // One fewer stone is lying there
            --building_site->stones;
            ++building_site->used_stones;
            // We can execute 8 construction steps
            building_steps_available = 8;

            return true;
        }
    }

    return false;
}
