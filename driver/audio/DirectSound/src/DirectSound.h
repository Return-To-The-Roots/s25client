// $Id: DirectSound.h 6458 2010-05-31 11:38:51Z FloSoft $
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
#ifndef DIRECTSOUND_H_INCLUDED
#define DIRECTSOUND_H_INCLUDED

#include <AudioDriver.h>

#include <dsound.h>
#include <vector>

/// Anzahl der Channels
const unsigned CHANNELS_COUNT = 32;

class DS_Effect;
 
/// Klasse für den DirectSound Audiotreiber
class AudioDirectSound : public AudioDriver
{
private:

	/// DirectSound Interface
	LPDIRECTSOUND8 ds; 
	/// Fenster-Handle
	HWND hWnd;
	/// Device ID vom MCI
	MCIDEVICEID mci_device_id;
	/// Erstellte Sounds
	std::vector<DS_Effect*> effects;
	/// Gesamtlautstärke der Effekte
	unsigned char effects_master_volume;
	/// Hook, um MCI-Messages abzufangen
	HHOOK mci_hook;
	/// Musik-Wiederholungen, die noch gespielt werden müssen
	unsigned music_repeats;

private:

	/// Gibt Effekt zu bestimmter Play-ID zurück
	DS_Effect * GetEffect(const unsigned play_id);
	/// Spielt Musik nochmal von vorne
	void PlayMusicAgain();

	/// Hook-Prozedur, um MCI-Messages abzufangen
	static LRESULT CALLBACK HookCallback(int nCode,WPARAM wParam,LPARAM lParam);


public:
	/// Konstruktor von @p AudioSDL.
	AudioDirectSound(AudioDriverLoaderInterface * CallBack, const HWND hWnd);

	/// Destruktor von @p AudioDirectSound.
	~AudioDirectSound(void);

	/// Treiberinitialisierungsfunktion.
	bool Initialize(void);

	/// Treiberaufräumfunktion.
	void CleanUp(void);

	Sound *LoadEffect(unsigned int data_type, unsigned char *data, unsigned long size);
	Sound *LoadMusic(unsigned int data_type, unsigned char *data, unsigned long size);

	/// Spielt Sound ab
	unsigned int PlayEffect(Sound *sound, const unsigned char volume, const bool loop);
	/// Spielt Midi ab
	void PlayMusic(Sound * sound, const unsigned repeats);
	/// Stoppt die Musik.
	void StopMusic(void);
	/// Wird der Sound (noch) abgespielt?
	bool IsEffectPlaying(const unsigned play_id);
	/// Stoppt einen Sound
	void StopEffect(const unsigned play_id); 
	/// Verändert die Lautstärke von einem abgespielten Sound (falls er noch abgespielt wird)
	void ChangeVolume(const unsigned play_id,const unsigned char volume);

	void SetMasterEffectVolume(unsigned char volume);
	void SetMasterMusicVolume(unsigned char volume);
};



#endif
