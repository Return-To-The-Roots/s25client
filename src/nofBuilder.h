// $Id: nofBuilder.h 9357 2014-04-25 15:35:25Z FloSoft $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef NOF_BUILDER_H_
#define NOF_BUILDER_H_

#include "noFigure.h"

class noBuildingSite;

class nofBuilder : public noFigure
{
    private:

        // Wie weit der Bauarbeiter maximal in alle vier richtungen laufen darf (in Pixeln, rel..)
        static const short LEFT_MAX = -28;
        static const short RIGHT_MAX = 28;
        static const short UP_MAX = 0;
        static const short DOWN_MAX = 16;


        enum BuilderState
        {
            STATE_FIGUREWORK = 0,
            STATE_WAITINGFREEWALK, // Bauarbeiter geht auf und ab und wartet auf Rohstoffe
            STATE_BUILDFREEWALK, // Bauarbeiter geht auf und ab und baut
            STATE_BUILD // Bauarbeiter "baut" gerade (hämmert auf Gebäude ein)
        } state;

        /// Baustelle des Bauarbeiters
        noBuildingSite* building_site;


        //EventManager::EventPointer current_ev;

        /// X,Y relativ zur Baustelle in Pixeln
        /// next ist der angesteuerte Punkt
        short rel_x, rel_y;
        short next_rel_x, next_rel_y;

        /// Wie viele Bauschritte noch verfügbar sind, bis der nächste Rohstoff geholt werden muss
        unsigned char building_steps_available;

    private:

        void GoalReached();
        void Walked();
        void AbrogateWorkplace();
        void HandleDerivedEvent(const unsigned int id);

        /// In neue Richtung laufen (Freewalk)
        void StartFreewalk();
        /// "Frisst" eine passende Ware (falls vorhanden, gibt true in dem Fall zurück!) von der Baustelle, d.h nimmt sie in die Hand und erhöht die building_steps_avaible
        bool ChooseWare();

    public:

        nofBuilder(const unsigned short x, const unsigned short y, const unsigned char player, noRoadNode* building_site);
        nofBuilder(SerializedGameData* sgd, const unsigned obj_id);

        /// Serialisierungsfunktionen
    protected:  void Serialize_nofBuilder(SerializedGameData* sgd) const;
    public:     void Serialize(SerializedGameData* sgd) const { Serialize_nofBuilder(sgd); }

        GO_Type GetGOT() const { return GOT_NOF_BUILDER; }

        void Draw(int x, int y);

        // Wird von der Baustelle aus aufgerufen, um den Bauarbeiter zu sagen, dass er gehen kann
        void LostWork();
};


#endif
