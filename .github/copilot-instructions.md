# Project Guidelines

## Code Style
- Use C++17 and JUCE style consistent with existing files in `src/`.
- Keep real-time audio paths safe: avoid heap allocation, blocking calls, file/network I/O, and locks inside audio callbacks.
- Prefer clear, minimal changes in `MainComponent`, `SpectrumAnalyser`, and `FrequencyToColour`; keep rendering/audio responsibilities separated.

## Architecture
- `src/Main.cpp`: JUCE application/bootstrap and window setup.
- `src/MainComponent.h/.cpp`: UI, audio device integration, timer-driven rendering, settings panel behavior.
- `src/SpectrumAnalyser.h/.cpp`: FFT pipeline and analysis state.
- `src/FrequencyToColour.h`: frequency-to-visible-spectrum colour mapping.
- Treat `build/` and `build/SpectrumToColour_artefacts/` as generated outputs; do not hand-edit generated files.

## Build and Test
- Prerequisites: macOS + Xcode tools, CMake 3.22+, JUCE available at `$HOME/Dev/JUCE` (see `CMakeLists.txt`).
- Configure and build from repo root:
  - `cmake -S . -B build`
  - `cmake --build build --parallel`
- Launch app bundle:
  - `open build/SpectrumToColour_artefacts/Sound\ Lighter.app`
- No formal test suite is currently defined; validate changes by building successfully and smoke-testing audio input/rendering paths.

## Conventions
- Keep FFT/color mapping logic deterministic and side-effect free where possible.
- Respect existing slider/setting semantics when adjusting UI behavior.
- Avoid broad refactors unless requested; preserve public behavior and current control flow.
- Link to details instead of duplicating docs: see `README.md` for features and architecture summary.
