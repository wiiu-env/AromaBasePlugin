#include "config.h"

bool gActivateUStealth          = ACTIVATE_USTEALTH_DEFAULT;
bool gSkip4SecondOffStatusCheck = SKIP_4_SECOND_OFF_STATUS_CHECK_DEFAULT;
bool gConfigMenuHintShown       = CONFIG_MENU_HINT_SHOWN_DEFAULT;
bool gUpdateChecked             = UPDATE_CHECKED_DEFAULT;
bool gForceNDMSuspendSuccess    = FORCE_NDM_SUSPEND_SUCCESS_DEFAULT;
bool gAllowErrorNotifications   = ALLOW_ERROR_NOTIFICATIONS_DEFAULT;
std::string gLastHash           = LAST_UPDATE_HASH_DEFAULT;

void boolItemChangedConfig(ConfigItemBoolean *item, bool newValue) {
    WUPSStorageError storageError;
    auto subItemConfig = WUPSStorageAPI::GetSubItem(CAT_CONFIG, storageError);
    if (!subItemConfig) {
        DEBUG_FUNCTION_LINE_ERR("Failed to get sub item \"%s\": %s", CAT_CONFIG, WUPSStorageAPI::GetStatusStr(storageError).data());
        return;
    }
    if (std::string_view(USTEALTH_CONFIG_ID) == item->identifier) {
        gActivateUStealth = newValue;
        storageError      = subItemConfig->Store(USTEALTH_CONFIG_ID, newValue);
    } else if (std::string_view(POWEROFFWARNING_CONFIG_ID) == item->identifier) {
        gSkip4SecondOffStatusCheck = newValue;
        storageError               = subItemConfig->Store(POWEROFFWARNING_CONFIG_ID, newValue);
    } else if (std::string_view(ALLOW_ERROR_NOTIFICATIONS) == item->identifier) {
        gAllowErrorNotifications = newValue;
        storageError             = subItemConfig->Store(ALLOW_ERROR_NOTIFICATIONS, newValue);
    } else if (std::string_view(FORCE_NDM_SUSPEND_SUCCESS_CONFIG_ID) == item->identifier) {
        gForceNDMSuspendSuccess = newValue;
        storageError            = subItemConfig->Store(FORCE_NDM_SUSPEND_SUCCESS_CONFIG_ID, newValue);
    } else {
        return;
    }
    if (storageError != WUPS_STORAGE_ERROR_SUCCESS) {
        DEBUG_FUNCTION_LINE_ERR("Failed to store %s. New value was %d", item->identifier, newValue);
    }
}

WUPSConfigAPICallbackStatus ConfigMenuOpenedCallback(WUPSConfigCategoryHandle rootHandle) {
    try {
        WUPSConfigCategory root = WUPSConfigCategory(rootHandle);

        auto menuPatches = WUPSConfigCategory::Create("Wii U Menu patches");

        menuPatches.add(WUPSConfigItemBoolean::Create(USTEALTH_CONFIG_ID,
                                                      "Avoid \"Format\" dialog on Wii U Menu",
                                                      ACTIVATE_USTEALTH_DEFAULT, gActivateUStealth,
                                                      &boolItemChangedConfig));

        menuPatches.add(WUPSConfigItemBoolean::Create(POWEROFFWARNING_CONFIG_ID,
                                                      "Skip \"Shutdown warning\" on boot",
                                                      SKIP_4_SECOND_OFF_STATUS_CHECK_DEFAULT, gSkip4SecondOffStatusCheck,
                                                      &boolItemChangedConfig));

        root.add(std::move(menuPatches));

        auto otherPatches = WUPSConfigCategory::Create("Other patches");

        otherPatches.add(WUPSConfigItemBoolean::Create(ALLOW_ERROR_NOTIFICATIONS,
                                                       "Allow error notifications",
                                                       ALLOW_ERROR_NOTIFICATIONS_DEFAULT, gAllowErrorNotifications,
                                                       &boolItemChangedConfig));

        otherPatches.add(WUPSConfigItemBoolean::Create(FORCE_NDM_SUSPEND_SUCCESS_CONFIG_ID,
                                                       "Fix connecting to a 3DS in Mii Maker",
                                                       FORCE_NDM_SUSPEND_SUCCESS_DEFAULT, gForceNDMSuspendSuccess,
                                                       &boolItemChangedConfig));
        root.add(std::move(otherPatches));

    } catch (std::exception &e) {
        DEBUG_FUNCTION_LINE_ERR("Exception: %s\n", e.what());
        return WUPSCONFIG_API_CALLBACK_RESULT_ERROR;
    }
    return WUPSCONFIG_API_CALLBACK_RESULT_SUCCESS;
}

void ConfigMenuClosedCallback() {
    WUPSStorageError storageError;
    if ((storageError = WUPSStorageAPI::SaveStorage()) != WUPS_STORAGE_ERROR_SUCCESS) {
        DEBUG_FUNCTION_LINE_ERR("Failed to save storage: %d %s", storageError, WUPSStorageAPI_GetStatusStr(storageError));
    }
}

void InitConfigMenu() {
    WUPSConfigAPIOptionsV1 configOptions = {.name = "Aroma Base Plugin"};
    if (WUPSConfigAPI_Init(configOptions, ConfigMenuOpenedCallback, ConfigMenuClosedCallback) != WUPSCONFIG_API_RESULT_SUCCESS) {
        DEBUG_FUNCTION_LINE_ERR("Failed to init config api");
    }
}