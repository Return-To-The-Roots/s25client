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
    void wareNotNeeded(Ware* ware);
    /// Zerstört Anbauten, falls es sich um ein großes Gebäude handelt (wo es diese auch gibt)
    void destroyBuildingExtensions();

public:
    noBaseBuilding(NodalObjectType nop, BuildingType type, MapPoint pos, unsigned char player);
    noBaseBuilding(SerializedGameData& sgd, unsigned obj_id);

    ~noBaseBuilding() override;

    void destroy() override;
    void Serialize(SerializedGameData& sgd) const override;

    /// Eine bestellte Ware konnte doch nicht kommen
    virtual void wareLost(Ware& ware) = 0;

    BuildingQuality getSize() const;
    BuildingType getBuildingType() const { return bldType_; }
    Nation getNation() const { return nation; }
    BlockingManner getBM() const override;

    /// Return the radius in which this building holds land
    virtual unsigned getMilitaryRadius() const = 0;

    /// Ermittelt die Flagge, die vor dem Gebäude steht
    noFlag* getFlag() const;
    /// Same as GetFlag()->GetPos()
    MapPoint getFlagPos() const;
    /// Is the building (flag) connected to anything?
    bool isConnected() const;

    /// Return the offset of the door, which is also where people disappear into the building, the builder is building
    /// and the wares are lying
    Position getDoorPoint() { return Position(getDoorPointX(), getDoorPointY()); }
    int getDoorPointX();
    int getDoorPointY() const { return door_point_y; }

    /*/// Gibt die Warenverteilungspunkte zurück (bei 0 wurde kein Weg gefunden)
    virtual unsigned CalcDistributionPoints(noRoadNode * start,const GoodType type) = 0;*/
    /// Wird aufgerufen, wenn eine neue Ware zum dem Gebäude geliefert wird (nicht wenn sie bestellt wurde vom Gebäude!)
    virtual void takeWare(Ware* ware) = 0;
    /// Wird aufgerufen, wenn ein bestimmter Arbeiter für das hier gerufen wurde
    virtual void gotWorker(Job /*job*/, noFigure& /*worker*/){};

    /// Gibt ein Bild zurück für das normale Gebäude
    ITexture& getBuildingImage() const;
    static ITexture& getBuildingImage(BuildingType type, Nation nation);
    /// Gibt ein Bild zurück für die Tür des Gebäudes
    ITexture& getDoorImage() const;
};
