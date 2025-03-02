#include "argus_timer.h"
#include "tim.h"
#include "assert.h"
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


/*! Storage for the callback parameter */
static void *callback_param_;
/*! Timer interval in microseconds */
static uint32_t period_us_;
/*!***************************************************************************
 * @brief Starts the timer for a specified callback parameter.
 * @details Sets the callback interval for the specified parameter and starts
 * the timer with a new interval. If there is already an interval with
 * the given parameter, the timer is restarted with the given interval.
 * Passing an interval of 0 disables the timer.
 * @param dt_microseconds The callback interval in microseconds.
 * @param param An abstract parameter to be passed to the callback. This is
 * also the identifier of the given interval.
 * @return Returns the \link #status_t status\endlink (#STATUS_OK on success).
 *****************************************************************************/
status_t Timer_Start(uint32_t period, void *param) {
	callback_param_ = param;
	if (period == period_us_)
		return STATUS_OK;
	period_us_ = period;
	uint32_t prescaler = SystemCoreClock / 1000000U;
	while (period > 0xFFFF) {
		period >>= 1U;
		prescaler <<= 1U;
	}
	assert(prescaler <= 0x10000U);
	/* Set prescaler and period values */
	__HAL_TIM_SET_PRESCALER(&htim4, prescaler - 1);
	__HAL_TIM_SET_AUTORELOAD(&htim4, period - 1);
	/* Enable interrupt and timer */
	__HAL_TIM_ENABLE_IT(&htim4, TIM_IT_UPDATE);
	__HAL_TIM_ENABLE(&htim4);
	return STATUS_OK;
}

/*!***************************************************************************
 * @brief Stops the timer for a specified callback parameter.
 * @details Stops a callback interval for the specified parameter.
 * @param param An abstract parameter that identifies the interval to be stopped.
 * @return Returns the \link #status_t status\endlink (#STATUS_OK on success).
 *****************************************************************************/
status_t Timer_Stop(void *param) {
	period_us_ = 0;
	callback_param_ = 0;
	/* Disable interrupt and timer */
	__HAL_TIM_DISABLE_IT(&htim4, TIM_IT_UPDATE);
	__HAL_TIM_DISABLE(&htim4);;
	return STATUS_OK;
}

/*!***************************************************************************
 * @brief Sets the timer interval for a specified callback parameter.
 * @details Sets the callback interval for the specified parameter and starts
 * the timer with a new interval. If there is already an interval with
 * the given parameter, the timer is restarted with the given interval.
 * If the same time interval as already set is passed, nothing happens.
 * Passing a interval of 0 disables the timer.
 * @param dt_microseconds The callback interval in microseconds.
 * @param param An abstract parameter to be passed to the callback. This is
 * also the identifier of the given interval.
 * @return Returns the \link #status_t status\endlink (#STATUS_OK on success).
 *****************************************************************************/
status_t Timer_SetInterval(uint32_t dt_microseconds, void *param) {
	return dt_microseconds ? Timer_Start(dt_microseconds, param) : Timer_Stop(param);
}
