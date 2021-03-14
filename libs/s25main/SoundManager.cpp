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

#include "SoundManager.h"
#include "Loader.h"
#include "Settings.h"
#include "drivers/AudioDriverWrapper.h"
#include "helpers/random.h"
#include "ogl/SoundEffectItem.h"

namespace {
auto& getSoundRng()
{
    static auto soundRng = helpers::getRandomGenerator();
    return soundRng;
}
} // namespace

SoundManager::SoundManager()
    : minNextBirdSound(Clock::now()), oceanPlayId(EffectPlayId::Invalid), birdPlayId(EffectPlayId::Invalid)
{}

SoundManager::~SoundManager()
{
    stopAll();
}

void SoundManager::playNOSound(unsigned soundLstId, const noBase& obj, unsigned id, uint8_t volume)
{
    if(!SETTINGS.sound.effectsEnabled)
        return;

    // Check how many times this sound is already played
    unsigned numPlayedCt = 0;
    for(auto& sound : noSounds)
    {
        if(sound.soundId == soundLstId)
        {
            // if the object is playing it itself already, ignore
            if(sound.obj == &obj && sound.objSoundId == id)
                return;
            numPlayedCt++;
        }
    }
    if(numPlayedCt >= maxPlayCtPerSound)
        return;

    EffectPlayId playId = LOADER.GetSoundN("sound", soundLstId)->Play(volume, false);

    if(playId != EffectPlayId::Invalid)
        noSounds.push_back(NOSound{soundLstId, &obj, id, playId});
}

void SoundManager::stopSounds(const noBase& obj)
{
    // Stop and remove all sounds of this object
    for(auto it = noSounds.begin(); it != noSounds.end();)
    {
        if(it->obj == &obj)
        {
            AUDIODRIVER.StopEffect(it->playId);
            it = noSounds.erase(it);
        } else
            ++it;
    }
}

void SoundManager::playAnimalSound(unsigned soundLstId)
{
    // Count how often this sound is already played and also remove finished ones
    unsigned numPlayedCt = 0;
    for(auto it = animalSounds.begin(); it != animalSounds.end();)
    {
        if(it->soundId == soundLstId)
        {
            if(AUDIODRIVER.IsEffectPlaying(it->playId))
                ++numPlayedCt;
            else
            {
                it = animalSounds.erase(it);
                continue;
            }
        }
        ++it;
    }
    if(numPlayedCt >= maxPlayCtPerSound)
        return;

    const unsigned volume = helpers::randomValue(getSoundRng(), 50u, 70u);
    EffectPlayId playId = LOADER.GetSoundN("sound", soundLstId)->Play(volume, false);

    if(playId != EffectPlayId::Invalid)
        animalSounds.push_back(AnimalSound{soundLstId, playId});
}

void SoundManager::playBirdSounds(const unsigned treeCount)
{
    if(!SETTINGS.sound.effectsEnabled)
        return;

    using namespace std::chrono;
    using namespace std::chrono_literals;

    // Depending on the number of trees the next sound will be played a bit later (more trees -> earlier)
    constexpr milliseconds maxTreeInterval = 1s;
    const milliseconds intervalReduction = treeCount * 10ms;
    const auto nextBirdSoundTime = minNextBirdSound + std::max(0ms, maxTreeInterval - intervalReduction);

    const auto now = Clock::now();
    if(now >= nextBirdSoundTime)
    {
        // no trees -> no birds
        if(treeCount > 0)
        {
            const unsigned soundIdx = helpers::randomValue(getSoundRng(), 87u, 91u);
            const unsigned volume = helpers::randomValue(getSoundRng(), 50u, 70u);
            birdPlayId = LOADER.GetSoundN("sound", soundIdx)->Play(volume, false);
        }
        minNextBirdSound = now + milliseconds(helpers::randomValue(getSoundRng(), 150u, 1000u));
    }
}

void SoundManager::playOceanBrawling(const unsigned waterPercent)
{
    RTTR_Assert(waterPercent <= 100u);

    if(!SETTINGS.sound.effectsEnabled)
        return;

    // Play ocean sound for at least 10% water
    if(waterPercent >= 10)
    {
        const unsigned volume = waterPercent * 2u + 55u;
        if(!AUDIODRIVER.IsEffectPlaying(oceanPlayId))
        {
            // SDL Mixer may wrongly return that the sound is not playing, so make sure the old effect is stopped.
            // DO NOT REMOVE - THIS PREVENTS A BUG!
            if(oceanPlayId != EffectPlayId::Invalid)
                AUDIODRIVER.StopEffect(oceanPlayId);

            const unsigned soundIdx = helpers::randomValue(getSoundRng(), 98u, 100u);
            oceanPlayId = LOADER.GetSoundN("sound", soundIdx)->Play(volume, true);
        } else
            AUDIODRIVER.ChangeVolume(oceanPlayId, volume);
    } else if(oceanPlayId != EffectPlayId::Invalid)
        AUDIODRIVER.StopEffect(oceanPlayId);
}

void SoundManager::stopAll()
{
    if(oceanPlayId != EffectPlayId::Invalid)
        AUDIODRIVER.StopEffect(oceanPlayId);
    if(birdPlayId != EffectPlayId::Invalid)
        AUDIODRIVER.StopEffect(birdPlayId);

    for(auto& sound : noSounds)
        AUDIODRIVER.StopEffect(sound.playId);
    noSounds.clear();
    for(auto& sound : animalSounds)
        AUDIODRIVER.StopEffect(sound.playId);
    animalSounds.clear();
}
