/*
 * tca6416a.c
 *
 *  Created on: 10 oct. 2026
 *      Author: Jonathan Marois
 */



#include "tca6416a.h"

#define TCA_TIMEOUT 100U

static HAL_StatusTypeDef ReadRegister(
    TCA6416A_HandleTypeDef *dev,
    uint8_t reg,
    uint8_t *value)
{
    if (dev == NULL || dev->hi2c == NULL || value == NULL)
        return HAL_ERROR;

    return HAL_I2C_Mem_Read(
        dev->hi2c,
        dev->address,
        reg,
        I2C_MEMADD_SIZE_8BIT,
        value,
        1,
        TCA_TIMEOUT);
}

static HAL_StatusTypeDef WriteRegister(
    TCA6416A_HandleTypeDef *dev,
    uint8_t reg,
    uint8_t value)
{
    if (dev == NULL || dev->hi2c == NULL)
        return HAL_ERROR;

    return HAL_I2C_Mem_Write(
        dev->hi2c,
        dev->address,
        reg,
        I2C_MEMADD_SIZE_8BIT,
        &value,
        1,
        TCA_TIMEOUT);
}

HAL_StatusTypeDef TCA6416A_Init(
    TCA6416A_HandleTypeDef *dev,
    I2C_HandleTypeDef *hi2c,
    uint16_t address)
{
    if (dev == NULL || hi2c == NULL)
        return HAL_ERROR;

    dev->hi2c = hi2c;
    dev->address = address;

    return HAL_I2C_IsDeviceReady(
        dev->hi2c,
        dev->address,
        3,
        TCA_TIMEOUT);
}

HAL_StatusTypeDef TCA6416A_PinMode(
    TCA6416A_HandleTypeDef *dev,
    uint8_t pin,
    uint8_t mode)
{
    if (pin >= 16 || mode > 1)
        return HAL_ERROR;

    uint8_t regAddress = TCA6416A_CONFIG_PORT0 + (pin / 8U);
    uint8_t bit = pin % 8U;
    uint8_t reg;

    HAL_StatusTypeDef status =
        ReadRegister(dev, regAddress, &reg);

    if (status != HAL_OK)
        return status;

    if (mode == TCA6416A_INPUT)
        reg |= (uint8_t)(1U << bit);
    else
        reg &= (uint8_t)~(1U << bit);

    return WriteRegister(dev, regAddress, reg);
}

HAL_StatusTypeDef TCA6416A_ReadPin(
    TCA6416A_HandleTypeDef *dev,
    uint8_t pin,
    uint8_t *value)
{
    if (pin >= 16 || value == NULL)
        return HAL_ERROR;

    uint8_t regAddress = TCA6416A_INPUT_PORT0 + (pin / 8U);
    uint8_t bit = pin % 8U;
    uint8_t reg;

    HAL_StatusTypeDef status =
        ReadRegister(dev, regAddress, &reg);

    if (status != HAL_OK)
        return status;

    *value = (reg >> bit) & 1U;

    return HAL_OK;
}

HAL_StatusTypeDef TCA6416A_WritePin(
    TCA6416A_HandleTypeDef *dev,
    uint8_t pin,
    uint8_t value)
{
    if (pin >= 16 || value > 1)
        return HAL_ERROR;

    uint8_t regAddress = TCA6416A_OUTPUT_PORT0 + (pin / 8U);
    uint8_t bit = pin % 8U;
    uint8_t reg;

    HAL_StatusTypeDef status =
        ReadRegister(dev, regAddress, &reg);

    if (status != HAL_OK)
        return status;

    if (value)
        reg |= (uint8_t)(1U << bit);
    else
        reg &= (uint8_t)~(1U << bit);

    return WriteRegister(dev, regAddress, reg);
}

HAL_StatusTypeDef TCA6416A_ReadPort(
    TCA6416A_HandleTypeDef *dev,
    uint8_t port,
    uint8_t *value)
{
    if (port > 1 || value == NULL)
        return HAL_ERROR;

    return ReadRegister(
        dev,
        TCA6416A_INPUT_PORT0 + port,
        value);
}

HAL_StatusTypeDef TCA6416A_WritePort(
    TCA6416A_HandleTypeDef *dev,
    uint8_t port,
    uint8_t value)
{
    if (port > 1)
        return HAL_ERROR;

    return WriteRegister(
        dev,
        TCA6416A_OUTPUT_PORT0 + port,
        value);
}


