// $Id: DS_Effect.h 6458 2010-05-31 11:38:51Z FloSoft $
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
#ifndef DS_EFFECT_H_INCLUDED
#define DS_EFFECT_H_INCLUDED

#include <Sound.h>
#include <dsound.h>

class DS_Effect : public Sound
{
private:

	/// ID dieses Sounds zur Durchnummerierung in der DirectSound
	const unsigned short id;

	/// Maximale Soundbuffer, die gleichzeitig gespielt werden können
	static const unsigned MAX_SOUNDBUFFERS_SIMUTANEOUSLY = 64;

	struct PlayInstance
	{
		PlayInstance() : sound_buffer(NULL), play_id(0) {}

		/// Sound-Buffer, der bespielt wird
		LPDIRECTSOUNDBUFFER8 sound_buffer;
		// Generierte Abspiel-ID
		unsigned play_id;
	};
		
	/// Soundbuffer, die bespielt werden können
	PlayInstance play_instances[MAX_SOUNDBUFFERS_SIMUTANEOUSLY];

public:

	/// Konstruktor von @p SoundSDL_Effect.
	DS_Effect(LPDIRECTSOUND8 ds,const unsigned short id,const unsigned char * const data, const unsigned length);

	/// Destruktor von @p SoundSDL_Effect.
	~DS_Effect();

	/// Spielt Buffer ab
	bool Play(LPDIRECTSOUND8 ds, const unsigned char effects_master_volume,const const unsigned char volume, const bool loop, const unsigned play_id);

	/// Stoppt bestimmten Buffer
	void Stop(const unsigned play_id);
	/// Verändert Lautstärke von abgespielten Buffer
	void ChangeVolume(const unsigned play_id, const unsigned char effects_master_volume, const unsigned char volume);
	/// Wird bestimmer Sound noch abgespielt?
	bool IsPlaying(const unsigned play_id);

	/// Gibt ID zurück
	unsigned GetID() const { return id; }


private:

	/// Setzt Lautstärke einer PlayInstance (0-255)
	void SetVolume(PlayInstance& pi, const unsigned char effects_master_volume,const unsigned char volume);
	/// Spielt eine bestimmte PlayInstance ab, gibt Play-ID zurück
	bool PlayPlayInstance(PlayInstance& pi, const unsigned char effects_master_volume, const const unsigned char volume, const bool loop, const unsigned play_id);
	/// Gibt zu bestimmer Play-ID die PlayInstance zurück bzw. NULL, wenn keine gefunden wurde
	PlayInstance * GetPI(const unsigned play_id);
};

#endif // !SOUNDSDL_EFFECT_H_INCLUDED
