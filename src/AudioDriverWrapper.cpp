// $Id: AudioDriverWrapper.cpp 9357 2014-04-25 15:35:25Z FloSoft $
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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "main.h"
#include "AudioDriverWrapper.h"

#include "VideoDriverWrapper.h"
#include "Settings.h"
#include "MusicPlayer.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/// Konstruktor von @p DriverWrapper
AudioDriverWrapper::AudioDriverWrapper() : audiodriver(0)
{
}

AudioDriverWrapper::~AudioDriverWrapper()
{
    delete audiodriver;
}

/// Lädt den Treiber
bool AudioDriverWrapper::LoadDriver(void)
{
    // DLL laden
    if(!driver_wrapper.Load(DriverWrapper::DT_AUDIO, SETTINGS.driver.audio))
        return false;

    PDRIVER_CREATEAUDIOINSTANCE CreateAudioInstance = pto2ptf<PDRIVER_CREATEAUDIOINSTANCE>(driver_wrapper.GetDLLFunction("CreateAudioInstance"));

    // Instanz erzeugen
    if(!(audiodriver = CreateAudioInstance(this, VideoDriverWrapper::inst().GetWindowPointer())))
        return false;

    if(!audiodriver->Initialize())
    {
        delete audiodriver;
        audiodriver = NULL;
        return false;
    }

    return true;
}


///////////////////////////////////////////////////////////////////////////////
/**
 *  Lädt einen Sound.
 *
 *  @param[in] type      Typ (Musik/Effekt)
 *  @param[in] data_type Datentyp
 *  @param[in] data      Datenblock
 *  @param[in] size      Größe des Datenblocks
 *
 *  @return Sounddeskriptor bei Erfolg, @p NULL bei Fehler
 *
 *  @author FloSoft
 */
Sound* AudioDriverWrapper::LoadMusic(unsigned int data_type, unsigned char* data, unsigned int size)
{
    if(audiodriver == NULL)
        return NULL;

    return audiodriver->LoadMusic(data_type, data, size);
}

Sound* AudioDriverWrapper::LoadEffect(unsigned int data_type, unsigned char* data, unsigned int size)
{
    if(audiodriver == NULL)
        return NULL;

    return audiodriver->LoadEffect(data_type, data, size);
}


unsigned AudioDriverWrapper::PlayEffect(Sound* sound, const unsigned char volume, const bool loop)
{
    if(audiodriver == NULL)
        return 0;

    return audiodriver->PlayEffect(sound, volume, loop);
}

void AudioDriverWrapper::StopEffect(const unsigned int play_id)
{
    if(audiodriver == NULL)
        return;

    return audiodriver->StopEffect(play_id);
}

void AudioDriverWrapper::Msg_MusicFinished()
{
    // MusicManager Bescheid sagen
    MusicPlayer::inst().MusicFinished();
}

