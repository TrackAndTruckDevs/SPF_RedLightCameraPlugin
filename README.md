<h1 align="center">SPF Red Light Camera Plugin</h1>

<p align="center">
    <a href="https://github.com/TrackAndTruckDevs/SPF_RedLightCameraPlugin/releases/latest/" target="_blank" title="SPF Red Light Camera Plugin releas"><img alt="GitHub Release" src="https://img.shields.io/github/v/release/TrackAndTruckDevs/SPF_RedLightCameraPlugin"></a>
    <a href="/LICENSE" title="SPF Red Light Camera Plugin license"><img alt="GitHub License" src="https://img.shields.io/github/license/TrackAndTruckDevs/SPF_RedLightCameraPlugin"></a>
</p>

<p align="center">
    <a href="https://www.patreon.com/TrackAndTruckDevs" target="_blank" title="Support us on Patreon"><img alt="Patreon" src="https://img.shields.io/badge/patreon-Becoming a patron-3404021712?style=flat&logo=patreon"></a>
    <a href="https://github.com/TrackAndTruckDevs/SPF_RedLightCameraPlugin/stargazers" title="Liked it? Starred"><img src="https://img.shields.io/github/stars/TrackAndTruckDevs/SPF_RedLightCameraPlugin?style=flat&logo=github" alt="Stars" /></a>
    <a href="https://youtube.com/@TrackAndTruck" target="_blank" title="Subscribe to our channel"><img alt="Youtube" src="https://img.shields.io/badge/youtube-subscribe-orange?logo=youtube&style=flat"></a>
</p>

---

This plugin automatically takes a screenshot from a unique, customized camera angle every time you get a ticket in ATS and ETS2 for running a red light.

## Tutorial Project

This plugin was created as a hands-on example for a video tutorial series on how to use the SPF Framework. It demonstrates a wide range of the framework's capabilities.

**[Watch the full tutorial on YouTube](https://youtu.be/96Dfu6V9NF0)**

This project serves as a practical example for:
- **Manifest System**: Defining plugin metadata, configuration policies, required hooks, and default settings for logging and localization.
- **UI Metadata**: Creating automatic UI sliders with custom ranges, titles, and descriptions.
- **Lifecycle Management**: Correctly using `OnLoad`, `OnActivated`, `OnUpdate`, and `OnUnload` for plugin initialization and cleanup.
- **Logging & Formatting API**: Writing formatted, contextual log messages for easier debugging.
- **Configuration API**: Reading settings at startup (`OnLoad`) and reacting to live changes from the UI (`OnSettingChanged`).
- **Event-Driven Logic**: Using the `OnGameplayEvents` callback to react to a specific game event (a `player.fined` event for a `red_signal`).
- **Camera API**:
    - Switching between camera types (`SwitchTo`).
    - Reading and setting camera position and orientation.
    - Calculating coordinates and vectors for precise camera placement.
    - Setting the camera's Field of View (FOV).
    - Checking the current camera type to create a smooth, stutter-free live preview.
- **UI API**:
    - Registering custom windows and draw callbacks.
    - Drawing simple shapes (`AddRectFilled`) to create a full-screen visual effect (camera flash).
    - Controlling window visibility programmatically.
- **Telemetry API**: Reading the truck's world position and game timestamps to create unique screenshot filenames.
- **Game Console API**: Executing game console commands from the plugin to trigger a screenshot.

## Installation

### Prerequisites
You **must** have the SPF Framework installed for this plugin to work.
- **[Download the SPF-Framework here](https://github.com/TrackAndTruckDevs/SPF-Framework)**

### Steps
1. If you haven't already, download and install the SPF Framework according to its instructions.
2. Download the latest release of this plugin.
3. You will have a folder named `SPF_RedLightCamera`. Copy this **entire folder** into your game's `\bin\win_x64\plugins\spfPlugins\` directory.
4. The final folder structure should look like this:

   ```
   ...your_game_root\bin\win_x64\plugins\spfPlugins\
   └───SPF_RedLightCamera
       │   SPF_RedLightCamera.dll
       │
       └───localization
               en.json
   ```

## How to Use

1. Start the game.
2. Press the `DELETE` key to open the main SPF Framework window.
3. In the plugin list, find `SPF_RedLightCamera` and enable it.
4. If you wish to adjust the camera, go to the "Plugin Settings" tab, select `SPF_RedLightCamera`, and use the sliders to configure the position (distance, height, FOV). The changes are applied instantly in a live preview.

## Important Note

❗️ **This plugin only works if traffic offense fines are enabled in the game.** If you have fines turned off in your game settings, the game will not generate a "fine" event, and the plugin will not take a screenshot.

## Screenshot Location

Screenshots are saved to the game's default screenshot folder, which is typically located at:
`Documents\<Your Game Name>\screenshot`
