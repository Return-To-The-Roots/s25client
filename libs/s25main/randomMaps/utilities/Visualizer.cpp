// Copyright (c) 2017 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "rttrDefines.h" // IWYU pragma: keep
#include "randomMaps/utilities/Visualizer.h"
#include "randomMaps/utilities/BitmapWriter.h"

unsigned char Visualizer::RedFromTexture(TextureType texture)
{
    switch (texture)
    {
        case CoastToGreen2: return 188;
        case Mountain1: return 210;
        case MountainPeak: return 255;
        case Coast: return 245;
        case Water: return 0;
        case Grass1: return 0;
        case Grass2: return 0;
        case Grass3: return 0;
        case Mountain2: return 205;
        case Mountain3: return 160;
        case Mountain4: return 139;
        case GrassFlower: return 0;
        case Lava: return 255;
        case GrassToMountain: return 155;
        default: return 0;
    }
}

unsigned char Visualizer::GreenFromTexture(TextureType texture)
{
    switch (texture)
    {
        case CoastToGreen2: return 255;
        case Mountain1: return 180;
        case MountainPeak: return 255;
        case Coast: return 222;
        case Water: return 0;
        case Grass1: return 255;
        case Grass2: return 255;
        case Grass3: return 255;
        case Mountain2: return 133;
        case Mountain3: return 82;
        case Mountain4: return 69;
        case GrassFlower: return 255;
        case Lava: return 0;
        case GrassToMountain: return 155;
        default: return 0;
    }
}

unsigned char Visualizer::BlueFromTexture(TextureType texture)
{
    switch (texture)
    {
        case CoastToGreen2: return 0;
        case Mountain1: return 140;
        case MountainPeak: return 255;
        case Coast: return 179;
        case Water: return 255;
        case Grass1: return 0;
        case Grass2: return 0;
        case Grass3: return 10;
        case Mountain2: return 63;
        case Mountain3: return 45;
        case Mountain4: return 19;
        case GrassFlower: return 0;
        case Lava: return 0;
        case GrassToMountain: return 155;
        default: return 0;
    }
}

Bitmap Visualizer::From(const std::vector<double>& points, int width, int height)
{
    std::vector<unsigned char> rgb(points.size()*3);
    
    for (int x = 0; x < width; x++)
    {
        for (int y = 0; y < height; y++)
        {
            int i = x + y * width;
            auto color = (unsigned char) (points[i] * 255.0);
            rgb[i*3]   = color;
            rgb[i*3+1] = color;
            rgb[i*3+2] = color;
        }
    }
    
    Bitmap bitmap(width, height, rgb);
    return bitmap;
}

Bitmap Visualizer::From(const std::vector<unsigned char>& points, int width, int height)
{
    std::vector<unsigned char> rgb(points.size()*3);
    
    for (int x = 0; x < width; x++)
    {
        for (int y = 0; y < height; y++)
        {
            int i = x + y * width;
            rgb[i*3]   = points[i];
            rgb[i*3+1] = points[i];
            rgb[i*3+2] = points[i];
        }
    }
    
    Bitmap bitmap(width, height, rgb);
    return bitmap;
}

Bitmap Visualizer::From(const std::vector<bool>& points, int width, int height)
{
    std::vector<unsigned char> rgb(points.size()*3);
    
    for (int x = 0; x < width; x++)
    {
        for (int y = 0; y < height; y++)
        {
            int i = x + y * width;
            rgb[i*3]   = points[i] ? 50 : 0;
            rgb[i*3+1] = points[i] ? 50 : 0;
            rgb[i*3+2] = points[i] ? 255 : 0;
        }
    }
    
    Bitmap bitmap(width, height, rgb);
    return bitmap;
}

Bitmap Visualizer::From(const std::vector<unsigned char>& z,
                        const std::vector<bool>& water, int width, int height)
{
    std::vector<unsigned char> rgb(z.size() * 3);
    
    for (int x = 0; x < width; x++)
    {
        for (int y = 0; y < height; y++)
        {
            int i = x + y * width;
            rgb[i*3] =   water[i] ? 0 : z[i];
            rgb[i*3+1] = water[i] ? 0 : z[i];
            rgb[i*3+2] = water[i] ? 255 : z[i];
        }
    }
    
    Bitmap bitmap(width, height, rgb);
    return bitmap;
}

Bitmap Visualizer::From(const std::vector<unsigned char>& z,
                               const std::vector<TextureType>& texture,
                               const HeightSettings& settings,
                               int width, int height)
{
    std::vector<unsigned char> rgb(z.size() * 3);
    
    for (int x = 0; x < width; x++)
    {
        for (int y = 0; y < height; y++)
        {
            int i = x + y * width;
            double scale = 0.5 + ((z[i] - settings.minimum) / double(settings.maximum - settings.minimum)) / 2;
            
            rgb[i*3] = char(RedFromTexture(texture[i]) * scale);
            rgb[i*3+1] = char(GreenFromTexture(texture[i]) * scale);
            rgb[i*3+2] = char(BlueFromTexture(texture[i]) * scale);
        }
    }
    
    Bitmap bitmap(width, height, rgb);
    return bitmap;
}

Bitmap Visualizer::From(const IntSet& rsu, const IntSet& lsd, int width, int height)
{
    std::vector<unsigned char> rgb(width * height * 3);
    
    for (int x = 0; x < width; x++)
    {
        for (int y = 0; y < height; y++)
        {
            int i = x + y * width;
            bool isRsu = rsu.find(i) != rsu.end();
            bool isLsd = lsd.find(i) != lsd.end();
            
            rgb[i*3]   = isRsu ? 255 : 0;
            rgb[i*3+1] = isLsd ? 255 : 0;
            rgb[i*3+2] = 0;
        }
    }
    
    Bitmap bitmap(width, height, rgb);
    return bitmap;
}

void Visualizer::Write(const Bitmap& bitmap, std::string filename)
{
    bitmap_image image(bitmap.width, bitmap.height);
    
    for (int x = 0; x < bitmap.width; x++)
    {
        for (int y = 0; y < bitmap.height; y++)
        {
            int i = x + y * bitmap.width;
            image.set_pixel(x, y,
                            bitmap.rgb_data[i*3],
                            bitmap.rgb_data[i*3+1],
                            bitmap.rgb_data[i*3+2]);
        }
    }
    
    image.save_image(filename);
}
