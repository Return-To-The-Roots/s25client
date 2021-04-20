// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "noBaseBuilding.h"
class FOWObject;
class SerializedGameData;

class noBuilding : public noBaseBuilding
{
    /// How many people opened the door. positive: open, 0: closed, negative: error
    signed char opendoor;

protected:
    noBuilding(BuildingType type, MapPoint pos, unsigned char player, Nation nation);
    noBuilding(SerializedGameData& sgd, unsigned obj_id);

    /// Called to destroy derived classes after building was replaced by fire and removed from players inventory
    virtual void DestroyBuilding() = 0;

public:
    void Destroy() override;
    void Serialize(SerializedGameData& sgd) const override;

    /// Draws the basic building (no fires etc.) with the door
    void DrawBaseBuilding(DrawPoint drawPt) const;
    /// Draws the door only (if building is drawn at x, y)
    void DrawDoor(DrawPoint drawPt) const;

    void OpenDoor();
    void CloseDoor();
    bool IsDoorOpen() const { return opendoor > 0; }

    /// Wird aufgerufen, wenn von der Fahne vor dem Gebäude ein Rohstoff aufgenommen wurde
    virtual bool FreePlaceAtFlag() = 0;

    /// Erzeugt von ihnen selbst ein FOW Objekt als visuelle "Erinnerung" für den Fog of War
    std::unique_ptr<FOWObject> CreateFOWObject() const override;
};
