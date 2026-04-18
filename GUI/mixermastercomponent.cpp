#include "mixermastercomponent.h"

#include <cmath>

namespace
{
    constexpr float titleStripHeight = 26.0f;
    constexpr float titleStripGap = 8.0f;
    constexpr float headerControlHeight = 26.0f;
    constexpr float pluginSlotHeight = 12.0f;
    constexpr float pluginSlotGap = 7.0f;
    constexpr float pluginAddGap = 8.0f;
    constexpr float pluginAddHeight = 12.0f;

    juce::Colour getMasterPanelColour() { return juce::Colour(0xff2d4299); }
    juce::Colour getTrackFocusColour() { return juce::Colour(0xff2b3f92); }
    juce::Colour getPanelEdgeColour() { return juce::Colour(0xff9db7ff).withAlpha(0.24f); }

    juce::String getPanLabelText(float pan)
    {
        if (std::abs(pan) < 0.04f)
            return "C";

        const int amount = static_cast<int>(std::round(std::abs(pan) * 100.0f));
        return (pan < 0.0f ? "L" : "R") + juce::String(amount);
    }
}

MixerMasterComponent::MixerMasterComponent(DAWState& stateIn)
    : state(stateIn)
{
}

bool MixerMasterComponent::isShowingMaster() const
{
    return state.isMasterMixerFocused();
}

int MixerMasterComponent::getFocusedTrackIndex() const
{
    return juce::jlimit(0, state.trackCount - 1, state.selectedTrackIndex);
}

juce::String MixerMasterComponent::getFocusedTitle() const
{
    if (isShowingMaster())
        return "MASTER";

    return state.getTrackName(getFocusedTrackIndex());
}

juce::Rectangle<float> MixerMasterComponent::getInnerBounds() const
{
    return getLocalBounds().toFloat().reduced(20.0f, 18.0f);
}

juce::Rectangle<float> MixerMasterComponent::getHeaderMasterFocusBounds() const
{
    auto innerBounds = getInnerBounds();
    innerBounds.removeFromTop(titleStripHeight);
    innerBounds.removeFromTop(titleStripGap);
    auto headerBounds = innerBounds.removeFromTop(headerControlHeight);
    const bool patternMode = state.isTrackPatternMode(getFocusedTrackIndex()) && !isShowingMaster();
    auto actionArea = headerBounds.removeFromRight(patternMode ? 292.0f : 178.0f);

    if (patternMode)
    {
        actionArea.removeFromLeft(98.0f);
        actionArea.removeFromLeft(10.0f);
        actionArea.removeFromLeft(92.0f);
        actionArea.removeFromLeft(10.0f);
    }
    else
    {
        actionArea.removeFromLeft(98.0f);
        actionArea.removeFromLeft(10.0f);
    }

    return actionArea.removeFromLeft(74.0f);
}

juce::Rectangle<float> MixerMasterComponent::getHeaderModeBounds() const
{
    auto innerBounds = getInnerBounds();
    innerBounds.removeFromTop(titleStripHeight);
    innerBounds.removeFromTop(titleStripGap);
    auto headerBounds = innerBounds.removeFromTop(headerControlHeight);
    auto actionArea = headerBounds.removeFromRight(state.isTrackPatternMode(getFocusedTrackIndex()) && !isShowingMaster() ? 292.0f : 178.0f);
    return actionArea.removeFromLeft(98.0f);
}

juce::Rectangle<float> MixerMasterComponent::getHeaderInstrumentBounds() const
{
    if (isShowingMaster() || !state.isTrackPatternMode(getFocusedTrackIndex()))
        return {};

    auto innerBounds = getInnerBounds();
    innerBounds.removeFromTop(titleStripHeight);
    innerBounds.removeFromTop(titleStripGap);
    auto headerBounds = innerBounds.removeFromTop(headerControlHeight);
    auto actionArea = headerBounds.removeFromRight(292.0f);
    actionArea.removeFromLeft(98.0f);
    actionArea.removeFromLeft(10.0f);
    return actionArea.removeFromLeft(92.0f);
}

juce::Rectangle<float> MixerMasterComponent::getControlButtonsBounds() const
{
    auto innerBounds = getInnerBounds();
    innerBounds.removeFromTop(titleStripHeight + titleStripGap + headerControlHeight + titleStripGap);
    auto topRow = innerBounds.removeFromTop(58.0f);
    return topRow.removeFromLeft(isShowingMaster() ? 156.0f : 190.0f);
}

juce::Rectangle<float> MixerMasterComponent::getButtonBounds(int buttonIndex) const
{
    auto row = getControlButtonsBounds().withHeight(28.0f);

    if (isShowingMaster())
    {
        static constexpr float widths[] { 32.0f, 32.0f, 44.0f };
        for (int i = 0; i < buttonIndex; ++i)
        {
            row.removeFromLeft(widths[i]);
            row.removeFromLeft(6.0f);
        }

        return row.removeFromLeft(widths[buttonIndex]);
    }

    static constexpr float widths[] { 32.0f, 32.0f, 42.0f, 42.0f };
    for (int i = 0; i < buttonIndex; ++i)
    {
        row.removeFromLeft(widths[i]);
        row.removeFromLeft(6.0f);
    }

    return row.removeFromLeft(widths[buttonIndex]);
}

juce::Rectangle<float> MixerMasterComponent::getIoButtonBounds() const
{
    auto bounds = getControlButtonsBounds();
    bounds.removeFromTop(34.0f);
    return bounds.removeFromTop(18.0f).withTrimmedRight(isShowingMaster() ? 10.0f : 18.0f);
}

juce::Rectangle<float> MixerMasterComponent::getPanKnobBounds() const
{
    auto innerBounds = getInnerBounds();
    innerBounds.removeFromTop(titleStripHeight + titleStripGap + headerControlHeight + titleStripGap);
    auto topRow = innerBounds.removeFromTop(58.0f);
    topRow.removeFromLeft(isShowingMaster() ? 156.0f : 190.0f);
    return topRow.removeFromLeft(74.0f).reduced(2.0f);
}

juce::Rectangle<float> MixerMasterComponent::getPluginLaneBounds() const
{
    auto innerBounds = getInnerBounds();
    innerBounds.removeFromTop(titleStripHeight + titleStripGap + headerControlHeight + titleStripGap);
    innerBounds.removeFromTop(58.0f);
    innerBounds.removeFromTop(8.0f);
    return innerBounds.removeFromTop(78.0f);
}

juce::Rectangle<float> MixerMasterComponent::getPluginSlotBounds(int slotIndex) const
{
    auto slotArea = getPluginLaneBounds().reduced(14.0f, 10.0f);
    slotArea.removeFromTop(22.0f);

    for (int i = 0; i < slotIndex; ++i)
    {
        slotArea.removeFromTop(pluginSlotHeight);
        slotArea.removeFromTop(pluginSlotGap);
    }

    return slotArea.removeFromTop(pluginSlotHeight);
}

juce::Rectangle<float> MixerMasterComponent::getAddFxSlotBounds() const
{
    auto slotArea = getPluginLaneBounds().reduced(14.0f, 10.0f);
    slotArea.removeFromTop(22.0f);
    const int slotCount = isShowingMaster() ? state.getMasterFxSlotCount() : state.getTrackFxSlotCount(getFocusedTrackIndex());

    for (int i = 0; i < slotCount; ++i)
    {
        slotArea.removeFromTop(pluginSlotHeight);
        if (i < slotCount - 1)
            slotArea.removeFromTop(pluginSlotGap);
    }

    slotArea.removeFromTop(pluginAddGap);
    return slotArea.removeFromTop(pluginAddHeight);
}

juce::Rectangle<float> MixerMasterComponent::getFaderBounds() const
{
    auto innerBounds = getInnerBounds();
    innerBounds.removeFromTop(titleStripHeight + titleStripGap + headerControlHeight + titleStripGap);
    innerBounds.removeFromTop(58.0f);
    innerBounds.removeFromTop(8.0f);
    innerBounds.removeFromTop(78.0f);
    auto lowerArea = innerBounds;
    lowerArea.removeFromTop(8.0f);
    auto stripArea = lowerArea.withTrimmedBottom(4.0f);
    stripArea.removeFromLeft(18.0f);
    stripArea.removeFromLeft(12.0f);
    return stripArea.reduced(2.0f, 0.0f);
}

juce::String MixerMasterComponent::getTooltipForPosition(juce::Point<float> position) const
{
    if (!isShowingMaster() && getHeaderModeBounds().contains(position))
        return "Choose whether this track uses audio clips or patterns";

    if (!isShowingMaster() && state.isTrackPatternMode(getFocusedTrackIndex())
        && getHeaderInstrumentBounds().contains(position))
    {
        const auto& instrumentSlot = state.getTrackInstrumentSlot(getFocusedTrackIndex());
        if (instrumentSlot.hasPlugin)
            return "Instrument: " + instrumentSlot.name;

        return "Choose an instrument for this pattern track";
    }

    if (!isShowingMaster() && getHeaderMasterFocusBounds().contains(position))
        return "Jump to the master mixer";

    const int buttonCount = isShowingMaster() ? 3 : 4;
    for (int buttonIndex = 0; buttonIndex < buttonCount; ++buttonIndex)
    {
        if (!getButtonBounds(buttonIndex).contains(position))
            continue;

        if (isShowingMaster())
        {
            if (buttonIndex == 0)
                return "Mute the master output";
            if (buttonIndex == 1)
                return "Solo the master output";
            return "Arm the master output";
        }

        if (buttonIndex == 0)
            return "Mute this track";
        if (buttonIndex == 1)
            return "Solo this track";
        if (buttonIndex == 2)
            return "Toggle input monitoring for this track";
        return "Arm this track for recording";
    }

    if (getIoButtonBounds().contains(position))
        return isShowingMaster() ? "Choose the master output device"
                                 : "Choose this track's input and output assignments";

    if (getPanKnobBounds().contains(position))
        return isShowingMaster() ? "Master pan" : "Track pan";

    if (getPluginLaneBounds().contains(position))
    {
        const int slotCount = isShowingMaster() ? state.getMasterFxSlotCount() : state.getTrackFxSlotCount(getFocusedTrackIndex());
        for (int slotIndex = 0; slotIndex < slotCount; ++slotIndex)
        {
            if (!getPluginSlotBounds(slotIndex).contains(position))
                continue;

            const auto& fxSlot = isShowingMaster() ? state.getMasterFxSlot(slotIndex)
                                                   : state.getTrackFxSlot(getFocusedTrackIndex(), slotIndex);
            if (fxSlot.hasPlugin)
                return "FX slot " + juce::String(slotIndex + 1) + ": " + fxSlot.name;

            return "Add a plugin to FX slot " + juce::String(slotIndex + 1);
        }

        const bool canAdd = isShowingMaster() ? state.canAddMasterFxSlot()
                                              : state.canAddTrackFxSlot(getFocusedTrackIndex());
        if (canAdd && getAddFxSlotBounds().contains(position))
            return "Add another FX slot";
    }

    auto innerBounds = getInnerBounds();
    innerBounds.removeFromTop(titleStripHeight + titleStripGap + headerControlHeight + titleStripGap);
    innerBounds.removeFromTop(58.0f);
    innerBounds.removeFromTop(8.0f);
    innerBounds.removeFromTop(78.0f);
    auto lowerArea = innerBounds;
    lowerArea.removeFromTop(8.0f);
    auto stripArea = lowerArea.withTrimmedBottom(4.0f);
    auto meterBounds = stripArea.removeFromLeft(18.0f).reduced(0.0f, 2.0f);

    if (meterBounds.contains(position))
        return isShowingMaster() ? "Master level meter" : "Track level meter";

    if (getFaderBounds().contains(position))
        return isShowingMaster() ? "Master volume" : "Track volume";

    return {};
}

void MixerMasterComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::transparentBlack);

    auto bounds = getLocalBounds().toFloat().reduced(4.0f);
    g.setColour(isShowingMaster() ? getMasterPanelColour() : getTrackFocusColour());
    g.fillRoundedRectangle(bounds, 24.0f);

    g.setColour(getPanelEdgeColour());
    g.drawRoundedRectangle(bounds.reduced(1.5f), 24.0f, 2.0f);

    auto innerBounds = getInnerBounds();
    auto titleStripBounds = innerBounds.removeFromTop(titleStripHeight);
    innerBounds.removeFromTop(titleStripGap);
    auto headerBounds = innerBounds.removeFromTop(headerControlHeight);

    g.setColour(juce::Colours::white.withAlpha(0.08f));
    g.fillRoundedRectangle(titleStripBounds, 10.0f);
    g.setColour(juce::Colours::white.withAlpha(0.16f));
    g.drawRoundedRectangle(titleStripBounds, 10.0f, 1.0f);
    g.setColour(juce::Colours::white.withAlpha(0.88f));
    g.setFont(juce::Font(15.0f, juce::Font::bold));
    g.drawText(getFocusedTitle(),
               titleStripBounds.reduced(12.0f, 0.0f).toNearestInt(),
               juce::Justification::centredLeft,
               false);

    if (!isShowingMaster())
    {
        auto modeBounds = getHeaderModeBounds();
        const auto contentType = state.getTrackContentType(getFocusedTrackIndex());
        g.setColour(contentType == TrackMixerState::ContentType::pattern
                        ? juce::Colour(0xff2a5d94)
                        : juce::Colour(0xff3f4d7f));
        g.fillRoundedRectangle(modeBounds, 8.0f);
        g.setColour(juce::Colours::white.withAlpha(0.2f));
        g.drawRoundedRectangle(modeBounds, 8.0f, 1.0f);
        g.setColour(juce::Colours::white.withAlpha(0.84f));
        g.setFont(juce::Font(10.0f, juce::Font::bold));
        g.drawText(DAWState::getTrackContentTypeLabel(contentType) + " v",
                   modeBounds.toNearestInt(),
                   juce::Justification::centred,
                   false);

        if (contentType == TrackMixerState::ContentType::pattern)
        {
            const auto& instrumentSlot = state.getTrackInstrumentSlot(getFocusedTrackIndex());
            auto instrumentBounds = getHeaderInstrumentBounds();
            g.setColour(juce::Colour(0xff21406b).withAlpha(instrumentSlot.hasPlugin ? 0.92f : 0.62f));
            g.fillRoundedRectangle(instrumentBounds, 8.0f);
            g.setColour(instrumentSlot.bypassed ? juce::Colour(0xffffd34a).withAlpha(0.76f)
                                                : juce::Colours::white.withAlpha(0.18f));
            g.drawRoundedRectangle(instrumentBounds, 8.0f, 1.0f);
            g.setColour(instrumentSlot.hasPlugin ? juce::Colours::white.withAlpha(0.84f)
                                                 : juce::Colours::white.withAlpha(0.48f));
            g.setFont(juce::Font(9.2f, juce::Font::bold));
            const auto label = instrumentSlot.hasPlugin ? instrumentSlot.name.upToFirstOccurrenceOf(" (", false, false) : "Instrument";
            g.drawText(label,
                       instrumentBounds.reduced(6.0f, 0.0f).toNearestInt(),
                       juce::Justification::centred,
                       false);
        }

        auto actionBounds = getHeaderMasterFocusBounds();
        g.setColour(juce::Colour(0xff22346e));
        g.fillRoundedRectangle(actionBounds, 8.0f);
        g.setColour(juce::Colours::white.withAlpha(0.18f));
        g.drawRoundedRectangle(actionBounds, 8.0f, 1.0f);
        g.setColour(juce::Colours::white.withAlpha(0.72f));
        g.setFont(juce::Font(10.0f, juce::Font::bold));
        g.drawText("MASTER", actionBounds.toNearestInt(), juce::Justification::centred, false);
    }

    innerBounds.removeFromTop(titleStripGap);
    if (isShowingMaster())
        paintMasterView(g, innerBounds);
    else
        paintTrackView(g, innerBounds, getFocusedTrackIndex());
}

void MixerMasterComponent::mouseDown(const juce::MouseEvent& e)
{
    if (!isShowingMaster() && getHeaderModeBounds().contains(e.position))
    {
        showTrackContentTypeMenu();
        return;
    }

    if (!isShowingMaster() && state.isTrackPatternMode(getFocusedTrackIndex())
        && getHeaderInstrumentBounds().contains(e.position))
    {
        showInstrumentSlotMenu();
        return;
    }

    if (!isShowingMaster() && getHeaderMasterFocusBounds().contains(e.position))
    {
        state.showMasterMixerFocus();
        repaint();
        return;
    }

    const int slotCount = isShowingMaster() ? state.getMasterFxSlotCount() : state.getTrackFxSlotCount(getFocusedTrackIndex());
    for (int slotIndex = 0; slotIndex < slotCount; ++slotIndex)
    {
        if (getPluginSlotBounds(slotIndex).contains(e.position))
        {
            showFxSlotMenu(slotIndex);
            return;
        }
    }

    if ((isShowingMaster() ? state.canAddMasterFxSlot() : state.canAddTrackFxSlot(getFocusedTrackIndex()))
        && getAddFxSlotBounds().contains(e.position))
    {
        if (isShowingMaster())
            state.addMasterFxSlot();
        else
            state.addTrackFxSlot(getFocusedTrackIndex());

        repaint();
        return;
    }

    const int buttonCount = isShowingMaster() ? 3 : 4;
    for (int buttonIndex = 0; buttonIndex < buttonCount; ++buttonIndex)
    {
        if (getButtonBounds(buttonIndex).contains(e.position))
        {
            if (isShowingMaster())
            {
                if (buttonIndex == 0) state.toggleMasterMuted();
                else if (buttonIndex == 1) state.toggleMasterSoloed();
                else state.toggleMasterArmed();
            }
            else
            {
                const int trackIndex = getFocusedTrackIndex();
                if (buttonIndex == 0) state.toggleTrackMuted(trackIndex);
                else if (buttonIndex == 1) state.toggleTrackSoloed(trackIndex);
                else if (buttonIndex == 2) state.toggleTrackMonitoringEnabled(trackIndex);
                else state.toggleTrackArmed(trackIndex);
            }

            repaint();
            return;
        }
    }

    if (getIoButtonBounds().contains(e.position))
    {
        showIoMenu();
        return;
    }

    if (getPanKnobBounds().contains(e.position))
    {
        activeDragTarget = DragTarget::pan;
        updatePanFromPosition(e.position);
        return;
    }

    if (getFaderBounds().contains(e.position))
    {
        activeDragTarget = DragTarget::level;
        updateLevelFromPosition(e.position);
    }
}

void MixerMasterComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (activeDragTarget == DragTarget::pan)
    {
        updatePanFromPosition(e.position);
        return;
    }

    if (activeDragTarget == DragTarget::level)
        updateLevelFromPosition(e.position);
}

void MixerMasterComponent::mouseMove(const juce::MouseEvent& e)
{
    setTooltip(getTooltipForPosition(e.position));
}

void MixerMasterComponent::mouseUp(const juce::MouseEvent&)
{
    activeDragTarget = DragTarget::none;
}

void MixerMasterComponent::mouseExit(const juce::MouseEvent&)
{
    setTooltip({});
}

void MixerMasterComponent::paintMasterView(juce::Graphics& g, juce::Rectangle<float> innerBounds)
{
    auto topRow = innerBounds.removeFromTop(58.0f);
    innerBounds.removeFromTop(8.0f);
    auto pluginBounds = innerBounds.removeFromTop(78.0f);
    auto lowerArea = innerBounds;

    drawTransportButtons(g, topRow.removeFromLeft(156.0f));
    drawPanKnob(g, topRow.removeFromLeft(74.0f).reduced(2.0f));
    drawPluginLane(g, pluginBounds);

    lowerArea.removeFromTop(8.0f);
    auto stripArea = lowerArea.withTrimmedBottom(4.0f);
    auto meterBounds = stripArea.removeFromLeft(18.0f).reduced(0.0f, 2.0f);
    stripArea.removeFromLeft(12.0f);
    drawMeter(g, meterBounds);
    drawFader(g, stripArea.reduced(2.0f, 0.0f));
}

void MixerMasterComponent::paintTrackView(juce::Graphics& g, juce::Rectangle<float> innerBounds, int)
{
    auto topRow = innerBounds.removeFromTop(58.0f);
    innerBounds.removeFromTop(8.0f);
    auto pluginBounds = innerBounds.removeFromTop(78.0f);
    auto lowerArea = innerBounds;

    drawTransportButtons(g, topRow.removeFromLeft(190.0f));
    drawPanKnob(g, topRow.removeFromLeft(74.0f).reduced(2.0f));
    drawPluginLane(g, pluginBounds);

    lowerArea.removeFromTop(8.0f);
    auto stripArea = lowerArea.withTrimmedBottom(4.0f);
    auto meterBounds = stripArea.removeFromLeft(18.0f).reduced(0.0f, 2.0f);
    stripArea.removeFromLeft(12.0f);
    drawMeter(g, meterBounds);
    drawFader(g, stripArea.reduced(2.0f, 0.0f));
}

void MixerMasterComponent::drawTransportButtons(juce::Graphics& g, juce::Rectangle<float> bounds) const
{
    if (isShowingMaster())
    {
        const char* labels[] = { "M", "S", "ARM" };
        static constexpr float widths[] { 32.0f, 32.0f, 44.0f };
        auto row = bounds.withHeight(28.0f);

        for (int i = 0; i < 3; ++i)
        {
            auto button = row.removeFromLeft(widths[i]);
            if (i < 2)
                row.removeFromLeft(6.0f);

            const bool isActive = (i == 0 && state.masterMixerState.muted)
                               || (i == 1 && state.masterMixerState.soloed)
                               || (i == 2 && state.masterMixerState.armed);
            auto fill = isActive ? (i == 2 ? juce::Colour(0xffc2485d) : juce::Colour(0xff3a7afe))
                                 : juce::Colour(0xff2f437e);
            g.setColour(fill);
            g.fillRoundedRectangle(button, 9.0f);
            g.setColour(juce::Colours::white.withAlpha(isActive ? 0.95f : 0.58f));
            g.drawRoundedRectangle(button, 9.0f, 1.2f);
            g.setColour(juce::Colours::white.withAlpha(isActive ? 1.0f : 0.82f));
            g.setFont(juce::Font(i == 2 ? 9.5f : 12.0f, juce::Font::bold));
            g.drawText(labels[i], button.toNearestInt(), juce::Justification::centred, false);
        }

        auto ioBounds = getIoButtonBounds();
        g.setColour(juce::Colour(0xff22346e));
        g.fillRoundedRectangle(ioBounds, 8.0f);
        g.setColour(juce::Colours::white.withAlpha(0.18f));
        g.drawRoundedRectangle(ioBounds, 8.0f, 1.0f);
        g.setColour(juce::Colours::white.withAlpha(0.56f));
        g.setFont(juce::Font(9.5f, juce::Font::plain));
        g.drawText(state.getMasterOutputAssignment(),
                   ioBounds.reduced(6.0f, 0.0f).toNearestInt(),
                   juce::Justification::centredLeft,
                   false);
        return;
    }

    const auto& trackState = state.getTrackMixerState(getFocusedTrackIndex());
    const char* labels[] = { "M", "S", "MON", "ARM" };
    static constexpr float widths[] { 32.0f, 32.0f, 42.0f, 42.0f };
    auto row = bounds.withHeight(28.0f);

    for (int i = 0; i < 4; ++i)
    {
        auto button = row.removeFromLeft(widths[i]);
        if (i < 3)
            row.removeFromLeft(6.0f);

        const bool isActive = (i == 0 && trackState.muted)
                           || (i == 1 && trackState.soloed)
                           || (i == 2 && trackState.monitoringEnabled)
                           || (i == 3 && trackState.armed);
        auto fill = juce::Colour(0xff2f437e);
        if (isActive)
        {
            if (i == 2) fill = juce::Colour(0xff1f7a4d);
            else if (i == 3) fill = juce::Colour(0xffc2485d);
            else fill = juce::Colour(0xff3a7afe);
        }

        g.setColour(fill);
        g.fillRoundedRectangle(button, 9.0f);
        g.setColour(juce::Colours::white.withAlpha(isActive ? 0.95f : 0.58f));
        g.drawRoundedRectangle(button, 9.0f, 1.2f);
        g.setColour(juce::Colours::white.withAlpha(isActive ? 1.0f : 0.82f));
        g.setFont(juce::Font(i >= 2 ? 9.5f : 12.0f, juce::Font::bold));
        g.drawText(labels[i], button.toNearestInt(), juce::Justification::centred, false);
    }

    auto ioBounds = getIoButtonBounds();
    g.setColour(juce::Colour(0xff22346e));
    g.fillRoundedRectangle(ioBounds, 8.0f);
    g.setColour(juce::Colours::white.withAlpha(0.18f));
    g.drawRoundedRectangle(ioBounds, 8.0f, 1.0f);
    g.setColour(juce::Colours::white.withAlpha(0.56f));
    g.setFont(juce::Font(9.5f, juce::Font::plain));
    g.drawText(state.getTrackIoAssignment(getFocusedTrackIndex()),
               ioBounds.reduced(6.0f, 0.0f).toNearestInt(),
               juce::Justification::centredLeft,
               false);
}

void MixerMasterComponent::drawPanKnob(juce::Graphics& g, juce::Rectangle<float> bounds) const
{
    const float pan = isShowingMaster() ? state.masterMixerState.pan : state.getTrackMixerState(getFocusedTrackIndex()).pan;
    g.setColour(juce::Colour(0xff182447));
    g.fillEllipse(bounds);
    g.setColour(juce::Colours::white.withAlpha(0.9f));
    g.drawEllipse(bounds, 1.6f);

    const float angle = juce::jmap(pan, -1.0f, 1.0f,
                                   -juce::MathConstants<float>::pi * 0.75f,
                                   juce::MathConstants<float>::pi * 0.75f);
    const auto endPoint = juce::Point<float>(bounds.getCentreX() + std::sin(angle) * 13.0f,
                                             bounds.getCentreY() - std::cos(angle) * 13.0f);
    g.drawLine(bounds.getCentreX(), bounds.getCentreY(), endPoint.x, endPoint.y, 2.1f);

    auto labelBounds = bounds.withY(bounds.getY() - 14.0f).withHeight(12.0f);
    g.setColour(juce::Colours::white.withAlpha(0.52f));
    g.setFont(juce::Font(10.0f, juce::Font::plain));
    g.drawText(getPanLabelText(pan), labelBounds.toNearestInt(), juce::Justification::centred, false);
}

void MixerMasterComponent::drawMeter(juce::Graphics& g, juce::Rectangle<float> bounds) const
{
    const bool muted = isShowingMaster() ? state.masterMixerState.muted : !state.isTrackAudible(getFocusedTrackIndex());
    const bool soloed = isShowingMaster() ? state.masterMixerState.soloed
                                          : (state.getTrackMixerState(getFocusedTrackIndex()).soloed && state.isTrackAudible(getFocusedTrackIndex()));
    const float level = isShowingMaster() ? state.masterMixerState.level : state.getTrackMixerState(getFocusedTrackIndex()).level;

    auto laneBounds = bounds.reduced(1.0f, 1.0f);
    g.setColour(juce::Colours::white.withAlpha(0.14f));
    g.fillRoundedRectangle(laneBounds, 5.0f);

    float fillRatio = level * (muted ? 0.15f : 0.95f);
    if (soloed)
        fillRatio = juce::jmin(1.0f, fillRatio + 0.08f);
    fillRatio = juce::jlimit(0.05f, 0.98f, fillRatio);

    auto fillBounds = laneBounds.withY(laneBounds.getBottom() - laneBounds.getHeight() * fillRatio)
                                .withHeight(laneBounds.getHeight() * fillRatio);
    juce::ColourGradient gradient(juce::Colour(0xff3bf38d), fillBounds.getCentreX(), fillBounds.getBottom(),
                                  juce::Colour(0xffffd34a), fillBounds.getCentreX(), fillBounds.getY(), false);
    gradient.addColour(0.8, juce::Colour(0xffff8847));
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(fillBounds, 5.0f);
}

void MixerMasterComponent::drawFader(juce::Graphics& g, juce::Rectangle<float> bounds) const
{
    const float level = isShowingMaster() ? state.masterMixerState.level : state.getTrackMixerState(getFocusedTrackIndex()).level;
    auto content = bounds.reduced(2.0f, 0.0f);
    auto lane = content.removeFromTop(26.0f);
    content.removeFromTop(6.0f);
    auto valueBounds = content.removeFromTop(14.0f);

    g.setColour(juce::Colours::white.withAlpha(0.5f));
    g.setFont(juce::Font(10.0f, juce::Font::plain));
    const auto dbValue = juce::jmap(level, 0.0f, 1.0f, -60.0f, 6.0f);
    g.setColour(juce::Colours::white.withAlpha(0.08f));
    g.fillRoundedRectangle(lane, 13.0f);
    g.setColour(juce::Colours::white.withAlpha(0.18f));
    g.drawRoundedRectangle(lane, 13.0f, 1.1f);

    auto rail = lane.reduced(12.0f, 10.0f);
    g.setColour(juce::Colour(0xff172344));
    g.fillRoundedRectangle(rail, 3.0f);

    const float thumbWidth = 20.0f;
    const float thumbTravel = juce::jmax(0.0f, rail.getWidth() - thumbWidth);
    const float thumbX = rail.getX() + thumbTravel * level;
    auto thumb = juce::Rectangle<float>(thumbX, lane.getCentreY() - 9.0f, thumbWidth, 18.0f);
    g.setColour(juce::Colour(0xffdfe9ff));
    g.fillRoundedRectangle(thumb, 6.0f);
    g.setColour(juce::Colour(0xffeef4ff).withAlpha(0.8f));
    g.drawLine(thumb.getCentreX(), thumb.getY() + 3.0f, thumb.getCentreX(), thumb.getBottom() - 3.0f, 1.1f);

    g.setColour(juce::Colours::white.withAlpha(0.5f));
    g.setFont(juce::Font(10.0f, juce::Font::plain));
    g.drawText(juce::String(static_cast<int>(std::round(dbValue))) + " dB",
               valueBounds.toNearestInt(),
               juce::Justification::centredLeft,
               false);
}

void MixerMasterComponent::drawPluginLane(juce::Graphics& g, juce::Rectangle<float> bounds) const
{
    g.setColour(juce::Colours::white.withAlpha(0.08f));
    g.fillRoundedRectangle(bounds, 18.0f);
    g.setColour(juce::Colours::white.withAlpha(0.28f));
    g.drawRoundedRectangle(bounds, 18.0f, 1.3f);

    auto labelBounds = bounds.reduced(14.0f, 10.0f).removeFromTop(16.0f);
    g.setColour(juce::Colours::white.withAlpha(0.6f));
    g.setFont(juce::Font(10.5f, juce::Font::plain));
    g.drawText(isShowingMaster() ? "MASTER FX RACK" : "TRACK FX RACK",
               labelBounds.toNearestInt(), juce::Justification::centredLeft, false);

    auto slotArea = bounds.reduced(14.0f, 10.0f);
    slotArea.removeFromTop(22.0f);
    const int trackIndex = getFocusedTrackIndex();
    const int slotCount = isShowingMaster() ? state.getMasterFxSlotCount() : state.getTrackFxSlotCount(trackIndex);

    for (int i = 0; i < slotCount; ++i)
    {
        auto slot = slotArea.removeFromTop(pluginSlotHeight);
        if (i < slotCount - 1)
            slotArea.removeFromTop(pluginSlotGap);

        const auto& fxSlot = isShowingMaster() ? state.getMasterFxSlot(i) : state.getTrackFxSlot(trackIndex, i);
        g.setColour(juce::Colours::white.withAlpha(0.08f));
        g.fillRoundedRectangle(slot, 7.0f);
        g.setColour(fxSlot.bypassed ? juce::Colour(0xffffd34a).withAlpha(0.7f)
                                    : juce::Colours::white.withAlpha(0.2f));
        g.drawRoundedRectangle(slot, 7.0f, 1.0f);

        g.setColour(fxSlot.hasPlugin ? juce::Colours::white.withAlpha(0.68f)
                                     : juce::Colours::white.withAlpha(0.34f));
        g.setFont(juce::Font(9.0f, juce::Font::plain));
        g.drawText(fxSlot.hasPlugin ? (fxSlot.bypassed ? fxSlot.name + " (Byp)" : fxSlot.name) : "Add plugin",
                   slot.reduced(5.0f, 0.0f).toNearestInt(),
                   juce::Justification::centredLeft,
                   false);
    }

    const bool canAdd = isShowingMaster() ? state.canAddMasterFxSlot() : state.canAddTrackFxSlot(trackIndex);
    if (canAdd)
    {
        slotArea.removeFromTop(pluginAddGap);
        auto addSlot = slotArea.removeFromTop(pluginAddHeight);
        g.setColour(juce::Colour(0xff3a7afe).withAlpha(0.24f));
        g.fillRoundedRectangle(addSlot, 7.0f);
        g.setColour(juce::Colours::white.withAlpha(0.46f));
        g.drawRoundedRectangle(addSlot, 7.0f, 1.0f);
        g.setFont(juce::Font(8.5f, juce::Font::bold));
        g.drawText("+ FX Slot", addSlot.toNearestInt(), juce::Justification::centred, false);
    }
}

void MixerMasterComponent::showFxSlotMenu(int slotIndex)
{
    juce::PopupMenu menu;
    const int trackIndex = getFocusedTrackIndex();
    const auto& fxSlot = isShowingMaster() ? state.getMasterFxSlot(slotIndex) : state.getTrackFxSlot(trackIndex, slotIndex);

    if (!fxSlot.hasPlugin)
    {
        juce::PopupMenu addPluginMenu;
        const auto availablePlugins = isShowingMaster()
            ? (getAvailableMasterPlugins != nullptr ? getAvailableMasterPlugins() : juce::StringArray{})
            : (getAvailableTrackPlugins != nullptr ? getAvailableTrackPlugins() : juce::StringArray{});
        for (int i = 0; i < availablePlugins.size(); ++i)
            addPluginMenu.addItem(100 + i, availablePlugins[i]);

        if (availablePlugins.isEmpty())
            addPluginMenu.addItem(1, "No plugins available", false);

        menu.addSubMenu("Add Plugin", addPluginMenu);
        if (isShowingMaster() ? state.canRemoveMasterFxSlot() : state.canRemoveTrackFxSlot(trackIndex))
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
        menu.addItem(4, "Remove FX Slot",
                     isShowingMaster() ? state.canRemoveMasterFxSlot() : state.canRemoveTrackFxSlot(trackIndex));
    }

    auto target = getPluginSlotBounds(slotIndex).toNearestInt();
    juce::Component::SafePointer<MixerMasterComponent> safeThis(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea(localAreaToGlobal(target)),
                       [safeThis, slotIndex, trackIndex, hasPlugin = fxSlot.hasPlugin](int result)
                       {
                           if (safeThis == nullptr || result == 0)
                               return;

                           if (!hasPlugin && result >= 100)
                           {
                               const auto availablePlugins = safeThis->isShowingMaster()
                                   ? (safeThis->getAvailableMasterPlugins != nullptr ? safeThis->getAvailableMasterPlugins() : juce::StringArray{})
                                   : (safeThis->getAvailableTrackPlugins != nullptr ? safeThis->getAvailableTrackPlugins() : juce::StringArray{});
                               const int pluginIndex = result - 100;
                               if (!juce::isPositiveAndBelow(pluginIndex, availablePlugins.size()))
                                   return;

                               if (safeThis->isShowingMaster())
                               {
                                   if (safeThis->onMasterPluginLoadRequested != nullptr
                                       && safeThis->onMasterPluginLoadRequested(slotIndex, availablePlugins[pluginIndex]))
                                       safeThis->state.loadMasterPluginIntoFxSlot(slotIndex, availablePlugins[pluginIndex]);
                               }
                               else
                               {
                                   if (safeThis->onTrackPluginLoadRequested != nullptr
                                       && safeThis->onTrackPluginLoadRequested(trackIndex, slotIndex, availablePlugins[pluginIndex]))
                                       safeThis->state.loadTrackPluginIntoFxSlot(trackIndex, slotIndex, availablePlugins[pluginIndex]);
                               }
                           }
                           else if (hasPlugin && result == 3)
                           {
                               if (safeThis->isShowingMaster())
                               {
                                   if (safeThis->onMasterPluginEditorRequested != nullptr)
                                       safeThis->onMasterPluginEditorRequested(slotIndex);
                               }
                               else if (safeThis->onTrackPluginEditorRequested != nullptr)
                               {
                                   safeThis->onTrackPluginEditorRequested(trackIndex, slotIndex);
                               }
                           }
                           else if (hasPlugin && result == 1)
                           {
                               if (safeThis->isShowingMaster())
                               {
                                   const bool bypass = !safeThis->state.getMasterFxSlot(slotIndex).bypassed;
                                   if (safeThis->onMasterPluginBypassRequested != nullptr)
                                       safeThis->onMasterPluginBypassRequested(slotIndex, bypass);
                                   safeThis->state.toggleMasterFxSlotBypassed(slotIndex);
                               }
                               else
                               {
                                   const bool bypass = !safeThis->state.getTrackFxSlot(trackIndex, slotIndex).bypassed;
                                   if (safeThis->onTrackPluginBypassRequested != nullptr)
                                       safeThis->onTrackPluginBypassRequested(trackIndex, slotIndex, bypass);
                                   safeThis->state.toggleTrackFxSlotBypassed(trackIndex, slotIndex);
                               }
                           }
                           else if (hasPlugin && result == 2)
                           {
                               if (safeThis->isShowingMaster())
                               {
                                   if (safeThis->onMasterPluginRemoveRequested != nullptr)
                                       safeThis->onMasterPluginRemoveRequested(slotIndex);
                                   safeThis->state.clearMasterFxSlot(slotIndex);
                               }
                               else
                               {
                                   if (safeThis->onTrackPluginRemoveRequested != nullptr)
                                       safeThis->onTrackPluginRemoveRequested(trackIndex, slotIndex);
                                   safeThis->state.clearTrackFxSlot(trackIndex, slotIndex);
                               }
                           }
                           else if (result == 4)
                           {
                               if (safeThis->isShowingMaster())
                               {
                                   if (safeThis->onMasterPluginSlotRemoveRequested != nullptr)
                                       safeThis->onMasterPluginSlotRemoveRequested(slotIndex);
                                   safeThis->state.removeMasterFxSlot(slotIndex);
                               }
                               else
                               {
                                   if (safeThis->onTrackPluginSlotRemoveRequested != nullptr)
                                       safeThis->onTrackPluginSlotRemoveRequested(trackIndex, slotIndex);
                                   safeThis->state.removeTrackFxSlot(trackIndex, slotIndex);
                               }
                           }

                           safeThis->repaint();
                       });
}

void MixerMasterComponent::showInstrumentSlotMenu()
{
    if (isShowingMaster())
        return;

    const int trackIndex = getFocusedTrackIndex();
    const auto& instrumentSlot = state.getTrackInstrumentSlot(trackIndex);
    juce::PopupMenu menu;

    if (!instrumentSlot.hasPlugin)
    {
        juce::PopupMenu addPluginMenu;
        const auto availablePlugins = getAvailableTrackInstrumentPlugins != nullptr
            ? getAvailableTrackInstrumentPlugins()
            : juce::StringArray{};

        for (int i = 0; i < availablePlugins.size(); ++i)
            addPluginMenu.addItem(100 + i, availablePlugins[i]);

        if (availablePlugins.isEmpty())
            addPluginMenu.addItem(1, "No instruments available", false);

        menu.addSubMenu("Add Instrument", addPluginMenu);
    }
    else
    {
        menu.addItem(3, "Open Instrument Editor");
        menu.addSeparator();
        menu.addItem(1, instrumentSlot.bypassed ? "Enable Instrument" : "Bypass Instrument");
        menu.addItem(2, "Remove Instrument");
    }

    auto target = getHeaderInstrumentBounds().toNearestInt();
    juce::Component::SafePointer<MixerMasterComponent> safeThis(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea(localAreaToGlobal(target)),
                       [safeThis, trackIndex, hasPlugin = instrumentSlot.hasPlugin](int result)
                       {
                           if (safeThis == nullptr || result == 0)
                               return;

                           if (!hasPlugin && result >= 100)
                           {
                               const auto availablePlugins = safeThis->getAvailableTrackInstrumentPlugins != nullptr
                                   ? safeThis->getAvailableTrackInstrumentPlugins()
                                   : juce::StringArray{};
                               const int pluginIndex = result - 100;
                               if (!juce::isPositiveAndBelow(pluginIndex, availablePlugins.size()))
                                   return;

                               if (safeThis->onTrackInstrumentPluginLoadRequested != nullptr
                                   && safeThis->onTrackInstrumentPluginLoadRequested(trackIndex, availablePlugins[pluginIndex]))
                               {
                                   safeThis->state.loadTrackInstrumentPlugin(trackIndex, availablePlugins[pluginIndex]);
                               }
                           }
                           else if (hasPlugin && result == 3)
                           {
                               if (safeThis->onTrackInstrumentPluginEditorRequested != nullptr)
                                   safeThis->onTrackInstrumentPluginEditorRequested(trackIndex);
                           }
                           else if (hasPlugin && result == 1)
                           {
                               const bool bypass = !safeThis->state.getTrackInstrumentSlot(trackIndex).bypassed;
                               if (safeThis->onTrackInstrumentPluginBypassRequested != nullptr)
                                   safeThis->onTrackInstrumentPluginBypassRequested(trackIndex, bypass);
                               safeThis->state.toggleTrackInstrumentBypassed(trackIndex);
                           }
                           else if (hasPlugin && result == 2)
                           {
                               if (safeThis->onTrackInstrumentPluginRemoveRequested != nullptr)
                                   safeThis->onTrackInstrumentPluginRemoveRequested(trackIndex);
                               safeThis->state.clearTrackInstrumentSlot(trackIndex);
                           }

                           safeThis->repaint();
                       });
}

void MixerMasterComponent::showTrackContentTypeMenu()
{
    if (isShowingMaster())
        return;

    const int trackIndex = getFocusedTrackIndex();
    const auto currentType = state.getTrackContentType(trackIndex);

    juce::PopupMenu menu;
    menu.addItem(1,
                 "Recording",
                 state.canTrackUseContentType(trackIndex, TrackMixerState::ContentType::recording),
                 currentType == TrackMixerState::ContentType::recording);
    menu.addItem(2,
                 "Pattern",
                 state.canTrackUseContentType(trackIndex, TrackMixerState::ContentType::pattern),
                 currentType == TrackMixerState::ContentType::pattern);

    auto target = getHeaderModeBounds().toNearestInt();
    juce::Component::SafePointer<MixerMasterComponent> safeThis(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea(localAreaToGlobal(target)),
                       [safeThis, trackIndex](int result)
                       {
                           if (safeThis == nullptr || result == 0)
                               return;

                           const auto newType = result == 2
                               ? TrackMixerState::ContentType::pattern
                               : TrackMixerState::ContentType::recording;

                           if (! safeThis->state.trySetTrackContentType(trackIndex, newType))
                               return;

                           if (safeThis->onTrackContentTypeChanged != nullptr)
                               safeThis->onTrackContentTypeChanged(trackIndex);

                           safeThis->repaint();
                       });
}

void MixerMasterComponent::showIoMenu()
{
    juce::PopupMenu menu;

    if (isShowingMaster())
    {
        juce::PopupMenu outputMenu;
        const auto outputs = getAvailableMasterOutputs != nullptr ? getAvailableMasterOutputs() : juce::StringArray{};
        if (outputs.isEmpty())
            outputMenu.addItem(1, "No Outputs Available", false);
        else
            for (int i = 0; i < outputs.size(); ++i)
                outputMenu.addItem(200 + i, outputs[i], true, outputs[i] == state.getMasterOutputAssignment());

        const auto label = getMasterOutputDeviceName != nullptr ? getMasterOutputDeviceName() : juce::String("Outputs");
        menu.addSubMenu("Outputs (" + label + ")", outputMenu);
    }
    else
    {
        const int trackIndex = getFocusedTrackIndex();
        const auto inputs = getAvailableTrackInputs != nullptr ? getAvailableTrackInputs() : juce::StringArray{};
        const auto outputs = getAvailableTrackOutputs != nullptr ? getAvailableTrackOutputs() : juce::StringArray{};
        juce::PopupMenu inputMenu, outputMenu;

        if (inputs.isEmpty()) inputMenu.addItem(1, "No Inputs Available", false);
        else for (int i = 0; i < inputs.size(); ++i)
            inputMenu.addItem(100 + i, inputs[i], true, inputs[i] == state.getTrackInputAssignment(trackIndex));

        if (outputs.isEmpty()) outputMenu.addItem(1, "No Outputs Available", false);
        else for (int i = 0; i < outputs.size(); ++i)
            outputMenu.addItem(200 + i, outputs[i], true, outputs[i] == state.getTrackOutputAssignment(trackIndex));

        const auto inLabel = getTrackInputDeviceName != nullptr ? getTrackInputDeviceName() : juce::String("Inputs");
        const auto outLabel = getTrackOutputDeviceName != nullptr ? getTrackOutputDeviceName() : juce::String("Outputs");
        menu.addSubMenu("Inputs (" + inLabel + ")", inputMenu);
        menu.addSubMenu("Outputs (" + outLabel + ")", outputMenu);
    }

    auto target = getIoButtonBounds().toNearestInt();
    juce::Component::SafePointer<MixerMasterComponent> safeThis(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea(localAreaToGlobal(target)),
                       [safeThis](int result)
                       {
                           if (safeThis == nullptr || result == 0)
                               return;

                           if (safeThis->isShowingMaster())
                           {
                               const auto outputs = safeThis->getAvailableMasterOutputs != nullptr ? safeThis->getAvailableMasterOutputs() : juce::StringArray{};
                               const int outputIndex = result - 200;
                               if (juce::isPositiveAndBelow(outputIndex, outputs.size()))
                                   safeThis->state.setMasterOutputAssignment(outputs[outputIndex]);
                           }
                           else
                           {
                               const int trackIndex = safeThis->getFocusedTrackIndex();
                               const auto inputs = safeThis->getAvailableTrackInputs != nullptr ? safeThis->getAvailableTrackInputs() : juce::StringArray{};
                               const auto outputs = safeThis->getAvailableTrackOutputs != nullptr ? safeThis->getAvailableTrackOutputs() : juce::StringArray{};
                               if (result >= 100 && result < 200)
                               {
                                   const int inputIndex = result - 100;
                                   if (juce::isPositiveAndBelow(inputIndex, inputs.size()))
                                       safeThis->state.setTrackInputAssignment(trackIndex, inputs[inputIndex]);
                               }
                               else if (result >= 200)
                               {
                                   const int outputIndex = result - 200;
                                   if (juce::isPositiveAndBelow(outputIndex, outputs.size()))
                                       safeThis->state.setTrackOutputAssignment(trackIndex, outputs[outputIndex]);
                               }
                           }

                           safeThis->repaint();
                       });
}

void MixerMasterComponent::updatePanFromPosition(juce::Point<float> position)
{
    auto knobBounds = getPanKnobBounds();
    const float normalised = juce::jlimit(0.0f, 1.0f, (position.x - knobBounds.getX()) / juce::jmax(1.0f, knobBounds.getWidth()));
    const float newPan = juce::jmap(normalised, 0.0f, 1.0f, -1.0f, 1.0f);

    if (isShowingMaster()) state.masterMixerState.pan = newPan;
    else state.setTrackPan(getFocusedTrackIndex(), newPan);

    repaint();
}

void MixerMasterComponent::updateLevelFromPosition(juce::Point<float> position)
{
    auto faderBounds = getFaderBounds();
    const float normalised = juce::jlimit(0.0f, 1.0f, (position.x - faderBounds.getX()) / juce::jmax(1.0f, faderBounds.getWidth()));

    if (isShowingMaster()) state.masterMixerState.level = normalised;
    else state.setTrackLevel(getFocusedTrackIndex(), normalised);

    repaint();
}
