//
// Created by Kaitlyn on 2/24/2025.
//

#include "argus_s2pi.h"

#include "dma.h"
#include "gpio.h"
#include "spi.h"

status_t S2PI_SetBaudRate(uint32_t baudRate_Bps);

/*! A structure that holds the mapping to port and pin for all SPI modules. */
typedef struct {
	/*! The GPIO port */
	GPIO_TypeDef *Port;
	/*! The GPIO pin */
	uint32_t Pin;
}
s2pi_gpio_mapping_t;

/*! A structure to hold all internal data required by the S2PI module. */
typedef struct {
	/*! Determines the current driver status. */
	volatile status_t Status;
	/*! Determines the current S2PI slave. */
	volatile s2pi_slave_t Slave;
	/*! A callback function to be called after transfer/run mode is completed. */
	s2pi_callback_t Callback;
	/*! A parameter to be passed to the callback function. */
	void *CallbackData;
	/*! A callback function to be called after external interrupt is triggered. */
	s2pi_irq_callback_t IrqCallback;
	/*! A parameter to be passed to the interrupt callback function. */
	void *IrqCallbackData;
	/*! The alternate function for this SPI port. */
	const uint32_t SpiAlternate;
	/*! The mapping of the GPIO blocks and pins for this device. */
	const s2pi_gpio_mapping_t GPIOs[S2PI_IRQ + 1];
}
s2pi_handle_t;

s2pi_handle_t s2pi_ = {
	.SpiAlternate = GPIO_AF5_SPI1,
	.GPIOs = {
		[ S2PI_CLK ] = {GPIOA, GPIO_PIN_5},
		[ S2PI_CS ] = {GPIOC, GPIO_PIN_13},
		[ S2PI_MOSI ] = {GPIOC, GPIO_PIN_3},
		[ S2PI_MISO ] = {GPIOC, GPIO_PIN_2},
		[ S2PI_IRQ ] = {GPIOC, GPIO_PIN_15}
	}
};

status_t S2PI_Init(SPI_HandleTypeDef *defaultSlave, uint32_t baudRate_Bps) {
	if (defaultSlave != &hspi2)
		return ERROR_S2PI_INVALID_SLAVE;
	return S2PI_SetBaudRate(baudRate_Bps);
}

/*!***************************************************************************
 * @brief Sets the SPI baud rate in bps.
 * @param baudRate_Bps The default SPI baud rate in bauds-per-second.
 * @return Returns the \link #status_t status\endlink (#STATUS_OK on success).
 * - #STATUS_OK on success
 * - #ERROR_S2PI_INVALID_BAUD_RATE on invalid baud rate value.
 *****************************************************************************/
status_t S2PI_SetBaudRate(uint32_t baudRate_Bps) {
	uint32_t prescaler = 0;
	/* Determine the maximum possible value not greater than baudRate_Bps */
	for (; prescaler < 8; ++prescaler)
		if (SystemCoreClock >> (prescaler + 1) <= baudRate_Bps)
			break;
	MODIFY_REG(hspi2.Instance->CR1, SPI_CR1_BR, prescaler << SPI_CR1_BR_Pos);
	return STATUS_OK;
}

/*!***************************************************************************
 * @brief Returns the status of the SPI module.
 *
 * @return Returns the \link #status_t status\endlink:
 * - #STATUS_IDLE: No SPI transfer or GPIO access is ongoing.
 * - #STATUS_BUSY: An SPI transfer is in progress.
 * - #STATUS_S2PI_GPIO_MODE: The module is in GPIO mode.
 *****************************************************************************/
status_t S2PI_GetStatus(void) {
	return s2pi_.Status;
}

status_t S2PI_TryGetMutex(s2pi_slave_t slave) {
}

void S2PI_ReleaseMutex(s2pi_slave_t slave) {
}

status_t S2PI_TransferFrame(s2pi_slave_t slave,
                            uint8_t const *txData,
                            uint8_t *rxData,
                            size_t frameSize,
                            s2pi_callback_t callback,
                            void *callbackData) {
}

status_t S2PI_Abort(s2pi_slave_t slave) {
}

status_t S2PI_SetIrqCallback(s2pi_slave_t slave,
                             s2pi_irq_callback_t callback,
                             void *callbackData) {
}

uint32_t S2PI_ReadIrqPin(s2pi_slave_t slave) {
}

status_t S2PI_CycleCsPin(s2pi_slave_t slave) {
}

status_t S2PI_CaptureGpioControl(s2pi_slave_t slave) {
}

status_t S2PI_ReleaseGpioControl(s2pi_slave_t slave) {
}

status_t S2PI_WriteGpioPin(s2pi_slave_t slave, s2pi_pin_t pin, uint32_t value) {
}

status_t S2PI_ReadGpioPin(s2pi_slave_t slave, s2pi_pin_t pin, uint32_t *value) {
}
