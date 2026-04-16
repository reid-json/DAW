#include "tracklistcomponent.h"
#include <cmath>

namespace
{
    constexpr auto panelColour     = juce::uint32(0xff2b3f92);
    constexpr auto masterColour    = juce::uint32(0xff31489f);
    constexpr auto edgeColour      = juce::uint32(0xff9db7ff);
    constexpr auto pillOffColour   = juce::uint32(0xff2f437e);
    constexpr auto muteColour      = juce::uint32(0xff3a7afe);
    constexpr auto armColour       = juce::uint32(0xffc2485d);
    constexpr auto fxColour        = juce::uint32(0xff7b3fa0);
    constexpr auto inputColour     = juce::uint32(0xff1f7a7a);

    juce::File findResourcesDir()
    {
        auto dir = juce::File::getSpecialLocation (juce::File::currentExecutableFile).getParentDirectory();
        for (int i = 0; i < 4; ++i)
        {
            if (dir.getChildFile ("Resources").isDirectory())
                return dir.getChildFile ("Resources");
            dir = dir.getParentDirectory();
        }

        return dir.getChildFile ("Resources");
    }

    juce::Image loadSpriteImage (const juce::String& fileName)
    {
        return juce::ImageFileFormat::loadFrom (findResourcesDir().getChildFile ("ui")
                                                                  .getChildFile ("sprites")
                                                                  .getChildFile (fileName));
    }

    juce::Image loadBodySpiceImage()
    {
        return loadSpriteImage ("bodySpice.png");
    }

    juce::String formatDbValue(float level)
    {
        const auto db = juce::jmap(level, 0.0f, 1.0f, -60.0f, 6.0f);
        if (db <= -59.5f)
            return "-inf dB";

        return juce::String(db, db < 0.0f ? 1 : 0) + " dB";
    }

    juce::String formatPanValue(float pan)
    {
        if (std::abs(pan) < 0.05f)
            return "C";

        const auto amount = juce::roundToInt(std::abs(pan) * 100.0f);
        return (pan < 0.0f ? "L" : "R") + juce::String(amount);
    }
}

TrackListComponent::TrackListComponent(DAWState& stateIn, ThemeData& themeIn) : state(stateIn), theme(themeIn)
{
    mutedSprite = loadSpriteImage ("muteTriggered.png");
    unmutedSprite = loadSpriteImage ("unmuted.png");
    armSprite = loadSpriteImage ("arm.png");
    bodySpiceImage = loadBodySpiceImage();
}

// geometry

int TrackListComponent::getVisualRowCount() const { return state.trackCount + 1; }
bool TrackListComponent::isMasterRow(int rowIndex) const { return rowIndex == 0; }

int TrackListComponent::getTrackIndexForRow(int rowIndex) const
{
    return isMasterRow(rowIndex) ? -1 : rowIndex - 1;
}

juce::Rectangle<float> TrackListComponent::getRowBounds(int rowIndex) const
{
    const float w = juce::jmax(0.0f, static_cast<float>(getWidth() - 8));
    const float y = static_cast<float>(rowIndex * (rowHeight + rowGap)) - state.verticalScrollOffset;
    return { 4.0f, y, w, static_cast<float>(rowHeight) };
}

juce::Rectangle<float> TrackListComponent::getAddTrackButtonBounds() const
{
    const float w = juce::jmax(0.0f, static_cast<float>(getWidth() - 8));
    const float y = static_cast<float>(getVisualRowCount() * (rowHeight + rowGap)) - state.verticalScrollOffset + 4.0f;
    return { 4.0f, y, w, 36.0f };
}

int TrackListComponent::getVisualRowAt(juce::Point<float> point) const
{
    const int row = static_cast<int>(point.y + state.verticalScrollOffset) / (rowHeight + rowGap);
    return (row >= 0 && row < getVisualRowCount()) ? row : -1;
}

int TrackListComponent::getContentHeight() const
{
    return getVisualRowCount() * (rowHeight + rowGap) + 76;
}

int TrackListComponent::getMaxScroll() const
{
    return juce::jmax(0, getContentHeight() - getHeight());
}

// All layout helpers use the same inner bounds as drawing:
// card = getRowBounds(row).reduced(0, 2), inner = card.reduced(10, 6)
// Row 1: header 20px
// Gap: 4px
// Row 2: indicators 20px
// Gap: 4px
// Row 3: controls 20px

static juce::Rectangle<float> getInner(juce::Rectangle<float> rowBounds)
{
    return rowBounds.reduced(0.0f, 2.0f).reduced(10.0f, 6.0f);
}

juce::Rectangle<float> TrackListComponent::getIndicatorBounds(int rowIndex, int indicatorIndex) const
{
    auto inner = getInner(getRowBounds(rowIndex));
    inner.removeFromTop(20.0f); // header
    inner.removeFromTop(4.0f);  // gap
    auto row = inner.removeFromTop(20.0f);

    if (isMasterRow(rowIndex))
    {
        static constexpr float widths[] { 30.0f, 22.0f };
        for (int i = 0; i < indicatorIndex; ++i)
            row.removeFromLeft(widths[i]);
        return row.removeFromLeft(widths[indicatorIndex]);
    }

    static constexpr float widths[] { 30.0f, 22.0f, 22.0f, 22.0f };
    for (int i = 0; i < indicatorIndex; ++i)
        row.removeFromLeft(widths[i]);
    return row.removeFromLeft(widths[indicatorIndex]);
}

juce::Rectangle<float> TrackListComponent::getRemoveButtonBounds(int trackIndex) const
{
    auto inner = getInner(getRowBounds(trackIndex + 1));
    auto header = inner.removeFromTop(20.0f);
    return header.removeFromRight(22.0f);
}

juce::Rectangle<float> TrackListComponent::getFaderBounds(int rowIndex) const
{
    auto inner = getInner(getRowBounds(rowIndex));
    inner.removeFromTop(20.0f + 4.0f + 20.0f + 4.0f); // header + gap + indicators + gap
    return inner.removeFromLeft(inner.getWidth() * 0.55f).withHeight(20.0f);
}

juce::Rectangle<float> TrackListComponent::getPanKnobBounds(int rowIndex) const
{
    auto inner = getInner(getRowBounds(rowIndex));
    inner.removeFromTop(20.0f + 4.0f + 20.0f + 4.0f);
    auto controlRow = inner.removeFromTop(20.0f);
    controlRow.removeFromLeft(controlRow.getWidth() * 0.55f + 8.0f);
    auto knobBounds = controlRow.removeFromLeft(28.0f);
    return knobBounds.withSizeKeepingCentre(24.0f, 24.0f);
}

juce::Rectangle<float> TrackListComponent::getRoutingButtonBounds(int rowIndex) const
{
    auto inner = getInner(getRowBounds(rowIndex));
    inner.removeFromTop(20.0f + 4.0f + 20.0f + 4.0f);
    auto controlRow = inner.removeFromTop(20.0f);
    return controlRow.removeFromRight(36.0f);
}

// drawing

void TrackListComponent::paint(juce::Graphics& g)
{
    g.fillAll(theme.colour("tracklist.background", juce::Colour(0xff0f141e)));
    if (bodySpiceImage.isValid())
    {
        const int imageYOffset = juce::roundToInt(getHeight() * 0.28f);
        g.setOpacity(0.9f);
        g.drawImage(bodySpiceImage,
                    0,
                    imageYOffset,
                    getWidth(),
                    getHeight(),
                    0,
                    0,
                    bodySpiceImage.getWidth(),
                    bodySpiceImage.getHeight());
        g.setOpacity(1.0f);
    }

    for (int row = 0; row < getVisualRowCount(); ++row)
    {
        auto bounds = getRowBounds(row);
        if (bounds.getBottom() < 0.0f) continue;
        if (bounds.getY() > static_cast<float>(getHeight())) break;
        drawStrip(g, row);
    }

    auto addBounds = getAddTrackButtonBounds();
    if (addBounds.getY() < static_cast<float>(getHeight()))
    {
        g.setColour(theme.colour("tracklist.add-track.background", juce::Colour(pillOffColour).withAlpha(0.6f)));
        g.fillRoundedRectangle(addBounds, 10.0f);
        g.setColour(theme.colour("tracklist.add-track.border", juce::Colours::white.withAlpha(0.55f)));
        g.drawRoundedRectangle(addBounds, 10.0f, 1.0f);
        g.setFont(juce::Font(12.0f, juce::Font::bold));
        g.setColour(theme.colour("tracklist.add-track.text", juce::Colours::white.withAlpha(0.7f)));
        g.drawText("Add Track +", addBounds.toNearestInt(), juce::Justification::centred, false);
    }
}

void TrackListComponent::drawStrip(juce::Graphics& g, int rowIndex)
{
    const bool master = isMasterRow(rowIndex);
    const int trackIdx = getTrackIndexForRow(rowIndex);
    const bool selected = master ? state.isMasterMixerFocused()
                                 : (state.isTrackSelected(trackIdx) && !state.isMasterMixerFocused());
    const bool audible = master ? !state.masterMixerState.muted : state.isTrackAudible(trackIdx);
    const float level = master ? state.masterMixerState.level : state.getTrackMixerState(trackIdx).level;
    const float pan   = master ? state.masterMixerState.pan   : state.getTrackMixerState(trackIdx).pan;

    auto card = getRowBounds(rowIndex).reduced(0.0f, 2.0f);

    // Background
    auto bg = theme.colour(master ? "tracklist.master-panel" : "tracklist.panel",
                           juce::Colour(master ? masterColour : panelColour));
    if (selected) bg = bg.brighter(0.08f);
    if (!audible) bg = bg.darker(0.2f).withMultipliedAlpha(0.9f);
    g.setColour(bg);
    g.fillRoundedRectangle(card, 14.0f);

    auto edgeCol = selected ? theme.colour("tracklist.edge.selected", juce::Colour(0xffd7e3ff).withAlpha(0.72f))
                            : theme.colour("tracklist.edge.default", juce::Colour(edgeColour).withAlpha(0.24f));
    g.setColour(edgeCol);
    g.drawRoundedRectangle(card.reduced(1.0f), 14.0f, selected ? 2.0f : 1.2f);

    auto inner = getInner(getRowBounds(rowIndex));

    // header row
    auto headerRow = inner.removeFromTop(20.0f);
    if (editingTrackIndex != trackIdx || master)
    {
        const auto title = master ? juce::String("MASTER") : state.getTrackName(trackIdx);
        g.setColour(audible ? theme.colour("tracklist.text", juce::Colours::white.withAlpha(0.88f))
                            : theme.colour("tracklist.text-muted", juce::Colours::white.withAlpha(0.5f)));
        g.setFont(juce::Font(13.0f, juce::Font::bold));
        g.drawText(title, headerRow.withTrimmedRight(master ? 0.0f : 72.0f).toNearestInt(),
                   juce::Justification::centredLeft, false);
    }

    if (!master)
    {
        // Remove button
        auto rem = getRemoveButtonBounds(trackIdx);
        g.setColour(theme.colour("tracklist.remove-button.background", juce::Colour(0xff23357f)));
        g.fillRoundedRectangle(rem, 6.0f);
        g.setColour(theme.colour("tracklist.remove-button.text", juce::Colours::white.withAlpha(0.65f)));
        g.setFont(juce::Font(14.0f, juce::Font::bold));
        g.drawText("-", rem.toNearestInt(), juce::Justification::centred, false);
    }

    inner.removeFromTop(4.0f);

    // indicator pills (mute, arm, fx, input)
    inner.removeFromTop(20.0f); // space is used by getIndicatorBounds

    if (master)
    {
        bool hasMasterFx = false;
        for (int s = 0; s < state.getMasterFxSlotCount(); ++s)
            if (state.getMasterFxSlot(s).hasPlugin) { hasMasterFx = true; break; }

        const char* labels[] = { "Mute", "FX" };
        const bool states[]  = { state.masterMixerState.muted, hasMasterFx };
        const juce::Colour colours[] = { theme.colour("tracklist.mute", juce::Colour(muteColour)),
                                         theme.colour("tracklist.fx", juce::Colour(fxColour)) };

        auto firstBounds = getIndicatorBounds(rowIndex, 0).withHeight(18.0f);
        auto lastBounds = getIndicatorBounds(rowIndex, 1).withHeight(18.0f);
        auto groupBounds = firstBounds.getUnion(lastBounds).withCentre({ firstBounds.getUnion(lastBounds).getCentreX(), firstBounds.getCentreY() });

        g.setColour(theme.colour("tracklist.pill-off", juce::Colour(pillOffColour)));
        g.fillRoundedRectangle(groupBounds, groupBounds.getHeight() * 0.5f);
        g.setColour(theme.colour("tracklist.pill-border-inactive", juce::Colours::white.withAlpha(0.5f)));
        g.drawRoundedRectangle(groupBounds, groupBounds.getHeight() * 0.5f, 1.0f);

        auto dividerX = getIndicatorBounds(rowIndex, 0).getRight();
        g.setColour(theme.colour("tracklist.pill-divider", juce::Colours::white.withAlpha(0.24f)));
        g.drawVerticalLine(juce::roundToInt(dividerX), groupBounds.getY() + 2.0f, groupBounds.getBottom() - 2.0f);

        for (int i = 0; i < 2; ++i)
        {
            const juce::Image* sprite = (i == 0 ? (states[i] ? &mutedSprite : &unmutedSprite) : nullptr);
            drawIndicatorPill(g, getIndicatorBounds(rowIndex, i), labels[i], states[i], colours[i],
                              i == 0, i == 1, sprite);
        }
    }
    else
    {
        const auto& ts = state.getTrackMixerState(trackIdx);
        bool hasTrackFx = false;
        for (const auto& slot : ts.fxSlots)
            if (slot.hasPlugin) { hasTrackFx = true; break; }
        bool hasInput = ts.inputAssignment.isNotEmpty() && ts.inputAssignment != "Input channel 1";

        const char* labels[] = { "Mute", "Arm", "FX", "IN" };
        const bool states[]  = { ts.muted, ts.armed, hasTrackFx, hasInput };
        const juce::Colour colours[] = { theme.colour("tracklist.mute", juce::Colour(muteColour)),
                                         theme.colour("tracklist.arm", juce::Colour(armColour)),
                                         theme.colour("tracklist.fx", juce::Colour(fxColour)),
                                         theme.colour("tracklist.input", juce::Colour(inputColour)) };

        auto firstBounds = getIndicatorBounds(rowIndex, 0).withHeight(18.0f);
        auto lastBounds = getIndicatorBounds(rowIndex, 3).withHeight(18.0f);
        auto groupBounds = firstBounds.getUnion(lastBounds).withCentre({ firstBounds.getUnion(lastBounds).getCentreX(), firstBounds.getCentreY() });

        g.setColour(theme.colour("tracklist.pill-off", juce::Colour(pillOffColour)));
        g.fillRoundedRectangle(groupBounds, groupBounds.getHeight() * 0.5f);
        g.setColour(theme.colour("tracklist.pill-border-inactive", juce::Colours::white.withAlpha(0.5f)));
        g.drawRoundedRectangle(groupBounds, groupBounds.getHeight() * 0.5f, 1.0f);

        g.setColour(theme.colour("tracklist.pill-divider", juce::Colours::white.withAlpha(0.24f)));
        for (int i = 0; i < 3; ++i)
        {
            const auto dividerX = getIndicatorBounds(rowIndex, i).getRight();
            g.drawVerticalLine(juce::roundToInt(dividerX), groupBounds.getY() + 2.0f, groupBounds.getBottom() - 2.0f);
        }

        for (int i = 0; i < 4; ++i)
        {
            const juce::Image* sprite = nullptr;
            if (i == 0)
                sprite = states[i] ? &mutedSprite : &unmutedSprite;
            else if (i == 1)
                sprite = &armSprite;

            drawIndicatorPill(g, getIndicatorBounds(rowIndex, i), labels[i], states[i], colours[i],
                              i == 0, i == 3, sprite);
        }
    }

    inner.removeFromTop(4.0f);

    // fader, pan knob, routing button
    drawFader(g, getFaderBounds(rowIndex), level, audible, rowIndex);
    drawPanKnob(g, getPanKnobBounds(rowIndex), pan, audible, rowIndex);

    // Routing button
    auto routeBtn = getRoutingButtonBounds(rowIndex);
    g.setColour(theme.colour("tracklist.routing.background", juce::Colour(pillOffColour)));
    g.fillRoundedRectangle(routeBtn, 6.0f);
    g.setColour(theme.colour("tracklist.routing.border", juce::Colours::white.withAlpha(0.6f)));
    g.drawRoundedRectangle(routeBtn, 6.0f, 1.0f);
    g.setFont(juce::Font(9.0f, juce::Font::bold));
    g.setColour(theme.colour("tracklist.routing.text", juce::Colours::white.withAlpha(0.7f)));

    if (!master)
    {
        const auto& out = state.getTrackOutputAssignment(trackIdx);
        const auto label = out.containsIgnoreCase("Master") ? juce::String("MST") : out.substring(0, 5);
        g.drawText(label, routeBtn.toNearestInt(), juce::Justification::centred, false);
    }
    else
    {
        g.drawText("OUT", routeBtn.toNearestInt(), juce::Justification::centred, false);
    }
}

void TrackListComponent::drawIndicatorPill(juce::Graphics& g, juce::Rectangle<float> bounds,
                                            const char* label, bool active, juce::Colour activeColour,
                                            bool roundLeft, bool roundRight,
                                            const juce::Image* sprite) const
{
    auto pillBounds = bounds.withHeight(18.0f).withCentre({ bounds.getCentreX(), bounds.getCentreY() });
    const float cornerSize = pillBounds.getHeight() * 0.5f;

    auto path = juce::Path();
    const auto x = pillBounds.getX();
    const auto y = pillBounds.getY();
    const auto w = pillBounds.getWidth();
    const auto h = pillBounds.getHeight();
    const auto r = juce::jmin(cornerSize, h * 0.5f, w * 0.5f);

    path.startNewSubPath(x + (roundLeft ? r : 0.0f), y);
    path.lineTo(x + w - (roundRight ? r : 0.0f), y);

    if (roundRight)
        path.addArc(x + w - 2.0f * r, y, 2.0f * r, 2.0f * r, -juce::MathConstants<float>::halfPi, juce::MathConstants<float>::halfPi, true);
    else
        path.lineTo(x + w, y);

    path.lineTo(x + w, y + h);
    path.lineTo(x + (roundLeft ? r : 0.0f), y + h);

    if (roundLeft)
        path.addArc(x, y, 2.0f * r, 2.0f * r, juce::MathConstants<float>::halfPi, juce::MathConstants<float>::pi * 1.5f, true);
    else
        path.lineTo(x, y);

    path.closeSubPath();

    if (active)
    {
        g.setColour(activeColour);
        g.fillPath(path);
    }

    if (sprite != nullptr && sprite->isValid())
    {
        auto iconBounds = pillBounds.reduced(5.0f, 4.0f);
        g.setOpacity(active ? 1.0f : 0.78f);
        g.drawImageWithin(*sprite,
                          juce::roundToInt(iconBounds.getX()),
                          juce::roundToInt(iconBounds.getY()),
                          juce::roundToInt(iconBounds.getWidth()),
                          juce::roundToInt(iconBounds.getHeight()),
                          juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize,
                          false);
        g.setOpacity(1.0f);
        return;
    }

    g.setColour(active ? theme.colour("tracklist.pill-text-active", juce::Colours::white)
                       : theme.colour("tracklist.pill-text-inactive", juce::Colours::white.withAlpha(0.7f)));
    g.setFont(juce::Font(8.5f, juce::Font::bold));
    g.drawText(label, pillBounds.toNearestInt(), juce::Justification::centred, false);
}

void TrackListComponent::drawFader(juce::Graphics& g, juce::Rectangle<float> bounds, float level, bool active, int rowIndex) const
{
    auto track = bounds.withHeight(6.0f).withCentre({ bounds.getCentreX(), bounds.getCentreY() });
    g.setColour(active ? theme.colour("tracklist.fader.track-active", juce::Colours::white.withAlpha(0.12f))
                       : theme.colour("tracklist.fader.track-inactive", juce::Colours::white.withAlpha(0.06f)));
    g.fillRoundedRectangle(track, 3.0f);
    g.setColour(theme.colour("tracklist.fader.outline", juce::Colours::white.withAlpha(0.75f)));
    g.drawRoundedRectangle(track, 3.0f, 1.0f);

    g.setColour(active ? theme.colour("tracklist.fader.fill-active", juce::Colour(0xff3a7afe).withAlpha(0.7f))
                       : theme.colour("tracklist.fader.fill-inactive", juce::Colours::white.withAlpha(0.15f)));
    g.fillRoundedRectangle(track.withWidth(track.getWidth() * level), 3.0f);

    float tx = track.getX() + track.getWidth() * level;
    auto handle = juce::Rectangle<float>(0, 0, 10, bounds.getHeight())
                      .withCentre({ tx, bounds.getCentreY() });
    g.setColour(active ? theme.colour("tracklist.fader.handle-active", juce::Colours::white.withAlpha(0.85f))
                       : theme.colour("tracklist.fader.handle-inactive", juce::Colours::white.withAlpha(0.35f)));
    g.fillRoundedRectangle(handle, 4.0f);
    g.setColour(theme.colour("tracklist.fader.outline", juce::Colours::white.withAlpha(0.75f)));
    g.drawRoundedRectangle(handle, 4.0f, 1.0f);

    if (draggingFaderRow == rowIndex)
    {
        auto readoutBounds = juce::Rectangle<float>(0.0f, 0.0f, 56.0f, 18.0f)
                                 .withCentre({ handle.getCentreX(), handle.getY() - 12.0f });

        if (readoutBounds.getX() < 4.0f)
            readoutBounds.setX(4.0f);
        if (readoutBounds.getRight() > static_cast<float>(getWidth() - 4))
            readoutBounds.setX(static_cast<float>(getWidth()) - 4.0f - readoutBounds.getWidth());

        g.setColour(theme.colour("tracklist.fader-readout.background", juce::Colour(0xff101729).withAlpha(0.96f)));
        g.fillRoundedRectangle(readoutBounds, 6.0f);
        g.setColour(theme.colour("tracklist.fader-readout.outline", juce::Colours::white.withAlpha(0.65f)));
        g.drawRoundedRectangle(readoutBounds, 6.0f, 1.0f);
        g.setColour(theme.colour("tracklist.fader-readout.text", juce::Colours::white.withAlpha(0.92f)));
        g.setFont(juce::Font(9.0f, juce::Font::bold));
        g.drawText(formatDbValue(level), readoutBounds.toNearestInt(), juce::Justification::centred, false);
    }
}

void TrackListComponent::drawPanKnob(juce::Graphics& g, juce::Rectangle<float> bounds, float pan, bool active, int rowIndex) const
{
    auto centre = bounds.getCentre();
    const float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.44f;

    auto knobBounds = bounds.reduced(1.0f);
    g.setColour(active ? theme.colour("tracklist.pan.ring-active", juce::Colour(0xffe68000))
                       : theme.colour("tracklist.pan.ring-inactive", juce::Colour(0xffe68000)));
    g.fillEllipse(knobBounds);
    g.setColour(juce::Colours::white.withAlpha(0.9f));
    g.drawEllipse(knobBounds, 1.2f);

    const float angle = juce::MathConstants<float>::pi * 0.75f
                      + (pan + 1.0f) * 0.5f * juce::MathConstants<float>::pi * 1.5f;
    const float endX = centre.x + std::cos(angle) * radius;
    const float endY = centre.y + std::sin(angle) * radius;
    g.setColour(active ? theme.colour("tracklist.pan.indicator-active", juce::Colours::white)
                       : theme.colour("tracklist.pan.indicator-inactive", juce::Colours::white));
    g.drawLine(centre.x, centre.y, endX, endY, 2.0f);

    if (draggingPanRow == rowIndex)
    {
        auto readoutBounds = juce::Rectangle<float>(0.0f, 0.0f, 42.0f, 18.0f)
                                 .withCentre({ bounds.getCentreX(), bounds.getY() - 12.0f });

        if (readoutBounds.getX() < 4.0f)
            readoutBounds.setX(4.0f);
        if (readoutBounds.getRight() > static_cast<float>(getWidth() - 4))
            readoutBounds.setX(static_cast<float>(getWidth()) - 4.0f - readoutBounds.getWidth());

        g.setColour(theme.colour("tracklist.pan-readout.background", juce::Colour(0xff101729).withAlpha(0.96f)));
        g.fillRoundedRectangle(readoutBounds, 6.0f);
        g.setColour(theme.colour("tracklist.pan-readout.outline", juce::Colours::white.withAlpha(0.65f)));
        g.drawRoundedRectangle(readoutBounds, 6.0f, 1.0f);
        g.setColour(theme.colour("tracklist.pan-readout.text", juce::Colours::white.withAlpha(0.92f)));
        g.setFont(juce::Font(9.0f, juce::Font::bold));
        g.drawText(formatPanValue(pan), readoutBounds.toNearestInt(), juce::Justification::centred, false);
    }
}

// mouse handling

void TrackListComponent::mouseDown(const juce::MouseEvent& e)
{
    if (getAddTrackButtonBounds().contains(e.position))
    {
        if (onAddTrackRequested) onAddTrackRequested();
        return;
    }

    const int rowIndex = getVisualRowAt(e.position);
    if (rowIndex < 0) return;

    const bool master = isMasterRow(rowIndex);
    const int trackIdx = getTrackIndexForRow(rowIndex);

    // Right-click context menu (tracks only)
    if (!master && e.mods.isPopupMenu())
    {
        state.selectTrack(trackIdx);
        repaint();
        showTrackContextMenu(trackIdx);
        return;
    }

    // Indicator button clicks — check these FIRST before selecting
    const int numIndicators = master ? 2 : 4;
    for (int i = 0; i < numIndicators; ++i)
    {
        if (getIndicatorBounds(rowIndex, i).contains(e.position))
        {
            if (master)
            {
                if (i == 0) state.toggleMasterMuted();
                else if (i == 1) showFxMenu(rowIndex);
            }
            else
            {
                if (i == 0) state.toggleTrackMuted(trackIdx);
                else if (i == 1) state.toggleTrackArmed(trackIdx);
                else if (i == 2) showFxMenu(rowIndex);
                else if (i == 3) showInputMenu(rowIndex);
            }
            repaint();
            return;
        }
    }

    // Fader drag
    if (getFaderBounds(rowIndex).contains(e.position))
    {
        draggingFaderRow = rowIndex;
        auto f = getFaderBounds(rowIndex);
        float val = juce::jlimit(0.0f, 1.0f, (e.position.x - f.getX()) / f.getWidth());
        if (master) state.masterMixerState.level = val;
        else        state.setTrackLevel(trackIdx, val);
        repaint();
        return;
    }

    // Pan knob
    if (getPanKnobBounds(rowIndex).contains(e.position))
    {
        draggingPanRow = rowIndex;
        auto k = getPanKnobBounds(rowIndex);
        float val = juce::jlimit(-1.0f, 1.0f, (e.position.x - k.getCentreX()) / (k.getWidth() * 0.5f));
        if (master) state.masterMixerState.pan = val;
        else        state.setTrackPan(trackIdx, val);
        repaint();
        return;
    }

    // Routing button
    if (getRoutingButtonBounds(rowIndex).contains(e.position))
    {
        showRoutingMenu(rowIndex);
        return;
    }

    // Remove button
    if (!master && getRemoveButtonBounds(trackIdx).contains(e.position))
    {
        if (onRemoveTrackRequested) onRemoveTrackRequested(trackIdx);
        return;
    }

    // Default: select track
    if (master)
        state.showMasterMixerFocus();
    else
    {
        state.selectTrack(trackIdx);
        state.showSelectedTrackMixerFocus();
        if (onTrackSelected) onTrackSelected(trackIdx);
    }
    if (onMixerFocusChanged) onMixerFocusChanged();
    repaint();
}

void TrackListComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (draggingFaderRow >= 0)
    {
        auto f = getFaderBounds(draggingFaderRow);
        float val = juce::jlimit(0.0f, 1.0f, (e.position.x - f.getX()) / f.getWidth());
        if (isMasterRow(draggingFaderRow)) state.masterMixerState.level = val;
        else                               state.setTrackLevel(getTrackIndexForRow(draggingFaderRow), val);
        repaint();
    }
    else if (draggingPanRow >= 0)
    {
        auto k = getPanKnobBounds(draggingPanRow);
        float val = juce::jlimit(-1.0f, 1.0f, (e.position.x - k.getCentreX()) / (k.getWidth() * 0.5f));
        if (isMasterRow(draggingPanRow)) state.masterMixerState.pan = val;
        else                             state.setTrackPan(getTrackIndexForRow(draggingPanRow), val);
        repaint();
    }
}

void TrackListComponent::mouseUp(const juce::MouseEvent&)
{
    draggingFaderRow = -1;
    draggingPanRow = -1;
}

void TrackListComponent::mouseDoubleClick(const juce::MouseEvent& e)
{
    const int row = getVisualRowAt(e.position);
    if (row <= 0) return;
    const int trackIdx = getTrackIndexForRow(row);
    if (trackIdx < 0) return;

    auto inner = getInner(getRowBounds(row));
    auto header = inner.removeFromTop(20.0f);
    if (header.contains(e.position) && !getRemoveButtonBounds(trackIdx).contains(e.position))
        promptRenameTrack(trackIdx);
}

void TrackListComponent::mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
    const int delta = static_cast<int>(wheel.deltaY * -300.0f);
    state.verticalScrollOffset = juce::jlimit(0, getMaxScroll(), state.verticalScrollOffset + delta);
    if (auto* top = getTopLevelComponent())
        top->repaint();
    else
        repaint();
}

// menus

void TrackListComponent::promptRenameTrack(int trackIndex)
{
    startInlineRename(trackIndex);
}

void TrackListComponent::startInlineRename(int trackIndex)
{
    cancelInlineRename();

    editingTrackIndex = trackIndex;
    inlineEditor = std::make_unique<juce::TextEditor>();
    inlineEditor->setText(state.getTrackName(trackIndex), false);
    inlineEditor->selectAll();
    inlineEditor->setFont(juce::Font(13.0f, juce::Font::bold));
    inlineEditor->setColour(juce::TextEditor::backgroundColourId, theme.colour("tracklist.inline-editor.background", juce::Colour(0xff1b2744)));
    inlineEditor->setColour(juce::TextEditor::textColourId, theme.colour("tracklist.inline-editor.text", juce::Colours::white));
    inlineEditor->setColour(juce::TextEditor::outlineColourId, theme.colour("tracklist.inline-editor.outline", juce::Colour(0xff3a7afe)));

    auto inner = getInner(getRowBounds(trackIndex + 1));
    auto header = inner.removeFromTop(20.0f).withTrimmedRight(72.0f);
    inlineEditor->setBounds(header.toNearestInt());

    addAndMakeVisible(inlineEditor.get());
    inlineEditor->grabKeyboardFocus();

    inlineEditor->onReturnKey = [this] { commitInlineRename(); };
    inlineEditor->onEscapeKey = [this] { cancelInlineRename(); };
    inlineEditor->onFocusLost = [this] { commitInlineRename(); };
}

void TrackListComponent::commitInlineRename()
{
    if (inlineEditor == nullptr || editingTrackIndex < 0) return;

    auto newName = inlineEditor->getText().trim();
    if (newName.isNotEmpty())
        state.setTrackName(editingTrackIndex, newName);

    editingTrackIndex = -1;
    inlineEditor.reset();
    repaint();
}

void TrackListComponent::cancelInlineRename()
{
    editingTrackIndex = -1;
    inlineEditor.reset();
    repaint();
}

void TrackListComponent::showTrackContextMenu(int trackIndex)
{
    juce::PopupMenu menu;
    menu.addItem(1, "Rename Track");
    menu.addSeparator();
    menu.addItem(2, "Arm Exclusively");
    menu.addItem(3, "Disarm All Tracks");
    menu.addSeparator();
    menu.addItem(4, "Delete Track", state.trackCount > 1);

    juce::Component::SafePointer<TrackListComponent> safeThis(this);
    menu.showMenuAsync(juce::PopupMenu::Options(),
        [safeThis, trackIndex](int result)
        {
            if (safeThis == nullptr || result == 0) return;
            if (result == 1) safeThis->promptRenameTrack(trackIndex);
            else if (result == 2) { safeThis->state.armOnlyTrack(trackIndex); safeThis->repaint(); }
            else if (result == 3) { safeThis->state.disarmAllTracks(); safeThis->repaint(); }
            else if (result == 4) { if (safeThis->onRemoveTrackRequested) safeThis->onRemoveTrackRequested(trackIndex); }
        });
}

void TrackListComponent::showRoutingMenu(int rowIndex)
{
    const bool master = isMasterRow(rowIndex);
    const int trackIdx = getTrackIndexForRow(rowIndex);

    juce::PopupMenu menu;

    if (master)
    {
        menu.addItem(1, "Output channel 1", true, true);
    }
    else
    {
        menu.addItem(1, "Master", true, state.getTrackOutputAssignment(trackIdx) == "Master");
        menu.addSeparator();
        for (int i = 0; i < state.trackCount; ++i)
        {
            if (i == trackIdx) continue;
            const auto name = state.getTrackName(i);
            menu.addItem(100 + i, "Track: " + name, true,
                         state.getTrackOutputAssignment(trackIdx) == name);
        }
    }

    juce::Component::SafePointer<TrackListComponent> safeThis(this);
    menu.showMenuAsync(juce::PopupMenu::Options(),
        [safeThis, trackIdx, master](int result)
        {
            if (safeThis == nullptr || result == 0) return;
            if (master) return;
            if (result == 1)
                safeThis->state.setTrackOutputAssignment(trackIdx, "Master");
            else if (result >= 100)
                safeThis->state.setTrackOutputAssignment(trackIdx, safeThis->state.getTrackName(result - 100));
            safeThis->repaint();
        });
}

void TrackListComponent::showFxMenu(int rowIndex)
{
    const bool master = isMasterRow(rowIndex);
    const int trackIdx = getTrackIndexForRow(rowIndex);

    juce::StringArray available;
    if (onGetAvailablePlugins)
        available = onGetAvailablePlugins(master);

    juce::PopupMenu menu;

    for (int slot = 0; slot < 5; ++slot)
    {
        juce::String name;
        if (onGetPluginName)
            name = onGetPluginName(master, trackIdx, slot);

        juce::PopupMenu slotMenu;
        if (name.isNotEmpty())
        {
            slotMenu.addItem(slot * 1000 + 1, "Open Editor");
            slotMenu.addItem(slot * 1000 + 2, "Remove");
            slotMenu.addSeparator();
        }
        for (int p = 0; p < available.size(); ++p)
            slotMenu.addItem(slot * 1000 + 10 + p, available[p]);

        const auto label = name.isEmpty()
            ? juce::String(slot + 1) + ": ---"
            : juce::String(slot + 1) + ": " + name;
        menu.addSubMenu(label, slotMenu);
    }

    juce::Component::SafePointer<TrackListComponent> safeThis(this);
    menu.showMenuAsync(juce::PopupMenu::Options(),
        [safeThis, master, trackIdx, available](int result)
        {
            if (safeThis == nullptr || result == 0) return;
            const int slot = result / 1000;
            const int action = result % 1000;
            if (action == 1 && safeThis->onShowPluginEditor)
                safeThis->onShowPluginEditor(master, trackIdx, slot);
            else if (action == 2 && safeThis->onRemovePlugin)
                safeThis->onRemovePlugin(master, trackIdx, slot);
            else if (action >= 10 && safeThis->onLoadPlugin)
            {
                const int pluginIdx = action - 10;
                if (pluginIdx < available.size())
                    safeThis->onLoadPlugin(master, trackIdx, slot, available[pluginIdx]);
            }
            safeThis->repaint();
        });
}

void TrackListComponent::showInputMenu(int rowIndex)
{
    int ti = getTrackIndexForRow(rowIndex);
    if (ti < 0) return;

    auto inputs = onGetAvailableInputs ? onGetAvailableInputs() : juce::StringArray {};
    auto current = state.getTrackInputAssignment(ti);

    juce::PopupMenu menu;
    for (int i = 0; i < inputs.size(); ++i)
        menu.addItem(10 + i, inputs[i], true, current == inputs[i]);

    juce::Component::SafePointer<TrackListComponent> safe(this);
    menu.showMenuAsync({}, [safe, ti, inputs](int r)
    {
        if (safe == nullptr || r < 10 || r - 10 >= inputs.size()) return;
        safe->state.setTrackInputAssignment(ti, inputs[r - 10]);
        safe->repaint();
    });
}
