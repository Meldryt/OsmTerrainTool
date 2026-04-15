#include "DtmReader.h"
#include <iostream>
#include <fstream>

bool DtmReader::readElevationData(std::string strFileName, uint32_t size, std::vector<int16_t>& heights)
{
    std::ifstream file(strFileName, std::ios::in | std::ios::binary);
    if (!file)
    {
        std::cout << __FUNCTION__ << " Error opening file! " << strFileName << std::endl;
        return false;
    }

    int16_t min_height = 10000;
    int16_t max_height = -10000;

    unsigned char buffer[2];

    for (int32_t y = size-1; y >= 0; --y)
    {
        for (uint32_t x = 0; x < size; ++x)
        {
            if (!file.read(reinterpret_cast<char*>(buffer), sizeof(buffer)))
            {
                std::cout << "DtmReader::readElevationData() Error reading file!" << std::endl;
                return false;
            }

            uint32_t index = x + y * size;

            int16_t height = (buffer[0] << 8) | buffer[1];

            //@todo: how to handle invalid heights?
            if(height > 10000)
            {
                height = 0;
            }

            heights[index] = height;

            if (min_height > height)
            {
                min_height = height;
            }
            else if (max_height < height)
            {
                max_height = heights[index];
            }
        }
    }

    std::cout << __FUNCTION__ << " " << strFileName  << " min_height: " << min_height << " max_height: " << max_height << std::endl;

    return true;
}
