#include "message_processor.h"
#include "message_queue.h"
#include "protocol.h"
#include "receive_message.h"
#include "boot_state.h"
#include "stm32f4xx_hal.h"
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#define APP_FLASH_START_ADDRESS  0x08008000U

static uint32_t current_flash_address = APP_FLASH_START_ADDRESS;
static uint8_t target_sector = 0;
#define PACKET_TIMEOUT_MS  7000U

static const uint32_t sectorStart[12] = {
    0x08000000, 0x08004000, 0x08008000, 0x0800C000,
    0x08010000, 0x08020000, 0x08040000, 0x08060000,
    0x08080000, 0x080A0000, 0x080C0000, 0x080E0000
};

uint32_t get_sector_start_address(uint8_t sector)
{
    if (sector < 12) {
        return sectorStart[sector];
    }
    return 0xFFFFFFFF;
}

void jump_to_application(uint8_t sector)
{
    uint32_t app_address = get_sector_start_address(sector);
    uint32_t app_stack = *(volatile uint32_t*)app_address;
    uint32_t app_reset_handler = *(volatile uint32_t*)(app_address + 4);

    typedef void (*pFunction)(void);
    pFunction app_entry = (pFunction)app_reset_handler;

    __disable_irq();

    HAL_RCC_DeInit();
    HAL_DeInit();

    SCB->VTOR = app_address;

    __set_MSP(app_stack);
    app_entry();
}



void process_message_task(void)
{
    Message_t message;

    if (!is_message_available() || !dequeue_message(&message))
        return;

    // Mesaj alındı, zaman damgası güncelle
    bootState.last_msg_tick = HAL_GetTick();

    switch (message.type)
    {
        case MESSAGE_TYPE_CMD_WRITE:
            bootState.last_cmd = MESSAGE_TYPE_CMD_WRITE;
            target_sector = message.data[1];  // data[0] = type, data[1] = sector
            current_flash_address = get_sector_start_address(target_sector);

            if (current_flash_address != 0xFFFFFFFF)
            {
                HAL_FLASH_Unlock();
                FLASH_Erase_Sector(target_sector, VOLTAGE_RANGE_3);
                HAL_FLASH_Lock();
            }
            break;

        case MESSAGE_TYPE_CMD_ERASE:
            bootState.last_cmd = MESSAGE_TYPE_CMD_ERASE;
            target_sector = message.data[1];
            current_flash_address = get_sector_start_address(target_sector);

            if (current_flash_address != 0xFFFFFFFF)
            {
                HAL_FLASH_Unlock();
                FLASH_Erase_Sector(target_sector, VOLTAGE_RANGE_3);
                HAL_FLASH_Lock();
            }
            break;

        case MESSAGE_TYPE_DATA:
            HAL_FLASH_Unlock();
            for (int i = 0; i < UART_DATA_PAYLOAD_LENGTH; i += 4)
            {
                uint32_t word = 0;
                word |= ((uint32_t)message.data[i + 1]) << 0;
                word |= ((uint32_t)message.data[i + 2]) << 8;
                word |= ((uint32_t)message.data[i + 3]) << 16;
                word |= ((uint32_t)message.data[i + 4]) << 24;

                if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, current_flash_address, word) == HAL_OK)
                {
                    //printf("Wrote 0x%08lX to 0x%08lX\n", word, current_flash_address);
                    current_flash_address += 4;
                }
                else
                {
                    //printf("Write FAILED at 0x%08lX\n", current_flash_address);
                }
            }
            HAL_FLASH_Lock();
            break;

        case MESSAGE_TYPE_FINISH:
            if (bootState.last_cmd == MESSAGE_TYPE_CMD_WRITE)
            {
            	// Program update tamamlandı, uygulamaya geç
            	jump_to_application(target_sector);
            }
            // ERASE sonrası yalnızca bekleme moduna dön
            bootState.last_cmd = MESSAGE_TYPE_FINISH;
            break;

        default:
    break;
    }

}

bool is_transfer_active(void)
{
    return (bootState.last_cmd == MESSAGE_TYPE_CMD_WRITE || bootState.last_cmd == MESSAGE_TYPE_CMD_ERASE);
}

void message_timeout_task(void)
{
    if (bootState.last_cmd == MESSAGE_TYPE_FINISH)
        return;

    uint32_t now = HAL_GetTick();
    if ((now - bootState.last_msg_tick) < PACKET_TIMEOUT_MS)
        return;

    // Timeout gerçekleşti
    if (bootState.last_cmd == MESSAGE_TYPE_CMD_WRITE)
    {
        if (target_sector <= 11)
        {
            HAL_FLASH_Unlock();
            FLASH_Erase_Sector(target_sector, VOLTAGE_RANGE_3);
            HAL_FLASH_Lock();
        }
    }

    // Durum makinelerini sıfırla
    bootState.last_cmd = MESSAGE_TYPE_FINISH;
    receive_message_reset_state();
}
