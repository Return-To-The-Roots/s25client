// $Id: noGranite.h 9357 2014-04-25 15:35:25Z FloSoft $
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

#ifndef NO_GRANITE_H_
#define NO_GRANITE_H_

#include "noBase.h"
#include "MapConsts.h"

class noGranite : public noBase
{
        GraniteType type; // Welcher Typ ( gibt 2 )
        unsigned char state; // Status, 0 - 5, von sehr wenig bis sehr viel

    public:

        noGranite(const GraniteType type, const unsigned char state);
        noGranite(SerializedGameData* sgd, const unsigned obj_id);

        /// Aufräummethoden
    protected:  void Destroy_noGranite() { Destroy_noBase(); }
    public:     void Destroy() { Destroy_noGranite(); }

        /// Serialisierungsfunktionen
    protected:  void Serialize_noGranite(SerializedGameData* sgd) const;
    public:     void Serialize(SerializedGameData* sgd) const { Serialize_noGranite(sgd); }

        GO_Type GetGOT() const { return GOT_GRANITE; }

        void Draw(int x, int y);

        BlockingManner GetBM() const { return BM_GRANITE; }

        /// Erzeugt von ihnen selbst ein FOW Objekt als visuelle "Erinnerung" für den Fog of War
        FOWObject* CreateFOWObject() const;

        /// "Bearbeitet" den Granitglotz --> haut ein Stein ab
        void Hew();
        /// Gibt true zurück, falls der Granitblock nur noch 1 Stein groß ist und damit dann vernichtet werden kann
        bool IsSmall() const { return (state == 0); }

};


#endif
