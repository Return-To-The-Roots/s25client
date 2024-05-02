// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ctrlMapSelection.h"
#include "CollisionDetection.h"
#include "Loader.h"
#include "RttrConfig.h"
#include "driver/MouseCoords.h"
#include "helpers/Range.h"
#include "ogl/glArchivItem_Bitmap.h"
#include <libsiedler2/ArchivItem_Bitmap.h>
#include <libsiedler2/ColorBGRA.h>
#include <libsiedler2/IAllocator.h>
#include <libsiedler2/libsiedler2.h>
#include <helpers/format.hpp>

ctrlMapSelection::ctrlMapSelection(Window* parent, unsigned id, const DrawPoint& pos, const Extent& size,
                                   const SelectionMapInputData& inputData)
    : Window(parent, id, pos, size), inputData(inputData), missionStatus(inputData.missionSelectionInfos.size()),
      preview(false)
{
    if(!LOADER.LoadFiles({inputData.background.filePath.string(), inputData.map.filePath.string(),
                          inputData.missionMapMask.filePath.string(), inputData.marker.filePath.string(),
                          inputData.conquered.filePath.string()}))
        throw std::runtime_error("Loading of images for map failed.");

    auto getImage = [](const ImageResource& res) {
        return LOADER.GetImageN(ResourceId::make(res.filePath), res.index);
    };

    mapImages.background = getImage(inputData.background);
    mapImages.map = getImage(inputData.map);
    mapImages.missionMapMask = getImage(inputData.missionMapMask);
    mapImages.marker = getImage(inputData.marker);
    mapImages.conquered = getImage(inputData.conquered);
    mapImages.enabledMask = createEnabledMask(mapImages.missionMapMask->GetSize());

    if(!mapImages.isValid())
        throw std::runtime_error("Setup of images for map failed");

    updateEnabledMask();
}

ctrlMapSelection::~ctrlMapSelection() = default;

void ctrlMapSelection::updateEnabledMask()
{
    for(const auto x : helpers::range(mapImages.enabledMask->getWidth()))
    {
        for(const auto y : helpers::range(mapImages.enabledMask->getHeight()))
        {
            const auto pixelColor = mapImages.missionMapMask->getPixel(x, y);
            const auto matchingMission =
              std::find_if(inputData.missionSelectionInfos.begin(), inputData.missionSelectionInfos.end(),
                           [&pixelColor](const auto& val) { return val.maskAreaColor == pixelColor.asValue(); });

            auto const index = std::distance(inputData.missionSelectionInfos.begin(), matchingMission);
            if(matchingMission == inputData.missionSelectionInfos.end() || missionStatus[index].playable)
            {
                mapImages.enabledMask->setPixel(x, y, libsiedler2::ColorBGRA());
            } else
                mapImages.enabledMask->setPixel(x, y, libsiedler2::ColorBGRA(inputData.disabledColor));
        }
    }
}

void ctrlMapSelection::setMissionsStatus(const std::vector<MissionStatus>& status)
{
    if(inputData.missionSelectionInfos.size() != status.size())
        throw std::runtime_error(
          helpers::format("List has wrong size. %1% != %2%", inputData.missionSelectionInfos.size(), status.size()));

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

int ctrlMapSelection::getCurrentSelection() const
{
    const auto match = std::find_if(inputData.missionSelectionInfos.begin(), inputData.missionSelectionInfos.end(),
                                    [&](const auto& val) { return val.ankerPos == currentSelectionPos; });
    return match != inputData.missionSelectionInfos.end() ?
             static_cast<int>(std::distance(inputData.missionSelectionInfos.begin(), match)) :
             -1;
}

void ctrlMapSelection::setPreview(bool previewOnly)
{
    preview = previewOnly;
}

bool ctrlMapSelection::Msg_LeftUp(const MouseCoords& mc)
{
    if(!preview && IsMouseOver(mc.GetPos()))
    {
        const auto pickPos = invertScale(mc.GetPos() - getMapPosition());

        const auto pixelColor =
          mapImages.missionMapMask->getPixel(std::max(0, std::min(pickPos.x, (int)(mapImages.map->GetSize().x - 1))),
                                             std::max(0, std::min(pickPos.y, (int)(mapImages.map->GetSize().y - 1))));

        const auto matchingMission =
          std::find_if(inputData.missionSelectionInfos.begin(), inputData.missionSelectionInfos.end(),
                       [&pixelColor](const auto& val) { return val.maskAreaColor == pixelColor.asValue(); });

        if(matchingMission != inputData.missionSelectionInfos.end())
        {
            setSelection(std::distance(inputData.missionSelectionInfos.begin(), matchingMission));
            GetParent()->Msg_ButtonClick(GetID());
        }
        return true;
    }
    return false;
}

glArchivItem_Bitmap* ctrlMapSelection::createEnabledMask(const Extent& extent)
{
    auto enabledMask =
      libsiedler2::getAllocator().create<libsiedler2::baseArchivItem_Bitmap>(libsiedler2::BobType::Bitmap);
    enabledMask->init(extent.x, extent.y, libsiedler2::TextureFormat::BGRA);
    auto* res = dynamic_cast<glArchivItem_Bitmap*>(enabledMask.get());
    RTTR_Assert(!enabledMask.get() || res);
    mapImages.enabledMaskMemory = std::move(enabledMask);
    return res;
}

bool ctrlMapSelection::IsMouseOver(const Position& mousePos) const
{
    return IsPointInRect(mousePos, GetDrawRect());
}

float ctrlMapSelection::getScaleFactor()
{
    const auto ratio = PointF(GetSize()) / mapImages.background->GetSize();
    return ratio.x < ratio.y ? ratio.x : ratio.y;
}

DrawPoint ctrlMapSelection::invertScale(const DrawPoint& scaleIt)
{
    return DrawPoint(scaleIt / getScaleFactor());
}

DrawPoint ctrlMapSelection::getBackgroundPosition()
{
    return GetDrawPos() + (GetSize() - scale(mapImages.background->GetSize())) / 2;
}

DrawPoint ctrlMapSelection::getMapOffsetRelativeToBackground()
{
    return scale(inputData.mapOffsetInBackground);
}

DrawPoint ctrlMapSelection::getMapPosition()
{
    return getBackgroundPosition() + getMapOffsetRelativeToBackground();
}

DrawPoint ctrlMapSelection::getScaledImageOriginOffset(glArchivItem_Bitmap* bitmap)
{
    return bitmap->GetOrigin() - scale(bitmap->GetOrigin());
}

void ctrlMapSelection::drawImageOnMap(glArchivItem_Bitmap* image, const Position& drawPos)
{
    image->DrawFull(
      Rect(getMapPosition() + scale(drawPos) + getScaledImageOriginOffset(image), scale(image->GetSize())));
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
        if(!missionStatus[idx].occupied)
            continue;

        drawImageOnMap(mapImages.conquered, inputData.missionSelectionInfos[idx].ankerPos);
    }

    if(!preview && currentSelectionPos.isValid())
    {
        drawImageOnMap(mapImages.marker, currentSelectionPos);
    }
}
