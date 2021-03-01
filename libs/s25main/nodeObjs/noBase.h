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

#include "DrawPoint.h"
#include "GameObject.h"
#include "NodalObjectTypes.h"

class FOWObject;
class SerializedGameData;

/// How does an object influence other objects/Building quality
enum class BlockingManner
{
    None,         /// Does not block and can be removed
    Flag,         /// Is a flag (Block pt and no flags around)
    Building,     /// Is a building (Like Single, but special handling in BQ calculation)
    Single,       /// Blocks only the point this is on
    Tree,         /// Is a tree. Passable by figures but allows only huts around
    FlagsAround,  /// Allow only flags around
    NothingAround /// Allow nothing around
};

class noBase : public GameObject
{
public:
    noBase(const NodalObjectType nop) : nop(nop) {}
    noBase(SerializedGameData& sgd, unsigned obj_id);

    /// An x,y zeichnen.
    virtual void Draw(DrawPoint drawPt) = 0;

    /// Type zurückgeben.
    NodalObjectType GetType() const { return nop; }

    void Serialize(SerializedGameData& sgd) const override;

    /// Erzeugt von ihnen selbst ein FOW Objekt als visuelle "Erinnerung" für den Fog of War
    virtual FOWObject* CreateFOWObject() const;

    virtual BlockingManner GetBM() const;
    /// Gibt zurück, ob sich das angegebene Objekt zwischen zwei Punkten bewegt
    virtual bool IsMoving() const;

private:
    NodalObjectType nop; /// Typ des NodeObjekt ( @see NodalObjectTypes.h )
};
