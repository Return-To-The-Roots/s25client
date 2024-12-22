// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "DrawPoint.h"
#include "GameObject.h"
#include "NodalObjectTypes.h"
#include <memory>

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
    virtual std::unique_ptr<FOWObject> CreateFOWObject() const;

    virtual BlockingManner GetBM() const;
    /// Gibt zurück, ob sich das angegebene Objekt zwischen zwei Punkten bewegt
    virtual bool IsMoving() const;

private:
    NodalObjectType nop; /// Typ des NodeObjekt ( @see NodalObjectTypes.h )
protected:
    noBase(const noBase&) = default;
};
