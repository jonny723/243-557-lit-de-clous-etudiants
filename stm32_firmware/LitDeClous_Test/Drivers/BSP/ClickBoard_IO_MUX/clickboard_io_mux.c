/*
 * clickboard_io_mux.c
 *
 *  Created on: 12 oct. 2026
 *      Author: Jonathan Marois
 *
 */


#include "clickboard_io_mux.h"

/* Broches du TCA6416A reliées au TMUX1308 */
#define MUX_A0_PIN       8
#define MUX_A1_PIN       9
#define MUX_A2_PIN       10
#define MUX_EN_PIN       11

/* EN_B : 1 = désactivé, 0 = activé */
#define MUX_DISABLE      0x08

HAL_StatusTypeDef ClickBoard_InitPWM(
    ClickBoard_HandleTypeDef *dev,
    I2C_HandleTypeDef *hi2c)
{
    if (dev == NULL || hi2c == NULL)
        return HAL_ERROR;

    // Initialiser le TCA6416A de la carte PWM
    if (TCA6416A_Init(
            &dev->tca,
            hi2c,
            TCA6416A_ADDR_PWM) != HAL_OK)
        return HAL_ERROR;

    // Désactiver le TMUX1308 avant de configurer ses broches
    if (TCA6416A_WritePin(&dev->tca, 11, 1) != HAL_OK)
        return HAL_ERROR;

    // Configurer A0, A1, A2 et EN_B en sorties
    for (uint8_t pin = 8; pin <= 11; pin++)
    {
        if (TCA6416A_PinMode(
                &dev->tca,
                pin,
                TCA6416A_OUTPUT) != HAL_OK)
            return HAL_ERROR;
    }

    // Laisser le multiplexeur désactivé
    if (ClickBoard_DisableMux(dev) != HAL_OK)
        return HAL_ERROR;

    dev->selected_channel = 0;

    return HAL_OK;
}

HAL_StatusTypeDef ClickBoard_Init(
    ClickBoard_HandleTypeDef *dev,
    I2C_HandleTypeDef *hi2c,
    MCP3423_Resolution resolution)
{
    if (dev == NULL || hi2c == NULL)
        return HAL_ERROR;

    /* Initialisation du TCA6416A */
    if (TCA6416A_Init(
            &dev->tca,
            hi2c,
            TCA6416A_ADDR_ADC) != HAL_OK)
        return HAL_ERROR;

    /* Désactiver le multiplexeur avant de
       configurer les broches en sortie */
    if (TCA6416A_WritePin(&dev->tca, MUX_EN_PIN, 1) != HAL_OK)
        return HAL_ERROR;

    for (uint8_t pin = MUX_A0_PIN; pin <= MUX_EN_PIN; pin++)
    {
        if (TCA6416A_PinMode(&dev->tca, pin,
                            TCA6416A_OUTPUT) != HAL_OK)
            return HAL_ERROR;
    }

    /* Multiplexeur désactivé */
    if (ClickBoard_DisableMux(dev) != HAL_OK)
        return HAL_ERROR;

    /* Initialisation du MCP3423 */
    if (MCP3423_Init(&dev->mcp, hi2c) != HAL_OK)
        return HAL_ERROR;

    /* Canal CH1, mode one-shot, résolution configurable */
    if (MCP3423_Configure(
            &dev->mcp,
            MCP3423_CHANNEL_1,
            MCP3423_MODE_ONESHOT,
            resolution) != HAL_OK)
        return HAL_ERROR;

    dev->selected_channel = 0;

    return HAL_OK;
}

HAL_StatusTypeDef ClickBoard_SelectChannel(
    ClickBoard_HandleTypeDef *dev,
    uint8_t channel)
{
    if (dev == NULL || channel > 7)
        return HAL_ERROR;

    /* P1.0 à P1.2 = adresse du canal
       P1.3 = EN_B = 0 : multiplexeur activé */
    if (TCA6416A_WritePort(&dev->tca, 1, channel) != HAL_OK)
        return HAL_ERROR;

    dev->selected_channel = channel;

    return HAL_OK;
}

HAL_StatusTypeDef ClickBoard_DisableMux(
    ClickBoard_HandleTypeDef *dev)
{
    if (dev == NULL)
        return HAL_ERROR;

    /* EN_B = 1 : désactivation du TMUX1308 */
    return TCA6416A_WritePort(&dev->tca, 1, MUX_DISABLE);
}

HAL_StatusTypeDef ClickBoard_ReadVoltage(
    ClickBoard_HandleTypeDef *dev,
    float *voltage)
{
    if (dev == NULL || voltage == NULL)
        return HAL_ERROR;

    return MCP3423_ReadVoltage(&dev->mcp, voltage);
}


HAL_StatusTypeDef ClickBoard_ReadChannel(
    ClickBoard_HandleTypeDef *dev,
    uint8_t channel,
    float *voltage)
{
    if (dev == NULL || voltage == NULL || channel > 7)
        return HAL_ERROR;

    /* Sélection du canal S0 à S7 */
    HAL_StatusTypeDef status =
        ClickBoard_SelectChannel(dev, channel);

    if (status != HAL_OK)
        return status;

    /* Stabilisation du signal analogique lié au changement de canal du TMUX1308 */
    HAL_Delay(20);

    /* Déclenchement d'une nouvelle conversion one-shot */
    status = MCP3423_StartConversion(&dev->mcp);

    if (status != HAL_OK)
        return status;

    /* Délai maximal selon la résolution du MCP3423 */
    uint32_t timeout =
        MCP3423_GetConversionTimeout(&dev->mcp);

    if (timeout == 0U)
        return HAL_ERROR;

    /* Attente de la fin de conversion */
    uint32_t start = HAL_GetTick();

    do
    {
        status = MCP3423_ReadVoltage(&dev->mcp, voltage);

        if (status == HAL_OK)
            return HAL_OK;

        if (status != HAL_BUSY)
            return status;

        HAL_Delay(1);

    } while ((HAL_GetTick() - start) < timeout);

    return HAL_TIMEOUT;
}

// Configuration d'une broche numérique
HAL_StatusTypeDef ClickBoard_PinMode(
    ClickBoard_HandleTypeDef *dev,
    uint8_t pin,
    uint8_t mode)
{
    if (dev == NULL || pin > 7)
        return HAL_ERROR;

    return TCA6416A_PinMode(&dev->tca, pin, mode);
}

// Écriture sur une sortie numérique
HAL_StatusTypeDef ClickBoard_WritePin(
    ClickBoard_HandleTypeDef *dev,
    uint8_t pin,
    uint8_t value)
{
    if (dev == NULL || pin > 7 || value > 1)
        return HAL_ERROR;

    return TCA6416A_WritePin(&dev->tca, pin, value);
}


// Lecture d'une entrée numérique
HAL_StatusTypeDef ClickBoard_ReadPin(
    ClickBoard_HandleTypeDef *dev,
    uint8_t pin,
    uint8_t *value)
{
    if (dev == NULL || pin > 7 || value == NULL)
        return HAL_ERROR;

    return TCA6416A_ReadPin(&dev->tca, pin, value);
}





