// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "noDisappearingEnvObject.h"
class SerializedGameData;

/// Verschwindendes Umwelt-Objekt ohne weiter Bedeutung (z.b. Baumstamm etc.)
class noDisappearingMapEnvObject : public noDisappearingEnvObject
{
public:
    noDisappearingMapEnvObject(MapPoint pos, unsigned short map_id);
    noDisappearingMapEnvObject(SerializedGameData& sgd, unsigned obj_id);

    void Serialize(SerializedGameData& sgd) const override;

    GO_Type GetGOT() const final { return GO_Type::Disappearingmapenvobject; }

    /// An x,y zeichnen.
    void Draw(DrawPoint drawPt) override;

private:
    /// ID in der mapsx.lst
    const unsigned short map_id;
};
