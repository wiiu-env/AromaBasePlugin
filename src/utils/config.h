#pragma once
#include "logger.h"
#include <string>
#include <wups/config/WUPSConfigItemBoolean.h>
#include <wups/storage.h>

#define CAT_CONFIG                             "config"
#define CAT_OTHER                              "other"

#define USTEALTH_CONFIG_ID                     "ustealth"
#define POWEROFFWARNING_CONFIG_ID              "SkipPowerOffWarning"
#define FORCE_NDM_SUSPEND_SUCCESS_CONFIG_ID    "forceNDMSuspendSuccess"
#define ALLOW_ERROR_NOTIFICATIONS              "allowErrorNotifications"

#define CONFIG_MENU_HINT_SHOWN_ID              "configMenuHintShown"
#define LAST_UPDATE_HASH_ID                    "lastUpdateHash"

#define ACTIVATE_USTEALTH_DEFAULT              false
#define SKIP_4_SECOND_OFF_STATUS_CHECK_DEFAULT false
#define CONFIG_MENU_HINT_SHOWN_DEFAULT         false
#define UPDATE_CHECKED_DEFAULT                 false
#define FORCE_NDM_SUSPEND_SUCCESS_DEFAULT      false
#define ALLOW_ERROR_NOTIFICATIONS_DEFAULT      false
#define LAST_UPDATE_HASH_DEFAULT               std::string()

extern bool gActivateUStealth;
extern bool gSkip4SecondOffStatusCheck;
extern bool gConfigMenuHintShown;
extern std::string gLastHash;
extern bool gUpdateChecked;
extern bool gForceNDMSuspendSuccess;
extern bool gAllowErrorNotifications;