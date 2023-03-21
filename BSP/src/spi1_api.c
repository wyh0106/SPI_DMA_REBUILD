#include "spi1_api.h"

/**
  * @brief	通过SPI1的DMA方式发送数据包
  * @param	*pBuffer 发送数据的数据帧
  * @retval	SPI发送状态
  */
uint8_t SPI1_SendDataPack(uint8_t* pBuffer)
{
    uint16_t crc_code = CRC16_CALCULATE(pBuffer, 1620);             // 计算CRC校验

    PackageData(header, data_rows, pBuffer, crc_code);              // 数据包封装

    if(HAL_OK != HAL_SPI_Transmit_DMA(&hspi1, DataPack, 1626))      // DMA发送
    {
        return SPI1_ERROR;
    }

    return SPI1_OK;

}

/**
  * @brief	通过SPI1的DMA方式接收数据包
  * @param	*pBuffer 接收数据缓冲区
  * @retval	SPI发送状态
  */
uint8_t SPI1_ReceiveDataPack(uint8_t* pBuffer)
{
    if(HAL_OK != HAL_SPI_Receive_DMA(&hspi1, pBuffer, 1626))        // DMA接收
    {
        return SPI1_ERROR;
    }

    return SPI1_OK;

}

/**
  * @brief	通过SPI1的DMA方式同时发送和接收数据包
  * @param	*pTxBuffer 发送数据的数据帧
  * @param	*pRxBuffer 接收数据缓冲区
  * @retval	SPI发送状态
  */
uint8_t SPI1_TxRxDataPack(uint8_t* pTxBuffer, uint8_t* pRxBuffer)
{
    uint16_t crc_code = CRC16_CALCULATE(pTxBuffer, 1620);             // 计算CRC校验
    PackageData(header, data_rows, pTxBuffer, crc_code);              // 数据包封装

    if(HAL_OK != HAL_SPI_TransmitReceive_DMA(&hspi1, DataPack, pRxBuffer, 1626))      // DMA发送
    {
        return SPI1_ERROR;
    }

    return SPI1_OK;

}
