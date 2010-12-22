// $Id: DirectSound.cpp 6458 2010-05-31 11:38:51Z FloSoft $
//
// Copyright (c) 2005-2009 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Siedler II.5 RTTR.
//
// Siedler II.5 RTTR is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Siedler II.5 RTTR is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Siedler II.5 RTTR. If not, see <http://www.gnu.org/licenses/>.

///////////////////////////////////////////////////////////////////////////////
// Header
#include "DirectSound.h"
#include "AudioInterface.h"
#include "tempname.h"
#include "DS_Music.h"
#include "DS_Effect.h"


AudioDirectSound *nthis = NULL;


/// Größe der SoundBuffer
const unsigned SOUND_BUFFER_SIZE = 8192;

/// DLL-Handle
HINSTANCE hInstance_dll;

/// Einstiegspunkt in der DLL, um hInstance zu bekommen
BOOL WINAPI DllMain(
  HINSTANCE hinstDLL,
  DWORD fdwReason,
  LPVOID lpvReserved
)
{
	hInstance_dll = hinstDLL;

	return TRUE;
}



///////////////////////////////////////////////////////////////////////////////
/**
 *  Instanzierungsfunktion von @p AudioSDL.
 *
 *  @return liefert eine Instanz des jeweiligen Treibers
 *
 *  @author FloSoft
 */
DRIVERDLLAPI AudioDriver *CreateAudioInstance(AudioDriverLoaderInterface * CallBack, void * device_dependent)
{
	nthis = new AudioDirectSound(CallBack,(HWND)device_dependent);
	return nthis;
}

DRIVERDLLAPI const char * GetDriverName(void)
{
	return "(DirectSound) Audio via DirectSound / MCI";
}


/// Konstruktor von @p AudioSDL.
AudioDirectSound::AudioDirectSound(AudioDriverLoaderInterface * CallBack,const HWND hWnd) : AudioDriver(CallBack), ds(NULL),
hWnd(hWnd), mci_device_id(0), effects_master_volume(255), mci_hook(NULL), music_repeats(0)
{
}

/// Destruktor von @p AudioSDL.
AudioDirectSound::~AudioDirectSound(void)
{
	CleanUp();
}

/// Treiberinitialisierungsfunktion.
bool AudioDirectSound::Initialize(void)
{
	if(DirectSoundCreate8(NULL,&ds,NULL) != DS_OK)
		return false;

	if(ds->SetCooperativeLevel(hWnd,DSSCL_NORMAL) != DS_OK)
		return false;

	if((mci_hook = SetWindowsHookEx(WH_GETMESSAGE,AudioDirectSound::HookCallback,hInstance_dll,
		GetWindowThreadProcessId(hWnd,NULL))) == NULL)
		return false;

	// MCI Öffnen
    MCI_OPEN_PARMS mciOpenParams;
	memset(&mciOpenParams,0,sizeof(mciOpenParams));

	mciOpenParams.lpstrDeviceType = "sequencer";
    if (mciSendCommand(NULL, MCI_OPEN,
        MCI_OPEN_TYPE |MCI_OPEN_SHAREABLE ,
        (DWORD_PTR)(LPVOID) &mciOpenParams))
    {
        // Failed to open device. Don't close it; just return error.
        return false;
    }

	this->mci_device_id = mciOpenParams.wDeviceID;

	return true;
}

/// Treiberaufräumfunktion.
void AudioDirectSound::CleanUp(void)
{
	UnhookWindowsHookEx(this->mci_hook);

	if(ds != NULL)
	{
		ds->Release();
		ds = NULL;
	}
}

Sound *AudioDirectSound::LoadEffect(unsigned int data_type, unsigned char *data, unsigned long size)
{
	if(data_type == AudioDriver::AD_WAVE)
	{
		// data-Chunk suchen
		unsigned char chunk[4] = {'d','a','t','a'};
		if(memcmp(&data[36],chunk,4) != 0)
			return NULL;

		// Länge auslesen
		unsigned length;
		memcpy(&length,&data[40],4);

		DS_Effect * effect = new DS_Effect(ds,static_cast<unsigned short>(effects.size()),&data[44],length);
		effects.push_back(effect);
		return effect;
	}

	return NULL;
}

Sound *AudioDirectSound::LoadMusic(unsigned int data_type, unsigned char *data, unsigned long size)
{
	if(data_type == AudioDriver::AD_MIDI)
	{
		// Midis in temporäre Dateien speichern und diese dann mit MCI abspielen, nur Dateinamen erstmal merken
		char file[512];
		if(!tempname(file, 512))
			return 0;

		strncat(file, ".mid", 512);

		FILE *dat = fopen(file, "wb");
		if(!dat)
			return false;

		fwrite(data, 1, size, dat);
		fclose(dat);

		return new DS_Music(file);
	}

	return NULL;
}

/// Spielt Sound ab
unsigned int AudioDirectSound::PlayEffect(Sound *sound, const unsigned char volume, const bool loop)
{
	// Play-ID generieren
	unsigned play_id = AudioDriver::GeneratePlayID();

	// ID des Sounds in die höheren 16 Bits schreiben
	play_id |= (static_cast<DS_Effect*>(sound)->GetID()<<16);

	// Abspielen
	if(static_cast<DS_Effect*>(sound)->Play(ds,effects_master_volume,volume,loop,play_id))
		return play_id;
	else
		return 0;

}
/// Spielt Midi ab
void AudioDirectSound::PlayMusic(Sound * sound, const unsigned repeats)
{
	// mit MCI Datei Öffnen
    MCI_LOAD_PARMS mciLoadParms;
	memset(&mciLoadParms,0,sizeof(mciLoadParms));
    mciLoadParms.lpfilename = static_cast<DS_Music*>(sound)->music_file;
	MCIERROR error;
    if (error = mciSendCommand(mci_device_id, MCI_LOAD,MCI_LOAD_FILE,(DWORD_PTR)(LPVOID) &mciLoadParms))
    {
        // Failed to open device. Don't close it; just return error.
	 char szErrorBuf[MAXERRORLENGTH];
		mciGetErrorString(error, (LPSTR) szErrorBuf, MAXERRORLENGTH);

        return;
    }


	PlayMusicAgain();

	music_repeats = repeats;
}
/// Stoppt die Musik.
void AudioDirectSound::StopMusic(void)
{
	mciSendCommand(mci_device_id,MCI_STOP,0,NULL);
}
/// Wird der Sound (noch) abgespielt?
bool AudioDirectSound::IsEffectPlaying(const unsigned play_id)
{
	return GetEffect(play_id)->IsPlaying(play_id);
}
/// Stoppt einen Sound
void AudioDirectSound::StopEffect(const unsigned play_id)
{
	GetEffect(play_id)->Stop(play_id);
} 
/// Verändert die Lautstärke von einem abgespielten Sound (falls er noch abgespielt wird)
void AudioDirectSound::ChangeVolume(const unsigned play_id,const unsigned char volume)
{
	GetEffect(play_id)->ChangeVolume(play_id,effects_master_volume,volume);
}

void AudioDirectSound::SetMasterEffectVolume(unsigned char volume)
{
	effects_master_volume = volume;
}
void AudioDirectSound::SetMasterMusicVolume(unsigned char volume)
{
	unsigned volume_left_right = 0xFF*volume | ((0xFF*volume) << 16);

	HMIDIOUT out;
	midiOutOpen(&out,0,NULL,NULL,0);
	midiOutSetVolume(out,volume_left_right);
	midiOutClose(out);
}

/// Gibt Effekt zu bestimmter Play-ID zurück
DS_Effect * AudioDirectSound::GetEffect(const unsigned play_id)
{
	// Höherwertiger Anteil ist die Sound-ID
	return effects[play_id>>16];
}

void AudioDirectSound::PlayMusicAgain()
{
	// Abspielen
	MCI_PLAY_PARMS mciPlayParms;
	memset(&mciPlayParms,0,sizeof(mciPlayParms));
	mciPlayParms.dwCallback = (DWORD_PTR)hWnd;
    if (mciSendCommand(mci_device_id, MCI_PLAY, MCI_NOTIFY, 
        (DWORD_PTR)(LPVOID) &mciPlayParms))
    {
        mciSendCommand(mci_device_id, MCI_CLOSE, 0, NULL);
        return;
    }
}

LRESULT CALLBACK AudioDirectSound::HookCallback(int nCode,WPARAM wParam,LPARAM lParam)
{
	MSG *msg = (MSG *)lParam;

	switch(msg->message)
	{
	case MM_MCINOTIFY:
		{
			// Musik zu Ende gespielt
			// music_repeats = 0 --> immer wiederholen
			if(nthis->music_repeats == 0)
				nthis->PlayMusicAgain();
			else if(--nthis->music_repeats == 0)
			{
				// Alle Repeats durchlaufen, Callback Bescheid sagen
				nthis->adli->Msg_MusicFinished();
			}
			else
			{
				// Es sind noch Durchläufe zu erledigen
				nthis->PlayMusicAgain();
			}
		} break;
	}

	return CallNextHookEx(nthis->mci_hook,nCode,wParam,lParam);
}