#include "common.h"
#include "utils/DownloadUtils.h"
#include "utils/LatestVersion.h"
#include "utils/config.h"
#include "utils/input.h"
#include <coreinit/cache.h>
#include <coreinit/thread.h>
#include <coreinit/time.h>
#include <memory>
#include <notifications/notifications.h>
#include <padscore/wpad.h>
#include <rpxloader/rpxloader.h>
#include <string>
#include <sys/stat.h>
#include <thread>
#include <vpad/input.h>
#include <wups/function_patching.h>

static std::string sAromaUpdaterPath               = AROMA_UPDATER_NEW_PATH_FULL;
static NotificationModuleHandle sAromaUpdateHandle = 0;
std::unique_ptr<std::thread> sCheckUpdateThread;
static bool sShutdownUpdateThread = false;
void UpdateCheckThreadEntry();

void ShowUpdateNotification();
constexpr uint32_t HIDE_UPDATE_WARNING_VPAD_COMBO  = VPAD_BUTTON_MINUS;
constexpr uint32_t LAUNCH_AROMA_UPDATER_VPAD_COMBO = VPAD_BUTTON_PLUS;
constexpr uint32_t sHoldForFramesTarget            = 60;

void StartUpdaterCheckThread() {
    if (!gUpdateChecked && DownloadUtils::Init()) {
        sShutdownUpdateThread = false;
        sCheckUpdateThread    = std::make_unique<std::thread>(UpdateCheckThreadEntry);
    } else {
        sCheckUpdateThread.reset();
    }
}

void StopUpdaterCheckThread() {
    if (sCheckUpdateThread != nullptr) {
        sShutdownUpdateThread = true;
        OSMemoryBarrier();
        sCheckUpdateThread->join();
        sCheckUpdateThread.reset();
    }
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

void UpdateCheckThreadEntry() {
    bool isOverlayReady = false;
    while (!sShutdownUpdateThread &&
           NotificationModule_IsOverlayReady(&isOverlayReady) == NOTIFICATION_MODULE_RESULT_SUCCESS && !isOverlayReady) {
        OSSleepTicks(OSMillisecondsToTicks(16));
    }
    if (sShutdownUpdateThread || !isOverlayReady) {
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
        AromaUpdater::LatestVersion data = nlohmann::json::parse(outBuffer);
        gUpdateChecked                   = true;

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

void ShowUpdateNotification() {
    struct stat st {};
    // Check if the Aroma Updater is on the sd card
    if (stat(AROMA_UPDATER_OLD_PATH_FULL, &st) >= 0 && S_ISREG(st.st_mode)) {
        sAromaUpdaterPath = AROMA_UPDATER_OLD_PATH;
    } else if (stat(AROMA_UPDATER_NEW_PATH_FULL, &st) >= 0 && S_ISREG(st.st_mode)) {
        sAromaUpdaterPath = AROMA_UPDATER_NEW_PATH;
    } else {
        NotificationModule_SetDefaultValue(NOTIFICATION_MODULE_NOTIFICATION_TYPE_INFO, NOTIFICATION_MODULE_DEFAULT_OPTION_DURATION_BEFORE_FADE_OUT, 15.0f);
        NotificationModule_AddInfoNotification("A new Aroma Update is available. Please launch the Aroma Updater!");
        return;
    }

    NotificationModuleStatus err;
    if ((err = NotificationModule_AddDynamicNotification("A new Aroma Update is available. "
                                                         "Hold \ue045 to launch the Aroma Updater, press \ue046 to hide this message",
                                                         &sAromaUpdateHandle)) != NOTIFICATION_MODULE_RESULT_SUCCESS) {
        DEBUG_FUNCTION_LINE_ERR("Failed to add update notification. %s", NotificationModule_GetStatusStr(err));
        sAromaUpdateHandle = 0;
    }
}

extern "C" uint32_t VPADGetButtonProcMode(uint32_t);

static bool updaterLaunched     = false;
static uint32_t sHoldForXFrames = 0;

bool CheckForButtonCombo(uint32_t trigger, uint32_t hold, uint32_t &holdForXFrames, uint32_t holdForFramesTarget) {
    if (trigger == HIDE_UPDATE_WARNING_VPAD_COMBO) {
        NotificationModule_FinishDynamicNotification(sAromaUpdateHandle, 0.0f);
        sAromaUpdateHandle = 0;
        return true;
    }
    if (hold == LAUNCH_AROMA_UPDATER_VPAD_COMBO) {
        if (++holdForXFrames > holdForFramesTarget) {
            NotificationModule_FinishDynamicNotification(sAromaUpdateHandle, 0.5f);
            sAromaUpdateHandle = 0;
            RPXLoaderStatus err;
            if ((err = RPXLoader_LaunchHomebrew(sAromaUpdaterPath.c_str())) == RPX_LOADER_RESULT_SUCCESS) {
                updaterLaunched = true;
            } else {
                DEBUG_FUNCTION_LINE_ERR("RPXLoader_LaunchHomebrew failed: %s", RPXLoader_GetStatusStr(err));
                NotificationModule_AddErrorNotification("Failed to launch Aroma Updater");
            }
            return true;
        }
    } else {
        holdForXFrames = 0;
    }
    return false;
}

DECL_FUNCTION(int32_t, VPADRead, VPADChan chan,
              VPADStatus *buffers,
              uint32_t count,
              VPADReadError *outError) {
    if (!sAromaUpdateHandle) {
        return real_VPADRead(chan, buffers, count, outError);
    }
    VPADReadError real_error;
    auto result = real_VPADRead(chan, buffers, count, &real_error);

    if (!updaterLaunched && result > 0 && real_error == VPAD_READ_SUCCESS) {
        uint32_t end = 1;
        // Fix games like TP HD
        if (VPADGetButtonProcMode(chan) == 1) {
            end = result;
        }
        for (uint32_t i = 0; i < end; i++) {
            if (CheckForButtonCombo((buffers[i].trigger & 0x000FFFFF), (buffers[i].hold & 0x000FFFFF), sHoldForXFrames, sHoldForFramesTarget)) {
                break;
            }
        }
    }
    if (outError) {
        *outError = real_error;
    }
    return result;
}

static uint32_t sWPADLastButtonHold[4] = {0, 0, 0, 0};
static uint32_t sHoldForXFramesWPAD[4] = {0, 0, 0, 0};

DECL_FUNCTION(void, WPADRead, WPADChan chan, WPADStatusProController *data) {
    real_WPADRead(chan, data);
    if (!sAromaUpdateHandle) {
        return;
    }

    if (data && !updaterLaunched && chan >= 0 && chan < 4) {
        if (data[0].err == 0) {
            if (data[0].extensionType != 0xFF) {
                uint32_t curButtonHold = 0;
                if (data[0].extensionType == WPAD_EXT_CORE || data[0].extensionType == WPAD_EXT_NUNCHUK) {
                    // button data is in the first 2 bytes for wiimotes
                    curButtonHold = remapWiiMoteButtons(((uint16_t *) data)[0]);
                } else if (data[0].extensionType == WPAD_EXT_CLASSIC) {
                    curButtonHold = remapClassicButtons(((uint32_t *) data)[10] & 0xFFFF);
                } else if (data[0].extensionType == WPAD_EXT_PRO_CONTROLLER) {
                    curButtonHold = remapProButtons(data[0].buttons);
                }

                uint32_t curButtonTrigger = (curButtonHold & (~(sWPADLastButtonHold[chan])));

                if (CheckForButtonCombo(curButtonTrigger, curButtonHold, sHoldForXFramesWPAD[chan], sHoldForFramesTarget * 2)) {
                    return;
                }

                sWPADLastButtonHold[chan] = curButtonHold;
            }
        }
    }
}

// We only want to patch Wii U Menu
WUPS_MUST_REPLACE_FOR_PROCESS(VPADRead, WUPS_LOADER_LIBRARY_VPAD, VPADRead, WUPS_FP_TARGET_PROCESS_WII_U_MENU);
WUPS_MUST_REPLACE_FOR_PROCESS(WPADRead, WUPS_LOADER_LIBRARY_PADSCORE, WPADRead, WUPS_FP_TARGET_PROCESS_WII_U_MENU);
