rttr:RegisterTranslations(
{
    en =
    {
        name = 'Roman campaign',
        shortDescription = 'Roman campaign',
        longDescription= 'We are in a large dessert with low amount of water and few space for farming. We need to hunt for wild and use our ore mines to produce gold and exchange it for goods we can not produce.'
    },
    de =
    {
        name = 'Römische Kampagne',
        shortDescription = 'Römische Kampagne',
        longDescription= 'Wir sind in einer grossen Wüste mit wenig Wasser und wenig Platz für Ackerbau. Wir müssen nach Wild jagen und unsere Minen nutzen um Gold zu produzieren und es gegen Güter tauschen, die wir nicht selber herstellen können.'
    }
})

rttr:AddCampaign{
    version= 1,
    author= "Bluebyte",
    name = _"name",
    shortDescription = _"shortDescription",
    longDescription = _"longDescription",
    maxHumanPlayers= 1,
    mapFolder = "<RTTR_GAME>/DATA/MAPS",
    luaFolder = "<RTTR_RTTR>/CAMPAIGNS/ROMAN",
    maps = { "MISS200.WLD","MISS201.WLD","MISS202.WLD","MISS203.WLD","MISS204.WLD","MISS205.WLD","MISS206.WLD","MISS207.WLD","MISS208.WLD","MISS209.WLD"}
}
