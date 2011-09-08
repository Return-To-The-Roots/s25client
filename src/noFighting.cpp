// $Id: noFighting.cpp 7521 2011-09-08 20:45:55Z FloSoft $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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
// Header
#include "main.h"
#include "noFighting.h"

#include "nofActiveSoldier.h"
#include "Random.h"
#include "EventManager.h"
#include "GameWorld.h"
#include "GameClient.h"
#include "Loader.h"
#include "noSkeleton.h"
#include "nobBaseMilitary.h"
#include "SoundManager.h"
#include "SerializedGameData.h"


///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
	#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
	#undef THIS_FILE
	static char THIS_FILE[] = __FILE__;
#endif

/// Kampfanimationskonstanten für einen Soldatenrang (Gespeichert werden jeweils die IDs in der ROM_BOBS.LST!)
struct FightAnimation
{
	// Angreifen (8 Frames)
	unsigned short attacking[8];
	// 3xVerteidigen mit jeweils 8 Frames
	unsigned short defending[3][8];
	
};


/// Diese gibts für alle beiden Richtung, für alle 5 Ränge und jweils nochmal für alle 4 Völker
const FightAnimation FIGHT_ANIMATIONS[4][5][2] =
{
	// Nubier
	{
		// Schütze (Rang 1)
		{
			{
				{ 595, 596, 597, 598, 599, 600, 601, 602 },
				{{1382,1383,1384,1385,1386,1387,1387,1388},{1388,1389,1390,1391,1392,1393,1393,1394},{ 1394, 1395, 1396, 1397, 1398, 1399, 1400, 1400 }}
			},
			{
				{ 633, 634, 635, 636, 637, 638, 639, 640 },
				{{ 1469, 1470, 1471, 1472, 1473, 1474, 1472, 1471 },{ 1469, 1475, 1476, 1477, 1478, 1479, 1480, 1469 },{ 1481, 1482, 1483, 1484, 1485, 1486, 1487, 1481 }}
			}
		},
		// Gefreiter (Rang 2)
		{
			{
				{ 603, 604, 605, 606, 607, 608, 609, 610 },
				{{ 1401, 1402, 1403, 1404, 1405, 1406, 1404, 1401 },{ 1407, 1407, 1408, 1409, 1410, 1411, 1412, 1412 },{ 1413, 1413, 1414, 1415, 1416, 1417, 1418, 1419 }}
			},
			{
				{ 641, 634, 642, 643, 644, 645, 646, 647 },
				{{ 1488, 1489, 1490, 1491, 1492, 1493, 1490, 1488 },{ 1494, 1494, 1495, 1496, 1497, 1498, 1499, 1494 },{ 1500, 1500, 1501, 1502, 1503, 1504, 1505, 1506 }}
			}
		},
		// Unteroffizier (Rang 3)
		{
			{
				{ 611, 612, 613, 614, 615, 616, 615, 614 },
				{{ 1420, 1420, 1421, 1422, 1423, 1424, 1422, 1420 },{ 1425, 1426, 1427, 1428, 1429, 1428, 1426, 1425 },{ 1430, 1430, 1431, 1432, 1433, 1434, 1432, 1431 }}
			},
			{
				{ 648, 649, 650, 651, 652, 653, 652, 651 },
				{{ 1507, 1507, 1508, 1509, 1510, 1511, 1508, 1508 },{ 1512, 1513, 1514, 1515, 1516, 1514, 1513, 1512 },{ 1517, 1517, 1518, 1519, 1520, 1521, 1519, 1517 }}
			}
		},
		// Offizier (Rang 4)
		{
			{
				{ 617, 618, 619, 620, 621, 622, 623, 624 },
				{{ 1435, 1435, 1436, 1437, 1438, 1439, 1437, 1436 },{ 1440, 1441, 1442, 1443, 1444, 1442, 1441, 1440 },{ 1445, 1445, 1446, 1447, 1448, 1449, 1447, 1445 }}
			},
			{
				{ 654, 655, 656, 657, 658, 659, 660, 661 },
				{{ 1522, 1522, 1523, 1524, 1525, 1526, 1524, 1524 },{ 1527, 1528, 1529, 1530, 1531, 1529, 1528, 1527 },{ 1527, 1532, 1533, 1534, 1535, 1536, 1528, 1527 }}
			}
		},
		// General (Rang 5)
		{
			{
				{ 625, 626, 627, 628, 629, 630, 631, 632 },
				{{ 1450, 1451, 1452, 1453, 1454, 1455, 1456, 1450 },{ 1457, 1457, 1458, 1459, 1460, 1458, 1457, 1457 },{ 1461, 1462, 1463, 1464, 1465, 1466, 1467, 1468 }}
			},
			{
				{ 662, 663, 664, 665, 666, 667, 668, 669 },
				{{ 1537, 1538, 1539, 1540, 1541, 1542, 1543, 1537 },{ 1544, 1544, 1545, 1546, 1547, 1545, 1544, 1544 },{ 1548, 1549, 1550, 1551, 1552, 1553, 1554, 1555 }}
			}
		}
	},

	// Japaner
	{
		// Schütze (Rang 1)
		{
			{
				{ 444, 445, 446, 447, 448, 446, 445, 444 },
				{{ 922, 922, 917, 918, 919, 920, 921, 922 },{ 917, 918, 925, 926, 927, 928, 929, 930 },{ 931, 932, 933, 934, 935, 935, 935, 935 }}
			},
			{
				{ 481, 482, 483, 484, 485, 484, 483, 481 },
				{{1029,1030,1031,1032,1033,1034,1035,1036},{1036,1037,1038,1039,1040,1041,1042,1042},{1042,1043,1044,1045,1046,1047,1048,1049}}
			}
		},
		// Gefreiter (Rang 2)
		{
			{
				{ 449, 450, 451, 452, 453, 454, 455, 456 },
				{{ 938, 939, 940, 941, 942, 943, 944, 945 },{ 946, 947, 948, 949, 950, 951, 952, 953 },{951, 952, 953, 954, 955, 956, 957, 958}}
			},
			{
				{ 486, 487, 488, 489, 490, 491, 491, 492 },
				{{1050,1051,1052,1053,1054,1055,1056,1057},{1058,1059,1060,1061,1062,1063,1064,1065},{1065,1066,1067,1068,1069,1070,1071,1072}}
			}
		},
		// Unteroffizier (Rang 3)
		{
			{
				{ 457, 458, 459, 460, 461, 462, 463, 464 },
				{{ 961, 962, 963, 964, 965, 966, 967, 967 },{ 968, 969, 970, 971, 972, 973, 974, 968 },{ 977, 978, 979, 980, 981, 982, 982, 976 }}
			},
			{
				{ 493, 494, 495, 496, 497, 498, 499, 500 },
				{{1073,1074,1075,1076,1077,1078,1079,1080},{1081,1082,1083,1084,1085,1086,1087,1088},{1089,1090,1091,1092,1093,1094,1095,1096}}
			}
		},
		// Offizier (Rang 4)
		{
			{
				{ 465, 466, 467, 468, 469, 470, 471, 472 },
				{{ 985, 986, 987, 988, 989, 990, 991, 992 },{ 985, 993, 994, 995, 996, 996, 997, 997 },{ 998, 999, 1000, 1001, 1002, 1003, 1004, 998 }}
			},
			{
				{ 502, 503, 504, 505, 506, 507, 508, 502 },
				{{1097,1098,1099,1100,1101,1102,1103,1104},{1105,1106,1107,1108,1109,1110,1111,1106},{1112,1113,1114,1115,1116,1117,1118,1112}}
			}
		},
		// General (Rang 5)
		{
			{
				{ 473, 474, 475, 476, 477, 478, 479, 480 },
				{{ 1007, 1008, 1009, 1010, 1011, 1012, 1013, 1014 },{ 1015, 1016, 1017, 1018, 1019, 1020, 1020, 1015 },{ 1023, 1024, 1025, 1026, 1027, 1028, 1023, 1023 }}
			},
			{
				{ 510, 511, 512, 513, 514, 515, 516, 510 },
				{{1119,1120,1121,1122,1123,1124,1125,1126},{1127,1128,1129,1130,1131,1132,1133,1134},{1135,1136,1137,1138,1139,1140,1141,1142}}
			}
		}
	},




	// Römer
	{
		// Schütze (Rang 1)
		{
			{
				{ 364, 365, 366, 367, 368, 369, 370, 371 },
				{{ 670, 671, 672, 673, 674, 675, 676, 676 },{ 677, 678, 679, 680, 681, 675, 682, 683 },{ 684, 685, 686, 687, 688, 689, 690, 690 }}
			},
			{
				{ 404, 405, 406, 407, 408, 409, 410, 411 },
				{{ 784, 785, 786, 787, 788, 789, 790, 790 },{ 791, 792, 793, 794, 795, 789, 796, 797 },{ 795, 796, 797, 798, 799, 800, 801, 802 }}
			}
		},
		// Gefreiter (Rang 2)
		{
			{
				{ 372, 373, 374, 375, 376, 377, 378, 379 },
				{{ 691, 692, 693, 694, 695, 696, 697, 697 },{ 698, 699, 700, 701, 702, 696, 703, 704 },{ 705, 706, 707, 708, 709, 710, 711, 705 }}
			},
			{
				{ 412, 413, 414, 415, 416, 417, 418, 419 },
				{{ 805, 806, 807, 808, 809, 810, 811, 805 },{ 812, 813, 814, 815, 816, 810, 817, 818 },{ 819, 820, 821, 822, 823, 824, 825, 819 }}
			}
		},

		// Unteroffizier (Rang 3)
		{
			{
				{ 380, 381, 382, 383, 384, 385, 386, 387 },
				{{ 712, 713, 714, 715, 716, 717, 718, 719 },{ 720, 721, 722, 723, 724, 725, 726, 727 },{ 728, 729, 730, 731, 732, 733, 734, 735 }}
			},
			{
				{ 420, 421, 422, 423, 424, 425, 426, 427 },
				{{ 826, 827, 828, 829, 830, 831, 832, 833 },{ 834, 835, 836, 837, 838, 839, 840, 841 },{ 842, 843, 844, 845, 846, 847, 840, 841 }}
			}
		},
		// Offizier (Rang 4)
		{
			{
				{ 388, 389, 390, 391, 392, 393, 394, 395 },
				{{ 736, 737, 738, 739, 740, 741, 742, 743 },{ 744, 745, 746, 747, 748, 749, 750, 751 },{ 752, 753, 754, 755, 756, 757, 758, 759 }}
			},
			{
				{ 428, 429, 430, 431, 432, 433, 434, 435 },
				{{ 850, 851, 852, 853, 854, 855, 856, 857 },{ 858, 859, 860, 861, 862, 863, 864, 865 },{ 864, 865, 866, 867, 868, 869, 870, 864 }}
			}
		},
		// General (Rang 5)
		{
			{
				{ 396, 397, 398, 399, 400, 401, 402, 403 },
				{{ 760, 761, 762, 763, 764, 765, 766, 767 },{ 768, 769, 770, 771, 772, 773, 774, 775 },{ 776, 777, 778, 779, 780, 781, 782, 783 }}
			},
			{
				{ 436, 437, 438, 439, 440, 441, 442, 443 },
				{{871, 872, 873, 874, 875, 876, 877, 878},{879, 880, 881, 882, 883, 884, 885, 886},{887,888,889,890,891,892,893,894}}
			}
		}
	},

	// Wikinger
	{
		// Schütze (Rang 1)
		{
			{
				{ 517, 518, 519, 520, 521, 522, 523, 524 },
				{{1151,1152,1153,1154,1155,1156,1157,1158},{1158,1159,1160,1161,1162,1163,1163,1164},{1165,1166,1167,1168,1169,1170,1171,1172}}
			},
			{
				{ 556, 557, 558, 559, 560, 561, 562, 563 },
				{{1265,1266,1267,1268,1269,1270,1271,1272},{1272,1273,1274,1275,1276,1277,1277,1278},{1280,1281,1282,1283,1284,1285,1286,1287}}
			}
		},
		// Gefreiter (Rang 2)
		{
			{
				{ 525, 526, 527, 528, 529, 528, 530, 531 },
				{{1173,1174,1175,1176,1177,1178,1179,1180},{1180,1181,1182,1183,1184,1185,1185,1186},{1187,1188,1189,1190,1191,1192,1193,1194}}
			},
			{
				{ 564, 565, 566, 567, 568, 567, 566, 565 },
				{{1287,1288,1289,1290,1291,1292,1293,1294},{1294,1295,1296,1297,1298,1299,1299,1300},{1301,1302,1303,1304,1305,1306,1307,1308}}
			}
		},
		// Unteroffizier (Rang 3)
		{
			{
				{ 532, 533, 534, 535, 536, 537, 538, 539 },
				{{1195,1196,1197,1198,1199,1200,1201,1202},{1203,1204,1205,1206,1207,1208,1209,1210},{1211,1212,1213,1214,1215,1216,1217,1218}}
			},
			{
				{ 571, 572, 573, 574, 575, 576, 577, 578 },
				{{1309,1310,1311,1312,1313,1314,1315,1316},{1317,1318,1319,1320,1321,1322,1323,1324},{1325,1326,1327,1328,1329,1330,1331,1332}}
			}
		},
		// Offizier (Rang 4)
		{
			{
				{ 540, 541, 542, 543, 544, 545, 546, 547 },
				{{1219,1220,1221,1222,1223,1224,1225,1225},{1226,1227,1228,1229,1230,1231,1232,1233},{1234,1235,1236,1237,1238,1239,1240,1235}}
			},
			{
				{ 579, 580, 581, 582, 583, 584, 585, 586 },
				{{1333,1334,1335,1336,1337,1338,1339,1339},{1340,1341,1342,1343,1344,1345,1346,1347},{1333,1334,1335,1336,1337,1338,1339,1339}}
			}
		},
		// General (Rang 5)
		{
			{
				{ 548, 549, 550, 551, 552, 553, 554, 555 },
				{{1241,1242,1243,1244,1245,1246,1247,1248},{1249,1250,1251,1252,1253,1254,1255,1256},{1257,1258,1259,1260,1261,1262,1263,1264}}
			},
			{
				{ 587, 588, 589, 590, 591, 592, 593, 594 },
				{{1356,1357,1358,1359,1360,1361,1362,1363},{1364,1365,1366,1367,1368,1369,1370,1371},{1348,1349,1350,1351,1352,1353,1354,1355}}
			}
		}
	},

};

/// IDs für die getroffenen (aufleuchtenden) Soldaten für jedes Volk
const unsigned short HIT_SOLDIERS[4][5] = { {1556,1558,1560,1562,1564},
											{1143,1145,1147,1149,1147},
											{895,897,899,901,899},
											{1372,1374,1376,1378,1380} };

/// Bestimmt den Aufblinkframe vom den Opfern der folgenden Angreifer (nach Rängen)
const unsigned short HIT_MOMENT[5] = {4,4,4,4,6};


noFighting::noFighting(nofActiveSoldier * soldier1,nofActiveSoldier * soldier2) : noBase(NOP_FIGHTING)
{
	assert(soldier1->GetPlayer() != soldier2->GetPlayer());

	soldiers[0] = soldier1;
	soldiers[1] = soldier2;
	turn = 2;
	defending_animation = 0;
	player_won = 0xFF;

	// Die beiden Soldaten erstmal aus der Liste hauen
	gwg->RemoveFigure(soldier1,soldier1->GetX(),soldier1->GetY());
	gwg->RemoveFigure(soldier2,soldier1->GetX(),soldier1->GetY());

	// Beginn-Event Anmelden (Soldaten gehen auf ihre Seiten)
	current_ev = em->AddEvent(this,15);

	// anderen Leute, die auf diesem Punkt zulaufen, stoppen
	gwg->StopOnRoads(soldier1->GetX(),soldier1->GetY());

	// Sichtradius behalten
	gwg->SetVisibilitiesAroundPoint(soldier1->GetX(),soldier1->GetY(),VISUALRANGE_SOLDIER,soldier1->GetPlayer());
	gwg->SetVisibilitiesAroundPoint(soldier1->GetX(),soldier1->GetY(),VISUALRANGE_SOLDIER,soldier2->GetPlayer());
}

void noFighting::Serialize_noFighting(SerializedGameData * sgd) const
{
	Serialize_noBase(sgd);

	sgd->PushUnsignedChar(turn);
	sgd->PushUnsignedChar(defending_animation);
	sgd->PushObject(current_ev,true);
	sgd->PushUnsignedChar(player_won);

	for(unsigned i = 0;i<2;++i)
		sgd->PushObject(soldiers[i],false);
}

noFighting::noFighting(SerializedGameData * sgd, const unsigned obj_id) : noBase(sgd,obj_id),
turn(sgd->PopUnsignedChar()),
defending_animation(sgd->PopUnsignedChar()),
current_ev(sgd->PopObject<EventManager::Event>(GOT_EVENT)),
player_won(sgd->PopUnsignedChar())

{
	for(unsigned i = 0;i<2;++i)
		soldiers[i] = sgd->PopObject<nofActiveSoldier>(GOT_UNKNOWN);
}

void noFighting::Destroy_noFighting()
{
	Destroy_noBase();
}

void noFighting::Draw(int x, int y)
{
	switch(turn)
	{
	case 3:
	case 4:
		{
			unsigned animation = GAMECLIENT.Interpolate(16,current_ev);

			// Sterben des einen Soldatens (letzte Phase)

			if(animation < 4)
			{
				// Noch kurz dastehen und warten, bis man stirbt
				glArchivItem_Bitmap *image = LOADER.GetImageN("rom_bobs", FIGHT_ANIMATIONS[gwg->GetPlayer(soldiers[turn-3]->GetPlayer())->nation][soldiers[turn-3]->GetRank()][turn-3].defending[0][0]);
				image->Draw(x,y,0,0,0,0,0,0,COLOR_WHITE, COLORS[gwg->GetPlayer(soldiers[turn-3]->GetPlayer())->color]);
			}
			else
				// Sich in Luft auflösen
				LOADER.GetImageN("rom_bobs", 903+animation-4)->Draw(x +((turn-3 == 0)?(-12):12),y,0,0,0,0,0,0,COLOR_WHITE, COLORS[gwg->GetPlayer(soldiers[turn-3]->GetPlayer())->color]);

			// Sterbesound abspielen
			if(animation == 6)
				SoundManager::inst().PlayNOSound(104,this,2);
					

		} break;
	case 2:
		{
			// Erste Phase des Kampfes, die Soldaten gehen jeweils nach links bzw. rechts
			int x_diff = int(GAMECLIENT.Interpolate(12,current_ev));
		
			for(unsigned i = 0;i<2;++i)
			{
				LOADER.GetBobN("jobs")->Draw(30+NATION_RTTR_TO_S2[gwg->GetPlayer(soldiers[i]->GetPlayer())->nation]*6+soldiers[i]->GetRank()
					,(i==0)?0:3,false,GAMECLIENT.Interpolate(8,current_ev),x + ((i == 0)?(-x_diff):x_diff),y,COLORS[gwg->GetPlayer(soldiers[i]->GetPlayer())->color]);
				soldiers[i]->DrawShadow(x + ((i == 0)?(-x_diff):x_diff),y,GAMECLIENT.Interpolate(8,current_ev),soldiers[i]->GetDir());
			}

		} break;
	default:
		{
			// Kampf 
			// Aktueller Animationsframe
			unsigned animation = GAMECLIENT.Interpolate(8,current_ev);

			for(unsigned i = 0;i<2;++i)
			{
				// Ist der Soldat mit Angreifen dran?
				if(turn == i)
				{
					// Angreifen
					LOADER.GetImageN("rom_bobs", 
					FIGHT_ANIMATIONS[gwg->GetPlayer(soldiers[i]->GetPlayer())->nation][soldiers[i]->GetRank()][i].
					attacking[animation])->Draw(x,y,0,0,0,0,0,0,COLOR_WHITE, COLORS[gwg->GetPlayer(soldiers[i]->GetPlayer())->color]);
				}
				else
				{
					// Verteidigen
					if(defending_animation < 3)
					{
						// Verteidigungsanimation
						LOADER.GetImageN("rom_bobs", 
						FIGHT_ANIMATIONS[gwg->GetPlayer(soldiers[i]->GetPlayer())->nation][soldiers[i]->GetRank()][i].
						defending[defending_animation][animation])->Draw(x,y,0,0,0,0,0,0,COLOR_WHITE, COLORS[gwg->GetPlayer(soldiers[i]->GetPlayer())->color]);

						// Wenn schwache Soldaten Schild hinhalten (Ani 0 und 1) und stärkere sich mit den Schwertern schützen (Ani 0)
						// dann Schwert-aneinanderklirr-Sound abspielen
						if( (animation == 5) && ((soldiers[i]->GetRank() < 2 && (defending_animation < 2)) || (soldiers[i]->GetRank() > 1 && (defending_animation == 0))))
							SoundManager::inst().PlayNOSound(101,this,1);

					}
					else
					{
						// Getroffen-Animation (weißes Aufblinken)
						if(GAMECLIENT.Interpolate(8,current_ev) == HIT_MOMENT[soldiers[!i]->GetRank()])
						{
							// weiß aufblinken
							LOADER.GetImageN("rom_bobs", 
							HIT_SOLDIERS[gwg->GetPlayer(soldiers[i]->GetPlayer())->nation][soldiers[i]->GetRank()]+i)
							->Draw(x,y,0,0,0,0,0,0,COLOR_WHITE, COLORS[gwg->GetPlayer(soldiers[i]->GetPlayer())->color]);

							// Treffersound
							SoundManager::inst().PlayNOSound(105,this,1);
						}
						else
							// normal dastehen
							LOADER.GetImageN("rom_bobs", 
							FIGHT_ANIMATIONS[gwg->GetPlayer(soldiers[i]->GetPlayer())->nation][soldiers[i]->GetRank()][i].
							defending[0][0])->Draw(x,y,0,0,0,0,0,0,COLOR_WHITE, COLORS[gwg->GetPlayer(soldiers[i]->GetPlayer())->color]);
					}
				}
			}

			// Angriffssound
			if(animation == 3)
				SoundManager::inst().PlayNOSound(103,this,0);



		} break;
	}

}

void noFighting::HandleEvent(const unsigned int id)
{
	// Normales Ablaufevent?
	if(id == 0)
	{
		switch(turn)
		{
		case 2:
			{
				// Der Kampf hat gerade begonnen

				// "Auslosen", wer als erstes dran ist mit Angreifen
				turn = static_cast<unsigned char>(RANDOM.Rand(__FILE__,__LINE__,obj_id,2));
				// anfangen anzugreifen
				StartAttack();
			} return;
		case 0:
		case 1:
			{
				// Sounds löschen von der letzten Kampfphase
				SoundManager::inst().WorkingFinished(this);

				// Wurde der eine getroffen?
				if(defending_animation == 3)
				{
					if(--soldiers[!turn]->hitpoints == 0)
					{
						// Soldat Bescheid sagen, dass er stirbt
						soldiers[!turn]->LostFighting();
						// Anderen Soldaten auf die Karte wieder setzen, Bescheid sagen, er kann wieder loslaufen
						gwg->AddFigure(soldiers[turn],soldiers[turn]->GetX(),soldiers[turn]->GetY());
						soldiers[turn]->WonFighting();
						// Besitzer merken für die Sichtbarkeiten am Ende dann
						player_won = soldiers[turn]->GetPlayer();
						soldiers[turn] = 0;
						// Hitpoints sind 0 --> Soldat ist tot, Kampf beendet, turn = 3+welche Soldat stirbt
						turn = 3+(!turn);
						// Event zum Sterben des einen Soldaten anmelden
						current_ev = em->AddEvent(this,30);
						// Umstehenden Figuren Bescheid nach gewisser Zeit Bescheid sagen
						/*em->AddEvent(this,RELEASE_FIGURES_OFFSET,1);*/
						gwg->RoadNodeAvailable(soldiers[turn-3]->GetX(),soldiers[turn-3]->GetY());

            // In die Statistik eintragen
						gwg->GetPlayer(player_won)->ChangeStatisticValue(STAT_VANQUISHED, 1);
						return;
					}
				}

				turn = !turn;
				StartAttack();
			} return;
		case 3:
		case 4:
			{
				unsigned player_lost = turn-3;
				MapCoord x = soldiers[player_lost]->GetX(), y = soldiers[player_lost]->GetY();

				// Sounds löschen vom Sterben
				SoundManager::inst().WorkingFinished(this);

				// Kampf ist endgültig beendet
				em->AddToKillList(this);
				gwg->RemoveFigure(this,x,y);

				// Wenn da nix war bzw. nur ein Verzierungsobjekt, kommt nun ein Skelett hin
				noBase * no = gwg->GetNO(x,y);
				if(no->GetType() == NOP_NOTHING || no->GetType() == NOP_ENVIRONMENT)
				{
					if(no->GetType() != NOP_NOTHING)
					{
						no->Destroy();
						delete no;
					}

					gwg->SetNO(new noSkeleton(x,y),x,y);
				}

				// Umstehenden Figuren Bescheid sagen
				//gwg->RoadNodeAvailable(soldiers[turn-3]->GetX(),soldiers[turn-3]->GetY());

				// Sichtradius ausblenden am Ende des Kampfes, an jeweiligen Soldaten dann übergeben, welcher überlebt hat
				gwg->RecalcVisibilitiesAroundPoint(x,y,VISUALRANGE_SOLDIER,soldiers[player_lost]->GetPlayer(),NULL);
				gwg->RecalcVisibilitiesAroundPoint(x,y,VISUALRANGE_SOLDIER,player_won,NULL);

				// Soldaten endgültig umbringen
				gwg->GetPlayer(soldiers[player_lost]->GetPlayer())->DecreaseInventoryJob(soldiers[player_lost]->GetJobType(),1);
				soldiers[player_lost]->Destroy();
				delete soldiers[player_lost];


				
			} break;
		}
	}
	// Figur-Weiterlauf-Event
	else if(id == 1)
	{
		//// Figuren weiterlaufen lassen
		//gwg->RoadNodeAvailable(soldiers[turn-3]->GetX(),soldiers[turn-3]->GetY());
	}
}

void noFighting::StartAttack()
{
	// "Auswürfeln", ob der Angreifer (also der, der gerade den Angriff vollzieht) trifft oder ob sich der andere
	// erfolgreich verteidigt

	unsigned char results[2];	
	for(unsigned i = 0;i<2;++i){
		switch (GameClient::inst().GetGGS().getSelection(ADDON_ADJUST_MILITARY_STRENGTH))
		{
		case 0: // Maximale Stärke
			{
				results[i] = RANDOM.Rand(__FILE__,__LINE__,obj_id,soldiers[i]->GetRank() + 6);
			} break;
		case 1: // Mittlere Stärke
			{
				results[i] = RANDOM.Rand(__FILE__,__LINE__,obj_id,soldiers[i]->GetRank() + 10);
			} break;
		case 2: // Minimale Stärke
			{
				results[i] = RANDOM.Rand(__FILE__,__LINE__,obj_id,10);
			} break;
		}		
	}

	if((turn == 0 && results[0] > results[1])
		|| (turn == 1 && results[1] > results[0]))
		// Der Angreifer hat diesen Zug gewonnen
		defending_animation = 3;
	else
		// Der Verteidiger hat diesen Zug gewonnen, zufällige Verteidigungsanimation
		defending_animation = static_cast<unsigned char>(RANDOM.Rand(__FILE__,__LINE__,obj_id,3));

	// Entsprechendes Event anmelden
	current_ev = em->AddEvent(this,15);

}

bool noFighting::IsActive() const
{
	// Figuren dürfen vorbei, wenn Kampf an sich und die Offset-Zeit abgelaufen ist
	return (turn < 3/* || GameClient::inst().GetGFNumber()-current_ev->gf < RELEASE_FIGURES_OFFSET*/);
}

bool noFighting::IsSoldierOfPlayer(const unsigned char player) const
{
	// Soldat 1 prüfen
	if(soldiers[0])
	{
		if(soldiers[0]->GetPlayer() == player)
			return true;
	}

	// Soldat 2 prüfen
	if(soldiers[1])
	{
		if(soldiers[1]->GetPlayer() == player)
			return true;
	}

	// Der Spieler der gewonnen hat und schon wieder gegangen ist (taucht dann nicht bei den ersten beiden mit
	// auf, wenn der Kampf beendet ist)
	if(player_won == player)
		return true;

	return false;
}

