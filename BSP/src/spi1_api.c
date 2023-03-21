#include "spi1_api.h"

/**
  * @brief	通过SPI1的DMA方式发送数据包
  * @param	data 数据帧
  * @retval	SPI发送状态
  */
uint8_t SPI1_SendDataPack(uint8_t* data)
{
    uint16_t crc_code = CRC16_CALCULATE(data, 1620);            // 计算CRC校验

    PackageData(header, data_rows, data, crc_code);             // 数据包封装

    if(HAL_OK != HAL_SPI_Transmit_DMA(&hspi1, DataPack, 1626))  // DMA发送
    {
        return SPI1_ERROR;
    }
    else
    {
        return SPI1_OK;
    }
}
