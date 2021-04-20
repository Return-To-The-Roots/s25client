// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "figures/noFigure.h"

class noBuildingSite;
class SerializedGameData;

/// Der Planierer
class nofPlaner : public noFigure
{
    /// Arbeitsstelle des Planierers
    noBuildingSite* building_site;

    /// Was der Planierer gerade so schönes macht
    enum class PlanerState : uint8_t
    {
        FigureWork,
        Walking, /// läuft zum nächsten Punkt, um zu graben
        Planing  /// planiert einen Punkt (Abspielen der Animation
    } state;
    friend constexpr auto maxEnumValue(PlanerState) { return PlanerState::Planing; }

    /// Wie rum er geht
    enum class PlaningDir : uint8_t
    {
        NotWorking,
        Clockwise,       /// Uhrzeigersinn
        Counterclockwise /// entgegen Uhrzeigersinn
    } pd;
    friend constexpr auto maxEnumValue(PlaningDir) { return PlaningDir::Counterclockwise; }

private:
    void GoalReached() override;
    void Walked() override;
    void AbrogateWorkplace() override;
    void HandleDerivedEvent(unsigned id) override;

public:
    nofPlaner(MapPoint pos, unsigned char player, noBuildingSite* building_site);
    nofPlaner(SerializedGameData& sgd, unsigned obj_id);

    void Serialize(SerializedGameData& sgd) const override;

    GO_Type GetGOT() const final { return GO_Type::NofPlaner; }

    void Draw(DrawPoint drawPt) override;

    /// Wird von der Baustelle aus aufgerufen, um den Bauarbeiter zu sagen, dass er gehen kann
    void LostWork();
};
