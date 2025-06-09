/*!
 * \file      smtc_hal_gpio.c
 *
 * \brief     GPIO Hardware Abstraction Layer implementation for ESP32
 *
 * The Clear BSD License
 * Copyright Semtech Corporation 2021. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the disclaimer
 * below) provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Semtech corporation nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY
 * THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT
 * NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SEMTECH CORPORATION BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * -----------------------------------------------------------------------------
 * --- DEPENDENCIES ------------------------------------------------------------
 */

#include "smtc_hal_gpio.h"
#include "driver/gpio.h"
#include "esp_log.h"

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE MACROS-----------------------------------------------------------
 */

#define MAX_GPIO_IRQ_HANDLERS 40

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE CONSTANTS -------------------------------------------------------
 */

static const char *TAG = "smtc_hal_gpio";

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE TYPES -----------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE VARIABLES -------------------------------------------------------
 */

static hal_gpio_irq_t *gpio_irq_handlers[MAX_GPIO_IRQ_HANDLERS] = {NULL};

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DECLARATION -------------------------------------------
 */

static void IRAM_ATTR gpio_isr_handler(void *arg);

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC FUNCTIONS DEFINITION ---------------------------------------------
 */

void hal_gpio_init_out(const hal_gpio_pin_names_t pin, const uint32_t value)
{
    if (pin == NC)
    {
        return;
    }

    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << pin),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE};
    gpio_config(&io_conf);
    gpio_set_level(pin, value);
}

void hal_gpio_init_in(const hal_gpio_pin_names_t pin, const hal_gpio_pull_mode_t pull_mode,
                      const hal_gpio_irq_mode_t irq_mode, hal_gpio_irq_t *irq)
{
    if (pin == NC)
    {
        return;
    }

    gpio_config_t io_conf = {
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << pin),
        .intr_type = GPIO_INTR_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE};

    // Configure pull mode
    switch (pull_mode)
    {
    case BSP_GPIO_PULL_MODE_UP:
        io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
        break;
    case BSP_GPIO_PULL_MODE_DOWN:
        io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
        break;
    case BSP_GPIO_PULL_MODE_NONE:
    default:
        break;
    }

    // Configure interrupt mode
    switch (irq_mode)
    {
    case BSP_GPIO_IRQ_MODE_RISING:
        io_conf.intr_type = GPIO_INTR_POSEDGE;
        break;
    case BSP_GPIO_IRQ_MODE_FALLING:
        io_conf.intr_type = GPIO_INTR_NEGEDGE;
        break;
    case BSP_GPIO_IRQ_MODE_RISING_FALLING:
        io_conf.intr_type = GPIO_INTR_ANYEDGE;
        break;
    case BSP_GPIO_IRQ_MODE_OFF:
    default:
        io_conf.intr_type = GPIO_INTR_DISABLE;
        break;
    }

    gpio_config(&io_conf);

    if (irq != NULL && irq_mode != BSP_GPIO_IRQ_MODE_OFF)
    {
        irq->pin = pin;
        gpio_irq_handlers[pin] = irq;
        gpio_install_isr_service(0);
        gpio_isr_handler_add(pin, gpio_isr_handler, (void *)irq);
    }
}

void hal_gpio_irq_attach(const hal_gpio_irq_t *irq)
{
    if (irq == NULL || irq->pin == NC)
    {
        return;
    }

    gpio_irq_handlers[irq->pin] = (hal_gpio_irq_t *)irq;
    gpio_install_isr_service(0);
    gpio_isr_handler_add(irq->pin, gpio_isr_handler, (void *)irq);
}

void hal_gpio_irq_deatach(const hal_gpio_irq_t *irq)
{
    if (irq == NULL || irq->pin == NC)
    {
        return;
    }

    gpio_isr_handler_remove(irq->pin);
    gpio_irq_handlers[irq->pin] = NULL;
}

void hal_gpio_irq_enable(void)
{
    // GPIO interrupts are enabled by default in ESP32 when configured
    // This function is mainly for compatibility
}

void hal_gpio_irq_disable(void)
{
    // Disable all GPIO interrupts
    for (int i = 0; i < MAX_GPIO_IRQ_HANDLERS; i++)
    {
        if (gpio_irq_handlers[i] != NULL)
        {
            gpio_intr_disable(i);
        }
    }
}

void hal_gpio_set_value(const hal_gpio_pin_names_t pin, const uint32_t value)
{
    if (pin == NC)
    {
        return;
    }

    gpio_set_level(pin, value);
}

uint32_t hal_gpio_get_value(const hal_gpio_pin_names_t pin)
{
    if (pin == NC)
    {
        return 0;
    }

    return gpio_get_level(pin);
}

void hal_gpio_clear_pending_irq(const hal_gpio_pin_names_t pin)
{
    if (pin == NC)
    {
        return;
    }

    // ESP32 GPIO interrupts are automatically cleared when the ISR is called
    // This function is mainly for compatibility
}

void hal_gpio_enable_clock(const hal_gpio_pin_names_t pin)
{
    if (pin == NC)
    {
        return;
    }

    // ESP32 GPIO clocks are enabled by default
    // This function is mainly for compatibility
}

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DEFINITION --------------------------------------------
 */

static void gpio_isr_handler(void *arg)
{
    hal_gpio_irq_t *irq = (hal_gpio_irq_t *)arg;

    if (irq != NULL && irq->callback != NULL)
    {
        irq->callback(irq->context);
    }
}

/* --- EOF ------------------------------------------------------------------ */
