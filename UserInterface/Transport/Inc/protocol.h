/*
 * protocol.h
 */

#ifndef TRANSPORT_INC_PROTOCOL_H_
#define TRANSPORT_INC_PROTOCOL_H_

#define UART_MESSAGE_MAX_LENGTH         21							// Same length for all message types
#define UART_RX_BUFFER_SIZE             UART_MESSAGE_MAX_LENGTH

#define UART_CMD_PAYLOAD_LENGTH         1   						// Sector number
#define UART_DATA_PAYLOAD_LENGTH        16  						// Data length (adjust 4x for writing as a word)
#define UART_FINISH_PAYLOAD_LENGTH      0

#define UART_CRC_LENGTH                 4							// CRC32

#define UART_CMD_MESSAGE_LENGTH         UART_MESSAGE_MAX_LENGTH    	// 21 bytes total
#define UART_DATA_MESSAGE_LENGTH        UART_MESSAGE_MAX_LENGTH    	// 21 bytes total
#define UART_FINISH_MESSAGE_LENGTH      UART_MESSAGE_MAX_LENGTH   	// 21 bytes total

#define UART_ACK_BYTE                   0xAA
#define UART_NACK_HEADER_BYTE           0x55

typedef enum {
    MESSAGE_TYPE_CMD_WRITE = 0x01,
    MESSAGE_TYPE_CMD_ERASE = 0x02,
    MESSAGE_TYPE_DATA      = 0x03,
    MESSAGE_TYPE_FINISH    = 0x04
} MessageType_t;

typedef enum {
    STATE_WAIT_CMD,
    STATE_WAIT_DATA_OR_FINISH,
    STATE_WAIT_FINISH,
    STATE_IDLE
} BootloaderState_t;

typedef enum {
    NACK_INVALID_TYPE       = 0x01,
    NACK_INVALID_CRC        = 0x02,
    NACK_QUEUE_FULL         = 0x03,
    NACK_INVALID_COMMAND    = 0x04,
    NACK_SEQUENCE_ERROR     = 0x05
} NackErrorCode_t;

#endif /* TRANSPORT_INC_PROTOCOL_H_ */
