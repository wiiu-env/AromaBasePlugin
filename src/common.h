#pragma once
#define SD_CARD_PATH                     "fs:/vol/external01/"
#define AROMA_UPDATER_OLD_PATH           "wiiu/apps/AromaUpdater.wuhb"
#define AROMA_UPDATER_NEW_DIRECTORY      "wiiu/apps/AromaUpdater"
#define AROMA_UPDATER_NEW_PATH           AROMA_UPDATER_NEW_DIRECTORY "/AromaUpdater.wuhb"
#define AROMA_UPDATER_OLD_PATH_FULL      SD_CARD_PATH AROMA_UPDATER_OLD_PATH
#define AROMA_UPDATER_NEW_PATH_FULL      SD_CARD_PATH AROMA_UPDATER_NEW_PATH
#define AROMA_UPDATER_NEW_DIRECTORY_FULL SD_CARD_PATH AROMA_UPDATER_NEW_DIRECTORY
#define AROMA_UPDATER_LAST_UPDATE_URL    "https://aroma.foryour.cafe/api/latest_version"


#define BACKUPS_DIRECTORY                "wiiu/backups"
#define BACKUPS_DIRECTORY_FULL           SD_CARD_PATH BACKUPS_DIRECTORY