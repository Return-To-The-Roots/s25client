# Random Map Generation

Random map generation is based on a combination of algorithms for height map generation, smoothing, texturing, object- and player placement. This is only a short overview of the folder structure and the final output of the random map generator. 

## Algorithm

For most random map types the high-level algorithm for map generation looks like this:
1. Generate a random height map
2. Smooth height map to get nice transitions
3. Compute the sea- and mountain level based on desired map coverage
4. Apply textures - based on height map, sea- and mountain-level
5. Add resources (trees, granite, animals, etc.) based on textures
6. Place harbor positions at the coastline
7. Place head quarters

## Folder structure

The top-level directory "randomMaps" contains the most high-level entry point to generate a random map and store it in the file system. All this requires is to create an instance of `MapGenerator` and call the `Create` method with a filepath to the output file where to store the generated map (natually with the ".swd" file extension) and the `MapSettings` which are usually setup through the UI.

The "algorithm" folder contains more general utilities and algorithmic functions used by several classes throughout the random map generation process. 

All other folders are more or less corresponding to one of the previously outlined algorithmic steps:
1. "elevation" directory contains classes used to compute, shape and smooth the height map
2. "objects" directory contains utilities to generate harbor and HQ positions
3. "resources" contains classes to place resoures on the map (mines, trees, granite, animals, etc.)
4. "terrain" folder contains classes around texturing of the map
5. "utilities" contains some utility classes to debug algorithm by writing data structures into bitmaps
6. "waters" is a special folder about island- and river generation

## Random Map Types

There're different types of random maps the generator can currently produce. 

*Continent* - "A large island, sometimes surounded by smaller islands. Players can build harbors, but they'll most likely stay and fight each other on the one main continent."

*Edgecase* - "Everything is surounded by mountains, the center of the world is a large lake. Rivers can make their way from the top of the mountain wall down to the lake. Players are placed around the lake in the middle."

*Migration* - "Each player starts on it's own island. But wait - we're running out of resources! Let's build a harbor and make our way to colonize the big central island!"

*Random* - "You don't know what's waiting for you!"

*Rivers* - "Mainly greenland crossed by a large river which may also contains little islands. Get your ass up, build a harbor and explore!"
