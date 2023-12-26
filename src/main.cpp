#include "main.h"
#include "Hints.h"
#include "UpdaterCheck.h"
#include "utils/DownloadUtils.h"
#include "utils/LatestVersion.h"
#include "utils/config.h"
#include <coreinit/title.h>
#include <malloc.h>
#include <nn/spm.h>
#include <notifications/notifications.h>
#include <rpxloader/rpxloader.h>
#include <sdutils/sdutils.h>
#include <utils/logger.h>
#include <wups.h>

WUPS_PLUGIN_NAME("Aroma Base Plugin");
WUPS_PLUGIN_DESCRIPTION("Implements small patches and checks for Aroma updates.");
WUPS_PLUGIN_VERSION(PLUGIN_VERSION_FULL);
WUPS_PLUGIN_AUTHOR("Maschell");
WUPS_PLUGIN_LICENSE("GPL");

WUPS_USE_WUT_DEVOPTAB();
WUPS_USE_STORAGE("aroma_base_plugin"); // Unique id for the storage api

bool InitConfigValuesFromStorage() {
    WUPSStorageError storageError;
    auto subItemConfig = WUPSStorageAPI::GetOrCreateSubItem(CAT_CONFIG, storageError);
    if (!subItemConfig) {
        DEBUG_FUNCTION_LINE_ERR("Failed to get or create sub category \"%s\"", CAT_CONFIG);
        return false;
    }

    if (subItemConfig->GetOrStoreDefault(USTEALTH_CONFIG_ID, gActivateUStealth, gActivateUStealth) != WUPS_STORAGE_ERROR_SUCCESS) {
        DEBUG_FUNCTION_LINE_ERR("Failed to get or create item \"%s\"", USTEALTH_CONFIG_ID);
        return false;
    }
    if (subItemConfig->GetOrStoreDefault(POWEROFFWARNING_CONFIG_ID, gSkip4SecondOffStatusCheck, gSkip4SecondOffStatusCheck) != WUPS_STORAGE_ERROR_SUCCESS) {
        DEBUG_FUNCTION_LINE_ERR("Failed to get or create item \"%s\"", POWEROFFWARNING_CONFIG_ID);
        return false;
    }
    if (subItemConfig->GetOrStoreDefault(FORCE_NDM_SUSPEND_SUCCESS_CONFIG_ID, gForceNDMSuspendSuccess, gForceNDMSuspendSuccess) != WUPS_STORAGE_ERROR_SUCCESS) {
        DEBUG_FUNCTION_LINE_ERR("Failed to get or create item \"%s\"", FORCE_NDM_SUSPEND_SUCCESS_CONFIG_ID);
        return false;
    }
    if (subItemConfig->GetOrStoreDefault(ALLOW_ERROR_NOTIFICATIONS, gAllowErrorNotifications, gAllowErrorNotifications) != WUPS_STORAGE_ERROR_SUCCESS) {
        DEBUG_FUNCTION_LINE_ERR("Failed to get or create item \"%s\"", ALLOW_ERROR_NOTIFICATIONS);
        return false;
    }

    auto subItemOther = WUPSStorageAPI::GetOrCreateSubItem(CAT_OTHER, storageError);
    if (!subItemOther) {
        DEBUG_FUNCTION_LINE_ERR("Failed to get or create sub category \"%s\"", CAT_OTHER);
        return false;
    }

    if (subItemOther->GetOrStoreDefault(CONFIG_MENU_HINT_SHOWN_ID, gConfigMenuHintShown, gConfigMenuHintShown) != WUPS_STORAGE_ERROR_SUCCESS) {
        DEBUG_FUNCTION_LINE_ERR("Failed to get or create item \"%s\"", CONFIG_MENU_HINT_SHOWN_ID);
        return false;
    }
    if (subItemOther->GetOrStoreDefault(CONFIG_MENU_HINT_SHOWN_ID, gLastHash, gLastHash) != WUPS_STORAGE_ERROR_SUCCESS) {
        DEBUG_FUNCTION_LINE_ERR("Failed to get or create item \"%s\"", CONFIG_MENU_HINT_SHOWN_ID);
        return false;
    }

    if (WUPSStorageAPI::SaveStorage() != WUPS_STORAGE_ERROR_SUCCESS) {
        DEBUG_FUNCTION_LINE_ERR("Failed to save storage");
    }

    return true;
}

INITIALIZE_PLUGIN() {
    initLogging();
    if (NotificationModule_InitLibrary() != NOTIFICATION_MODULE_RESULT_SUCCESS) {
        DEBUG_FUNCTION_LINE_ERR("NotificationModule_InitLibrary failed");
    }
    if (RPXLoader_InitLibrary() != RPX_LOADER_RESULT_SUCCESS) {
        DEBUG_FUNCTION_LINE_ERR("RPXLoader_InitLibrary failed");
    }
    if (SDUtils_InitLibrary() != SDUTILS_RESULT_SUCCESS) {
        DEBUG_FUNCTION_LINE_ERR("SDUtils_InitLibrary failed");
    }

    InitConfigValuesFromStorage();

    InitConfigMenu();
}

ON_APPLICATION_START() {
    initLogging();
    uint64_t titleID = OSGetTitleID();
    if (titleID == 0x0005001010040000L || // Wii U Menu
        titleID == 0x0005001010040100L ||
        titleID == 0x0005001010040200L) {

        StartHintThread();
        StartUpdaterCheckThread();
    }
}

ON_APPLICATION_ENDS() {
    StopHintThread();
    StopUpdaterCheckThread();
    DownloadUtils::Deinit();
    deinitLogging();
}

DEINITIALIZE_PLUGIN() {
    NotificationModule_DeInitLibrary();
    RPXLoader_DeInitLibrary();
    SDUtils_DeInitLibrary();
}

DECL_FUNCTION(uint32_t, SuspendDaemonsAndDisconnectIfWireless__Q2_2nn3ndmFv) {
    auto res = real_SuspendDaemonsAndDisconnectIfWireless__Q2_2nn3ndmFv();

    if (res != 0) {
        DEBUG_FUNCTION_LINE_ERR("SuspendDaemonsAndDisconnectIfWireless__Q2_2nn3ndmFv returned %08X", res);
        if (res == 0xA0B12C80 && gForceNDMSuspendSuccess) {
            DEBUG_FUNCTION_LINE_INFO("Patch SuspendDaemonsAndDisconnectIfWireless__Q2_2nn3ndmFv to return 0 instead of %08X", res);
            return 0;
        } else if (gAllowErrorNotifications) {
            NotificationModule_SetDefaultValue(NOTIFICATION_MODULE_NOTIFICATION_TYPE_ERROR, NOTIFICATION_MODULE_DEFAULT_OPTION_DURATION_BEFORE_FADE_OUT, 10.0f);
            NotificationModule_AddErrorNotification("\"nn::ndm::SuspendDaemonsAndDisconnectIfWireless\" failed. Connection to 3DS not possible");
        }
    }
    return res;
}

DECL_FUNCTION(int32_t, IsStorageMaybePcFormatted, bool *isPcFormatted, nn::spm::StorageIndex *index) {
    // Make sure the index is valid
    int32_t res = real_IsStorageMaybePcFormatted(isPcFormatted, index);
    if (gActivateUStealth && res == 0) {
        // always return false which makes the Wii U menu stop nagging about this drive
        *isPcFormatted = false;
    }

    return res;
}

DECL_FUNCTION(bool, MCP_Get4SecondOffStatus, int32_t handle) {
    if (gSkip4SecondOffStatusCheck) {
        return false;
    }

    return real_MCP_Get4SecondOffStatus(handle);
}

// Only replace for the Wii U Menu
WUPS_MUST_REPLACE_FOR_PROCESS(IsStorageMaybePcFormatted, WUPS_LOADER_LIBRARY_NN_SPM, IsStorageMaybePcFormatted__Q2_2nn3spmFPbQ3_2nn3spm12StorageIndex, WUPS_FP_TARGET_PROCESS_WII_U_MENU);
WUPS_MUST_REPLACE_FOR_PROCESS(MCP_Get4SecondOffStatus, WUPS_LOADER_LIBRARY_COREINIT, MCP_Get4SecondOffStatus, WUPS_FP_TARGET_PROCESS_WII_U_MENU);
WUPS_MUST_REPLACE(SuspendDaemonsAndDisconnectIfWireless__Q2_2nn3ndmFv, WUPS_LOADER_LIBRARY_NN_NDM, SuspendDaemonsAndDisconnectIfWireless__Q2_2nn3ndmFv);
