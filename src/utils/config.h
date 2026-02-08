#pragma once

#include <string>

#include <cstdint>

#define CAT_CONFIG                             "config"
#define CAT_OTHER                              "other"
#define CAT_DEBUG                              "debug"

#define USTEALTH_CONFIG_ID                     "ustealth"
#define POWEROFFWARNING_CONFIG_ID              "SkipPowerOffWarning"
#define FORCE_NDM_SUSPEND_SUCCESS_CONFIG_ID    "forceNDMSuspendSuccess"
#define ALLOW_ERROR_NOTIFICATIONS              "allowErrorNotifications"
#define CONFIG_MENU_HINT_SHOWN_ID              "configMenuHintShown"
#define LAST_UPDATE_HASH_ID                    "lastUpdateHash"
#define TCP_LOGGING_ENABLED_ID                 "tcpLoggingEnabled"
#define TCP_LOGGING_IP_FILTER_ACTIVE_ID        "tcpLoggingIPFilterActive"
#define TCP_LOGGING_IP_ID                      "tcpLoggingIp"

#define ACTIVATE_USTEALTH_DEFAULT              false
#define SKIP_4_SECOND_OFF_STATUS_CHECK_DEFAULT true
#define CONFIG_MENU_HINT_SHOWN_DEFAULT         false
#define UPDATE_CHECKED_DEFAULT                 false
#define FORCE_NDM_SUSPEND_SUCCESS_DEFAULT      true
#define ALLOW_ERROR_NOTIFICATIONS_DEFAULT      true
#define LAST_UPDATE_HASH_DEFAULT               std::string()
#define TCP_LOGGING_ENABLED_DEFAULT            false
#define TCP_LOGGING_IP_FILTER_ACTIVE_DEFAULT   false
#define TCP_LOGGING_IP_DEFAULT                 (uint32_t) 0x00000000

extern bool gActivateUStealth;
extern bool gSkip4SecondOffStatusCheck;
extern bool gConfigMenuHintShown;
extern std::string gLastHash;
extern bool gUpdateChecked;
extern bool gForceNDMSuspendSuccess;
extern bool gAllowErrorNotifications;
extern bool gTCPLoggingEnabled;
extern bool gTCPLoggingIPFilterActive;
extern uint32_t gTCPLoggingIP;
extern int32_t gLibMochaAPIVersion;

void InitConfigMenu();