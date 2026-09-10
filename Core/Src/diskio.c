/*-----------------------------------------------------------------------*/
/* Low level disk I/O module for STM32 SDIO (FatFs)                      */
/*-----------------------------------------------------------------------*/

#include "ff.h"
#include "diskio.h"
#include "stm32f4xx_hal.h"

extern SD_HandleTypeDef hsd;

/* Disk status */
static DSTATUS Stat = STA_NOINIT;

/*-----------------------------------------------------------------------*/
/* Initialize Disk Drive                                                 */
/*-----------------------------------------------------------------------*/
DSTATUS disk_initialize(BYTE pdrv)
{
    if (pdrv != 0) return STA_NOINIT;

    if (HAL_SD_Init(&hsd) == HAL_OK) {
        /* main.c 里只把 hsd.Init.BusWide 设成 4B, 但真正切到 4 位总线
           必须在卡初始化完成后显式调用 HAL_SD_ConfigWideBusOperation(),
           否则一直跑在 1 位模式 (速度只有 1/4)。
           4 位协商失败时退回 1 位, 保证至少还能读写。 */
        if (HAL_SD_ConfigWideBusOperation(&hsd, SDIO_BUS_WIDE_4B) != HAL_OK) {
            HAL_SD_ConfigWideBusOperation(&hsd, SDIO_BUS_WIDE_1B);
        }
        Stat &= ~STA_NOINIT;
    }

    return Stat;
}

/*-----------------------------------------------------------------------*/
/* Get Disk Status                                                       */
/*-----------------------------------------------------------------------*/
DSTATUS disk_status(BYTE pdrv)
{
    if (pdrv != 0) return STA_NOINIT;
    return Stat;
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/
DRESULT disk_read(BYTE pdrv, BYTE* buff, LBA_t sector, UINT count)
{
    if (pdrv != 0) return RES_PARERR;
    if (Stat & STA_NOINIT) return RES_NOTRDY;

    if (HAL_SD_ReadBlocks(&hsd, buff, sector, count, HAL_MAX_DELAY) == HAL_OK) {
        return RES_OK;
    }

    return RES_ERROR;
}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/
#if FF_FS_READONLY == 0
DRESULT disk_write(BYTE pdrv, const BYTE* buff, LBA_t sector, UINT count)
{
    if (pdrv != 0) return RES_PARERR;
    if (Stat & STA_NOINIT) return RES_NOTRDY;

    if (HAL_SD_WriteBlocks(&hsd, (uint8_t*)buff, sector, count, HAL_MAX_DELAY) == HAL_OK) {
        return RES_OK;
    }

    return RES_ERROR;
}
#endif

/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/
DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void* buff)
{
    if (pdrv != 0) return RES_PARERR;
    if (Stat & STA_NOINIT) return RES_NOTRDY;

    switch (cmd) {
        case CTRL_SYNC:
            /* Make sure that no pending write process */
            if (HAL_SD_GetCardState(&hsd) == HAL_SD_CARD_TRANSFER) {
                return RES_OK;
            }
            return RES_ERROR;

        case GET_SECTOR_COUNT:
            /* Get number of sectors on the disk (DWORD) */
            *(LBA_t*)buff = hsd.SdCard.BlockNbr;
            return RES_OK;

        case GET_SECTOR_SIZE:
            /* Get R/W sector size (WORD) */
            *(WORD*)buff = hsd.SdCard.BlockSize;
            return RES_OK;

        case GET_BLOCK_SIZE:
            /* Get erase block size in unit of sector (DWORD) */
            *(DWORD*)buff = hsd.SdCard.LogBlockSize / hsd.SdCard.BlockSize;
            return RES_OK;

        default:
            return RES_PARERR;
    }
}
