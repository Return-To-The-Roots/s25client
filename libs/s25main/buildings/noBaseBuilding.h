// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "nodeObjs/noRoadNode.h"
#include "gameTypes/BuildingQuality.h"
#include "gameTypes/BuildingType.h"
#include "gameTypes/JobTypes.h"
#include "gameTypes/Nation.h"

class ITexture;
class noFlag;
class SerializedGameData;
class Ware;
class noFigure;

class noBaseBuilding : public noRoadNode
{
protected:
    /// Type of the building
    BuildingType bldType_;

    /// Nation of the building (stored separately because foreign buildings can be conquered)
    const Nation nation;
    /// Door points indicating where the door is and how far carriers may go
    int door_point_x;
    int door_point_y;
    /// Game frame when construction began
    unsigned buildStartingFrame;
    /// Game frame when construction completed
    unsigned buildCompleteFrame;

    /// Notify a ware that it no longer needs to come here
    void WareNotNeeded(Ware* ware);
    /// Destroy extensions if this is a large building that has them
    void DestroyBuildingExtensions();

public:
    noBaseBuilding(NodalObjectType nop, BuildingType type, MapPoint pos, unsigned char player);
    noBaseBuilding(SerializedGameData& sgd, unsigned obj_id);

    ~noBaseBuilding() override;

    void Destroy() override;
    void Serialize(SerializedGameData& sgd) const override;

    /// A requested ware could not arrive
    virtual void WareLost(Ware& ware) = 0;

    BuildingQuality GetSize() const;
    BuildingType GetBuildingType() const { return bldType_; }
    Nation GetNation() const { return nation; }
    BlockingManner GetBM() const override;

    /// Return the radius in which this building holds land
    virtual unsigned GetMilitaryRadius() const = 0;

    /// Determine the flag in front of the building
    noFlag* GetFlag() const;
    /// Same as GetFlag()->GetPos()
    MapPoint GetFlagPos() const;

    /// Return the offset of the door, which is also where people disappear into the building, the builder is building
    /// and the wares are lying
    Position GetDoorPoint() { return Position(GetDoorPointX(), GetDoorPointY()); }
    int GetDoorPointX();
    int GetDoorPointY() const { return door_point_y; }

    /*/// Returns the distribution score for wares (0 if no path was found)
    virtual unsigned CalcDistributionPoints(noRoadNode * start,const GoodType type) = 0;*/
    /// Called when a new ware is delivered to the building (not when it is merely ordered)
    virtual void TakeWare(Ware* ware) = 0;
    /// Called when a specific worker was requested for this building
    virtual void GotWorker(Job /*job*/, noFigure& /*worker*/){};

    unsigned GetBuildStartingFrame() const { return buildStartingFrame; }
    unsigned GetBuildCompleteFrame() const { return buildCompleteFrame; }
    void SetBuildStartingFrame(unsigned frame) { buildStartingFrame = frame; }
    void SetBuildCompleteFrame(unsigned frame) { buildCompleteFrame = frame; }

    /// Return the standard building texture
    ITexture& GetBuildingImage() const;
    static ITexture& GetBuildingImage(BuildingType type, Nation nation);
    /// Return the texture for the building door
    ITexture& GetDoorImage() const;
};
