2010.01.17                    RETURN TO THE ROOTS                          0.7.2
--------------------------------------------------------------------------------

A. Allgemeine Hinweise
B. Installation
   1. Windows
   2. Linux
C. Spiel
   1. Erstellen eines Spiels
   2. Multiplayer-Spiele
   3. Replays
   4. Optionen
D. Abstürze und Fehler
E. Übersicht: Updates und Änderungen

--------------------------------------------------------------------------------

A. Allgemeine Hinweise

   Das Spiel benötigt eine OpenGL-fähige Grafikkarte mit schätzungsweise
   mindestens 64 MiB Grafikspeicher. Ein Prozessor mit 800 MHz reicht aus.

   Weiterhin benötigt man eine installierte "Siedler 2 Gold-Edition" oder die
   Originalversion + Missions-CD.

--------------------------------------------------------------------------------

B. Installation

   1. Windows

      Windows 95/98/ME/Vista ist nicht offiziell unterstützt. Vista sollte
      aber eigentlich funktionieren.

      a) Zip-Archive
         Zum Spielen sucht man die Verzeichnisse "DATA" und "GFX" im
         Original "Die Siedler 2 Gold" Spiel (oder S2 + Mission CD) und kopiert
         diese in das Nightly-Verzeichnis (so dass s25client.exe sich auf 
         der selben Ebene wie S2.EXE befindet).

         Darin kann man dann s25client.exe zum Spielen von Siedler II.5 RTTR
         ausführen.
         
         (In Nightlies kann man mit RTTR.BAT updaten.)

      b) Setup
         Setup ausführen und als Installationsverzeichnis die installierte
         "Siedler 2 Gold-Edition" auswählen.
         Alternativ kann man natürlich nachtäglich den Inhalt der
         Gold-Edition in den Installationsordner von S25rttr kopieren.

         Das Spiel kann man dann bequem mit der installierten
         Desktopverknüpfung starten.

      Es werden KEINE originalen Spieldateien verändert. Man kann
      weiterhin das Original ohne Beeinträchtigung spielen.
      
   -------------------------------------------------------------------------

   2. Linux

      Zunächst benötigt man folgende Pakete
      (ggf über Paketmanager/per Hand installieren):

      libsndfile libsamplerate libsdl1.2 libsdl-mixer1.2 gettext timidity

      Weiterhin sollte man sicherstellen, dass DirectRendering funktioniert:

      glxinfo | grep direct

      Wenn die Ausgabe "direct rendering: Yes" kommt, ist alles in Ordnung,
      ansonsten kann es sein, dass man eine miese Spielperformance hat.

      Die eigentliche Installation:

      z.B. nach /opt/s25rttr entpacken:

      mkdir -p /opt/s25rttr
      cd /opt/s25rttr
      tar -jxvf s25rttr_???.tar.bz2

      Nun muss man entweder nur noch die original "Siedler 2"-Installation
      nach /opt/s25rttr/share/s25rttr/S2

      kopieren, oder einen Symlink dorthin anlegen.

      Starten kann man das Ganze dann mit

      /opt/s25rttr/bin/rttr.sh
      
      (Nightly Versionen updaten sich selber. Wenn man rttr.sh mit der
      Option noupdate startet, wird kein Update durchgeführt.)

--------------------------------------------------------------------------------

C. Spiel

   1. Erstellen eines Spiels

      Einen richtigen Einzelspielermodus gibt es in dem Sinne noch nicht, man
      kann aber trotzdem folgendermaßen alleine im Multiplayermodus siedeln:

      1. Im Hauptmenü auf Mehrspieler gehen
      2. Direkte IP
      3. Spiel erstellen
      4. Im folgenden Fenster einen Namen für das Spiel eingeben (Passwort
         kann weggelassen werden).
      5. Im Kartenauswahl-Fenster kann oben eine Kategorie ausgewählt
         werden, ähnlich wie im Original.
         Alternativ kann unten über "Spiel laden" ein Spielstand geladen
         werden.
      6. Nach der Auswahl der Karte und einem Klick auf "Weiter" gelangt man
         ins Host-Menü, wo im oberen Teil die einzelnen Spieler samt ihrer
         Eigenschaften aufgelistet sind.
         Spielziel, Aufklärung und Teams sperren sind bisher noch ohne
         Bedeutung.
         Achtung: Um das Spiel starten zu können, müssen entweder alle Plätze
         mit menschlichen oder KI-Spielern besetzt werden, oder die Slots
         müssen geschlossen werden (mit Klick auf die Buttons unter
         "Spielername").

         Die KI-Spieler sind zur Zeit noch nicht lebendig.

   2. Multiplayer-Spiele

      a) Direktes Spielen
         Die Erstellung eines Multiplayerspiels erfolgt genauso wie bei 1. Die
         weiteren Spieler gehen bei "Direkte IP" unter "Spiel beitreten" und
         geben die IP oder den Hostnamen des Spielerstellers (Host) ein.

      b) Spielen über integrierte Lobby
         Über die eingebaute Lobby kann man vorhandene Spiele einsehen,
         diese eröffnen und auch beitreten. Weiterhin kann man sich hier
         mit anderen Spielern direkt unterhalten.

         Der Funktionsumfang der Lobby ist momentan noch sehr beschränkt
         und es kann zeitweise zu Datenbank-Problemen kommen.
         Ein Punktesystem ist vorgesehen, aber noch nicht implementiert.

         Alle eingegebenen Daten werden nicht weitergegeben und dienen rein
         zur Identifikation des Spielers. Die zur Registrierung notwendige
         Emailadresse ist momentan nur ggf. zum Zusenden eines neuen
         Passworts gedacht und ist nicht öffentlich einsehbar.

      Für das Spiel wird Port 3665 (TCP) genutzt. Dieser muss, falls man selbst
      das Spiel eröffnen möchte, u.a. bei Verwendung eines Routers, einer
      Firewall usw. freigegeben werden!
      Router nennen diese Einstellung "Virtual Server" oder einfach nur
      "Port Forwarding". Dort ist dann Port 3665 vom Typ TCP auf die interne IP
      des Spiel-PCs weiterzuleiten.

      Die Schneckensymbole im oberen Teil stehen für Lags der jeweiligen
      Spieler.

      Wenn der Host das Spiel verlässt, können die übrigen Spieler noch nicht
      das Spiel weiter fortführen.

      Es sind sowohl Spiele über das Internet als auch über LAN möglich, wenn
      die jeweilige IP bekannt ist.

      Das Übertragen der Karte geschieht immer automatisch (auch wenn die
      Spielteilnehmer die Karte des Hosts schon besitzen).

      Drücken der Taste "P" pausiert das Spiel (dies kann nur der Host tun),
      weiteres Drücken von "P" lässt das spiel fortführen.

      Zum Chatten muss man "Enter" drücken, dann kommt ein Chatfenster.

   3. Replays

      Replays sind Aufzeichnungen von gespielten Partien. Sie können unter
      "Einzelspieler", "Replay abspielen" angesehen werden.

      Mit den Tasten [+] und [-] kann die Geschwindigkeit erhöht bzw.
      verringert werden. Mit der Taste "J" ist es möglich, Abschnitte im
      Replay zu überspringen.

      Replays werden aufgezeichnet und im Verzeichnis RTTR/REPLAYS
      abgespeichert.

   4. Speichern

      Speichern ist im Spielmenü an der "originalen" Stelle möglich.
      Speichern ist auch aus dem Replaymodus an beliebiger Stelle möglich.
      Laden von Spielen kann man nur aus dem Kartenauswahlbildschirm.
      Mitspieler benötigen kein eigenes Savegame, dieses wird automatisch
      beim Verbinden übertragen.

   5. Optionen

      Die Einstellungen im Optionsmenü sollten so weit selbsterklärend sein.


--------------------------------------------------------------------------------

D. Abstürze und Fehler

   Es handelt sich selbst bei dieser Version zwar schon um eine stabilere
   Variante. Abstürze, Asyncs, Speicherzugriffs- und sonstige Fehler
   lassen sich aber leider immer noch nicht ausschließen.

   Durch Replays können Fehler leicht rekonstruiert werden. Ihr könnt uns
   mit dem Zusenden euer Replays helfen, weitere zu finden. Bitte stellt
   aber sicher, dass ihr immer die neuste Daily-Version verwendet. Fehler
   aus möglicherweise veralteten Releaseversionen sind mitunter schon lange
   behoben.

   Falls ein Async aufgetreten ist, findet ihr im RTTR/LOGS/-Verzeichnis
   Log-Dateien. Schickt uns diese bitte von JEDEM(!) Spieler einschließlich
   eines Replays (Verzeichnis RTTR/REPLAYS).
   Bitte stellt sicher, dass ihr den Async nicht absichtlich durch Spielen mit
   verschiedenen Versionen oder Cheaten herbeigeführt habt!

   Wenn ihr Fehler findet, bitte diese im Launchpad posten:

               http://bugs.siedler25.org/

   Das Launchpad erhöht die Übersichtlichkeit für Bugs und ermöglicht euch
   zudem selber einsehen zu können, wann der Bug gefixt wurde.

   Bei weiteren Fragen bitte an
        
               bugs@siedler25.org

   wenden.

   Alternativ könnt ihr euch auch im Forum oder im IRC-Channel unter
   irc.freenode.net:6667/#siedler2.5 melden. Ihr könnt dem Chat auch über
   unsere Seite beitreten.

   Vielen Dank!

   Settlers Freaks
   23. November 2009

--------------------------------------------------------------------------------

E. Übersicht: Updates und Änderungen

 * 0.7.2 - 17.01.2011 *
   --------------------------------------------------------------------------
   - Kritische Fehlerkorrekturen

 * 0.7 - Seventh version - 24.12.2010 *
   --------------------------------------------------------------------------
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
   -------------------------------------------------------------------------
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
   -------------------------------------------------------------------------
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
   -------------------------------------------------------------------------
   - Speichern und Laden von Spielen eingebaut, inklusive Autosave
   - zig zum Teil schwere Bugs behoben

   * 0.3 - Dritte Version Fix01 -  13.09.2007 *
   -------------------------------------------------------------------------
   - kritischen Absturz behoben

   * 0.3 - Dritte Version - 12.09.2007 *
   -------------------------------------------------------------------------
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
   -------------------------------------------------------------------------
   - diverse Bugs behoben und diverse Verzögerungsbugs unterdrückt
   - Menüs in variabler Auflösung
   - Prüfen der Synchronität und Rausschmiss asynchroner Spieler
   - Inventurfenster eingebaut
   - "Nicht Einlagern"/"Auslagern" möglich
   - Bereit-Button im Host-Menü
   - Verteilung korrigiert
   - diverse fehlende Zierobjekte (z.B Stalagmiten, Ruinen) eingebaut
   - zu allen GebÃ¤uden kann nun gesprungen werden
   - einige Zierobjekte werden beim Wegbau nun abgerissen
   - Produktivitätsanzeige und Namenanzeige (C, S) teilweise schon korrekt
     implementiert
   - Abrissbestätigung eingebaut
   - RoadWindow schließt sich nun, wenn man woanders hinklickt

   * 0.1 - Erster Release - 01.07.2007 *
   -------------------------------------------------------------------------
   - alles ;-)

--------------------------------------------------------------------------------
http://www.siedler25.org                     Copyright (C) 2005-2009 Settlers Freaks
--------------------------------------------------------------------------------
