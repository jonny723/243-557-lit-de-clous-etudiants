/*
 * command_handler.h
 *
 *  Created on: 16 oct. 2026
 *      Author: Jonathan Marois
 */

#ifndef INC_COMMAND_HANDLER_H_
#define INC_COMMAND_HANDLER_H_

#include <stdint.h>

void CommandHandler_Process(const uint8_t *command, uint32_t length);
void CommandHandler_Task(void);

#endif /* INC_COMMAND_HANDLER_H_ */
