#include "UpdaterCheck.h"
#include "common.h"
#include "utils/DownloadUtils.h"
#include "utils/LatestVersion.h"
#include "utils/config.h"
#include "utils/logger.h"

#include <notifications/notifications.h>
#include <rpxloader/rpxloader.h>

#include <wups/button_combo/api.h>
#include <wups/button_combo/defines.h>
#include <wups/storage.h>

#include <coreinit/cache.h>
#include <coreinit/thread.h>
#include <coreinit/time.h>

#include <optional>
#include <string>
#include <thread>

#include <sys/stat.h>

using namespace std::literals;

static std::string sAromaUpdaterPath               = AROMA_UPDATER_NEW_PATH_FULL;
static NotificationModuleHandle sAromaUpdateHandle = 0;
std::optional<std::jthread> sCheckUpdateThread;
void UpdateCheckThreadEntry(std::stop_token token);

void ShowUpdateNotification();
constexpr WUPSButtonCombo_Buttons HIDE_UPDATE_WARNING_VPAD_COMBO  = WUPS_BUTTON_COMBO_BUTTON_MINUS;
constexpr WUPSButtonCombo_Buttons LAUNCH_AROMA_UPDATER_VPAD_COMBO = WUPS_BUTTON_COMBO_BUTTON_PLUS;
constexpr uint32_t HOLD_COMBO_DURATION_IN_MS                      = 1000;

static std::forward_list<WUPSButtonComboAPI::ButtonCombo> sButtonComboHandles;

void StartUpdaterCheckThread() {
    if (!gUpdateChecked && DownloadUtils::Init()) {
        sCheckUpdateThread.emplace(UpdateCheckThreadEntry);
    } else {
        sCheckUpdateThread.reset();
    }
}

void RemoveButtonComboHandles(NotificationModuleHandle, void *) {
    sButtonComboHandles.clear();
}

void StopUpdaterCheckThread() {
    if (sCheckUpdateThread) {
        sCheckUpdateThread.reset();
    }
    RemoveButtonComboHandles(0, nullptr);
}


bool saveLatestUpdateHash(const std::string &hash) {
    WUPSStorageError err;
    auto subItem = WUPSStorageAPI::GetSubItem(CAT_OTHER, err);
    if (!subItem) {
        DEBUG_FUNCTION_LINE_ERR("Failed to get sub category");
        return false;
    }
    if (subItem->Store(LAST_UPDATE_HASH_ID, hash) != WUPS_STORAGE_ERROR_SUCCESS) {
        DEBUG_FUNCTION_LINE_ERR("Failed to store latest update hash");
        return false;
    }

    if (WUPSStorageAPI::SaveStorage() != WUPS_STORAGE_ERROR_SUCCESS) {
        DEBUG_FUNCTION_LINE_ERR("Failed to save storage");
        return false;
    }

    return true;
}

void UpdateCheckThreadEntry(std::stop_token token) {
    bool isOverlayReady = false;
    while (!token.stop_requested() &&
           NotificationModule_IsOverlayReady(&isOverlayReady) == NOTIFICATION_MODULE_RESULT_SUCCESS && !isOverlayReady) {
        std::this_thread::sleep_for(16ms);
    }
    if (token.stop_requested() || !isOverlayReady) {
        return;
    }

    std::string outBuffer;
    int responseCodeOut;
    int errorOut;
    std::string errorTextOut;
    float progress;

    if (DownloadUtils::DownloadFileToBuffer(AROMA_UPDATER_LAST_UPDATE_URL, outBuffer, responseCodeOut, errorOut, errorTextOut, &progress) != 0) {
        DEBUG_FUNCTION_LINE_INFO("Download failed: %d %s", errorOut, errorTextOut.c_str());
        return;
    }
    try {
        const AromaUpdater::LatestVersion data = nlohmann::json::parse(outBuffer);
        gUpdateChecked                         = true;

        if (gLastHash.empty()) { // don't show update warning on first boot
            gLastHash = data.getHash();
        } else if (gLastHash != data.getHash()) {
            ShowUpdateNotification();
        } else if (gLastHash == data.getHash()) {
            DEBUG_FUNCTION_LINE_VERBOSE("We don't need to update the hash");
            return;
        }

        // Update hash
        gLastHash = data.getHash();

        saveLatestUpdateHash(gLastHash);
    } catch (std::exception &e) {
        DEBUG_FUNCTION_LINE_WARN("Failed to parse AromaUpdater::LatestVersion");
    }
}

static bool sUpdaterLaunched = false;

void HandleCancelNotification(WUPSButtonCombo_ControllerTypes, WUPSButtonCombo_ComboHandle, void *) {
    if (!sAromaUpdateHandle || sUpdaterLaunched) {
        return;
    }
    NotificationModule_FinishDynamicNotification(sAromaUpdateHandle, 0.0f);
    sAromaUpdateHandle = 0;
}

void HandleOpenAromaUpdater(WUPSButtonCombo_ControllerTypes, WUPSButtonCombo_ComboHandle, void *) {
    if (!sAromaUpdateHandle || sUpdaterLaunched) {
        return;
    }
    NotificationModule_FinishDynamicNotification(sAromaUpdateHandle, 0.5f);
    sAromaUpdateHandle = 0;
    if (const RPXLoaderStatus err = RPXLoader_LaunchHomebrew(sAromaUpdaterPath.data()); err == RPX_LOADER_RESULT_SUCCESS) {
        sUpdaterLaunched = true;
    } else {
        DEBUG_FUNCTION_LINE_ERR("RPXLoader_LaunchHomebrew failed: %s", RPXLoader_GetStatusStr(err));
        NotificationModule_AddErrorNotification("Failed to launch Aroma Updater");
    }
}

void ShowUpdateNotification() {
    bool buttonCombosValid = true;
    WUPSButtonCombo_Error comboErr;
    WUPSButtonCombo_ComboStatus status;

    if (auto comboOpt = WUPSButtonComboAPI::CreateComboHoldObserver("AromaBasePlugin: Launch Aroma Updater Combo",
                                                                    LAUNCH_AROMA_UPDATER_VPAD_COMBO,
                                                                    HOLD_COMBO_DURATION_IN_MS,
                                                                    HandleOpenAromaUpdater,
                                                                    nullptr,
                                                                    status,
                                                                    comboErr);
        comboOpt && comboErr == WUPS_BUTTON_COMBO_ERROR_SUCCESS && status == WUPS_BUTTON_COMBO_COMBO_STATUS_VALID) {
        sButtonComboHandles.push_front(std::move(*comboOpt));
    } else {
        DEBUG_FUNCTION_LINE_WARN("Failed to create launch aroma updater combo: %s", WUPSButtonComboAPI_GetStatusStr(comboErr));
        buttonCombosValid = false;
    }

    if (auto comboOpt = WUPSButtonComboAPI::CreateComboPressDownObserver("AromaBasePlugin: Cancel Aroma Updater Notification",
                                                                         HIDE_UPDATE_WARNING_VPAD_COMBO,
                                                                         HandleCancelNotification,
                                                                         nullptr,
                                                                         status,
                                                                         comboErr);
        comboOpt && comboErr == WUPS_BUTTON_COMBO_ERROR_SUCCESS && status == WUPS_BUTTON_COMBO_COMBO_STATUS_VALID) {
        sButtonComboHandles.push_front(std::move(*comboOpt));
    } else {
        DEBUG_FUNCTION_LINE_WARN("Failed to create cancel aroma updater notification combo: %s", WUPSButtonComboAPI_GetStatusStr(comboErr));
        buttonCombosValid = false;
    }

    struct stat st {};
    // Check if the Aroma Updater is on the sd card
    if (buttonCombosValid && stat(AROMA_UPDATER_OLD_PATH_FULL, &st) >= 0 && S_ISREG(st.st_mode)) {
        sAromaUpdaterPath = AROMA_UPDATER_OLD_PATH;
    } else if (buttonCombosValid && stat(AROMA_UPDATER_NEW_PATH_FULL, &st) >= 0 && S_ISREG(st.st_mode)) {
        sAromaUpdaterPath = AROMA_UPDATER_NEW_PATH;
    } else {
        RemoveButtonComboHandles(0, nullptr);
        NotificationModule_SetDefaultValue(NOTIFICATION_MODULE_NOTIFICATION_TYPE_INFO, NOTIFICATION_MODULE_DEFAULT_OPTION_DURATION_BEFORE_FADE_OUT, 15.0f);
        NotificationModule_AddInfoNotification("A new Aroma Update is available. Please launch the Aroma Updater!");
        return;
    }

    if (const NotificationModuleStatus err =
                NotificationModule_AddDynamicNotificationWithCallback("A new Aroma Update is available. "
                                                                      "Hold \ue045 to launch the Aroma Updater, press \ue046 to hide this message",
                                                                      &sAromaUpdateHandle,
                                                                      RemoveButtonComboHandles,
                                                                      nullptr);
        err != NOTIFICATION_MODULE_RESULT_SUCCESS) {
        DEBUG_FUNCTION_LINE_ERR("Failed to add update notification. %s", NotificationModule_GetStatusStr(err));
        sAromaUpdateHandle = 0;
    }
}
