# Spectrum → Colour

A JUCE audio visualiser that maps the audio frequency spectrum to the visible light spectrum.

**Low frequencies → long wavelengths (red/orange)**  
**High frequencies → short wavelengths (blue/violet)**

---

## Features
- Real-time FFT spectrum analyser (2048-point, Hann window)
- Each frequency bar coloured by its corresponding visible light wavelength
- Waterfall display showing frequency history over time
- Visible spectrum reference strip at the top
- Toggle between "natural" (low=violet) and "warm bass" (low=red) direction

## Prerequisites
- macOS with Xcode installed
- CMake 3.22+ (`brew install cmake`)
- JUCE cloned to `~/Dev/JUCE`

## Build

```bash
mkdir build && cd build
cmake ..
cmake --build . --parallel
open SpectrumToColour.app
```

## Project Structure

```
src/
├── Main.cpp              # App entry point
├── MainComponent.h/.cpp  # UI + audio, rendering
├── SpectrumAnalyser.h    # FFT, lock-free FIFO
└── FrequencyToColour.h   # Hz → wavelength → RGB
```
