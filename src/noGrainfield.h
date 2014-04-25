// $Id: noGrainfield.h 9357 2014-04-25 15:35:25Z FloSoft $
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

#ifndef NO_GRAINFIELD_H_
#define NO_GRAINFIELD_H_

#include "noCoordBase.h"
#include "EventManager.h"

class noGrainfield : public noCoordBase
{
    private:

        /// Typ des Getreidefelds (2 verschiedene Typen)
        unsigned char type;

        /// Status
        enum State
        {
            STATE_GROWING_WAITING, /// Wachsphase, wartet auf den nächsten Wachstumsschub
            STATE_GROWING, /// wächst
            STATE_NORMAL, /// ist ausgewachsen und verdorrt nach einer Weile
            STATE_WITHERING /// verdorrt (verschwindet)
        } state;

        /// Größe des Feldes (0-3), 3 ist ausgewachsen
        unsigned char size;

        /// Wachs-Event
        EventManager::EventPointer event;

    public:

        noGrainfield(const unsigned short x, const unsigned short y);
        noGrainfield(SerializedGameData* sgd, const unsigned obj_id);

        ~noGrainfield();

        /// Aufräummethoden
    protected:  void Destroy_noGrainfield();
    public:     void Destroy() { Destroy_noGrainfield(); }

        /// Serialisierungsfunktionen
    protected:  void Serialize_noGrainfield(SerializedGameData* sgd) const;
    public:     void Serialize(SerializedGameData* sgd) const { Serialize_noGrainfield(sgd); }

        GO_Type GetGOT() const { return GOT_GRAINFIELD; }

        void Draw(int x, int y);
        void HandleEvent(const unsigned int id);

        BlockingManner GetBM() const { return BM_GRANITE; }

        /// Kann man es abernten?
        bool IsHarvestable() const { return size == 3 && state == STATE_NORMAL;}

        /// Gibt die ID des abgeernteten Getreidefelds in der map_last zurück
        unsigned GetHarvestMapLstID() const { return 532 + type * 5 + 4; }

        /// Bauer beginnt dieses Feld abzuernten
        void BeginHarvesting();
        /// Bauer wird beim Abernten unterbrochen
        void EndHarvesting();

};


#endif
