# Project Overview

- Chroma is a simple digital audio workstation built in C++ with the JUCE framework and JIVE library. MSVC and CMake have been used to configure and compile the project. Major features implemented over the course of four sprints include the following: audio recording, audio playback, custom plugins, plugin hosting, piano roll, mix/master tracks internal FX & routing, and a simple to use UI/UX experience.

## Audio Engine Methodologies (Recording/Playback/Mix & Mastering):
- The core of the project is built around the JUCE real-time callback architecture. When a
user records in Chroma, the engine begins sampling audio at a set buffer rate and
attaching the buffers to a temporary buffer until the user is done recording. Once the user
is complete, the aggregation of buffers is what is saved as an audio clip and is what is
placed in the timeline of the armed track and in the recent clips section. With playback,
all mixing tracks provide clip audio, internal FX, and additional plugins FX specific to
the track and get routed to the master track. The master track is where the final buffer is
outputted to the desired audio device.


## Plugins (Internal & Third-Party):
- Chroma supports the ability for third party plugins through the JUCE plugin hosting API.
Chroma scans the ExternalPlugins directory for VST3 formatted files, recognizes plugins,
and allows for selection in the mix and master track’s FX list. Chroma internally includes
a proprietary compressor, gan/pan filter, lowpass/highpass filter, and a sevenband EQ. In
further detail the sevenband EQ is a plugin that allows for the user to adjust seven
specific frequency ranges for a more precise control over the sound.


## Piano Roll:
- The Piano Roll within Chroma offers the users to create/edit MIDI data in order to
construct audio patterns. Each note on the Piano Roll internally represents a trigger. This
trigger allows Chroma to know what note to generate. It first sends the request data to the
selected instrument plugin (no internal default instruments, users have to select an
instrument). Next, based on the request data, the instrument generates an audio buffer.
For Chroma, this happens on the creation of the pattern, after the notes have been placed
and saved. The user is then able to listen to the pattern on the timeline. 


## UI/UX:
- Chroma’s GUI was designed using a combination of basic JUCE GUI components as
well as JIVE components, all using a JIVE stylesheet. JIVE utilizes json and xml files for
styles and layouts respectively, but can be applied inline within component files as well.
- The timeline/track section (middle) is used to measure the length of the project and
manipulate the different track layers using saved audio recordings and piano roll patterns.
- The recent clips section (right) is used to store and manipulate audio recordings and piano
roll patterns.
- The piano roll window is used to create audio patterns, apply instruments to those
patterns, and to save them to the recent clips section.
- The header section is used primarily for audio playback, but also includes other features
such as saving project/audio files, changing settings, and setting program themes. 


## Future Development

- MIDI Support
- Additional Custom Plugins
- Additional Audio Clip Editing Support
- Help Guide

## Download The Latest Release With The Link Below

-https://github.com/reid-json/DAW/releases/download/v1.0.3/Chroma.zip