// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#pragma once

#include "gameTypes/Nation.h"

const unsigned NUM_SHIP_NAMESS = 55;

const helpers::MultiArray<const char*, NUM_NATIONS, NUM_SHIP_NAMESS> ship_names = {
  {/* Nubier */ {"Aica",    "Aida",    "Ainra",    "Alayna", "Alisha",   "Alma",    "Amila",  "Anina",
                 "Armina",  "Banu",    "Baya",     "Bea",    "Bia",      "Bisa",    "Cheche", "Dafina",
                 "Daria",   "Dina",    "Do",       "Dofi",   "Efia",     "Erin",    "Esi",    "Esra",
                 "Fahari",  "Faraya",  "Fujo",     "Ghiday", "Habiaba",  "Hajunza", "Ina",    "Layla",
                 "Lenia",   "Lillian", "Malika",   "Mona",   "Naja",     "Neriman", "Nyela",  "Olufunmilayo",
                 "Panyin",  "Rayyan",  "Rhiannon", "Safiya", "Sahra",    "Selda",   "Senna",  "Shaira",
                 "Shakira", "Sharina", "Sinah",    "Suada",  "Sulamith", "Tiada",   "Yelda"},
   /* Japaner */ {"Ai",     "Aiko",    "Aimi",    "Akemi",  "Amaya",   "Aoi",    "Ayaka",  "Ayano",
                  "Beniko", "Chiyo",   "Chiyoko", "Emi",    "Fumiko",  "Haruka", "Hiroko", "Hotaru",
                  "Kaori",  "Kasumi",  "Kazuko",  "Kazumi", "Keiko",   "Kiriko", "Kumiko", "Mai",
                  "Mayumi", "Megumi",  "Midori",  "Misaki", "Miu",     "Moe",    "Nanami", "Naoko",
                  "Naomi",  "Natsuki", "Noriko",  "Reika",  "Sachiko", "Sadako", "Sakura", "Satsuki",
                  "Sayuri", "Setsuko", "Shigeko", "Teiko",  "Tomomi",  "Umeko",  "Yoko",   "Yoshiko",
                  "Youko",  "Yukiko",  "Yumi",    "Yumiko", "Yuna",    "Yuuka",  "Yuzuki"},
   /* Römer */ {"Antia",      "Ateia",    "Aurelia",  "Camilia",   "Claudia",  "Duccia",   "Epidia",    "Equitia",
                "Fabia",      "Galeria",  "Helvetia", "Iunia",     "Iusta",    "Iuventia", "Lafrenia",  "Livia",
                "Longinia",   "Maelia",   "Maxima",   "Nigilia",   "Nipia",    "Norbana",  "Novia",     "Orania",
                "Otacilia",   "Petronia", "Pinaria",  "Piscia",    "Pisentia", "Placidia", "Quintia",   "Quirinia",
                "Rusonia",    "Rutilia",  "Sabucia",  "Sallustia", "Salonia",  "Salvia",   "Scribonia", "Secundia",
                "Secundinia", "Tadia",    "Talmudia", "Tanicia",   "Tertinia", "Tita",     "Ulpia",     "Umbrenia",
                "Valeria",    "Varia",    "Vassenia", "Vatinia",   "Vedia",    "Velia",    "Verania"},
   /* Wikinger */ {"Adelberga", "Adelgund", "Adelheid",   "Adelinde", "Alsuna",    "Alwina",   "Amelinde",  "Astrid",
                   "Baltrun",   "Bernhild", "Bothilde",   "Dagny",    "Dankrun",   "Eldrid",   "Erlgard",   "Fehild ",
                   "Ferun",     "Frauke ",  "Freya",      "Gerda ",   "Gesa",      "Gismara",  "Hella",     "Henrike ",
                   "Hilke",     "Ida",      "Irma",       "Irmlinde", "Isantrud ", "Kunheide", "Kunigunde", "Lioba",
                   "Lykke",     "Marada",   "Margard ",   "Merlinde", "Minnegard", "Nanna",    "Norwiga",   "Oda",
                   "Odarike",   "Osrun ",   "Raginhild ", "Raskild ", "Rinelda",   "Runa",     "Runhild ",  "Salgard",
                   "Sarhild",   "Tanka",    "Tyra",       "Ulla",     "Uta",       "Walda",    "Wiebke"},
   /* Babylonier */
   {"Anu",
    "Enlil",
    "Ea",
    "Sin",
    "Samas",
    "Istar",
    "Marduk",
    "Nabu",
    "Ninurta",
    "Nusku",
    "Nergal",
    "Adad",
    "Tammuz",
    "Asalluchi",
    "Tutu",
    "Nabu-mukin-zeri",
    "Tiglath-Pileser",
    "Shalmaneser",
    "Marduk-apla-iddina",
    "Sharrukin",
    "Sin-ahhe-eriba",
    "Bel-ibni",
    "Ashur-nadin-shumi",
    "Kandalanu",
    "Sin-shumu-lishir",
    "Sinsharishkun",
    "Ninurta-apla",
    "Agum",
    "Burnaburiash",
    "Kashtiliash",
    "Ulamburiash",
    "Karaindash",
    "Kurigalzu",
    "Shuzigash",
    "Gandas",
    "Abi-Rattas",
    "Hurbazum",
    "Gulkishar",
    "Peshgaldaramesh",
    "Ayadaragalama",
    "Akurduana",
    "Melamkurkurra",
    "Hammurabi",
    "Samsu-Ditana",
    "Bishapur",
    "Ekbatana",
    "Gundischapur",
    "Ktesiphon",
    "Bactra",
    "Pasargadae",
    "Persepolis",
    "Susa",
    "Rayy",
    "Pa-Rasit",
    "Spi-Keone"}}};

//{"FloSoftius", "Demophobius", "Olivianus", "Spikeonius", "Nastius"};
