#include "Hints.h"
#include "utils/config.h"
#include <coreinit/cache.h>
#include <coreinit/thread.h>
#include <notifications/notification_defines.h>
#include <notifications/notifications.h>
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
    if (!gConfigMenuHintShown) {
        sShutdownHintThread = false;
        sShowHintThread     = std::make_unique<std::thread>(ShowHints);
    } else {
        sShowHintThread.reset();
    }
}

void StopHintThread() {
    if (sShowHintThread != nullptr) {
        sShutdownHintThread = true;
        OSMemoryBarrier();
        sShowHintThread->join();
        sShowHintThread.reset();
    }
}