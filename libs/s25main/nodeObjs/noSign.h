// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "noDisappearingEnvObject.h"
#include "gameTypes/Resource.h"
class SerializedGameData;

/// Stellt ein Ressourcen-Schild dar
class noSign : public noDisappearingEnvObject
{
public:
    noSign(MapPoint pos, Resource resource);
    noSign(SerializedGameData& sgd, unsigned obj_id);

    void serialize(SerializedGameData& sgd) const override;

    GO_Type getGOT() const final { return GO_Type::Sign; }

    /// An x,y zeichnen.
    void draw(DrawPoint drawPt) override;

    /// Return the resources found, as drawn on the sign.
    Resource getResource() const { return resource; }

private:
    Resource resource;
};
