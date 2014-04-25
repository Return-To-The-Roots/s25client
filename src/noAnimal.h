// $Id: noAnimal.h 9357 2014-04-25 15:35:25Z FloSoft $
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

#ifndef NO_ANIMAL_H_
#define NO_ANIMAL_H_

#include "GameConsts.h"
#include "noMovable.h"

class nofHunter;

/// Klasse für die Tiere (ausgenommen Esel und Schweine natürlich)
class noAnimal : public noMovable
{
        /// Tierart
        Species species;

        /// Was macht das Tier gerade?
        enum State
        {
            STATE_WALKING = 0, /// Läuft dumm in der Gegend rum
            STATE_PAUSED, /// macht mal ne kurze Verschnaufpause ;)
            STATE_WAITINGFORHUNTER, /// wartet auf den Jäger bis er es abknallt
            STATE_WALKINGUNTILWAITINGFORHUNTER, /// läuft weiter, wartet aber dann auf den Jäger
            STATE_DEAD, /// wurde erschossen und liegt tot rum
            STATE_DISAPPEARING /// Leiche verschwindet langsam...
        } state;

        /// Wie weit kann es noch rumlaufen, bis es mal wieder eine Pause machen muss
        unsigned short pause_way;
        /// Jäger, der das Tier jagt (0, falls nicht gejagt)
        nofHunter* hunter;
        /// Nächster Zeitpunkt, ab wann der Sound gespielt werden soll (bei Enten und Schafen)
        unsigned int sound_moment;

    private:


        /// entscheidet, was nach einem gelaufenen Abschnitt weiter zu tun ist
        void Walked();
        /// Sucht eine Richtung, in die das Tier gehen kann
        unsigned char FindDir();
        /// Fängt an zu laufen
        void StartWalking(const unsigned char dir);
        /// Sucht eine neue Richtung und läuft in diese, ansonsten stirbt es
        void StandardWalking();

    public:


        /// Konstruktor
        noAnimal(const Species species, const unsigned short x, const unsigned short y);
        noAnimal(SerializedGameData* sgd, const unsigned obj_id);

        ~noAnimal() {}

        /// Serialisierungsfunktionen
    protected:  void Serialize_noAnimal(SerializedGameData* sgd) const;
    public:     void Serialize(SerializedGameData* sgd) const { Serialize_noAnimal(sgd); }

        /// Aufräummethoden
    protected:  void Destroy_noAnimal() { Destroy_noMovable(); }
    public:     void Destroy() { Destroy_noAnimal(); }

        GO_Type GetGOT() const { return GOT_ANIMAL; }

        // An x,y zeichnen
        void Draw(int x, int y);
        // Benachrichtigen, wenn neuer gf erreicht wurde
        void HandleEvent(const unsigned int id);

        /// Wird aufgerufen, nachdem das Tier erzeugt wurde und zur Figurenliste hinzugefügt wurde
        void StartLiving();

        /// Kann das Tier noch vom Jäger gejagt werden?
        bool CanHunted() const;

        /// Ein Jäger geht das Tier jagen
        void BeginHunting(nofHunter* hunter);
        /// Ein Jäger ist in der Nähe vom Tier und will es abschießen --> Tier muss stillhalten
        /// gibt die Koordinaten seines (späteren) Standorts zurück
        void HunterIsNear(unsigned short* location_x, unsigned short* location_y);
        /// Ein Jäger kann doch nicht mehr zum Tier kommen --> kann wieder normal weiterlaufen
        void StopHunting();


        /// Steht das Tier schon schön ruhig, damit der Jäger es erschießen kann?
        bool IsReadyForShooting() const { return (state == STATE_WAITINGFORHUNTER); }
        /// Tier soll sterben - erstmal nur Leiche liegen lassen
        void Die();
        /// Tier wurde vom Jäger ausgenommen und muss sofort verschwinden
        void Eviscerated();
};


#endif

