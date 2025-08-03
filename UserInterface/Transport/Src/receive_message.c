#include "receive_message.h"
#include "stm32f4xx_hal.h"
#include "protocol.h"
#include "message_queue.h"
#include "message_processor.h"
#include "boot_state.h"
#include <stdio.h>
#include <string.h>

static uint8_t uart_rx_buffer[UART_RX_BUFFER_SIZE];

void receive_message_reset_state(void)
{
    boot_state_reset();
}

extern UART_HandleTypeDef huart4;

uint32_t calculate_crc32(const uint8_t *data, size_t length)
{
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++)
            crc = (crc >> 1) ^ (0xEDB88320U & -(crc & 1));
    }
    return ~crc;
}

bool validate_crc32(const uint8_t *data)
{
    uint32_t received_crc;
    memcpy(&received_crc, data + UART_MESSAGE_MAX_LENGTH - UART_CRC_LENGTH, UART_CRC_LENGTH);
    uint32_t computed_crc = calculate_crc32(data, UART_MESSAGE_MAX_LENGTH - UART_CRC_LENGTH);
    return (received_crc == computed_crc);
}

void send_ack(void)
{
    uint8_t ack = UART_ACK_BYTE;
    HAL_UART_Transmit(&huart4, &ack, 1, HAL_MAX_DELAY);  // Blocking mode
}

void send_nack(NackErrorCode_t error_code)
{
    uint8_t nack[2] = { UART_NACK_HEADER_BYTE, error_code };
    HAL_UART_Transmit(&huart4, nack, 2, HAL_MAX_DELAY);  // Blocking mode
}

static bool is_valid_command(const MessageType_t type, const uint8_t *data)
{
    if (type == MESSAGE_TYPE_CMD_WRITE || type == MESSAGE_TYPE_CMD_ERASE)
    {
        uint8_t sector = data[1];
        uint32_t address = get_sector_start_address(sector);
        return (address != 0xFFFFFFFF);
    }
    return true;
}

static bool is_write_sequence_respected(const MessageType_t type)
{
    switch (bootState.current_state)
    {
        case STATE_WAIT_CMD:
            if (type == MESSAGE_TYPE_CMD_WRITE)
            {
                bootState.current_state = STATE_WAIT_DATA_OR_FINISH;
                return true;
            }
            else if (type == MESSAGE_TYPE_CMD_ERASE)
            {
                bootState.current_state = STATE_WAIT_FINISH;
                return true;
            }
            return false;

        case STATE_WAIT_DATA_OR_FINISH:
            if (type == MESSAGE_TYPE_DATA)
                return true;
            else if (type == MESSAGE_TYPE_FINISH)
            {
                bootState.current_state = STATE_WAIT_CMD;
                return true;
            }
            return false;

        case STATE_WAIT_FINISH:
            if (type == MESSAGE_TYPE_FINISH)
            {
                bootState.current_state = STATE_WAIT_CMD;
                return true;
            }
            return false;

        case STATE_IDLE:
            // Pasif durumdan yeniden komut kabul et
            bootState.current_state = STATE_WAIT_CMD;
            return is_write_sequence_respected(type);
        default:
            return false;
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance != UART4)
        return;

    MessageType_t type = (MessageType_t)uart_rx_buffer[0];
    if (!validate_crc32(uart_rx_buffer))
    {
        send_nack(NACK_INVALID_CRC);
    }
    else if (!is_write_sequence_respected(type))
    {
        send_nack(NACK_SEQUENCE_ERROR);
    }
    else if (!is_valid_command(type, uart_rx_buffer))
    {
        send_nack(NACK_INVALID_COMMAND);
    }
    else if (!enqueue_message(type, uart_rx_buffer, UART_MESSAGE_MAX_LENGTH))
    {
        send_nack(NACK_QUEUE_FULL);
    }
    else
    {
        send_ack();
    }

    receive_message_init();
}

void receive_message_init(void)
{
    HAL_UART_Receive_IT(&huart4, uart_rx_buffer, UART_MESSAGE_MAX_LENGTH);
}
