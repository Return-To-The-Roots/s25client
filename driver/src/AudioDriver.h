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
#ifndef AUDIODRIVER_H_INCLUDED
#define AUDIODRIVER_H_INCLUDED

#include "AudioInterface.h"
#include <vector>
class AudioDriverLoaderInterface;
class Sound;

/// Basisklasse für einen Audiotreiber.
class AudioDriver: public IAudioDriver
{
    public:
        AudioDriver(AudioDriverLoaderInterface* adli);

        ~AudioDriver() override;

        /// Funktion zum Auslesen des Treibernamens.
        const char* GetName() const override;

        /// prüft auf Initialisierung.
        bool IsInitialized() override { return initialized; }

    protected:

        /// "Generiert" eine Play-ID
        unsigned GeneratePlayID();

    private:

        /// Counter für Play-IDs
        unsigned play_id_counter;


    protected:

        AudioDriverLoaderInterface* adli;

        /// Das DriverCallback für Rückmeldungen.

        bool initialized; /// Initialisierungsstatus.

        std::vector<Sound*> sounds;

        /// Anzahl Channels, die reserviert werden können (für Effekte!)
        static const unsigned CHANNEL_COUNT = 64;


};

#endif // !AUDIODRIVER_H_INCLUDED
