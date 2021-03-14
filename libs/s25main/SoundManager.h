// Copyright (c) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
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

#include "Clock.h"
#include "driver/EffectPlayId.h"
#include <cstdint>
#include <list>
#include <vector>

class noBase;

/// Constains state of sounds played in-game
/// Decides whether to play a sound depending on currently played sounds
class SoundManager
{
    /// object specific sound
    struct NOSound
    {
        /// ID of the sound itself, i.e. the same sound should have the same soundId
        unsigned soundId;
        /// Object that is playing the sound
        const noBase* obj;
        /// ID of the sound of that object
        unsigned objSoundId;
        /// Reference ID to the played sound
        EffectPlayId playId;
    };
    struct AnimalSound
    {
        unsigned soundId;
        EffectPlayId playId;
    };

    /// Currently active node sounds
    std::list<NOSound> noSounds;
    std::vector<AnimalSound> animalSounds;
    /// Earliest timepoint of the next bird sound
    Clock::time_point minNextBirdSound;
    /// Play ids of the ambient sounds
    EffectPlayId oceanPlayId, birdPlayId;

public:
    SoundManager();
    ~SoundManager();

    /// Play a sound for a node object
    void playNOSound(unsigned soundLstId, const noBase& obj, unsigned id, uint8_t volume = 255);
    /// Stop all sounds from this object, usually when the work of it is finished
    void stopSounds(const noBase& obj);
    /// Play the sound of an animal
    void playAnimalSound(unsigned soundLstId);

    /// Play the bird sound effect which is influenced by the number of trees
    void playBirdSounds(unsigned treeCount);
    /// Play the ocean sound effect which depends on the ratio of water currently visible
    void playOceanBrawling(unsigned waterPercent);

    /// Stop all sound effects
    void stopAll();

    static constexpr unsigned maxPlayCtPerSound = 3;
};
