#include "tracklistcomponent.h"

#include <cmath>

namespace
{
    constexpr float pluginSlotHeight = 12.0f;
    constexpr float pluginSlotGap = 7.0f;
    constexpr float pluginAddGap = 8.0f;
    constexpr float pluginAddHeight = 12.0f;

    juce::Colour getTrackPanelColour()
    {
        return juce::Colour(0xff2b3f92);
    }

    juce::Colour getTrackEdgeColour()
    {
        return juce::Colour(0xff9db7ff).withAlpha(0.24f);
    }
}

TrackListComponent::TrackListComponent(DAWState& stateIn)
    : state(stateIn)
{
}

void TrackListComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::transparentBlack);

    for (int rowIndex = 0; rowIndex < state.trackCount; ++rowIndex)
    {
        auto rowBounds = getRowBounds(rowIndex);
        if (rowBounds.getBottom() < 0.0f || rowBounds.getY() > static_cast<float>(getHeight()))
            continue;

        drawMixerStrip(g, rowBounds, state.getTrackName(rowIndex), rowIndex);
    }

    if (getMaxScroll() > 0)
    {
        g.setColour(juce::Colours::white.withAlpha(0.12f));
        g.fillRoundedRectangle(getScrollbarTrackBounds(), 4.0f);

        g.setColour(juce::Colour(0xffd3e3ff).withAlpha(0.86f));
        g.fillRoundedRectangle(getScrollbarThumbBounds(), 4.0f);
    }
}

void TrackListComponent::mouseDown(const juce::MouseEvent& e)
{
    if (getScrollbarThumbBounds().contains(e.position))
    {
        draggingScrollbar = true;
        scrollbarDragOffset = e.position.y - getScrollbarThumbBounds().getY();
        return;
    }

    const int trackIndex = getTrackIndexAt(e.position);
    if (trackIndex < 0)
        return;

    state.selectTrack(trackIndex);
    repaint();

    if (e.mods.isPopupMenu())
    {
        showTrackContextMenu(trackIndex, getRowBounds(trackIndex).toNearestInt());
        return;
    }

    if (getRemoveButtonBounds(trackIndex).contains(e.position))
    {
        if (onRemoveTrackRequested)
            onRemoveTrackRequested(trackIndex);
        return;
    }

    for (int buttonIndex = 0; buttonIndex < 3; ++buttonIndex)
    {
        if (getButtonBounds(trackIndex, buttonIndex).contains(e.position))
        {
            if (buttonIndex == 0)
                state.toggleTrackMuted(trackIndex);
            else if (buttonIndex == 1)
                state.toggleTrackSoloed(trackIndex);
            else
                state.toggleTrackArmed(trackIndex);

            repaint();
            return;
        }
    }

    for (int slotIndex = 0; slotIndex < state.getTrackFxSlotCount(trackIndex); ++slotIndex)
    {
        if (getPluginSlotBounds(trackIndex, slotIndex).contains(e.position))
        {
            showFxSlotMenu(trackIndex, slotIndex);
            return;
        }
    }

    if (state.canAddTrackFxSlot(trackIndex) && getAddFxSlotBounds(trackIndex).contains(e.position))
    {
        state.addTrackFxSlot(trackIndex);
        repaint();
        return;
    }

    if (getPanKnobBounds(trackIndex).contains(e.position))
    {
        activeDragTarget = DragTarget::pan;
        dragTrackIndex = trackIndex;
        updatePanFromPosition(trackIndex, e.position);
        return;
    }

    if (getFaderBounds(trackIndex).contains(e.position))
    {
        activeDragTarget = DragTarget::level;
        dragTrackIndex = trackIndex;
        updateLevelFromPosition(trackIndex, e.position);
    }
}

void TrackListComponent::mouseDoubleClick(const juce::MouseEvent& e)
{
    const int trackIndex = getTrackIndexAt(e.position);
    if (trackIndex < 0)
        return;

    if (getHeaderBounds(trackIndex).contains(e.position)
        && !getRemoveButtonBounds(trackIndex).contains(e.position))
    {
        promptRenameTrack(trackIndex);
    }
}

void TrackListComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (draggingScrollbar)
    {
        const auto trackBounds = getScrollbarTrackBounds();
        const auto thumbBounds = getScrollbarThumbBounds();
        const float maxThumbY = trackBounds.getBottom() - thumbBounds.getHeight();
        const float thumbY = juce::jlimit(trackBounds.getY(), maxThumbY, e.position.y - scrollbarDragOffset);
        const float proportion = (thumbY - trackBounds.getY()) / juce::jmax(1.0f, maxThumbY - trackBounds.getY());
        setScrollOffset(static_cast<int>(std::round(proportion * static_cast<float>(getMaxScroll()))));
        return;
    }

    if (dragTrackIndex < 0)
        return;

    if (activeDragTarget == DragTarget::pan)
    {
        updatePanFromPosition(dragTrackIndex, e.position);
        return;
    }

    if (activeDragTarget == DragTarget::level)
        updateLevelFromPosition(dragTrackIndex, e.position);
}

void TrackListComponent::mouseUp(const juce::MouseEvent&)
{
    draggingScrollbar = false;
    activeDragTarget = DragTarget::none;
    dragTrackIndex = -1;
}

void TrackListComponent::mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
    if (wheel.deltaY != 0.0f)
        scrollBy(-wheel.deltaY * 112.0f);
}

int TrackListComponent::getContentHeight() const
{
    return juce::jmax(rowHeight, state.trackCount * rowHeight + juce::jmax(0, state.trackCount - 1) * rowGap);
}

int TrackListComponent::getMaxScroll() const
{
    return juce::jmax(0, getContentHeight() - getHeight());
}

int TrackListComponent::toContentY(float y) const
{
    return static_cast<int>(std::floor(y + static_cast<float>(scrollOffset)));
}

int TrackListComponent::getTrackIndexAt(juce::Point<float> point) const
{
    const int rowIndex = toContentY(point.y) / (rowHeight + rowGap);
    return rowIndex >= 0 && rowIndex < state.trackCount ? rowIndex : -1;
}

juce::Rectangle<float> TrackListComponent::getCardInnerBounds(int trackIndex) const
{
    return getRowBounds(trackIndex).reduced(16.0f, 14.0f);
}

juce::Rectangle<float> TrackListComponent::getHeaderBounds(int trackIndex) const
{
    auto bounds = getCardInnerBounds(trackIndex);
    return bounds.removeFromTop(28.0f);
}

juce::Rectangle<float> TrackListComponent::getRemoveButtonBounds(int trackIndex) const
{
    auto headerBounds = getHeaderBounds(trackIndex);
    return headerBounds.removeFromRight(28.0f);
}

juce::Rectangle<float> TrackListComponent::getControlButtonsBounds(int trackIndex) const
{
    auto bounds = getCardInnerBounds(trackIndex);
    bounds.removeFromTop(36.0f);
    auto topCluster = bounds.removeFromTop(58.0f);
    return topCluster.removeFromLeft(156.0f);
}

juce::Rectangle<float> TrackListComponent::getButtonBounds(int trackIndex, int buttonIndex) const
{
    auto row = getControlButtonsBounds(trackIndex).withHeight(28.0f);
    for (int i = 0; i < buttonIndex; ++i)
    {
        row.removeFromLeft(38.0f);
        row.removeFromLeft(10.0f);
    }

    return row.removeFromLeft(38.0f);
}

juce::Rectangle<float> TrackListComponent::getPanKnobBounds(int trackIndex) const
{
    auto bounds = getCardInnerBounds(trackIndex);
    bounds.removeFromTop(36.0f);
    auto topCluster = bounds.removeFromTop(58.0f);
    topCluster.removeFromLeft(156.0f);
    return topCluster.removeFromLeft(74.0f).reduced(2.0f);
}

juce::Rectangle<float> TrackListComponent::getFaderBounds(int trackIndex) const
{
    auto bounds = getCardInnerBounds(trackIndex);
    bounds.removeFromTop(36.0f);
    bounds.removeFromTop(58.0f);
    bounds.removeFromTop(84.0f);
    bounds.removeFromTop(12.0f);
    auto lowerCluster = bounds;
    lowerCluster.removeFromLeft(20.0f);
    lowerCluster.removeFromLeft(12.0f);
    return lowerCluster.reduced(2.0f, 0.0f);
}

juce::Rectangle<float> TrackListComponent::getPluginLaneBounds(int trackIndex) const
{
    auto bounds = getCardInnerBounds(trackIndex);
    bounds.removeFromTop(36.0f);
    bounds.removeFromTop(58.0f);
    return bounds.removeFromTop(84.0f);
}

juce::Rectangle<float> TrackListComponent::getPluginSlotBounds(int trackIndex, int slotIndex) const
{
    auto slotArea = getPluginLaneBounds(trackIndex).reduced(14.0f, 10.0f);
    slotArea.removeFromTop(22.0f);

    for (int i = 0; i < slotIndex; ++i)
    {
        slotArea.removeFromTop(pluginSlotHeight);
        slotArea.removeFromTop(pluginSlotGap);
    }

    return slotArea.removeFromTop(pluginSlotHeight);
}

juce::Rectangle<float> TrackListComponent::getAddFxSlotBounds(int trackIndex) const
{
    auto slotArea = getPluginLaneBounds(trackIndex).reduced(14.0f, 10.0f);
    slotArea.removeFromTop(22.0f);

    const int slotCount = state.getTrackFxSlotCount(trackIndex);
    for (int i = 0; i < slotCount; ++i)
    {
        slotArea.removeFromTop(pluginSlotHeight);
        if (i < slotCount - 1)
            slotArea.removeFromTop(pluginSlotGap);
    }

    slotArea.removeFromTop(pluginAddGap);
    return slotArea.removeFromTop(pluginAddHeight);
}

juce::Rectangle<float> TrackListComponent::getScrollbarTrackBounds() const
{
    return { static_cast<float>(getWidth() - scrollbarWidth), 2.0f, static_cast<float>(scrollbarWidth - 2), static_cast<float>(getHeight() - 4) };
}

juce::Rectangle<float> TrackListComponent::getScrollbarThumbBounds() const
{
    const auto trackBounds = getScrollbarTrackBounds();
    if (getMaxScroll() <= 0)
        return {};

    const float visibleRatio = static_cast<float>(getHeight()) / static_cast<float>(getContentHeight());
    const float thumbHeight = juce::jlimit(28.0f, trackBounds.getHeight(), trackBounds.getHeight() * visibleRatio);
    const float scrollRatio = static_cast<float>(scrollOffset) / static_cast<float>(getMaxScroll());
    const float thumbY = trackBounds.getY() + (trackBounds.getHeight() - thumbHeight) * scrollRatio;
    return { trackBounds.getX(), thumbY, trackBounds.getWidth(), thumbHeight };
}

juce::Rectangle<float> TrackListComponent::getRowBounds(int rowIndex) const
{
    const float contentWidth = juce::jmax(0.0f, static_cast<float>(getWidth() - scrollbarWidth - 12));
    const float y = static_cast<float>(rowIndex * (rowHeight + rowGap) - scrollOffset);
    return { 8.0f, y, contentWidth, static_cast<float>(rowHeight) };
}

void TrackListComponent::drawMixerStrip(juce::Graphics& g,
                                        juce::Rectangle<float> bounds,
                                        const juce::String& title,
                                        int trackIndex)
{
    auto cardBounds = bounds.reduced(0.0f, 2.0f);
    const bool isSelected = state.isTrackSelected(trackIndex);
    const bool isAudible = state.isTrackAudible(trackIndex);
    auto panelColour = isSelected ? juce::Colour(0xff314ba9) : getTrackPanelColour();
    if (!isAudible)
        panelColour = panelColour.darker(0.25f).withMultipliedAlpha(0.88f);
    g.setColour(panelColour);
    g.fillRoundedRectangle(cardBounds, 24.0f);

    auto edgeColour = isSelected ? juce::Colour(0xffd7e3ff).withAlpha(0.65f) : getTrackEdgeColour();
    if (!isAudible)
        edgeColour = edgeColour.withAlpha(0.22f);
    g.setColour(edgeColour);
    g.drawRoundedRectangle(cardBounds.reduced(1.5f), 24.0f, isSelected ? 2.4f : 2.0f);

    auto innerBounds = cardBounds.reduced(16.0f, 14.0f);
    auto headerBounds = innerBounds.removeFromTop(28.0f);

    g.setColour(juce::Colours::white.withAlpha(isAudible ? 0.82f : 0.5f));
    g.setFont(juce::Font(15.0f, juce::Font::bold));
    g.drawText(title,
               headerBounds.withTrimmedRight(36.0f).toNearestInt(),
               juce::Justification::centredLeft,
               false);

    auto removeBounds = getRemoveButtonBounds(trackIndex);
    g.setColour(juce::Colour(0xff23357f).withMultipliedAlpha(isAudible ? 1.0f : 0.6f));
    g.fillRoundedRectangle(removeBounds, 8.0f);
    g.setColour(juce::Colours::white.withAlpha(isAudible ? 0.75f : 0.45f));
    g.setFont(juce::Font(18.0f, juce::Font::bold));
    g.drawText("-", removeBounds.toNearestInt(), juce::Justification::centred, false);

    innerBounds.removeFromTop(8.0f);

    auto topCluster = innerBounds.removeFromTop(58.0f);
    auto fxBounds = innerBounds.removeFromTop(84.0f);
    innerBounds.removeFromTop(12.0f);
    auto lowerCluster = innerBounds;

    auto buttonsBounds = topCluster.removeFromLeft(156.0f);
    auto knobBounds = topCluster.removeFromLeft(74.0f).reduced(2.0f);

    drawTransportButtons(g, buttonsBounds, trackIndex);
    drawPanKnob(g, knobBounds, trackIndex);
    drawPluginLane(g, fxBounds, trackIndex);

    auto labelArea = lowerCluster.removeFromTop(14.0f);
    lowerCluster.removeFromTop(8.0f);
    auto meterLane = lowerCluster.removeFromLeft(20.0f).reduced(0.0f, 4.0f);
    lowerCluster.removeFromLeft(12.0f);
    auto faderLane = lowerCluster.reduced(2.0f, 0.0f);
    g.setColour(juce::Colours::white.withAlpha(isAudible ? 0.48f : 0.28f));
    g.setFont(juce::Font(11.0f, juce::Font::plain));
    g.drawText("LEVEL",
               labelArea.toNearestInt(),
               juce::Justification::centredLeft,
               false);

    drawMeter(g, meterLane, trackIndex);
    drawFader(g, faderLane, trackIndex);
}

void TrackListComponent::drawTransportButtons(juce::Graphics& g, juce::Rectangle<float> bounds, int trackIndex) const
{
    const char* labels[] = { "M", "S", "R" };
    const auto& trackState = state.getTrackMixerState(trackIndex);
    const bool isAudible = state.isTrackAudible(trackIndex);

    auto row = bounds.withHeight(28.0f);
    for (int i = 0; i < 3; ++i)
    {
        auto button = row.removeFromLeft(38.0f);
        row.removeFromLeft(10.0f);

        auto fill = juce::Colour(0xff4460cf);
        auto border = juce::Colours::white.withAlpha(0.72f);
        auto text = juce::Colours::white.withAlpha(0.9f);

        const bool isActive = (i == 0 && trackState.muted)
                           || (i == 1 && trackState.soloed)
                           || (i == 2 && trackState.armed);

        if (isActive)
        {
            fill = i == 2 ? juce::Colour(0xffc2485d) : juce::Colour(0xff3a7afe);
            border = juce::Colours::white.withAlpha(0.95f);
            text = juce::Colours::white;
        }
        else
        {
            fill = juce::Colour(0xff2f437e);
            border = juce::Colours::white.withAlpha(0.58f);
            text = juce::Colours::white.withAlpha(0.82f);
        }

        if (!isAudible && !isActive)
        {
            fill = fill.darker(0.18f).withMultipliedAlpha(0.82f);
            border = border.withAlpha(border.getFloatAlpha() * 0.7f);
            text = text.withAlpha(text.getFloatAlpha() * 0.68f);
        }

        g.setColour(fill);
        g.fillRoundedRectangle(button, 9.0f);
        g.setColour(border);
        g.drawRoundedRectangle(button, 9.0f, 1.2f);
        g.setFont(juce::Font(12.0f, juce::Font::bold));
        g.setColour(text);
        g.drawText(labels[i], button.toNearestInt(), juce::Justification::centred, false);
    }

    auto routeBounds = bounds.withY(bounds.getBottom() - 18.0f).withHeight(18.0f);
    g.setColour(juce::Colours::white.withAlpha(isAudible ? 0.42f : 0.24f));
    g.setFont(juce::Font(10.0f, juce::Font::plain));
    g.drawText("IN 1  ->  BUS A", routeBounds.toNearestInt(), juce::Justification::centredLeft, false);
}

void TrackListComponent::drawPanKnob(juce::Graphics& g, juce::Rectangle<float> bounds, int trackIndex) const
{
    const float pan = state.getTrackMixerState(trackIndex).pan;
    const bool isAudible = state.isTrackAudible(trackIndex);
    g.setColour(juce::Colour(0xff182447).withMultipliedAlpha(isAudible ? 1.0f : 0.72f));
    g.fillEllipse(bounds);
    g.setColour(juce::Colours::white.withAlpha(isAudible ? 0.9f : 0.58f));
    g.drawEllipse(bounds, 1.6f);
    const float angle = juce::jmap(pan, -1.0f, 1.0f, -juce::MathConstants<float>::pi * 0.75f, juce::MathConstants<float>::pi * 0.75f);
    const auto endPoint = juce::Point<float>(bounds.getCentreX() + std::sin(angle) * 12.0f,
                                             bounds.getCentreY() - std::cos(angle) * 12.0f);
    g.drawLine(bounds.getCentreX(), bounds.getCentreY(), endPoint.x, endPoint.y, 2.0f);
}

void TrackListComponent::drawMeter(juce::Graphics& g, juce::Rectangle<float> bounds, int trackIndex) const
{
    const auto& trackState = state.getTrackMixerState(juce::jlimit(0, state.trackCount - 1, trackIndex));
    auto laneBounds = bounds.reduced(1.0f, 1.0f);
    g.setColour(juce::Colours::white.withAlpha(0.14f));
    g.fillRoundedRectangle(laneBounds, 5.0f);

    float fillRatio = trackState.level * (state.isTrackAudible(trackIndex) ? 0.95f : 0.08f);
    if (trackState.soloed && state.isTrackAudible(trackIndex))
        fillRatio = juce::jmin(1.0f, fillRatio + 0.08f);
    fillRatio = juce::jlimit(0.03f, 0.98f, fillRatio);
    auto fillBounds = laneBounds.withY(laneBounds.getBottom() - laneBounds.getHeight() * fillRatio)
                                .withHeight(laneBounds.getHeight() * fillRatio);

    juce::ColourGradient gradient(juce::Colour(0xff3bf38d), fillBounds.getCentreX(), fillBounds.getBottom(),
                                  juce::Colour(0xffffd34a), fillBounds.getCentreX(), fillBounds.getY(), false);
    gradient.addColour(0.78, juce::Colour(0xffff8847));
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(fillBounds, 5.0f);
}

void TrackListComponent::drawFader(juce::Graphics& g, juce::Rectangle<float> bounds, int trackIndex) const
{
    const auto& trackState = state.getTrackMixerState(trackIndex);
    const bool isAudible = state.isTrackAudible(trackIndex);
    auto content = bounds.reduced(2.0f, 0.0f);
    auto valueBounds = content.removeFromTop(14.0f);
    content.removeFromTop(8.0f);
    auto lane = content.withHeight(26.0f).withCentre({ content.getCentreX(), content.getY() + 13.0f });

    g.setColour(juce::Colours::white.withAlpha(isAudible ? 0.46f : 0.28f));
    g.setFont(juce::Font(10.0f, juce::Font::plain));
    const auto dbValue = juce::jmap(trackState.level, 0.0f, 1.0f, -60.0f, 6.0f);
    const auto dbText = juce::String(static_cast<int>(std::round(dbValue))) + " dB";
    g.drawText(dbText,
               valueBounds.toNearestInt(),
               juce::Justification::centredLeft,
               false);

    g.setColour(juce::Colours::white.withAlpha(isAudible ? 0.08f : 0.05f));
    g.fillRoundedRectangle(lane, 13.0f);
    g.setColour(juce::Colours::white.withAlpha(isAudible ? 0.18f : 0.11f));
    g.drawRoundedRectangle(lane, 13.0f, 1.1f);

    auto rail = lane.reduced(12.0f, 10.0f);
    g.setColour(juce::Colour(0xff172344).withMultipliedAlpha(isAudible ? 1.0f : 0.68f));
    g.fillRoundedRectangle(rail, 3.0f);

    const float thumbWidth = 20.0f;
    const float thumbTravel = juce::jmax(0.0f, rail.getWidth() - thumbWidth);
    const float thumbX = rail.getX() + thumbTravel * trackState.level;
    auto thumb = juce::Rectangle<float>(thumbX, lane.getCentreY() - 9.0f, thumbWidth, 18.0f);
    g.setColour(juce::Colour(0xffdce8ff).withMultipliedAlpha(isAudible ? 1.0f : 0.72f));
    g.fillRoundedRectangle(thumb, 6.0f);

    g.setColour(juce::Colour(0xffeef4ff).withAlpha(isAudible ? 0.8f : 0.5f));
    g.drawLine(thumb.getCentreX(), thumb.getY() + 3.0f, thumb.getCentreX(), thumb.getBottom() - 3.0f, 1.1f);
}

void TrackListComponent::drawPluginLane(juce::Graphics& g, juce::Rectangle<float> bounds, int trackIndex) const
{
    const bool isAudible = state.isTrackAudible(trackIndex);
    g.setColour(juce::Colours::white.withAlpha(isAudible ? 0.08f : 0.05f));
    g.fillRoundedRectangle(bounds, 18.0f);
    g.setColour(juce::Colours::white.withAlpha(isAudible ? 0.26f : 0.16f));
    g.drawRoundedRectangle(bounds, 18.0f, 1.2f);

    auto labelBounds = bounds.reduced(14.0f, 10.0f).removeFromTop(16.0f);
    g.setColour(juce::Colours::white.withAlpha(isAudible ? 0.58f : 0.34f));
    g.setFont(juce::Font(10.5f, juce::Font::plain));
    g.drawText("FX SLOTS", labelBounds.toNearestInt(), juce::Justification::centredLeft, false);

    auto slotArea = bounds.reduced(14.0f, 10.0f);
    slotArea.removeFromTop(22.0f);

    const int slotCount = state.getTrackFxSlotCount(trackIndex);
    for (int i = 0; i < slotCount; ++i)
    {
        auto slot = slotArea.removeFromTop(pluginSlotHeight);
        if (i < slotCount - 1)
            slotArea.removeFromTop(pluginSlotGap);
        const auto& fxSlot = state.getTrackFxSlot(trackIndex, i);
        g.setColour(juce::Colours::white.withAlpha(isAudible ? 0.08f : 0.05f));
        g.fillRoundedRectangle(slot, 7.0f);
        g.setColour(fxSlot.bypassed ? juce::Colour(0xffffd34a).withAlpha(0.7f)
                                    : juce::Colours::white.withAlpha(isAudible ? 0.2f : 0.12f));
        g.drawRoundedRectangle(slot, 7.0f, 1.0f);

        if (fxSlot.hasPlugin)
        {
            g.setColour(juce::Colours::white.withAlpha(isAudible ? 0.72f : 0.44f));
            g.setFont(juce::Font(9.0f, juce::Font::plain));
            g.drawText(fxSlot.bypassed ? fxSlot.name + " (Bypassed)" : fxSlot.name,
                       slot.reduced(6.0f, 0.0f).toNearestInt(),
                       juce::Justification::centredLeft,
                       false);
        }
        else
        {
            g.setColour(juce::Colours::white.withAlpha(isAudible ? 0.34f : 0.22f));
            g.setFont(juce::Font(9.0f, juce::Font::plain));
            g.drawText("Add plugin",
                       slot.reduced(6.0f, 0.0f).toNearestInt(),
                       juce::Justification::centredLeft,
                       false);
        }
    }

    if (state.canAddTrackFxSlot(trackIndex))
    {
        slotArea.removeFromTop(pluginAddGap);
        auto addSlot = slotArea.removeFromTop(pluginAddHeight);
        g.setColour(juce::Colour(0xff3a7afe).withAlpha(isAudible ? 0.24f : 0.16f));
        g.fillRoundedRectangle(addSlot, 7.0f);
        g.setColour(juce::Colours::white.withAlpha(isAudible ? 0.46f : 0.28f));
        g.drawRoundedRectangle(addSlot, 7.0f, 1.0f);
        g.setFont(juce::Font(8.5f, juce::Font::bold));
        g.drawText("+ FX Slot", addSlot.toNearestInt(), juce::Justification::centred, false);
    }
}

void TrackListComponent::scrollBy(float deltaY)
{
    setScrollOffset(scrollOffset + static_cast<int>(std::round(deltaY)));
}

void TrackListComponent::setScrollOffset(int newOffset)
{
    scrollOffset = juce::jlimit(0, getMaxScroll(), newOffset);
    repaint();
}

void TrackListComponent::promptRenameTrack(int trackIndex)
{
    auto* renameWindow = new juce::AlertWindow("Rename Track",
                                               "Enter a track name.",
                                               juce::AlertWindow::NoIcon);
    renameWindow->addTextEditor("trackName", state.getTrackName(trackIndex), "Track Name");
    renameWindow->addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey));
    renameWindow->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

    juce::Component::SafePointer<TrackListComponent> safeThis(this);
    juce::Component::SafePointer<juce::AlertWindow> safeWindow(renameWindow);
    renameWindow->enterModalState(true,
                                  juce::ModalCallbackFunction::create([safeThis, safeWindow, trackIndex](int result)
                                  {
                                      if (result != 1 || safeThis == nullptr || safeWindow == nullptr)
                                          return;

                                      if (auto* editor = safeWindow->getTextEditor("trackName"))
                                      {
                                          safeThis->state.setTrackName(trackIndex, editor->getText());
                                          safeThis->repaint();
                                      }
                                  }),
                                  true);
}

void TrackListComponent::showFxSlotMenu(int trackIndex, int slotIndex)
{
    juce::PopupMenu menu;
    const auto& fxSlot = state.getTrackFxSlot(trackIndex, slotIndex);

    if (!fxSlot.hasPlugin)
    {
        juce::PopupMenu addPluginMenu;
        const auto availablePlugins = getAvailableTrackPlugins != nullptr
            ? getAvailableTrackPlugins()
            : juce::StringArray{};
        for (int i = 0; i < availablePlugins.size(); ++i)
            addPluginMenu.addItem(100 + i, availablePlugins[i]);

        if (availablePlugins.isEmpty())
            addPluginMenu.addItem(1, "No plugins available", false);

        menu.addSubMenu("Add Plugin", addPluginMenu);
        if (state.canRemoveTrackFxSlot(trackIndex))
        {
            menu.addSeparator();
            menu.addItem(4, "Remove FX Slot");
        }
    }
    else
    {
        menu.addItem(3, "Open Plugin Editor");
        menu.addSeparator();
        menu.addItem(1, fxSlot.bypassed ? "Enable Plugin" : "Bypass Plugin");
        menu.addItem(2, "Remove Plugin");
        menu.addSeparator();
        menu.addItem(4, "Remove FX Slot", state.canRemoveTrackFxSlot(trackIndex));
    }

    auto target = getPluginSlotBounds(trackIndex, slotIndex).toNearestInt();
    juce::Component::SafePointer<TrackListComponent> safeThis(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea(localAreaToGlobal(target)),
                       [safeThis, trackIndex, slotIndex, hasPlugin = fxSlot.hasPlugin](int result)
                       {
                           if (safeThis == nullptr || result == 0)
                               return;

                           if (!hasPlugin && result >= 100)
                           {
                               const auto availablePlugins = safeThis->getAvailableTrackPlugins != nullptr
                                   ? safeThis->getAvailableTrackPlugins()
                                   : juce::StringArray{};
                               const int pluginIndex = result - 100;
                               if (juce::isPositiveAndBelow(pluginIndex, availablePlugins.size())
                                   && safeThis->onTrackPluginLoadRequested != nullptr
                                   && safeThis->onTrackPluginLoadRequested(trackIndex, slotIndex, availablePlugins[pluginIndex]))
                               {
                                   safeThis->state.loadTrackPluginIntoFxSlot(trackIndex, slotIndex, availablePlugins[pluginIndex]);
                               }
                           }
                           else if (hasPlugin && result == 3)
                           {
                               if (safeThis->onTrackPluginEditorRequested != nullptr)
                                   safeThis->onTrackPluginEditorRequested(trackIndex, slotIndex);
                           }
                           else if (hasPlugin && result == 1)
                           {
                               const bool shouldBeBypassed = !safeThis->state.getTrackFxSlot(trackIndex, slotIndex).bypassed;
                               if (safeThis->onTrackPluginBypassRequested != nullptr)
                                   safeThis->onTrackPluginBypassRequested(trackIndex, slotIndex, shouldBeBypassed);
                               safeThis->state.toggleTrackFxSlotBypassed(trackIndex, slotIndex);
                           }
                           else if (hasPlugin && result == 2)
                           {
                               if (safeThis->onTrackPluginRemoveRequested != nullptr)
                                   safeThis->onTrackPluginRemoveRequested(trackIndex, slotIndex);
                               safeThis->state.clearTrackFxSlot(trackIndex, slotIndex);
                           }
                           else if (result == 4)
                           {
                               if (safeThis->onTrackPluginSlotRemoveRequested != nullptr)
                                   safeThis->onTrackPluginSlotRemoveRequested(trackIndex, slotIndex);
                               safeThis->state.removeTrackFxSlot(trackIndex, slotIndex);
                           }

                           safeThis->repaint();
                       });
}

void TrackListComponent::showTrackContextMenu(int trackIndex, juce::Rectangle<int> targetBounds)
{
    juce::PopupMenu menu;
    menu.addItem(1, "Rename Track");
    menu.addSeparator();
    menu.addItem(2, "Arm Exclusively");
    menu.addItem(3, "Disarm All Tracks");
    menu.addSeparator();
    menu.addItem(4, "Delete Track", state.trackCount > 1);

    juce::Component::SafePointer<TrackListComponent> safeThis(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea(localAreaToGlobal(targetBounds)),
                       [safeThis, trackIndex](int result)
                       {
                           if (safeThis == nullptr || result == 0)
                               return;

                           if (result == 1)
                           {
                               safeThis->promptRenameTrack(trackIndex);
                           }
                           else if (result == 2)
                           {
                               safeThis->state.armOnlyTrack(trackIndex);
                               safeThis->repaint();
                           }
                           else if (result == 3)
                           {
                               safeThis->state.disarmAllTracks();
                               safeThis->repaint();
                           }
                           else if (result == 4)
                           {
                               if (safeThis->onRemoveTrackRequested)
                                   safeThis->onRemoveTrackRequested(trackIndex);
                           }
                       });
}

void TrackListComponent::updatePanFromPosition(int trackIndex, juce::Point<float> position)
{
    auto knobBounds = getPanKnobBounds(trackIndex);
    const float normalised = juce::jlimit(0.0f, 1.0f, (position.x - knobBounds.getX()) / juce::jmax(1.0f, knobBounds.getWidth()));
    state.setTrackPan(trackIndex, juce::jmap(normalised, 0.0f, 1.0f, -1.0f, 1.0f));
    repaint();
}

void TrackListComponent::updateLevelFromPosition(int trackIndex, juce::Point<float> position)
{
    auto faderBounds = getFaderBounds(trackIndex);
    const float normalised = juce::jlimit(0.0f, 1.0f, (position.x - faderBounds.getX()) / juce::jmax(1.0f, faderBounds.getWidth()));
    state.setTrackLevel(trackIndex, normalised);
    repaint();
}
