/*
 * message_queue.h
 *
 *  Created on: Jul 28, 2025
 *      Author: Sema Dilber
 */

#ifndef MESSAGE_QUEUE_H
#define MESSAGE_QUEUE_H

#include <stdint.h>
#include <stdbool.h>
#include "protocol.h"

#define MESSAGE_QUEUE_SIZE  15

typedef struct {
    MessageType_t type;
    uint8_t data[UART_MESSAGE_MAX_LENGTH];
    uint8_t length;
} Message_t;


void message_queue_init(void);
bool enqueue_message(MessageType_t type, const uint8_t *data, uint8_t length);
bool dequeue_message(Message_t *out_message);
bool is_message_available(void);

#endif // MESSAGE_QUEUE_H
