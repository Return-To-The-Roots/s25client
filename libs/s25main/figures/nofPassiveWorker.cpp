// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nofPassiveWorker.h"
#include "Loader.h"
#include "WineLoader.h"
#include "buildings/nobBaseWarehouse.h"
#include "world/GameWorld.h"
#include "s25util/colors.h"
class SerializedGameData;
class noRoadNode;

nofPassiveWorker::nofPassiveWorker(const Job job, const MapPoint pos, const unsigned char player, noRoadNode* goal)
    : noFigure(job, pos, player, goal)
{}

nofPassiveWorker::nofPassiveWorker(SerializedGameData& sgd, const unsigned obj_id) : noFigure(sgd, obj_id) {}

/// von noFigure aufgerufen
// wenn man gelaufen ist
void nofPassiveWorker::Walked() {}

// wenn das Ziel erreicht wurde
void nofPassiveWorker::GoalReached()
{
    // Mich hier einquartieren
    auto* wh = world->GetSpecObj<nobBaseWarehouse>(pos);
    RTTR_Assert(wh);
    wh->AddFigure(world->RemoveFigure(pos, *this));
}

void nofPassiveWorker::AbrogateWorkplace() {}

/// Zeichnen
void nofPassiveWorker::Draw(DrawPoint drawPt)
{
    switch(job_)
    {
        case Job::PackDonkey:
        {
            const unsigned ani_step = CalcWalkAnimationFrame();
            drawPt = InterpolateWalkDrawPos(drawPt);

            LOADER.GetMapTexture(2000 + rttr::enum_cast(GetCurMoveDir() + 3u) * 8 + ani_step)->DrawFull(drawPt);
            LOADER.GetMapTexture(2048 + rttr::enum_cast(GetCurMoveDir()) % 3)->DrawFull(drawPt, COLOR_SHADOW);
        }
        break;
        case Job::CharBurner: DrawWalking(drawPt, "charburner_bobs", 53); break;
        case Job::Vintner:
            DrawWalking(drawPt, "wine_bobs", wineaddon::bobIndex[wineaddon::BobTypes::VINTNER_WALKING]);
            break;
        case Job::Winegrower:
            DrawWalking(drawPt, "wine_bobs", wineaddon::bobIndex[wineaddon::BobTypes::WINEGROWER_WALKING_WITH_SHOVEL]);
            break;
        case Job::TempleServant:
            DrawWalking(drawPt, "wine_bobs", wineaddon::bobIndex[wineaddon::BobTypes::TEMPLESERVANT_WALKING]);
            break;
        default: DrawWalkingBobJobs(drawPt, job_); break;
    }
}

/// Für alle restlichen Events, die nicht von noFigure behandelt werden
void nofPassiveWorker::HandleDerivedEvent(const unsigned /*id*/) {}
