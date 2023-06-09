#ifndef __SPI1_API_H
#define __SPI1_API_H

#include "stm32f4xx.h"
#include "spi.h"
#include "DataPackage.h"

#define SPI1_ERROR  1;
#define SPI1_OK     0;

extern uint8_t DataPack[1626];
extern uint16_t header, data_rows;

uint8_t SPI1_SendDataPack(uint8_t* data);

#endif // __SPI1_API_H