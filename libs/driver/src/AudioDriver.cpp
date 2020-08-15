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

#include "driver/AudioDriver.h"
#include "RTTR_Assert.h"
#include "helpers/containerUtils.h"
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace driver {

// Do not inline! That would break DLL compatibility:
// http://stackoverflow.com/questions/32444520/how-to-handle-destructors-in-dll-exported-interfaces
IAudioDriver::~IAudioDriver() = default;

AudioDriver::AudioDriver(IAudioDriverCallback* driverCallback)
    : driverCallback(driverCallback), initialized(false), nextPlayID_(EffectPlayId(0))
{}

AudioDriver::~AudioDriver()
{
    // This should have been done in Cleanup or by the subclass
    RTTR_Assert(loadedSounds_.empty());
}

void AudioDriver::CleanUp()
{
    const auto soundsCopy = loadedSounds_;
    for(const auto& sound : soundsCopy)
        unloadSound(sound);
    initialized = false;
}

EffectPlayId AudioDriver::PlayEffect(const RawSoundHandle& sound, uint8_t volume, bool loop)
{
    if(!sound.getDriverData())
        return EffectPlayId::Invalid;
    RTTR_Assert(sound.getType() == driver::SoundType::Effect);
    if(sound.getType() != driver::SoundType::Effect)
        return EffectPlayId::Invalid;

    const int channel = doPlayEffect(sound.getDriverData(), volume, loop);
    if(channel < 0 || static_cast<unsigned>(channel) >= channels_.size())
        return EffectPlayId::Invalid;
    const EffectPlayId newId = GeneratePlayID();
    channels_[channel] = newId;
    return newId;
}

void AudioDriver::StopEffect(EffectPlayId play_id)
{
    const int channel = GetEffectChannel(play_id);
    if(channel >= 0)
    {
        doStopEffect(channel);
        RemoveEffect(play_id);
    }
}

void AudioDriver::unloadSound(RawSoundHandle handle)
{
    RTTR_Assert(handle.driverData); // Otherwise handle is invalid
    const auto it = std::find(loadedSounds_.begin(), loadedSounds_.end(), handle);
    if(it == loadedSounds_.end())
        throw std::invalid_argument("Sound is not currently loaded");
    doUnloadSound(handle);
    loadedSounds_.erase(it);
    // Notify all subscribed handles by resetting driverData to nullptr
    auto itNotify = handlesRegisteredForUnload_.begin();
    while((itNotify = std::find_if(itNotify, handlesRegisteredForUnload_.end(),
                                   [handle](const RawSoundHandle* curHandle) { return *curHandle == handle; }))
          != handlesRegisteredForUnload_.end())
    {
        (*itNotify)->invalidate();
        itNotify = handlesRegisteredForUnload_.erase(itNotify);
    }
}

void AudioDriver::registerForUnload(RawSoundHandle* handlePtr)
{
    handlesRegisteredForUnload_.push_back(handlePtr);
}

void AudioDriver::SetNumChannels(unsigned numChannels)
{
    channels_.resize(numChannels);
    std::fill(channels_.begin(), channels_.end(), EffectPlayId::Invalid);
}

int AudioDriver::GetEffectChannel(EffectPlayId playId) const
{
    const auto it = helpers::find(channels_, playId);
    return (it == channels_.end()) ? -1 : static_cast<int>(std::distance(channels_.begin(), it));
}

void AudioDriver::RemoveEffect(EffectPlayId playId)
{
    int channel = GetEffectChannel(playId);
    if(channel >= 0)
        channels_[channel] = EffectPlayId::Invalid;
}

RawSoundHandle AudioDriver::createRawSoundHandle(RawSoundHandle::DriverData driverData, SoundType type)
{
    RawSoundHandle handle(driverData, type);
    if(driverData)
        loadedSounds_.push_back(handle);
    return handle;
}

EffectPlayId AudioDriver::GeneratePlayID()
{
    using intType = std::underlying_type_t<EffectPlayId>;

    EffectPlayId result = nextPlayID_;
    if(static_cast<intType>(nextPlayID_) == std::numeric_limits<intType>::max())
        nextPlayID_ = EffectPlayId(0);
    else
        nextPlayID_ = EffectPlayId(static_cast<intType>(nextPlayID_) + 1);
    return result;
}

} // namespace driver