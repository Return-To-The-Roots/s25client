// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "noBase.h"
#include "gameTypes/MapTypes.h"
class FOWObject;
class SerializedGameData;

class noGranite final : public noBase
{
    GraniteType type;    // Welcher Typ ( gibt 2 )
    unsigned char state; // Status, 0 - 5, von sehr wenig bis sehr viel

public:
    noGranite(GraniteType type, unsigned char state);
    noGranite(SerializedGameData& sgd, unsigned obj_id);

    void destroy() override {}
    void serialize(SerializedGameData& sgd) const override;

    GO_Type getGOT() const final { return GO_Type::Granite; }

    void draw(DrawPoint drawPt) override;

    BlockingManner getBM() const override { return BlockingManner::FlagsAround; }

    /// Erzeugt von ihnen selbst ein FOW Objekt als visuelle "Erinnerung" für den Fog of War
    std::unique_ptr<FOWObject> createFOWObject() const override;

    /// "Bearbeitet" den Granitglotz --> haut ein Stein ab
    void hew();

    /// Gibt true zurück, falls der Granitblock nur noch 1 Stein groß ist und damit dann vernichtet werden kann
    bool isSmall() const { return (state == 0); }

    /// Return the size of this pile.
    unsigned char getSize() const { return state; }
};
