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
#ifndef NOBASE_H_INCLUDED
#define NOBASE_H_INCLUDED

#pragma once

#include "GameObject.h"
#include "NodalObjectTypes.h"

class FOWObject;
class SerializedGameData;

class noBase : public GameObject
{
    public:
        noBase(const NodalObjectType nop) : nop(nop) {}
        noBase(SerializedGameData& sgd, const unsigned obj_id);

        /// An x,y zeichnen.
        virtual void Draw(int x, int y) = 0;

        /// Type zurückgeben.
        NodalObjectType GetType() const { return nop; }
        /// Serialisierungsfunktion.
        void Serialize(SerializedGameData& sgd) const override { Serialize_noBase(sgd); }

        /// Erzeugt von ihnen selbst ein FOW Objekt als visuelle "Erinnerung" für den Fog of War
        virtual FOWObject* CreateFOWObject() const;

        /// Gibt an, inwieweit ein Objekt auf der Karte die BQ beeinflusst
        enum BlockingManner
        {
            BM_NOTBLOCKING, // blockiert gar nicht (z.B. Zierobjekte)
            BM_HUT,
            BM_HOUSE,
            BM_CASTLE,
            BM_MINE,
            BM_SINGLEBLOCKING, // Blockiert nur einzelnen Punkt, hat aber sonst keinen weiteren Einfluss auf Umgebung
            BM_GRANITE,
            BM_TREE,
            BM_FLAG,
            BM_CHARBURNERPILE
        };

        virtual BlockingManner GetBM() const;
        /// Gibt zurück, ob sich das angegebene Objekt zwischen zwei Punkten bewegt
        virtual bool IsMoving() const;


    protected:
        /// Räumt das Basisobjekt auf.
        void Destroy_noBase() {}
        /// serialisiert das Basisobjekt.
        void Serialize_noBase(SerializedGameData& sgd) const;

    protected:
        NodalObjectType nop; /// Typ des NodeObjekt ( @see NodalObjectTypes.h )
};

#endif // !NOBASE_H_INCLUDED
