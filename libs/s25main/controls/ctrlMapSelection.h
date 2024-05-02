// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "DrawPoint.h"
#include "Window.h"
#include "ogl/glArchivItem_Bitmap.h"
#include <gameData/SelectionMapInputData.h>

class glArchivItem_Bitmap;

struct MissionStatus
{
    bool playable = false;
    bool occupied = false;
};

class ctrlMapSelection : public Window
{
public:
    ctrlMapSelection(Window* parent, unsigned id, const DrawPoint& pos, const Extent& size,
                     const SelectionMapInputData& inputData);
    ~ctrlMapSelection() override;

    void setMissionsStatus(const std::vector<MissionStatus>& status);
    void setSelection(size_t select);
    int getCurrentSelection() const;
    void setPreview(bool previewOnly);

    bool Msg_LeftUp(const MouseCoords& mc) override;

protected:
    bool IsMouseOver(const Position& mousePos) const;
    void Draw_() override;

    glArchivItem_Bitmap* createEnabledMask(const Extent& extent);
    void updateEnabledMask();

    float getScaleFactor();
    template<class Type>
    Type scale(const Type& scaleIt)
    {
        return Type(scaleIt * getScaleFactor());
    }
    DrawPoint invertScale(const DrawPoint& scaleIt);

    DrawPoint getBackgroundPosition();
    DrawPoint getMapOffsetRelativeToBackground();
    DrawPoint getMapPosition();
    DrawPoint getScaledImageOriginOffset(glArchivItem_Bitmap* bitmap);

    void drawImageOnMap(glArchivItem_Bitmap* image, const Position& drawPos);

    struct MapImages
    {
        glArchivItem_Bitmap* background;
        glArchivItem_Bitmap* map;
        glArchivItem_Bitmap* missionMapMask;
        glArchivItem_Bitmap* marker;
        glArchivItem_Bitmap* conquered;
        glArchivItem_Bitmap* enabledMask;
        std::unique_ptr<libsiedler2::baseArchivItem_Bitmap> enabledMaskMemory;
        bool isValid() const { return map && background && missionMapMask && conquered && marker && enabledMask; };
    };

    MapImages mapImages;
    SelectionMapInputData inputData;
    std::vector<MissionStatus> missionStatus;
    Position currentSelectionPos;
    bool preview;
};
