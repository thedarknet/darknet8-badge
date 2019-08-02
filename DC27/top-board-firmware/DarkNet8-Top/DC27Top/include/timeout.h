/**
 *\file
 *
 *\brief Timeout related functionality declaration
 *
 (c) 2018 Microchip Technology Inc. and its subsidiaries.

    Subject to your compliance with these terms,you may use this software and
    any derivatives exclusively with Microchip products.It is your responsibility
    to comply with third party license terms applicable to your use of third party
    software (including open source software) that may accompany Microchip software.

    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
    WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
    PARTICULAR PURPOSE.

    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
    BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
    FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
    ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
    THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 *
 *
 */

#ifndef TIMEOUTDRIVER_H
#define TIMEOUTDRIVER_H

#include <compiler.h>
#include <stdint.h>
#include <stdbool.h>

/** Datatype used to hold the number of ticks until a timer expires */
typedef uint32_t absolutetime_t;

/** Typedef for the function pointer for the timeout callback function */
typedef absolutetime_t (*timercallback_ptr_t)(void *payload);

/** Data structure completely describing one timer */
typedef struct timer_struct_s {
	timercallback_ptr_t    callback_ptr; ///< Pointer to a callback function that is called when this timer expires
	void *                 payload; ///< Pointer to data that user would like to pass along to the callback function
	struct timer_struct_s *next;    ///< Pointer to a linked list of all timers that have expired and whose callback
	                                ///< functions are due to be called
	absolutetime_t absolute_time;   ///< The number of ticks the timer will count before it expires
} timer_struct_t;

/**
 * \brief Initialize the Timeout driver
 *
 * \return Nothing
 */
void TIMER_0_timeout_init(void);

//********************************************************
// The following functions form the API for scheduler mode.
//********************************************************

/**
 * \brief Schedule the specified timer task to execute at the specified time
 *
 * \param[in] timer Pointer to struct describing the task to execute
 * \param[in] timeout Number of ticks to wait before executing the task
 *
 * \return Nothing
 */
void TIMER_0_timeout_create(timer_struct_t *timer, absolutetime_t timeout);

/**
 * \brief Delete the specified timer task so it won't be executed
 *
 * \param[in] timer Pointer to struct describing the task to execute
 *
 * \return Nothing
 */
void TIMER_0_timeout_delete(timer_struct_t *timer);

/**
 * \brief Delete all scheduled timer tasks
 *
 * \return Nothing
 */
void TIMER_0_timeout_flush_all(void);

/**
 * \brief Execute the next timer task that has been scheduled for execution.
 *
 * If no task has been scheduled for execution, the function
 * returns immediately, so there is no need for any polling.
 *
 * \return Nothing
 */
void TIMER_0_timeout_call_next_callback(void);

//********************************************************
// The following functions form the API for stopwatch mode.
//********************************************************

/**
 * \brief Start a stopwatch using the specified timer struct
 *
 * Start a timer with (maximum range)/2. You cannot time more than
 * this and the timer will stop after this time elapses.
 *
 * \param[in] timer Struct holding the stopwatch
 *
 * \return Nothing
 */
void TIMER_0_timeout_start_timer(timer_struct_t *timer);

/**
 * \brief Stop the specified stopwatch and return the elapsed number of ticks.
 *
 * \param[in] timer Struct holding the stopwatch
 *
 * \return The number of ticks passed since starting the stopwatch
 */
absolutetime_t TIMER_0_timeout_stop_timer(timer_struct_t *timer);

#endif /* TIMEOUTDRIVER_H */

/** @}*/
