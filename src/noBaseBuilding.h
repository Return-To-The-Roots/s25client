// $Id: noBaseBuilding.h 9357 2014-04-25 15:35:25Z FloSoft $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef NO_BASEBUILDING_H_
#define NO_BASEBUILDING_H_

#include "noRoadNode.h"
#include "noFlag.h"
#include "BuildingConsts.h"


class noBaseBuilding : public noRoadNode
{
    protected:

        /// Typ des Gebäudes
        BuildingType type;

        /// Volk des Gebäudes (muss extra gespeichert werden, da ja auch z.B. fremde Gebäude erobert werden können)
        const Nation nation;
        /// Doorpoints - Punkte, wo die Tür ist, bis wohin die Träger gehen dürfen
        int door_point_x;
        int door_point_y;

    protected:

        /// Ware Bescheid sagen, dass sie nicht mehr hierher kommen brauch
        void WareNotNeeded(Ware* ware);
        /// Zerstört Anbauten, falls es sich um ein großes Gebäude handelt (wo es diese auch gibt)
        void DestroyBuildingExtensions();

    public:

        noBaseBuilding(const NodalObjectType nop, const BuildingType type, const unsigned short x, const unsigned short y, const unsigned char player);
        noBaseBuilding(SerializedGameData* sgd, const unsigned obj_id);

        virtual ~noBaseBuilding();

        /// Aufräummethoden
    protected:  void Destroy_noBaseBuilding();
    public:     void Destroy() { Destroy_noBaseBuilding(); }

        /// Serialisierungsfunktionen
    protected:  void Serialize_noBaseBuilding(SerializedGameData* sgd) const;
    public:     void Serialize(SerializedGameData* sgd) const { Serialize_noBaseBuilding(sgd); }

        /// Eine bestellte Ware konnte doch nicht kommen
        virtual void WareLost(Ware* ware) = 0;

        /// Gibt diverse Sachen zurück
        BuildingQuality GetSize() const { return BUILDING_SIZE[type]; }
        BuildingType GetBuildingType () const { return type; }
        Nation GetNation() const { return nation; }
        virtual BlockingManner GetBM() const;

        /// Harbor, storehouse or headquarters?
        bool IsWarehouse() const
        {
            return (GetBuildingType() == BLD_HEADQUARTERS ||
                    GetBuildingType() == BLD_STOREHOUSE || GetBuildingType() == BLD_HARBORBUILDING);
        }

        /// Ermittelt die Flagge, die vor dem Gebäude steht
        noFlag* GetFlag() const;

        short GetDoorPointX();
        short GetDoorPointY() const { return (door_point_y & 0xFFFF); }

        /*/// Gibt die Warenverteilungspunkte zurück (bei 0 wurde kein Weg gefunden)
        virtual unsigned CalcDistributionPoints(noRoadNode * start,const GoodType type) = 0;*/
        /// Wird aufgerufen, wenn eine neue Ware zum dem Gebäude geliefert wird (nicht wenn sie bestellt wurde vom Gebäude!)
        virtual void TakeWare(Ware* ware) = 0;
        /// Wird aufgerufen, wenn ein bestimmter Arbeiter für das hier gerufen wurde
        virtual void GotWorker(Job job, noFigure* worker) {};


        /// Gibt ein Bild zurück für das normale Gebäude
        glArchivItem_Bitmap* GetBuildingImage() const;
        /// Gibt ein Bild zurück für das Gebäudegerüst
        glArchivItem_Bitmap* GetBuildingSkeletonImage() const;
        /// Gibt ein Bild zurück für das normale Gebäude
        glArchivItem_Bitmap* GetBuildingImageShadow() const;
        /// Gibt ein Bild zurück für das Gebäudegerüst
        glArchivItem_Bitmap* GetBuildingSkeletonImageShadow() const;
        /// Gibt ein Bild zurück für die Tür des Gebäudes
        glArchivItem_Bitmap* GetDoorImage() const;

};


#endif

