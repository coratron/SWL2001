/*!
 * \file      smtc_modem_hal_esp.h
 *
 * \brief     ESP32-specific Modem HAL extensions
 *
 * The Clear BSD License
 * Copyright Semtech Corporation 2021. All rights reserved.
 */

#ifndef SMTC_MODEM_HAL_ESP_H
#define SMTC_MODEM_HAL_ESP_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * -----------------------------------------------------------------------------
 * --- DEPENDENCIES ------------------------------------------------------------
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC FUNCTIONS PROTOTYPES ---------------------------------------------
 */

/*!
 * \brief Set the main task handle for immediate timer IRQ notifications
 *
 * \param [in] task_handle FreeRTOS task handle of the main application task
 */
void smtc_modem_hal_set_main_task_handle(TaskHandle_t task_handle);

#ifdef __cplusplus
}
#endif

#endif // SMTC_MODEM_HAL_ESP_H

/* --- EOF ------------------------------------------------------------------ */
