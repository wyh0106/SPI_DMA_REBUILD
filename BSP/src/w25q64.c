#include "w25q64.h"

extern uint8_t flag;
uint8_t W25QXX_BUFFER[4096];

/**
 * @brief W25Q64初始化
 * @retval W25Q64状态
 */
uint8_t BSP_W25Qx_Init(void)
{
    BSP_W25Qx_Reset();
    return BSP_W25Qx_GetStatus();
}

static void BSP_W25Qx_Reset(void)
{
    uint8_t cmd[2] = {RESET_ENABLE_CMD, RESET_MEMORY_CMD};

    W25Qx_Enable();
    /* Send the reset command */
    HAL_SPI_Transmit(&hspi2, cmd, 2, W25Qx_TIMEOUT_VALUE);
    W25Qx_Disable();
}

/**
 * @brief 获取W25Q64状态寄存器1读数
 * @retval W25Q64状态
 */
static uint8_t BSP_W25Qx_GetStatus(void)
{
    uint8_t cmd[] = {READ_STATUS_REG1_CMD};
    uint8_t status;

    W25Qx_Enable();
    /* Send the read status command */
    HAL_SPI_Transmit(&hspi2, cmd, 1, W25Qx_TIMEOUT_VALUE);
    /* Reception of the data */
    HAL_SPI_Receive(&hspi2, &status, 1, W25Qx_TIMEOUT_VALUE);
    W25Qx_Disable();

    /* Check the value of the register */
    if ((status & W25Q64_FSR_BUSY) != 0) {
        return W25Qx_BUSY;
    } else {
        return W25Qx_OK;
    }
}

/**
 * @brief W25Q64写使能
 * @retval W25Q64状态
 */
uint8_t BSP_W25Qx_WriteEnable(void)
{
    uint8_t cmd[]      = {WRITE_ENABLE_CMD};
    uint32_t tickstart = HAL_GetTick();

    /*Select the FLASH: Chip Select low */
    W25Qx_Enable();
    /* Send the read ID command */
    HAL_SPI_Transmit(&hspi2, cmd, 1, W25Qx_TIMEOUT_VALUE);
    /*Deselect the FLASH: Chip Select high */
    W25Qx_Disable();

    /* Wait the end of Flash writing */
    while (BSP_W25Qx_GetStatus() == W25Qx_BUSY) {
        /* Check for the Timeout */
        if ((HAL_GetTick() - tickstart) > W25Qx_TIMEOUT_VALUE) {
            return W25Qx_TIMEOUT;
        }
    }

    return W25Qx_OK;
}

/**
 * @brief 读取W25Q64唯一ID
 * @param *ID 存放ID的数组
 * @retval NONE
 */
void BSP_W25Qx_Read_ID(uint8_t *ID)
{
    uint8_t cmd[4] = {READ_ID_CMD, 0x00, 0x00, 0x00};

    W25Qx_Enable();
    /* Send the read ID command */
    HAL_SPI_Transmit(&hspi2, cmd, 4, W25Qx_TIMEOUT_VALUE);
    /* Reception of the data */
    HAL_SPI_Receive(&hspi2, ID, 2, W25Qx_TIMEOUT_VALUE);
    W25Qx_Disable();
}

/**
 * @brief 查询方式读数据（阻塞）
 * @param *pData    读缓存指针
 * @param ReadAddr  flash的地址
 * @param Size      字节大小
 * @retval W25Q64状态
 */
uint8_t BSP_W25Qx_Read(uint8_t *pData, uint32_t ReadAddr, uint32_t Size)
{
    uint8_t cmd[4] = {0};

    /* Configure the command */
    cmd[0] = READ_CMD;
    cmd[1] = (uint8_t)(ReadAddr >> 16);
    cmd[2] = (uint8_t)(ReadAddr >> 8);
    cmd[3] = (uint8_t)(ReadAddr);

    W25Qx_Enable();
    /* Send the read ID command */
    HAL_SPI_Transmit(&hspi2, cmd, 4, 10);
    /* Reception of the data */
    if (HAL_SPI_Receive(&hspi2, pData, Size, 10) != HAL_OK) {
        return W25Qx_ERROR;
    }

    W25Qx_Disable();

    return W25Qx_OK;
}

/**
 * @brief DMA方式读数据（非阻塞）
 * @param *pData    读缓存指针
 * @param ReadAddr  flash的地址
 * @param Size      字节大小
 * @retval W25Q64状态
 */
uint8_t BSP_W25Qx_ReadDMA(uint8_t *pData, uint32_t ReadAddr, uint32_t Size)
{
    uint8_t cmd[4] = {0};

    /* Configure the command */
    cmd[0] = READ_CMD;
    cmd[1] = (uint8_t)(ReadAddr >> 16);
    cmd[2] = (uint8_t)(ReadAddr >> 8);
    cmd[3] = (uint8_t)(ReadAddr);

    W25Qx_Enable();

    HAL_SPI_Transmit(&hspi2, cmd, 1, 10); // 发送读取命令
    /* Reception of the data */
    if (HAL_SPI_TransmitReceive_DMA(&hspi2, &cmd[1], pData - 3, 3 + Size)) // 发送24bit地址 != HAL_OK)
    {
        return W25Qx_ERROR;
    }

    // W25Qx_Disable();

    return W25Qx_OK;
}

/**
 * @brief   DMA方式无检验写数据
 * @param   *pData    写缓存指针
 * @param   WriteAddr flash的地址
 * @param   Size      字节大小
 * @note    该函数写数据前不会自动擦除，使用前需要提前擦除数据，虽然是DMA，但是每写一页数据都需要等待，相当于半阻塞。
 * @retval  W25Q64状态
 */
uint8_t BSP_W25Qx_Write(uint8_t *pData, uint32_t WriteAddr, uint32_t Size)
{
    uint8_t cmd[4];
    uint32_t end_addr, current_size, current_addr;
    uint32_t tickstart = HAL_GetTick();

    // 计算写入地址和页末地址之间的大小
    current_addr = 0;

    while (current_addr <= WriteAddr) {
        current_addr += W25Q64_PAGE_SIZE;
    }
    current_size = current_addr - WriteAddr;

    // 检查数据的大小是否小于页中的剩余位置
    if (current_size > Size) {
        current_size = Size;
    }

    // 初始化地址变量
    current_addr = WriteAddr;
    end_addr     = WriteAddr + Size;

    // 逐页写入，每次写入一页数据
    do
    {
        cmd[0] = PAGE_PROG_CMD;
        cmd[1] = (uint8_t)(current_addr >> 16);
        cmd[2] = (uint8_t)(current_addr >> 8);
        cmd[3] = (uint8_t)(current_addr);

        // 写使能
        BSP_W25Qx_WriteEnable();

        W25Qx_Enable();
        // 发送写入命令和写入地址
        if (HAL_SPI_Transmit(&hspi2, cmd, 4, 10) != HAL_OK) {
            return W25Qx_ERROR;
        }

        // SPI2发送数据缓冲区写入数据到当前地址
        if (HAL_SPI_Transmit_DMA(&hspi2, pData, current_size) != HAL_OK){
            return W25Qx_ERROR;
        }

        if(flag){
            HAL_Delay(1);
        }
        flag = 1;

        // 等待flash完成写入
        while (BSP_W25Qx_GetStatus() == W25Qx_BUSY) {
            /* Check for the Timeout */
            if ((HAL_GetTick() - tickstart) > W25Qx_TIMEOUT_VALUE) {
                return W25Qx_TIMEOUT;
            }
        }

        // 为下一页编程更新地址和大小变量
        current_addr += current_size;
        pData += current_size;
        current_size = ((current_addr + W25Q64_PAGE_SIZE) > end_addr) ? (end_addr - current_addr) : W25Q64_PAGE_SIZE;
    } while (current_addr < end_addr);

    return W25Qx_OK;
}

/**
 * @brief   页写入
 * @param   *pBuffer          写缓存指针
 * @param   WriteAddr         flash的地址
 * @param   NumByteToWrite    字节大小（最大256）
 * @note    轮询方式写入（阻塞），NumByteToWrite不应超过该页的剩余字节数
 * @retval  NONE
 */
static void BSP_W25Qx_WritePage(uint8_t *pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite)
{
    uint32_t i;
    uint8_t cmd[4];
    cmd[0] = PAGE_PROG_CMD;
    cmd[1] = (uint8_t)((WriteAddr) >> 16);
    cmd[2] = (uint8_t)((WriteAddr) >> 8);
    cmd[3] = (uint8_t)(WriteAddr);

    BSP_W25Qx_WriteEnable(); // 写使能

    while (BSP_W25Qx_GetStatus() == W25Qx_BUSY)
        ;

    W25Qx_Enable();                                          // 使能器件
    HAL_SPI_Transmit(&hspi2, cmd, 4, 10);                    // 发送写页命令
    HAL_SPI_Transmit(&hspi2, pBuffer, NumByteToWrite, 4000); // 发送24bit地址

    W25Qx_Disable(); // 取消片选
    while (BSP_W25Qx_GetStatus() == W25Qx_BUSY)
        ; // 等待写入结束
}

/**
 * @brief   无检验写数据
 * @param   *pBuffer          写缓存指针
 * @param   WriteAddr         flash的地址
 * @param   NumByteToWrite    字节大小(最大65535)
 * @note    该方案为轮询写入（阻塞）
 * @retval  NONE
 */
static void BSP_W25Qx_WriteNoCheck(uint8_t *pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite)
{
    uint16_t PageRemain;
    uint16_t NumByteToWriteNow;
    PageRemain        = W25Q64_PAGE_SIZE - WriteAddr % W25Q64_PAGE_SIZE; // 单页剩余的字节数
    NumByteToWriteNow = NumByteToWrite;
    if (NumByteToWrite <= PageRemain)
        PageRemain = NumByteToWriteNow; // 不大于256个字节
    while (1)
    {
        BSP_W25Qx_WritePage(pBuffer, WriteAddr, PageRemain);

        if (NumByteToWriteNow == PageRemain) // 写入结束
            break; 
        else // NumByteToWrite>PageRemain
        {
            pBuffer += PageRemain;
            WriteAddr += PageRemain;

            NumByteToWriteNow -= PageRemain;    // 减去已经写入了的字节数
            if (NumByteToWriteNow > W25Q64_PAGE_SIZE)
                PageRemain = W25Q64_PAGE_SIZE;  // 一次可以写入256个字节
            else
                PageRemain = NumByteToWriteNow; // 不够256个字节了
        }
    };
}

/**
 * @brief   通用写数据
 * @param   *pData    写缓存指针
 * @param   WriteAddr flash的地址
 * @param   Size      字节大小
 * @note    会自动识别目标地址所在Sector是否写入数据，如果写入了数据，则会自动擦除
 * @retval  NONE
 */
void BSP_W25Qx_EraseWrite(uint8_t *pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite)
{
    uint16_t secpos;
    uint16_t secoff;
    uint16_t secremain;
    uint16_t i;
    uint8_t *W25QXX_BUF;

    W25QXX_BUF = W25QXX_BUFFER;
    secpos     = WriteAddr / W25Q64_SECTOR_SIZE;    // 扇区地址
    secoff     = WriteAddr % W25Q64_SECTOR_SIZE;    // 在扇区内的偏移
    secremain  = W25Q64_SECTOR_SIZE - secoff;       // 扇区剩余空间大小

    if (NumByteToWrite <= secremain)
        secremain = NumByteToWrite;                 // 不大于4096个字节

    while (1) {
        BSP_W25Qx_Read(W25QXX_BUF, secpos * W25Q64_SECTOR_SIZE, W25Q64_SECTOR_SIZE); // 读出整个扇区的内容

        for (i = 0; i < secremain; i++)             // 校验是否存有数据
        {
            if (W25QXX_BUF[secoff + i] != 0XFF)     // 需要擦除
                break; 
        }

        if (i < secremain)                          // 判断是否擦除
        {
            BSP_W25Qx_Erase_Sector(secpos);         // 擦除这个扇区
            for (i = 0; i < secremain; i++)         // 复制
            {
                W25QXX_BUF[i + secoff] = pBuffer[i];
            }
            BSP_W25Qx_Write(W25QXX_BUF, secpos * W25Q64_SECTOR_SIZE, W25Q64_SECTOR_SIZE);   // 写入整个扇区

        }
        else
            BSP_W25Qx_Write(pBuffer, WriteAddr, secremain);     // 写入已擦除扇区,直接写入剩余区间.

        if (NumByteToWrite == secremain)                        // 写入结束
            break;                          
        else                                                    // 写入未结束
        {
            secpos++;                                           // 扇区地址增1
            secoff = 0;                                         // 偏移位置为0
            pBuffer += secremain;                               // 指针偏移
            WriteAddr += secremain;                             // 写地址偏移
            NumByteToWrite -= secremain;                        // 字节数递减
            if (NumByteToWrite > W25Q64_SECTOR_SIZE)
                secremain = W25Q64_SECTOR_SIZE;                 // 下一个扇区写不完
            else
                secremain = NumByteToWrite;                     // 下一个扇区能写完
        }
    };
}

/**
 * @brief   扇区擦除
 * @param   Address     擦除扇区的地址
 * @retval  W25Q64状态
 */
uint8_t BSP_W25Qx_Erase_Sector(uint32_t Address)
{
    uint8_t cmd[4];
    uint32_t tickstart = HAL_GetTick();
    cmd[0]             = SECTOR_ERASE_CMD;
    cmd[1]             = (uint8_t)(Address >> 16);
    cmd[2]             = (uint8_t)(Address >> 8);
    cmd[3]             = (uint8_t)(Address);

    /* Enable write operations */
    BSP_W25Qx_WriteEnable();

    /*Select the FLASH: Chip Select low */
    W25Qx_Enable();
    /* Send the read ID command */
    HAL_SPI_Transmit(&hspi2, cmd, 4, W25Qx_TIMEOUT_VALUE);
    /*Deselect the FLASH: Chip Select high */
    W25Qx_Disable();

    /* Wait the end of Flash writing */
    while (BSP_W25Qx_GetStatus() == W25Qx_BUSY)
        ;
    {
        /* Check for the Timeout */
        if ((HAL_GetTick() - tickstart) > W25Q64_SECTOR_ERASE_MAX_TIME) {
            return W25Qx_TIMEOUT;
        }
    }
    return W25Qx_OK;
}

/**
 * @brief   全片擦除
 * @retval  W25Q64
 */
uint8_t BSP_W25Qx_Erase_Chip(void)
{
    uint8_t cmd[4];
    uint32_t tickstart = HAL_GetTick();
    cmd[0]             = CHIP_ERASE_CMD;

    /* Enable write operations */
    BSP_W25Qx_WriteEnable();

    /*Select the FLASH: Chip Select low */
    W25Qx_Enable();
    /* Send the read ID command */
    HAL_SPI_Transmit(&hspi2, cmd, 1, W25Qx_TIMEOUT_VALUE);
    /*Deselect the FLASH: Chip Select high */
    W25Qx_Disable();

    /* Wait the end of Flash writing */
    while (BSP_W25Qx_GetStatus() != W25Qx_BUSY)
        ;
    {
        /* Check for the Timeout */
        if ((HAL_GetTick() - tickstart) > W25Q64_BULK_ERASE_MAX_TIME) {
            return W25Qx_TIMEOUT;
        }
    }
    return W25Qx_OK;
}
