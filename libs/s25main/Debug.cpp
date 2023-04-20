// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

///////////////////////////////////////////////////////////////////////////////

#include "Debug.h"
#include "RTTR_Version.h"
#include "Replay.h"
#include "Settings.h"
#include "network/GameClient.h"
#include "s25util/Log.h"
#include <boost/endian/arithmetic.hpp>
#include <boost/endian/conversion.hpp>
#include <boost/nowide/iostream.hpp>
#include <bzlib.h>
#include <memory>
#include <vector>

#if defined(_WIN32) || defined(__CYGWIN__)
#    define RTTR_USE_WIN_API
#endif

#ifdef RTTR_USE_WIN_API
#    ifdef HAVE_DBGHELP_H
#        include <windows.h>
// Disable warning for faulty nameless enum typedef (check sfImage.../hdBase...)
#        pragma warning(push)
#        pragma warning(disable : 4091)
#        include <dbghelp.h>
#        pragma warning(pop)

#        ifdef _MSC_VER
#            pragma comment(lib, "dbgHelp.lib")
#        else
typedef WINBOOL(WINAPI* SymInitializeType)(HANDLE hProcess, PSTR UserSearchPath, WINBOOL fInvadeProcess);
typedef WINBOOL(WINAPI* SymCleanupType)(HANDLE hProcess);
typedef VOID(WINAPI* RtlCaptureContextType)(PCONTEXT ContextRecord);
typedef WINBOOL(WINAPI* StackWalkType)(DWORD MachineType, HANDLE hProcess, HANDLE hThread, LPSTACKFRAME64 StackFrame,
                                       PVOID ContextRecord, PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemoryRoutine,
                                       PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine,
                                       PGET_MODULE_BASE_ROUTINE64 GetModuleBaseRoutine,
                                       PTRANSLATE_ADDRESS_ROUTINE64 TranslateAddress);
#        endif // _MSC_VER
#    endif     // HAVE_DBGHELP_H

#else
#    include <execinfo.h>
#endif

namespace {
#ifdef RTTR_USE_WIN_API
#    ifdef HAVE_DBGHELP_H
bool captureBacktrace(std::vector<void*>& stacktrace, LPCONTEXT ctx = nullptr) noexcept
{
    CONTEXT context;
#        ifndef _MSC_VER

    HMODULE kernel32 = LoadLibraryA("kernel32.dll");
    HMODULE dbghelp = LoadLibraryA("dbghelp.dll");

    if(!kernel32 || !dbghelp)
        return false;

#            if __GNUC__ >= 9
#                pragma GCC diagnostic push
#                pragma GCC diagnostic ignored "-Wcast-function-type"
        // error: cast between incompatible function types from
        // 'FARPROC' {aka 'int (__attribute__((stdcall)) *)()'}
        // to
        // 'RtlCaptureContextType' {aka 'void (__attribute__((stdcall)) *)(CONTEXT*)'}
        // [-Werror=cast-function-type]
        // and so on
#            endif
    RtlCaptureContextType RtlCaptureContext = (RtlCaptureContextType)(GetProcAddress(kernel32, "RtlCaptureContext"));

    SymInitializeType SymInitialize = (SymInitializeType)(GetProcAddress(dbghelp, "SymInitialize"));
    SymCleanupType SymCleanup = (SymCleanupType)(GetProcAddress(dbghelp, "SymCleanup"));
    StackWalkType StackWalk64 = (StackWalkType)(GetProcAddress(dbghelp, "StackWalk64"));
    PFUNCTION_TABLE_ACCESS_ROUTINE64 SymFunctionTableAccess64 =
      (PFUNCTION_TABLE_ACCESS_ROUTINE64)(GetProcAddress(dbghelp, "SymFunctionTableAccess64"));
    PGET_MODULE_BASE_ROUTINE64 SymGetModuleBase64 =
      (PGET_MODULE_BASE_ROUTINE64)(GetProcAddress(dbghelp, "SymGetModuleBase64"));
#            if __GNUC__ >= 9
#                pragma GCC diagnostic pop
#            endif

    if(!SymInitialize || !StackWalk64 || !SymFunctionTableAccess64 || !SymGetModuleBase64 || !RtlCaptureContext)
        return false;
#        endif

    const HANDLE process = GetCurrentProcess();
    if(!SymInitialize(process, nullptr, true))
        return false;

    if(!ctx)
    {
        context.ContextFlags = CONTEXT_FULL;
        RtlCaptureContext(&context);
        ctx = &context;
    }

    STACKFRAME64 frame;
    memset(&frame, 0, sizeof(frame));

#        ifdef _WIN64
    frame.AddrPC.Offset = ctx->Rip;
    frame.AddrStack.Offset = ctx->Rsp;
    frame.AddrFrame.Offset = ctx->Rbp;
#        else
    frame.AddrPC.Offset = ctx->Eip;
    frame.AddrStack.Offset = ctx->Esp;
    frame.AddrFrame.Offset = ctx->Ebp;
#        endif

    frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrStack.Mode = AddrModeFlat;
    frame.AddrFrame.Mode = AddrModeFlat;

    HANDLE thread = GetCurrentThread();
#        ifdef _WIN64
    DWORD machineType = IMAGE_FILE_MACHINE_AMD64;
#        else
    DWORD machineType = IMAGE_FILE_MACHINE_I386;
#        endif

    for(unsigned i = 0; i < stacktrace.size(); i++)
    {
        if(!StackWalk64(machineType, process, thread, &frame, ctx, nullptr, SymFunctionTableAccess64,
                        SymGetModuleBase64, nullptr))
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
#    else  // HAVE_DBGHELP_H
bool captureBacktrace(std::vector<void*>&, void* = nullptr) noexcept
{
    return false;
}
#    endif // HAVE_DBGHELP_H

#else
void captureBacktrace(std::vector<void*>& stacktrace) noexcept
{
    unsigned num_frames = backtrace(&stacktrace[0], stacktrace.size());
    stacktrace.resize(num_frames);
}
#endif
} // namespace

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

    SendString(rttr::version::GetVersion());
    SendString(rttr::version::GetRevision());

    SendUnsigned(GAMECLIENT.GetGFNumber());
}

DebugInfo::~DebugInfo()
{
    SendString("DONE");
    sock.Close();
}

std::vector<void*> DebugInfo::GetStackTrace(void* ctx) noexcept
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

bool DebugInfo::Send(const void* buffer, size_t length)
{
    const auto* ptr = (const char*)buffer;

    while(length > 0)
    {
        int res = sock.Send(ptr, length);

        if(res >= 0)
        {
            size_t numSend = res;
            if(numSend >= length)
                break;
            ptr += numSend;
            length -= numSend;
        } else
        {
            boost::nowide::cerr << (boost::format("failed to send: %1% left\n") % length).str();
            return false;
        }
    }

    return true;
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

bool DebugInfo::SendString(const char* str, size_t len)
{
    if(len == 0)
        len = strlen(str) + 1;

    if(!SendUnsigned(len))
        return (false);

    return (Send(str, len));
}

bool DebugInfo::SendString(const std::string& str)
{
    return SendString(str.c_str(), str.length() + 1); // +1 to include nullptr terminator
}

bool DebugInfo::SendStackTrace(const std::vector<void*>& stacktrace)
{
    if(stacktrace.empty())
        return false;

    LOG.write("Will now send %1% stack frames\n") % stacktrace.size();

    if(!SendString("StackTrace"))
        return false;

    using littleVoid_t =
      std::conditional_t<sizeof(void*) == 4, boost::endian::little_int32_t, boost::endian::little_int64_t>;
    static_assert(sizeof(void*) <= sizeof(littleVoid_t), "Size of pointer did not fit!");
    std::vector<littleVoid_t> endStacktrace;
    endStacktrace.reserve(stacktrace.size());
    for(void* ptr : stacktrace)
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
        const auto replayPath = rpl->GetPath();
        rpl->Close();

        BinaryFile f;
        if(f.Open(replayPath, OpenFileMode::OFM_READ))
        {
            if(!SendString("Replay"))
                return false;
            if(SendFile(f))
                return true;
        }
        // Empty replay
        SendUnsigned(0);
        return false;
    } else
    {
        LOG.write("-> Already in replay mode, do not send replay\n");
    }

    return true;
}

bool DebugInfo::SendAsyncLog(const boost::filesystem::path& asyncLogFilepath)
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

    auto fileData = std::unique_ptr<char[]>(new char[fileSize]);
    unsigned compressed_len = fileSize * 2 + 600;
    auto compressed = std::unique_ptr<char[]>(new char[compressed_len]);

    file.Seek(0, SEEK_SET);
    file.ReadRawData(fileData.get(), fileSize);

    LOG.write("- Compressing...\n");
    if(BZ2_bzBuffToBuffCompress(compressed.get(), &compressed_len, fileData.get(), fileSize, 9, 0, 250) == BZ_OK)
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
