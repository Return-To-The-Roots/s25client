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

///////////////////////////////////////////////////////////////////////////////

#include "rttrDefines.h" // IWYU pragma: keep
#include "Debug.h"
#include "RTTR_Version.h"
#include "Replay.h"
#include "Settings.h"
#include "network/GameClient.h"
#include "libutil/Log.h"
#include "libutil/unique_ptr.h"
#include <boost/endian/arithmetic.hpp>
#include <boost/endian/conversion.hpp>
#include <boost/foreach.hpp>
#include <boost/static_assert.hpp>
#include <boost/type_traits/conditional.hpp>
#include <bzlib.h>
#include <vector>

#if defined(_WIN32) || defined(__CYGWIN__)
#define RTTR_USE_WIN_API
#endif

#ifdef RTTR_USE_WIN_API
#ifdef HAVE_DBGHELP_H
#include <windows.h>
// Disable warning for faulty nameless enum typedef (check sfImage.../hdBase...)
#pragma warning(push)
#pragma warning(disable : 4091)
#include <dbghelp.h>
#pragma warning(pop)

#ifdef _MSC_VER
#pragma comment(lib, "dbgHelp.lib")
#else
typedef WINBOOL(WINAPI* SymInitializeType)(HANDLE hProcess, PSTR UserSearchPath, WINBOOL fInvadeProcess);
typedef WINBOOL(WINAPI* SymCleanupType)(HANDLE hProcess);
typedef VOID(WINAPI* RtlCaptureContextType)(PCONTEXT ContextRecord);
typedef WINBOOL(WINAPI* StackWalkType)(DWORD MachineType, HANDLE hProcess, HANDLE hThread, LPSTACKFRAME64 StackFrame, PVOID ContextRecord,
                                       PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemoryRoutine,
                                       PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine,
                                       PGET_MODULE_BASE_ROUTINE64 GetModuleBaseRoutine, PTRANSLATE_ADDRESS_ROUTINE64 TranslateAddress);
#endif // _MSC_VER
#endif // HAVE_DBGHELP_H

#else
#include <execinfo.h>
#endif

#ifdef RTTR_USE_WIN_API
#ifdef HAVE_DBGHELP_H
bool captureBacktrace(std::vector<void*>& stacktrace, LPCONTEXT ctx = NULL)
{
    CONTEXT context;
#ifndef _MSC_VER

    HMODULE kernel32 = LoadLibraryA("kernel32.dll");
    HMODULE dbghelp = LoadLibraryA("dbghelp.dll");

    if(!kernel32 || !dbghelp)
        return false;

    RtlCaptureContextType RtlCaptureContext = (RtlCaptureContextType)(GetProcAddress(kernel32, "RtlCaptureContext"));

    SymInitializeType SymInitialize = (SymInitializeType)(GetProcAddress(dbghelp, "SymInitialize"));
    SymCleanupType SymCleanup = (SymCleanupType)(GetProcAddress(dbghelp, "SymCleanup"));
    StackWalkType StackWalk64 = (StackWalkType)(GetProcAddress(dbghelp, "StackWalk64"));
    PFUNCTION_TABLE_ACCESS_ROUTINE64 SymFunctionTableAccess64 =
      (PFUNCTION_TABLE_ACCESS_ROUTINE64)(GetProcAddress(dbghelp, "SymFunctionTableAccess64"));
    PGET_MODULE_BASE_ROUTINE64 SymGetModuleBase64 = (PGET_MODULE_BASE_ROUTINE64)(GetProcAddress(dbghelp, "SymGetModuleBase64"));

    if(!SymInitialize || !StackWalk64 || !SymFunctionTableAccess64 || !SymGetModuleBase64 || !RtlCaptureContext)
        return false;
#endif

    const HANDLE process = GetCurrentProcess();
    if(!SymInitialize(process, NULL, true))
        return false;

    if(!ctx)
    {
        context.ContextFlags = CONTEXT_FULL;
        RtlCaptureContext(&context);
        ctx = &context;
    }

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

    HANDLE thread = GetCurrentThread();
#ifdef _WIN64
    DWORD machineType = IMAGE_FILE_MACHINE_AMD64;
#else
    DWORD machineType = IMAGE_FILE_MACHINE_I386;
#endif

    for(unsigned i = 0; i < stacktrace.size(); i++)
    {
        if(!StackWalk64(machineType, process, thread, &frame, ctx, NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL))
        {
            stacktrace.resize(i);
            break;
        }
        LOG.write("Reading stack frame %1%\n", LogTarget::Stdout) % i;
        stacktrace[i] = (void*)frame.AddrPC.Offset;
    }

    SymCleanup(process);
    return true;
}
#else  // HAVE_DBGHELP_H
bool captureBacktrace(std::vector<void*>&, void* = NULL)
{
    return false;
}
#endif // HAVE_DBGHELP_H

#else
void captureBacktrace(std::vector<void*>& stacktrace)
{
    unsigned num_frames = backtrace(&stacktrace[0], stacktrace.size());
    stacktrace.resize(num_frames);
}
#endif

DebugInfo::DebugInfo()
{
    sock.Connect("debug.rttr.info", 4123, false, SETTINGS.proxy);

    Send("RTTRDBG", 7);

    // Protocol Version
    SendUnsigned(2);

// OS
#if defined _WIN32 || defined __CYGWIN__
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
    SendUnsigned(sizeof(void*) * 8u);

    SendString(RTTR_Version::GetVersionDate());
    SendString(RTTR_Version::GetRevision());

    SendUnsigned(GAMECLIENT.GetGFNumber());
}

DebugInfo::~DebugInfo()
{
    SendString("DONE");
    sock.Close();
}

std::vector<void*> DebugInfo::GetStackTrace(void* ctx)
{
    std::vector<void*> stacktrace(256);
#ifdef _MSC_VER
    if(!captureBacktrace(stacktrace, static_cast<LPCONTEXT>(ctx)))
        stacktrace.clear();
#else
    RTTR_UNUSED(ctx);
    captureBacktrace(stacktrace);
#endif
    return stacktrace;
}

bool DebugInfo::Send(const void* buffer, int length)
{
    char* ptr = (char*)buffer;

    while(length > 0)
    {
        int res = sock.Send(ptr, length);

        if(res >= 0)
        {
            ptr += res;
            length -= res;
        } else
        {
            fprintf(stderr, "failed to send: %i left\n", length);
            return (false);
        }
    }

    return (true);
}

bool DebugInfo::SendUnsigned(uint32_t i)
{
    // Debug server does not handle endianness and is little endian... TODO: Fix server
    boost::endian::native_to_little_inplace(i);
    return (Send(&i, 4));
}

bool DebugInfo::SendSigned(int32_t i)
{
    // Debug server does not handle endianness and is little endian... TODO: Fix server
    boost::endian::native_to_little_inplace(i);
    return (Send(&i, 4));
}

bool DebugInfo::SendString(const char* str, unsigned len)
{
    if(len == 0)
        len = strlen(str) + 1;

    if(!SendUnsigned(len))
        return (false);

    return (Send(str, len));
}

bool DebugInfo::SendString(const std::string& str)
{
    return SendString(str.c_str(), str.length() + 1); // +1 to include NULL terminator
}

bool DebugInfo::SendStackTrace(const std::vector<void*>& stacktrace)
{
    if(stacktrace.empty())
        return false;

    LOG.write("Will now send %1% stack frames\n") % stacktrace.size();

    if(!SendString("StackTrace"))
        return false;

    typedef boost::conditional<sizeof(void*) == 4, boost::endian::little_int32_t, boost::endian::little_int64_t>::type littleVoid_t;
    BOOST_STATIC_ASSERT_MSG(sizeof(void*) <= sizeof(littleVoid_t), "Size of pointer did not fit!");
    std::vector<littleVoid_t> endStacktrace;
    endStacktrace.reserve(stacktrace.size());
    BOOST_FOREACH(void* ptr, stacktrace)
        endStacktrace.push_back(reinterpret_cast<littleVoid_t::value_type>(ptr));

    unsigned stacktraceLen = sizeof(littleVoid_t) * endStacktrace.size();
    return SendString(reinterpret_cast<const char*>(&endStacktrace[0]), stacktraceLen);
}

bool DebugInfo::SendReplay()
{
    LOG.write("Sending replay...\n");

    // Replay mode is on, no recording of replays active
    if(!GAMECLIENT.IsReplayModeOn())
    {
        Replay* rpl = GAMECLIENT.GetReplay();

        if(!rpl || !rpl->IsRecording())
            return true;

        BinaryFile& f = rpl->GetFile();

        f.Flush();

        if(!SendString("Replay"))
            return false;
        if(SendFile(f))
            return true;
        // Empty replay
        SendUnsigned(0);
        return false;
    } else
    {
        LOG.write("-> Already in replay mode, do not send replay\n");
    }

    return true;
}

bool DebugInfo::SendAsyncLog(const std::string& asyncLogFilepath)
{
    BinaryFile file;
    if(!file.Open(asyncLogFilepath, OFM_READ))
        return false;

    if(!SendString("AsyncLog"))
        return false;

    if(SendFile(file))
        return true;
    // Empty
    SendUnsigned(0);
    return false;
}

bool DebugInfo::SendFile(BinaryFile& file)
{
    file.Seek(0, SEEK_END);
    unsigned fileSize = file.Tell();

    LOG.write("- File size: %u\n") % fileSize;

    libutil::unique_ptr<char[]> fileData(new char[fileSize]);
    unsigned compressed_len = fileSize * 2 + 600;
    libutil::unique_ptr<char[]> compressed(new char[compressed_len]);

    file.Seek(0, SEEK_SET);
    file.ReadRawData(fileData.get(), fileSize);

    LOG.write("- Compressing...\n");
    if(BZ2_bzBuffToBuffCompress(compressed.get(), (unsigned*)&compressed_len, fileData.get(), fileSize, 9, 0, 250) == BZ_OK)
    {
        LOG.write("- Sending...\n");

        if(SendString(compressed.get(), compressed_len))
        {
            LOG.write("-> success\n");
            return true;
        }

        LOG.write("-> Sending file failed :(\n");
    } else
        LOG.write("-> BZ2 compression failed.\n");
    return false;
}
