// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ctrlMapSelection.h"
#include "CollisionDetection.h"
#include "Loader.h"
#include "RttrForeachPt.h"
#include "driver/MouseCoords.h"
#include "helpers/Range.h"
#include "helpers/containerUtils.h"
#include "helpers/format.hpp"
#include "mygettext/mygettext.h"
#include "ogl/glArchivItem_Bitmap.h"
#include <libsiedler2/ArchivItem_Bitmap.h>
#include <libsiedler2/ColorBGRA.h>
#include <libsiedler2/IAllocator.h>
#include <libsiedler2/libsiedler2.h>
#include <algorithm>

ctrlMapSelection::MapImages::MapImages(const SelectionMapInputData& data)
{
    auto getImage = [](const ImageResource& res) {
        auto* img = LOADER.GetImageN(ResourceId::make(res.filePath), res.index);
        if(!img)
            throw std::runtime_error(
              helpers::format(_("Loading of images %s for map selection failed."), res.filePath));
        return img;
    };

    {
        std::vector<std::string> pathsToLoad;
        for(const auto& res : {data.background, data.map, data.missionMapMask, data.marker, data.conquered})
            pathsToLoad.push_back(res.filePath.string());
        LOADER.LoadFiles(pathsToLoad);
    }

    background = getImage(data.background);
    map = getImage(data.map);
    missionMapMask = getImage(data.missionMapMask);
    marker = getImage(data.marker);
    conquered = getImage(data.conquered);
    if(map->GetSize() != missionMapMask->GetSize())
        throw std::runtime_error(_("Map and mission mask have different sizes."));

    enabledMaskMemory =
      libsiedler2::getAllocator().create<libsiedler2::baseArchivItem_Bitmap>(libsiedler2::BobType::Bitmap);
    enabledMask = dynamic_cast<glArchivItem_Bitmap*>(enabledMaskMemory.get());
    RTTR_Assert(enabledMask);
    enabledMask->init(missionMapMask->getWidth(), missionMapMask->getHeight(), libsiedler2::TextureFormat::BGRA);
}

ctrlMapSelection::ctrlMapSelection(Window* parent, unsigned id, const DrawPoint& pos, const Extent& size,
                                   const SelectionMapInputData& inputData)
    : Window(parent, id, pos, size), mapImages(inputData), inputData(inputData),
      missionStatus(inputData.missionSelectionInfos.size()), preview(false)
{
    updateEnabledMask();
}

ctrlMapSelection::~ctrlMapSelection() = default;

void ctrlMapSelection::updateEnabledMask()
{
    RTTR_Assert(mapImages.enabledMask->GetSize() == mapImages.missionMapMask->GetSize());
    const libsiedler2::ColorBGRA disabledColor(inputData.disabledColor);

    RTTR_FOREACH_PT(Point<uint16_t>, mapImages.enabledMask->GetSize())
    {
        const auto pixelColor = mapImages.missionMapMask->getPixel(pt.x, pt.y).asValue();
        const auto matchingMissionIdx = helpers::indexOf_if(
          inputData.missionSelectionInfos, [pixelColor](const auto& val) { return val.maskAreaColor == pixelColor; });

        libsiedler2::ColorBGRA newColor;
        if(matchingMissionIdx >= 0 && !missionStatus[matchingMissionIdx].playable)
            newColor = disabledColor;
        mapImages.enabledMask->setPixel(pt.x, pt.y, newColor);
    }
}

void ctrlMapSelection::setMissionsStatus(const std::vector<MissionStatus>& status)
{
    if(inputData.missionSelectionInfos.size() != status.size())
    {
        throw std::runtime_error(
          helpers::format("List has wrong size. %1% != %2%", inputData.missionSelectionInfos.size(), status.size()));
    }

    missionStatus = status;
    updateEnabledMask();
}

void ctrlMapSelection::setSelection(size_t select)
{
    if(select < inputData.missionSelectionInfos.size())
    {
        if(missionStatus[select].playable)
            currentSelectionPos = inputData.missionSelectionInfos[select].ankerPos;
    } else
        currentSelectionPos = Position::Invalid();
}

std::optional<unsigned> ctrlMapSelection::getSelection() const
{
    if(!currentSelectionPos.isValid())
        return std::nullopt;
    const auto result = helpers::indexOf_if(inputData.missionSelectionInfos,
                                            [currentSelectionPos = this->currentSelectionPos](const auto& val) {
                                                return val.ankerPos == currentSelectionPos;
                                            });
    RTTR_Assert(result >= 0);
    return static_cast<unsigned>(result);
}

void ctrlMapSelection::setPreview(bool previewOnly)
{
    preview = previewOnly;
}

bool ctrlMapSelection::Msg_LeftUp(const MouseCoords& mc)
{
    if(!preview && IsMouseOver(mc))
    {
        const auto pickPos = invertScale(mc.GetPos() - getMapPosition());

        const auto pixelColor = mapImages.missionMapMask
                                  ->getPixel(helpers::clamp(pickPos.x, 0u, mapImages.map->GetSize().x - 1),
                                             helpers::clamp(pickPos.y, 0u, mapImages.map->GetSize().y - 1))
                                  .asValue();

        const auto matchingMissionIdx = helpers::indexOf_if(
          inputData.missionSelectionInfos, [pixelColor](const auto& val) { return val.maskAreaColor == pixelColor; });

        if(matchingMissionIdx >= 0)
        {
            setSelection(matchingMissionIdx);
            GetParent()->Msg_ButtonClick(GetID());
        }
        return true;
    }
    return false;
}

float ctrlMapSelection::getScaleFactor()
{
    const auto ratio = PointF(GetSize()) / mapImages.background->GetSize();
    return std::min(ratio.x, ratio.y);
}

DrawPoint ctrlMapSelection::invertScale(const DrawPoint& scaleIt)
{
    return DrawPoint(scaleIt / getScaleFactor());
}

DrawPoint ctrlMapSelection::getBackgroundPosition()
{
    return GetDrawPos() + (GetSize() - scale(mapImages.background->GetSize())) / 2;
}

DrawPoint ctrlMapSelection::getMapPosition()
{
    return getBackgroundPosition() + scale(inputData.mapOffsetInBackground);
}

void ctrlMapSelection::drawImageOnMap(glArchivItem_Bitmap* image, const Position& drawPos)
{
    const auto originCorrection = image->GetOrigin() - scale(image->GetOrigin());
    image->DrawFull(Rect(getMapPosition() + scale(drawPos) + originCorrection, scale(image->GetSize())));
}

void ctrlMapSelection::Draw_()
{
    Window::Draw_();

    mapImages.background->DrawFull(Rect(getBackgroundPosition(), scale(mapImages.background->GetSize())));

    const Rect mapRect = Rect(getMapPosition(), scale(mapImages.map->GetSize()));
    mapImages.map->DrawFull(mapRect);
    mapImages.enabledMask->DrawFull(mapRect);

    for(const auto idx : helpers::range(missionStatus.size()))
    {
        if(missionStatus[idx].conquered)
            drawImageOnMap(mapImages.conquered, inputData.missionSelectionInfos[idx].ankerPos);
    }

    if(!preview && currentSelectionPos.isValid())
        drawImageOnMap(mapImages.marker, currentSelectionPos);
}
