#include "config.h"

bool gActivateUStealth          = false;
bool gSkip4SecondOffStatusCheck = true;
bool gConfigMenuHintShown       = false;
bool gUpdateChecked             = false;
bool gForceNDMSuspendSuccess    = true;
bool gAllowErrorNotifications   = true;
std::string gLastHash           = {};

void boolItemChangedConfig(ConfigItemBoolean *item, bool newValue) {
    wups_storage_item_t *cat_config;
    if (WUPS_GetSubItem(nullptr, CAT_CONFIG, &cat_config) == WUPS_STORAGE_ERROR_SUCCESS) {
        PROCESS_BOOL_ITEM_CHANGED(cat_config, USTEALTH_CONFIG_ID, gActivateUStealth);
        PROCESS_BOOL_ITEM_CHANGED(cat_config, POWEROFFWARNING_CONFIG_ID, gSkip4SecondOffStatusCheck);
        PROCESS_BOOL_ITEM_CHANGED(cat_config, FORCE_NDM_SUSPEND_SUCCESS_CONFIG_ID, gForceNDMSuspendSuccess);
        PROCESS_BOOL_ITEM_CHANGED(cat_config, ALLOW_ERROR_NOTIFICATIONS, gAllowErrorNotifications);
    } else {
        DEBUG_FUNCTION_LINE_ERR("Failed to get sub item: %s", CAT_CONFIG);
    }
}

WUPS_GET_CONFIG() {
    // We open the storage, so we can persist the configuration the user did.
    if (WUPS_OpenStorage() != WUPS_STORAGE_ERROR_SUCCESS) {
        DEBUG_FUNCTION_LINE("Failed to open storage");
        return 0;
    }

    WUPSConfigHandle config;
    WUPSConfig_CreateHandled(&config, "Aroma Base Plugin");

    WUPSConfigCategoryHandle cat;
    WUPSConfig_AddCategoryByNameHandled(config, "Wii U Menu patches", &cat);
    WUPSConfigCategoryHandle catOther;
    WUPSConfig_AddCategoryByNameHandled(config, "Other patches", &catOther);

    WUPSConfigItemBoolean_AddToCategoryHandled(config, cat, USTEALTH_CONFIG_ID, "Avoid \"Format\" dialog on Wii U Menu", gActivateUStealth, &boolItemChangedConfig);
    WUPSConfigItemBoolean_AddToCategoryHandled(config, cat, POWEROFFWARNING_CONFIG_ID, "Skip \"Shutdown warning\" on boot", gSkip4SecondOffStatusCheck, &boolItemChangedConfig);

    WUPSConfigItemBoolean_AddToCategoryHandled(config, catOther, ALLOW_ERROR_NOTIFICATIONS, "Allow error notifications", gAllowErrorNotifications, &boolItemChangedConfig);
    WUPSConfigItemBoolean_AddToCategoryHandled(config, catOther, FORCE_NDM_SUSPEND_SUCCESS_CONFIG_ID, "Fix connecting to a 3DS in Mii Maker", gForceNDMSuspendSuccess, &boolItemChangedConfig);

    return config;
}

WUPS_CONFIG_CLOSED() {
    // Save all changes
    if (WUPS_CloseStorage() != WUPS_STORAGE_ERROR_SUCCESS) {
        DEBUG_FUNCTION_LINE_ERR("Failed to close storage");
    }
}