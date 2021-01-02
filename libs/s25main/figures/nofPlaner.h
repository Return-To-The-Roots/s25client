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
    /// Was der Planierer gerade so schönes macht
    enum PlanerState
    {
        STATE_FIGUREWORK,
        STATE_WALKING, /// läuft zum nächsten Punkt, um zu graben
        STATE_PLANING  /// planiert einen Punkt (Abspielen der Animation
    } state;

    /// Arbeitsstelle des Planierers
    noBuildingSite* building_site;

    /// Wie rum er geht
    enum PlaningDir
    {
        PD_NOTWORKING,
        PD_CLOCKWISE,       /// Uhrzeigersinn
        PD_COUNTERCLOCKWISE /// entgegen Uhrzeigersinn
    } pd;

private:
    void GoalReached() override;
    void Walked() override;
    void AbrogateWorkplace() override;
    void HandleDerivedEvent(unsigned id) override;

public:
    nofPlaner(MapPoint pos, unsigned char player, noBuildingSite* building_site);
    nofPlaner(SerializedGameData& sgd, unsigned obj_id);

    /// Serialisierungsfunktionen
protected:
    void Serialize_nofPlaner(SerializedGameData& sgd) const;

public:
    void Serialize(SerializedGameData& sgd) const override { Serialize_nofPlaner(sgd); }

    GO_Type GetGOT() const override { return GOT_NOF_PLANER; }

    void Draw(DrawPoint drawPt) override;

    /// Wird von der Baustelle aus aufgerufen, um den Bauarbeiter zu sagen, dass er gehen kann
    void LostWork();
};
