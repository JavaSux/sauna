# Sauna

Sauna is a spatial audio VST plugin adapting Valve's Steam Audio, created with [JUCE](https://github.com/juce-framework/JUCE) Projucer.

## Development

This project uses the `develop` branch of JUCE.
To edit, launch Projucer, selecting `Open in IDE`, and then editing the `sauna_SharedCode` project in the Visual Studio solution.

It expects the Steam Audio C API extracted into `steamaudio`, and JUCE library code placed by Projucer at `JuceLibraryCode`.


## Quirks

All project configuration is administered through Projucer, including adding new source files and resources.
Binary resources are compiled by Projucer into a `BinaryData` class, and if stale, must be refreshed by saving the Projucer project again.
