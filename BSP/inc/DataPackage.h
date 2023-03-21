#ifndef __DATAPACKAGE_H
#define __DATAPACKAGE_H

#include <stdint.h>

uint16_t CRC16_CALCULATE(uint8_t *data, uint16_t len);
void PackageData(uint16_t header, uint16_t data_rows, uint8_t *data, uint16_t crc_code);


#endif // DATAPACKAGE_H
