// $Id: DS_Effect.cpp 6458 2010-05-31 11:38:51Z FloSoft $
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
#include "DS_Effect.h"
#include <memory.h>

/// Konstruktor von @p SoundSDL_Effect.
DS_Effect::DS_Effect(LPDIRECTSOUND8 ds,const unsigned short id,const unsigned char * const data, const unsigned length) : id(id)
{
	WAVEFORMATEX wfe;
	wfe.wFormatTag = WAVE_FORMAT_PCM;
	wfe.nChannels = 1;
	wfe.nSamplesPerSec = 44100;
	wfe.wBitsPerSample = 16;
	wfe.nBlockAlign = wfe.nChannels*wfe.wBitsPerSample/8;
	wfe.nAvgBytesPerSec = wfe.nSamplesPerSec * wfe.nBlockAlign;
	wfe.cbSize = 0;
	
	DSBUFFERDESC desc;
	desc.dwSize = sizeof(desc);
	desc.dwFlags = DSBCAPS_CTRLVOLUME;
	desc.dwBufferBytes = length;
	desc.dwReserved = 0;
	desc.lpwfxFormat = &wfe;
	desc.guid3DAlgorithm = DS3DALG_DEFAULT;

	LPDIRECTSOUNDBUFFER buffer;
	if(ds->CreateSoundBuffer(&desc,&buffer,NULL) != DS_OK)
		return;

	if(buffer->QueryInterface(IID_IDirectSoundBuffer8,(void**)&play_instances[0].sound_buffer) != S_OK)
		return;

	void * lock_data, *lock_data2;
	DWORD data_size, data_size2;
	if(play_instances[0].sound_buffer->Lock(0,0,&lock_data,&data_size,&lock_data2,&data_size2,DSBLOCK_ENTIREBUFFER) != DS_OK)
		return;

	if(lock_data2 != NULL || data_size < length || data_size2 != 0)
	{
		play_instances[0].sound_buffer->Unlock(lock_data,data_size,lock_data2,data_size2);
		return;
	}

	memcpy(lock_data,data,length);
	if(data_size-length)
		memset(&static_cast<unsigned char*>(lock_data)[length],0,data_size-length);

	if(play_instances[0].sound_buffer->Unlock(lock_data,data_size,lock_data2,data_size2) != DS_OK)
		return;
}


/// Destruktor von @p SoundSDL_Effect.
DS_Effect::~DS_Effect()
{
	// Freien Buffer suchen, der noch nicht abgespielt wird
	for(unsigned i = 0;i<DS_Effect::MAX_SOUNDBUFFERS_SIMUTANEOUSLY;++i)
	{
		// Soundbuffer gültig?
		if(play_instances[i].sound_buffer)
			play_instances[i].sound_buffer->Release();
	}
}

/// Setzt Lautstärke einer PlayInstance (0-255)
void DS_Effect::SetVolume(PlayInstance& pi, const unsigned char effects_master_volume,const unsigned char volume)
{
	long val = long(255-volume)*DSBVOLUME_MIN/4/255 + long(255-effects_master_volume)*DSBVOLUME_MIN/3/255;
	pi.sound_buffer->SetVolume(val);
}

/// Spielt eine bestimmte PlayInstance ab
bool DS_Effect::PlayPlayInstance(PlayInstance& pi, const unsigned char effects_master_volume,const const unsigned char volume, const bool loop, const unsigned play_id)
{
	SetVolume(pi,effects_master_volume,volume);

	// Abspielen
	if(pi.sound_buffer->Play(0,0,loop ? DSBPLAY_LOOPING : 0) != DS_OK)
		return false;

	// Play-ID merken
	pi.play_id = play_id;

	return true;
}

/// Spielt Buffer ab, gibt zurück, ob erfolgreich
bool DS_Effect::Play(LPDIRECTSOUND8 ds, const unsigned char effects_master_volume,const const unsigned char volume, const bool loop, const unsigned play_id)
{
	// Freien Buffer suchen, der noch nicht abgespielt wird
	for(unsigned i = 0;i<DS_Effect::MAX_SOUNDBUFFERS_SIMUTANEOUSLY;++i)
	{
		// Soundbuffer noch nicht gültig?
		if(!play_instances[i].sound_buffer)
		{
			// MS-COM-Bürokratie, Interfacekonvertiererei
			LPDIRECTSOUNDBUFFER tmp1;
			if(play_instances[0].sound_buffer->QueryInterface(IID_IDirectSoundBuffer,(void**)&tmp1)
				!= DS_OK)
				return false;
			LPDIRECTSOUNDBUFFER tmp2;

			// Dann entsprechend duplizieren
			if(ds->DuplicateSoundBuffer(play_instances[0].sound_buffer,&tmp2) != DS_OK)
				// Fehlgeschlagen, Sound kann nicht abgespielt werden
				return false;

			// MS-COM-Bürokratie, Interfacekonvertiererei
			if(tmp2->QueryInterface(IID_IDirectSoundBuffer8,(void**)&play_instances[i].sound_buffer) != DS_OK)
				return false;

			// Abspielen
			return PlayPlayInstance(play_instances[i],effects_master_volume,volume,loop,play_id);
		}
		
		else
		{
			DWORD status;
			if(play_instances[i].sound_buffer->GetStatus(&status)!= DS_OK)
				return 0;

			// Soundbuffer nicht (mehr) abgespielt?
			if((status & DSBSTATUS_PLAYING) == 0)
				// Dann von neuen abspielen
				return PlayPlayInstance(play_instances[i],effects_master_volume,volume,loop,play_id);
		}
	}

	// Keine freien Buffer mehr vorhandne, es kann nicht abgespielt werden
	return false;
}

/// Gibt zu bestimmer Play-ID die PlayInstance zurück bzw. NULL, wenn keine gefunden wurde
DS_Effect::PlayInstance * DS_Effect::GetPI(const unsigned play_id)
{
	for(unsigned i = 0;i<this->MAX_SOUNDBUFFERS_SIMUTANEOUSLY;++i)
	{
		if(!this->play_instances[i].sound_buffer)
			// Alle belegten Soundbuffer abgeklappert, nichts gefunden
			return NULL;
		
		if(play_instances[i].play_id == play_id)
			return &play_instances[i];
	}

	// Alle Soundbuffer durchsucht, nichts gefunden
	return NULL;
}

/// Stoppt bestimmten Buffer
void DS_Effect::Stop(const unsigned play_id)
{
	PlayInstance * pi = GetPI(play_id);

	if(pi)
		pi->sound_buffer->Stop();
}

/// Verändert Lautstärke von abgespielten Buffer
void DS_Effect::ChangeVolume(const unsigned play_id, const unsigned char effects_master_volume, const unsigned char volume)
{
	PlayInstance * pi = GetPI(play_id);

	if(pi)
		SetVolume(*pi,effects_master_volume,volume);
}

/// Wird bestimmer Sound noch abgespielt?
bool DS_Effect::IsPlaying(const unsigned play_id)
{
	PlayInstance * pi = GetPI(play_id);

	if(pi)
	{
		DWORD status;
		if(pi->sound_buffer->GetStatus(&status)!= DS_OK)
			return false;

		// Soundbuffer noch abgespielt?
		if((status & DSBSTATUS_PLAYING))
			return true;
		else
			return false;
	}
	else
		return false;
}
