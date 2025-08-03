/*
 * boot_state.h
 *
 *  Created on: Jul 29, 2025
 *      Author: hp
 */

#ifndef TRANSPORT_INC_BOOT_STATE_H_
#define TRANSPORT_INC_BOOT_STATE_H_

#include "protocol.h"
#include <stdint.h>

typedef struct {
    MessageType_t last_cmd;
    BootloaderState_t current_state;
    uint32_t last_msg_tick;
} BootState_t;

extern BootState_t bootState;

void boot_state_init(void);
void boot_state_reset(void);

#endif /* TRANSPORT_INC_BOOT_STATE_H_ */ 