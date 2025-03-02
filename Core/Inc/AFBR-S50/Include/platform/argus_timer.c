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

void Timer_GetCounterValue(uint32_t *hct, uint32_t *lct) {
}

status_t Timer_SetCallback(timer_cb_t f) {
}

status_t Timer_SetInterval(uint32_t dt_microseconds, void *param) {
}

void Timer_Start(uint32_t dt_microseconds, void *param) {
}

void Timer_Stop(void *param) {
}
