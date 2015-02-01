################################################################################
### $Id: CMakeLists.txt 9596 2015-02-01 09:41:54Z marcus $
################################################################################

INCLUDE(FindBZip2)
IF (NOT BZIP2_FOUND)
	MESSAGE(FATAL_ERROR "FATAL ERROR: BZip2 library not found")
ENDIF (NOT BZIP2_FOUND)
ADD_DEFINITIONS(${BZIP2_DEFINITIONS})

INCLUDE(FindOpenGL)
IF (NOT OPENGL_FOUND)
	MESSAGE(FATAL_ERROR "FATAL ERROR: OpenGL library not found")
ENDIF (NOT OPENGL_FOUND)

INCLUDE(FindGettext)
IF (NOT GETTEXT_FOUND)
	MESSAGE(FATAL_ERROR "FATAL ERROR: gettext library not found")
ENDIF (NOT GETTEXT_FOUND)

################################################################################

INCLUDE_DIRECTORIES("../" "../liblobby/src/" "../libutil/src/" "../mygettext/src/" "../libsiedler2/src/" "../libendian/src/")
LINK_DIRECTORIES("../libsiedler2/src" "../mygettext/src" "../libutil/src" "../liblobby/src" "../libendian/src")

################################################################################

SET(SOURCES_CONTROLS
	ctrlBuildingIcon.cpp                ctrlBuildingIcon.h
	ctrlButton.cpp                      ctrlButton.h
	ctrlChat.cpp                        ctrlChat.h
	ctrlCheck.cpp                       ctrlCheck.h
	ctrlComboBox.cpp                    ctrlComboBox.h
	ctrlDeepening.cpp                   ctrlDeepening.h
	ctrlEdit.cpp                        ctrlEdit.h
	ctrlGroup.cpp                       ctrlGroup.h
	ctrlImage.cpp                       ctrlImage.h
	ctrlList.cpp                        ctrlList.h
	ctrlMinimap.cpp                     ctrlMinimap.h
	ctrlPreviewMinimap.cpp              ctrlPreviewMinimap.h
	ctrlIngameMinimap.cpp               ctrlIngameMinimap.h
	ctrlMultiline.cpp                   ctrlMultiline.h
	ctrlOptionGroup.cpp                 ctrlOptionGroup.h
	ctrlPercent.cpp                     ctrlPercent.h
	ctrlProgress.cpp                    ctrlProgress.h
	ctrlRectangle.cpp                   ctrlRectangle.h
	ctrlScrollBar.cpp                   ctrlScrollBar.h
	ctrlTab.cpp                         ctrlTab.h
	ctrlTable.cpp                       ctrlTable.h
	ctrlText.cpp                        ctrlText.h
	ctrlTimer.cpp                       ctrlTimer.h
	ctrlVarDeepening.cpp                ctrlVarDeepening.h
	ctrlVarText.cpp                     ctrlVarText.h
	ctrlMultiSelectGroup.cpp            ctrlMultiSelectGroup.h
   )

SET(SOURCES_DESKTOPS
	dskAboutRTTR.cpp                   dskAboutRTTR.h
	dskCredits.cpp                     dskCredits.h
	dskDirectIP.cpp                    dskDirectIP.h
	dskGameInterface.cpp               dskGameInterface.h
	dskHostGame.cpp                    dskHostGame.h
	dskIntro.cpp                       dskIntro.h
	dskLobby.cpp                       dskLobby.h
	dskMainMenu.cpp                    dskMainMenu.h
	dskMultiPlayer.cpp                 dskMultiPlayer.h
	dskOptions.cpp                     dskOptions.h
	dskSelectMap.cpp                   dskSelectMap.h
	dskSinglePlayer.cpp                dskSinglePlayer.h
	dskSplash.cpp                      dskSplash.h
	dskGameLoader.cpp                  dskGameLoader.h
   )

SET(SOURCES_INGAMEWINDOWS
	iwAction.cpp                        iwAction.h
	iwAddons.cpp                        iwAddons.h
	iwAIDebug.cpp                       iwAIDebug.h
	iwBaseWarehouse.cpp                 iwBaseWarehouse.h
	iwBuilding.cpp                      iwBuilding.h
	iwBuildingProductivities.cpp        iwBuildingProductivities.h
	iwBuildings.cpp                     iwBuildings.h
	iwBuildingSite.cpp                  iwBuildingSite.h
	iwBuildOrder.cpp                    iwBuildOrder.h
	iwChat.cpp                          iwChat.h
	iwDirectIPConnect.cpp               iwDirectIPConnect.h
	iwDemolishBuilding.cpp              iwDemolishBuilding.h
	iwDiplomacy.cpp                     iwDiplomacy.h
	iwDirectIPCreate.cpp                iwDirectIPCreate.h
	iwDistribution.cpp                  iwDistribution.h
	iwEndgame.cpp                       iwEndgame.h
	iwHarborBuilding.cpp                iwHarborBuilding.h
	iwHelp.cpp                          iwHelp.h
	iwHQ.cpp                            iwHQ.h
	iwInventory.cpp                     iwInventory.h
	iwLobbyConnect.cpp                  iwLobbyConnect.h
	iwLobbyRanking.cpp                  iwLobbyRanking.h
	iwLobbyServerInfo.cpp               iwLobbyServerInfo.h
	iwMainMenu.cpp                      iwMainMenu.h
	iwMilitaryBuilding.cpp              iwMilitaryBuilding.h
	iwMilitary.cpp                      iwMilitary.h
	iwMinimap.cpp                       iwMinimap.h
	iwMsgbox.cpp                        iwMsgbox.h
	iwMusicPlayer.cpp                   iwMusicPlayer.h
	iwObservate.cpp                     iwObservate.h
	iwOptionsWindow.cpp                 iwOptionsWindow.h
	iwPlayReplay.cpp                    iwPlayReplay.h
	iwPleaseWait.cpp                    iwPleaseWait.h
	iwPostWindow.cpp                    iwPostWindow.h
	iwRoadWindow.cpp                    iwRoadWindow.h
	iwSave.cpp                          iwSave.h
	iwSettings.cpp                      iwSettings.h
	iwShip.cpp                          iwShip.h
	iwSkipGFs.cpp                       iwSkipGFs.h
	iwStorehouse.cpp                    iwStorehouse.h
	iwSurrender.cpp                     iwSurrender.h
	iwStatistics.cpp                    iwStatistics.h
	iwTextfile.cpp                      iwTextfile.h
	iwTools.cpp                         iwTools.h
	iwTrade.cpp                         iwTrade.h
	iwTransport.cpp                     iwTransport.h
	iwWares.cpp                         iwWares.h
	iwMerchandiseStatistics.cpp         iwMerchandiseStatistics.h
	iwMissionStatement.cpp              iwMissionStatement.h
   )

SET(SOURCES_AI
	AIBase.h
	AIPlayer.cpp                        AIPlayer.h
	AIPlayerJH.cpp                      AIPlayerJH.h
	AIJHHelper.cpp                      AIJHHelper.h
	AIEventManager.cpp                  AIEventManager.h
	AIConstruction.cpp                  AIConstruction.h
	AIInterface.cpp                     AIInterface.h
   )

SET(SOURCES_NO
	noAnimal.cpp                        noAnimal.h
	noBase.cpp                          noBase.h
	noBaseBuilding.cpp                  noBaseBuilding.h
	noCoordBase.cpp                     noCoordBase.h
	noBuilding.cpp                      noBuilding.h
	noBuildingSite.cpp                  noBuildingSite.h
	noDisappearingEnvObject.cpp         noDisappearingEnvObject.h
	noDisappearingMapEnvObject.cpp      noDisappearingMapEnvObject.h
	noEnvObject.cpp                     noEnvObject.h
	noExtension.cpp                     noExtension.h
	noFighting.cpp                      noFighting.h
	noFigure.cpp                        noFigure.h
	noFire.cpp                          noFire.h
	noFlag.cpp                          noFlag.h
	noGrainfield.cpp                    noGrainfield.h
	noGranite.cpp                       noGranite.h
	noMovable.cpp                       noMovable.h
	noNothing.cpp                       noNothing.h
	noRoadNode.cpp                      noRoadNode.h
	noShipBuildingSite.cpp              noShipBuildingSite.h
	noSign.cpp                          noSign.h
	noShip.cpp                          noShip.h
	noSkeleton.cpp                      noSkeleton.h
	noStaticObject.cpp                  noStaticObject.h
	noTree.cpp                          noTree.h
	noCharburnerPile.cpp                noCharburnerPile.h
   )

SET(SOURCES_NOB
	nobBaseMilitary.cpp                 nobBaseMilitary.h
	nobBaseWarehouse.cpp                nobBaseWarehouse.h
	nobHarborBuilding.cpp               nobHarborBuilding.h
	nobHQ.cpp                           nobHQ.h
	nobMilitary.cpp                     nobMilitary.h
	nobShipYard.cpp                     nobShipYard.h
	nobStorehouse.cpp                   nobStorehouse.h
	nobUsual.cpp                        nobUsual.h
   )

SET(SOURCES_NOF
	nofActiveSoldier.cpp                nofActiveSoldier.h
	nofAggressiveDefender.cpp           nofAggressiveDefender.h
	nofArmorer.cpp                      nofArmorer.h
	nofAttacker.cpp                     nofAttacker.h
	nofBaker.cpp                        nofBaker.h
	nofBrewer.cpp                       nofBrewer.h
	nofBuilder.cpp                      nofBuilder.h
	nofBuildingWorker.cpp               nofBuildingWorker.h
	nofButcher.cpp                      nofButcher.h
	nofCarpenter.cpp                    nofCarpenter.h
	nofCarrier.cpp                      nofCarrier.h
	nofCatapultMan.cpp                  nofCatapultMan.h
	nofDefender.cpp                     nofDefender.h
	nofFarmer.cpp                       nofFarmer.h
	nofFarmhand.cpp                     nofFarmhand.h
	nofFisher.cpp                       nofFisher.h
	nofFlagWorker.cpp                   nofFlagWorker.h
	nofForester.cpp                     nofForester.h
	nofGeologist.cpp                    nofGeologist.h
	nofDonkeybreeder.cpp                nofDonkeybreeder.h
	nofHunter.cpp                       nofHunter.h
	nofIronfounder.cpp                  nofIronfounder.h
	nofMetalworker.cpp                  nofMetalworker.h
	nofMiller.cpp                       nofMiller.h
	nofMiner.cpp                        nofMiner.h
	nofMinter.cpp                       nofMinter.h
	nofPassiveSoldier.cpp               nofPassiveSoldier.h
	nofPassiveWorker.cpp                nofPassiveWorker.h
	nofPigbreeder.cpp                   nofPigbreeder.h
	nofPlaner.cpp                       nofPlaner.h
	nofScout_Free.cpp                   nofScout_Free.h
	nofScout_LookoutTower.cpp           nofScout_LookoutTower.h
	nofShipWright.cpp                   nofShipWright.h
	nofSoldier.cpp                      nofSoldier.h
	nofStonemason.cpp                   nofStonemason.h
	nofTradeLeader.cpp                  nofTradeLeader.h
	nofTradeDonkey.cpp                  nofTradeDonkey.h
	nofWarehouseWorker.cpp              nofWarehouseWorker.h
	nofWellguy.cpp                      nofWellguy.h
	nofWoodcutter.cpp                   nofWoodcutter.h
	nofWorkman.cpp                      nofWorkman.h
	nofCharburner.cpp                   nofCharburner.h
	)

SET(SOURCES_OTHER
										BuildingConsts.h
	BurnedWarehouse.cpp                 BurnedWarehouse.h
	CatapultStone.cpp                   CatapultStone.h
	CollisionDetection.cpp              CollisionDetection.h
	ClientInterface.cpp                 ClientInterface.h
										const_gui_ids.h
										controls.h
	customborderbuilder.h               customborderbuilder.cpp
	Desktop.cpp                         Desktop.h
										DoorConsts.h
	EventManager.cpp                    EventManager.h
	ExtensionList.cpp                   ExtensionList.h
	FileChecksum.cpp                    FileChecksum.h
	GameClient.cpp                      GameClient.h
	GameClientCommands.cpp
	GameClientGF_Game.cpp
	GameClientGF_Replay.cpp
	GameClientPlayer.cpp                GameClientPlayer.h
										GameConsts.h
	GameSavedFile.cpp                   GameSavedFile.h
	GameReplay.cpp                      GameReplay.h
	GameSavegame.cpp                    GameSavegame.h
										GameInterface.h
	GameManager.cpp                     GameManager.h
	GameMessage.cpp                     GameMessage.h
	GameMessageInterface.cpp            GameMessageInterface.h
	GameCommands.cpp                    GameCommands.h
	GameObject.cpp                      GameObject.h
	GamePlayerInfo.cpp                  GamePlayerInfo.h
	GameProtocol.h
	GameServer.cpp                      GameServer.h
	GameServerPlayer.cpp                GameServerPlayer.h
	GameWorld.cpp                       GameWorld.h
	GameWorldBase.cpp
	GameWorldGame.cpp
	GameWorldView.cpp
	GameWorldViewer.cpp
										GammaTable.h
	GlobalGameSettings.cpp              GlobalGameSettings.h
	GlobalVars.cpp                      GlobalVars.h
	IngameWindow.cpp                    IngameWindow.h
										JobConsts.h
	languages.cpp                       languages.h
										list.h
	ListDir.cpp                         ListDir.h
	Loader.cpp                          Loader.h
										macros.h
	main.cpp                            main.h
										MapConsts.h
	MapGeometry.cpp                     MapGeometry.h
										MouseAndKeys.h
	Messenger.cpp                       Messenger.h
										MilitaryConsts.h
	Minimap.cpp                         Minimap.h
										MinimapConsts.h
	MusicPlayer.cpp                     MusicPlayer.h
										NodalObjectTypes.h
										Node.h
	DriverWrapper.cpp                   DriverWrapper.h
	VideoDriverWrapper.cpp              VideoDriverWrapper.h
	AudioDriverWrapper.cpp              AudioDriverWrapper.h
										AudioDriverLoaderInterface.h
										VideoDriverLoaderInterface.h
	Pathfinding.cpp
										PointDistance.h
	Random.cpp                          Random.h
	RoadSegment.cpp                     RoadSegment.h
	SerializedGameData.cpp
	Settings.cpp                        Settings.h
	signale.cpp                         signale.h
	SoundManager.cpp
										Swap.h
	TerrainRenderer.cpp                 TerrainRenderer.h
	TerritoryRegion.cpp                 TerritoryRegion.h
	TradeGraph.cpp                      TradeGraph.h
	Ware.cpp                            Ware.h
	Window.cpp                          Window.h
	WindowManager.cpp                   WindowManager.h
	FOWObjects.cpp                      FOWObjects.h
	PostMsg.cpp                         PostMsg.h
	Addons.cpp                          Addons.h
										AddonAsyncDebug.h
										AddonChangeGoldDeposits.h
										AddonMaxWaterwayLength.h
										AddonMaxRank.h
										AddonCustomBuildSequence.h
										AddonStatisticsVisibility.h
										AddonLimitCatapults.h
										AddonInexhaustibleFish.h
										AddonInexhaustibleMines.h
										AddonInexhaustibleGraniteMines.h
										AddonDefenderBehavior.h
										AddonAIDebugWindow.h
										AddonToolOrdering.h
										AddonAdjustMilitaryStrength.h
										AddonCatapultGraphics.h
										AddonExhaustibleWells.h
										AddonManualRoadEnlargement.h
										AddonNoCoinsDefault.h
										AddonSeaAttack.h
										AddonTrade.h
										AddonRefundMaterials.h
										AddonCharburner.h
										AddonDemolitionProhibition.h
										AddonMilitaryAid.h
										AddonRefundOnEmergency.h
										AddonShipSpeed.h
										AddonMoreAnimals.h
										AddonBurnDuration.h
										AddonNoAlliedPush.h
										AddonBattlefieldPromotion.h
										AddonHalfCostMilEquip.h
										AddonMilitaryControl.h
										const_addons.h
	win32_nanosleep.cpp                 win32_nanosleep.h
	Debug.cpp                           Debug.h
   )

SET(SOURCES_GL
	glAllocator.cpp                     glAllocator.h
	glArchivItem_Bitmap.cpp             glArchivItem_Bitmap.h
	glArchivItem_Bitmap_Direct.cpp      glArchivItem_Bitmap_Direct.h
	glArchivItem_Bitmap_Player.cpp      glArchivItem_Bitmap_Player.h
	glArchivItem_Bitmap_Raw.cpp         glArchivItem_Bitmap_Raw.h
	glArchivItem_Bitmap_RLE.cpp         glArchivItem_Bitmap_RLE.h
	glArchivItem_Bitmap_Shadow.cpp      glArchivItem_Bitmap_Shadow.h
	glArchivItem_Bob.cpp                glArchivItem_Bob.h
	glArchivItem_Font.cpp               glArchivItem_Font.h
	glArchivItem_Map.cpp                glArchivItem_Map.h
	glArchivItem_Music.cpp              glArchivItem_Music.h
	glArchivItem_Sound.cpp              glArchivItem_Sound.h
	glArchivItem_Sound_Midi.cpp         glArchivItem_Sound_Midi.h
	glArchivItem_Sound_Wave.cpp         glArchivItem_Sound_Wave.h
	glArchivItem_Sound_XMidi.cpp        glArchivItem_Sound_XMidi.h
	glArchivItem_Sound_Other.cpp        glArchivItem_Sound_Other.h
	glSmartBitmap.cpp                   glSmartBitmap.h
   )

################################################################################

GETTEXT_CREATE_TRANSLATIONS(../RTTR/languages/rttr.pot ALL
							../RTTR/languages/rttr-de.po
							../RTTR/languages/rttr-es.po
							../RTTR/languages/rttr-sv.po
							../RTTR/languages/rttr-fi.po
							../RTTR/languages/rttr-fr.po
							../RTTR/languages/rttr-ru.po
							../RTTR/languages/rttr-nl.po
							../RTTR/languages/rttr-et.po
							../RTTR/languages/rttr-en_GB.po
							../RTTR/languages/rttr-sk.po
							../RTTR/languages/rttr-nds.po
							../RTTR/languages/rttr-tr.po
							../RTTR/languages/rttr-pl.po
							../RTTR/languages/rttr-pt.po
							../RTTR/languages/rttr-he.po
							../RTTR/languages/rttr-cs.po
							../RTTR/languages/rttr-hu.po
							../RTTR/languages/rttr-it.po
							../RTTR/languages/rttr-sl.po
							../RTTR/languages/rttr-nb.po
						   )

################################################################################

# bzip linkerbug-fix
SET(SOURCES_BZIP "")
IF ( "${COMPILEFOR}" STREQUAL "windows" )
	IF( IS_DIRECTORY "${CMAKE_SOURCE_DIR}/contrib/bzip2-1.0.3" )
		SET(SOURCES_BZIP
			${CMAKE_SOURCE_DIR}/contrib/bzip2-1.0.3/blocksort.c
			${CMAKE_SOURCE_DIR}/contrib/bzip2-1.0.3/huffman.c
			${CMAKE_SOURCE_DIR}/contrib/bzip2-1.0.3/crctable.c
			${CMAKE_SOURCE_DIR}/contrib/bzip2-1.0.3/randtable.c
			${CMAKE_SOURCE_DIR}/contrib/bzip2-1.0.3/compress.c
			${CMAKE_SOURCE_DIR}/contrib/bzip2-1.0.3/decompress.c
			${CMAKE_SOURCE_DIR}/contrib/bzip2-1.0.3/bzlib.c
		)
	ENDIF( IS_DIRECTORY "${CMAKE_SOURCE_DIR}/contrib/bzip2-1.0.3" )
ENDIF ( "${COMPILEFOR}" STREQUAL "windows" )

INCLUDE(AddFileDependencies)
ADD_FILE_DEPENDENCIES(../build_version.cpp ${CMAKE_BINARY_DIR}/build_version.h)

SET(s25client_SRCS
	../build_version.cpp
	${SOURCES_CONTROLS}
	${SOURCES_DESKTOPS}
	${SOURCES_GL}
	${SOURCES_BZIP}
	${SOURCES_INGAMEWINDOWS}
	${SOURCES_OTHER}
	${SOURCES_AI}
	${SOURCES_NO}
	${SOURCES_NOB}
	${SOURCES_NOF}
)

IF ( "${COMPILEFOR}" STREQUAL "windows")
	ADD_DEFINITIONS("-DAFX_TARG_DEU")

	IF ( "${COMPILEARCH}" STREQUAL "i386" )
		FIND_PROGRAM(WINDRES NAMES windres i686-pc-mingw32-windres i686-mingw32-windres i586-mingw32msvc-windres DOC "path to mingw's windres executable")
	ELSE ( "${COMPILEARCH}" STREQUAL "i386" )
		FIND_PROGRAM(WINDRES NAMES windres x86_64-pc-mingw32-windres x86_64-mingw32-windres amd64-mingw32msvc-windres DOC "path to mingw's windres executable")
	ENDIF ( "${COMPILEARCH}" STREQUAL "i386" )

	# Add Icon to Application
	ADD_CUSTOM_COMMAND(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/s25client.res.o
					   COMMAND ${WINDRES} -I${CMAKE_CURRENT_SOURCE_DIR}/../win32/ -i${CMAKE_CURRENT_SOURCE_DIR}/../win32/s25client.rc -o ${CMAKE_CURRENT_BINARY_DIR}/s25client.res.o)

	SET(s25client_SRCS ${s25client_SRCS} ${CMAKE_CURRENT_BINARY_DIR}/s25client.res.o)

	ADD_FLAGS(CMAKE_EXE_LINKER_FLAGS -Wl,--stack,8388608)
ENDIF ( "${COMPILEFOR}" STREQUAL "windows")

IF ( "${COMPILEFOR}" STREQUAL "apple")
	CORRECT_LIB(OPENGL_gl_LIBRARY OpenGL)
	CORRECT_LIB(SDL_LIBRARY SDL)

	# Add the SDL-include flags to an apple build
	ADD_FLAGS(CMAKE_C_FLAGS -I${SDL_INCLUDE_DIR})
	ADD_FLAGS(CMAKE_CXX_FLAGS -I${SDL_INCLUDE_DIR})

	# Add library
	ADD_FLAGS(CMAKE_EXE_LINKER_FLAGS -framework OpenGL -L${CMAKE_SOURCE_DIR}/macos -lSDLmain)
ENDIF ( "${COMPILEFOR}" STREQUAL "apple")

################################################################################
# LUA
################################################################################

SET(LUA_VERSION "52")

IF ( "${COMPILEFOR}" STREQUAL "windows")
	IF ( "${COMPILEARCH}" STREQUAL "i386" )
		SET(LUA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../contrib/lua/win32")
	ELSE ( "${COMPILEARCH}" STREQUAL "i386" )
		SET(LUA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../contrib/lua/win64")
	ENDIF ( "${COMPILEARCH}" STREQUAL "i386" )
ENDIF ( "${COMPILEFOR}" STREQUAL "windows")

IF ( "${COMPILEFOR}" STREQUAL "linux")
	IF ( "${COMPILEARCH}" STREQUAL "i386" )
		SET(LUA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../contrib/lua/lin32")
	ELSE ( "${COMPILEARCH}" STREQUAL "i386" )
		SET(LUA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../contrib/lua/lin64")
	ENDIF ( "${COMPILEARCH}" STREQUAL "i386" )
ENDIF ( "${COMPILEFOR}" STREQUAL "linux")

IF ( "${COMPILEFOR}" STREQUAL "apple")
	SET(LUA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../contrib/lua/mac")
ENDIF ( "${COMPILEFOR}" STREQUAL "apple")

SET(LUA_LIB "${LUA_DIR}/liblua${LUA_VERSION}.a")

MESSAGE(STATUS "Checking LUA library at ${LUA_LIB}")
IF ( NOT EXISTS "${LUA_LIB}" )
	SET(LUA_LIB "/usr/lib/liblua5.2.a")
	ADD_FLAGS(CMAKE_C_FLAGS -I/usr/include/lua5.2)
	ADD_FLAGS(CMAKE_CXX_FLAGS -I/usr/include/lua5.2)
ENDIF ( NOT EXISTS "${LUA_LIB}" )

IF ( NOT EXISTS "${LUA_LIB}" )
	SET(LUA_LIB "/usr/lib/liblua.so")
	ADD_FLAGS(CMAKE_C_FLAGS -I/usr/include/)
	ADD_FLAGS(CMAKE_CXX_FLAGS -I/usr/include/)
ENDIF ( NOT EXISTS "${LUA_LIB}" )

MESSAGE(STATUS "Using LUA library from ${LUA_LIB}")

ADD_FLAGS(CMAKE_C_FLAGS -I${LUA_DIR}/include)
ADD_FLAGS(CMAKE_CXX_FLAGS -I${LUA_DIR}/include)

################################################################################

ADD_EXECUTABLE(s25client ${s25client_SRCS})
TARGET_LINK_LIBRARIES(s25client
	siedler2
	lobby_c
	s25util
	mygettext
	${BZIP2_LIBRARIES}
	pthread
	${OPENGL_gl_LIBRARY}
	${LUA_LIB}
)

IF ( "${COMPILEFOR}" STREQUAL "apple")
	TARGET_LINK_LIBRARIES(s25client ${SDL_LIBRARY} SDLmain)
	SET_TARGET_PROPERTIES(s25client PROPERTIES LINK_FLAGS "-framework Cocoa")
ENDIF ( "${COMPILEFOR}" STREQUAL "apple")

IF ( "${COMPILEFOR}" STREQUAL "windows")
	TARGET_LINK_LIBRARIES(s25client ole32 ws2_32 shlwapi bz2 bfd iberty imagehlp ${TARGET_LINK_LIBRARIES} )
ENDIF ( "${COMPILEFOR}" STREQUAL "windows")

IF ( "${COMPILEFOR}" STREQUAL "linux")
	TARGET_LINK_LIBRARIES(s25client dl ${TARGET_LINK_LIBRARIES} )
ENDIF ( "${COMPILEFOR}" STREQUAL "linux")

INSTALL(TARGETS s25client RUNTIME DESTINATION ${BINDIR})

################################################################################
