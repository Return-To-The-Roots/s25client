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
