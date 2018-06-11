/* Copyright (c) 2009 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 */

/** @file
 * @defgroup rtc_example_main main.c
 * @{
 * @ingroup rtc_example
 * @brief Real Time Counter Example Application main file.
 *
 * This file contains the source code for a sample application using the Real Time Counter (RTC).
 *
 * @image html example_board_setup_a.jpg "Use board setup A for this example."
 */

#include <stdbool.h>
#include "nrf.h"
#include "nrf_gpio.h"
#include "main.h"

extern UINT64      g_ullTickCount;

extern void osTickHandler (void);                                         /**< osTickHandler not declared in any header file, extern here. */

#define GPIO_TOGGLE_TICK_EVENT    (LED_0)                                 /**< Pin number to toggle when there is a tick event in RTC. */
#define GPIO_TOGGLE_COMPARE_EVENT (LED_1)                                 /**< Pin number to toggle when there is compare event in RTC. */
#define LFCLK_FREQUENCY           (32768UL)                               /**< LFCLK frequency in Hertz, constant. */
#define RTC_FREQUENCY             (8UL)                                   /**< Required RTC working clock RTC_FREQUENCY Hertz. Changable. */
#define COMPARE_COUNTERTIME       (3UL)                                   /**< Get Compare event COMPARE_TIME seconds after the counter starts from 0. */
#define COUNTER_PRESCALER         ((LFCLK_FREQUENCY/RTC_FREQUENCY) - 1)   /* f = LFCLK/(prescaler + 1) */

/** @brief Function starting the internal LFCLK XTAL oscillator.
 */
static void lfclk_config(void)
{
    NRF_CLOCK->LFCLKSRC             = (CLOCK_LFCLKSRC_SRC_Xtal << CLOCK_LFCLKSRC_SRC_Pos);
    NRF_CLOCK->EVENTS_LFCLKSTARTED  = 0;
    NRF_CLOCK->TASKS_LFCLKSTART     = 1;
    while (NRF_CLOCK->EVENTS_LFCLKSTARTED == 0)
    {
        //Do nothing.
    }
    NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;
}

/** @brief Function for configuring the RTC with TICK frequency.
 */
static void rtc_start(void)
{
    unsigned int prescaler = (LFCLK_FREQUENCY / LOSCFG_BASE_CORE_TICK_PER_SECOND) - 1;

    if ((LFCLK_FREQUENCY % LOSCFG_BASE_CORE_TICK_PER_SECOND)
        > (LOSCFG_BASE_CORE_TICK_PER_SECOND >> 1))
    {
        prescaler++;
    }

    NVIC_EnableIRQ(RTC0_IRQn);                                      // Enable Interrupt for the RTC in the core.
    NVIC_SetPriority (RTC0_IRQn, (1UL << __NVIC_PRIO_BITS) - 1UL);
    NRF_RTC0->PRESCALER     = prescaler;                            // Set prescaler to a TICK of RTC_FREQUENCY.

    // Enable TICK event and TICK interrupt:
    NRF_RTC0->EVTENSET      = RTC_EVTENSET_TICK_Msk;
    NRF_RTC0->INTENSET      = RTC_INTENSET_TICK_Msk;

    NRF_RTC0->TASKS_START = 1;
}

/** @brief: Function for handling the RTC0 interrupts.
 * Triggered on TICK updated.
 */
void RTC0_IRQHandler()
{
    if ((NRF_RTC0->EVENTS_TICK != 0) && 
        ((NRF_RTC0->INTENSET & RTC_INTENSET_TICK_Msk) != 0))
    {
        NRF_RTC0->EVENTS_TICK = 0;

        g_ullTickCount++;

#if(LOSCFG_BASE_CORE_TIMESLICE == YES)
        osTimesliceCheck();
#endif

        osTaskScan();   //task timeout scan

#if (LOSCFG_BASE_CORE_SWTMR == YES)
        (VOID)osSwtmrScan();
#endif
    }
}

/**
 * @brief Function for application main entry.
 */
UINT32 osTickStart(void)
{
    g_ullTickCount = 0;

    lfclk_config();
    rtc_start();

    return LOS_OK;
}

/**  @} */
