// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "nofWorkman.h"

class SerializedGameData;
class nobUsual;
class noAnimal;

#ifdef _MSC_VER
#    pragma warning(disable : 4646) // function declared with [[noreturn]] has non-void return type
#endif

class nofSkinner : public nofWorkman
{
    /// animal, which is skinned
    noAnimal* animal;

    /// Draw worker at work
    void DrawWorking(DrawPoint drawPt) override;
    /// Id in jobs.bob or carrier.bob when carrying a ware
    [[noreturn]] unsigned short GetCarryID() const override;
    helpers::OptionalEnum<GoodType> ProduceWare() override;

    /// Draws the figure while returning home / entering the building (often carrying wares)
    void DrawWalkingWithWare(DrawPoint drawPt) override;

    void HandleDerivedEvent(unsigned id) override;
    void TryToWork() override;
    void WalkedDerived() override;

    void TryStartSkinning();
    void HandleStateSkinningCarcass();
    void StartWalkingHome();
    void HandleStateWalkingHome();

public:
    nofSkinner(MapPoint pos, unsigned char player, nobUsual* workplace);
    nofSkinner(SerializedGameData& sgd, unsigned obj_id);

    void Serialize(SerializedGameData& sgd) const override;

    GO_Type GetGOT() const final { return GO_Type::NofSkinner; }

    void WorkAborted() override;
};
