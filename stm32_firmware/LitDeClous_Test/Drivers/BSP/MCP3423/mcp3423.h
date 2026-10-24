/*
 * mcp3423.h
 *
 *  Created on: 12 oct. 2026
 *      Author: Jonathan Marois
 */

#ifndef BSP_MCP3423_MCP3423_H_
#define BSP_MCP3423_MCP3423_H_

#include "stm32f4xx_hal.h"

#define MCP3423_ADDR       (0x68U << 1)

#define MCP3423_CHANNEL_1  0U
#define MCP3423_CHANNEL_2  1U

typedef enum
{
    MCP3423_MODE_ONESHOT = 0,
    MCP3423_MODE_CONTINUOUS = 1

} MCP3423_Mode;

typedef enum
{
    MCP3423_RESOLUTION_12BIT = 0,
    MCP3423_RESOLUTION_14BIT = 1,
    MCP3423_RESOLUTION_16BIT = 2,
    MCP3423_RESOLUTION_18BIT = 3

} MCP3423_Resolution;

typedef struct
{
    I2C_HandleTypeDef *hi2c;
    uint16_t address;
    uint8_t config;

    MCP3423_Mode mode;
    MCP3423_Resolution resolution;

} MCP3423_HandleTypeDef;

/* Temps maximal d'attente d'une conversion, en ms */
uint32_t MCP3423_GetConversionTimeout(
    const MCP3423_HandleTypeDef *dev);

/* Initialisation du MCP3423 */
HAL_StatusTypeDef MCP3423_Init(
    MCP3423_HandleTypeDef *dev,
    I2C_HandleTypeDef *hi2c);

/* Configuration du canal, du mode et de la résolution */
HAL_StatusTypeDef MCP3423_Configure(
    MCP3423_HandleTypeDef *dev,
    uint8_t channel,
    MCP3423_Mode mode,
    MCP3423_Resolution resolution);

/* Déclenchement d'une conversion one-shot */
HAL_StatusTypeDef MCP3423_StartConversion(
    MCP3423_HandleTypeDef *dev);

/* Lecture du résultat brut */
HAL_StatusTypeDef MCP3423_ReadRaw(
    MCP3423_HandleTypeDef *dev,
    int32_t *value);

/* Lecture de la tension en volts */
HAL_StatusTypeDef MCP3423_ReadVoltage(
    MCP3423_HandleTypeDef *dev,
    float *voltage);


#endif /* BSP_MCP3423_MCP3423_H_ */
