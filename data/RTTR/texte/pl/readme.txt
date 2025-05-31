                              RETURN TO THE ROOTS
--------------------------------------------------------------------------------

A. Nota referencyjna
B. Instalacja
  1. Windows
  2. Linux
  3. Mac OSX
  4. Katalog ustawień
C. Gra
  1. Stwórz grę
  2. Gra wieloosobowa
  3. Powtórki
  4. Opcje
D. Awaria i błędy
E. Podsumowanie: Aktualizacje i dziennik zmian

--------------------------------------------------------------------------------

A. Nota referencyjna

  Gra wymaga karty graficznej kompatybilnej z OpenGL2.0 z co najmniej
  64 MB pamięci graficznej. Wystarczy procesor z taktowaniem 800 MHz.
  
  Dodatkowo potrzebna będzie zainstalowana wersja "The Settlers 2
  Gold-Edition" lub oryginalna wersja + Mission-CD.

--------------------------------------------------------------------------------

B. Instalacja
  1. Windows
  
    Obsługiwane są Windows 7 i 10. Windows Vista lub starsze mogą działać,
    ale nie są oficjalnie wspierane.
    
    Aby zagrać w grę, wyszukaj katalogi "DATA" i "GFX" w oryginalnej grze
    S2 Gold-Edition (lub S2 + Mission CD) (z pełnej instalacji lub bezpośrednio 
    z CD) i skopiuj je do folderu RttR w wersji official luub nightly 
    (gdzie znajdziesz plik "Put your S2-Files in here").
    
    Alternatywnie możesz jako Administrator utworzyć symboliczne linki NTFS:
    mklink /D DATA "C:\S2\DATA"
    mklink /D GFX "C:\S2\GFX"
    
    Aby zagrać w RTTR, wystarczy uruchomić s25client.exe.
    
    (W wersjach nightly możesz zaktualizować za pomocą pliku RTTR.BAT)
    
    Uwaga: RTTR nie zmienia żadnych plików z oryginalnej gry.
    Działa jako mod do gry.

   -----------------------------------------------------------------------------

  2. Linux
  
    Będziesz potrzebować następujących pakietów:
    (Użyj swojego menedżera pakietów lub zainstaluj je ręcznie)
    
    libsdl2 libsdl-mixer2 gettext
    
    Upewnij się, że DirectRendering działa:
    
    glxinfo | grep direct
    
    Jeśli pojawi się wynik "direct rendering: Yes", jest ok.
    Jeśli nie, wydajność będzie niska. Sprawdź akcelerację graficzną.
    
    Instalacja:
    
    wyodrębnij archiwum RTTR, np. do /opt/s25rttr
    
    mkdir -p /opt/s25rttr
    cd /opt/s25rttr
    tar -jxvf s25rttr_*.tar.bz2
    
    Teraz wystarczy skopiować oryginalne pliki do:
    
    /opt/s25rttr/share/s25rttr/S2
    
    Alternatywnie możesz utworzyć symboliczny link do oryginalnego folderu.
    
    /opt/s25rttr/bin/rttr.sh uruchomi grę.
    
    (Wersje nightly automatycznie aktualizują się po uruchomieniu rttr.sh.
     Użyj "sh rttr.sh noupdate", aby utrzymać obecną wersję.)

  ------------------------------------------------------------------------------

  3. Mac OSX
    
    Uruchom grę bezpośrednio z pobranego pakietu aplikacji.
  
  ------------------------------------------------------------------------------
  
  4. Katalog ustawień
    
    Katalog ustawień znajduje się w:
    
    Windows: <UserDir>/Saved Games/Return To The Roots
    Linux:   ~/.s25rttr
    Mac OSX: ~/Library/Application Support/Return To The Roots
    
    Ten folder jest tworzony przy pierwszym uruchomieniu, jeśli nie istnieje.
    Wszystkie logi, ustawienia, zapisy i powtórki znajdują się w podkatalogach 
    tego folderu.

--------------------------------------------------------------------------------

C. Gra

  1. Stwórz grę

    Istnieje tryb dla jednego gracza i tryb wieloosobowy, które można znaleźć 
    w głównym menu. Możesz również grać samodzielnie w trybie wieloosobowym:

    1. Wybierz "Gra wieloosobowa"
    2. "TCP/IP"
    3. "Stwórz grę"
    4. W następnym oknie wybierz nazwę gry, hasło nie jest wymagane.
    5. Na ekranie mapy możesz wybrać jedną z kategorii po lewej stronie 
       (podobnie jak w oryginale) i następnie mapę lub załadować 
       grę (przycisk "Wczytaj grę").
    6. Po wybraniu mapy kliknij "Kontynuuj", co przeniesie Cię do menu hosta. 
        Górna część tego menu pokazuje graczy i ich konfigurację. 
        Uwaga: Aby rozpocząć grę, musisz wypełnić wszystkie sloty graczami 
        komputerowymi lub ludźmi. 
        Alternatywnie możesz zamknąć sloty, klikając na nie.

    Aby szybko rozpocząć grę dla jednego gracza, kliknij "Nieograniczona gra" 
    w menu dla jednego gracza i wykonaj kroki 5 i 6 powyżej.

  2. Gra wieloosobowa

    a) Bezpośrednia gra: Procedura jest taka sama jak w punkcie 1. 
       Inni gracze muszą wybrać "Direct IP", "Dołącz do gry" i następnie 
       wprowadzić adres IP lub nazwę hosta. 
       "Połącz" przeniesie ich do menu hosta.

    b) Dostęp do lobby: Lobby służy do tworzenia własnych gier lub dołączania 
       do gier innych graczy. Zintegrowana jest również funkcja czatu. 
       Potrzebujesz konta na forum na stronie http://www.siedler25.org 
       i użyć go do zalogowania się do lobby gry.

    c) Gry sieciowe (LAN): Obszar LAN jest podobny do lobby, ale pokazuje 
      tylko mapy utworzone w bieżącej sieci LAN i nie wymaga logowania do 
       dołączenia. Możesz również użyć Hamachi lub podobnego programu do 
       tworzenia wirtualnych sieci LAN przez Internet.

    RTTR używa portu 3665 (TCP). Ten port musi być otwarty, aby utworzyć grę 
    i umożliwić innym osobom dołączenie. Jeśli używasz routera, musisz 
    przekierować ten port do swojej stacji roboczej. Poszukaj "Wirtualny serwer" 
    lub "Przekierowanie portów" w menu routera. Jeśli używasz osobistego zapory 
    na swojej stacji roboczej, port TCP 3665 również musi być tam dozwolony. 
    Ta metoda działa w Internecie i w sieci LAN. W trybie LAN używane są porty 
    3666 i 3667 (UDP) do wyszukiwania gier.

    Nie jest konieczne posiadanie mapy gry na każdym komputerze. Zostanie ona 
    automatycznie przesłana z hosta do każdego gracza.

    Kolorowy symbol ślimaka w prawym górnym rogu ekranu wskazuje na 
    złe połączenie z graczem o tym samym kolorze.

    Gra zostanie zatrzymana, gdy host opuści i nie można jej kontynuować.

    Aby wstrzymać grę, naciśnij "P" (tylko host) i ponownie naciśnij "P", 
    aby wznowić. 
    Aby porozmawiać z innymi graczami, naciśnij "Enter".

  3. Powtórki

    Powtórki rejestrują każde działanie w grze. Możesz je oglądać, wybierając 
    "Gra dla jednego gracza" - "Odtwórz powtórkę".

    Klawisze "+" i "-" zwiększą lub zmniejszą prędkość odtwarzania. 
    "J" pozwoli ci przejść do określonej klatki gry.

    Wszystkie powtórki można znaleźć w podfolderze "REPLAYS" w katalogu 
    ustawień gry.

  4. Zapisywanie

    Zapisywanie gry działa tak samo jak w oryginalnej grze. Działa również 
    w dowolnym miejscu powtórki. Aby załadować zapis gry wieloosobowej, 
    wybierz "Załaduj grę" w menu mapy. Dla trybu dla jednego gracza znajduje się 
    przycisk w menu dla jednego gracza. Wystarczy, aby tylko jeden gracz miał 
    plik zapisu gry.

  5. Opcje

    Opcje w głównym menu powinny być jasne i zrozumiałe.

  D. Awarie i błędy

  Mogą nadal występować awarie, asynchroniczności, błędy zapisu i inne błędy 
  w grze. Obecnie bardzo ciężko nad tym pracujemy.

  Możemy lepiej odtworzyć błędy, jeśli dostępna jest powtórka. 
  Możesz nam pomóc, wysyłając swoje powtórki (podfolder "REPLAYS" w folderze 
  ustawień) z opisem błędu. Upewnij się, że używasz najnowszej wersji nightly.

  Jeśli wystąpi błąd asynchroniczny, znajdziesz dodatkowe logi w 
  folderze "LOGS" w folderze ustawień. Wyślij nam logi KAŻDEGO gracza 
  i wszystkie powtórki. Ważne jest, aby podać, której wersji RTTR używałeś. 
  Na przykład: "20170630-49c9bb8 (nightly) lub 0.8.2".
  Upewnij się również, że nie spowodowałeś asynchroniczności celowo poprzez 
  oszukiwanie lub używanie różnych wersji.

  Jeśli znajdziesz błędy, zgłoś je na:

      https://github.com/Return-To-The-Roots/s25client

  Korzystając z Githuba, możemy uporządkować raporty o błędach. 
  Ułatwia to również sprawdzenie, czy twój błąd został już naprawiony. 
  Pozwala również na dalszą komunikację między tobą a nami, 
  np. gdy potrzebujemy od ciebie więcej informacji.

  Alternatywnie możesz zamieścić błąd na forum lub odwiedzić nas 
  na Discordzie: https://discord.gg/kyTQsSx Możesz również dołączyć 
  do kanału IRC i Discorda, odwiedzając naszą stronę główną.

  Dzięki!

  Settlers Freaks
  7 lipca 2017

--------------------------------------------------------------------------------

E. Podsumowanie: Aktualizacje i dziennik zmian

  * **0.9.4 - 06.01.2022**
  ------------------------------------------------------------------------------
  - Różne poprawki błędów prowadzących do niemożliwości wczytania zapisów gry 
    i awarii.
  - Naprawiono problemy z rysowaniem związane z wysokim terenem.
  - Naciśnięcie ESC nie powoduje już odrzucenia oczekujących zmian 
    w oknach ustawień.
  - Naprawiono okno akcji, które nie dało się zamknąć.
  - Po uruchomieniu gry ponownie otwierane są okna otwarte w poprzedniej sesji 
    i przywracane są ich pozycje.
  - Naprawiono wadliwą obsługę wersji (problemy wizualne i niemożność dołączenia 
    do innych graczy).

  * **0.9.1 - 24.07.2021**
  ------------------------------------------------------------------------------
  - Tryb pełnoekranowy na wszystkich sterownikach i systemach operacyjnych.
  - Generator losowych map.
  - Wsparcie znaków specjalnych w nazwie użytkownika.
  - Naprawiono niektóre błędy, awarie i problemy z asynchronicznością.
  - Dołączony edytor map.
  - Ulepszono wydajność.

  * **0.8.2 - 22.08.2017**
  ------------------------------------------------------------------------------
  - Wiele poprawek asynchroniczności.
  - Wprowadzono kodowanie UTF8 dla obsługi większej liczby języków.
  - Dołączono wiele dodatków.
  - Obsługa wszystkich typów terenu S2.
  - Funkcja przybliżenia.
  - Skryptowanie Lua.
  - Lobby sieciowe (LAN).
  - Poprawa jakości kodu.

  * **0.8.1**
  ------------------------------------------------------------------------------
  - Poprawki błędów.
  - Ulepszenia sztucznej inteligencji.
  - Pełna funkcjonalność żeglugi.

  * **0.8.0**
  ------------------------------------------------------------------------------
  - Znacznie ulepszona sztuczna inteligencja gracza.
  - Naprawiono wiele błędów.
  - Poprawa szybkości działania.
  - Okna obserwacyjne.

  * **0.7.2 - 17.01.2011**
  ------------------------------------------------------------------------------
  - Krytyczne poprawki błędów.

  * **0.7 - Siódma wersja - 24.12.2010**
  ------------------------------------------------------------------------------
  - OpenSource!
  - Tłumaczenie: Holenderskie
  - Tłumaczenie: Rosyjskie (brakuje czcionki)
  - Tłumaczenie: Czeskie
  - Tłumaczenie: Estońskie
  - Tłumaczenie: Włoskie
  - Tłumaczenie: Norweskie
  - Tłumaczenie: Polskie
  - Tłumaczenie: Słoweńskie
  - Tłumaczenie: Słowackie
  - Statystyki
  - Poczta
  - Dyplomacja - niedokończona
  - Pierwsza sztuczna inteligencja (jh)
  - Żegluga (jeszcze niedokończona)
  - Menu dodatków

  * **0.6 - Szósta wersja - 25.01.2009**
  ------------------------------------------------------------------------------
  - Naprawiono wiele błędów.
  - Mgła wojny (z opcją widoku drużynowego).
  - Dodano wieżę obserwacyjną.
  - Minimapa.
  - Minimapa w menu gry hosta.
  - Planer.
  - Balans gry.
  - Dodano animacje pomocników.
  - Wsparcie dla wielu języków.
  - Tłumaczenie: Hiszpańskie
  - Tłumaczenie: Węgierskie
  - Tłumaczenie: Szwedzkie
  - Tłumaczenie: Fińskie (niedokończone)
  - Tłumaczenie: Francuskie (niedokończone)
  - Komunikaty zwycięstwa.
  - Klawisze skrótów + plik CzytajTo.
  - Opcja w menu hosta: zakaz rozbiórki.
  - Informacje o budynkach.
  - Dodano kilka animacji budynków.

  * **0.5 - Piąta wersja - 27.01.2008**
  ------------------------------------------------------------------------------
  - Naprawiono wiele błędów.
  - Drogi dla osłów, hodowla osłów.
  - Drogi wodne z łodziami i stocznią (stocznia buduje tylko łodzie).
  - Droga górska.
  - Katapulty.
  - Zmienione zachowanie żołnierzy (kolejka).
  - Podpalenie składu powoduje ucieczkę wszystkich ludzi.
  - Log asynchroniczny.
  - Zmieniono niektóre okna akcji i podpowiedzi.
  - Włączanie/wyłączanie dźwięku w grze.

  * **0.4 - Czwarta wersja - 09.10.2007**
  ------------------------------------------------------------------------------
  - Zapisywanie i wczytywanie gier, automatyczne zapisywanie.
  - Naprawiono bardzo poważne błędy.

  * **0.3 - Trzecia wersja - 12.09.2007**
  ------------------------------------------------------------------------------
  - Naprawiono krytyczne błędy powodujące awarię gry.
  - Osadnicy ustawiają się w kolejce, gdy miejsce przed nimi jest zajęte.
  - Plik konfiguracyjny znajduje się odtąd w ~/.s25rttr (Linux).
  - Nowy format plików konfiguracyjnych (GER-File).
  - System budowania oparty na CMake (Linux).
  - Bot CIA dla kanału IRC.
  - Lobby.
  - Przygotowania do zapisywania i wczytywania gry.

  * **0.2 - Druga wersja - 15.07.2007**
------------------------------------------------------------------------------
  - Naprawiono wiele błędów.
  - Nowe rozdzielczości ekranu.
  - Możliwość wyrzucania asynchronicznych graczy.
  - Okno inwentarza.
  - Funkcja "Wyjmij/zatrzymaj magazynowanie" działa poprawnie.
  - Przycisk "Gotowy" w menu hosta.
  - Działa rozdzielanie towarów.
  - Dodano obiekty dekoracyjne (ruiny, ...).
  - Możliwość przejścia do każdego domu działa poprawnie.
  - Niektóre obiekty dekoracyjne są niszczone podczas układania drogi.
  - Widoczny status domów (C, S).
  - Potwierdzenie przed rozbiórką.
  - Okno drogi zamyka się po kliknięciu w innym miejscu.

  * 0.1 - Pierwsze Wydanie - 01.07.2007 *
  ------------------------------------------------------------------------------
  - Wszystko! ;-)

--------------------------------------------------------------------------------
https://www.siedler25.org                Copyright (C) 2005-2025 Settlers Freaks
--------------------------------------------------------------------------------
