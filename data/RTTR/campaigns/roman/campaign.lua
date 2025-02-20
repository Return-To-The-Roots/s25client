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
    },    
    pl =
    {
        name = 'Kampania Rzymian',
        shortDescription = 'Kampania Rzymian',
		longDescription= 'Dziennik Oktawiusza:\n\n\nCzwarty dzień po rozbiciu statku.\n\n\nWczoraj wszyscy ocalali spotkali się, aby omówić sytuację.\n\nPonieważ nie ma perspektyw na szybką pomoc,\npostanowiliśmy osiedlić się na tej obcej wyspie.\n\nRzeczy uratowane ze statku będą dla nas wielką pomocą.\n\nNajważniejsze jest, by wykorzystać surowce z wyspy w celu założenia osady.\n\nPilnie potrzebujemy schronienia dla:\ndrwala, cieśli i kamieniarza.'
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
    luaFolder = "",
    maps = { "MISS200.WLD","MISS201.WLD","MISS202.WLD","MISS203.WLD","MISS204.WLD","MISS205.WLD","MISS206.WLD","MISS207.WLD","MISS208.WLD","MISS209.WLD"}
}
