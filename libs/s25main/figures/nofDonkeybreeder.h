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

#include "nofWorkman.h"

class SerializedGameData;
class nobUsual;

/// Klasse für den Eselzüchter
class nofDonkeybreeder : public nofWorkman
{
public:
    nofDonkeybreeder(MapPoint pos, unsigned char player, nobUsual* workplace);
    nofDonkeybreeder(SerializedGameData& sgd, unsigned obj_id);

    GO_Type GetGOT() const override { return GOT_NOF_DONKEYBREEDER; }

private:
    /// Zeichnet ihn beim Arbeiten.
    void DrawWorking(DrawPoint drawPt) override;
    /// Der Arbeiter erzeugt eine Ware.
    helpers::OptionalEnum<GoodType> ProduceWare() override;
    /// Wird aufgerufen, wenn er fertig mit arbeiten ist
    void WorkFinished() override;

    /// Id in jobs.bob or carrier.bob when carrying a ware
    unsigned short GetCarryID() const override { return 0; }
};
