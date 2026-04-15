#pragma once

#include <vector>
#include <string>
#include <stdint.h>

class DtmReader
{

public:
    static bool readElevationData(std::string strFileName, uint32_t size, std::vector<int16_t>& heights);
};
