2010.01.17                    RETURN TO THE ROOTS                          0.7.2
--------------------------------------------------------------------------------

A. Reference Note
B. Installation
   1. Windows
   2. Linux
C. Game
   1. Create a game
   2. Multiplayer game
   3. Replays
   4. Options
D. Crash and bugs
E. Summary: Updates and changelog

--------------------------------------------------------------------------------

A. Reference Note

   The game requires an OpenGl-compatible graphics card with at
   least 64 MB graphics-memory. A CPU with 800Mhz will suffice.

   Further on you will need an installed version of "The Settlers 2 
   Gold-Edition" or the original version + Mission-Cd. 

--------------------------------------------------------------------------------

B. Installation

   1. Windows

      We don't support Windows 95/98/ME/Vista/7 officially. Vista and 7
     seem to work.

      a) Zip-archiv's
         To play the game search for the "DATA" und "GFX" directories
         in the original S2 Gold (or S2 + Mission CD) game. Copy them
         into the nightly folder
         (where you find the file "Put your S2-Files in here").
         
         Alternatively you can make NTFS symbolic links as Administrator:
         mklink /D DATA "C:\S2\DATA"
         mklink /D GFX "C:\S2\GFX"

         If you can't install S2 via the original installer (i.e on 64bit), 
         you can simply copy DATA and GFX from the S2-Directory of your CD.
         
         To play RTTR, you simply have to run s25client.exe.
         
         (In Nightly's you can update with: RTTR.BAT)

      b) Setup
         Run the setup-file and choose the "The Settlers 2 Gold-Edition" 
         folder as the destination you want to install RTTR into.
         Of course you can alternatively copy the Gold-Edition files
         to the installation-folder of RTTR. Afterwards the game can
         be launched by clicking the shortcut on the desktop.

      Note: RTTR does not change any files from the original game. 
      It acts like a game-mod. 
         
   --------------------------------------------------------------------------
   
   2. Linux

      You will need the following packages:
      (Use your package-manager or do it manually)

      libsndfile libsamplerate libsdl1.2 libsdl-mixer1.2 gettext

      Make sure that DirectRendering works:

      glxinfo | grep direct

      When the output "direct rendering: Yes" appears, you are ok. 
      If not, your performance will be lousy. Please check your
      graphics acceleration.

      Installation:

      extract RTTR archive e.g. to /opt/s25rttr

      mkdir -p /opt/s25rttr
      cd /opt/s25rttr
      tar -jxvf s25rttr_???.tar.bz2

      Now all you have to do is copying your original files to:
      /opt/s25rttr/share/s25rttr/S2
      
      Alternatively you can make a symbolic link to your original folder.

      /opt/s25rttr/bin/rttr.sh will launch the game.
      
      (Nightly versions automatically update when you start rttr.sh.
      Use "sh rttr.sh noupdate" to keep the actual version.)

--------------------------------------------------------------------------------

C. Game

   1. Create a game
   
      A singleplayer mode is not available yet, but you can play on your 
      own by doing this:      

      1. Choose "Multiplayer"
      2. "Direct IP"
      3. "Create game"
      4. Choose the name of the game, no password needed. 
      5. Choose your map or load a game ("Load Game" button)
      6. After choosing your map, click on "Continue" which leads you
         to the host-menu.
         The top part of this menu shows you the players and their
         configuration. "Goal" and "Lock teams" do not have an effect
         until now.         
         Notice: To start the game you have to fill all player slots with 
         computer players or human players.
         Alternatively you can close the slots by clicking on them. 
         Note that the artificial intelligence does not work yet.
   
   2. Multiplayer game

      a) Direct game
         The proceeding is equivalent to point 1. Other human players
         have to choose "Direct IP", "Join game" and then enter the IP or
         hostname.
         "Connect" will lead them back into the host menu.

      b) Lobby access
         The lobby is used for creating your own games or for joining
         games of other players. A chat function is also integrated.

         Note that the lobby is still very buggy and it's possible that you
         cannot log in. A ranking system is planed.

         Data for the log in is used to identify players. Your e-mail
         address will not be published publicly. At this point we use it
         only to send your password in case it was lost.

      RTTR uses port 3665 (TCP). This port must be open to create a
      game and makes other people able to join your game. If you use a
      router, you have to  forward this port to your workstation.
      Look for "Virtual server" or "Port forwarding" in your router's
      menu. If you use a personal firewall on your workstation, TCP port
      3665 has to be allowed there too. This method works on internet
      and LAN.

      It's not necessary to have the game's map on every machine. It
      will automatically be transfered from the host to every
      joined player.

      A coloured snail symbol in the top right of the display indicates
      a bad connection to the player with same colour.
      To pause the game, press "P". Only the Host can do that.
      To chat with other players press "Enter".

   3. Replays

      Replays record every action in a game. You can watch them by
      choosing "Single player" - "Play replay".

      The keys [+] and [-] will raise or low the replay-speed. "J" will make
      you able to jump to a specific gameframe. 
      All replays can be found in the subfolder RTTR/REPLAYS.

      Possible key configuration can be found in the
      keyboardlayout-Readme.

   4. Save
      
      Saving a game works just like in the original game. To load you
      have to make a multiplayer game and choose "Load game" in the
      map menu.
      It is sufficient if only one player has the savegame file.


   5. Options

      The options in the main menu are self-explanatory.

--------------------------------------------------------------------------------

D. Crash and bugs

   There are still crashes, asynchronities, save-bugs and other bugs
   in the game. We are currently working on this very hard.

   We can reproduce bugs better if there is a replay available. You
   can help us by sending your replays (subfolder RTTR/REPLAYS)
   with a describtion of the bug. Be sure you used the latest
   nightly-build version.

   If there is an async-bug you will find additional logs in the folder
   RTTR/LOGS. Please send us the logs of EVERY player and for sure
   every replay too. 
   It is important that you tell us which version of RTTR you used.
   For example: "20080703-3721(nightly) or 0.6"   

   Also make sure you did not make the async on purpose by cheating
   or using different versions.

   If you find bugs, please report them on:

               http://bugs.siedler25.org/

   Launchpad increases the facility of inspection and makes it easy
   for you to see if your bug is already fixed.

   If you have further questions, please contact:
        
               bugs@siedler25.org

   Alternatively you can post the bug in the forum or visit us on IRC
   channel: irc.freenode.net:6667/#siedler2.5

   You can also join the irc-channel by visiting our homepage. 

   Thanks a lot

   Settlers Freaks
   February 15th 2010

--------------------------------------------------------------------------------

E. Summary: Updates and changelog

 * 0.7.2 - 17.01.2011 *
   --------------------------------------------------------------------------
   - Critical Bugfixes

 * 0.7 - Seventh version - 24.12.2010 *
   --------------------------------------------------------------------------
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
   --------------------------------------------------------------------------
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
   -------------------------------------------------------------------------
   - Fixed a lot of bugs
   - Donkeyraods, Donkey breeder
   - Boatroads with boats and shipyard (shipyard only builds boats)
   - Mountain road
   - Catapults
   - Soldier-behaviour changed (queue)
   - Burn down the depot makes all people escape somewhere
   - Async-log
   - Changed some action windows and tooltips
   - Sound on/off in the game

   * 0.4 - Fourth version - 09.10.2007 *
   -------------------------------------------------------------------------
   - Save and load games, Autosave
   - Fixed very bad bugs

   * 0.3 - Thrid version Fix01 -  13.09.2007 *
   -------------------------------------------------------------------------
   - critical crash-bug fixed

   * 0.3 - Third version - 12.09.2007 *
   -------------------------------------------------------------------------
   - fixed crash-bugs
   - Settlers are in queue when the place in front is occupied
   - Settings-file in ~/.s25rttr (Linux)
   - New format for settings (GER-File)
   - Build-System is now cmake (Linux)
   - CIA-Bot for irc-channel
   - Lobby
   - Preperation for load and save

   * 0.2 - Second version - 15.07.2007 *
   -------------------------------------------------------------------------
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
   -------------------------------------------------------------------------
   - Everything! ;-)

--------------------------------------------------------------------------------
http://www.siedler25.org                 Copyright (C) 2005-2011 Settlers Freaks
--------------------------------------------------------------------------------
