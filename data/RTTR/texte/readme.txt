                              RETURN TO THE ROOTS
--------------------------------------------------------------------------------

A. Reference Note
B. Installation
  1. Windows
  2. Linux
  3. Mac OSX
  4. Settings directory
C. Game
  1. Create a game
  2. Multiplayer game
  3. Replays
  4. Options
D. Crash and bugs
E. Summary: Updates and changelog

--------------------------------------------------------------------------------

A. Reference Note

  The game requires an OpenGL2.0-compatible graphics card with at
  least 64 MB graphics-memory. A CPU with 800Mhz will suffice.

  Further on you will need an installed version of "The Settlers 2
  Gold-Edition" or the original version + Mission-CD.

--------------------------------------------------------------------------------

B. Installation

  1. Windows

    Windows 7 and 10 are supported. Windows Vista or older might work but
    are not officially supported.

    To play the game search for the "DATA" und "GFX" directories in the
    original S2 Gold (or S2 + Mission CD) game (from a valid installation
    or directly from the CD) and copy them into the nightly folder
    (where you find the file "Put your S2-Files in here").

    Alternatively you can make NTFS symbolic links as Administrator:
    mklink /D DATA "C:\S2\DATA"
    mklink /D GFX "C:\S2\GFX"

    To play RTTR, you simply have to run s25client.exe.

    (In Nightly's you can update with RTTR.BAT)

    Note: RTTR does not change any files from the original game.
    It acts as a game-mod.

   -----------------------------------------------------------------------------

  2. Linux

    You will need the following packages:
    (Use your package-manager or do it manually)

    libsdl2 libsdl-mixer2 gettext

    Make sure that DirectRendering works:

    glxinfo | grep direct

    When the output "direct rendering: Yes" appears, you are ok.
    If not, your performance will be lousy. Please check your
    graphics acceleration.

    Installation:

    extract RTTR archive e.g. to /opt/s25rttr

    mkdir -p /opt/s25rttr
    cd /opt/s25rttr
    tar -jxvf s25rttr_*.tar.bz2

    Now all you have to do is copying your original files to:
    /opt/s25rttr/share/s25rttr/S2

    Alternatively you can make a symbolic link to your original folder.

    /opt/s25rttr/bin/rttr.sh will launch the game.

    (Nightly versions automatically update when you start rttr.sh.
     Use "sh rttr.sh noupdate" to keep the actual version.)

  ------------------------------------------------------------------------------

  3. Mac OSX

    Start the game directly from the downloaded app bundle.

  ------------------------------------------------------------------------------

  4. Settings directory

    The settings directory is found in:

    Windows: <UserDir>/My Games/Return To The Roots
    Linux:   ~/.s25rttr
    Mac OSX: ~/Library/Application Support/Return To The Roots

    This folder is created on first start if it does not exist.
    All logs, settings, saves and replays are in subfolders to that.

--------------------------------------------------------------------------------

C. Game

  1. Create a game

    There is a Singleplayer- and a Multiplayer-Modus which can be found in
    the main menu. You can also play alone in the Multiplayer-Modus:

    1. Choose "Multiplayer"
    2. "Direct IP"
    3. "Create game"
    4. In the next window choose the name of the game, no password needed.
    5. In the map screen you can choose from one of the categories on the
       left side (similar to the original) and then a map or you can load
       a game ("Load Game" button)
    6. After choosing your map, click on "Continue" which leads you
       to the host-menu.
       The top part of this menu shows you the players and their
       configuration.
       Notice: To start the game you have to fill all player slots with
       computer players or human players.
       Alternatively you can close the slots by clicking on them.

    For a quick Singleplayer-Game click "Unlimited Play" in the
    Singleplayer-Menu and follow the steps 5 & 6 above.

  2. Multiplayer game

    a) Direct game
       The proceeding is equivalent to point 1. Other human players
       have to choose "Direct IP", "Join game" and then enter the IP or
       hostname. "Connect" will lead them into the host menu.

    b) Lobby access
       The lobby is used for creating your own games or for joining
       games of other players. A chat function is also integrated.

       You need to create a forum account on http://www.siedler25.org
       and use that to login into the game lobby.

    c) LAN games
       The LAN area is similar to the lobby but shows only maps created
       in the current LAN and does not require a login to join. You can
       also use Hamachi or a similar program to create virtual LANs over
       the internet.

    RTTR uses port 3665 (TCP). This port must be open to create a
    game and makes other people able to join your game. If you use a
    router, you have to  forward this port to your workstation.
    Look for "Virtual server" or "Port forwarding" in your router's
    menu. If you use a personal firewall on your workstation, TCP port
    3665 has to be allowed there too. This method works on internet
    and LAN.
    In LAN mode ports 3666 and 3667 (UDP) are used to find games.

    It's not necessary to have the game's map on every machine. It
    will automatically be transfered from the host to every player.

    A coloured snail symbol in the top right of the display indicates
    a bad connection to the player with same colour.

    The game will be stopped when the host leaves and cannot be
    continued.

    To pause the game, press "P" (Host only) and again to unpause.
    To chat with other players press "Enter".

  3. Replays

    Replays record every action in a game. You can watch them by
    choosing "Single player" - "Play replay".

    The keys [+] and [-] will raise or lower the replay-speed. "J" will make
    you able to jump to a specific gameframe.

    All replays can be found in the subfolder Replays in the game settings dir.

  4. Save

    Saving a game works just like in the original game. It also works at any
    position in a replay.
    To load a multiplayer savegame choose "Load game" in the map menu. 
    For singleplayer there is a button in the singleplayer menu.
    It is sufficient if only one player has the savegame file.


  5. Options

    The options in the main menu are self-explanatory.

--------------------------------------------------------------------------------

D. Crash and bugs

  There may still be crashes, asynchronities, save-bugs and other bugs
  in the game. We are currently working on this very hard.

  We can reproduce bugs better if there is a replay available. You
  can help us by sending your replays (subfolder Replays in the settings 
  folder) with a describtion of the bug. Be sure you used the latest 
  nightly-build version.

  If there is an async-bug you will find additional logs in the folder "Logs"
  in the settings folder. Please send us the logs of EVERY player and all
  of the replays accordingly.
  It is important that you tell us which version of RTTR you used.
  For example: "20170630-49c9bb8(nightly) or 0.8.2"

  Also make sure you did not make the async on purpose by cheating
  or using different versions.

  If you find bugs, please report them on:

      https://github.com/Return-To-The-Roots/s25client

  By using Github we can keep the bug reports organized. Also it makes it
  easier for you to see if your bug is already fixed. It also allows further
  communication between you and us, e.g. when we need more information
  from you.

  Alternatively you can post the bug in the forum or visit us on Discord:
  https://discord.gg/kyTQsSx
  You can also join the IRC-channel and Discord by visiting our homepage.

  Thanks a lot

  Settlers Freaks
  July 7th 2017

--------------------------------------------------------------------------------

E. Summary: Updates and changelog

  * 0.9.4 - 06.01.2022
  ------------------------------------------------------------------------------
  - Various fixes for bugs leading to unloadable savegames and crashes
  - Fix drawing issues related to high terrain
  - Pressing ESC now does no longer discard pending changes of setting windows
  - Fix uncloseable action window
  - On game start reopen windows opened in last game and restorr their positions
  - Fix faulty version handling (visual issues and unable to join other players)

  * 0.9.1 - 24.07.2021
  ------------------------------------------------------------------------------
  - Fullscreen mode on all drivers and OSs
  - Random map generator
  - Allow special chars in user name
  - Fix some bugs, crashes and asyncs
  - Map editor included
  - Improved performance

  * 0.8.2 - 22.08.2017
  ------------------------------------------------------------------------------
  - Many async fixes
  - Usage of UTF8 to support more languages
  - Many addons added
  - Support for all S2 terrain types
  - Zoom function
  - Lua scripting
  - LAN Lobby
  - Code quality improvements

  * 0.8.1
  ------------------------------------------------------------------------------
  - Bug fixes
  - AI Improvements
  - Complete Seafaring

  * 0.8.0
  ------------------------------------------------------------------------------
  - Greatly improved AI player
  - A lot of bugs fixed
  - Speed improvements
  - Observation windows

 * 0.7.2 - 17.01.2011 *
  ------------------------------------------------------------------------------
  - Critical Bugfixes

  * 0.7 - Seventh version - 24.12.2010 *
  ------------------------------------------------------------------------------
  - OpenSource!
  - Translation: Dutch
  - Translation: Russian (Font is missing)
  - Translation: Czech
  - Translation: Estonian
  - Translation: Italian
  - Translation: Norwegian
  - Translation: Polish
  - Translation: Slovenian
  - Translation: Slovak
  - Statistics
  - Postoffice
  - Diplomacy - unfinished
  - First AI (jh)
  - Seafaring (not finished yet)
  - Addon Menu

  * 0.6 - Sixth version - 25.01.2009 *
  ------------------------------------------------------------------------------
  - Fixed a lot of bugs
  - Fog of War (with Teamview-option)
  - Watchout tower added
  - Minimap
  - Minimap in hostgame-menu
  - Planer
  - Balancing
  - Helper animations added
  - Multilanguage-support
  - Translation: Spanish
  - Translation: Hungarian
  - Translation: Swedish
  - Translation: Finnish (not completed)
  - Translation: French (not completed)
  - Victory-messages
  - HotKeys + Readme
  - Hostmenu option: Demolition prohibition
  - Building Info
  - Some building animations added

  * 0.5 - Fifth version - 27.01.2008 *
  ------------------------------------------------------------------------------
  - Fixed a lot of bugs
  - Donkeyroads, Donkey breeder
  - Boatroads with boats and shipyard (shipyard only builds boats)
  - Mountain road
  - Catapults
  - Soldier-behaviour changed (queue)
  - Burn down the depot makes all people escape somewhere
  - Async-log
  - Changed some action windows and tooltips
  - Sound on/off in the game

  * 0.4 - Fourth version - 09.10.2007 *
  ------------------------------------------------------------------------------
  - Save and load games, Autosave
  - Fixed very bad bugs

  * 0.3 - Third version Fix01 -  13.09.2007 *
  ------------------------------------------------------------------------------
  - critical crash-bug fixed

  * 0.3 - Third version - 12.09.2007 *
  ------------------------------------------------------------------------------
  - fixed crash-bugs
  - Settlers are in queue when the place in front is occupied
  - Settings-file in ~/.s25rttr (Linux)
  - New format for settings (GER-File)
  - Build-System is now cmake (Linux)
  - CIA-Bot for irc-channel
  - Lobby
  - Preperation for load and save

  * 0.2 - Second version - 15.07.2007 *
  ------------------------------------------------------------------------------
  - Fixed a lot of bugs
  - New screen resolutions
  - Kicking asynchron players now
  - Inventory-window
  - "Take out/stop storage" works now
  - "Ready"-Button in host-menu
  - Distribution of goods works
  - Adornment objects added (Ruins,...)
  - Jump to every house works now
  - Some adornment object are demolished when setting a road
  - Status of houses now visible (C, S)
  - Demolish-interrogation
  - RoadWindow closes now, when clicking somewhere else

  * 0.1 - First Release - 01.07.2007 *
  ------------------------------------------------------------------------------
  - Everything! ;-)

--------------------------------------------------------------------------------
https://www.siedler25.org                Copyright (C) 2005-2022 Settlers Freaks
--------------------------------------------------------------------------------
