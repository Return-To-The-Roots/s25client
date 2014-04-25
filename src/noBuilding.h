// $Id: noBuilding.h 9357 2014-04-25 15:35:25Z FloSoft $
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

#ifndef NO_BUILDING_H_
#define NO_BUILDING_H_

#include "noBaseBuilding.h"
#include "noFlag.h"
#include "BuildingConsts.h"

class noBuilding : public noBaseBuilding
{
    protected:

        /// Gibt an, wie viele Leute die Tür geöffnet haben (wenns 0 ist, ist die Tür zu, ansonsten offen)
        unsigned char opendoor;

    public:


        noBuilding(const BuildingType type, const unsigned short x, const unsigned short y, const unsigned char player, const Nation nation);
        noBuilding(SerializedGameData* sgd, const unsigned obj_id);

        /// Aufräummethoden
    protected:  void Destroy_noBuilding();
    public:     void Destroy() { Destroy_noBuilding(); }

        /// Serialisierungsfunktionen
    protected:  void Serialize_noBuilding(SerializedGameData* sgd) const;
    public:     void Serialize(SerializedGameData* sgd) const { Serialize_noBuilding(sgd); }

        /// Zeichnet das Gebäude an sich mit Tür ohne zusätzlichen Schnickschnack
        void DrawBaseBuilding(int x, int y);

        void OpenDoor() {++opendoor;}
        void CloseDoor() {--opendoor;}

        void GotWorker(Job job, noFigure* worker);

        /// Wird aufgerufen, wenn von der Fahne vor dem Gebäude ein Rohstoff aufgenommen wurde
        virtual bool FreePlaceAtFlag() = 0;

        /// Erzeugt von ihnen selbst ein FOW Objekt als visuelle "Erinnerung" für den Fog of War
        FOWObject* CreateFOWObject() const;
};


/// Prüft, ob es sich um ein Gebäude handelt
inline bool IsBuilding(const GO_Type got)
{ return (got >= GOT_NOB_HQ && got <= GOT_NOB_USUAL); }
/// Prüft, ob es sich um ein Gebäude oder eine Baustelle handelt
inline bool IsBaseBuilding(const GO_Type got)
{ return (got >= GOT_NOB_HQ && got <= GOT_BUILDINGSITE); }

#endif
