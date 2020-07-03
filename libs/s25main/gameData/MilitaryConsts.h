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

#ifndef MILITARY_CONSTS_H_
#define MILITARY_CONSTS_H_

#include "DrawPoint.h"
#include "helpers/MultiArray.h"
#include "gameTypes/JobTypes.h"
#include "gameData/NationConsts.h"
#include <array>

/// Größe der Militärquadrate (in Knotenpunkten), in die die Welt eingeteilt wurde für Militärgebäude
const unsigned short MILITARY_SQUARE_SIZE = 20;

/// Maximale Entfernungen für "nahe Militärgebäudedistanz" und "mittlere Militärgebäudedistanz"
const unsigned MAX_MILITARY_DISTANCE_NEAR = 18;
const unsigned MAX_MILITARY_DISTANCE_MIDDLE = 26;

/// highest military rank - currently ranks 0-4 available
const unsigned MAX_MILITARY_RANK = NUM_SOLDIER_RANKS - 1u;
/// Number of military buildings
constexpr unsigned NUM_MILITARY_BLDS = 4;

/// Basisangriffsreichweite (Angriff mit allen Soldaten möglich)
const unsigned BASE_ATTACKING_DISTANCE = 21;

/// Erweiterte Reichweite, für die jeweils ein Soldat von der Angriffsarmee abgezogen wird
const unsigned EXTENDED_ATTACKING_DISTANCE = 1;

/// Maximale Länge für den Laufweg beim Angriff
const unsigned MAX_ATTACKING_RUN_DISTANCE = 40;

/// Distanz zwischen zwei Gegnern, sodass diese aufeinander zugehen
const unsigned MEET_FOR_FIGHT_DISTANCE = 5;

/// Besatzung in den einzelnen Militärgebäuden und nach Nation
const helpers::MultiArray<int, NUM_NATS, NUM_MILITARY_BLDS> NUM_TROOPS = {
  {{2, 3, 6, 9}, {2, 3, 6, 9}, {2, 3, 6, 9}, {2, 3, 6, 9}, {2, 3, 6, 9}}};

/// Gold in den einzelnen Militärgebäuden und nach Nation
const helpers::MultiArray<unsigned short, NUM_NATS, NUM_MILITARY_BLDS> NUM_GOLDS = {
  {{1, 2, 4, 6}, {1, 2, 4, 6}, {1, 2, 4, 6}, {1, 2, 4, 6}, {1, 2, 4, 6}}};

/// Radien der Militärgebäude
const std::array<unsigned, NUM_MILITARY_BLDS> SUPPRESS_UNUSED MILITARY_RADIUS = {{8, 9, 10, 11}};
// Radius für einzelne Hafen(baustellen)
const unsigned HARBOR_RADIUS = 8;
const unsigned HQ_RADIUS = 9;

/// Offset of the troop flag per nation and type from the buildings origin
const helpers::MultiArray<DrawPoint, NUM_NATS, NUM_MILITARY_BLDS> TROOPS_FLAG_OFFSET = {{{{24, -41}, {19, -41}, {31, -88}, {35, -67}},
                                                                                         {{-9, -49}, {14, -59}, {16, -63}, {0, -44}},
                                                                                         {{-24, -36}, {9, -62}, {-2, -80}, {23, -75}},
                                                                                         {{-5, -50}, {-5, -51}, {-9, -74}, {-12, -58}},
                                                                                         {{-22, -37}, {-2, -51}, {20, -70}, {-46, -64}}}};

/// Offset of the troop flag per nation from the HQs origin
const std::array<DrawPoint, NUM_NATS> TROOPS_FLAG_HQ_OFFSET = {{{-12, -102}, {-19, -94}, {-18, -112}, {20, -54}, {-33, -81}}};

/// Offset of the border indicator flag per nation from the buildings origin
const helpers::MultiArray<DrawPoint, NUM_NATS, NUM_MILITARY_BLDS> BORDER_FLAG_OFFSET = {{{{-6, -36}, {7, -48}, {-18, -28}, {-47, -64}},
                                                                                         {{17, -45}, {-3, -49}, {-30, -25}, {22, -53}},
                                                                                         {{28, -19}, {29, -18}, {-27, -12}, {-49, -62}},
                                                                                         {{24, -19}, {24, -19}, {17, -52}, {-37, -32}},
                                                                                         {{8, -26}, {13, -36}, {-1, -59}, {-10, -61}}}};

/// maximale Hitpoints der Soldaten von jedem Volk
const helpers::MultiArray<unsigned char, NUM_NATS, NUM_SOLDIER_RANKS> HITPOINTS = {
  {{3, 4, 5, 6, 7}, {3, 4, 5, 6, 7}, {3, 4, 5, 6, 7}, {3, 4, 5, 6, 7}, {3, 4, 5, 6, 7}}};

/// Max distance for an attacker to reach a building and join in capturing
const unsigned MAX_FAR_AWAY_CAPTURING_DISTANCE = 15;

/// Sichtweite der Militärgebäude (relativ); wird auf die normale Grenzweite draufaddiert
const unsigned VISUALRANGE_MILITARY = 3;
/// Sichtweite von Spähtürmen (absolut)
const unsigned VISUALRANGE_LOOKOUTTOWER = 20;
/// Sichtweite von Spähern
const unsigned VISUALRANGE_SCOUT = 3;
/// Sichtweite von Soldaten
const unsigned VISUALRANGE_SOLDIER = 2;
/// Sichtweite von Schiffen
const unsigned VISUALRANGE_SHIP = 2;
/// Sichtweite von Erkundungs-Schiffen
const unsigned VISUALRANGE_EXPLORATION_SHIP = 12;

/// Beförderungszeit von Soldaten ( =UPGRADE_TIME + rand(UPGRADE_TIME_RANDOM) )
const unsigned UPGRADE_TIME = 100;
const unsigned UPGRADE_TIME_RANDOM = 300;
/// Genesungszeit von Soldaten in Häusern, Zeit, die gebraucht wird um sich um einen Hitpoint zu erholen
// ( =CONVALESCE_TIME + rand(CONVALESCE_TIME_RANDOM) )
const unsigned CONVALESCE_TIME = 500;
const unsigned CONVALESCE_TIME_RANDOM = 500;

/// Maximale Entfernung des Militärgebäudes von dem Hafen bei Seeangriffen
const unsigned SEAATTACK_DISTANCE = 15;

/// Kampfanimationskonstanten für einen Soldatenrang (Gespeichert werden jeweils die IDs in der ROM_BOBS.LST!)
struct FightAnimation
{
    // Angreifen (8 Frames)
    std::array<unsigned short, 8> attacking;
    // 3xVerteidigen mit jeweils 8 Frames
    unsigned short defending[3][8];
};

/// Diese gibts für alle beiden Richtung, für alle 5 Ränge und jweils nochmal für alle 4 Völker
const FightAnimation FIGHT_ANIMATIONS[NUM_NATS][NUM_SOLDIER_RANKS][2] = {
  // Nubier
  {// Schütze (Rang 1)
   {{{595, 596, 597, 598, 599, 600, 601, 602},
     {{1382, 1383, 1384, 1385, 1386, 1387, 1387, 1388},
      {1388, 1389, 1390, 1391, 1392, 1393, 1393, 1394},
      {1394, 1395, 1396, 1397, 1398, 1399, 1400, 1400}}},
    {{633, 634, 635, 636, 637, 638, 639, 640},
     {{1469, 1470, 1471, 1472, 1473, 1474, 1472, 1471},
      {1469, 1475, 1476, 1477, 1478, 1479, 1480, 1469},
      {1481, 1482, 1483, 1484, 1485, 1486, 1487, 1481}}}},
   // Gefreiter (Rang 2)
   {{{603, 604, 605, 606, 607, 608, 609, 610},
     {{1401, 1402, 1403, 1404, 1405, 1406, 1404, 1401},
      {1407, 1407, 1408, 1409, 1410, 1411, 1412, 1412},
      {1413, 1413, 1414, 1415, 1416, 1417, 1418, 1419}}},
    {{641, 634, 642, 643, 644, 645, 646, 647},
     {{1488, 1489, 1490, 1491, 1492, 1493, 1490, 1488},
      {1494, 1494, 1495, 1496, 1497, 1498, 1499, 1494},
      {1500, 1500, 1501, 1502, 1503, 1504, 1505, 1506}}}},
   // Unteroffizier (Rang 3)
   {{{611, 612, 613, 614, 615, 616, 615, 614},
     {{1420, 1420, 1421, 1422, 1423, 1424, 1422, 1420},
      {1425, 1426, 1427, 1428, 1429, 1428, 1426, 1425},
      {1430, 1430, 1431, 1432, 1433, 1434, 1432, 1431}}},
    {{648, 649, 650, 651, 652, 653, 652, 651},
     {{1507, 1507, 1508, 1509, 1510, 1511, 1508, 1508},
      {1512, 1513, 1514, 1515, 1516, 1514, 1513, 1512},
      {1517, 1517, 1518, 1519, 1520, 1521, 1519, 1517}}}},
   // Offizier (Rang 4)
   {{{617, 618, 619, 620, 621, 622, 623, 624},
     {{1435, 1435, 1436, 1437, 1438, 1439, 1437, 1436},
      {1440, 1441, 1442, 1443, 1444, 1442, 1441, 1440},
      {1445, 1445, 1446, 1447, 1448, 1449, 1447, 1445}}},
    {{654, 655, 656, 657, 658, 659, 660, 661},
     {{1522, 1522, 1523, 1524, 1525, 1526, 1524, 1524},
      {1527, 1528, 1529, 1530, 1531, 1529, 1528, 1527},
      {1527, 1532, 1533, 1534, 1535, 1536, 1528, 1527}}}},
   // General (Rang 5)
   {{{625, 626, 627, 628, 629, 630, 631, 632},
     {{1450, 1451, 1452, 1453, 1454, 1455, 1456, 1450},
      {1457, 1457, 1458, 1459, 1460, 1458, 1457, 1457},
      {1461, 1462, 1463, 1464, 1465, 1466, 1467, 1468}}},
    {{662, 663, 664, 665, 666, 667, 668, 669},
     {{1537, 1538, 1539, 1540, 1541, 1542, 1543, 1537},
      {1544, 1544, 1545, 1546, 1547, 1545, 1544, 1544},
      {1548, 1549, 1550, 1551, 1552, 1553, 1554, 1555}}}}},

  // Japaner
  {// Schütze (Rang 1)
   {{{444, 445, 446, 447, 448, 446, 445, 444},
     {{922, 922, 917, 918, 919, 920, 921, 922}, {917, 918, 925, 926, 927, 928, 929, 930}, {931, 932, 933, 934, 935, 935, 935, 935}}},
    {{481, 482, 483, 484, 485, 484, 483, 481},
     {{1029, 1030, 1031, 1032, 1033, 1034, 1035, 1036},
      {1036, 1037, 1038, 1039, 1040, 1041, 1042, 1042},
      {1042, 1043, 1044, 1045, 1046, 1047, 1048, 1049}}}},
   // Gefreiter (Rang 2)
   {{{449, 450, 451, 452, 453, 454, 455, 456},
     {{938, 939, 940, 941, 942, 943, 944, 945}, {946, 947, 948, 949, 950, 951, 952, 953}, {951, 952, 953, 954, 955, 956, 957, 958}}},
    {{486, 487, 488, 489, 490, 491, 491, 492},
     {{1050, 1051, 1052, 1053, 1054, 1055, 1056, 1057},
      {1058, 1059, 1060, 1061, 1062, 1063, 1064, 1065},
      {1065, 1066, 1067, 1068, 1069, 1070, 1071, 1072}}}},
   // Unteroffizier (Rang 3)
   {{{457, 458, 459, 460, 461, 462, 463, 464},
     {{961, 962, 963, 964, 965, 966, 967, 967}, {968, 969, 970, 971, 972, 973, 974, 968}, {977, 978, 979, 980, 981, 982, 982, 976}}},
    {{493, 494, 495, 496, 497, 498, 499, 500},
     {{1073, 1074, 1075, 1076, 1077, 1078, 1079, 1080},
      {1081, 1082, 1083, 1084, 1085, 1086, 1087, 1088},
      {1089, 1090, 1091, 1092, 1093, 1094, 1095, 1096}}}},
   // Offizier (Rang 4)
   {{{465, 466, 467, 468, 469, 470, 471, 472},
     {{985, 986, 987, 988, 989, 990, 991, 992}, {985, 993, 994, 995, 996, 996, 997, 997}, {998, 999, 1000, 1001, 1002, 1003, 1004, 998}}},
    {{502, 503, 504, 505, 506, 507, 508, 502},
     {{1097, 1098, 1099, 1100, 1101, 1102, 1103, 1104},
      {1105, 1106, 1107, 1108, 1109, 1110, 1111, 1106},
      {1112, 1113, 1114, 1115, 1116, 1117, 1118, 1112}}}},
   // General (Rang 5)
   {{{473, 474, 475, 476, 477, 478, 479, 480},
     {{1007, 1008, 1009, 1010, 1011, 1012, 1013, 1014},
      {1015, 1016, 1017, 1018, 1019, 1020, 1020, 1015},
      {1023, 1024, 1025, 1026, 1027, 1028, 1023, 1023}}},
    {{510, 511, 512, 513, 514, 515, 516, 510},
     {{1119, 1120, 1121, 1122, 1123, 1124, 1125, 1126},
      {1127, 1128, 1129, 1130, 1131, 1132, 1133, 1134},
      {1135, 1136, 1137, 1138, 1139, 1140, 1141, 1142}}}}},

  // Römer
  {// Schütze (Rang 1)
   {{{364, 365, 366, 367, 368, 369, 370, 371},
     {{670, 671, 672, 673, 674, 675, 676, 676}, {677, 678, 679, 680, 681, 675, 682, 683}, {684, 685, 686, 687, 688, 689, 690, 690}}},
    {{404, 405, 406, 407, 408, 409, 410, 411},
     {{784, 785, 786, 787, 788, 789, 790, 790}, {791, 792, 793, 794, 795, 789, 796, 797}, {795, 796, 797, 798, 799, 800, 801, 802}}}},
   // Gefreiter (Rang 2)
   {{{372, 373, 374, 375, 376, 377, 378, 379},
     {{691, 692, 693, 694, 695, 696, 697, 697}, {698, 699, 700, 701, 702, 696, 703, 704}, {705, 706, 707, 708, 709, 710, 711, 705}}},
    {{412, 413, 414, 415, 416, 417, 418, 419},
     {{805, 806, 807, 808, 809, 810, 811, 805}, {812, 813, 814, 815, 816, 810, 817, 818}, {819, 820, 821, 822, 823, 824, 825, 819}}}},

   // Unteroffizier (Rang 3)
   {{{380, 381, 382, 383, 384, 385, 386, 387},
     {{712, 713, 714, 715, 716, 717, 718, 719}, {720, 721, 722, 723, 724, 725, 726, 727}, {728, 729, 730, 731, 732, 733, 734, 735}}},
    {{420, 421, 422, 423, 424, 425, 426, 427},
     {{826, 827, 828, 829, 830, 831, 832, 833}, {834, 835, 836, 837, 838, 839, 840, 841}, {842, 843, 844, 845, 846, 847, 840, 841}}}},
   // Offizier (Rang 4)
   {{{388, 389, 390, 391, 392, 393, 394, 395},
     {{736, 737, 738, 739, 740, 741, 742, 743}, {744, 745, 746, 747, 748, 749, 750, 751}, {752, 753, 754, 755, 756, 757, 758, 759}}},
    {{428, 429, 430, 431, 432, 433, 434, 435},
     {{850, 851, 852, 853, 854, 855, 856, 857}, {858, 859, 860, 861, 862, 863, 864, 865}, {864, 865, 866, 867, 868, 869, 870, 864}}}},
   // General (Rang 5)
   {{{396, 397, 398, 399, 400, 401, 402, 403},
     {{760, 761, 762, 763, 764, 765, 766, 767}, {768, 769, 770, 771, 772, 773, 774, 775}, {776, 777, 778, 779, 780, 781, 782, 783}}},
    {{436, 437, 438, 439, 440, 441, 442, 443},
     {{871, 872, 873, 874, 875, 876, 877, 878}, {879, 880, 881, 882, 883, 884, 885, 886}, {887, 888, 889, 890, 891, 892, 893, 894}}}}},

  // Wikinger
  {// Schütze (Rang 1)
   {{{517, 518, 519, 520, 521, 522, 523, 524},
     {{1151, 1152, 1153, 1154, 1155, 1156, 1157, 1158},
      {1158, 1159, 1160, 1161, 1162, 1163, 1163, 1164},
      {1165, 1166, 1167, 1168, 1169, 1170, 1171, 1172}}},
    {{556, 557, 558, 559, 560, 561, 562, 563},
     {{1265, 1266, 1267, 1268, 1269, 1270, 1271, 1272},
      {1272, 1273, 1274, 1275, 1276, 1277, 1277, 1278},
      {1280, 1281, 1282, 1283, 1284, 1285, 1286, 1287}}}},
   // Gefreiter (Rang 2)
   {{{525, 526, 527, 528, 529, 528, 530, 531},
     {{1173, 1174, 1175, 1176, 1177, 1178, 1179, 1180},
      {1180, 1181, 1182, 1183, 1184, 1185, 1185, 1186},
      {1187, 1188, 1189, 1190, 1191, 1192, 1193, 1194}}},
    {{564, 565, 566, 567, 568, 567, 566, 565},
     {{1287, 1288, 1289, 1290, 1291, 1292, 1293, 1294},
      {1294, 1295, 1296, 1297, 1298, 1299, 1299, 1300},
      {1301, 1302, 1303, 1304, 1305, 1306, 1307, 1308}}}},
   // Unteroffizier (Rang 3)
   {{{532, 533, 534, 535, 536, 537, 538, 539},
     {{1195, 1196, 1197, 1198, 1199, 1200, 1201, 1202},
      {1203, 1204, 1205, 1206, 1207, 1208, 1209, 1210},
      {1211, 1212, 1213, 1214, 1215, 1216, 1217, 1218}}},
    {{571, 572, 573, 574, 575, 576, 577, 578},
     {{1309, 1310, 1311, 1312, 1313, 1314, 1315, 1316},
      {1317, 1318, 1319, 1320, 1321, 1322, 1323, 1324},
      {1325, 1326, 1327, 1328, 1329, 1330, 1331, 1332}}}},
   // Offizier (Rang 4)
   {{{540, 541, 542, 543, 544, 545, 546, 547},
     {{1219, 1220, 1221, 1222, 1223, 1224, 1225, 1225},
      {1226, 1227, 1228, 1229, 1230, 1231, 1232, 1233},
      {1234, 1235, 1236, 1237, 1238, 1239, 1240, 1235}}},
    {{579, 580, 581, 582, 583, 584, 585, 586},
     {{1333, 1334, 1335, 1336, 1337, 1338, 1339, 1339},
      {1340, 1341, 1342, 1343, 1344, 1345, 1346, 1347},
      {1333, 1334, 1335, 1336, 1337, 1338, 1339, 1339}}}},
   // General (Rang 5)
   {{{548, 549, 550, 551, 552, 553, 554, 555},
     {{1241, 1242, 1243, 1244, 1245, 1246, 1247, 1248},
      {1249, 1250, 1251, 1252, 1253, 1254, 1255, 1256},
      {1257, 1258, 1259, 1260, 1261, 1262, 1263, 1264}}},
    {{587, 588, 589, 590, 591, 592, 593, 594},
     {{1356, 1357, 1358, 1359, 1360, 1361, 1362, 1363},
      {1364, 1365, 1366, 1367, 1368, 1369, 1370, 1371},
      {1348, 1349, 1350, 1351, 1352, 1353, 1354, 1355}}}}},
  // Babylonier
  {
    { // Private (Rank 0)
     {// left
      {1810, 1811, 1812, 1813, 1814, 1815, 1816, 1817},
      {{1818, 1819, 1820, 1821, 1822, 1823, 1824, 1825},
       {1826, 1827, 1828, 1829, 1830, 1831, 1832, 1833},
       {1834, 1835, 1836, 1837, 1838, 1839, 1840, 1841}}},
     {// right
      {1842, 1843, 1844, 1845, 1846, 1847, 1848, 1849},
      {{1850, 1851, 1852, 1853, 1854, 1855, 1856, 1857},
       {1858, 1859, 1860, 1861, 1862, 1863, 1864, 1865},
       {1866, 1867, 1868, 1869, 1870, 1871, 1872, 1873}}}},
    { // Private FC (Rank 1)
     {// left
      {1874, 1875, 1876, 1877, 1878, 1879, 1880, 1881},
      {{1882, 1883, 1884, 1885, 1886, 1887, 1888, 1889},
       {1890, 1891, 1892, 1893, 1894, 1895, 1896, 1897},
       {1898, 1899, 1900, 1901, 1902, 1903, 1904, 1905}}},
     {// right
      {1906, 1907, 1908, 1909, 1910, 1911, 1912, 1913},
      {{1914, 1915, 1916, 1917, 1918, 1919, 1920, 1921},
       {1922, 1923, 1924, 1925, 1926, 1927, 1928, 1929},
       {1930, 1931, 1932, 1933, 1934, 1935, 1936, 1937}}}},
    { // Sergeant (Rank 2)
     {// left
      {1938, 1939, 1940, 1941, 1942, 1943, 1944, 1945},
      {{1946, 1947, 1948, 1949, 1950, 1951, 1952, 1953},
       {1954, 1955, 1956, 1957, 1958, 1959, 1960, 1961},
       {1962, 1963, 1964, 1965, 1966, 1967, 1968, 1969}}},
     {// right
      {1970, 1971, 1972, 1973, 1974, 1975, 1976, 1977},
      {{1978, 1979, 1980, 1981, 1982, 1983, 1984, 1985},
       {1986, 1987, 1988, 1989, 1990, 1991, 1992, 1993},
       {1994, 1995, 1996, 1997, 1998, 1999, 2000, 2001}}}},
    { // Officer (Rank 3)
     {// left
      {2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009},
      {{2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017},
       {2018, 2019, 2020, 2021, 2022, 2023, 2024, 2025},
       {2026, 2027, 2028, 2029, 2030, 2031, 2032, 2033}}},
     {// right
      {2034, 2035, 2036, 2037, 2038, 2039, 2040, 2041},
      {{2042, 2043, 2044, 2045, 2046, 2047, 2048, 2049},
       {2050, 2051, 2052, 2053, 2054, 2055, 2056, 2057},
       {2058, 2059, 2060, 2061, 2062, 2063, 2064, 2065}}}},
    { // General (Rank 4)
     {// left
      {2066, 2067, 2068, 2069, 2070, 2071, 2072, 2073},
      {{2074, 2075, 2076, 2077, 2078, 2079, 2080, 2081},
       {2082, 2083, 2084, 2085, 2086, 2087, 2088, 2089},
       {2090, 2091, 2092, 2093, 2094, 2095, 2096, 2097}}},
     {// right
      {2098, 2099, 2100, 2101, 2102, 2103, 2104, 2105},
      {{2106, 2107, 2108, 2109, 2110, 2111, 2112, 2113},
       {2114, 2115, 2116, 2117, 2118, 2119, 2120, 2121},
       {2122, 2123, 2124, 2125, 2126, 2127, 2128, 2129}}}},
  },
};

/// IDs für die getroffenen (aufleuchtenden) Soldaten für jedes Volk
const helpers::MultiArray<unsigned short, NUM_NATS, NUM_SOLDIER_RANKS> HIT_SOLDIERS = {{{1556, 1558, 1560, 1562, 1564},
                                                                                        {1143, 1145, 1147, 1149, 1147},
                                                                                        {895, 897, 899, 901, 899},
                                                                                        {1372, 1374, 1376, 1378, 1380},
                                                                                        {2130, 2132, 2134, 2136, 2138}}};

/// Bestimmt den Aufblinkframe vom den Opfern der folgenden Angreifer (nach Rängen)
const std::array<unsigned short, NUM_SOLDIER_RANKS> SUPPRESS_UNUSED HIT_MOMENT = {{4, 4, 4, 4, 6}};

#endif
