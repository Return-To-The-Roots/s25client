// $Id: BurnedWarehouse.h 9357 2014-04-25 15:35:25Z FloSoft $
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
#ifndef BURNED_WAREHOUSE_H_
#define BURNED_WAREHOUSE_H_

#include "noCoordBase.h"

/// Unsichtbares Objekt, welches die fliehenden Leute aus einem ehemaligen abgebrannten Lagerhaus/HQ spuckt
class BurnedWarehouse : public noCoordBase
{
        /// Spieler des ehemaligen Lagerhauses
        const unsigned char player;
        /// Aktuelle Rausgeh-Phase
        unsigned go_out_phase;
        // Leute, die noch rauskommen müssen
        unsigned people[30];

    public:

        BurnedWarehouse(const unsigned short x, const unsigned short y, const unsigned char player, const unsigned* people);
        BurnedWarehouse(SerializedGameData* sgd, const unsigned obj_id);

        ~BurnedWarehouse();

        void Destroy(void);


        /// Serialisierungsfunktionen
    protected:  void Serialize_BurnedWarehouse(SerializedGameData* sgd) const;
    public:     void Serialize(SerializedGameData* sgd) const { Serialize_BurnedWarehouse(sgd); }

        GO_Type GetGOT() const { return GOT_BURNEDWAREHOUSE; }

        /// Benachrichtigen, wenn neuer GF erreicht wurde.
        void HandleEvent(const unsigned int id);

        void Draw(int x, int y) {}
};

#endif
