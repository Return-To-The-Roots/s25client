// $Id: Debug.cpp 7688 2011-12-30 09:33:43Z marcus $
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

#include "Debug.h"

DebugInfo::DebugInfo() : Socket()
{
	Connect("188.40.245.45", 4123, false, (Socket::PROXY_TYPE)SETTINGS.proxy.typ, SETTINGS.proxy.proxy, SETTINGS.proxy.port);

	Send("RTTRDBG", 7);

	// Protocol Version
	SendUnsigned(0x00000001);

	// OS
#ifdef _WIN32
	SendString("WIN");
#elif defined __APPLE__
	SendString("MAC");
#else
	SendString("LNX");
#endif

	// Bits
	SendUnsigned(sizeof(void *) << 3);

	SendString(GetWindowVersion());
	SendString(GetWindowRevision());

	SendUnsigned(GAMECLIENT.GetGFNumber());
}

DebugInfo::~DebugInfo()
{
	SendString("DONE");
	Close();
}

bool DebugInfo::Send(const void *buffer, int length)
{
	char *ptr = (char *) buffer;

	while (length > 0)
	{
		int res = Socket::Send(ptr, length);

		if (res >= 0)
		{
			ptr += res;
			length -= res;
		} else
		{
			fprintf(stderr, "failed to send: %i left\n", length);
			return(false);
		}
	}

	return(true);
}


bool DebugInfo::SendUnsigned(unsigned i)
{
	return(Send(&i, 4));
}

bool DebugInfo::SendSigned(signed i)
{
	return(Send(&i, 4));
}

bool DebugInfo::SendString(const char *str, unsigned len)
{
	if (len == 0)
	{
		len  = strlen(str) + 1;
	}

	if (!SendUnsigned(len))
	{
		return(false);
	}

	return(Send(str, len));
}

bool DebugInfo::SendStackTrace()
{
	void *stacktrace[63];

#if defined _WIN32
	unsigned num_frames = CaptureStackBackTrace(0, 63, stacktrace, NULL);
#else
	unsigned num_frames = backtrace(stacktrace, 63);
#endif
	if (!SendString("StackTrace"))
	{
		return(false);
	}

	num_frames *=  sizeof(void *);

	return(SendString((char *) &stacktrace, num_frames));
}

bool DebugInfo::SendReplay()
{
	// Replay mode is on, no recording of replays active
	if (!GAMECLIENT.IsReplayModeOn())
	{
		Replay rpl = GAMECLIENT.GetReplay();

		FILE *f = fopen(rpl.GetFileName().c_str(), "r");

		if (f != NULL)
		{
			// find out size of replay file by seeking to its end
			fseek(f, 0, SEEK_END);

			unsigned in_len = ftell(f);
			char *in = new char[in_len];

			fseek(f, 0, SEEK_SET);
			fread(in, in_len, 1, f);

			fclose(f);

			unsigned int out_len = in_len * 2 + 600;
			char *out = new char[out_len];

			// send size of replay via socket
			if (!SendString("Replay"))
			{
				return(false);
			}

			if (BZ2_bzBuffToBuffCompress(out, (unsigned int*)&out_len, in, in_len, 9, 0, 250) == BZ_OK)
			{
				if (!SendString(out, out_len))
				{
					delete[] in;
					delete[] out;
				
					return(false);
				}
			} else
			{
				if (!SendUnsigned(0))
				{
					delete[] in;
					delete[] out;
				
					return(false);
				}
			}

			delete[] in;
			delete[] out;
		}
	}

	return(true);
}

bool DebugInfo::SendAsyncLog(std::list<RandomEntry> *other)
{
	std::list<RandomEntry> *my = RANDOM.GetAsyncLog();

	std::list<RandomEntry>::iterator my_it = my->begin();
	std::list<RandomEntry>::iterator other_it = other->begin();

	// compare counters, adjust them so we're comparing the same counter numbers
	if (my_it->counter > other_it->counter)
	{
		for(; other_it != other->end(); ++other_it)
		{
			if (other_it->counter == my_it->counter)
				break;
		}
	} else if (my_it->counter < other_it->counter)
	{
		for(; my_it != my->end(); ++my_it)
		{
			if (other_it->counter == my_it->counter)
				break;
		}
	}

	// count identical lines
	unsigned identical = 0;
	while ((my_it != my->end()) && (other_it != other->end()) && (my_it->max == other_it->max) && (my_it->value == other_it->value) && (my_it->obj_id == other_it->obj_id))
	{
		++identical; ++my_it; ++other_it;
	}

	if (!SendString("AsyncLog"))
	{
		return(false);
	}

	// calculate size
	unsigned len =  4;
	unsigned cnt = 0;

	std::list<RandomEntry>::iterator my_start_it = my_it;
	std::list<RandomEntry>::iterator other_start_it = other_it;

	// if there were any identical lines, include only the last one
	if (identical)
	{
		--my_it;

		len += 4 + 4 + 4 + strlen(my_it->src_name) + 1 + 4 + 4 + 4;
		cnt++;

		++my_it;
	}

	while ((my_it != my->end()) && (other_it != other->end()))
	{
		len += 4 + 4 + 4 + strlen(my_it->src_name) + 1 + 4 + 4 + 4;
		len += 4 + 4 + 4 + strlen(other_it->src_name) + 1 + 4 + 4 + 4;

		cnt += 2;

		++my_it;
		++other_it;
	}

	my_it = my_start_it;
	other_it = other_start_it;

	if (!SendUnsigned(len))			return(false);
	if (!SendUnsigned(identical))		return(false);
	if (!SendUnsigned(cnt))			return(false);

	// if there were any identical lines, include only the last one
	if (identical)
	{
		--my_it;

		if (!SendUnsigned(my_it->counter))	return(false);
		if (!SendSigned(my_it->max))		return(false);
		if (!SendSigned(my_it->value))		return(false);
		if (!SendString(my_it->src_name))	return(false);
		if (!SendUnsigned(my_it->src_line))	return(false);
		if (!SendUnsigned(my_it->obj_id))	return(false);

		++my_it;
	}

	while ((my_it != my->end()) && (other_it != other->end()))
	{
		if (!SendUnsigned(my_it->counter))	return(false);
		if (!SendSigned(my_it->max))		return(false);
		if (!SendSigned(my_it->value))		return(false);
		if (!SendString(my_it->src_name))	return(false);
		if (!SendUnsigned(my_it->src_line))	return(false);
		if (!SendUnsigned(my_it->obj_id))	return(false);

		if (!SendUnsigned(other_it->counter))	return(false);
		if (!SendSigned(other_it->max))		return(false);
		if (!SendSigned(other_it->value))	return(false);
		if (!SendString(other_it->src_name))	return(false);
		if (!SendUnsigned(other_it->src_line))	return(false);
		if (!SendUnsigned(other_it->obj_id))	return(false);

		++my_it;
		++other_it;
	}

	return(true);
}



