/*
 * command_handler.c
 *
 *  Created on: 16 oct. 2026
 *      Author: Jonathan Marois
 */


#include "command_handler.h"
#include "main.h"
#include "usbd_cdc_if.h"
#include "usb_device.h"
#include "clickboard_io_mux.h"
#include "tca6416a.h"
#include <string.h>
#include <stdlib.h>
#include "mcp3423.h"
#include <stdio.h>

extern USBD_HandleTypeDef hUsbDeviceFS;
extern ClickBoard_HandleTypeDef clickboard_pwm;
extern TIM_HandleTypeDef htim3;
extern TCA6416A_HandleTypeDef tca_adc;
extern MCP3423_HandleTypeDef mcp3423_adc;
extern ClickBoard_HandleTypeDef clickboard_adc;

static uint8_t usb_tx_buffer[64];
static char pending_response[64];
static uint8_t response_pending = 0;

// Le tableau occupera 4 000 octets de RAM.
#define ADC_CAPTURE_MAX_SAMPLES 1000U					// fixe la capacité maximale du tampon.

static int32_t adc_samples[ADC_CAPTURE_MAX_SAMPLES];	// conserve les valeurs brutes retournées par le MCP3423.
static uint16_t adc_sample_count = 0;					// indique combien d'échantillons ont été acquis.

static uint32_t adc_timestamps[ADC_CAPTURE_MAX_SAMPLES]; // Ce tableau conservera le temps de chaque lecture,
															//en millisecondes depuis le début de l'acquisition.


static HAL_StatusTypeDef ADC_Capture(uint32_t duration_ms)
{
    HAL_StatusTypeDef status;
    uint32_t start;
    int32_t raw;

    adc_sample_count = 0;

    status = MCP3423_Configure(
        &mcp3423_adc,
        MCP3423_CHANNEL_1,
        MCP3423_MODE_CONTINUOUS,
        MCP3423_RESOLUTION_12BIT
    );

    if (status != HAL_OK)
        return status;

    start = HAL_GetTick();

    while ((HAL_GetTick() - start) < duration_ms &&
           adc_sample_count < ADC_CAPTURE_MAX_SAMPLES)
    {
        status = MCP3423_ReadRaw(&mcp3423_adc, &raw);

        if (status == HAL_OK)
        {
        	adc_samples[adc_sample_count] = raw;
			adc_timestamps[adc_sample_count] = HAL_GetTick() - start;
			adc_sample_count++;
        }
        else if (status != HAL_BUSY)
        {
            break;
        }

        HAL_Delay(1);
    }

    /* Restaurer le mode one-shot */
    HAL_StatusTypeDef restore_status = MCP3423_Configure(
        &mcp3423_adc,
        MCP3423_CHANNEL_1,
        MCP3423_MODE_ONESHOT,
        MCP3423_RESOLUTION_12BIT
    );

    if (status != HAL_OK && status != HAL_BUSY)
        return status;

    return restore_status;
}



uint8_t CommandHandler_IsResponsePending(void)
{
    return response_pending;
}

static uint8_t SendResponse(const char *response)
{
    uint16_t length = (uint16_t)strlen(response);

    if (length > sizeof(usb_tx_buffer))
    {
        return USBD_FAIL;
    }

    if (hUsbDeviceFS.dev_state != USBD_STATE_CONFIGURED)
    {
        return USBD_FAIL;
    }

    USBD_CDC_HandleTypeDef *hcdc =
        (USBD_CDC_HandleTypeDef *)hUsbDeviceFS.pClassData;

    if (hcdc == NULL || hcdc->TxState != 0)
    {
        return USBD_BUSY;
    }

    memcpy(usb_tx_buffer, response, length);

    return CDC_Transmit_FS(usb_tx_buffer, length);
}


void CommandHandler_Task(void)
{
    if (response_pending == 0)
    {
        return;
    }

    uint8_t status = SendResponse(pending_response);

    if (status == USBD_OK)
    {
        response_pending = 0;
    }
}


static const char *HandlePWM(const uint8_t *command, uint32_t length)
{
    if (length == 12 &&
        memcmp(command, "PWM SELECT ", 11) == 0 &&
        command[11] >= '0' && command[11] <= '7')
    {
        uint8_t channel = command[11] - '0';

        return (ClickBoard_SelectChannel(&clickboard_pwm, channel) == HAL_OK)
               ? "OK\r\n" : "ERROR\r\n";
    }

    if (length >= 10 && length <= 12 &&
        memcmp(command, "PWM DUTY ", 9) == 0)
    {
        uint32_t duty = 0;

        for (uint32_t i = 9; i < length; i++)
        {
            if (command[i] < '0' || command[i] > '9')
                return "ERROR\r\n";

            duty = duty * 10 + (command[i] - '0');
        }

        if (duty > 100)
            return "ERROR\r\n";

        uint32_t period = __HAL_TIM_GET_AUTORELOAD(&htim3) + 1;
        uint32_t pulse = (period * duty) / 100;

        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4, pulse);

        return "OK\r\n";
    }

    if (length >= 10 && length <= 14 &&
        memcmp(command, "PWM FREQ ", 9) == 0)
    {
        uint32_t frequency = 0;

        for (uint32_t i = 9; i < length; i++)
        {
            if (command[i] < '0' || command[i] > '9')
                return "ERROR\r\n";

            frequency = frequency * 10 + (command[i] - '0');
        }

        if (PWM_SetFrequency(frequency) != HAL_OK)
            return "ERROR\r\n";

        return "OK\r\n";
    }

    return "ERROR\r\n";
}



static const char *HandleGPIO(
    const uint8_t *command,
    uint32_t length,
    TCA6416A_HandleTypeDef *tca)
{
    static char response[8];

    if (length == 11 &&
        memcmp(command, "GPIO READ ", 10) == 0 &&
        command[10] >= '0' && command[10] <= '7')
    {
        uint8_t pin = command[10] - '0';
        uint8_t value = 0;

        if (TCA6416A_ReadPin(tca, pin, &value) != HAL_OK)
            return "ERROR\r\n";

        snprintf(response, sizeof(response),
                 "%u\r\n", (unsigned int)value);

        return response;
    }

    if (length == 14 &&
        memcmp(command, "GPIO WRITE ", 11) == 0 &&
        command[11] >= '0' && command[11] <= '7' &&
        command[12] == ' ' &&
        (command[13] == '0' || command[13] == '1'))
    {
        uint8_t pin = command[11] - '0';
        uint8_t value = command[13] - '0';

        return (TCA6416A_WritePin(tca, pin, value) == HAL_OK)
               ? "OK\r\n" : "ERROR\r\n";
    }

    if ((length == 14 || length == 15) &&
        memcmp(command, "GPIO MODE ", 10) == 0 &&
        command[10] >= '0' && command[10] <= '7' &&
        command[11] == ' ')
    {
        uint8_t pin = command[10] - '0';
        uint8_t mode;

        if (length == 14 &&
            memcmp(&command[12], "IN", 2) == 0)
        {
            mode = TCA6416A_INPUT;
        }
        else if (length == 15 &&
                 memcmp(&command[12], "OUT", 3) == 0)
        {
            mode = TCA6416A_OUTPUT;
        }
        else
        {
            return "ERROR\r\n";
        }

        return (TCA6416A_PinMode(tca, pin, mode) == HAL_OK)
               ? "OK\r\n" : "ERROR\r\n";
    }

    return "ERROR\r\n";
}


static const char *HandleADC(const uint8_t *command, uint32_t length)
{
    static char adc_response[64];

    // Sélection du canal du TMUX1308
    if (length == 12 &&
        memcmp(command, "ADC SELECT ", 11) == 0 &&
        command[11] >= '0' && command[11] <= '7')
    {
        uint8_t channel = command[11] - '0';

        if (ClickBoard_SelectChannel(&clickboard_adc, channel) != HAL_OK)
            return "ERROR\r\n";

        HAL_Delay(1);  // Stabilisation du signal analogique

        return "OK\r\n";
    }

    // Configuration de la résolution du MCP3423
    if (length == 17 &&
        memcmp(command, "ADC RESOLUTION ", 15) == 0)
    {
        MCP3423_Resolution resolution;

        if (memcmp(&command[15], "12", 2) == 0)
            resolution = MCP3423_RESOLUTION_12BIT;
        else if (memcmp(&command[15], "14", 2) == 0)
            resolution = MCP3423_RESOLUTION_14BIT;
        else if (memcmp(&command[15], "16", 2) == 0)
            resolution = MCP3423_RESOLUTION_16BIT;
        else if (memcmp(&command[15], "18", 2) == 0)
            resolution = MCP3423_RESOLUTION_18BIT;
        else
            return "ERROR\r\n";

        HAL_StatusTypeDef status = MCP3423_Configure(
            &mcp3423_adc,
            MCP3423_CHANNEL_1,
            MCP3423_MODE_ONESHOT,
            resolution
        );

        return (status == HAL_OK) ? "OK\r\n" : "ERROR\r\n";
    }

    // Conversion one-shot et lecture du MCP3423
    if (length == 8 &&
        memcmp(command, "ADC READ", 8) == 0)
    {
        int32_t raw = 0;

        HAL_StatusTypeDef status =
            MCP3423_StartConversion(&mcp3423_adc);

        if (status == HAL_OK)
        {
            uint32_t start = HAL_GetTick();
            uint32_t timeout =
                MCP3423_GetConversionTimeout(&mcp3423_adc);

            do
            {
                status = MCP3423_ReadRaw(&mcp3423_adc, &raw);

                if (status != HAL_BUSY)
                    break;

                HAL_Delay(1);

            } while ((HAL_GetTick() - start) < timeout);
        }

        if (status == HAL_OK)
        {
            snprintf(adc_response, sizeof(adc_response),
                     "%ld\r\n", (long)raw);

            return adc_response;
        }

        if (status == HAL_BUSY)
            return "TIMEOUT\r\n";

        return "ERROR\r\n";
    }

    if (length == 16 &&
        memcmp(command, "ADC CAPTURE 3000", 16) == 0)
    {
        HAL_StatusTypeDef status = ADC_Capture(3000);

        if (status != HAL_OK)
            return "ERROR\r\n";

        snprintf(adc_response, sizeof(adc_response),
                 "SAMPLES %u\r\n", (unsigned int)adc_sample_count);

        return adc_response;
    }

    if (length >= 12 &&
        memcmp(command, "ADC SAMPLE ", 11) == 0)
    {
        uint32_t index = 0;

        for (uint32_t i = 11; i < length; i++)
        {
            if (command[i] < '0' || command[i] > '9')
                return "ERROR\r\n";

            index = index * 10 + (command[i] - '0');

            if (index >= ADC_CAPTURE_MAX_SAMPLES)
                return "ERROR\r\n";
        }

        if (index >= adc_sample_count)
            return "ERROR\r\n";

        snprintf(adc_response, sizeof(adc_response),
                 "%lu,%ld\r\n",
                 (unsigned long)adc_timestamps[index],
                 (long)adc_samples[index]);

        return adc_response;
    }

    if (length >= 11 &&
        length <= 14 &&
        memcmp(command, "ADC BLOCK ", 10) == 0)
    {
        uint32_t start_index = 0;

        for (uint32_t i = 10; i < length; i++)
        {
            if (command[i] < '0' || command[i] > '9')
                return "ERROR\r\n";

            start_index = start_index * 10 + (command[i] - '0');
        }

        if (start_index >= adc_sample_count)
            return "ERROR\r\n";

        size_t used = 0;
        adc_response[0] = '\0';

        for (uint32_t i = start_index;
             i < adc_sample_count && i < start_index + 4;
             i++)
        {
            int written = snprintf(
                adc_response + used,
                sizeof(adc_response) - used,
                "%lu,%ld;",
                (unsigned long)adc_timestamps[i],
                (long)adc_samples[i]
            );

            if (written < 0 ||
                (size_t)written >= sizeof(adc_response) - used)
                return "ERROR\r\n";

            used += (size_t)written;
        }

        if (used + 2 >= sizeof(adc_response))
            return "ERROR\r\n";

        adc_response[used++] = '\r';
        adc_response[used++] = '\n';
        adc_response[used] = '\0';

        return adc_response;
    }

    return "ERROR\r\n";
}



void CommandHandler_Process(const uint8_t *command, uint32_t length)
{
    const char *response;

    if (length == 4 && memcmp(command, "PING", 4) == 0)
    {
        response = "PONG\r\n";
    }
    else if (length == 6 && memcmp(command, "LED ON", 6) == 0)
    {
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_SET);
        response = "OK\r\n";
    }
    else if (length == 7 && memcmp(command, "LED OFF", 7) == 0)
    {
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_RESET);
        response = "OK\r\n";
    }

    else if (length >= 4 &&
             memcmp(command, "PWM ", 4) == 0)
    {
        response = HandlePWM(command, length);
    }


    else if (length >= 9 &&
             memcmp(command, "GPIO ADC ", 9) == 0)
    {
        // Retirer "ADC " pour réutiliser HandleGPIO()
        uint8_t gpio_command[32];
        uint32_t gpio_length = length - 4;

        if (gpio_length > sizeof(gpio_command))
        {
            response = "ERROR\r\n";
        }
        else
        {
            memcpy(gpio_command, command, 5);
            memcpy(&gpio_command[5], &command[9], length - 9);

            response = HandleGPIO(gpio_command, gpio_length, &tca_adc);
        }
    }
    else if (length >= 5 &&
             memcmp(command, "GPIO ", 5) == 0)
    {
        response = HandleGPIO(command, length, &clickboard_pwm.tca);
    }


    else if (length >= 4 &&
             memcmp(command, "ADC ", 4) == 0)
    {
        response = HandleADC(command, length);
    }


    else
    {
        response = "ERROR\r\n";
    }


    uint8_t status = SendResponse(response);

    if (status == USBD_BUSY)
    {
        if (response_pending == 0)
        {
            strcpy(pending_response, response);
            response_pending = 1;
        }
    }
}
