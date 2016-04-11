// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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
#ifndef NO_DISAPPEARINGMAPENVOBJECT
#define NO_DISAPPEARINGMAPENVOBJECT


#include "noDisappearingEnvObject.h"
class SerializedGameData;

/// Verschwindendes Umwelt-Objekt ohne weiter Bedeutung (z.b. Baumstamm etc.)
class noDisappearingMapEnvObject : public noDisappearingEnvObject
{
    public:
        noDisappearingMapEnvObject(const MapPoint pt, const unsigned short map_id);
        noDisappearingMapEnvObject(SerializedGameData& sgd, const unsigned obj_id);

        /// Aufräummethoden
    protected:  void Destroy_noDisappearingMapEnvObject();
    public:     void Destroy() override { Destroy_noDisappearingMapEnvObject(); }
        /// Serialisierungsfunktionen
    protected:  void Serialize_noDisappearingMapEnvObject(SerializedGameData& sgd) const;
    public:     void Serialize(SerializedGameData& sgd) const override { Serialize_noDisappearingMapEnvObject(sgd); }

        GO_Type GetGOT() const override { return GOT_DISAPPEARINGMAPENVOBJECT; }

        /// An x,y zeichnen.
        void Draw(int x, int y) override;

        void HandleEvent(const unsigned int id) override;

    private:

        /// ID in der mapsx.lst
        const unsigned short map_id;
};


#endif
