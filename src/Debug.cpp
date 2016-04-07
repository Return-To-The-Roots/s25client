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

///////////////////////////////////////////////////////////////////////////////

#include "defines.h" // IWYU pragma: keep
#include "Debug.h"
#include "build_version.h"

#include "Settings.h"
#include "GameClient.h"
#include "helpers/Deleter.h"

#ifdef _WIN32
#   include <windows.h>

// Disable warning for faulty nameless enum typdef (check sfImage.../hdBase...)
#   pragma warning(push)
#   pragma warning(disable: 4091)
#   include <dbghelp.h>

#   pragma warning(pop)
#else
#   include <execinfo.h>
#endif

#include "../libutil/src/Log.h"
#include <boost/interprocess/smart_ptr/unique_ptr.hpp>
#include <bzlib.h>

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

#ifdef _WIN32

typedef USHORT (WINAPI* CaptureStackBackTraceType)(ULONG, ULONG, PVOID*, PULONG);

#   ifndef _MSC_VER
typedef WINBOOL (IMAGEAPI* SymInitializeType)(HANDLE hProcess, PSTR UserSearchPath, WINBOOL fInvadeProcess);
typedef WINBOOL (IMAGEAPI* SymCleanupType)(HANDLE hProcess);
typedef NTSYSAPI VOID (NTAPI* RtlCaptureContextType)(PCONTEXT ContextRecord);

typedef WINBOOL (IMAGEAPI* StackWalkType)(DWORD MachineType, HANDLE hProcess, HANDLE hThread, LPSTACKFRAME64 StackFrame, PVOID ContextRecord, PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemoryRoutine, PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine, PGET_MODULE_BASE_ROUTINE64 GetModuleBaseRoutine, PTRANSLATE_ADDRESS_ROUTINE64 TranslateAddress);
#   else
#       undef CaptureStackBackTrace
#       pragma comment(lib, "dbgHelp.lib")
#   endif
#endif

DebugInfo::DebugInfo()
{
    sock.Connect("debug.rttr.info", 4123, false, (Socket::PROXY_TYPE)SETTINGS.proxy.typ, SETTINGS.proxy.proxy, SETTINGS.proxy.port); //-V807

    Send("RTTRDBG", 7);

    // Protocol Version
    SendUnsigned(0x00000001);

    // OS
#ifdef _WIN32
    SendString("WIN");
    // TODO: These should be based on uname(3) output.
#elif defined __APPLE__
    SendString("MAC");
#elif defined __FreeBSD__
    SendString("BSD");
#else
    SendString("LNX");
#endif

    // Bits
    SendUnsigned(sizeof(void*) << 3);

    SendString(GetWindowVersion());
    SendString(GetWindowRevision());

    SendUnsigned(GAMECLIENT.GetGFNumber());
}

DebugInfo::~DebugInfo()
{
    SendString("DONE");
    sock.Close();
}

bool DebugInfo::Send(const void* buffer, int length)
{
    char* ptr = (char*) buffer;

    while (length > 0)
    {
        int res = sock.Send(ptr, length);

        if (res >= 0)
        {
            ptr += res;
            length -= res;
        }
        else
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

bool DebugInfo::SendString(const char* str, unsigned len)
{
    if (len == 0)
        len  = strlen(str) + 1;

    if (!SendUnsigned(len))
        return(false);

    return(Send(str, len));
}

bool DebugInfo::SendString(const std::string& str)
{
    return SendString(str.c_str(), str.length() + 1); // +1 to include NULL terminator
}

#ifdef _WIN32
void* CALLBACK FunctionTableAccess(HANDLE hProcess, DWORD64 AddrBase)
{
    return NULL;
}
#endif

#ifdef _MSC_VER
bool DebugInfo::SendStackTrace(LPCONTEXT ctx)
#else
bool DebugInfo::SendStackTrace()
#endif
{
    const unsigned int maxTrace = 256;
    void* stacktrace[maxTrace];

#ifdef _WIN32

#ifndef _MSC_VER
    CONTEXT context;
    LPCONTEXT ctx = NULL;

    HMODULE kernel32 = LoadLibrary("kernel32.dll");
    HMODULE dbghelp = LoadLibrary("dbghelp.dll");

    if ((!kernel32) || (!dbghelp))
    {
        return(false);
    }

    RtlCaptureContextType RtlCaptureContext = (RtlCaptureContextType)(GetProcAddress(kernel32, "RtlCaptureContext"));

    SymInitializeType SymInitialize = (SymInitializeType)(GetProcAddress(dbghelp, "SymInitialize"));
    SymCleanupType SymCleanup = (SymCleanupType)(GetProcAddress(dbghelp, "SymCleanup"));

    StackWalkType StackWalk64 = (StackWalkType)(GetProcAddress(dbghelp, "StackWalk64"));
    PFUNCTION_TABLE_ACCESS_ROUTINE64 SymFunctionTableAccess64 = (PFUNCTION_TABLE_ACCESS_ROUTINE64)(GetProcAddress(dbghelp, "SymFunctionTableAccess64"));
    PGET_MODULE_BASE_ROUTINE64 SymGetModuleBase64 = (PGET_MODULE_BASE_ROUTINE64)(GetProcAddress(dbghelp, "SymGetModuleBase64"));

    if ((!SymInitialize) || (!StackWalk64) || (!SymFunctionTableAccess64) || (!SymGetModuleBase64) || (!RtlCaptureContext))
    {
        return(false);
    }
#endif

    if (!SymInitialize(GetCurrentProcess(), NULL, true))
    {
        return(false);
    }

#ifndef _MSC_VER
    if (!ctx)
    {
        context.ContextFlags = CONTEXT_FULL;
        RtlCaptureContext(&context);
        ctx = &context;
    }
#endif

    STACKFRAME64 frame;
    memset(&frame, 0, sizeof(frame));

#ifdef _WIN64
    frame.AddrPC.Offset = ctx->Rip;
    frame.AddrStack.Offset = ctx->Rsp;
    frame.AddrFrame.Offset = ctx->Rbp;
#else
    frame.AddrPC.Offset = ctx->Eip;
    frame.AddrStack.Offset = ctx->Esp;
    frame.AddrFrame.Offset = ctx->Ebp;
#endif

    frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrStack.Mode = AddrModeFlat;
    frame.AddrFrame.Mode = AddrModeFlat;

    HANDLE process = GetCurrentProcess();
    HANDLE thread = GetCurrentThread();

    unsigned num_frames = 0;
    while (StackWalk64(
#ifdef _WIN64
                IMAGE_FILE_MACHINE_AMD64,
#else
                IMAGE_FILE_MACHINE_I386,
#endif
                process, thread, &frame,
                ctx, NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL) && (num_frames < maxTrace))
    {
        LOG.lprintf("Reading stack frame %d\n", num_frames);
        stacktrace[num_frames++] = (void*) frame.AddrPC.Offset;
    }

    SymCleanup(GetCurrentProcess());

    /*CaptureStackBackTraceType CaptureStackBackTrace = (CaptureStackBackTraceType)(GetProcAddress(LoadLibraryA("kernel32.dll"), "RtlCaptureStackBackTrace"));

    if (!CaptureStackBackTrace)
    {
        return(false);
    }

    unsigned num_frames = CaptureStackBackTrace(0, maxTrace, stacktrace, NULL);
    LOG.lprintf("Read Frames %d\n", num_frames);
    */
#else
    unsigned num_frames = backtrace(stacktrace, maxTrace);
#endif

    LOG.lprintf("Will now send %d stack frames\n", num_frames);

    if (!SendString("StackTrace"))
        return(false);

    num_frames *= sizeof(void*);

    return(SendString((char*) &stacktrace, num_frames));
}

bool DebugInfo::SendReplay()
{
    LOG.lprintf("Sending replay...\n");

    // Replay mode is on, no recording of replays active
    if (!GAMECLIENT.IsReplayModeOn())
    {
        Replay& rpl = GAMECLIENT.GetReplay();

        if(!rpl.IsValid())
            return true;

        BinaryFile* f = rpl.GetFile();

        if(!f) // no replay to send
            return true;

        f->Flush();

        unsigned replay_len = f->Tell();

        LOG.lprintf("- Replay length: %u\n", replay_len);

        boost::interprocess::unique_ptr<char, Deleter<char[]> > replay(new char[replay_len]);

        f->Seek(0, SEEK_SET);

        f->ReadRawData(replay.get(), replay_len);

        unsigned int compressed_len = replay_len * 2 + 600;
        boost::interprocess::unique_ptr<char, Deleter<char[]> > compressed(new char[compressed_len]);

        // send size of replay via socket
        if (!SendString("Replay"))
        {
            return false;
        }

        LOG.lprintf("- Compressing...\n");
        if (BZ2_bzBuffToBuffCompress(compressed.get(), (unsigned int*) &compressed_len, replay.get(), replay_len, 9, 0, 250) == BZ_OK)
        {
            LOG.lprintf("- Sending...\n");

            if (SendString(compressed.get(), compressed_len))
            {
                LOG.lprintf("-> success\n");
                return true;
            }

            LOG.lprintf("-> Sending replay failed :(\n");
        }
        else
        {
            LOG.lprintf("-> BZ2 compression failed.\n");
        }

        SendUnsigned(0);
        return false;
    }
    else
    {
        LOG.lprintf("-> Already in replay mode, do not send replay\n");
    }

    return true;
}

bool DebugInfo::SendAsyncLog(std::vector<RandomEntry>::const_iterator first_a, std::vector<RandomEntry>::const_iterator first_b,
                             const std::vector<RandomEntry> &a, const std::vector<RandomEntry> &b, unsigned identical)
{
    if (!SendString("AsyncLog"))
    {
        return(false);
    }

    // calculate size
    unsigned len =  4;
    unsigned cnt = 0;

    std::vector<RandomEntry>::const_iterator it_a = first_a;
    std::vector<RandomEntry>::const_iterator it_b = first_b;

    // if there were any identical lines, include only the last one
    if (identical)
    {
        // sizes of: counter, max, rngState
        //           string = length Bytes + 1 NULL terminator + 4B length
        //           srcLine, objId
        len += 4 + 4 + 4 + it_a->src_name.length() + 1 + 4 + 4 + 4;

        ++cnt; ++it_a; ++it_b;
    }

    while ((it_a != a.end()) && (it_b != b.end()))
    {
        len += 4 + 4 + 4 + it_a->src_name.length() + 1 + 4 + 4 + 4;
        len += 4 + 4 + 4 + it_b->src_name.length() + 1 + 4 + 4 + 4;

        cnt += 2;
        ++it_a; ++it_b;
    }

    if (!SendUnsigned(len))         return(false);
    if (!SendUnsigned(identical))   return(false);
    if (!SendUnsigned(cnt))         return(false);

    it_a = first_a;
    it_b = first_b;

    // if there were any identical lines, send only one each
    for(unsigned i = 0; i< identical; i++)
    {
        if (!SendUnsigned(it_a->counter))   return(false);
        if (!SendSigned(it_a->max))         return(false);
        if (!SendSigned(it_a->rngState))    return(false);
        if (!SendString(it_a->src_name))    return(false);
        if (!SendUnsigned(it_a->src_line))  return(false);
        if (!SendUnsigned(it_a->obj_id))    return(false);

        ++it_a; ++it_b;
    }

    while ((it_a != a.end()) && (it_b != b.end()))
    {
        if (!SendUnsigned(it_a->counter))   return(false);
        if (!SendSigned(it_a->max))         return(false);
        if (!SendSigned(it_a->rngState))    return(false);
        if (!SendString(it_a->src_name))    return(false);
        if (!SendUnsigned(it_a->src_line))  return(false);
        if (!SendUnsigned(it_a->obj_id))    return(false);

        if (!SendUnsigned(it_b->counter))   return(false);
        if (!SendSigned(it_b->max))         return(false);
        if (!SendSigned(it_b->rngState))    return(false);
        if (!SendString(it_b->src_name))    return(false);
        if (!SendUnsigned(it_b->src_line))  return(false);
        if (!SendUnsigned(it_b->obj_id))    return(false);

        ++it_a; ++it_b;
    }

    return(true);
}
