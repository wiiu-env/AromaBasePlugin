#pragma once

#include <string>

#include <cstdint>

class DownloadUtils {
public:
    static bool Init();
    static void Deinit();

    static int DownloadFileToBuffer(const std::string &url, std::string &outBuffer, int &responseCodeOut, int &errorOut, std::string &errorTextOut, float *progress);

    static bool libInitDone;
};