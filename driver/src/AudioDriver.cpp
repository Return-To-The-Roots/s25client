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

#include "driverDefines.h" // IWYU pragma: keep
#include "AudioDriver.h"
#include "SoundHandle.h"
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <limits>
#include <stdexcept>

class IAudioDriverCallback;

// Do not inline! That would break DLL compatibility:
// http://stackoverflow.com/questions/32444520/how-to-handle-destructors-in-dll-exported-interfaces
IAudioDriver::~IAudioDriver() {}

AudioDriver::AudioDriver(IAudioDriverCallback* driverCallback)
    : driverCallback(driverCallback), initialized(false), nextPlayID_(0), numChannels_(0)
{}

AudioDriver::~AudioDriver()
{
    // This should have been done in Cleanup or by the subclass
    RTTR_Assert(sounds_.empty());
}

void AudioDriver::CleanUp()
{
    BOOST_FOREACH(SoundDesc* sound, sounds_)
    {
        RTTR_Assert(sound->isValid());
        // Note: Don't call UnloadSound as it would also remove it from sounds invalidating the iterator
        DoUnloadSound(*sound);
        RTTR_Assert(!sound->isValid());
    }
    sounds_.clear();
    initialized = false;
}

void AudioDriver::UnloadSound(AudioDriver& driver, SoundDesc* sound)
{
    if(sound->isValid())
    {
        std::vector<SoundDesc*>::iterator it = std::find(driver.sounds_.begin(), driver.sounds_.end(), sound);
        RTTR_Assert(it != driver.sounds_.end());
        driver.DoUnloadSound(*sound);
        RTTR_Assert(!sound->isValid());
        driver.sounds_.erase(it);
    }
    delete sound;
}

void AudioDriver::SetNumChannels(unsigned numChannels)
{
    if(numChannels > channels_.size())
        throw std::out_of_range("Number of channels exceeds max number of channels");
    numChannels_ = numChannels;
    std::fill(channels_.begin(), channels_.begin() + numChannels_, EffectPlayId(-1));
}

EffectPlayId AudioDriver::AddPlayedEffect(int channel)
{
    if(channel < 0)
        return -1;
    EffectPlayId newId = GeneratePlayID();
    channels_[channel] = newId;
    return newId;
}

int AudioDriver::GetEffectChannel(EffectPlayId playId)
{
    for(unsigned i = 0; i < channels_.size(); i++)
    {
        if(channels_[i] == playId)
            return static_cast<int>(i);
    }
    return -1;
}

void AudioDriver::RemoveEffect(EffectPlayId playId)
{
    int channel = GetEffectChannel(playId);
    if(channel >= 0)
        channels_[channel] = -1;
}

SoundHandle AudioDriver::CreateSoundHandle(SoundDesc* sound)
{
    RTTR_Assert(sound->isValid());
    sounds_.push_back(sound);
    return SoundHandle(SoundHandle::Descriptor(sound, boost::bind(&AudioDriver::UnloadSound, boost::ref(*this), _1)));
}

EffectPlayId AudioDriver::GeneratePlayID()
{
    EffectPlayId result = nextPlayID_;
    if(nextPlayID_ == std::numeric_limits<EffectPlayId>::max())
        nextPlayID_ = 0;
    else
        nextPlayID_++;
    return result;
}
