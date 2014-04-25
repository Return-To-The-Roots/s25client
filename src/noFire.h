// $Id: noFire.h 9357 2014-04-25 15:35:25Z FloSoft $
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

#ifndef NOP_FIRE_H_
#define NOP_FIRE_H_

#include "noCoordBase.h"
#include "EventManager.h"

// Klasse für ein brennendes Gebäude
class noFire : public noCoordBase
{
        /// Größe des Feuers: klein (0) oder groß (1)
        unsigned char size;
        /// "Todesevent" (also bis es abgebrannt ist) speichern, damit dann interpoliert wird
        EventManager::EventPointer dead_event;
        /// Wurden Feuersounds abgespielt
        bool was_sounding;
        /// Letzter Feuersound-Zeitpunkt
        unsigned last_sound;
        /// Intervall zum nächsten Feuersound
        unsigned next_interval;


    public:

        noFire(const unsigned short x, const unsigned short y, const unsigned char size);
        noFire(SerializedGameData* sgd, const unsigned obj_id);

        ~noFire();
        /// Aufräummethoden
    protected:  void Destroy_noFire();
    public:     void Destroy() { Destroy_noFire(); }

        /// Serialisierungsfunktionen
    protected:  void Serialize_noFire(SerializedGameData* sgd) const;
    public:     void Serialize(SerializedGameData* sgd) const { Serialize_noFire(sgd); }

        GO_Type GetGOT() const { return GOT_FIRE; }

        BlockingManner GetBM() const { return BM_GRANITE; }

        /// Zeichnen
        void Draw(int x, int y);
        /// Benachrichtigen, wenn neuer gf erreicht wurde
        void HandleEvent(const unsigned int id);

};

#endif
