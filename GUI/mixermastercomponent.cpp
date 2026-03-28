#include "mixermastercomponent.h"

#include <cmath>

namespace
{
    constexpr float pluginSlotHeight = 12.0f;
    constexpr float pluginSlotGap = 7.0f;
    constexpr float pluginAddGap = 8.0f;
    constexpr float pluginAddHeight = 12.0f;

    juce::Colour getMasterPanelColour()
    {
        return juce::Colour(0xff2d4299);
    }

    juce::Colour getMasterEdgeColour()
    {
        return juce::Colour(0xff9db7ff).withAlpha(0.24f);
    }
}

MixerMasterComponent::MixerMasterComponent(DAWState& stateIn)
    : state(stateIn)
{
}

void MixerMasterComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::transparentBlack);

    auto bounds = getLocalBounds().toFloat().reduced(4.0f);
    g.setColour(getMasterPanelColour());
    g.fillRoundedRectangle(bounds, 24.0f);

    g.setColour(getMasterEdgeColour());
    g.drawRoundedRectangle(bounds.reduced(1.5f), 24.0f, 2.0f);

    auto innerBounds = getInnerBounds();
    auto headerBounds = innerBounds.removeFromTop(26.0f);

    g.setColour(juce::Colours::white.withAlpha(0.82f));
    g.setFont(juce::Font(15.0f, juce::Font::bold));
    g.drawText("MASTER", headerBounds.toNearestInt(), juce::Justification::centredLeft, false);

    innerBounds.removeFromTop(8.0f);
    auto topRow = innerBounds.removeFromTop(48.0f);
    innerBounds.removeFromTop(8.0f);
    auto pluginBounds = innerBounds.removeFromTop(78.0f);
    innerBounds.removeFromTop(10.0f);
    auto lowerArea = innerBounds;

    auto buttonsBounds = topRow.removeFromLeft(156.0f);
    auto knobBounds = topRow.removeFromLeft(68.0f).reduced(4.0f);

    drawTransportButtons(g, buttonsBounds);
    drawPanKnob(g, knobBounds);
    drawPluginLane(g, pluginBounds);

    auto levelLabelBounds = lowerArea.removeFromTop(18.0f);
    g.setColour(juce::Colours::white.withAlpha(0.52f));
    g.setFont(juce::Font(11.0f, juce::Font::plain));
    g.drawText("MASTER LEVEL", levelLabelBounds.toNearestInt(), juce::Justification::centredLeft, false);

    lowerArea.removeFromTop(14.0f);
    auto stripArea = lowerArea.withTrimmedBottom(4.0f);
    auto meterBounds = stripArea.removeFromLeft(18.0f).reduced(0.0f, 2.0f);
    stripArea.removeFromLeft(12.0f);
    auto faderBounds = stripArea.reduced(2.0f, 0.0f);

    drawMeter(g, meterBounds);
    drawFader(g, faderBounds);
}

void MixerMasterComponent::mouseDown(const juce::MouseEvent& e)
{
    for (int slotIndex = 0; slotIndex < state.getMasterFxSlotCount(); ++slotIndex)
    {
        if (getPluginSlotBounds(slotIndex).contains(e.position))
        {
            showFxSlotMenu(slotIndex);
            return;
        }
    }

    if (state.canAddMasterFxSlot() && getAddFxSlotBounds().contains(e.position))
    {
        state.addMasterFxSlot();
        repaint();
        return;
    }

    for (int buttonIndex = 0; buttonIndex < 3; ++buttonIndex)
    {
        if (getButtonBounds(buttonIndex).contains(e.position))
        {
            if (buttonIndex == 0)
                state.toggleMasterMuted();
            else if (buttonIndex == 1)
                state.toggleMasterSoloed();
            else
                state.toggleMasterArmed();

            repaint();
            return;
        }
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

void MixerMasterComponent::mouseUp(const juce::MouseEvent&)
{
    activeDragTarget = DragTarget::none;
}

juce::Rectangle<float> MixerMasterComponent::getInnerBounds() const
{
    return getLocalBounds().toFloat().reduced(20.0f, 18.0f);
}

juce::Rectangle<float> MixerMasterComponent::getPanKnobBounds() const
{
    auto innerBounds = getInnerBounds();
    innerBounds.removeFromTop(34.0f);
    auto topRow = innerBounds.removeFromTop(48.0f);
    topRow.removeFromLeft(156.0f);
    return topRow.removeFromLeft(68.0f).reduced(4.0f);
}

juce::Rectangle<float> MixerMasterComponent::getControlButtonsBounds() const
{
    auto innerBounds = getInnerBounds();
    innerBounds.removeFromTop(34.0f);
    auto topRow = innerBounds.removeFromTop(48.0f);
    return topRow.removeFromLeft(156.0f);
}

juce::Rectangle<float> MixerMasterComponent::getButtonBounds(int buttonIndex) const
{
    auto row = getControlButtonsBounds().withHeight(28.0f);
    for (int i = 0; i < buttonIndex; ++i)
    {
        row.removeFromLeft(38.0f);
        row.removeFromLeft(10.0f);
    }

    return row.removeFromLeft(38.0f);
}

juce::Rectangle<float> MixerMasterComponent::getFaderBounds() const
{
    auto innerBounds = getInnerBounds();
    innerBounds.removeFromTop(34.0f);
    innerBounds.removeFromTop(48.0f);
    innerBounds.removeFromTop(8.0f);
    innerBounds.removeFromTop(78.0f);
    innerBounds.removeFromTop(10.0f);
    auto lowerArea = innerBounds;
    lowerArea.removeFromTop(18.0f);
    lowerArea.removeFromTop(14.0f);
    auto stripArea = lowerArea.withTrimmedBottom(4.0f);
    stripArea.removeFromLeft(18.0f);
    stripArea.removeFromLeft(12.0f);
    return stripArea.reduced(2.0f, 0.0f);
}

juce::Rectangle<float> MixerMasterComponent::getPluginLaneBounds() const
{
    auto innerBounds = getInnerBounds();
    innerBounds.removeFromTop(34.0f);
    innerBounds.removeFromTop(48.0f);
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

    const int slotCount = state.getMasterFxSlotCount();
    for (int i = 0; i < slotCount; ++i)
    {
        slotArea.removeFromTop(pluginSlotHeight);
        slotArea.removeFromTop(pluginSlotGap);
    }

    slotArea.removeFromTop(pluginAddGap);
    return slotArea.removeFromTop(pluginAddHeight);
}

void MixerMasterComponent::drawTransportButtons(juce::Graphics& g, juce::Rectangle<float> bounds) const
{
    const char* labels[] = { "M", "S", "R" };
    auto row = bounds.withHeight(28.0f);

    for (int i = 0; i < 3; ++i)
    {
        auto button = row.removeFromLeft(38.0f);
        row.removeFromLeft(10.0f);

        const bool isActive = (i == 0 && state.masterMixerState.muted)
                           || (i == 1 && state.masterMixerState.soloed)
                           || (i == 2 && state.masterMixerState.armed);

        auto fill = isActive
            ? (i == 2 ? juce::Colour(0xffc2485d) : juce::Colour(0xff3a7afe))
            : juce::Colour(0xff2f437e);
        auto border = isActive
            ? juce::Colours::white.withAlpha(0.95f)
            : juce::Colours::white.withAlpha(0.58f);
        auto text = isActive
            ? juce::Colours::white
            : juce::Colours::white.withAlpha(0.82f);

        g.setColour(fill);
        g.fillRoundedRectangle(button, 9.0f);
        g.setColour(border);
        g.drawRoundedRectangle(button, 9.0f, 1.2f);
        g.setFont(juce::Font(12.0f, juce::Font::bold));
        g.setColour(text);
        g.drawText(labels[i], button.toNearestInt(), juce::Justification::centred, false);
    }
}

void MixerMasterComponent::drawPanKnob(juce::Graphics& g, juce::Rectangle<float> bounds) const
{
    g.setColour(juce::Colour(0xff182447));
    g.fillEllipse(bounds);
    g.setColour(juce::Colours::white.withAlpha(0.9f));
    g.drawEllipse(bounds, 1.6f);

    const float angle = juce::jmap(state.masterMixerState.pan,
                                   -1.0f, 1.0f,
                                   -juce::MathConstants<float>::pi * 0.75f,
                                   juce::MathConstants<float>::pi * 0.75f);
    const auto endPoint = juce::Point<float>(bounds.getCentreX() + std::sin(angle) * 13.0f,
                                             bounds.getCentreY() - std::cos(angle) * 13.0f);
    g.drawLine(bounds.getCentreX(), bounds.getCentreY(), endPoint.x, endPoint.y, 2.1f);
}

void MixerMasterComponent::drawMeter(juce::Graphics& g, juce::Rectangle<float> bounds) const
{
    auto laneBounds = bounds.reduced(1.0f, 1.0f);
    g.setColour(juce::Colours::white.withAlpha(0.14f));
    g.fillRoundedRectangle(laneBounds, 5.0f);

    float fillRatio = state.masterMixerState.level * (state.masterMixerState.muted ? 0.15f : 0.95f);
    if (state.masterMixerState.soloed)
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
    auto content = bounds.reduced(2.0f, 0.0f);
    auto valueBounds = content.removeFromTop(14.0f);
    content.removeFromTop(8.0f);
    auto lane = content.withHeight(26.0f).withCentre({ content.getCentreX(), content.getY() + 13.0f });

    g.setColour(juce::Colours::white.withAlpha(0.5f));
    g.setFont(juce::Font(10.0f, juce::Font::plain));
    const auto dbValue = juce::jmap(state.masterMixerState.level, 0.0f, 1.0f, -60.0f, 6.0f);
    const auto dbText = juce::String(static_cast<int>(std::round(dbValue))) + " dB";
    g.drawText(dbText,
               valueBounds.toNearestInt(),
               juce::Justification::centredLeft,
               false);

    g.setColour(juce::Colours::white.withAlpha(0.08f));
    g.fillRoundedRectangle(lane, 13.0f);
    g.setColour(juce::Colours::white.withAlpha(0.18f));
    g.drawRoundedRectangle(lane, 13.0f, 1.1f);

    auto rail = lane.reduced(12.0f, 10.0f);
    g.setColour(juce::Colour(0xff172344));
    g.fillRoundedRectangle(rail, 3.0f);

    const float thumbWidth = 20.0f;
    const float thumbTravel = juce::jmax(0.0f, rail.getWidth() - thumbWidth);
    const float thumbX = rail.getX() + thumbTravel * state.masterMixerState.level;
    auto thumb = juce::Rectangle<float>(thumbX, lane.getCentreY() - 9.0f, thumbWidth, 18.0f);
    g.setColour(juce::Colour(0xffdfe9ff));
    g.fillRoundedRectangle(thumb, 6.0f);
    g.setColour(juce::Colour(0xffeef4ff).withAlpha(0.8f));
    g.drawLine(thumb.getCentreX(), thumb.getY() + 3.0f, thumb.getCentreX(), thumb.getBottom() - 3.0f, 1.1f);
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
    g.drawText("MASTER FX RACK", labelBounds.toNearestInt(), juce::Justification::centredLeft, false);

    auto slotArea = bounds.reduced(14.0f, 10.0f);
    slotArea.removeFromTop(22.0f);

    const int slotCount = state.getMasterFxSlotCount();
    for (int i = 0; i < slotCount; ++i)
    {
        auto slot = slotArea.removeFromTop(pluginSlotHeight);
        if (i < slotCount - 1)
            slotArea.removeFromTop(pluginSlotGap);
        const auto& fxSlot = state.getMasterFxSlot(i);
        g.setColour(juce::Colours::white.withAlpha(0.08f));
        g.fillRoundedRectangle(slot, 7.0f);
        g.setColour(fxSlot.bypassed ? juce::Colour(0xffffd34a).withAlpha(0.7f)
                                    : juce::Colours::white.withAlpha(0.2f));
        g.drawRoundedRectangle(slot, 7.0f, 1.0f);

        g.setColour(fxSlot.hasPlugin ? juce::Colours::white.withAlpha(0.68f)
                                     : juce::Colours::white.withAlpha(0.34f));
        g.setFont(juce::Font(9.0f, juce::Font::plain));
        g.drawText(fxSlot.hasPlugin ? (fxSlot.bypassed ? fxSlot.name + " (Byp)" : fxSlot.name)
                                    : "Add plugin",
                   slot.reduced(5.0f, 0.0f).toNearestInt(),
                   juce::Justification::centredLeft,
                   false);
    }

    if (state.canAddMasterFxSlot())
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
    const auto& fxSlot = state.getMasterFxSlot(slotIndex);

    if (!fxSlot.hasPlugin)
    {
        juce::PopupMenu addPluginMenu;
        const auto availablePlugins = getAvailableMasterPlugins != nullptr
            ? getAvailableMasterPlugins()
            : juce::StringArray{};
        for (int i = 0; i < availablePlugins.size(); ++i)
            addPluginMenu.addItem(100 + i, availablePlugins[i]);

        if (availablePlugins.isEmpty())
            addPluginMenu.addItem(1, "No plugins available", false);

        menu.addSubMenu("Add Plugin", addPluginMenu);
        if (state.canRemoveMasterFxSlot())
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
        menu.addItem(4, "Remove FX Slot", state.canRemoveMasterFxSlot());
    }

    auto target = getPluginSlotBounds(slotIndex).toNearestInt();
    juce::Component::SafePointer<MixerMasterComponent> safeThis(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea(localAreaToGlobal(target)),
                       [safeThis, slotIndex, hasPlugin = fxSlot.hasPlugin](int result)
                       {
                           if (safeThis == nullptr || result == 0)
                               return;

                           if (!hasPlugin && result >= 100)
                           {
                               const auto availablePlugins = safeThis->getAvailableMasterPlugins != nullptr
                                   ? safeThis->getAvailableMasterPlugins()
                                   : juce::StringArray{};
                               const int pluginIndex = result - 100;
                               if (juce::isPositiveAndBelow(pluginIndex, availablePlugins.size())
                                   && safeThis->onMasterPluginLoadRequested != nullptr
                                   && safeThis->onMasterPluginLoadRequested(slotIndex, availablePlugins[pluginIndex]))
                               {
                                   safeThis->state.loadMasterPluginIntoFxSlot(slotIndex, availablePlugins[pluginIndex]);
                               }
                           }
                           else if (hasPlugin && result == 3)
                           {
                               if (safeThis->onMasterPluginEditorRequested != nullptr)
                                   safeThis->onMasterPluginEditorRequested(slotIndex);
                           }
                           else if (hasPlugin && result == 1)
                           {
                               const bool shouldBeBypassed = !safeThis->state.getMasterFxSlot(slotIndex).bypassed;
                               if (safeThis->onMasterPluginBypassRequested != nullptr)
                                   safeThis->onMasterPluginBypassRequested(slotIndex, shouldBeBypassed);
                               safeThis->state.toggleMasterFxSlotBypassed(slotIndex);
                           }
                           else if (hasPlugin && result == 2)
                           {
                               if (safeThis->onMasterPluginRemoveRequested != nullptr)
                                   safeThis->onMasterPluginRemoveRequested(slotIndex);
                               safeThis->state.clearMasterFxSlot(slotIndex);
                           }
                           else if (result == 4)
                           {
                               if (safeThis->onMasterPluginSlotRemoveRequested != nullptr)
                                   safeThis->onMasterPluginSlotRemoveRequested(slotIndex);
                               safeThis->state.removeMasterFxSlot(slotIndex);
                           }

                           safeThis->repaint();
                       });
}

void MixerMasterComponent::updatePanFromPosition(juce::Point<float> position)
{
    auto knobBounds = getPanKnobBounds();
    const float normalised = juce::jlimit(0.0f, 1.0f, (position.x - knobBounds.getX()) / juce::jmax(1.0f, knobBounds.getWidth()));
    state.masterMixerState.pan = juce::jmap(normalised, 0.0f, 1.0f, -1.0f, 1.0f);
    repaint();
}

void MixerMasterComponent::updateLevelFromPosition(juce::Point<float> position)
{
    auto faderBounds = getFaderBounds();
    const float normalised = juce::jlimit(0.0f, 1.0f, (position.x - faderBounds.getX()) / juce::jmax(1.0f, faderBounds.getWidth()));
    state.masterMixerState.level = normalised;
    repaint();
}
