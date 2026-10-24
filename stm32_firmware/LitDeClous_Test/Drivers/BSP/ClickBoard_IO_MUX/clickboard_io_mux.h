/*
 * clickboard_io_mux.h
 *
 *  Created on: 12 oct. 2026
 *      Author: Jonathan Marois
 */

#ifndef BSP_CLICKBOARD_IO_MUX_CLICKBOARD_IO_MUX_H_
#define BSP_CLICKBOARD_IO_MUX_CLICKBOARD_IO_MUX_H_

#include "stm32f4xx_hal.h"
#include "tca6416a.h"
#include "mcp3423.h"

uint8_t CommandHandler_IsResponsePending(void);

typedef struct
{
    TCA6416A_HandleTypeDef tca;
    MCP3423_HandleTypeDef mcp;
    uint8_t selected_channel;

} ClickBoard_HandleTypeDef;

HAL_StatusTypeDef ClickBoard_InitPWM(
    ClickBoard_HandleTypeDef *dev,
    I2C_HandleTypeDef *hi2c
);

/* Initialisation de la ClickBoard */
HAL_StatusTypeDef ClickBoard_Init(
    ClickBoard_HandleTypeDef *dev,
    I2C_HandleTypeDef *hi2c,
    MCP3423_Resolution resolution
);

/* Sélection d'une entrée S0 à S7 */
HAL_StatusTypeDef ClickBoard_SelectChannel(
    ClickBoard_HandleTypeDef *dev,
    uint8_t channel
);

/* Désactivation du multiplexeur */
HAL_StatusTypeDef ClickBoard_DisableMux(
    ClickBoard_HandleTypeDef *dev
);

/* Lecture de la tension sur le canal sélectionné */
HAL_StatusTypeDef ClickBoard_ReadVoltage(
    ClickBoard_HandleTypeDef *dev,
    float *voltage
);

/* Sélection d'un canal S0 à S7 et lecture de sa tension */
HAL_StatusTypeDef ClickBoard_ReadChannel(
    ClickBoard_HandleTypeDef *dev,
    uint8_t channel,
    float *voltage
);


/* Configuration d'une broche numérique */
HAL_StatusTypeDef ClickBoard_PinMode(
    ClickBoard_HandleTypeDef *dev,
    uint8_t pin,
    uint8_t mode);

/* Écriture sur une sortie numérique */
HAL_StatusTypeDef ClickBoard_WritePin(
    ClickBoard_HandleTypeDef *dev,
    uint8_t pin,
    uint8_t value);

/* Lecture d'une entrée numérique */
HAL_StatusTypeDef ClickBoard_ReadPin(
    ClickBoard_HandleTypeDef *dev,
    uint8_t pin,
    uint8_t *value);

#endif /* BSP_CLICKBOARD_IO_MUX_CLICKBOARD_IO_MUX_H_ */
