// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AudioInterface.h"
#include <vector>

class IAudioDriverCallback;

namespace driver {

/// Base class for audio drivers
class AudioDriver : public IAudioDriver
{
public:
    AudioDriver(IAudioDriverCallback* driverCallback);
    ~AudioDriver() override;

    /// prüft auf Initialisierung.
    bool IsInitialized() final override { return initialized; }
    void CleanUp() override;

    EffectPlayId PlayEffect(const RawSoundHandle& sound, uint8_t volume, bool loop) final override;
    void StopEffect(EffectPlayId play_id) final override;

    void unloadSound(RawSoundHandle handle) final override;
    /// Register this handle pointer so that the driverData is reset when the sound gets unloaded
    void registerForUnload(RawSoundHandle* handlePtr) final override;

protected:
    static constexpr unsigned DEFAULT_NUM_CHANNELS = 64;
    /// Sets the actual number of channels used. Must be called before using channels
    void SetNumChannels(unsigned numChannels);
    /// Get the channel an effect is being played at or -1 if not found
    int GetEffectChannel(EffectPlayId playId) const;
    /// Removes the effect from the channel list
    void RemoveEffect(EffectPlayId playId);
    /// Add the sound to the list of loaded sounds and return a RawSoundHandle
    RawSoundHandle createRawSoundHandle(RawSoundHandle::DriverData driverData, SoundType type);

    // To be implemented by actual driver

    // Play the given effect. Returns the channel on which this is played
    virtual int doPlayEffect(RawSoundHandle::DriverData driverData, uint8_t volume, bool loop) = 0;
    // Stop the effect playing o this channel
    virtual void doStopEffect(int channel) = 0;
    /// Called for a still loaded sound and should unload the sound
    virtual void doUnloadSound(RawSoundHandle sound) = 0;

    IAudioDriverCallback* driverCallback;
    bool initialized; /// Is initialized?

private:
    /// Generates a play id
    EffectPlayId GeneratePlayID();
    /// Next play id
    EffectPlayId nextPlayID_;
    std::vector<RawSoundHandle> loadedSounds_;
    /// RawSoundHandle that were registered to be reset on unloading that sound
    std::vector<RawSoundHandle*> handlesRegisteredForUnload_;
    /// Which effect is played on which channel
    std::vector<EffectPlayId> channels_;
};
} // namespace driver
