/*
 * tca6416a.h
 *
 *  Created on: 10 oct. 2026
 *      Author: Jonathan Marois
 */

#ifndef BSP_TCA6416A_TCA6416A_H_
#define BSP_TCA6416A_TCA6416A_H_

#include "stm32f4xx_hal.h"

#define TCA6416A_ADDR_ADC         (0x20U << 1) // Carte ADC MCP3423

#define TCA6416A_ADDR_PWM         (0x21U << 1)	// Carte PWM

#define TCA6416A_INPUT_PORT0      0x00U
#define TCA6416A_INPUT_PORT1      0x01U

#define TCA6416A_OUTPUT_PORT0     0x02U
#define TCA6416A_OUTPUT_PORT1     0x03U

#define TCA6416A_POLARITY_PORT0   0x04U
#define TCA6416A_POLARITY_PORT1   0x05U

#define TCA6416A_CONFIG_PORT0     0x06U
#define TCA6416A_CONFIG_PORT1     0x07U

#define TCA6416A_INPUT            1U
#define TCA6416A_OUTPUT           0U

typedef struct
{
    I2C_HandleTypeDef *hi2c;
    uint16_t address;

} TCA6416A_HandleTypeDef;

HAL_StatusTypeDef TCA6416A_Init(
    TCA6416A_HandleTypeDef *dev,
    I2C_HandleTypeDef *hi2c,
    uint16_t address
);

HAL_StatusTypeDef TCA6416A_PinMode(
    TCA6416A_HandleTypeDef *dev,
    uint8_t pin,
    uint8_t mode
);

HAL_StatusTypeDef TCA6416A_ReadPin(
    TCA6416A_HandleTypeDef *dev,
    uint8_t pin,
    uint8_t *value
);

HAL_StatusTypeDef TCA6416A_WritePin(
    TCA6416A_HandleTypeDef *dev,
    uint8_t pin,
    uint8_t value
);

HAL_StatusTypeDef TCA6416A_ReadPort(
    TCA6416A_HandleTypeDef *dev,
    uint8_t port,
    uint8_t *value
);

HAL_StatusTypeDef TCA6416A_WritePort(
    TCA6416A_HandleTypeDef *dev,
    uint8_t port,
    uint8_t value
);

#endif /* BSP_TCA6416A_TCA6416A_H_ */
