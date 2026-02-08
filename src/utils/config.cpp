#include "config.h"

#include "logger.h"

#include <mocha/mocha.h>
#include <wups/config/WUPSConfigCategory.h>
#include <wups/config/WUPSConfigItemBoolean.h>
#include <wups/config/WUPSConfigItemIPAddress.h>
#include <wups/config/WUPSConfigItemStub.h>
#include <wups/storage.h>

#include <string>

bool gActivateUStealth          = ACTIVATE_USTEALTH_DEFAULT;
bool gSkip4SecondOffStatusCheck = SKIP_4_SECOND_OFF_STATUS_CHECK_DEFAULT;
bool gConfigMenuHintShown       = CONFIG_MENU_HINT_SHOWN_DEFAULT;
bool gUpdateChecked             = UPDATE_CHECKED_DEFAULT;
bool gForceNDMSuspendSuccess    = FORCE_NDM_SUSPEND_SUCCESS_DEFAULT;
bool gAllowErrorNotifications   = ALLOW_ERROR_NOTIFICATIONS_DEFAULT;
std::string gLastHash           = LAST_UPDATE_HASH_DEFAULT;
bool gTCPLoggingEnabled         = TCP_LOGGING_ENABLED_DEFAULT;
bool gTCPLoggingIPFilterActive  = TCP_LOGGING_IP_FILTER_ACTIVE_DEFAULT;
uint32_t gTCPLoggingIP          = TCP_LOGGING_IP_DEFAULT;
int32_t gLibMochaAPIVersion     = -1;

static void handleTCPServerChange() {
    if (gTCPLoggingEnabled) {
        if (const auto res = Mocha_StartTCPSyslogLogging(gTCPLoggingIPFilterActive, gTCPLoggingIPFilterActive ? gTCPLoggingIP : 0); res != MOCHA_RESULT_SUCCESS) {
            DEBUG_FUNCTION_LINE_WARN("Failed to start syslog tcp server: %s (%d)", Mocha_GetStatusStr(res), res);
        }
    } else {
        if (const auto res = Mocha_StopTCPSyslogLogging(); res != MOCHA_RESULT_SUCCESS) {
            DEBUG_FUNCTION_LINE_WARN("Failed to stop syslog tcp server: %s (%d)", Mocha_GetStatusStr(res), res);
        }
    }
}

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
    } else if (std::string_view(TCP_LOGGING_ENABLED_ID) == item->identifier) {
        gTCPLoggingEnabled = newValue;
        storageError       = subItemConfig->Store(TCP_LOGGING_ENABLED_ID, newValue);
        handleTCPServerChange();
    } else if (std::string_view(TCP_LOGGING_IP_FILTER_ACTIVE_ID) == item->identifier) {
        gTCPLoggingIPFilterActive = newValue;
        storageError              = subItemConfig->Store(TCP_LOGGING_IP_FILTER_ACTIVE_ID, newValue);
        handleTCPServerChange();
    } else {
        return;
    }
    if (storageError != WUPS_STORAGE_ERROR_SUCCESS) {
        DEBUG_FUNCTION_LINE_ERR("Failed to store %s. New value was %d", item->identifier, newValue);
    }
}

void ipAddressItemChangedConfig(ConfigItemIPAddress *item, uint32_t newValue) {
    WUPSStorageError storageError;
    auto subItemConfig = WUPSStorageAPI::GetSubItem(CAT_CONFIG, storageError);
    if (!subItemConfig) {
        DEBUG_FUNCTION_LINE_ERR("Failed to get sub item \"%s\": %s", CAT_CONFIG, WUPSStorageAPI::GetStatusStr(storageError).data());
        return;
    }
    if (std::string_view(TCP_LOGGING_IP_ID) == item->identifier) {
        gTCPLoggingIP = newValue;
        storageError  = subItemConfig->Store(TCP_LOGGING_IP_ID, newValue);
        handleTCPServerChange();
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

        auto debug = WUPSConfigCategory::Create("Debug");

        if (gLibMochaAPIVersion >= 2) {
            debug.add(WUPSConfigItemBoolean::Create(TCP_LOGGING_ENABLED_ID,
                                                    "Enables logging and shell access via TCP",
                                                    TCP_LOGGING_ENABLED_DEFAULT, gTCPLoggingEnabled,
                                                    &boolItemChangedConfig));

            debug.add(WUPSConfigItemBoolean::Create(TCP_LOGGING_IP_FILTER_ACTIVE_ID,
                                                    "Only allow connections from a specific IP address",
                                                    TCP_LOGGING_IP_FILTER_ACTIVE_DEFAULT, gTCPLoggingIPFilterActive,
                                                    &boolItemChangedConfig));

            debug.add(WUPSConfigItemIPAddress::Create(TCP_LOGGING_IP_ID,
                                                      "Allow only connections from",
                                                      TCP_LOGGING_IP_DEFAULT,
                                                      gTCPLoggingIP,
                                                      &ipAddressItemChangedConfig));
        } else {
            debug.add(WUPSConfigItemStub::Create("Update Aroma/Mocha to access logging via TCP"));
        }
        root.add(std::move(debug));

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