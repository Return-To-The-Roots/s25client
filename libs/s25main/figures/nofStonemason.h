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

#include "nofFarmhand.h"
class SerializedGameData;
class nobUsual;

class nofStonemason : public nofFarmhand
{
private:
    /// Malt den Arbeiter beim Arbeiten
    void DrawWorking(DrawPoint drawPt) override;
    /// Id in jobs.bob or carrier.bob when carrying a ware
    unsigned short GetCarryID() const override;

    /// Abgeleitete Klasse informieren, wenn sie anfängt zu arbeiten (Vorbereitungen)
    void WorkStarted() override;
    /// Abgeleitete Klasse informieren, wenn fertig ist mit Arbeiten
    void WorkFinished() override;

    /// Returns the quality of this working point or determines if the worker can work here at all
    PointQuality GetPointQuality(MapPoint pt) const override;

public:
    nofStonemason(MapPoint pos, unsigned char player, nobUsual* workplace);
    nofStonemason(SerializedGameData& sgd, unsigned obj_id);

    GO_Type GetGOT() const override { return GOT_NOF_STONEMASON; }
};
