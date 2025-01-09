---- Campaign lua version ------
function getRequiredLuaVersion()
    return 2
end

rttr:RegisterTranslations(
{
    en =
    {
        name = 'World campaign',
        shortDescription = 'Original world campaign',
        longDescription= 'The original world campaign from the gold edition.\n\nYou have to conquer the whole earth.'
    },
    de =
    {
        name = 'Welt Kampagne',
        shortDescription = 'Orginale Welt Kampagne',
        longDescription= 'Die orginale Welt Kampagne aus der Gold Edition.\n\nErobere die ganze Welt.'
    }
})

campaign = {
    version = 1,
    author = "Bluebyte",
    name = _"name",
    shortDescription = _"shortDescription",
    longDescription = _"longDescription",
    image = "<RTTR_GAME>/GFX/PICS/WORLD.LBM",
    maxHumanPlayers= 1,
    difficulty = "easy",
    mapFolder = "<RTTR_GAME>/DATA/MAPS2",
    luaFolder = "",
    maps = { "EUROPE.WLD","NAMERICA.WLD","SAMERICA.WLD","GREEN.WLD","AFRICA.WLD","NASIA.WLD","SASIA.WLD","JAPAN.WLD","AUSTRA.WLD"},
    selectionMap = {
        background = {"<RTTR_GAME>/GFX/PICS/SETUP990.LBM", 0},
        map = {"<RTTR_GAME>/GFX/PICS/WORLD.LBM", 0},
        missionMapMask = {"<RTTR_GAME>/GFX/PICS/WORLDMSK.LBM", 0},
        marker = {"<RTTR_GAME>/DATA/IO/IO.DAT", 231},
        conquered = {"<RTTR_GAME>/DATA/IO/IO.DAT", 232},
        backgroundOffset = {64, 70},
        disabledColor = 0x70000000,
        missionSelectionInfos = {
            {0xffffff00, 243, 97},
            {0xffaf73cb, 55,78},
            {0xff008fc3, 122, 193},
            {0xff43c373, 166, 36},
            {0xff27871b, 241,176},
            {0xffc32323, 366,87},
            {0xff573327, 375,145},
            {0xffcfaf4b, 486, 136},
            {0xffbb6313, 441, 264}
        }
    }
}
