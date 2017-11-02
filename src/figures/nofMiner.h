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

#ifndef NOF_MINER_H_
#define NOF_MINER_H_

#include "nofWorkman.h"

class SerializedGameData;
class nobUsual;

/// Klasse für den Schreiner
class nofMiner : public nofWorkman
{
protected:
    /// Zeichnet ihn beim Arbeiten
    void DrawWorking(DrawPoint drawPt) override;
    /// Gibt die ID in JOBS.BOB zurück, wenn der Beruf Waren rausträgt (bzw rein)
    unsigned short GetCarryID() const override;
    /// Der Arbeiter erzeugt eine Ware
    GoodType ProduceWare() override;

    bool AreWaresAvailable() const override;
    bool StartWorking() override;
    Resource::Type GetRequiredResType() const;

public:
    nofMiner(const MapPoint pt, const unsigned char player, nobUsual* workplace);
    nofMiner(SerializedGameData& sgd, const unsigned obj_id);

    GO_Type GetGOT() const override { return GOT_NOF_MINER; }
};

#endif
