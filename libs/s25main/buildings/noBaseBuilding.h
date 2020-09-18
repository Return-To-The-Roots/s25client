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

#pragma once

#include "nodeObjs/noRoadNode.h"
#include "gameTypes/BuildingQuality.h"
#include "gameTypes/BuildingType.h"
#include "gameTypes/JobTypes.h"
#include "gameTypes/Nation.h"

class glArchivItem_Bitmap;
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

    /// Aufräummethoden
protected:
    void Destroy_noBaseBuilding();

public:
    void Destroy() override { Destroy_noBaseBuilding(); }

    /// Serialisierungsfunktionen
protected:
    void Serialize_noBaseBuilding(SerializedGameData& sgd) const;

public:
    void Serialize(SerializedGameData& sgd) const override { Serialize_noBaseBuilding(sgd); }

    /// Eine bestellte Ware konnte doch nicht kommen
    virtual void WareLost(Ware* ware) = 0;

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
    virtual void GotWorker(Job /*job*/, noFigure* /*worker*/){};

    /// Gibt ein Bild zurück für das normale Gebäude
    ITexture* GetBuildingImage() const;
    static ITexture* GetBuildingImage(BuildingType type, Nation nation);
    /// Gibt ein Bild zurück für das Gebäudegerüst
    /// Gibt ein Bild zurück für die Tür des Gebäudes
    glArchivItem_Bitmap* GetDoorImage() const;
};
