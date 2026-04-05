#include "tracklistcomponent.h"

#include <cmath>

namespace
{
    juce::Colour getTrackPanelColour()
    {
        return juce::Colour(0xff2b3f92);
    }

    juce::Colour getTrackEdgeColour()
    {
        return juce::Colour(0xff9db7ff).withAlpha(0.24f);
    }

    juce::Colour getMasterStripColour()
    {
        return juce::Colour(0xff31489f);
    }

    juce::String getPanLabelText(float pan)
    {
        if (std::abs(pan) < 0.04f)
            return "C";

        const int amount = static_cast<int>(std::round(std::abs(pan) * 100.0f));
        return (pan < 0.0f ? "L" : "R") + juce::String(amount);
    }
}

TrackListComponent::TrackListComponent(DAWState& stateIn)
    : state(stateIn)
{
}

void TrackListComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::transparentBlack);

    for (int rowIndex = 0; rowIndex < getVisualRowCount(); ++rowIndex)
    {
        auto rowBounds = getRowBounds(rowIndex);
        if (rowBounds.getBottom() < 0.0f || rowBounds.getY() > static_cast<float>(getHeight()))
            continue;

        drawMixerStrip(g, rowBounds, rowIndex);
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

    const int rowIndex = getVisualRowAt(e.position);
    if (rowIndex < 0)
        return;

    if (isMasterRow(rowIndex))
    {
        state.showMasterMixerFocus();
        repaint();
        if (onMixerFocusChanged)
            onMixerFocusChanged();
        return;
    }

    const int trackIndex = getTrackIndexForRow(rowIndex);
    if (trackIndex < 0)
        return;

    state.selectTrack(trackIndex);
    state.showSelectedTrackMixerFocus();
    repaint();

    if (onTrackSelected)
        onTrackSelected(trackIndex);
    if (onMixerFocusChanged)
        onMixerFocusChanged();

    if (e.mods.isPopupMenu())
    {
        showTrackContextMenu(trackIndex, getRowBounds(rowIndex).toNearestInt());
        return;
    }

    if (getRemoveButtonBounds(trackIndex).contains(e.position))
    {
        if (onRemoveTrackRequested)
            onRemoveTrackRequested(trackIndex);
    }
}

void TrackListComponent::mouseDoubleClick(const juce::MouseEvent& e)
{
    const int rowIndex = getVisualRowAt(e.position);
    if (rowIndex <= 0)
        return;

    const int trackIndex = getTrackIndexForRow(rowIndex);
    if (trackIndex < 0)
        return;

    if (getHeaderBounds(rowIndex).contains(e.position)
        && !getRemoveButtonBounds(trackIndex).contains(e.position))
    {
        promptRenameTrack(trackIndex);
    }
}

void TrackListComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (!draggingScrollbar)
        return;

    const auto trackBounds = getScrollbarTrackBounds();
    const auto thumbBounds = getScrollbarThumbBounds();
    const float maxThumbY = trackBounds.getBottom() - thumbBounds.getHeight();
    const float thumbY = juce::jlimit(trackBounds.getY(), maxThumbY, e.position.y - scrollbarDragOffset);
    const float proportion = (thumbY - trackBounds.getY()) / juce::jmax(1.0f, maxThumbY - trackBounds.getY());
    setScrollOffset(static_cast<int>(std::round(proportion * static_cast<float>(getMaxScroll()))));
}

void TrackListComponent::mouseUp(const juce::MouseEvent&)
{
    draggingScrollbar = false;
}

void TrackListComponent::mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
    if (wheel.deltaY != 0.0f)
        scrollBy(-wheel.deltaY * 112.0f);
}

int TrackListComponent::getContentHeight() const
{
    return juce::jmax(rowHeight, getVisualRowCount() * rowHeight + juce::jmax(0, getVisualRowCount() - 1) * rowGap);
}

int TrackListComponent::getMaxScroll() const
{
    return juce::jmax(0, getContentHeight() - getHeight());
}

int TrackListComponent::getVisualRowCount() const
{
    return state.trackCount + 1;
}

int TrackListComponent::toContentY(float y) const
{
    return static_cast<int>(std::floor(y + static_cast<float>(scrollOffset)));
}

int TrackListComponent::getVisualRowAt(juce::Point<float> point) const
{
    const int rowIndex = toContentY(point.y) / (rowHeight + rowGap);
    return rowIndex >= 0 && rowIndex < getVisualRowCount() ? rowIndex : -1;
}

bool TrackListComponent::isMasterRow(int rowIndex) const
{
    return rowIndex == 0;
}

int TrackListComponent::getTrackIndexForRow(int rowIndex) const
{
    return isMasterRow(rowIndex) ? -1 : rowIndex - 1;
}

juce::Rectangle<float> TrackListComponent::getCardInnerBounds(int rowIndex) const
{
    return getRowBounds(rowIndex).reduced(12.0f, 10.0f);
}

juce::Rectangle<float> TrackListComponent::getHeaderBounds(int rowIndex) const
{
    auto bounds = getCardInnerBounds(rowIndex);
    return bounds.removeFromTop(24.0f);
}

juce::Rectangle<float> TrackListComponent::getRemoveButtonBounds(int trackIndex) const
{
    const int rowIndex = trackIndex + 1;
    auto headerBounds = getHeaderBounds(rowIndex);
    return headerBounds.removeFromRight(24.0f);
}

juce::Rectangle<float> TrackListComponent::getStatusIndicatorBounds(int rowIndex, int indicatorIndex) const
{
    auto bounds = getCardInnerBounds(rowIndex);
    bounds.removeFromTop(30.0f);
    auto row = bounds.removeFromTop(24.0f);

    static constexpr float widths[] { 30.0f, 30.0f, 40.0f, 40.0f };
    for (int i = 0; i < indicatorIndex; ++i)
    {
        row.removeFromLeft(widths[i]);
        row.removeFromLeft(6.0f);
    }

    return row.removeFromLeft(widths[indicatorIndex]);
}

juce::Rectangle<float> TrackListComponent::getCompactMeterBounds(int rowIndex) const
{
    auto bounds = getCardInnerBounds(rowIndex);
    bounds.removeFromTop(30.0f);
    bounds.removeFromTop(28.0f);
    bounds.removeFromTop(10.0f);
    return bounds.removeFromLeft(14.0f).withTrimmedBottom(6.0f);
}

juce::Rectangle<float> TrackListComponent::getCompactLevelBounds(int rowIndex) const
{
    auto bounds = getCardInnerBounds(rowIndex);
    bounds.removeFromTop(30.0f);
    bounds.removeFromTop(28.0f);
    bounds.removeFromTop(10.0f);
    bounds.removeFromLeft(24.0f);
    return bounds.removeFromTop(20.0f);
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

void TrackListComponent::drawMixerStrip(juce::Graphics& g, juce::Rectangle<float> bounds, int rowIndex)
{
    auto cardBounds = bounds.reduced(0.0f, 2.0f);
    const bool masterRow = isMasterRow(rowIndex);
    const int trackIndex = getTrackIndexForRow(rowIndex);
    const bool isSelected = masterRow ? state.isMasterMixerFocused() : state.isTrackSelected(trackIndex) && !state.isMasterMixerFocused();
    const bool isAudible = masterRow ? !state.masterMixerState.muted : state.isTrackAudible(trackIndex);
    auto panelColour = masterRow ? getMasterStripColour() : getTrackPanelColour();
    if (isSelected)
        panelColour = panelColour.brighter(0.08f);
    if (!isAudible)
        panelColour = panelColour.darker(0.2f).withMultipliedAlpha(0.9f);

    g.setColour(panelColour);
    g.fillRoundedRectangle(cardBounds, 20.0f);

    auto edgeColour = isSelected ? juce::Colour(0xffd7e3ff).withAlpha(0.72f) : getTrackEdgeColour();
    if (!isAudible)
        edgeColour = edgeColour.withAlpha(0.22f);
    g.setColour(edgeColour);
    g.drawRoundedRectangle(cardBounds.reduced(1.3f), 20.0f, isSelected ? 2.2f : 1.8f);

    auto innerBounds = cardBounds.reduced(12.0f, 10.0f);
    auto headerBounds = innerBounds.removeFromTop(24.0f);

    g.setColour(juce::Colours::white.withAlpha(isAudible ? 0.86f : 0.56f));
    g.setFont(juce::Font(masterRow ? 14.5f : 14.0f, juce::Font::bold));
    const auto title = masterRow ? juce::String("MASTER") : state.getTrackName(trackIndex);
    g.drawText(title,
               headerBounds.withTrimmedRight(masterRow ? 0.0f : 30.0f).toNearestInt(),
               juce::Justification::centredLeft,
               false);

    if (masterRow)
    {
        g.setColour(juce::Colours::white.withAlpha(0.46f));
        g.setFont(juce::Font(10.0f, juce::Font::plain));
        g.drawText("Focused bus",
                   headerBounds.toNearestInt(),
                   juce::Justification::centredRight,
                   false);
    }
    else
    {
        auto removeBounds = getRemoveButtonBounds(trackIndex);
        g.setColour(juce::Colour(0xff23357f).withMultipliedAlpha(isAudible ? 1.0f : 0.6f));
        g.fillRoundedRectangle(removeBounds, 7.0f);
        g.setColour(juce::Colours::white.withAlpha(isAudible ? 0.75f : 0.45f));
        g.setFont(juce::Font(16.0f, juce::Font::bold));
        g.drawText("-", removeBounds.toNearestInt(), juce::Justification::centred, false);
    }

    innerBounds.removeFromTop(6.0f);
    auto indicatorRow = innerBounds.removeFromTop(24.0f);
    auto summaryRow = innerBounds;

    if (masterRow)
    {
        auto row = indicatorRow;
        const char* labels[] = { "M", "S", "ARM" };
        const bool states[] = { state.masterMixerState.muted, state.masterMixerState.soloed, state.masterMixerState.armed };
        static constexpr float widths[] { 30.0f, 30.0f, 44.0f };

        for (int i = 0; i < 3; ++i)
        {
            auto pill = row.removeFromLeft(widths[i]);
            row.removeFromLeft(6.0f);
            auto fill = states[i] ? (i == 2 ? juce::Colour(0xffc2485d) : juce::Colour(0xff3a7afe)) : juce::Colour(0xff2f437e);
            g.setColour(fill);
            g.fillRoundedRectangle(pill, 8.0f);
            g.setColour(juce::Colours::white.withAlpha(states[i] ? 0.95f : 0.58f));
            g.drawRoundedRectangle(pill, 8.0f, 1.0f);
            g.setColour(juce::Colours::white.withAlpha(states[i] ? 1.0f : 0.82f));
            g.setFont(juce::Font(i == 2 ? 9.5f : 11.0f, juce::Font::bold));
            g.drawText(labels[i], pill.toNearestInt(), juce::Justification::centred, false);
        }

        auto meterBounds = getCompactMeterBounds(rowIndex);
        auto levelBounds = getCompactLevelBounds(rowIndex);
        drawCompactMeter(g, meterBounds, state.masterMixerState.level, !state.masterMixerState.muted);

        g.setColour(juce::Colours::white.withAlpha(0.56f));
        g.setFont(juce::Font(10.0f, juce::Font::plain));
        g.drawText(state.getMasterOutputAssignment(),
                   summaryRow.removeFromTop(16.0f).toNearestInt(),
                   juce::Justification::centredLeft,
                   false);

        g.setColour(juce::Colours::white.withAlpha(0.42f));
        g.drawText("Pan " + getPanLabelText(state.masterMixerState.pan),
                   levelBounds.toNearestInt(),
                   juce::Justification::centredLeft,
                   false);
        return;
    }

    drawStatusIndicators(g, indicatorRow, trackIndex);

    auto meterBounds = getCompactMeterBounds(rowIndex);
    auto levelBounds = getCompactLevelBounds(rowIndex);
    drawCompactMeter(g, meterBounds, state.getTrackMixerState(trackIndex).level, isAudible);

    g.setColour(juce::Colours::white.withAlpha(isAudible ? 0.56f : 0.34f));
    g.setFont(juce::Font(10.0f, juce::Font::plain));
    g.drawText(state.getTrackIoAssignment(trackIndex),
               summaryRow.removeFromTop(16.0f).toNearestInt(),
               juce::Justification::centredLeft,
               false);

    const auto dbValue = juce::jmap(state.getTrackMixerState(trackIndex).level, 0.0f, 1.0f, -60.0f, 6.0f);
    const auto levelText = juce::String(static_cast<int>(std::round(dbValue))) + " dB";
    g.setColour(juce::Colours::white.withAlpha(isAudible ? 0.44f : 0.28f));
    g.drawText("Pan " + getPanLabelText(state.getTrackMixerState(trackIndex).pan) + "   " + levelText,
               levelBounds.toNearestInt(),
               juce::Justification::centredLeft,
               false);
}

void TrackListComponent::drawStatusIndicators(juce::Graphics& g, juce::Rectangle<float>, int trackIndex) const
{
    const char* labels[] = { "M", "S", "MON", "ARM" };
    const auto& trackState = state.getTrackMixerState(trackIndex);
    const bool isAudible = state.isTrackAudible(trackIndex);
    const bool states[] = { trackState.muted, trackState.soloed, trackState.monitoringEnabled, trackState.armed };

    for (int i = 0; i < 4; ++i)
    {
        auto pill = getStatusIndicatorBounds(trackIndex + 1, i);
        auto fill = juce::Colour(0xff2f437e);
        if (states[i])
        {
            if (i == 2)
                fill = juce::Colour(0xff1f7a4d);
            else if (i == 3)
                fill = juce::Colour(0xffc2485d);
            else
                fill = juce::Colour(0xff3a7afe);
        }
        else if (!isAudible)
        {
            fill = fill.darker(0.18f).withMultipliedAlpha(0.82f);
        }

        g.setColour(fill);
        g.fillRoundedRectangle(pill, 8.0f);
        g.setColour(juce::Colours::white.withAlpha(states[i] ? 0.95f : (isAudible ? 0.58f : 0.36f)));
        g.drawRoundedRectangle(pill, 8.0f, 1.0f);
        g.setColour(juce::Colours::white.withAlpha(states[i] ? 1.0f : (isAudible ? 0.82f : 0.5f)));
        g.setFont(juce::Font(i >= 2 ? 9.5f : 11.0f, juce::Font::bold));
        g.drawText(labels[i], pill.toNearestInt(), juce::Justification::centred, false);
    }
}

void TrackListComponent::drawCompactMeter(juce::Graphics& g, juce::Rectangle<float> bounds, float level, bool isActive) const
{
    auto laneBounds = bounds.reduced(1.0f, 1.0f);
    g.setColour(juce::Colours::white.withAlpha(isActive ? 0.14f : 0.08f));
    g.fillRoundedRectangle(laneBounds, 5.0f);

    const float fillRatio = juce::jlimit(0.05f, 0.98f, level * (isActive ? 0.95f : 0.1f));
    auto fillBounds = laneBounds.withY(laneBounds.getBottom() - laneBounds.getHeight() * fillRatio)
                                .withHeight(laneBounds.getHeight() * fillRatio);

    juce::ColourGradient gradient(juce::Colour(0xff3bf38d), fillBounds.getCentreX(), fillBounds.getBottom(),
                                  juce::Colour(0xffffd34a), fillBounds.getCentreX(), fillBounds.getY(), false);
    gradient.addColour(0.78, juce::Colour(0xffff8847));
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(fillBounds, 5.0f);
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
