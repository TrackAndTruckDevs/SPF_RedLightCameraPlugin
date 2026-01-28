/**
 * @file SPF_RedLightCamera.cpp
 * @brief The main implementation file for the SPF_RedLightCamera.
 * @details This file contains the minimal implementation for a plugin to be loaded
 * and recognized by the SPF framework. It serves as a basic template for new plugins,
 * with clear explanations and commented-out sections for optional features.
 */

#include "SPF_RedLightCamera.hpp" // Always include your own header first
#define _USE_MATH_DEFINES
#include <cmath>
#include <cstring>                // For C-style string manipulation functions like strncpy_s.
#include <string>                 // For std::string and std::to_string

namespace SPF_RedLightCamera
{

    // =================================================================================================
    // 1. Constants & Global State
    // =================================================================================================

    /**
     * @brief A constant for the plugin's name.
     * @details This MUST match the name used in `Cfg_GetContext` calls for various APIs
     * and the plugin's directory name.
     */
    const char *PLUGIN_NAME = "SPF_RedLightCamera";

    /**
     * @brief The single, global instance of the plugin's context.
     * @details This is the central point for accessing all plugin state.
     */
    PluginContext g_ctx;

    // =================================================================================================
    // 2. Manifest Implementation
    // =================================================================================================

    void BuildManifest(SPF_Manifest_Builder_Handle *h, const SPF_Manifest_Builder_API *api)
    {
        // This function defines all the metadata for your plugin. The framework calls this
        // function *before* loading your plugin DLL to understand what it is.

        // --- 2.1. Plugin Information ---
        // This section provides the basic identity of your plugin.
        {
            api->Info_SetName(h, PLUGIN_NAME);
            api->Info_SetVersion(h, "1.0.2");
            api->Info_SetMinFrameworkVersion(h, "1.1.0");
            api->Info_SetAuthor(h, "Track'n'Truck Devs");
            api->Info_SetDescriptionLiteral(h, "Captures red light violation screenshots. Automatically triggers a camera at a custom distance, height, and FOV, with live in-game UI adjustments for the perfect shot.");

            api->Info_SetEmail(h, "mailto:spf.framework@gmail.com");
            api->Info_SetYoutubeUrl(h, "https://www.youtube.com/@TrackAndTruck");
            api->Info_SetPatreonUrl(h, "https://www.patreon.com/TrackAndTruckDevs");
        }

        // --- 2.2. Configuration Policy ---
        {
            api->Policy_SetAllowUserConfig(h, true);
            api->Policy_AddConfigurableSystem(h, "settings");
            api->Policy_AddRequiredHook(h, "GameConsole");
        }

        // --- 2.3. Custom Settings Defaults ---
        api->Settings_SetJson(h, R"json(
        {
            "distance_forward": 25.0,
            "height_above": 4.0,
            "field_of_view": 70.0
        }
    )json");

        // --- 2.4. Default Settings for Framework Systems ---

        // Logging
        api->Defaults_SetLogging(h, "info", true);

        // Localization
        api->Defaults_SetLocalization(h, "en");

        // UI
        {
            api->Defaults_AddWindow(h, "FlashWindow", false, false, 0, 0, 0, 0, false, false);
        }

        // =============================================================================================
        // 2.5. Metadata for UI Display (Optional)
        // =============================================================================================

        // --- Custom Settings Metadata ---
        
        auto AddSliderMeta = [&](const char *key, const char *title, const char *desc, float min, float max, const char *format)
        {
            std::string params = "{ \"min\": " + std::to_string(min) + 
                                 ", \"max\": " + std::to_string(max) + 
                                 ", \"format\": \"" + format + "\" }";
            api->Meta_AddCustomSetting(h, key, title, desc, "slider", params.c_str(), false);
        };

        { //--- Metadata for "distance_forward" ---
            AddSliderMeta("distance_forward", "Setting.DistanceForward.Title", "Setting.DistanceForward.Description", -100.0f, 100.0f, "%0.1f");
        }
        { //--- Metadata for "height_above" ---
            AddSliderMeta("height_above", "Setting.HeightAbove.Title", "Setting.HeightAbove.Description", -100.0f, 100.0f, "%0.1f");
        }
        { //--- Metadata for "field_of_view" ---
            AddSliderMeta("field_of_view", "Setting.FieldOfView.Title", "Setting.FieldOfView.Description", 0.0f, 120.0f, "%0.1f");
        }
    }

    // =================================================================================================
    // 3. Plugin Lifecycle Implementations
    // =================================================================================================
    // The following functions are the core lifecycle events for the plugin.

    void OnLoad(const SPF_Load_API *load_api)
    {
        // Cache the provided API pointers in our global context.
        g_ctx.loadAPI = load_api;

        // --- Essential API Initialization ---
        // Get and cache the logger and formatting API handles.
        if (g_ctx.loadAPI)
        {
            g_ctx.loggerHandle = g_ctx.loadAPI->logger->Log_GetContext(PLUGIN_NAME);
            g_ctx.formattingAPI = g_ctx.loadAPI->formatting;
            g_ctx.localizationHandle = g_ctx.loadAPI->localization->Loc_GetContext(PLUGIN_NAME);

            // --- Config API Initialization ---
            if (g_ctx.loadAPI->config)
            {
                g_ctx.configHandle = g_ctx.loadAPI->config->Cfg_GetContext(PLUGIN_NAME);
                if (g_ctx.configHandle)
                {
                    const auto config = g_ctx.loadAPI->config;
                    g_ctx.setting_distance_forward = config->Cfg_GetFloat(g_ctx.configHandle, "settings.distance_forward", 25.0f);
                    g_ctx.setting_height_above = config->Cfg_GetFloat(g_ctx.configHandle, "settings.height_above", 4.0f);
                    g_ctx.setting_field_of_view = config->Cfg_GetFloat(g_ctx.configHandle, "settings.field_of_view", 70.0f);
                }
            }

            if (g_ctx.loggerHandle && g_ctx.formattingAPI)
            {
                char log_buffer[256];
                g_ctx.formattingAPI->Fmt_Format(log_buffer, sizeof(log_buffer), "%s has been loaded!", PLUGIN_NAME);
                g_ctx.loadAPI->logger->Log(g_ctx.loggerHandle, SPF_LOG_INFO, log_buffer);
            }
        }

        // --- Optional API Initialization (Uncomment if needed) ---
        // Remember to also uncomment the relevant #include directives in SPF_RedLightCamera.hpp
        // and add corresponding members to the PluginContext struct.

        /*
        // Config API
        // Requires: SPF_Config_API.h
        */

        /*
        // Localization API
        // Requires: SPF_Localization_API.h
        */
       
    }

    void OnActivated(const SPF_Core_API *core_api)
    {
        g_ctx.coreAPI = core_api;

        if (g_ctx.coreAPI)
        {
            g_ctx.cameraAPI = g_ctx.coreAPI->camera;
            g_ctx.gameConsoleAPI = g_ctx.coreAPI->console;
            g_ctx.uiAPI = g_ctx.coreAPI->ui;

                    if (g_ctx.coreAPI->telemetry)
                    {
                        g_ctx.telemetryHandle = g_ctx.coreAPI->telemetry->Tel_GetContext(PLUGIN_NAME);
                    }
                }
            
                if (g_ctx.telemetryHandle && g_ctx.coreAPI && g_ctx.coreAPI->telemetry)
                {
                    g_ctx.gameplayEventsSubscription = g_ctx.coreAPI->telemetry->Tel_RegisterForGameplayEvents(g_ctx.telemetryHandle, OnGameplayEvents, &g_ctx);
                }
        if (g_ctx.loggerHandle && g_ctx.formattingAPI)
        {
            char log_buffer[256];
            g_ctx.formattingAPI->Fmt_Format(log_buffer, sizeof(log_buffer), "%s has been activated!", PLUGIN_NAME);
            g_ctx.loadAPI->logger->Log(g_ctx.loggerHandle, SPF_LOG_INFO, log_buffer);
            g_ctx.formattingAPI->Fmt_Format(log_buffer, sizeof(log_buffer), "Loaded settings: distance=%.1f, height=%.1f, fov=%.1f", g_ctx.setting_distance_forward, g_ctx.setting_height_above, g_ctx.setting_field_of_view);
            g_ctx.loadAPI->logger->Log(g_ctx.loggerHandle, SPF_LOG_INFO, log_buffer);
        }

        // --- Optional API Initialization & Callback Registration (Uncomment if needed) ---
        // Remember to also uncomment the relevant #include directives in SPF_RedLightCamera.hpp
        // and add corresponding members to the PluginContext struct.

        /*
        // Keybinds API
        // Requires: SPF_KeyBinds_API.h
        */

        /*
        // Game Log API
        // Requires: SPF_GameLog_API.h
        */

        /*
        // Telemetry API
        // Requires: SPF_Telemetry_API.h, SPF_TelemetryData.h (uncomment in SPF_RedLightCamera.hpp)
        // 1. Check if the telemetry API is available.
        if (g_ctx.coreAPI && g_ctx.coreAPI->telemetry) {
            // 2. Get the telemetry context handle for this plugin. This is the main handle
            //    that manages all telemetry subscriptions for this plugin.
            g_ctx.telemetryHandle = g_ctx.coreAPI->telemetry->GetContext(PLUGIN_NAME);

            // 3. Register callbacks for the telemetry data you need.
            //    The returned handles are managed by the framework and will be automatically
            //    cleaned up when the plugin unloads. Storing them in g_ctx is optional but good practice.
            //    The final parameter (e.g., &g_ctx) is user data passed to the callback.
            if (g_ctx.telemetryHandle) {
                const auto tel = g_ctx.coreAPI->telemetry; // Shortcut for Telemetry API
                g_ctx.gameStateSubscription = tel->RegisterForGameState(g_ctx.telemetryHandle, OnGameState, &g_ctx);
                g_ctx.timestampsSubscription = tel->RegisterForTimestamps(g_ctx.telemetryHandle, OnTimestamps, &g_ctx);
                g_ctx.commonDataSubscription = tel->RegisterForCommonData(g_ctx.telemetryHandle, OnCommonData, &g_ctx);
                g_ctx.truckConstantsSubscription = tel->RegisterForTruckConstants(g_ctx.telemetryHandle, OnTruckConstants, &g_ctx);
                g_ctx.trailerConstantsSubscription = tel->RegisterForTrailerConstants(g_ctx.telemetryHandle, OnTrailerConstants, &g_ctx);
                g_ctx.truckDataSubscription = tel->RegisterForTruckData(g_ctx.telemetryHandle, OnTruckData, &g_ctx);
                g_ctx.trailersSubscription = tel->RegisterForTrailers(g_ctx.telemetryHandle, OnTrailers, &g_ctx);
                g_ctx.jobConstantsSubscription = tel->RegisterForJobConstants(g_ctx.telemetryHandle, OnJobConstants, &g_ctx);
                g_ctx.jobDataSubscription = tel->RegisterForJobData(g_ctx.telemetryHandle, OnJobData, &g_ctx);
                g_ctx.navigationDataSubscription = tel->RegisterForNavigationData(g_ctx.telemetryHandle, OnNavigationData, &g_ctx);
                g_ctx.controlsSubscription = tel->RegisterForControls(g_ctx.telemetryHandle, OnControls, &g_ctx);
                g_ctx.specialEventsSubscription = tel->RegisterForSpecialEvents(g_ctx.telemetryHandle, OnSpecialEvents, &g_ctx);
                g_ctx.gameplayEventsSubscription = tel->RegisterForGameplayEvents(g_ctx.telemetryHandle, OnGameplayEvents, &g_ctx);
                g_ctx.gearboxConstantsSubscription = tel->RegisterForGearboxConstants(g_ctx.telemetryHandle, OnGearboxConstants, &g_ctx);
            }
        }
        */

        /*
        // Hooks API
        // Requires: SPF_Hooks_API.h
        */

        /*
        // Game Console API
        // Requires: SPF_GameConsole_API.h
        */

        /*
        // Virtual Input API
        // Requires: SPF_VirtInput_API.h
        */

        /*
        // Camera API
        // Requires: SPF_Camera_API.h
        */
    }

    void OnUpdate()
    {
        if (!g_ctx.sequence_active)
        {
            return;
        }

        g_ctx.sequence_frame_counter++;

        switch (g_ctx.sequence_frame_counter)
        {
        case 1:
            // --- Frame 1: Save original camera state and position new camera ---
            // 1. Save the current camera type
            if (g_ctx.cameraAPI && g_ctx.cameraAPI->Cam_GetCurrentCamera)
            {
                g_ctx.cameraAPI->Cam_GetCurrentCamera(&g_ctx.originalCameraType);
            }
            else
            {
                // Log an error if Camera API is not available or GetCurrentCamera is NULL.
                if (g_ctx.loggerHandle)
                    g_ctx.loadAPI->logger->Log(g_ctx.loggerHandle, SPF_LOG_ERROR, "OnUpdate (Frame 1): Camera API or GetCurrentCamera function is not available.");
                g_ctx.sequence_active = false; // Abort sequence if we can't get current camera.
                return;
            }

            // 2. Save Yaw and Pitch ONLY for SPF_CAMERA_INTERIOR.
            if (g_ctx.originalCameraType == SPF_CAMERA_INTERIOR)
            {
                if (g_ctx.cameraAPI->Cam_GetInteriorHeadRot)
                {
                    g_ctx.cameraAPI->Cam_GetInteriorHeadRot(&g_ctx.originalYaw, &g_ctx.originalPitch);
                    if (g_ctx.loggerHandle)
                        g_ctx.loadAPI->logger->Log(g_ctx.loggerHandle, SPF_LOG_INFO, "OnUpdate (Frame 1): Saved Interior camera Yaw/Pitch.");
                }
                else
                {
                    if (g_ctx.loggerHandle)
                        g_ctx.loadAPI->logger->Log(g_ctx.loggerHandle, SPF_LOG_WARN, "OnUpdate (Frame 1): GetInteriorHeadRot is NULL, cannot save Interior camera orientation.");
                }
            }
            else
            {
                // For other camera types, we are not saving orientation.
                if (g_ctx.loggerHandle)
                {
                    char log_buffer[128];
                    g_ctx.formattingAPI->Fmt_Format(log_buffer, sizeof(log_buffer), "OnUpdate (Frame 1): Original came type %d, orientation not saved for restoration.", g_ctx.originalCameraType);
                    g_ctx.loadAPI->logger->Log(g_ctx.loggerHandle, SPF_LOG_INFO, log_buffer);
                }
            }
            g_ctx.flash_alpha = 1.0f;
            // 3. Position and orient the red light camera. This function will switch to SPF_CAMERA_DEVELOPER_FREE internally.
            PositionAndOrientRedLightCamera();

            break;
        case 2:
            // --- Frame 2: Take a uniquely named screenshot ---
            if (g_ctx.gameConsoleAPI && g_ctx.coreAPI && g_ctx.coreAPI->telemetry && g_ctx.telemetryHandle && g_ctx.formattingAPI)
            {
                // 1. Get current truck and time data for the filename.
                SPF_TruckData truck_data;
                g_ctx.coreAPI->telemetry->Tel_GetTruckData(g_ctx.telemetryHandle, &truck_data, sizeof(SPF_TruckData));

                SPF_Timestamps timestamps;
                g_ctx.coreAPI->telemetry->Tel_GetTimestamps(g_ctx.telemetryHandle, &timestamps, sizeof(SPF_Timestamps));

                const auto &world_pos = truck_data.world_placement.position;
                const uint64_t sim_time = timestamps.simulation;

                // 2. Format the command string to create a unique filename.
                // Format: "screenshot red_light_X<coord>_Y<coord>_Z<coord>_T<time>"
                char command_buffer[256];
                g_ctx.formattingAPI->Fmt_Format(
                    command_buffer,
                    sizeof(command_buffer),
                    "screenshot red_light_X%d_Y%d_Z%d_T%llu",
                    (int)world_pos.x,
                    (int)world_pos.y,
                    (int)world_pos.z,
                    sim_time);

                // 3. Execute the command via the game console.
                g_ctx.gameConsoleAPI->GCon_ExecuteCommand(command_buffer);

                // 4. Log the action for debugging.
                if (g_ctx.loggerHandle)
                {
                    char log_buffer[512];
                    g_ctx.formattingAPI->Fmt_Format(log_buffer, sizeof(log_buffer), "OnUpdate (Frame 2): Executed nd: %s", command_buffer);
                    g_ctx.loadAPI->logger->Log(g_ctx.loggerHandle, SPF_LOG_INFO, log_buffer);
                }
            }
            break;
        case 3:
            // --- Frame 3: Restore the original camera and end the sequence ---
            if (g_ctx.cameraAPI)
            {
                // 1. Switch back to the camera type that was active before the sequence started.
                g_ctx.cameraAPI->Cam_SwitchTo(g_ctx.originalCameraType);
                break;
            case 4:
                g_ctx.flash_alpha = 0.7f;
                break;
            case 5:
                // 2. If the original camera was an interior camera, restore its saved Yaw and Pitch.
                if (g_ctx.originalCameraType == SPF_CAMERA_INTERIOR)
                {
                    if (g_ctx.cameraAPI->Cam_SetInteriorHeadRot)
                    {
                        g_ctx.cameraAPI->Cam_SetInteriorHeadRot(g_ctx.originalYaw, g_ctx.originalPitch);
                    }
                }
            }
            else
            {
                if (g_ctx.loggerHandle)
                    g_ctx.loadAPI->logger->Log(g_ctx.loggerHandle, SPF_LOG_ERROR, "OnUpdate  (Frame 3): Camera API not available, cannot restore camera.");
            }
            g_ctx.flash_alpha = 0.5f;
            break;
        case 6:
            g_ctx.flash_alpha = 0.3f;
            break;
        default:
        {
            // --- Frame 4 / Default: Clean up and end sequence ---

            // Hide the flash window.
            if (g_ctx.uiAPI && g_ctx.flash_window_handle)
            {
                g_ctx.uiAPI->UI_SetVisibility(g_ctx.flash_window_handle, false);
            }

            // Reset all state flags and counters.
            g_ctx.is_flash_active = false;
            g_ctx.flash_alpha = 0.0f;
            g_ctx.sequence_active = false;
            g_ctx.sequence_frame_counter = 0;

            if (g_ctx.loggerHandle)
            {
                g_ctx.loadAPI->logger->Log(g_ctx.loggerHandle, SPF_LOG_INFO, "Sequence finished.");
            }
            break;
        }
        }
        // This function is called every frame while the plugin is active.
        // Avoid performing heavy or blocking operations here, as it will directly impact game performance.

        // --- Optional API Usage (Uncomment if needed) ---
        // Remember to also uncomment the relevant #include directives in SPF_RedLightCamera.hpp/SPF_RedLightCamera.cpp
        // and add corresponding members to the PluginContext struct.

        /*
        // Example: Polling Telemetry data
        // Requires: SPF_Telemetry_API.h (and corresponding types in PluginContext)
        */

        /*
        // Example: Simulating Virtual Input (e.g., holding a button)
        // Requires: SPF_VirtInput_API.h (and corresponding types in PluginContext)
        */
    }

    void OnUnload()
    {
        // Perform cleanup. Nullify cached API pointers to prevent use-after-free
        // and ensure a clean shutdown. This is the last chance for cleanup.

        if (g_ctx.loadAPI && g_ctx.loggerHandle && g_ctx.formattingAPI)
        {
            char log_buffer[256];
            g_ctx.formattingAPI->Fmt_Format(log_buffer, sizeof(log_buffer), "%s is being unloaded.", PLUGIN_NAME);
            g_ctx.loadAPI->logger->Log(g_ctx.loggerHandle, SPF_LOG_INFO, log_buffer);
        }

        // --- Optional API Cleanup (Uncomment if needed) ---
        // Example: Unregistering keybinds (often handled by framework, but good practice if explicitly registered).
        // Requires: SPF_KeyBinds_API.h

        // Nullify all cached API pointers and handles.
        g_ctx.coreAPI = nullptr;
        g_ctx.loadAPI = nullptr;
        g_ctx.loggerHandle = nullptr;
        g_ctx.formattingAPI = nullptr;

        // --- Optional Handles (Nullify if used) ---
        // g_ctx.configHandle = nullptr;
        g_ctx.localizationHandle = nullptr;
        // g_ctx.keybindsHandle = nullptr;
        g_ctx.uiAPI = nullptr;
        // g_ctx.mainWindowHandle = nullptr;
        // g_ctx.telemetryHandle = nullptr;
        // g_ctx.hooksAPI = nullptr;
        // g_ctx.gameConsoleAPI = nullptr;
        // g_ctx.virtualDeviceHandle = nullptr;
        // g_ctx.cameraAPI = nullptr;
        // g_ctx.gameLogCallbackHandle = nullptr;
        //
        // // Telemetry Subscriptions (Nullify if used)
        // g_ctx.gameStateSubscription = nullptr;
        // g_ctx.timestampsSubscription = nullptr;
        // g_ctx.commonDataSubscription = nullptr;
        // g_ctx.truckConstantsSubscription = nullptr;
        // g_ctx.trailerConstantsSubscription = nullptr;
        // g_ctx.truckDataSubscription = nullptr;
        // g_ctx.trailersSubscription = nullptr;
        // g_ctx.jobConstantsSubscription = nullptr;
        // g_ctx.jobDataSubscription = nullptr;
        // g_ctx.navigationDataSubscription = nullptr;
        // g_ctx.controlsSubscription = nullptr;
        // g_ctx.specialEventsSubscription = nullptr;
        // g_ctx.gameplayEventsSubscription = nullptr;
        // g_ctx.gearboxConstantsSubscription = nullptr;
    }

    // =================================================================================================
    // 4. Optional Callback Implementations (Commented Out)
    // =================================================================================================
    // Implement these functions if your plugin needs to react to specific events.
    // Remember to also uncomment their prototypes in SPF_RedLightCamera.hpp and register them
    // in OnActivated or OnRegisterUI as appropriate.

    void OnSettingChanged(SPF_Config_Handle* config_handle, const char* keyPath) {
        // A setting has changed. Check the keyPath and use the config_handle
        // with the Config API to get the new value.
        if (!g_ctx.loadAPI || !g_ctx.loadAPI->config) {
            if (g_ctx.loggerHandle) g_ctx.loadAPI->logger->Log(g_ctx.loggerHandle, SPF_LOG_ERROR, "OnSettingChanged: LoadAPI or Config API not available.");
            return;
        }
        const auto config = g_ctx.loadAPI->config; 
    
        if (strcmp(keyPath, "settings.distance_forward") == 0) {
            g_ctx.setting_distance_forward = config->Cfg_GetFloat(config_handle, keyPath, 25.0f);
        } else if (strcmp(keyPath, "settings.height_above") == 0) {
            g_ctx.setting_height_above = config->Cfg_GetFloat(config_handle, keyPath, 4.0f);
        } else if (strcmp(keyPath, "settings.field_of_view") == 0) {
            g_ctx.setting_field_of_view = config->Cfg_GetFloat(config_handle, keyPath, 70.0f);
        }
    
        // Live Preview: Call PositionAndOrientRedLightCamera to immediately apply changes
        PositionAndOrientRedLightCamera();
    }

    /*
    // --- OnRegisterUI Callback ---
    // Requires: SPF_UI_API.h
    */
    void OnRegisterUI(SPF_UI_API *ui_api)
    {
        if (ui_api)
        {
            // 1. Register the drawing function, passing nullptr because g_ctx is global.
            ui_api->UI_RegisterDrawCallback(PLUGIN_NAME, "FlashWindow", RenderFlashWindow, nullptr);

            // 2. Get and store the window handle so we can control it later.
            g_ctx.flash_window_handle = ui_api->UI_GetWindowHandle(PLUGIN_NAME, "FlashWindow");
        }
    }

    /*
    // --- RenderMainWindow Callback (for UI) ---
    // Requires: SPF_UI_API.h
    // This function name should match what you passed to RegisterDrawCallback.
    */

    /*
    // --- OnKeybindAction Callback ---
    // Requires: SPF_KeyBinds_API.h
    // This function name should match what you passed to SPF_KeyBinds_API.Register.
    */

    /*
    // --- OnGameLogMessage Callback ---
    // Requires: SPF_GameLog_API.h
    // This function name should match what you passed to SPF_GameLog_API.RegisterCallback.
    */

    /*
    // --- OnGameWorldReady Callback ---
    // Called once when the game world has been fully loaded. Ideal for initializing
    // hooks or logic that depends on game objects being in memory.
    void OnGameWorldReady() {
        // if (g_ctx.coreAPI && g_ctx.coreAPI->logger) {
        //     g_ctx.coreAPI->logger->Log(g_ctx.loggerHandle, SPF_LOG_INFO, "Game world is ready!");
        // }
    }
    */

    // =================================================================================================
    // 5. Optional Helper Function Implementations (Commented Out)
    // =================================================================================================
    // Implement these functions if your plugin needs internal helper logic.
    // Remember to also uncomment their prototypes in SPF_RedLightCamera.hpp.

    /*
    // --- InitializeVirtualDevice Helper ---
    // Requires: SPF_VirtInput_API.h
    // This function could be called from OnActivated.
    */

    /*
    // --- Game Hook Implementation ---
    // Requires: SPF_Hooks_API.h
    */

    // =================================================================================================
    // 5.1. Optional Telemetry Callback Implementations (Commented Out)
    // =================================================================================================
    // Implement these functions if your plugin subscribes to telemetry data.
    // Remember to also uncomment their prototypes in SPF_RedLightCamera.hpp and register them in OnActivated.
    // Requires: SPF_Telemetry_API.h, SPF_TelemetryData.h (for data structures, uncomment in SPF_RedLightCamera.hpp)

    /*
    void OnGameState(const SPF_GameState* data, void* user_data) {
        // if (!data || !user_data) return;
        // SPF_RedLightCamera::PluginContext* ctx = reinterpret_cast<SPF_RedLightCamera::PluginContext*>(user_data);
        // // Example: Cache the latest game state data
        // // ctx->telemetryDataCache.gameState = *data;
    }
    */

    /*
    void OnTimestamps(const SPF_Timestamps* data, void* user_data) {
        // if (!data || !user_data) return;
        // SPF_RedLightCamera::PluginContext* ctx = reinterpret_cast<SPF_RedLightCamera::PluginContext*>(user_data);
        // // Example: Cache the latest timestamp data
        // // ctx->telemetryDataCache.timestamps = *data;
    }
    */

    /*
    void OnCommonData(const SPF_CommonData* data, void* user_data) {
        // if (!data || !user_data) return;
        // SPF_RedLightCamera::PluginContext* ctx = reinterpret_cast<SPF_RedLightCamera::PluginContext*>(user_data);
        // // Example: Cache the latest common data
        // // ctx->telemetryDataCache.commonData = *data;
    }
    */

    /*
    void OnTruckConstants(const SPF_TruckConstants* data, void* user_data) {
        // if (!data || !user_data) return;
        // SPF_RedLightCamera::PluginContext* ctx = reinterpret_cast<SPF_RedLightCamera::PluginContext*>(user_data);
        // // Example: Cache the latest truck constants
        // // ctx->telemetryDataCache.truckConstants = *data;
    }
    */

    /*
    void OnTrailerConstants(const SPF_TrailerConstants* data, void* user_data) {
        // if (!data || !user_data) return;
        // SPF_RedLightCamera::PluginContext* ctx = reinterpret_cast<SPF_RedLightCamera::PluginContext*>(user_data);
        // // Example: Cache the latest trailer constants
        // // ctx->telemetryDataCache.trailerConstants = *data;
    }
    */

    /*
    void OnTruckData(const SPF_TruckData* data, void* user_data) {
        // if (!data || !user_data) return;
        // SPF_RedLightCamera::PluginContext* ctx = reinterpret_cast<SPF_RedLightCamera::PluginContext*>(user_data);
        // // Example: Cache the latest truck data
        // // ctx->telemetryDataCache.truckData = *data;
    }
    */

    /*
    void OnTrailers(const SPF_Trailer* trailers, uint32_t count, void* user_data) {
        // if (!user_data) return;
        // SPF_RedLightCamera::PluginContext* ctx = reinterpret_cast<SPF_RedLightCamera::PluginContext*>(user_data);
        // // Example: Cache the latest trailers data
        // // ctx->telemetryDataCache.trailers.clear();
        // // if (trailers && count > 0) {
        // //     for (uint32_t i = 0; i < count; ++i) {
        // //         ctx->telemetryDataCache.trailers.push_back(trailers[i]);
        // //     }
        // // }
    }
    */

    /*
    void OnJobConstants(const SPF_JobConstants* data, void* user_data) {
        // if (!data || !user_data) return;
        // SPF_RedLightCamera::PluginContext* ctx = reinterpret_cast<SPF_RedLightCamera::PluginContext*>(user_data);
        // // Example: Cache the latest job constants
        // // ctx->telemetryDataCache.jobConstants = *data;
    }
    */

    /*
    void OnJobData(const SPF_JobData* data, void* user_data) {
        // if (!data || !user_data) return;
        // SPF_RedLightCamera::PluginContext* ctx = reinterpret_cast<SPF_RedLightCamera::PluginContext*>(user_data);
        // // Example: Cache the latest job data
        // // ctx->telemetryDataCache.jobData = *data;
    }
    */

    /*
    void OnNavigationData(const SPF_NavigationData* data, void* user_data) {
        // if (!data || !user_data) return;
        // SPF_RedLightCamera::PluginContext* ctx = reinterpret_cast<SPF_RedLightCamera::PluginContext*>(user_data);
        // // Example: Cache the latest navigation data
        // // ctx->telemetryDataCache.navigationData = *data;
    }
    */

    /*
    void OnControls(const SPF_Controls* data, void* user_data) {
        // if (!data || !user_data) return;
        // SPF_RedLightCamera::PluginContext* ctx = reinterpret_cast<SPF_RedLightCamera::PluginContext*>(user_data);
        // // Example: Cache the latest controls data
        // // ctx->telemetryDataCache.controls = *data;
    }
    */

    /*
    void OnSpecialEvents(const SPF_SpecialEvents* data, void* user_data) {
        // if (!data || !user_data) return;
        // SPF_RedLightCamera::PluginContext* ctx = reinterpret_cast<SPF_RedLightCamera::PluginContext*>(user_data);
        // // Example: Cache the latest special events data
        // // ctx->telemetryDataCache.specialEvents = *data;
    }
    */

    void OnGameplayEvents(const char *event_id, const SPF_GameplayEvents *data, void *user_data)
    {
        if (!event_id || !data || !user_data)
        {
            return;
        }

        if (strcmp(event_id, "player.fined") == 0)
        {
            if (strcmp(data->player_fined.fine_offence, "red_signal") == 0)
            {
                if (g_ctx.sequence_active)
                {
                    return;
                }

                if (g_ctx.loggerHandle && g_ctx.formattingAPI)
                {
                    g_ctx.loadAPI->logger->Log(g_ctx.loggerHandle, SPF_LOG_INFO, "Red signal event caught! Starting sequence.");
                }
                g_ctx.sequence_active = true;
                g_ctx.sequence_frame_counter = 0;

                if (g_ctx.uiAPI && g_ctx.flash_window_handle)
                {
                    g_ctx.is_flash_active = true;
                    g_ctx.uiAPI->UI_SetVisibility(g_ctx.flash_window_handle, true);
                    g_ctx.flash_alpha = 0.0f;
                }
            }
        }
    }

    /*
    void OnGearboxConstants(const SPF_GearboxConstants* data, void* user_data) {
        // if (!data || !user_data) return;
        // SPF_RedLightCamera::PluginContext* ctx = reinterpret_cast<SPF_RedLightCamera::PluginContext*>(user_data);
        // // Example: Cache the latest gearbox constants
        // // ctx->telemetryDataCache.gearboxConstants = *data;
    }
    */

    // --- RenderFlashWindow Function ---
    // This function is called by the UI framework to draw the content of the "FlashWindow".
    void RenderFlashWindow(SPF_UI_API *ui, void *user_data)
    {
        // We only draw if the flash is active, has some transparency, and the UI API is valid.
        if (!g_ctx.is_flash_active || g_ctx.flash_alpha <= 0.0f || !ui)
        {
            return;
        }

        float width, height;
        // Get the current viewport size to draw the rectangle across the entire screen.
        ui->UI_GetViewportSize(&width, &height);

        // Draw a white rectangle that covers the whole screen with the current flash_alpha transparency.
        // The RGBA components (1.0f, 1.0f, 1.0f, g_ctx.flash_alpha) define a white color with variable opacity.
        ui->UI_AddRectFilled(0, 0, width, height, 1.0f, 1.0f, 1.0f, g_ctx.flash_alpha);
    }

    // This function contains the full logic for positioning and orienting the camera.
    void PositionAndOrientRedLightCamera()
    {
        // --- 0. Safety Check ---
        // Ensure all required API pointers and handles are available before proceeding.
        if (!g_ctx.coreAPI || !g_ctx.coreAPI->telemetry || !g_ctx.telemetryHandle || !g_ctx.coreAPI->camera || !g_ctx.loggerHandle || !g_ctx.formattingAPI)
        {
            if (g_ctx.loggerHandle && g_ctx.formattingAPI)
            {
                g_ctx.loadAPI->logger->Log(g_ctx.loggerHandle, SPF_LOG_ERROR, "PositionAndOrientRedLightCamera: Required APIs or handles are not available.");
            }
            return; // Exit if something is missing to prevent a crash.
        }

        // --- 1. Get Truck Data from Telemetry ---
        // Fetch the latest data about the truck's state.
        SPF_TruckData truck_data;
        g_ctx.coreAPI->telemetry->Tel_GetTruckData(g_ctx.telemetryHandle, &truck_data, sizeof(SPF_TruckData));

        // Extract the truck's high-precision world position and heading (rotation).
        const SPF_DVector &truck_world_pos_d = truck_data.world_placement.position; // (double)
        const double heading_rad = truck_data.world_placement.orientation.heading;  // (double)

        // --- 2. Calculate the Camera's Target World Position ---
        // Use the settings loaded from the config file.
        const float distance_forward = g_ctx.setting_distance_forward;
        const float height_above = g_ctx.setting_height_above;

        // Convert the truck's heading from the SCS telemetry system to a standard mathematical angle.
        // SCS telemetry `heading_rad` is a normalized value (0.0 to 1.0) representing a full circle (360 degrees).
        // In SCS, 0.0 `heading` points North (along the +Z axis in SCS world coordinates), and the angle increases clockwise.
        // Standard mathematical functions (cos/sin) expect angles in radians, starting from the +X axis, increasing counter-clockwise.
        // The formula `(1.5 * M_PI) - (2.0 * M_PI * heading_rad)` performs this conversion:
        // 1. `(2.0 * M_PI * heading_rad)`: Converts the normalized SCS heading (0-1) into full radians (0-2*PI).
        // 2. `(1.5 * M_PI) - ...`: Adjusts for the coordinate system difference (SCS Z-forward vs. Math X-right, and clockwise vs. counter-clockwise).
        const double phi = (1.5 * M_PI) - (2.0 * M_PI * heading_rad);

        // Calculate the components of a normalized direction vector based on the angle.
        const double dir_x = cos(phi);
        const double dir_z = sin(phi);

        // Calculate the offset vector from the truck in world coordinates.
        // This vector points from the truck to where the camera should be.
        const SPF_DVector world_offset_from_truck = {
            dir_x * distance_forward,
            (double)height_above,
            dir_z * distance_forward};

        // Calculate the final target world position for the camera by adding the offset to the truck's position.
        const SPF_DVector cam_target_world_pos = {
            truck_world_pos_d.x + world_offset_from_truck.x,
            truck_world_pos_d.y + world_offset_from_truck.y,
            truck_world_pos_d.z + world_offset_from_truck.z};

        // --- 3. Switch to Free Camera (if needed) ---
        // Check if the developer camera is already active. If not, switch to it.
        // This prevents constant re-activation when dragging a slider.
        SPF_CameraType current_camera;
        if (g_ctx.cameraAPI->Cam_GetCurrentCamera(&current_camera) && current_camera != SPF_CAMERA_DEVELOPER_FREE)
        {
            g_ctx.cameraAPI->Cam_SwitchTo(SPF_CAMERA_DEVELOPER_FREE);
        }
        // --- 4. Find the Game's Local Grid Origin ---
        // Get the camera's current position in both world and local coordinates.
        float cam_current_world_x, cam_current_world_y, cam_current_world_z;
        g_ctx.cameraAPI->Cam_GetCameraWorldCoordinates(&cam_current_world_x, &cam_current_world_y, &cam_current_world_z);

        float cam_current_local_x, cam_current_local_y, cam_current_local_z;
        g_ctx.cameraAPI->Cam_GetFreePosition(&cam_current_local_x, &cam_current_local_y, &cam_current_local_z);

        // The difference between the world and local positions gives us the origin of the game's moving local grid.
        const double game_current_origin_x = cam_current_world_x - cam_current_local_x;
        const double game_current_origin_z = cam_current_world_z - cam_current_local_z;

        // --- 5. Set the Camera's Position ---
        // Calculate the final local position for the camera by making our target world position relative to the loca grid origin.
        const SPF_FVector final_local_pos_to_set = {
            (float)(cam_target_world_pos.x - game_current_origin_x), // X-coordinate relative to the local grid.
            (float)cam_target_world_pos.y,                           // Y-coordinate (height) is absolute.
            (float)(cam_target_world_pos.z - game_current_origin_z)  // Z-coordinate relative to the local grid.
        };

        // Set the free camera's position using the calculated local coordinates.
        g_ctx.cameraAPI->Cam_SetFreePosition(final_local_pos_to_set.x, final_local_pos_to_set.y, final_local_pos_to_set.z);

        // --- 6. Set the Camera's Orientation ---
        // To make the camera look at the truck, we need to calculate the yaw and pitch angles.
        // This is based on the direction vector pointing from the camera back to the truck.
        float yaw = 0.0f;
        float pitch = 0.0f;
        if (distance_forward != 0.0f || height_above != 0.0f)
        {
            // The look-at vector is the inverse of the offset vector.
            const SPF_FVector look_at_vec = {(float)(-world_offset_from_truck.x), (float)(-world_offset_from_truck.y), (float)(-world_offset_from_truck.z)};

            // Calculate the horizontal distance for the pitch calculation.
            const float horizontal_dist = sqrt(look_at_vec.x * look_at_vec.x + look_at_vec.z * look_at_vec.z);

            // Calculate yaw (horizontal angle) and pitch (vertical angle) using atan2 for correct quadrant handling.
            yaw = atan2(-look_at_vec.x, -look_at_vec.z);
            pitch = atan2(look_at_vec.y, horizontal_dist);

            // Set the free camera's orientation. Roll is set to 0.
            g_ctx.cameraAPI->Cam_SetFreeOrientation(yaw, pitch, 0.0f);
        }

        // --- 7. Set the Camera's Field of View (FOV) ---
        // Apply the FOV from our settings.
        g_ctx.cameraAPI->Cam_SetFreeFov(g_ctx.setting_field_of_view);

        // --- 8. Final Debug Logging ---
        if (g_ctx.loggerHandle && g_ctx.formattingAPI)
        {
            char log_buffer[512];
            g_ctx.formattingAPI->Fmt_Format(log_buffer, sizeof(log_buffer),
                                        "PositionAndOrientRedLightCamera: Full execution complete."
                                        " Set Local Pos: (%.2f, %.2f, %.2f),"
                                        " Set Orientation: (Yaw: %.2f, Pitch: %.2f),"
                                        " Set FOV: %.1f",
                                        final_local_pos_to_set.x, final_local_pos_to_set.y, final_local_pos_to_set.z, yaw, pitch, g_ctx.setting_field_of_view);
            g_ctx.loadAPI->logger->Log(g_ctx.loggerHandle, SPF_LOG_INFO, log_buffer);
        }
    }

    // =================================================================================================
    // 6. Plugin Exports
    // =================================================================================================
    // These are the two mandatory, C-style functions that the plugin DLL must export.
    // The `extern "C"` block is essential to prevent C++ name mangling, ensuring the framework
    // can find them by name.
    extern "C"
    {

        /**
         * @brief Exports the manifest API to the framework.
         * @details This function is mandatory for the framework to properly identify and configure the plugin.
         */
        SPF_PLUGIN_EXPORT bool SPF_GetManifestAPI(SPF_Manifest_API *out_api)
        {
            if (out_api)
            {
                out_api->BuildManifest = BuildManifest;
                return true;
            }
            return false;
        }

        /**
         * @brief Exports the plugin's main lifecycle and callback functions to the framework.
         * @details This function is mandatory for the framework to interact with the plugin's lifecycle.
         */
        SPF_PLUGIN_EXPORT bool SPF_GetPlugin(SPF_Plugin_Exports *exports)
        {
            if (exports)
            {
                // Connect the internal C++ functions to the C-style export struct.
                exports->OnLoad = OnLoad;
                exports->OnActivated = OnActivated;
                exports->OnUnload = OnUnload;
                exports->OnUpdate = OnUpdate;

                // Optional callbacks are set to nullptr by default.
                // Uncomment and assign your implementation if you use them.
                // exports->OnGameWorldReady = OnGameWorldReady; // Assign your OnGameWorldReady function for game-world-dependent logic.
                exports->OnRegisterUI = OnRegisterUI; // Assign your OnRegisterUI function if you have UI windows.
                exports->OnSettingChanged = OnSettingChanged; // Assign your OnSettingChanged function if you implement it.
                return true;
            }
            return false;
        }

    } // extern "C"

} // namespace SPF_RedLightCamera
