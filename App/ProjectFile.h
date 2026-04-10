#pragma once

#include <juce_core/juce_core.h>
#include "../GUI/dawstate.h"
#include "../Audio_Engine/Arrangement/ArrangementState.h"

bool saveProject(const juce::File& file,
                 const DAWState& dawState,
                 const ArrangementState& arrangement);

bool loadProject(const juce::File& file,
                 DAWState& dawState,
                 ArrangementState& arrangement);
