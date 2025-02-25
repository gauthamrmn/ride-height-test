//
// Created by Gautham Ramanarayanan on 2/24/25.
//

#include "argus_s2pi.h"

#include <stdint.h>

status_t S2PI_GetStatus(s2pi_slave_t slave) {}

status_t S2PI_TryGetMutex(s2pi_slave_t slave) {}

void S2PI_ReleaseMutex(s2pi_slave_t slave) {}

status_t S2PI_TransferFrame(s2pi_slave_t slave,
                            uint8_t const * txData,
                            uint8_t * rxData,
                            size_t frameSize,
                            s2pi_callback_t callback,
                            void * callbackData) {}

status_t S2PI_Abort(s2pi_slave_t slave) {}

status_t S2PI_SetIrqCallback(s2pi_slave_t slave,
                             s2pi_irq_callback_t callback,
                             void * callbackData) {}

uint32_t S2PI_ReadIrqPin(s2pi_slave_t slave) {}

status_t S2PI_CycleCsPin(s2pi_slave_t slave) {}

status_t S2PI_CaptureGpioControl(s2pi_slave_t slave) {}

status_t S2PI_ReleaseGpioControl(s2pi_slave_t slave) {}

status_t S2PI_WriteGpioPin(s2pi_slave_t slave, s2pi_pin_t pin, uint32_t value) {}

status_t S2PI_ReadGpioPin(s2pi_slave_t slave, s2pi_pin_t pin, uint32_t * value) {}