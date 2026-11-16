// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "nofWorkman.h"

class SerializedGameData;
class nobUsual;
class noAnimal;

class nofSkinner : public nofWorkman
{
    /// animal, which is skinned
    noAnimal* animal = nullptr;

    /// Draw worker at work
    void drawWorking(DrawPoint drawPt) override;
    /// Id in jobs.bob or carrier.bob when carrying a ware
    [[noreturn]] unsigned short getCarryID() const override;
    helpers::OptionalEnum<GoodType> produceWare() override;

    /// Draws the figure while returning home / entering the building (often carrying wares)
    void drawWalkingWithWare(DrawPoint drawPt) override;

    void handleDerivedEvent(unsigned id) override;
    void tryToWork() override;
    void walkedDerived() override;

    void tryStartSkinning();
    void handleStateSkinningCarcass();
    void startWalkingHome();
    void handleStateWalkingHome();

public:
    nofSkinner(MapPoint pos, unsigned char player, nobUsual* workplace);
    nofSkinner(SerializedGameData& sgd, unsigned obj_id);

    void Serialize(SerializedGameData& sgd) const override;

    GO_Type getGOT() const final { return GO_Type::NofSkinner; }

    void workAborted() override;
    void animalLost();
};
