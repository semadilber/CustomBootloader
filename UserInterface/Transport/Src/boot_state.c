/*
 * boot_state.c
 *
 *  Created on: Jul 29, 2025
 *      Author: hp
 */

#include "boot_state.h"

BootState_t bootState = {0};

void boot_state_init(void)
{
    bootState.last_cmd = MESSAGE_TYPE_FINISH;
    bootState.current_state = STATE_WAIT_CMD;
    bootState.last_msg_tick = 0;
}

void boot_state_reset(void)
{
    bootState.last_cmd = MESSAGE_TYPE_FINISH;
    bootState.current_state = STATE_WAIT_CMD;
} 