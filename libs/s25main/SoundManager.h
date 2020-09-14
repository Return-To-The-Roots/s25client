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

#include "driver/EffectPlayId.h"
#include "s25util/Singleton.h"
#include <list>

class noBase;

/// Game-naher SoundManager, der die abgespielten Sounds speichert und immer entscheidet, ob derjenige Sound abgespielt
/// wird oder nicht, weil er eben bereits abgespielt wird
/// verwaltet auch "globale" Sounds wie Vogelgezwitscher und Meeresrauschen
class SoundManager : public Singleton<SoundManager, SingletonPolicies::WithLongevity>
{
    /// Objekt-spezifischer Sound (NO-Sound)
    struct NOSound
    {
        /// Objekt, das den Sound "wiedergibt"
        noBase* obj;
        /// Zusätzliche ID, falls das Objekt im Zuge seiner Arbeit mehrere Sounds von sich gibt
        unsigned id;
        /// Abspiel ID - identifiziert ein abgespieltes Stück, mit dem man abgespielte Stücke stoppen kann
        EffectPlayId play_id;
    };

    /// Liste von NO-Sounds
    std::list<NOSound> no_sounds;

    //////////////////////////////////

    /// Wann wurde der letzte Vogelzwitschersound abgespielt?
    unsigned last_bird;
    /// Intervall zwischen den Vogelzwitschern
    unsigned bird_interval;
    /// Play-ID fürs Meeresrauschen
    EffectPlayId ocean_play_id;

public:
    static constexpr unsigned Longevity = 29;

    SoundManager();
    ~SoundManager();

    /// Versucht ggf. Objekt-Sound abzuspielen
    void PlayNOSound(unsigned sound_lst_id, noBase* obj, unsigned id, unsigned char volume = 255);
    /// Wenn die Arbeit (wo er Sounds von sich gegeben hat) von einem Objekt fertig ist bzw. abgebrochen wurde,
    /// wird diese Funktion aufgerufen, die alle Sounds von diesem Objekt entfernt
    void WorkingFinished(noBase* obj);

    /////////////////////////////////

    /// Wird immer aufgerufen, wenn der GameWorld alles gezeichnet hat und die Vögel abgespielt werden
    void PlayBirdSounds(unsigned short tree_count);
    /// Spielt Meeresrauschen ab (wird der Anteil von Wasser an der aktuell gezeichneten Fläche in % angegeben)
    void PlayOceanBrawling(unsigned water_percent);

    void StopAll();
};

#define SOUNDMANAGER SoundManager::inst()
