//
// Created by Kaitlyn on 2/24/2025.
//

#include "argus_s2pi.h"

#include "dma.h"
#include "gpio.h"
#include "spi.h"
#include "argus_irq.h"

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

/*! An additional delay to be added after each GPIO access in order to decrease
 * the baud rate of the software EEPROM protocol. Increase the delay if timing
 * issues occur while reading the EERPOM.
 * e.g. Delay = 10 µsec => Baud Rate < 100 kHz */
#ifndef S2PI_GPIO_DELAY_US
#define S2PI_GPIO_DELAY_US 10
#endif
#if (S2PI_GPIO_DELAY_US == 0)
#define S2PI_GPIO_DELAY() ((void)0)
#else
#include "time.h"
#define S2PI_GPIO_DELAY() Time_DelayUSec(S2PI_GPIO_DELAY_US)
#endif

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

/*!***************************************************************************
 * @brief Transfers a single SPI frame asynchronously.
 * @details Transfers a single SPI frame in asynchronous manner. The Tx data
 * buffer is written to the device via the MOSI line.
 * Optionally the data on the MISO line is written to the provided
 * Rx data buffer. If null, the read data is dismissed.
 * The transfer of a single frame requires to not toggle the chip
 * select line to high in between the data frame.
 * An optional callback is invoked when the asynchronous transfer
 * is finished. Note that the provided buffer must not change while
 * the transfer is ongoing. Use the slave parameter to determine
 * the corresponding slave via the given chip select line.
 *
 * @param slave The specified S2PI slave.
 * @param txData The 8-bit values to write to the SPI bus MOSI line.
 * @param rxData The 8-bit values received from the SPI bus MISO line
 * (pass a null pointer if the data don't need to be read).
 * @param frameSize The number of 8-bit values to be sent/received.
 * @param callback A callback function to be invoked when the transfer is
 * finished. Pass a null pointer if no callback is required.
 * @param callbackData A pointer to a state that will be passed to the
 * callback. Pass a null pointer if not used.
 *
 * @return Returns the \link #status_t status\endlink:
 * - #STATUS_OK: Successfully invoked the transfer.
 * - #ERROR_INVALID_ARGUMENT: An invalid parameter has been passed.
 * - #ERROR_S2PI_INVALID_SLAVE: A wrong slave identifier is provided.
 * - #STATUS_BUSY: An SPI transfer is already in progress. The
 * transfer was not started.
 * - #STATUS_S2PI_GPIO_MODE: The module is in GPIO mode. The transfer
 * was not started.
 *****************************************************************************/
status_t S2PI_TransferFrame(s2pi_slave_t spi_slave,
                            uint8_t const *txData,
                            uint8_t *rxData,
                            size_t frameSize,
                            s2pi_callback_t callback,
                            void *callbackData) {
	/* Verify arguments. */
	if (!txData || frameSize == 0 || frameSize >= 0x10000)
		return ERROR_INVALID_ARGUMENT;
	/* Check the spi slave.*/
	// if (spi_slave != S2PI_S1)
	// 	return ERROR_S2PI_INVALID_SLAVE;
	/* Check the driver status, lock if idle. */
	IRQ_LOCK();
	status_t status = s2pi_.Status;
	if (status != STATUS_IDLE) {
		IRQ_UNLOCK();
		return status;
	}
	s2pi_.Status = STATUS_BUSY;
	IRQ_UNLOCK();
	/* Set the callback information */
	s2pi_.Callback = callback;
	s2pi_.CallbackData = callbackData;
	/* Manually set the chip select (active low) */
	HAL_GPIO_WritePin(s2pi_.GPIOs[S2PI_CS].Port, s2pi_.GPIOs[S2PI_CS].Pin, GPIO_PIN_RESET);
	HAL_StatusTypeDef hal_error;
	/* Lock interrupts to prevent completion interrupt before setup is complete */
	IRQ_LOCK();
	if (rxData)
		hal_error = HAL_SPI_TransmitReceive_DMA(&hspi2, (uint8_t *) txData, rxData, (uint16_t)
		                                        frameSize);
	else
		hal_error = HAL_SPI_Transmit_DMA(&hspi2, (uint8_t *) txData, (uint16_t) frameSize);
	IRQ_UNLOCK();
	if (hal_error != HAL_OK)
		return ERROR_FAIL;
	return STATUS_OK;
}

status_t S2PI_Abort(s2pi_slave_t slave) {
}

status_t S2PI_SetIrqCallback(s2pi_slave_t slave,
                             s2pi_irq_callback_t callback,
                             void *callbackData) {
}

uint32_t S2PI_ReadIrqPin(s2pi_slave_t slave) {
}

/*!***************************************************************************
 * @brief Cycles the chip select line.
 * @details In order to cancel the integration on the ASIC, a fast toggling
 * of the chip select pin of the corresponding SPI slave is required.
 * Therefore, this function toggles the CS from high to low and back.
 * The SPI instance for the specified S2PI slave must be idle,
 * otherwise the status #STATUS_BUSY is returned.
 * @param slave The specified S2PI slave.
 * @return Returns the \link #status_t status\endlink (#STATUS_OK on success).
 *****************************************************************************/
status_t S2PI_CycleCsPin(s2pi_slave_t slave) {
	/* Check the driver status. */
	IRQ_LOCK();
	status_t status = s2pi_.Status;
	if (status != STATUS_IDLE) {
		IRQ_UNLOCK();
		return status;
	}
	s2pi_.Status = STATUS_BUSY;
	IRQ_UNLOCK();
	HAL_GPIO_WritePin(s2pi_.GPIOs[S2PI_CS].Port, s2pi_.GPIOs[S2PI_CS].Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(s2pi_.GPIOs[S2PI_CS].Port, s2pi_.GPIOs[S2PI_CS].Pin, GPIO_PIN_SET);
	s2pi_.Status = STATUS_IDLE;
	return STATUS_OK;
}

/*!***************************************************************************
 * @brief Sets the mode in which the S2PI pins operate.
 * @details This is a helper function to switch the modes between SPI and GPIO.
 * @param mode The gpio mode: GPIO_MODE_AF_PP for SPI,
 * GPIO_MODE_OUTPUT_PP for GPIO.
 *****************************************************************************/
static void S2PI_SetGPIOMode(uint32_t mode) {
	GPIO_InitTypeDef GPIO_InitStruct;
	/* SPI CLK GPIO pin configuration */
	GPIO_InitStruct.Pin = s2pi_.GPIOs[S2PI_CLK].Pin;
	GPIO_InitStruct.Mode = mode;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStruct.Alternate = s2pi_.SpiAlternate;
	HAL_GPIO_Init(s2pi_.GPIOs[S2PI_CLK].Port, &GPIO_InitStruct);
	/* SPI MISO GPIO pin configuration */
	GPIO_InitStruct.Pin = s2pi_.GPIOs[S2PI_MISO].Pin;
	HAL_GPIO_Init(s2pi_.GPIOs[S2PI_MISO].Port, &GPIO_InitStruct);
	/* SPI MOSI GPIO pin configuration */
	GPIO_InitStruct.Pin = s2pi_.GPIOs[S2PI_MOSI].Pin;
	HAL_GPIO_Init(s2pi_.GPIOs[S2PI_MOSI].Port, &GPIO_InitStruct);
}

/*!*****************************************************************************
 * @brief Captures the S2PI pins for GPIO usage.
 * @details The SPI is disabled (module status: #STATUS_S2PI_GPIO_MODE) and the
 * pins are configured for GPIO operation. The GPIO control must be
 * release with the #S2PI_ReleaseGpioControl function in order to
 * switch back to ordinary SPI functionality.
 * @return Returns the \link #status_t status\endlink (#STATUS_OK on success).
 *****************************************************************************/
status_t S2PI_CaptureGpioControl(void) {
	/* Check if something is ongoing. */
	IRQ_LOCK();
	status_t status = s2pi_.Status;
	if (status != STATUS_IDLE) {
		IRQ_UNLOCK();
		return status;
	}
	s2pi_.Status = STATUS_S2PI_GPIO_MODE;
	IRQ_UNLOCK();
	/* Note: Clock must be HI after capturing */
	HAL_GPIO_WritePin(s2pi_.GPIOs[S2PI_CLK].Port, s2pi_.GPIOs[S2PI_CLK].Pin, GPIO_PIN_SET);
	S2PI_SetGPIOMode(GPIO_MODE_OUTPUT_PP);
	return STATUS_OK;
}

/*!*****************************************************************************
 * @brief Releases the S2PI pins from GPIO usage and switches back to SPI mode.
 * @details The GPIO pins are configured for SPI operation and the GPIO mode is
 * left. Must be called if the pins are captured for GPIO operation via
 * the #S2PI_CaptureGpioControl function.
 * @return Returns the \link #status_t status\endlink (#STATUS_OK on success).
 *****************************************************************************/
status_t S2PI_ReleaseGpioControl(void) {
	/* Check if something is ongoing. */
	IRQ_LOCK();
	status_t status = s2pi_.Status;
	if (status != STATUS_S2PI_GPIO_MODE) {
		IRQ_UNLOCK();
		return status;
	}
	s2pi_.Status = STATUS_IDLE;
	IRQ_UNLOCK();
	S2PI_SetGPIOMode(GPIO_MODE_AF_PP);
	return STATUS_OK;
}

/*!*****************************************************************************
 * @brief Writes the output for a specified SPI pin in GPIO mode.
 * @details This function writes the value of an SPI pin if the SPI pins are
 * captured for GPIO operation via the #S2PI_CaptureGpioControl previously.
 * @param slave The specified S2PI slave.
 * @param pin The specified S2PI pin.
 * @param value The GPIO pin state to write (0 = low, 1 = high).
 * @return Returns the \link #status_t status\endlink (#STATUS_OK on success).
 *****************************************************************************/
status_t S2PI_WriteGpioPin(s2pi_slave_t slave, s2pi_pin_t pin, uint32_t value) {
	/* Check if pin is valid. */
	if (pin > S2PI_IRQ || value > 1)
		return ERROR_INVALID_ARGUMENT;
	/* Check if in GPIO mode. */
	if (s2pi_.Status != STATUS_S2PI_GPIO_MODE)
		return ERROR_S2PI_INVALID_STATE;
	HAL_GPIO_WritePin(s2pi_.GPIOs[pin].Port, s2pi_.GPIOs[pin].Pin, value);
	S2PI_GPIO_DELAY();
	return STATUS_OK;
}

/*!*****************************************************************************
 * @brief Reads the input from a specified SPI pin in GPIO mode.
 * @details This function reads the value of an SPI pin if the SPI pins are
 * captured for GPIO operation via the #S2PI_CaptureGpioControl previously.
 * @param slave The specified S2PI slave.
 * @param pin The specified S2PI pin.
 * @param value The GPIO pin state to read (0 = low, 1 = high).
 * @return Returns the \link #status_t status\endlink (#STATUS_OK on success).
 *****************************************************************************/
status_t S2PI_ReadGpioPin(s2pi_slave_t slave, s2pi_pin_t pin, uint32_t *value) {
	/* Check if pin is valid. */
	if (pin > S2PI_IRQ || !value)
		return ERROR_INVALID_ARGUMENT;
	/* Check if in GPIO mode. */
	if (s2pi_.Status != STATUS_S2PI_GPIO_MODE)
		return ERROR_S2PI_INVALID_STATE;
	*value = HAL_GPIO_ReadPin(s2pi_.GPIOs[pin].Port, s2pi_.GPIOs[pin].Pin);
	S2PI_GPIO_DELAY();
	return STATUS_OK;
}
