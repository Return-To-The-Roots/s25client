// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef NO_BUILDING_H_
#define NO_BUILDING_H_

#include "noBaseBuilding.h"
class FOWObject;
class SerializedGameData;
class noFigure;

class noBuilding : public noBaseBuilding
{
    protected:

        /// How many people opened the door. positive: open, 0: closed, negative: error
        signed char opendoor;

        noBuilding(const BuildingType type, const MapPoint pt, const unsigned char player, const Nation nation);
        noBuilding(SerializedGameData& sgd, const unsigned obj_id);

        /// Aufräummethoden
    protected:  void Destroy_noBuilding();
    public:     void Destroy() override { Destroy_noBuilding(); }

        /// Serialisierungsfunktionen
    protected:  void Serialize_noBuilding(SerializedGameData& sgd) const;
    public:     void Serialize(SerializedGameData& sgd) const override { Serialize_noBuilding(sgd); }

        /// Draws the basic building (no fires etc.) with the door
        void DrawBaseBuilding(int x, int y);
        /// Draws the door only (if building is drawn at x, y)
        void DrawDoor(int x, int y);

        void OpenDoor() {++opendoor;}
        void CloseDoor() {RTTR_Assert(opendoor); --opendoor;}

        void GotWorker(Job job, noFigure* worker) override;

        /// Wird aufgerufen, wenn von der Fahne vor dem Gebäude ein Rohstoff aufgenommen wurde
        virtual bool FreePlaceAtFlag() = 0;

        /// Erzeugt von ihnen selbst ein FOW Objekt als visuelle "Erinnerung" für den Fog of War
        FOWObject* CreateFOWObject() const override;
};


/// Prüft, ob es sich um ein Gebäude handelt
inline bool IsBuilding(const GO_Type got){ return (got >= GOT_NOB_HQ && got <= GOT_NOB_USUAL); }
/// Prüft, ob es sich um ein Gebäude oder eine Baustelle handelt
inline bool IsBaseBuilding(const GO_Type got){ return (got >= GOT_NOB_HQ && got <= GOT_BUILDINGSITE); }

#endif
