                              RETURN TO THE ROOTS
--------------------------------------------------------------------------------

A. Allgemeine Hinweise
B. Installation
  1. Windows
  2. Linux
  3. Mac OSX
  4. Settings-Ordner
C. Spiel
  1. Erstellen eines Spiels
  2. Multiplayer-Spiele
  3. Replays
  4. Optionen
D. Abstürze und Fehler
E. Übersicht: Updates und Änderungen

--------------------------------------------------------------------------------

A. Allgemeine Hinweise

  Das Spiel benötigt eine OpenGL2.0-fähige Grafikkarte mit mindestens
  64 MB Grafikspeicher. Ein Prozessor mit 800 MHz reicht aus.

  Weiterhin benötigt man eine installierte "Siedler 2 Gold-Edition" oder die
  Originalversion + Missions-CD.

--------------------------------------------------------------------------------

B. Installation

  1. Windows

    Windows 7 und 10 werden unterstüzt. Windows Vista und älter können
    funktionieren, werden aber nicht offiziell unterstützt.

    Zum Spielen sucht man die Verzeichnisse "DATA" und "GFX" im Original
    "Die Siedler 2 Gold" Spiel (oder S2 + Mission CD) und kopiert diese in das
    Nightly-Verzeichnis (in den Ordner wo sich auch die Datei
    "Put your S2-Files in here" befindet). Diese können sowohl von einer
    Installation als auch direkt von der CD kommen.

    Alternativ können symbolische Verknüpfungen als Admin erstellt werden:
    mklink /D DATA "C:\S2\DATA"
    mklink /D GFX "C:\S2\GFX"

    Darin kann man dann s25client.exe zum Spielen von Siedler II.5 RTTR
    ausführen.

    (In Nightlies kann man mit RTTR.BAT updaten.)

    Es werden KEINE originalen Spieldateien verändert. Man kann weiterhin
    das Original ohne Beeinträchtigung spielen.

  ------------------------------------------------------------------------------

  2. Linux

    Zunächst benötigt man folgende Pakete
    (ggf über Paketmanager/per Hand installieren):

    libsdl2 libsdl-mixer2 gettext

    Weiterhin sollte man sicherstellen, dass DirectRendering funktioniert:

    glxinfo | grep direct

    Wenn die Ausgabe "direct rendering: Yes" kommt, ist alles in Ordnung,
    ansonsten kann es sein, dass man eine miese Spielperformance hat. In
    dem Fall muss die Grafikbeschleunigung geprüft werden.

    Die eigentliche Installation:

    z.B. nach /opt/s25rttr entpacken:

    mkdir -p /opt/s25rttr
    cd /opt/s25rttr
    tar -jxvf s25rttr_*.tar.bz2

    Nun muss man entweder nur noch die original "Siedler 2"-Installation
    nach /opt/s25rttr/share/s25rttr/S2 kopieren, oder einen Symlink dorthin
    anlegen.

    Starten kann man das Ganze dann mit
    /opt/s25rttr/bin/rttr.sh

    (Nightly Versionen updaten sich selber. Wenn man rttr.sh mit der
    Option noupdate startet, wird kein Update durchgeführt.)

  ------------------------------------------------------------------------------

  3. Mac OSX

    Das Spiel kann direkt aus dem App-Bundle gestartet werden.

  ------------------------------------------------------------------------------

  4. Settings-Ordner

    Der Settings-Ordner befindet sich unter:

    Windows: <Benutzer-Ordner>/My Games/Return To The Roots
    Linux:   ~/.s25rttr
    Mac OSX: ~/Library/Application Support/Return To The Roots

    Er wird beim ersten Start erstellt, sofern er nicht existiert.
    Alle Logs, Einstellungen, Savegames und Replays befinden sich
    in den jeweiligen Unterordnern.

--------------------------------------------------------------------------------

C. Spiel

  1. Erstellen eines Spiels

    Es gibt sowohl einen Einzelspieler- als auch einen Mehrspieler-Modus.
    Die Einzelspieler-Modi sind unter dem entsprechenden Menüpunkt zu
    finden und weitestgehend selbsterklärend. Man kann auch alleine im
    Mehrspieler-Modus spielen:

    1. Im Hauptmenü auf Mehrspieler gehen
    2. Direkte IP
    3. Spiel erstellen
    4. Im folgenden Fenster einen Namen für das Spiel eingeben (Passwort
       kann weggelassen werden).
    5. In der Kartenauswahl kann oben eine Kategorie ausgewählt
       werden, ähnlich wie im Original.
       Alternativ kann unten über "Spiel laden" ein Spielstand geladen
       werden.
    6. Nach der Auswahl der Karte und einem Klick auf "Weiter" gelangt man
       ins Host-Menü, wo im oberen Teil die einzelnen Spieler samt ihrer
       Eigenschaften aufgelistet sind.
       Achtung: Um das Spiel starten zu können, müssen entweder alle Plätze
       mit menschlichen oder KI-Spielern besetzt werden, oder die Slots
       müssen geschlossen werden (mit Klick auf die Buttons unter
       "Spielername").

    Im Einzelspieler-Modus klickt man auf "Endlosspiel" und folgt dann
    den obigen Schritten ab Schritt 5.

  2. Multiplayer-Spiele

    a) Direktes Spielen
       Die Erstellung eines Multiplayerspiels erfolgt genauso wie bei 1. Die
       weiteren Spieler gehen bei "Direkte IP" unter "Spiel beitreten" und
       geben die IP oder den Hostnamen des Spielerstellers (Host) ein.

    b) Spielen über integrierte Lobby
       Über die eingebaute Lobby kann man vorhandene Spiele einsehen,
       diese eröffnen und auch beitreten. Weiterhin kann man sich hier
       mit anderen Spielern direkt unterhalten.

       Es wird ein Foren-Account von http://www.siedler25.org benötigt, dessen
       Login-Daten für den Login in die Spiel-Lobby verwendet werden.

    c) LAN Spiele
       Der LAN-Bereich ist ähnlich der Internet-Lobby, funktioniert aber über
       das lokale Netz und benötigt keinen Login. Es können auch Programme
       wie Hamachi verwendet werden, um virtuelle LANs über das Internet
       zu erstellen.

    Für das Spiel wird Port 3665 (TCP) genutzt. Dieser muss, falls man selbst
    das Spiel eröffnen möchte, u.a. bei Verwendung eines Routers, einer
    Firewall usw. freigegeben werden!
    Router nennen diese Einstellung "Virtual Server" oder einfach nur
    "Port Forwarding". Dort ist dann Port 3665 vom Typ TCP auf die interne IP
    des Spiel-PCs weiterzuleiten. Bei der Verwendung einer lokalen Firewall
    muss auch hier der Zugriff zugelassen werden.
    Im LAN Modus werden die Ports 3666 und 3667 (UDP) zum Finden von Spielen
    verwendet.

    Es ist nicht notwendig, dass jeder Spieler die Karte hat. Diese wird bei
    Spielbegin automatisch vom Host übertragen.

    Die Schneckensymbole im oberen Teil stehen für Lags der jeweiligen
    Spieler.

    Wenn der Host das Spiel verlässt können die übrigen Spieler das Spiel
    nicht weiter fortführen.

    Zum Pausieren/Fortführen dient die Taste "P" (nur vom Host möglich).
    Zum Chatten muss man "Enter" drücken, dann öffnet sich ein Chatfenster.

  3. Replays

    Replays sind Aufzeichnungen von gespielten Partien. Sie können unter
    "Einzelspieler", "Replay abspielen" angesehen werden.

    Mit den Tasten [+] und [-] kann die Geschwindigkeit erhöht bzw.
    verringert werden. Mit der Taste "J" ist es möglich, Abschnitte im
    Replay zu überspringen.

    Alle Replays sind im Replays Ordner des Settings-Ordner zu finden.

  4. Speichern

    Speichern ist im Spielmenü an der "originalen" Stelle möglich.
    Speichern ist auch aus dem Replaymodus an beliebiger Stelle möglich.
    Laden von gespeicherten Spieleständen erfolgt im Mehrspieler-Modus
    über die Kartenauswahl und im Einzelspieler-Modus über den
    entsprechenden Menüpunkt.
    Mitspieler benötigen kein eigenes Savegame, dieses wird automatisch
    beim Verbinden übertragen.

  5. Optionen

    Die Einstellungen im Optionsmenü sollten so weit selbsterklärend sein.


--------------------------------------------------------------------------------

D. Abstürze und Fehler

  Wir arbeiten hart daran das Spiel immer weiter zu verbessern. Leider
  lassen sich Abstürze, Asyncs, Speicherzugriffs- und sonstige Fehler
  nicht ganz ausschließen.

  Durch Replays können Fehler leicht rekonstruiert werden. Ihr könnt uns
  mit dem Zusenden euer Replays helfen, weitere zu finden. Bitte stellt
  dabei sicher, dass ihr immer die neuste Nightly-Version verwendet.
  Fehler aus veralteten Versionen sind mitunter schon lange behoben.

  Falls ein Async aufgetreten ist, findet ihr im LOGS-Verzeichnis
  Log-Dateien. Schickt uns diese bitte von JEDEM(!) Spieler einschließlich
  eines Replays (Verzeichnis REPLAYS).
  Bitte stellt sicher, dass ihr den Async nicht absichtlich durch Spielen mit
  verschiedenen Versionen oder Cheaten herbeigeführt habt!

  Wenn ihr Fehler findet, bitte diese in Github posten:

      https://github.com/Return-To-The-Roots/s25client

  Github erhöht die Übersichtlichkeit für Bugs und ermöglicht euch zudem
  selber einsehen zu können, wann der Bug gefixt wurde. Außerdem bietet
  es eine gute Möglichkeit der Kommunikation zwischen uns und euch, vor
  allem wenn wir noch weitere Informationen von euch benötigen.

  Alternativ könnt ihr euch auch im Forum oder im IRC-Channel unter
  irc.freenode.net:6667/#siedler2.5 melden.
  Ihr könnt dem Chat auch über unsere Seite beitreten.

  Vielen Dank!

  Settlers Freaks
  7. Juli 2017

--------------------------------------------------------------------------------

E. Übersicht: Updates und Änderungen

  * 0.9.4 - 06.01.2022
  ------------------------------------------------------------------------------
  - Verschiedene Fixes für Bugs, die nicht-ladbare Savegames und Crashes verursachten
  - Fix für bisuelle glitches bei hohem Terrain
  - Drücken von ESC speichert die Einstellungen vor dem Schließen des Fensters
  - Fix für das nicht-schließbar Action-Window
  - Bei Spielstart werden die zuletzt geöffneten Fesnter wieder geöffnet und deren Position wiederhergestellt
  - Fix fehlerhafte Behandlung der Versionen (behebt Anzeigefehler und Fehler bei Beitritt zu anderen Spielern)

  * 0.9.1 - 24.07.2021
  ------------------------------------------------------------------------------
  - Vollbild Modus für alle Treiber und Betriebssystem
  - Random map generator
  - Sonderzeichen im Benutzernamen werden unterstüzt
  - Einige Fehler, Abstürze und Asyncs behoben
  - Map editor
  - Performanceverbesserungen

  * 0.8.2 - 22.08.2017
  ------------------------------------------------------------------------------
  - Viele Async Fixes
  - Nutzung von UTF8 um mehr Sprachen zu unterstützen
  - Viele neue Addons
  - Unterstützung aller Terrains aus S2
  - Zoom Funktion
  - Lua Scripting
  - LAN Lobby
  - Verbesserung der Code Qualität

  * 0.8.1
  ------------------------------------------------------------------------------
  - Fehlerkorrekturen
  - Funktionierende KI
  - Vollständige Seefahrt

  * 0.8.0
  ------------------------------------------------------------------------------
  - Starke Verbesserung der KI
  - Viele Bugfixes
  - Verbesserung der Geschwindigkeit
  - Beobachtungsfenster

  * 0.7.2 - 17.01.2011 *
  ------------------------------------------------------------------------------
  - Kritische Fehlerkorrekturen

  * 0.7 - Seventh version - 24.12.2010 *
  ------------------------------------------------------------------------------
  - OpenSource!
  - Übersetzung: Niederländisch
  - Übersetzung: Russisch (Zeichensatz fehlt)
  - Übersetzung: Czech
  - Übersetzung: Estonian
  - Übersetzung: Italian
  - Übersetzung: Norwegian
  - Übersetzung: Polish
  - Übersetzung: Slovenian
  - Übersetzung: Slovak
  - Statistik
  - Post
  - Diplomatie - noch nicht fertig
  - Erste KI von jh
  - Seefahrt (noch nicht fertig)
  - Addon Menü

  * 0.6 - Sechste Version - 25.01.2009 *
  ------------------------------------------------------------------------------
  - viele Bugs behoben
  - Fog of War (mit Teamview-Option)
  - Spähturm
  - Minimap
  - Mapvorschau im Hostmenü
  - Planierer
  - Balancing
  - Trägeranimationen hinzugefügt
  - Mehrsprachenunterstützung
  - Übersetzung: Spanisch
  - Übersetzung: Ungarisch
  - Übersetzung: Schwedisch
  - Übersetzung: Französisch (nicht abgeschlossen)
  - Übersetzung: Finnisch (nicht abgeschlossen)
  - Siegesmeldungen
  - Tastaturbefehle + Readme
  - Option im Hostmenü: Abriss-Verbot
  - Gebäude Info
  - Einige Hausanimationen hinzugefügt

  * 0.5 - Fünfte Version - 27.01.2008 *
  ------------------------------------------------------------------------------
  - sehr viele Bugs behoben
  - Eselstraßen, Eselzüchter
  - Bootsstraßen mit Booten und Werft (baut nur Boote)
  - Bergstraßen
  - Katapulte
  - Soldatenverhalten verändert, rücken nicht mehr so schnell nach
  - beim Abbrennen von Lagerhäusern und dem Hauptquartier flüchten
   nun alle Leute nach draußen wie im Original
  - Async-Logdatei
  - diverse Kleinigkeiten wie ein das Original-Abfragefenster vor dem
   Abbrennen von Gebäuden oder fehlende/falsche Tooltips
  - Sound an/aus Buttons im Spiel

  * 0.4 - Vierte Version - 09.10.2007 *
  ------------------------------------------------------------------------------
  - Speichern und Laden von Spielen eingebaut, inklusive Autosave
  - zig zum Teil schwere Bugs behoben

  * 0.3 - Dritte Version Fix01 -  13.09.2007 *
  ------------------------------------------------------------------------------
  - kritischen Absturz behoben

  * 0.3 - Dritte Version - 12.09.2007 *
  ------------------------------------------------------------------------------
  - diverse Absturzbugs behoben
  - Siedler warten nun, falls der Platz vor ihnen besetzt ist
    (durch Kämpfe z.B.)
  - Settings unter Linux im Home-Verzeichnis untergebracht
  - Settings in lesbares Format gebracht (GER-File)
  - Build-System unter Linux von autotools auf CMake umgestellt
  - CIA-Bot für den IRC aktiviert
  - Lobbygrundfunktionen fertiggestellt
  - Vorbereitungen für Speichern und Laden getroffen

  * 0.2 - Zweite Version - 15.07.2007 *
  ------------------------------------------------------------------------------
  - diverse Bugs behoben und diverse Verzögerungsbugs unterdrückt
  - Menüs in variabler Auflösung
  - Prüfen der Synchronität und Rausschmiss asynchroner Spieler
  - Inventurfenster eingebaut
  - "Nicht Einlagern"/"Auslagern" möglich
  - Bereit-Button im Host-Menü
  - Verteilung korrigiert
  - diverse fehlende Zierobjekte (z.B Stalagmiten, Ruinen) eingebaut
  - zu allen Gebäuden kann nun gesprungen werden
  - einige Zierobjekte werden beim Wegbau nun abgerissen
  - Produktivitätsanzeige und Namenanzeige (C, S) teilweise schon korrekt
   implementiert
  - Abrissbestätigung eingebaut
  - RoadWindow schließt sich nun, wenn man woanders hinklickt

  * 0.1 - Erster Release - 01.07.2007 *
  ------------------------------------------------------------------------------
  - alles ;-)

--------------------------------------------------------------------------------
https://www.siedler25.org                Copyright (C) 2005-2022 Settlers Freaks
--------------------------------------------------------------------------------
