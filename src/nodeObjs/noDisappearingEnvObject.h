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
#ifndef NO_DISAPPEARING_ENVOBJECT
#define NO_DISAPPEARING_ENVOBJECT

#include "noCoordBase.h"
#include "EventManager.h"
class SerializedGameData;

class noDisappearingEnvObject : public noCoordBase
{
    public:
        noDisappearingEnvObject(const MapPoint pt, const unsigned living_time,
                                const unsigned add_var_living_time);
        noDisappearingEnvObject(SerializedGameData& sgd, const unsigned obj_id);

        /// Aufräummethoden
    protected:  void Destroy_noDisappearingEnvObject();
    public:     void Destroy() override { Destroy_noDisappearingEnvObject(); }
        /// Serialisierungsfunktionen
    protected:  void Serialize_noDisappearingEnvObject(SerializedGameData& sgd) const;
    public:     void Serialize(SerializedGameData& sgd) const override { Serialize_noDisappearingEnvObject(sgd); }

        /// Benachrichtigen, wenn neuer GF erreicht wurde.
        void HandleEvent_noDisappearingEnvObject(const unsigned int id);

    protected:

        /// Gibt Farbe zurück, mit der das Objekt gezeichnet werden soll
        unsigned GetDrawColor() const;
        /// Gibt Farbe zurück, mit der der Schatten des Objekts gezeichnet werden soll
        unsigned GetDrawShadowColor() const;

    private:

        /// Bin ich grad in der Sterbephase (in der das Schild immer transparenter wird, bevor es verschwindet)
        bool disappearing;
        /// Event, das bestimmt wie lange es noch lebt
        EventManager::EventPointer dead_event;
};


#endif
