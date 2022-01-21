// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Debug.h"
#include "GameManager.h"
#include "QuickStartGame.h"
#include "RTTR_AssertError.h"
#include "RTTR_Version.h"
#include "RttrConfig.h"
#include "Settings.h"
#include "SignalHandler.h"
#include "WindowManager.h"
#include "commands.h"
#include "drivers/AudioDriverWrapper.h"
#include "drivers/VideoDriverWrapper.h"
#include "files.h"
#include "helpers/format.hpp"
#include "mygettext/mygettext.h"
#include "ogl/glAllocator.h"
#include "libsiedler2/libsiedler2.h"
#include "s25util/LocaleHelper.h"
#include "s25util/Log.h"
#include "s25util/StringConversion.h"
#include "s25util/System.h"
#include "s25util/error.h"
#include <boost/filesystem.hpp>
#include <boost/nowide/args.hpp>
#include <boost/nowide/iostream.hpp>
#include <boost/program_options.hpp>
#include <array>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <limits>
#include <vector>
#if RTTR_HAS_VLD
#    include <vld.h>
#endif

#ifdef _WIN32
#    include <boost/nowide/convert.hpp>
#    include <windows.h>
#    include <s25clientResources.h>
#    if defined _DEBUG && defined _MSC_VER && defined RTTR_HWETRANS
#        include <eh.h>
#    endif
#endif
#ifndef _MSC_VER
#    include <csignal>
#endif

namespace bfs = boost::filesystem;
namespace bnw = boost::nowide;
namespace po = boost::program_options;

/// Calls a setGlobalInstance function for the (previously) singleton type T
/// on construction and destruction
template<class T>
class SetGlobalInstanceWrapper : public T
{
    using Setter = void (*)(T*);
    Setter setter_;

public:
    template<typename... Args>
    SetGlobalInstanceWrapper(Setter setter, Args&&... args) : T(std::forward<Args>(args)...), setter_(setter)
    {
        setter_(static_cast<T*>(this));
    }
    ~SetGlobalInstanceWrapper() noexcept { setter_(nullptr); }
};

// Throw this to terminate gracefully
struct RttrExitException : std::exception
{
    int code;
    RttrExitException(int code) : code(code) {}
};

namespace {
void WaitForEnter()
{
    static bool waited = false;
    if(waited)
        return;
    waited = true;
    bnw::cout << "\n\nPress ENTER to close this window . . ." << std::endl;
    bnw::cin.clear();
    bnw::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    bnw::cin.get();
}

std::string GetProgramDescription()
{
    std::stringstream s;
    s << rttr::version::GetTitle() << " v" << rttr::version::GetVersion() << "-" << rttr::version::GetRevision() << "\n"
      << "Compiled with " << System::getCompilerName() << " for " << System::getOSName();
    return s.str();
}

///////////////////////////////////////////////////////////////////////////////

#if defined _WIN32 && defined _DEBUG && defined _MSC_VER && defined RTTR_HWETRANS
/**
 *  Exception-Handler, wird bei einer C-Exception ausgeführt, falls dies mit RTTR_HWETRANS und
 *  im Projekt mit den Compilerflags (/EHa) aktiviert ist.
 *
 *  @param[in] exception_type    Typ der Exception (siehe GetExceptionCode)
 *  @param[in] exception_pointer Genaue Beschreibung der Exception (siehe GetExceptionInformation)
 */
void CExceptionHandler(unsigned exception_type, _EXCEPTION_POINTERS* exception_pointer)
{
    fatal_error("C-Exception caught\n");
}
#endif // _WIN32 && _DEBUG && RTTR_HWETRANS

bool askForDebugData()
{
#ifdef _WIN32
    std::string msg = gettext_noop("RttR crashed. Would you like to send debug information to RttR to help "
                                   "us avoiding this crash in the future? Thank you very much!");
    std::string errorTxt = gettext_noop("Error");
    try
    {
        msg = _(msg);
        errorTxt = _(errorTxt);
    } catch(...)
    {}
    std::wstring title = boost::nowide::widen(_(errorTxt));
    std::wstring text = boost::nowide::widen(_(msg));
    return (MessageBoxW(nullptr, text.c_str(), title.c_str(), MB_YESNO | MB_ICONERROR | MB_TASKMODAL | MB_SETFOREGROUND)
            == IDYES);
#else
    return false;
#endif
}
bool shouldSendDebugData()
{
    return (SETTINGS.global.submit_debug_data == 1) || askForDebugData();
}

void showCrashMessage()
{
    std::string text = gettext_noop("RttR crashed. Please restart the application!");
    std::string errorTxt = gettext_noop("Error");
    try
    {
        text = _(text);
        errorTxt = _(errorTxt);
    } catch(...)
    {}
#ifdef _WIN32
    MessageBoxW(nullptr, boost::nowide::widen(text).c_str(), boost::nowide::widen(errorTxt).c_str(),
                MB_OK | MB_ICONERROR | MB_TASKMODAL | MB_SETFOREGROUND);
#else
    RTTR_UNUSED(errorTxt);
    bnw::cerr << text << std::endl;
#endif
}

[[noreturn]] void terminateProgramm()
{
#ifdef _DEBUG
    abort();
#else
    throw RttrExitException(1);
#endif
}

void handleException(void* pCtx = nullptr) noexcept
{
    std::vector<void*> stacktrace = DebugInfo::GetStackTrace(pCtx);
    try
    {
        LogTarget target = (LOG.getFileWriter()) ? LogTarget::FileAndStderr : LogTarget::Stderr;
        LOG.write("RttR crashed. Backtrace:\n", target);
        // Don't let locale mess up addresses
        s25util::ClassicImbuedStream<std::stringstream> ss;
        for(void* p : stacktrace)
            ss << p << "\n";
        LOG.write("%1%", target) % ss.str();
        if(shouldSendDebugData())
        {
            DebugInfo di;
            di.SendReplay();
            di.SendStackTrace(stacktrace);
        }
    } catch(...)
    { //-V565
      // Could not write stacktrace or send debug data. Ignore errors
    }

    showCrashMessage();
}

#ifdef _MSC_VER
LONG WINAPI ExceptionHandler(LPEXCEPTION_POINTERS info)
{
    handleException(info->ContextRecord);
    terminateProgramm();
    return EXCEPTION_EXECUTE_HANDLER;
}
#else
[[noreturn]] void ExceptionHandler(int /*sig*/)
{
    handleException();
    terminateProgramm();
}
#endif

void InstallSignalHandlers()
{
#ifdef _WIN32
    SetConsoleCtrlHandler(ConsoleSignalHandler, TRUE);
#else
    struct sigaction sa;
    sa.sa_handler = ConsoleSignalHandler;
    sa.sa_flags = 0; // SA_RESTART would not allow to interrupt connect call;
    sigemptyset(&sa.sa_mask);

    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGPIPE, &sa, nullptr);
    sigaction(SIGALRM, &sa, nullptr);
#endif

#ifdef _MSC_VER
    SetUnhandledExceptionFilter(ExceptionHandler);
#    ifdef _DEBUG
#        ifdef RTTR_HWETRANS
    _set_se_translator(CExceptionHandler);
#        endif // RTTR_HWETRANS
#        ifdef RTTR_CRTDBG
    // Enable Memory-Leak-Detection
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF /*| _CRTDBG_CHECK_EVERY_1024_DF*/);
#        endif //  RTTR_CRTDBG
#    endif     // _DEBUG

#else
    signal(SIGSEGV, ExceptionHandler);
#endif // _MSC_VER
}

void UninstallSignalHandlers()
{
#ifdef _WIN32
    SetConsoleCtrlHandler(ConsoleSignalHandler, FALSE);
#else
    struct sigaction sa;
    sa.sa_handler = SIG_DFL;
    sa.sa_flags = 0; // SA_RESTART would not allow to interrupt connect call;
    sigemptyset(&sa.sa_mask);

    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGPIPE, &sa, nullptr);
    sigaction(SIGALRM, &sa, nullptr);
#endif // _WIN32

#ifdef _MSC_VER
    SetUnhandledExceptionFilter(nullptr);
#else
    signal(SIGSEGV, SIG_DFL);
#endif
}

/**
 *  Exit-Handler, wird bei @p exit ausgeführt.
 */
void ExitHandler()
{
    Socket::Shutdown();
    UninstallSignalHandlers();

#ifdef _DEBUG
    WaitForEnter();
#endif
}

void SetAppSymbol()
{
#ifdef _WIN32
    // set console window icon
    SendMessage(GetConsoleWindow(), WM_SETICON, ICON_BIG,
                (LPARAM)LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_SYMBOL)));
    SendMessage(GetConsoleWindow(), WM_SETICON, ICON_SMALL,
                (LPARAM)LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_SYMBOL)));
#endif // _WIN32
}

bool MigrateFilesAndDirectories()
{
    struct MigrationEntry
    {
        std::string oldName, newName;
        bool isFolder;
    };
    const std::string sharedLibext =
#ifdef _WIN32
      "dll";
#elif defined(__APPLE__)
      "dylib";
#else
      "so";
#endif

    // Mapping from old files or directories to new ones
    // Handled in order so multiple renamings are possible
    // Empty newName = Delete
    const std::vector<MigrationEntry> migrations = {
#ifdef _WIN32
      {"~/Siedler II.5 RttR", s25::folders::config, true},
#elif defined(__APPLE__)
      {"~/.s25rttr", s25::folders::config, true},
#endif
      {std::string(s25::folders::assetsUserOverrides).append("/SOUND.LST"), "", false},
      {std::string(s25::folders::driver).append("/video/libvideoSDL.").append(sharedLibext), "", false},
    };

    try
    {
        for(const MigrationEntry& entry : migrations)
        {
            const bfs::path oldName = RTTRCONFIG.ExpandPath(entry.oldName);
            if(!exists(oldName))
                continue;
            if(entry.isFolder && !is_directory(oldName))
                throw std::runtime_error(helpers::format("Expected folder but %1% is not a folder", oldName));
            else if(!entry.isFolder && !is_regular_file(oldName))
                throw std::runtime_error(helpers::format("Expected file but %1% is not a file", oldName));
            if(entry.newName.empty())
            {
                LOG.write("Removing %1% which is no longer needed\n", LogTarget::Stdout) % oldName;
                boost::system::error_code ec;
                if(!remove(oldName, ec))
                    throw std::runtime_error(helpers::format("Failed to delete %1%: %2%", oldName, ec.message()));
            } else
            {
                const bfs::path newName = RTTRCONFIG.ExpandPath(entry.newName);
                if(exists(newName))
                {
                    throw std::runtime_error(helpers::format(
                      "Old and new %1% found. Please delete the one you don't want to keep!\nOld: %2%\nNew: %3%",
                      is_directory(oldName) ? "directory" : "file", oldName, newName));
                }
                LOG.write("Filepath of %1% has changed to %2%. Renaming...\n", LogTarget::Stdout) % oldName % newName;
                boost::system::error_code ec;
                rename(oldName, newName, ec);
                if(ec)
                {
                    throw std::runtime_error(helpers::format("Renaming %1% to %2% failed\nError: %3%\nRename it "
                                                             "yourself and/or make sure the directory is writable!",
                                                             oldName, newName, ec.message()));
                }
            }
        }
    } catch(const std::exception& e)
    {
        LOG.write("ERROR: Migration of folders and files to new locations failed: %1%\n", LogTarget::Stderr) % e.what();
        return false;
    }
    return true;
}

bool InitDirectories()
{
    // Note: Do not use logger yet. Filepath may not exist
    const auto curPath = bfs::current_path();
    LOG.write("Starting in %1%\n", LogTarget::Stdout) % curPath;

    if(!MigrateFilesAndDirectories())
        return false;

    // Create all required/useful folders
    const std::array<std::string, 10> dirs = {
      {s25::folders::config, s25::folders::logs, s25::folders::mapsOwn, s25::folders::mapsPlayed, s25::folders::replays,
       s25::folders::save, s25::folders::assetsUserOverrides, s25::folders::screenshots, s25::folders::playlists}};

    for(const std::string& rawDir : dirs)
    {
        const bfs::path dir = RTTRCONFIG.ExpandPath(rawDir);
        boost::system::error_code ec;
        bfs::create_directories(dir, ec);
        if(ec != boost::system::errc::success)
        {
            // This writes to the log. If the log folder or file could not be created, an exception is thrown
            // Make sure we catch that
            try
            {
                s25util::error(std::string("Directory ") + dir.string() + " could not be created.");
                s25util::error("Failed to start the game");
            } catch(const std::runtime_error& error)
            {
                LOG.write("Additional error: %1%\n", LogTarget::Stderr) % error.what();
            }
            return false;
        }
    }
    LOG.write("Directory for user data (config etc.): %1%\n", LogTarget::Stdout)
      % RTTRCONFIG.ExpandPath(s25::folders::config);

    // Write this to file too, after folders are created
    LOG.setLogFilepath(RTTRCONFIG.ExpandPath(s25::folders::logs));
    try
    {
        LOG.open();
        LOG.write("%1%\n\n", LogTarget::File) % GetProgramDescription();
        LOG.write("Starting in %1%\n", LogTarget::File) % curPath;
    } catch(const std::exception& e)
    {
        LOG.write("Error initializing log: %1%\nSystem reports: %2%\n", LogTarget::Stderr) % e.what()
          % LOG.getLastError();
        return false;
    }
    return true;
}

bool InitGame(GameManager& gameManager)
{
    libsiedler2::setAllocator(new GlAllocator());

    // Socketzeug initialisieren
    if(!Socket::Initialize())
    {
        s25util::error("Could not init sockets!");
        s25util::error("Failed to start the game");
        return false;
    }

    // Spiel starten
    if(!gameManager.Start())
    {
        s25util::error("Failed to start the game");
        return false;
    }
    return true;
}

int RunProgram(po::variables_map& options)
{
    LOG.write("%1%\n\n", LogTarget::Stdout) % GetProgramDescription();
    if(!LocaleHelper::init())
        return 1;
    if(!RTTRCONFIG.Init())
        return 1;
    SetAppSymbol();
    InstallSignalHandlers();
    // Exit-Handler initialisieren
    atexit(&ExitHandler);
    if(!InitDirectories())
        return 1;

    // Zufallsgenerator initialisieren (Achtung: nur für Animations-Offsets interessant, für alles andere
    // (spielentscheidende) wird unser Generator verwendet)
    srand(static_cast<unsigned>(std::time(nullptr)));

    if(options.count("convert-sounds"))
    {
        try
        {
            convertAndSaveSounds(RTTRCONFIG, RTTRCONFIG.ExpandPath("<RTTR_USERDATA>/convertedSoundeffects"));
            return 0;
        } catch(const std::runtime_error& e)
        {
            bnw::cerr << "Error: " << e.what() << "\n";
            return 1;
        }
    }

    SetGlobalInstanceWrapper<GameManager> gameManager(setGlobalGameManager, LOG, SETTINGS, VIDEODRIVER, AUDIODRIVER,
                                                      WINDOWMANAGER);
    try
    {
        if(!InitGame(gameManager))
            return 2;

        if(options.count("map") && !QuickStartGame(options["map"].as<std::string>()))
            return 1;

        // Hauptschleife

        while(gameManager.Run())
        {
#ifndef _WIN32
            killme = false;
#endif // !_WIN32
        }

        // Spiel beenden
        gameManager.Stop();
        libsiedler2::setAllocator(nullptr);
    } catch(RTTR_AssertError& error)
    {
        // Write to log file, but don't throw any errors if this fails too
        try
        {
            LOG.writeToFile(error.what());
        } catch(...)
        { //-V565
        }
        return 42;
    }
    return 0;
}
} // namespace

/**
 *  Hauptfunktion von Siedler II.5 Return to the Roots
 *
 *  @param[in] argc Anzahl übergebener Argumente
 *  @param[in] argv Array der übergebenen Argumente
 *
 *  @return Exit Status, 0 bei Erfolg, > 0 bei Fehler
 */
// Exceptions handled by registred global handlers
// NOLINTNEXTLINE(bugprone-exception-escape)
int main(int argc, char** argv)
{
    bnw::args _(argc, argv);

    po::options_description desc("Allowed options");
    // clang-format off
    desc.add_options()
        ("help,h", "Show help")
        ("map,m", po::value<std::string>(),"Map to load")
        ("version", "Show version information and exit")
        ("convert-sounds", "Convert sounds and exit")
        ;
    // clang-format on
    po::positional_options_description positionalOptions;
    positionalOptions.add("map", 1);

    po::variables_map options;
    try
    {
        po::store(po::command_line_parser(argc, argv).options(desc).positional(positionalOptions).run(), options);
        // Catch the generic stdlib exception as hidden visibility messes up boost typeinfo on OSX
    } catch(const std::exception& e)
    {
        bnw::cerr << "Error: " << e.what() << "\n\n";
        bnw::cerr << desc << "\n";
        return 1;
    }
    po::notify(options);

    if(options.count("help"))
    {
        bnw::cout << desc << "\n";
        return 0;
    }
    if(options.count("version"))
    {
        bnw::cout << GetProgramDescription() << std::endl;
        return 0;
    }

    int result;
    try
    {
        result = RunProgram(options);
    } catch(const RttrExitException& e)
    {
        result = e.code;
    } catch(const std::exception& e)
    {
        bnw::cerr << "An exception occurred: " << e.what() << "\n\n";
        handleException(nullptr);
        result = 1;
    } catch(...)
    {
        bnw::cerr << "An unknown exception occurred\n";
        handleException(nullptr);
        result = 1;
    }
    if(result)
        WaitForEnter();

    return result;
}
