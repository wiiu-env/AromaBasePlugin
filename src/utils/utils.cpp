#include "utils.h"
#include "FSUtils.h"
#include "common.h"
#include "logger.h"
#include <coreinit/mcp.h>
#include <mocha/mocha.h>
#include <string>
#include <sys/stat.h>

bool Utils::GetSerialId(std::string &serialID) {
    bool result = false;
    alignas(0x40) MCPSysProdSettings settings{};
    auto handle = MCP_Open();
    if (handle >= 0) {
        if (MCP_GetSysProdSettings(handle, &settings) == 0) {
            serialID = std::string(settings.code_id) + settings.serial_id;
            result   = true;
        } else {
            DEBUG_FUNCTION_LINE_ERR("Failed to get SerialId");
        }
        MCP_Close(handle);
    } else {
        DEBUG_FUNCTION_LINE_ERR("MCP_Open failed");
    }
    return result;
}

static inline bool existsAsFile(std::string_view path) {
    struct stat st {};
    if (stat(path.data(), &st) >= 0 && S_ISREG(st.st_mode)) {
        DEBUG_FUNCTION_LINE_VERBOSE("\"%s\" exists", path.data());
        return true;
    } else {
        DEBUG_FUNCTION_LINE_VERBOSE("\"%s\" doesn't exists", path.data());
    }
    return false;
}

/*
 * Migrates wiiu/apps/AromaUpdater.wuhb to wiiu/apps/AromaUpdater/AromaUpdater.wuhb
 */
void Utils::MigrateAromaUpdater() {
    bool newExists = existsAsFile(AROMA_UPDATER_NEW_PATH_FULL);
    bool oldExists = existsAsFile(AROMA_UPDATER_OLD_PATH_FULL);

    if (newExists) {
        if (oldExists) {
            if (remove(AROMA_UPDATER_OLD_PATH_FULL) < 0) {
                DEBUG_FUNCTION_LINE_WARN("Failed to remove old Aroma Updater: %d", errno);
            }
        } else {
            DEBUG_FUNCTION_LINE_VERBOSE("Only new AromaUpdater.wuhb exists");
        }
        return;
    } else if (oldExists) {
        if (!FSUtils::CreateSubfolder(AROMA_UPDATER_NEW_DIRECTORY_FULL)) {
            DEBUG_FUNCTION_LINE_WARN("Failed to create \"%s\"", AROMA_UPDATER_NEW_DIRECTORY_FULL);
        }

        if (rename(AROMA_UPDATER_OLD_PATH_FULL, AROMA_UPDATER_NEW_PATH_FULL) < 0) {
            DEBUG_FUNCTION_LINE_WARN("Failed to move Aroma Updater to new path");
        }
    } else {
        DEBUG_FUNCTION_LINE_VERBOSE("No AromaUpdater.wuhb exists");
    }
}

/*
 * Dumps the OTP and SEEPROM when if it's no exists.
 */
void Utils::DumpOTPAndSeeprom() {
    std::string serialId;
    if (!Utils::GetSerialId(serialId)) {
        DEBUG_FUNCTION_LINE_WARN("Failed to get SerialId of the console, skip OTP/SEEPROM dumping");
        return;
    }
    std::string backupPathConsole            = string_format(BACKUPS_DIRECTORY_FULL "/%s", serialId.c_str());
    std::string backupPathConsoleOtpPath     = backupPathConsole + "/opt.bin";
    std::string backupPathConsoleSeepromPath = backupPathConsole + "/seeprom.bin";

    if (!FSUtils::CreateSubfolder(backupPathConsole.c_str())) {
        DEBUG_FUNCTION_LINE_WARN("Failed to create \"%s\"", backupPathConsole.c_str());
    }

    bool seepromExists = FSUtils::CheckFile(backupPathConsoleSeepromPath.c_str());
    bool optExists     = FSUtils::CheckFile(backupPathConsoleOtpPath.c_str());

    if (!seepromExists) {
        uint8_t data[0x200] = {};
        if (Mocha_SEEPROMRead(data, 0, sizeof(data)) != sizeof(data)) {
            DEBUG_FUNCTION_LINE_WARN("Failed to read SEEPROM");
        } else {
            if (FSUtils::saveBufferToFile(backupPathConsoleSeepromPath.c_str(), (void *) data, sizeof(data)) != sizeof(data)) {
                DEBUG_FUNCTION_LINE_WARN("Failed to write SEEPROM backup (\"%s\")", backupPathConsoleSeepromPath.c_str());
            } else {
                DEBUG_FUNCTION_LINE_INFO("Created SEEPROM backup: \"%s\"", backupPathConsoleSeepromPath.c_str());
            }
        }
    } else {
        DEBUG_FUNCTION_LINE_VERBOSE("SEEPROM backup already exists");
    }
    if (!optExists) {
        WiiUConsoleOTP otp = {};
        if (Mocha_ReadOTP(&otp) != MOCHA_RESULT_SUCCESS) {
            DEBUG_FUNCTION_LINE_WARN("Failed to read otp");
        } else {
            if (FSUtils::saveBufferToFile(backupPathConsoleOtpPath.c_str(), (void *) &otp, sizeof(otp)) != sizeof(otp)) {
                DEBUG_FUNCTION_LINE_WARN("Failed to write otp backup (\"%s\")", backupPathConsoleOtpPath.c_str());
            } else {
                DEBUG_FUNCTION_LINE_INFO("Created OTP backup: \"%s\"", backupPathConsoleOtpPath.c_str());
            }
        }
    } else {
        DEBUG_FUNCTION_LINE_VERBOSE("OTP backup already exists");
    }
}