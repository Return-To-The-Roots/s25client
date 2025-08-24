// Copyright (C) 2024 - 2025  Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "DrawPoint.h"
#include "Window.h"
#include <gameData/SelectionMapInputData.h>
#include <optional>

class glArchivItem_Bitmap;
namespace libsiedler2 {
class baseArchivItem_Bitmap;
} // namespace libsiedler2

struct MissionStatus
{
    bool playable = false;
    bool conquered = false;
};

class ctrlMapSelection : public Window
{
public:
    ctrlMapSelection(Window* parent, unsigned id, const DrawPoint& pos, const Extent& size,
                     const SelectionMapInputData& inputData);
    ~ctrlMapSelection() override;

    void setMissionsStatus(const std::vector<MissionStatus>& status);
    void setSelection(size_t select);
    std::optional<unsigned> getSelection() const;
    void setPreview(bool previewOnly);

    bool Msg_LeftUp(const MouseCoords& mc) override;

protected:
    void Draw_() override;

    void updateEnabledMask();

    float getScaleFactor();
    template<class Type>
    Type scale(const Type& scaleIt)
    {
        return Type(scaleIt * getScaleFactor());
    }
    DrawPoint invertScale(const DrawPoint& scaleIt);

    DrawPoint getBackgroundPosition();
    DrawPoint getMapPosition();

    void drawImageOnMap(glArchivItem_Bitmap* image, const Position& drawPos);

    struct MapImages
    {
        MapImages(const SelectionMapInputData& data);

        glArchivItem_Bitmap* background;
        glArchivItem_Bitmap* map;
        glArchivItem_Bitmap* missionMapMask;
        glArchivItem_Bitmap* marker;
        glArchivItem_Bitmap* conquered;
        glArchivItem_Bitmap* enabledMask;
        std::unique_ptr<libsiedler2::baseArchivItem_Bitmap> enabledMaskMemory;
    };

    const MapImages mapImages;
    SelectionMapInputData inputData;
    std::vector<MissionStatus> missionStatus;
    Position currentSelectionPos;
    bool preview;
};
