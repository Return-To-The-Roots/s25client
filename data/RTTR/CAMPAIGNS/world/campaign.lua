---- Campaign lua version ------
function getRequiredLuaVersion()
    return 1
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
    luaFolder = "<RTTR_RTTR>/CAMPAIGNS/WORLD",
    maps = { "AFRICA.WLD","AUSTRA.WLD","EUROPE.WLD","GREEN.WLD","JAPAN.WLD","NAMERICA.WLD","NASIA.WLD","SAMERICA.WLD","SASIA.WLD"}
}
