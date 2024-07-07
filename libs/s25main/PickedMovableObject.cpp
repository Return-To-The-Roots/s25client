// Copyright (C) 2005 - 2023 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "PickedMovableObject.h"
#include "desktops/dskGameInterface.h"
#include "drivers/VideoDriverWrapper.h"
#include "world/GameWorldBase.h"
#include "world/GameWorldView.h"
#include "nodeObjs/noMovable.h"
#include <cmath>

using namespace std::chrono_literals;

namespace {
constexpr unsigned PickRadius = 2;
constexpr unsigned TrackRadius = 5;
constexpr auto PickedObjectExpiration = 5s;

// Make (ab-)use of CheckPointsInRadius() easier
constexpr bool CheckPointsBreak = true;
constexpr bool CheckPointsContinue = false;
} // namespace

PickedMovableObject PickedMovableObject::pick(const GameWorldView& gwv, MapPoint mapPt, DrawPoint drawPt, bool expire)
{
    // DEBUG REMOVE BEFORE MERGE
    unsigned i = 1;

    const auto offset = gwv.GetOffset();
    const auto center = gwv.GetSize() / 2.f;
    const auto zoomFactor = gwv.GetZoomFactor();
    const auto& world = gwv.GetWorld();
    const auto worldSize = world.GetSize() * DrawPoint(TR_W, TR_H);
    auto minDistance = std::numeric_limits<float>::max();

    PickedMovableObject pmo;

    world.CheckPointsInRadius(
      mapPt, PickRadius,
      [&](MapPoint curPt, unsigned) {
          if(gwv.GetViewer().GetVisibility(curPt) != Visibility::Visible)
              return CheckPointsContinue;

          DrawPoint curDrawPt = world.GetNodePos(curPt);
          if(curDrawPt.x < offset.x)
              curDrawPt.x += worldSize.x;
          if(curDrawPt.y < offset.y)
              curDrawPt.y += worldSize.y;
          curDrawPt -= offset;
          for(const noBase& obj : world.GetFigures(curPt))
          {
              const auto* movable = dynamic_cast<const noMovable*>(&obj);
              if(!movable)
                  continue;

              DrawPoint objDrawPt = curDrawPt;
              if(movable->IsMoving())
                  objDrawPt += movable->CalcWalkingRelative();
              objDrawPt = DrawPoint((objDrawPt - center) * zoomFactor + center);

              // DEBUG REMOVE BEFORE MERGE
              dskGameInterface::SetDebugPoint(i++, objDrawPt, 8, 4, MakeColor(255, 255, 0, 255));

              auto diff = Point<float>(objDrawPt - drawPt);
              float distance = std::sqrt(diff.x * diff.x + diff.y * diff.y);
              if(distance < minDistance)
              {
                  pmo.id_ = movable->GetObjId();
                  pmo.mapPt_ = curPt;
                  pmo.drawPt_ = objDrawPt; // TODO Do we need the unwrapped point here or was this always wrong?
                  minDistance = distance;
              }
          }

          return CheckPointsContinue;
      },
      true);

    if(pmo.isValid() && expire)
        pmo.expiration_.start();

    return pmo;
}

PickedMovableObject PickedMovableObject::pickAtCursor(const GameWorldView& gwv, bool expire)
{
    return pick(gwv, gwv.GetSelectedPt(), DrawPoint(VIDEODRIVER.GetMousePos()), expire);
}

PickedMovableObject PickedMovableObject::pickAtViewCenter(const GameWorldView& gwv, bool expire)
{
    const auto centerMapPt = gwv.GetWorld().MakeMapPoint((gwv.GetFirstPt() + gwv.GetLastPt()) / 2);
    const auto centerDrawPt = DrawPoint(gwv.GetSize() / 2u);
    return pick(gwv, centerMapPt, centerDrawPt, expire);
}

bool PickedMovableObject::isValid() const
{
    return id_ != 0 && (!expiration_.isRunning() || expiration_.getElapsed() < PickedObjectExpiration);
}

void PickedMovableObject::cancelExpiration()
{
    expiration_.stop();
}

void PickedMovableObject::invalidate()
{
    id_ = 0;
}

bool PickedMovableObject::track(const GameWorldView& gwv)
{
    if(!isValid())
        return false;

    const auto& world = gwv.GetWorld();
    const auto success = world.CheckPointsInRadius(
      mapPt_, TrackRadius,
      [&](MapPoint curPt, unsigned) {
          if(gwv.GetViewer().GetVisibility(curPt) != Visibility::Visible)
              return CheckPointsContinue;

          for(const noBase& obj : world.GetFigures(curPt))
          {
              if(obj.GetObjId() != id_)
                  continue;

              const auto& movable = dynamic_cast<const noMovable&>(obj);
              mapPt_ = curPt;
              drawPt_ = world.GetNodePos(curPt);
              if(movable.IsMoving())
                  drawPt_ += movable.CalcWalkingRelative();

              return CheckPointsBreak;
          }
          return CheckPointsContinue;
      },
      true);

    if(!success)
        invalidate();

    return success;
}

bool PickedMovableObject::track(GameWorldView& gwv, bool moveTo)
{
    const auto success = track(gwv);
    if(success && moveTo)
        gwv.MoveTo(drawPt_ - gwv.GetSize() / 2u);
    return success;
}
