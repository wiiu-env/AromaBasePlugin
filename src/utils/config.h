#pragma once
#include "logger.h"
#include <string>
#include <wups/config/WUPSConfigItemBoolean.h>
#include <wups/storage.h>


#define CAT_CONFIG                "config"
#define CAT_OTHER                 "other"

#define USTEALTH_CONFIG_ID        "ustealth"
#define POWEROFFWARNING_CONFIG_ID "SkipPowerOffWarning"


#define CONFIG_MENU_HINT_SHOWN_ID "configMenuHintShown"
#define LAST_UPDATE_HASH_ID       "lastUpdateHash"

#define LOAD_BOOL_FROM_STORAGE(__cat, config_name, __variable__)                                                  \
    if ((storageRes = WUPS_GetBool(__cat, config_name, &__variable__)) == WUPS_STORAGE_ERROR_NOT_FOUND) {         \
        if (WUPS_StoreBool(__cat, config_name, __variable__) != WUPS_STORAGE_ERROR_SUCCESS) {                     \
            DEBUG_FUNCTION_LINE_WARN("Failed to store bool");                                                     \
        }                                                                                                         \
    } else if (storageRes != WUPS_STORAGE_ERROR_SUCCESS) {                                                        \
        DEBUG_FUNCTION_LINE_WARN("Failed to get bool %s (%d)", WUPS_GetStorageStatusStr(storageRes), storageRes); \
    }                                                                                                             \
    while (0)

#define LOAD_STRING_FROM_STORAGE(__cat, config_name, __string, __string_length)                                         \
    if ((storageRes = WUPS_GetString(__cat, config_name, __string, __string_length)) == WUPS_STORAGE_ERROR_NOT_FOUND) { \
        if (WUPS_StoreString(__cat, config_name, __string) != WUPS_STORAGE_ERROR_SUCCESS) {                             \
            DEBUG_FUNCTION_LINE_WARN("Failed to store string");                                                         \
        }                                                                                                               \
    } else if (storageRes != WUPS_STORAGE_ERROR_SUCCESS) {                                                              \
        DEBUG_FUNCTION_LINE_WARN("Failed to get bool %s (%d)", WUPS_GetStorageStatusStr(storageRes), storageRes);       \
    }                                                                                                                   \
    while (0)

#define PROCESS_BOOL_ITEM_CHANGED(cat, __config__name, __variable__)              \
    if (std::string_view(item->configId) == __config__name) {                     \
        DEBUG_FUNCTION_LINE_ERR("New value in %s: %d", __config__name, newValue); \
        __variable__ = newValue;                                                  \
        WUPS_StoreInt(cat, __config__name, __variable__);                         \
        return;                                                                   \
    }                                                                             \
    while (0)

extern bool gActivateUStealth;
extern bool gSkip4SecondOffStatusCheck;
extern bool gConfigMenuHintShown;
extern std::string gLastHash;
extern bool gUpdateChecked;