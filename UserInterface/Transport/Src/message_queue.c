/*
 * message_queue.c
 *
 *  Created on: Jul 28, 2025
 *      Author: hp
 */


#include "message_queue.h"
#include <string.h>
#include "stm32f4xx_hal.h"

static Message_t queue[MESSAGE_QUEUE_SIZE];
static volatile uint8_t head = 0;
static volatile uint8_t tail = 0;
static volatile uint8_t count = 0;

void message_queue_init(void) {
    head = 0;
    tail = 0;
    count = 0;
}

bool enqueue_message(MessageType_t type, const uint8_t *data, uint8_t length) {
    if (length > UART_MESSAGE_MAX_LENGTH)
        return false;

    __disable_irq();

    if (count >= MESSAGE_QUEUE_SIZE) {
        __enable_irq();
        return false;
    }

    queue[tail].type = type;
    queue[tail].length = length;
    memcpy(queue[tail].data, data, length);

    tail = (tail + 1) % MESSAGE_QUEUE_SIZE;
    count++;

    __enable_irq();
    return true;
}

bool dequeue_message(Message_t *out_message) {
    __disable_irq();

    if (count == 0) {
        __enable_irq();
        return false;
    }

    *out_message = queue[head];
    head = (head + 1) % MESSAGE_QUEUE_SIZE;
    count--;

    __enable_irq();
    return true;
}

bool is_message_available(void) {
    return (count > 0);
}
