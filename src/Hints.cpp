#include "Hints.h"
#include "utils/config.h"
#include <coreinit/cache.h>
#include <coreinit/thread.h>
#include <errno.h>
#include <notifications/notification_defines.h>
#include <notifications/notifications.h>
#include <sdutils/sdutils.h>
#include <thread>

std::unique_ptr<std::thread> sShowHintThread;
static bool sShutdownHintThread = false;

void ShowHints() {
    bool isOverlayReady = false;
    while (!sShutdownHintThread &&
           NotificationModule_IsOverlayReady(&isOverlayReady) == NOTIFICATION_MODULE_RESULT_SUCCESS && !isOverlayReady) {
        OSSleepTicks(OSMillisecondsToTicks(16));
    }
    if (sShutdownHintThread || !isOverlayReady) {
        return;
    }

    bool isMounted = false;
    if (SDUtils_IsSdCardMounted(&isMounted) != SDUTILS_RESULT_SUCCESS) {
        DEBUG_FUNCTION_LINE_ERR("SDUtils_IsSdCardMounted failed");
    }

    if (isMounted) {
        const char *tmp_file = "fs:/vol/external01/wiiu/write_lock";
        int fd               = -1;
        if ((fd = open(tmp_file, O_CREAT | O_TRUNC | O_RDWR)) < 0) {
            DEBUG_FUNCTION_LINE_ERR("SD Card mounted but not writable. %d , %s", errno, strerror(errno));
            NotificationModuleStatus err;
            NMColor red = {237, 28, 36, 255};
            NotificationModuleHandle outHandle;
            if ((err = NotificationModule_SetDefaultValue(NOTIFICATION_MODULE_NOTIFICATION_TYPE_DYNAMIC, NOTIFICATION_MODULE_DEFAULT_OPTION_BACKGROUND_COLOR, red)) != NOTIFICATION_MODULE_RESULT_SUCCESS ||
                (err = NotificationModule_AddDynamicNotification("Failed to write to the sd card. Please restart the console and make sure the sd card is not write locked.", &outHandle)) != NOTIFICATION_MODULE_RESULT_SUCCESS) {
                DEBUG_FUNCTION_LINE_ERR("Failed to display notification: %s", NotificationModule_GetStatusStr(err));
            }
        } else {
            DEBUG_FUNCTION_LINE_VERBOSE("SD Card is mounted and writeable");
            close(fd);
            remove(tmp_file);
        }
    } else {
        DEBUG_FUNCTION_LINE_VERBOSE("SD Card is not mounted");
    }

    if (!gConfigMenuHintShown) {
        NotificationModuleStatus err;
        if ((err = NotificationModule_SetDefaultValue(NOTIFICATION_MODULE_NOTIFICATION_TYPE_INFO, NOTIFICATION_MODULE_DEFAULT_OPTION_DURATION_BEFORE_FADE_OUT, 15.0f)) == NOTIFICATION_MODULE_RESULT_SUCCESS &&
            (err = NotificationModule_AddInfoNotification("Tip: You can open a configuration menu by pressing \ue052 + \ue07A + \ue046")) == NOTIFICATION_MODULE_RESULT_SUCCESS) {
            if (WUPS_OpenStorage() == WUPS_STORAGE_ERROR_SUCCESS) {
                gConfigMenuHintShown           = true;
                wups_storage_item_t *cat_other = nullptr;
                if (WUPS_GetSubItem(nullptr, CAT_OTHER, &cat_other) == WUPS_STORAGE_ERROR_SUCCESS) {
                    WUPS_StoreInt(cat_other, CONFIG_MENU_HINT_SHOWN_ID, gConfigMenuHintShown);
                }
                WUPS_CloseStorage();
            }
        } else {
            DEBUG_FUNCTION_LINE_ERR("Failed to show Notification: %d %s", err, NotificationModule_GetStatusStr(err));
        }
    }
}

void StartHintThread() {
    sShowHintThread.reset();
    sShutdownHintThread = false;
    sShowHintThread     = std::make_unique<std::thread>(ShowHints);
}

void StopHintThread() {
    if (sShowHintThread != nullptr) {
        sShutdownHintThread = true;
        OSMemoryBarrier();
        sShowHintThread->join();
        sShowHintThread.reset();
    }
}
