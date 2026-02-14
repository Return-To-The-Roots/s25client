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
    /// Typ des Gebäudes
    BuildingType bldType_;

    /// Volk des Gebäudes (muss extra gespeichert werden, da ja auch z.B. fremde Gebäude erobert werden können)
    const Nation nation;
    /// Doorpoints - Punkte, wo die Tür ist, bis wohin die Träger gehen dürfen
    int door_point_x;
    int door_point_y;

    /// Ware Bescheid sagen, dass sie nicht mehr hierher kommen brauch
    void WareNotNeeded(Ware* ware);
    /// Zerstört Anbauten, falls es sich um ein großes Gebäude handelt (wo es diese auch gibt)
    void DestroyBuildingExtensions();

public:
    noBaseBuilding(NodalObjectType nop, BuildingType type, MapPoint pos, unsigned char player);
    noBaseBuilding(SerializedGameData& sgd, unsigned obj_id);

    ~noBaseBuilding() override;

    void Destroy() override;
    void Serialize(SerializedGameData& sgd) const override;

    /// Eine bestellte Ware konnte doch nicht kommen
    virtual void WareLost(Ware& ware) = 0;

    BuildingQuality GetSize() const;
    BuildingType GetBuildingType() const { return bldType_; }
    Nation GetNation() const { return nation; }
    BlockingManner GetBM() const override;

    /// Return the radius in which this building holds land
    virtual unsigned GetMilitaryRadius() const = 0;

    /// Ermittelt die Flagge, die vor dem Gebäude steht
    noFlag* GetFlag() const;
    /// Same as GetFlag()->GetPos()
    MapPoint GetFlagPos() const;
    /// Is the building (flag) connected to anything?
    bool IsConnected() const;

    /// Return the offset of the door, which is also where people disappear into the building, the builder is building
    /// and the wares are lying
    Position GetDoorPoint() { return Position(GetDoorPointX(), GetDoorPointY()); }
    int GetDoorPointX();
    int GetDoorPointY() const { return door_point_y; }

    /*/// Gibt die Warenverteilungspunkte zurück (bei 0 wurde kein Weg gefunden)
    virtual unsigned CalcDistributionPoints(noRoadNode * start,const GoodType type) = 0;*/
    /// Wird aufgerufen, wenn eine neue Ware zum dem Gebäude geliefert wird (nicht wenn sie bestellt wurde vom Gebäude!)
    virtual void TakeWare(Ware* ware) = 0;
    /// Wird aufgerufen, wenn ein bestimmter Arbeiter für das hier gerufen wurde
    virtual void GotWorker(Job /*job*/, noFigure& /*worker*/){};

    /// Gibt ein Bild zurück für das normale Gebäude
    ITexture& GetBuildingImage() const;
    static ITexture& GetBuildingImage(BuildingType type, Nation nation);
    /// Gibt ein Bild zurück für die Tür des Gebäudes
    ITexture& GetDoorImage() const;
};
