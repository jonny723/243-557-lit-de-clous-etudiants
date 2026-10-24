/*
 * mcp3423.c
 *
 *  Created on: 12 oct. 2026
 *      Author: Jonathan Marois
 */

#include "mcp3423.h"

#define MCP3423_TIMEOUT 100U

HAL_StatusTypeDef MCP3423_Init(
    MCP3423_HandleTypeDef *dev,
    I2C_HandleTypeDef *hi2c)
{
    if (dev == NULL || hi2c == NULL)
        return HAL_ERROR;

    dev->hi2c = hi2c;
    dev->address = MCP3423_ADDR;
    dev->config = 0;
    dev->mode = MCP3423_MODE_CONTINUOUS;
    dev->resolution = MCP3423_RESOLUTION_12BIT;

    return HAL_I2C_IsDeviceReady(
        dev->hi2c,
        dev->address,
        3,
        MCP3423_TIMEOUT);
}

HAL_StatusTypeDef MCP3423_Configure(
    MCP3423_HandleTypeDef *dev,
    uint8_t channel,
    MCP3423_Mode mode,
    MCP3423_Resolution resolution)
{
    if (dev == NULL || dev->hi2c == NULL)
        return HAL_ERROR;

    if (channel > MCP3423_CHANNEL_2)
        return HAL_ERROR;

    if (mode != MCP3423_MODE_ONESHOT &&
        mode != MCP3423_MODE_CONTINUOUS)
        return HAL_ERROR;

    if (resolution > MCP3423_RESOLUTION_18BIT)
        return HAL_ERROR;

    /*
     * Bit 7   : RDY = 0
     * Bits 6-5: sélection CH1 ou CH2
     * Bit 4   : O/C (0 = one-shot, 1 = continu)
     * Bits 3-2: résolution
     * Bits 1-0: PGA x1
     */

    uint8_t config =
        (uint8_t)((channel << 5) |
                  (resolution << 2));

    if (mode == MCP3423_MODE_CONTINUOUS)
        config |= 0x10U;

    HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(
        dev->hi2c,
        dev->address,
        &config,
        1,
        MCP3423_TIMEOUT);

    if (status == HAL_OK)
    {
        dev->config = config;
        dev->mode = mode;
        dev->resolution = resolution;
    }

    return status;
}

HAL_StatusTypeDef MCP3423_StartConversion(
    MCP3423_HandleTypeDef *dev)
{
    if (dev == NULL || dev->hi2c == NULL)
        return HAL_ERROR;

    if (dev->mode != MCP3423_MODE_ONESHOT)
        return HAL_ERROR;

    /* RDY = 1 : démarrage d'une conversion */
    uint8_t config = dev->config | 0x80U;

    return HAL_I2C_Master_Transmit(
        dev->hi2c,
        dev->address,
        &config,
        1,
        MCP3423_TIMEOUT);
}

uint32_t MCP3423_GetConversionTimeout(
    const MCP3423_HandleTypeDef *dev)
{
    if (dev == NULL)
        return 0U;

    switch (dev->resolution)
    {
        case MCP3423_RESOLUTION_12BIT:
            return 10U;

        case MCP3423_RESOLUTION_14BIT:
            return 30U;

        case MCP3423_RESOLUTION_16BIT:
            return 100U;

        case MCP3423_RESOLUTION_18BIT:
            return 350U;

        default:
            return 0U;
    }
}

HAL_StatusTypeDef MCP3423_ReadRaw(
    MCP3423_HandleTypeDef *dev,
    int32_t *value)
{
    if (dev == NULL || dev->hi2c == NULL || value == NULL)
        return HAL_ERROR;

    if (dev->resolution > MCP3423_RESOLUTION_18BIT)
        return HAL_ERROR;

    uint8_t data[4];

    /* 18 bits : 3 octets de données + configuration */
    /* Autres résolutions : 2 octets + configuration */
    uint16_t length =
        (dev->resolution == MCP3423_RESOLUTION_18BIT)
        ? 4U : 3U;

    HAL_StatusTypeDef status = HAL_I2C_Master_Receive(
        dev->hi2c,
        dev->address,
        data,
        length,
        MCP3423_TIMEOUT);

    if (status != HAL_OK)
        return status;

    /* Le dernier octet contient le bit RDY */
    if ((data[length - 1U] & 0x80U) != 0U)
        return HAL_BUSY;

    uint32_t raw;
    uint8_t bits;

    if (dev->resolution == MCP3423_RESOLUTION_18BIT)
    {
        raw = ((uint32_t)data[0] << 16) |
              ((uint32_t)data[1] << 8) |
              data[2];

        bits = 18U;
    }
    else
    {
        raw = ((uint32_t)data[0] << 8) |
              data[1];

        bits = 12U + 2U * (uint8_t)dev->resolution;
    }

    /* Masquage des bits non significatifs */
    uint32_t mask = (1UL << bits) - 1UL;
    raw &= mask;

    /* Extension du signe */
    if ((raw & (1UL << (bits - 1U))) != 0U)
        raw |= ~mask;

    *value = (int32_t)raw;

    return HAL_OK;
}

HAL_StatusTypeDef MCP3423_ReadVoltage(
    MCP3423_HandleTypeDef *dev,
    float *voltage)
{
    if (dev == NULL || voltage == NULL)
        return HAL_ERROR;

    int32_t raw;

    HAL_StatusTypeDef status =
        MCP3423_ReadRaw(dev, &raw);

    if (status != HAL_OK)
        return status;

    float lsb;

    switch (dev->resolution)
    {
        case MCP3423_RESOLUTION_12BIT:
            lsb = 0.001f;
            break;

        case MCP3423_RESOLUTION_14BIT:
            lsb = 0.00025f;
            break;

        case MCP3423_RESOLUTION_16BIT:
            lsb = 0.0000625f;
            break;

        case MCP3423_RESOLUTION_18BIT:
            lsb = 0.000015625f;
            break;

        default:
            return HAL_ERROR;
    }

    /* PGA x1 */
    *voltage = raw * lsb;

    return HAL_OK;
}
