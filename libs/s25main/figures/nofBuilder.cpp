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
#include "world/GameWorldGame.h"
#include "gameData/BuildingConsts.h"
#include "gameData/BuildingProperties.h"

nofBuilder::nofBuilder(const MapPoint pos, const unsigned char player, noRoadNode* building_site)
    : noFigure(JOB_BUILDER, pos, player, building_site), state(STATE_FIGUREWORK),
      building_site(static_cast<noBuildingSite*>(building_site)), building_steps_available(0)
{
    // Sind wir schon an unsere Baustelle gleich hingesetzt worden (bei Häfen)?
    if(building_site)
    {
        if(pos == building_site->GetPos())
        {
            // Dann gleich mit dem Bauprozess beginnen
            fs = FS_JOB;
            GoalReached();
        }
    }
}

void nofBuilder::Serialize_nofBuilder(SerializedGameData& sgd) const
{
    Serialize_noFigure(sgd);

    sgd.PushUnsignedChar(static_cast<unsigned char>(state));
    sgd.PushObject(building_site, true);
    sgd.PushPoint(offsetSite);
    sgd.PushPoint(nextOffsetSite);
    sgd.PushUnsignedChar(building_steps_available);
}

nofBuilder::nofBuilder(SerializedGameData& sgd, const unsigned obj_id)
    : noFigure(sgd, obj_id), state(BuilderState(sgd.PopUnsignedChar())), building_site(sgd.PopObject<noBuildingSite>(GOT_BUILDINGSITE)),
      offsetSite(sgd.PopPoint<short>()), nextOffsetSite(sgd.PopPoint<short>()), building_steps_available(sgd.PopUnsignedChar())
{}

void nofBuilder::GoalReached()
{
    goal_ = nullptr;
    // an der Baustelle normal anfangen zu arbeiten
    state = STATE_WAITINGFREEWALK;

    // Sind jetzt an der Baustelle
    offsetSite = Point<short>(0, 0);

    // Anfangen um die Baustelle herumzulaufen
    StartFreewalk();
}

void nofBuilder::Walked() {}

void nofBuilder::AbrogateWorkplace()
{
    if(building_site)
    {
        state = STATE_FIGUREWORK;
        building_site->Abrogate();
        building_site = nullptr;
    }
}

void nofBuilder::LostWork()
{
    building_site = nullptr;

    if(state == STATE_FIGUREWORK)
        GoHome();
    else
    {
        // Event abmelden
        GetEvMgr().RemoveEvent(current_ev);

        StartWandering();
        Wander();
        state = STATE_FIGUREWORK;
    }
}

void nofBuilder::HandleDerivedEvent(const unsigned id)
{
    RTTR_Assert(id == 1u && current_ev->id == id);
    switch(state)
    {
        case STATE_WAITINGFREEWALK:
        {
            // Platz einnehmen
            offsetSite = nextOffsetSite;

            // Ware aufnehmen, falls es eine gibt
            if(ChooseWare())
                state = STATE_BUILDFREEWALK;

            // Weiter drumrumlaufen
            StartFreewalk();
        }
        break;
        case STATE_BUILDFREEWALK:
        {
            // Platz einnehmen
            offsetSite = nextOffsetSite;

            // Gibts noch was zu bauen?
            if(building_steps_available)
            {
                // dann mal schön bauen
                current_ev = GetEvMgr().AddEvent(this, 40, 1);
                state = STATE_BUILD;
            } else if(building_site->IsBuildingComplete())
            {
                // fertig mit Bauen!
                current_ev = nullptr;

                // Baustelle abreißen und Gebäude hinsetzen

                // Gebäudetyp merken und das Volk des Gebäudes
                BuildingType building_type = building_site->GetBuildingType();
                Nation building_nation = building_site->GetNation();

                state = STATE_FIGUREWORK;

                // Baustelle abmelden
                GamePlayer& owner = gwg->GetPlayer(player);
                owner.RemoveBuildingSite(building_site);
                if(gwg->IsHarborBuildingSiteFromSea(building_site))
                    gwg->RemoveHarborBuildingSiteFromSea(building_site);

                // Remove buildingsite, but don't destroy!
                gwg->SetNO(building_site->GetPos(), nullptr);
                deletePtr(building_site);

                noBuilding* bld = BuildingFactory::CreateBuilding(*gwg, building_type, pos, player, building_nation);
                gwg->GetNotifications().publish(BuildingNote(BuildingNote::Constructed, player, pos, building_type));

                // Special handling for warehouses
                if(BuildingProperties::IsWareHouse(building_type))
                {
                    nobBaseWarehouse* wh = static_cast<nobBaseWarehouse*>(bld);
                    // Mich dort gleich einquartieren und nicht erst zurücklaufen
                    wh->AddFigure(this);
                    gwg->RemoveFigure(pos, this);

                    // Evtl Träger aus dem HQ wieder verwenden
                    owner.FindCarrierForAllRoads();
                    owner.FindWarehouseForAllJobs(JOB_HELPER);

                    // Evtl gabs verlorene Waren, die jetzt in das WH wieder reinkönnen
                    owner.FindClientForLostWares();
                    return;
                }

                // Nach Hause laufen bzw. auch rumirren
                rs_pos = 0;
                rs_dir = true;
                cur_rs = gwg->GetSpecObj<noRoadNode>(pos)->GetRoute(Direction::SOUTHEAST);

                GoHome();
                StartWalking(Direction::SOUTHEAST);
            } else
            {
                // Brauchen neues Material

                // Ware aufnehmen, falls es eine gibt
                if(!ChooseWare())
                    state = STATE_WAITINGFREEWALK;

                // Weiter drumrumlaufen
                StartFreewalk();
            }
        }
        break;
        case STATE_BUILD:
        {
            // Sounds abmelden
            SOUNDMANAGER.WorkingFinished(this);

            // ein Bauschritt weniger, Haus um eins höher
            --building_steps_available;
            ++building_site->build_progress;
            // Fertig mit dem Bauschritt, dann an nächste Position gehen
            state = STATE_BUILDFREEWALK;
            StartFreewalk();
        }
        break;
        default: break;
    }
}

// Länge, die der Bauarbeiter in einem Free-Walk zurücklegt (in Pixeln)
const short FREEWALK_LENGTH[2] = {22, 11};          // waagerecht
const short FREEWALK_LENGTH_SLANTWISE[2] = {14, 7}; // schräg

void nofBuilder::StartFreewalk()
{
    std::vector<Direction> possible_directions;

    unsigned char waiting_walk = ((state == STATE_WAITINGFREEWALK) ? 0 : 1);

    // Wohin kann der Bauarbeiter noch laufen?

    // Nach links
    if(offsetSite.x - FREEWALK_LENGTH[waiting_walk] >= LEFT_MAX)
        possible_directions.push_back(Direction::WEST);
    // Nach rechts
    if(offsetSite.x + FREEWALK_LENGTH[waiting_walk] <= RIGHT_MAX)
        possible_directions.push_back(Direction::EAST);
    // Nach links/oben
    if(offsetSite.x - FREEWALK_LENGTH_SLANTWISE[waiting_walk] >= LEFT_MAX
       && offsetSite.y - FREEWALK_LENGTH_SLANTWISE[waiting_walk] >= UP_MAX)
        possible_directions.push_back(Direction::NORTHWEST);
    // Nach links/unten
    if(offsetSite.x - FREEWALK_LENGTH_SLANTWISE[waiting_walk] >= LEFT_MAX
       && offsetSite.y + FREEWALK_LENGTH_SLANTWISE[waiting_walk] <= DOWN_MAX)
        possible_directions.push_back(Direction::SOUTHWEST);
    // Nach rechts/oben
    if(offsetSite.x + FREEWALK_LENGTH_SLANTWISE[waiting_walk] <= RIGHT_MAX
       && offsetSite.y - FREEWALK_LENGTH_SLANTWISE[waiting_walk] >= UP_MAX)
        possible_directions.push_back(Direction::NORTHEAST);
    // Nach rechts/unten
    if(offsetSite.x + FREEWALK_LENGTH_SLANTWISE[waiting_walk] <= RIGHT_MAX
       && offsetSite.y + FREEWALK_LENGTH_SLANTWISE[waiting_walk] <= DOWN_MAX)
        possible_directions.push_back(Direction::SOUTHEAST);

    RTTR_Assert(!possible_directions.empty());
    // Zufällige Richtung von diesen auswählen
    FaceDir(possible_directions[RANDOM.Rand(__FILE__, __LINE__, GetObjId(), possible_directions.size())]);

    // Und dort auch hinlaufen
    current_ev = GetEvMgr().AddEvent(this, (state == STATE_WAITINGFREEWALK) ? 24 : 17, 1);

    // Zukünftigen Platz berechnen
    nextOffsetSite = offsetSite;

    switch(GetCurMoveDir().native_value())
    {
        case Direction::WEST: nextOffsetSite.x -= FREEWALK_LENGTH[waiting_walk]; break;
        case Direction::NORTHWEST:
            nextOffsetSite.x -= FREEWALK_LENGTH_SLANTWISE[waiting_walk];
            nextOffsetSite.y -= FREEWALK_LENGTH_SLANTWISE[waiting_walk];
            break;
        case Direction::NORTHEAST:
            nextOffsetSite.x += FREEWALK_LENGTH_SLANTWISE[waiting_walk];
            nextOffsetSite.y -= FREEWALK_LENGTH_SLANTWISE[waiting_walk];
            break;
        case Direction::EAST: nextOffsetSite.x += FREEWALK_LENGTH[waiting_walk]; break;
        case Direction::SOUTHEAST:
            nextOffsetSite.x += FREEWALK_LENGTH_SLANTWISE[waiting_walk];
            nextOffsetSite.y += FREEWALK_LENGTH_SLANTWISE[waiting_walk];
            break;
        case Direction::SOUTHWEST:
            nextOffsetSite.x -= FREEWALK_LENGTH_SLANTWISE[waiting_walk];
            nextOffsetSite.y += FREEWALK_LENGTH_SLANTWISE[waiting_walk];
            break;
    }
}

void nofBuilder::Draw(DrawPoint drawPt)
{
    switch(state)
    {
        case STATE_FIGUREWORK: { DrawWalkingBobJobs(drawPt, JOB_BUILDER);
        }
        break;
        case STATE_BUILDFREEWALK:
        case STATE_WAITINGFREEWALK:
        {
            // Interpolieren und Door-Point von Baustelle draufaddieren
            drawPt.x += GAMECLIENT.Interpolate(offsetSite.x, nextOffsetSite.x, current_ev);
            drawPt.y += GAMECLIENT.Interpolate(offsetSite.y, nextOffsetSite.y, current_ev);
            drawPt += building_site->GetDoorPoint();

            LOADER
              .bob_jobs_cache[building_site->GetNation()][JOB_BUILDER][GetCurMoveDir().toUInt()][GAMECLIENT.Interpolate(12, current_ev) % 8]
              .draw(drawPt, COLOR_WHITE, gwg->GetPlayer(player).color);
        }
        break;
        case STATE_BUILD:
        {
            const unsigned index = GAMECLIENT.Interpolate(28, current_ev);
            unsigned texture;
            unsigned soundId = 0;

            // Je nachdem, wie weit der Bauarbeiter links bzw rechts oder in der Mitte steht, so wird auch die Animation angezeigt
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
            drawPt += building_site->GetDoorPoint() + DrawPoint(offsetSite);
            LOADER.GetPlayerImage("rom_bobs", texture)->DrawFull(drawPt, COLOR_WHITE, gwg->GetPlayer(building_site->GetPlayer()).color);
            if(soundId && index % 4 == 2)
                SOUNDMANAGER.PlayNOSound(soundId, this, index, 160 - rand() % 60);
        }
        break;
    }

    // char number[256];
    // sprintf(number,"%u",obj_id);
    // NormalFont->Draw(x,y,number,0,0xFFFF0000);
}

bool nofBuilder::ChooseWare()
{
    // Brauch ich ein Brett(Rohbau und wenn kein Stein benötigt wird) oder Stein?
    const BuildingCost costs = BUILDING_COSTS[building_site->GetNation()][building_site->GetBuildingType()];
    if(building_site->GetBuildProgress(false) < costs.boards * 8 || !costs.stones)
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
