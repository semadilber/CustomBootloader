/*
 * message_processor.h
 *
 *  Created on: Jul 29, 2025
 *      Author: hp
 */

#ifndef APPLICATION_INC_MESSAGE_PROCESSOR_H_
#define APPLICATION_INC_MESSAGE_PROCESSOR_H_

#include <stdint.h>
#include <stdbool.h>

uint32_t get_sector_start_address(uint8_t sector);

void process_message_task(void);
void message_timeout_task(void);
bool is_transfer_active(void);

#endif /* APPLICATION_INC_MESSAGE_PROCESSOR_H_ */
