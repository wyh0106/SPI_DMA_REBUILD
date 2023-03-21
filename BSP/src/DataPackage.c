#include "DataPackage.h"

uint16_t header = 0xFCCF, data_rows = 0;
uint8_t DataPack[1626];

/**
 * @brief	该函数为将数据头，数据行数，数据帧，CRC校验封装为数据包
 * @param	header		数据缓存数组指针
 * @param	data_rows	数据缓存位长
 * @param	*data		数据帧首地址
 * @param	crc_code	校验位
 * @retval	数据包缓存数组首地址
 */
void PackageData(uint16_t header, uint16_t data_rows, uint8_t *data, uint16_t crc_code)
{
    uint32_t i;

    DataPack[0] = (uint8_t)(header & 0x00FF);
    DataPack[1] = (uint8_t)((header & 0xFF00) >> 0x08);
    DataPack[2] = (uint8_t)(data_rows & 0x00FF);
    DataPack[3] = (uint8_t)((data_rows & 0xFF00) >> 0x08);
    for (i = 4; i < 1624; i++)
        DataPack[i] = data[i - 4];
    DataPack[1624] = (uint8_t)(crc_code & 0x00FF);
    DataPack[1625] = (uint8_t)((crc_code & 0xFF00) >> 0x08);
}

/**
 * @brief	该函数为发送数据缓存生成基于CRC16 XMODEM的校验码
 * @param	*data	数据缓存数组指针.
 * @param	len		数据缓存位长
 * @retval	无符号整型16位CRC校验码
 */
uint16_t CRC16_CALCULATE(uint8_t *data, uint16_t len)
{
    uint16_t i, crc = 0x0000;

    while (len--) {
        crc ^= (unsigned short)(*data++) << 8;
        for (i = 0; i < 8; ++i) {
            if (crc & 0x8000)
                crc = (crc << 1) ^ 0x1021;
            else
                crc <<= 1;
        }
    }
    return crc;
}