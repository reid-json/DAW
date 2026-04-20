#include "recentclipscomponent.h"

#include <cmath>

namespace
{
constexpr int sectionHeaderHeight = 30;
constexpr int rowHeight = 56;
constexpr int sectionGap = 12;
constexpr int emptyRowHeight = 42;
const auto renameDialogBackground = juce::Colour (0xff18181b);

class RenameDialogLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawButtonBackground (juce::Graphics& g,
                               juce::Button& button,
                               const juce::Colour&,
                               bool shouldDrawButtonAsHighlighted,
                               bool shouldDrawButtonAsDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
        auto fill = button.findColour (juce::TextButton::buttonColourId);
        if (shouldDrawButtonAsDown)
            fill = fill.darker (0.34f);
        else if (shouldDrawButtonAsHighlighted)
            fill = fill.brighter (0.22f);

        g.setColour (juce::Colours::white.withAlpha (0.98f));
        g.fillRoundedRectangle (bounds, 8.0f);
        g.setColour (fill);
        g.fillRoundedRectangle (bounds.reduced (1.5f), 6.5f);
    }

    void drawButtonText (juce::Graphics& g,
                         juce::TextButton& button,
                         bool,
                         bool) override
    {
        g.setColour (juce::Colours::white);
        g.setFont (juce::Font (13.0f, juce::Font::bold));
        g.drawText (button.getButtonText(), button.getLocalBounds(), juce::Justification::centred, false);
    }

    juce::Font getAlertWindowTitleFont() override      { return juce::Font (16.0f, juce::Font::bold); }
    juce::Font getAlertWindowMessageFont() override    { return juce::Font (13.0f, juce::Font::bold); }
    juce::Font getTextButtonFont (juce::TextButton&, int) override { return juce::Font (13.0f, juce::Font::bold); }
};

RenameDialogLookAndFeel& getRenameDialogLookAndFeel()
{
    static RenameDialogLookAndFeel lookAndFeel;
    return lookAndFeel;
}

juce::Image createDragPreviewImage(const juce::String& name, bool isPattern, ThemeData& theme)
{
    juce::Image image(juce::Image::ARGB, 220, rowHeight - 6, true);
    juce::Graphics g(image);

    const auto prefix = isPattern ? "recent-clips.pattern-item." : "recent-clips.clip-item.";
    auto area = image.getBounds().reduced(4);
    g.setColour(theme.colour(prefix + juce::String("background")));
    g.fillRoundedRectangle(area.toFloat(), 8.0f);
    g.setColour(theme.colour(prefix + juce::String("outline")));
    g.drawRoundedRectangle(area.toFloat(), 8.0f, 1.2f);
    g.setColour(theme.colour("recent-clips.item.text"));
    g.drawText(name, area.reduced(10, 8), juce::Justification::centredLeft, true);

    return image;
}

juce::String getSectionTitle(RecentClipsComponent::ItemKind kind)
{
    return kind == RecentClipsComponent::ItemKind::clip ? "Recent Audio Files" : "Saved Patterns";
}
}

RecentClipsComponent::RecentClipsComponent(DAWState& stateIn, ThemeData& themeIn)
    : state(stateIn), theme(themeIn)
{
    popupMenuLookAndFeel = std::make_unique<SharedPopupMenuLookAndFeel> (theme);
}

juce::Rectangle<int> RecentClipsComponent::getItemBounds(ItemKind kind, int index) const
{
    const bool hasClips = ! state.recentClips.empty();
    const bool hasPatterns = ! state.savedPatterns.empty();

    int y = -scrollOffset;
    y += sectionHeaderHeight;

    if (kind == ItemKind::clip)
        return juce::Rectangle<int>(0, y + index * rowHeight, getWidth() - scrollbarWidth, rowHeight - 6).reduced(4);

    y += hasClips ? static_cast<int>(state.recentClips.size()) * rowHeight : (hasPatterns ? emptyRowHeight : 0);
    y += sectionGap + sectionHeaderHeight;
    return juce::Rectangle<int>(0, y + index * rowHeight, getWidth() - scrollbarWidth, rowHeight - 6).reduced(4);
}

void RecentClipsComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::transparentBlack);

    const bool hasClips = ! state.recentClips.empty();
    const bool hasPatterns = ! state.savedPatterns.empty();

    if (! hasClips && ! hasPatterns)
    {
        auto emptyBounds = getLocalBounds().reduced(10, 20);
        g.setColour(theme.colour("recent-clips.empty.title"));
        g.setFont(juce::Font(16.0f, juce::Font::bold));
        g.drawText("No clips or patterns", emptyBounds.removeFromTop(28), juce::Justification::centredTop, false);
        return;
    }

    int y = -scrollOffset;
    auto drawSection = [&] (ItemKind kind, int count, bool showInlineEmpty)
    {
        auto header = juce::Rectangle<int>(0, y, getWidth() - scrollbarWidth, sectionHeaderHeight).reduced(4, 2);
        if (header.getBottom() >= 0 && header.getY() <= getHeight())
        {
            g.setColour(theme.colour("recent-clips.section-title"));
            g.setFont(juce::Font(16.0f, juce::Font::bold));
            g.drawText(getSectionTitle(kind), header, juce::Justification::centredLeft, false);
        }

        y += sectionHeaderHeight;

        if (count == 0 && showInlineEmpty)
        {
            auto emptyArea = juce::Rectangle<int>(0, y, getWidth() - scrollbarWidth, emptyRowHeight).reduced(4);
            if (emptyArea.getBottom() >= 0 && emptyArea.getY() <= getHeight())
            {
                g.setColour(theme.colour("recent-clips.empty-row.background"));
                g.fillRoundedRectangle(emptyArea.toFloat(), 8.0f);
                g.setColour(theme.colour("recent-clips.empty-row.text"));
                g.setFont(juce::Font(13.0f, juce::Font::bold));
                g.drawText(kind == ItemKind::clip ? "No audio files yet" : "No saved patterns yet",
                           emptyArea.reduced(10, 8), juce::Justification::centredLeft, false);
            }
            y += emptyRowHeight + sectionGap;
            return;
        }

        for (int i = 0; i < count; ++i)
        {
            auto area = getItemBounds(kind, i);
            if (area.getBottom() < 0 || area.getY() > getHeight())
                continue;

            const auto prefix = kind == ItemKind::pattern ? "recent-clips.pattern-item." : "recent-clips.clip-item.";
            const auto& name = kind == ItemKind::pattern ? state.savedPatterns[(size_t) i].name
                                                         : state.recentClips[(size_t) i].name;

            g.setColour(theme.colour(prefix + juce::String("background")));
            g.fillRoundedRectangle(area.toFloat(), 8.0f);
            g.setColour(theme.colour(prefix + juce::String("outline")));
            g.drawRoundedRectangle(area.toFloat().reduced(0.5f), 8.0f, 1.2f);
            g.setColour(theme.colour("recent-clips.item.text"));
            g.setFont(juce::Font(13.0f, juce::Font::bold));
            g.drawText(name, area.reduced(10, 8), juce::Justification::centredLeft, true);
        }

        y += count * rowHeight + sectionGap;
    };

    drawSection(ItemKind::clip, (int) state.recentClips.size(), hasPatterns);
    drawSection(ItemKind::pattern, (int) state.savedPatterns.size(), hasClips);

    if (getMaxScroll() > 0)
    {
        g.setColour(theme.colour("recent-clips.scrollbar.track"));
        g.fillRoundedRectangle(getScrollbarTrackBounds(), 4.0f);
        g.setColour(theme.colour("recent-clips.scrollbar.thumb"));
        auto thumb = getScrollbarThumbBounds();
        g.fillRoundedRectangle(thumb, 4.0f);
        g.setColour(theme.colour("recent-clips.scrollbar.outline"));
        g.drawRoundedRectangle(thumb, 4.0f, 1.0f);
    }
}

void RecentClipsComponent::mouseDown(const juce::MouseEvent& e)
{
    if (getScrollbarThumbBounds().contains(e.position))
    {
        draggingScrollbar = true;
        scrollbarDragOffset = e.position.y - getScrollbarThumbBounds().getY();
        return;
    }

    const auto item = getItemAt(e.position);

    if (e.mods.isPopupMenu())
    {
        if (item.has_value())
            showItemMenu(*item);
        dragItem.reset();
        return;
    }

    dragItem = item;
}

void RecentClipsComponent::mouseDoubleClick(const juce::MouseEvent& e)
{
    if (const auto item = getItemAt(e.position))
        promptRenameItem(*item);
}

void RecentClipsComponent::mouseMove(const juce::MouseEvent& e)
{
    if (const auto item = getItemAt(e.position))
    {
        setTooltip(getTooltipForItem(*item));
        return;
    }

    setTooltip({});
}

void RecentClipsComponent::mouseDrag(const juce::MouseEvent& e)
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

    if (! dragItem.has_value()) return;

    const bool isPattern = dragItem->kind == ItemKind::pattern;
    const auto count = isPattern ? (int) state.savedPatterns.size() : (int) state.recentClips.size();
    if (dragItem->index < 0 || dragItem->index >= count) return;

    const auto i = (size_t) dragItem->index;
    const int assetId = isPattern ? state.savedPatterns[i].assetId : state.recentClips[i].assetId;
    const auto& name = isPattern ? state.savedPatterns[i].name : state.recentClips[i].name;
    if (assetId <= 0) return;

    if (auto* container = juce::DragAndDropContainer::findParentDragContainerFor(this))
    {
        container->startDragging("recent:" + juce::String(assetId), this,
                                 createDragPreviewImage(name, isPattern, theme),
                                 false, nullptr, &e.source);
        dragItem.reset();
    }
}

void RecentClipsComponent::mouseUp(const juce::MouseEvent&)
{
    draggingScrollbar = false;
    dragItem.reset();
}

void RecentClipsComponent::mouseExit(const juce::MouseEvent&)
{
    setTooltip({});
}

void RecentClipsComponent::mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
    if (wheel.deltaY != 0.0f)
        scrollBy(-wheel.deltaY * 72.0f);
}

std::optional<RecentClipsComponent::ItemRef> RecentClipsComponent::getItemAt(juce::Point<float> point) const
{
    for (int i = 0; i < (int) state.recentClips.size(); ++i)
    {
        if (getItemBounds(ItemKind::clip, i).toFloat().contains(point))
            return ItemRef { ItemKind::clip, i };
    }

    for (int i = 0; i < (int) state.savedPatterns.size(); ++i)
    {
        if (getItemBounds(ItemKind::pattern, i).toFloat().contains(point))
            return ItemRef { ItemKind::pattern, i };
    }

    return std::nullopt;
}

juce::String RecentClipsComponent::getTooltipForItem(const ItemRef& item) const
{
    if (item.kind == ItemKind::pattern)
        return "Double-click or right-click to rename, right-click to edit";

    return "Double-click or right-click to rename";
}

int RecentClipsComponent::getContentHeight() const
{
    const bool hasClips = ! state.recentClips.empty();
    const bool hasPatterns = ! state.savedPatterns.empty();

    if (! hasClips && ! hasPatterns)
        return getHeight();

    const int clipSectionBodyHeight = hasClips ? (int) state.recentClips.size() * rowHeight
                                               : (hasPatterns ? emptyRowHeight : 0);
    const int patternSectionBodyHeight = hasPatterns ? (int) state.savedPatterns.size() * rowHeight
                                                     : (hasClips ? emptyRowHeight : 0);

    return juce::jmax(rowHeight,
                      sectionHeaderHeight + clipSectionBodyHeight
                          + sectionGap
                          + sectionHeaderHeight + patternSectionBodyHeight);
}

int RecentClipsComponent::getMaxScroll() const
{
    return juce::jmax(0, getContentHeight() - getHeight());
}

juce::Rectangle<float> RecentClipsComponent::getScrollbarTrackBounds() const
{
    return { static_cast<float>(getWidth() - scrollbarWidth), 2.0f, static_cast<float>(scrollbarWidth - 2), static_cast<float>(getHeight() - 4) };
}

juce::Rectangle<float> RecentClipsComponent::getScrollbarThumbBounds() const
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

void RecentClipsComponent::setScrollOffset(int newOffset)
{
    scrollOffset = juce::jlimit(0, getMaxScroll(), newOffset);
    repaint();
}

void RecentClipsComponent::scrollBy(float deltaY)
{
    setScrollOffset(scrollOffset + static_cast<int>(std::round(deltaY)));
}

void RecentClipsComponent::promptRenameItem(const ItemRef& item)
{
    const bool isPattern = item.kind == ItemKind::pattern;
    const auto count = isPattern ? (int) state.savedPatterns.size() : (int) state.recentClips.size();
    if (! juce::isPositiveAndBelow(item.index, count)) return;

    const auto i = (size_t) item.index;
    const int assetId = isPattern ? state.savedPatterns[i].assetId : state.recentClips[i].assetId;
    const auto currentName = isPattern ? state.savedPatterns[i].name : state.recentClips[i].name;
    if (assetId <= 0) return;

    const auto renameDialogButton = theme.colour ("ui.button.background");
    auto* renameWindow = new juce::AlertWindow("Rename Item", "Enter a new name", juce::AlertWindow::NoIcon);
    renameWindow->setLookAndFeel (&getRenameDialogLookAndFeel());
    renameWindow->setColour (juce::AlertWindow::backgroundColourId, renameDialogBackground);
    renameWindow->setColour (juce::AlertWindow::textColourId, juce::Colours::white);
    renameWindow->setColour (juce::TextButton::buttonColourId, renameDialogButton);
    renameWindow->setColour (juce::TextButton::textColourOffId, juce::Colours::white);
    renameWindow->setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff232329));
    renameWindow->setColour (juce::TextEditor::outlineColourId, juce::Colours::white.withAlpha (0.25f));
    renameWindow->setColour (juce::TextEditor::focusedOutlineColourId, juce::Colours::white.withAlpha (0.75f));
    renameWindow->setColour (juce::TextEditor::textColourId, juce::Colours::white);
    renameWindow->setColour (juce::TextEditor::highlightColourId, renameDialogButton.withAlpha (0.35f));
    renameWindow->setColour (juce::TextEditor::highlightedTextColourId, juce::Colours::white);
    renameWindow->addTextEditor("itemName", currentName, "Name");
    renameWindow->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
    renameWindow->addButton("Rename", 1, juce::KeyPress(juce::KeyPress::returnKey));

    juce::Component::SafePointer<RecentClipsComponent> safeThis(this);
    juce::Component::SafePointer<juce::AlertWindow> safeWindow(renameWindow);
    renameWindow->enterModalState(true,
                                  juce::ModalCallbackFunction::create([safeThis, safeWindow, assetId](int result)
                                  {
                                      if (result != 1 || safeThis == nullptr || safeWindow == nullptr)
                                          return;

                                      if (auto* editor = safeWindow->getTextEditor("itemName"))
                                      {
                                          const auto renamed = editor->getText().trim();
                                          if (renamed.isNotEmpty() && safeThis->onAssetRenameRequested)
                                              safeThis->onAssetRenameRequested(assetId, renamed);
                                      }
                                  }),
                                  true);
}

void RecentClipsComponent::showItemMenu(const ItemRef& item)
{
    juce::PopupMenu menu;
    menu.setLookAndFeel (popupMenuLookAndFeel.get());
    menu.addItem(1, "Rename");

    if (item.kind == ItemKind::pattern)
        menu.addItem(2, "Edit");

    const auto target = getItemBounds(item.kind, item.index);
    juce::Component::SafePointer<RecentClipsComponent> safeThis(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea(localAreaToGlobal(target)),
                       [safeThis, item](int result)
                       {
                           if (safeThis == nullptr || result == 0)
                               return;

                           if (result == 1)
                           {
                               safeThis->promptRenameItem(item);
                               return;
                           }

                           if (result == 2
                               && item.kind == ItemKind::pattern
                               && juce::isPositiveAndBelow(item.index, (int) safeThis->state.savedPatterns.size())
                               && safeThis->onPatternEditRequested)
                           {
                               safeThis->onPatternEditRequested(safeThis->state.savedPatterns[(size_t) item.index].assetId);
                           }
                       });
}
