#include "argus_timer.h"
#include "tim.h"

#include <stdint.h>

/*!***************************************************************************
 * @brief Initializes the timer hardware.
 * @return -
 *****************************************************************************/
void Timer_Init(void) {
	/* Initialize the timers, see generated main.c */
	MX_TIM2_Init();
	MX_TIM4_Init();
	MX_TIM5_Init();
	/* Start the timers relevant for the LTC */
	HAL_TIM_Base_Start(&htim2);
	HAL_TIM_Base_Start(&htim5);
}

/*!***************************************************************************
 * @brief Obtains the lifetime counter value from the timers.
 *
 * @details The function is required to get the current time relative to any
 * point in time, e.g. the startup time. The returned values \p hct and
 * \p lct are given in seconds and microseconds respectively. The current
 * elapsed time since the reference time is then calculated from:
 *
 * t_now [µsec] = hct * 1000000 µsec + lct * 1 µsec
 *
 * @param hct A pointer to the high counter value bits representing current
 * time in seconds.
 * @param lct A pointer to the low counter value bits representing current
 * time in microseconds. Range: 0, .., 999999 µsec
 * @return -
 *****************************************************************************/
void Timer_GetCounterValue(uint32_t *hct, uint32_t *lct) {
	/* The loop makes sure that there are no glitches
	when the counter wraps between htim2 and htm2 reads. */
	do {
		*lct = __HAL_TIM_GET_COUNTER(&htim2);
		*hct = __HAL_TIM_GET_COUNTER(&htim5);
	} while (*lct > __HAL_TIM_GET_COUNTER(&htim2));
}

status_t Timer_SetCallback(timer_cb_t f) {
}

status_t Timer_SetInterval(uint32_t dt_microseconds, void *param) {
}

void Timer_Start(uint32_t dt_microseconds, void *param) {
}

void Timer_Stop(void *param) {
}
