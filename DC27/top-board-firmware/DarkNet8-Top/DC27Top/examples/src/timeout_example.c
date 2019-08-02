/**
 * \file
 *
 * \brief Timeout driver example.
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
 */

#include <stdio.h>
#include <string.h>
#include <atmel_start.h>
#include <timeout_example.h>
#include <timeout.h>
#include <atomic.h>

static volatile uint8_t  a = 0;
static volatile uint8_t  b = 0;
static volatile uint16_t cycles;

static absolutetime_t foo(void *payload)
{
	uint8_t *p = (uint8_t *)payload;
	*p         = *p + 1;
	if (*p < 11) // Run timer 10 times
	{
		return 10000; // Reschedule the timer after 10000 ticks
	}
	a = 1;
	return 0; // Stop the timer
}

static absolutetime_t bar()
{
	b = 1;    // Mark that timer has executed
	return 0; // Stop the timer
}

uint8_t TIMER_0_test_timeout(void)
{
	// Create payload parameter for foo
	uint8_t timer_param_a = 1;

	// Initialize the two first elements in the struct: callback_ptr & payload,
	// the remaining elements are initialized by TIMER_0_timeout_add()
	timer_struct_t foo_timer = {foo, (void *)&timer_param_a};
	timer_struct_t bar_timer = {bar, NULL};

	ENABLE_INTERRUPTS();

	// Let's test scheduler mode
	// Schedule foo() to be called in 10000 cycles
	TIMER_0_timeout_create(&foo_timer, 10000);
	// Schedule bar() to be called in 200000 cycles
	TIMER_0_timeout_create(&bar_timer, 200000);

	// Wait for both foo and bar to have executed at least once.
	while ((a == 0) && (b == 0)) // Wait for both foo and bar to have executed at least once.
	{
		// Returns immediately if no callback is ready to execute
		TIMER_0_timeout_call_next_callback();
	}

	// bar() hasn't timed out yet, let's delete it from
	// the schedule queue
	TIMER_0_timeout_delete(&bar_timer);

	// Let's test stopwatch mode, measuring the number of
	// cycles needed to execute a small loop
	TIMER_0_timeout_start_timer(&foo_timer);
	for (a = 0; a < 200; a++)
		;
	cycles = TIMER_0_timeout_stop_timer(&foo_timer);

	return 1;
}
