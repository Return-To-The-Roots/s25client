---- Campaign lua version ------
function getRequiredLuaVersion()
    return 1
end

rttr:RegisterTranslations(
{
    en =
    {
        name = 'Roman campaign',
        shortDescription = 'Roman campaign',
        longDescription= 'Diary of Octavius:\n\n\nFourth Day After Shipwreck.\n\n\nYesterday all the survivors met in order to discuss\nthe situation. Because there is no prospect of\nearly rescue, we decided to settle on this foreign\nisland. The items salvaged from the ship will be a\ngreat help to us. The most important thing is to\nmake use of the raw materials on the island in order\nto establish a settlement. We urgently need \naccommodations for a woodcutter, carpenter and \nstonemason.'
    },
    de =
    {
        name = 'Römische Kampagne',
        shortDescription = 'Römische Kampagne',
        longDescription= 'Tagebuch des Octavius:\n\n\nVierter Tag nach dem Schiffbruch.\n\n\nGestern fanden sich alle Überlebenden zusammen, um\ndie Lage zu beraten. Da keine Aussicht auf eine\nbaldige Rettung besteht, beschlossen wir, uns auf\ndieser fremden Insel niederzulassen. Dabei werden\nuns die geretteten Güter aus dem Schiff eine große\nHilfe sein. Das Wichtigste ist, die Rohstoffe der\nInsel für den Aufbau einer Siedlung nutzbar zu\nmachen. Wir benötigen dringend Unterkünfte für einen\nHolzfäller, Schreiner und den Steinmetz.'
    }
})

campaign = {
    version = 1,
    author = "Bluebyte",
    name = _"name",
    shortDescription = _"shortDescription",
    longDescription = _"longDescription",
    image = "<RTTR_GAME>/GFX/PICS/INSTALL.LBM",
    maxHumanPlayers = 1,
    difficulty = "easy",
    mapFolder = "<RTTR_GAME>/DATA/MAPS",
    luaFolder = "<RTTR_RTTR>/CAMPAIGNS/ROMAN",
    maps = { "MISS200.WLD","MISS201.WLD","MISS202.WLD","MISS203.WLD","MISS204.WLD","MISS205.WLD","MISS206.WLD","MISS207.WLD","MISS208.WLD","MISS209.WLD"}
}
