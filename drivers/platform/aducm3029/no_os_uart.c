/***************************************************************************//**
 *   @file   aducm3029/no_os_uart.c
 *   @brief  Implementation of UART driver for ADuCM302x.
 *   @author Mihail Chindris (mihail.chindris@analog.com)
********************************************************************************
 * Copyright 2019(c) Analog Devices, Inc.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *  - Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  - Neither the name of Analog Devices, Inc. nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *  - The use of this software may or may not infringe the patent rights
 *    of one or more patent holders.  This license does not release you
 *    from the requirement that you obtain separate licenses from these
 *    patent holders to use this software.
 *  - Use of the software either in source or binary form, must be run
 *    on or directly connected to an Analog Devices Inc. component.
 *
 * THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, NON-INFRINGEMENT,
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL ANALOG DEVICES BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, INTELLECTUAL PROPERTY RIGHTS, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/

/******************************************************************************/
/************************* Include Files **************************************/
/******************************************************************************/

#include <drivers/uart/adi_uart.h>
#include <stdlib.h>
#include "no_os_uart.h"
#include "no_os_irq.h"
#include "uart_extra.h"
#include "no_os_util.h"

/******************************************************************************/
/*************************** Types Declarations *******************************/
/******************************************************************************/

/**
 * @struct no_os_baud_desc
 * @brief Structure of an element from \ref baud_rates_26MHz.
 * The baud rate is calculated with the formula:
 * Baudrate = (NO_OS_CLK_FREQ / (div_m + div_n/2048) * pow(2,osr+2) * div_c)).
 */
struct no_os_baud_desc {
	/** Calculated baud rate from the following parameters */
	uint32_t	baud_rate;
	/** From 0 to 2047 */
	uint16_t	div_n;
	/** From 1 to 3 */
	uint8_t		div_m;
	/** From 1 to 65535 */
	uint16_t	div_c;
	/** From 0 to 3 */
	uint8_t		osr;
};

/**
 * @struct no_os_op_desc
 * @brief It stores the state of a operation
 */
struct no_os_op_desc {
	/** Is set when an write nonblocking operation is executing */
	bool		is_nonblocking;
	/** Current buffer*/
	uint8_t		*buff;
	/** Number of bytes pending to process */
	uint32_t	pending;
};

/**
 * @struct no_os_aducm_uart_desc
 * @brief Stores specific parameter needed by the UART driver for internal
 * operations
 */
struct no_os_aducm_uart_desc {
	/** Handle needed by low level functions */
	ADI_UART_HANDLE	uart_handler;
	/** Stores the error occurred */
	enum UART_ERROR	errors;
	/** Set if callback is enabled */
	bool		callback_enabled;
	/**
	 * Buffer needed by the ADI UART driver to operate.
	 * This buffer allocated and aligned at runtime to 32 bits
	 */
	uint8_t		*adi_uart_buffer;
	/**
	 * Stores the offset used to align adi_uart_buffer.
	 * Needed to deallocate \ref adi_uart_buffer
	 */
	uint32_t	adi_uart_buffer_offset;
	/** Status of a write operation */
	struct no_os_op_desc	write_desc;
	/** Status of a read operation */
	struct no_os_op_desc	read_desc;
};

/******************************************************************************/
/********************** Macros and Constants Definitions **********************/
/******************************************************************************/

#define NO_OS_MAX_BYTES	1024u
#define NO_OS_CLK_FREQ	26000000u
#define NO_OS_BAUDS_NB	10u
#if defined (__ADUCM4x50__)
/** In ADuCM302x UART0 and UART1 are available */
# define NO_OS_NUM_UART_DEVICES 2
#else
/** In ADuCM302x UART0 is available */
# define NO_OS_NUM_UART_DEVICES 1
#endif

/** Stores calculated baud rates and parameters */
const struct no_os_baud_desc baud_rates_26MHz[NO_OS_BAUDS_NB] = {
	{9600, 1078, 3, 24, 3},
	{19200, 1078, 3, 12, 3},
	{38400, 1321, 3, 8, 2},
	{57600, 1078, 3, 4, 3},
	{115200, 1563, 1, 4, 3},
	{230400, 1563, 1, 2, 3},
	{460800, 1563, 1, 1, 3},
	{921600, 1563, 1, 1, 2},
	{1000000,1280, 1, 1, 2},
	{1500000, 171, 1, 1, 2}
};

/**
 * Used to check if device already initialized
 */
static uint32_t initialized[NO_OS_NUM_UART_DEVICES];

/******************************************************************************/
/************************ Functions Definitions *******************************/
/******************************************************************************/

/**
 * @brief Allocates the memory needed for the UART descriptor.
 * @return Address to the allocated memory, NULL if the allocation fails
 */
static struct no_os_uart_desc *alloc_desc_mem(void)
{
	struct no_os_uart_desc	*desc;
	struct no_os_aducm_uart_desc	*aducm_desc;
	uint32_t		mem;

	desc = calloc(1, sizeof(*desc));
	if (!desc)
		return NULL;
	aducm_desc = calloc(1, sizeof(*aducm_desc));
	if (!aducm_desc) {
		free(desc);
		return NULL;
	}
	desc->extra = aducm_desc;

	/* Allocate and align buffer to 32 bits */
	aducm_desc->adi_uart_buffer = calloc(1, ADI_UART_BIDIR_MEMORY_SIZE + 3);
	if (!aducm_desc->adi_uart_buffer) {
		free(aducm_desc);
		free(desc);
		return NULL;
	}

	mem = (uint32_t)aducm_desc->adi_uart_buffer;
	aducm_desc->adi_uart_buffer = (uint8_t *)((mem+3u) & (~(3u)));
	aducm_desc->adi_uart_buffer_offset =
		(uint32_t)aducm_desc->adi_uart_buffer - mem;

	return desc;
}

/**
 * @brief Deallocates the memory needed for the UART descriptor.
 * @param desc - Descriptor to be deallocated
 */
static void free_desc_mem(struct no_os_uart_desc *desc)
{
	struct no_os_aducm_uart_desc	*aducm_desc = desc->extra;

	free((void *)((uint32_t)aducm_desc->adi_uart_buffer -
		      aducm_desc->adi_uart_buffer_offset));
	aducm_desc->adi_uart_buffer = NULL;
	aducm_desc->adi_uart_buffer_offset = 0;
	free(desc->extra);
	free(desc);
}

/**
 * @brief Call the user defined callback when a read/write operation completed.
 * @param desc:		Descriptor of the UART device
 * @param event:	Event ID from ADI_UART_EVENT
 * @param buff:		Pointer to the handled buffer or to an error code
 */
static void no_os_uart_callback(void *ctx, uint32_t event, void *buff)
{
	struct no_os_uart_desc	*desc = ctx;
	struct no_os_aducm_uart_desc	*extra = desc->extra;
	uint32_t		len;

	switch(event) {
	/* Read done */
	case ADI_UART_EVENT_RX_BUFFER_PROCESSED:
		if (extra->read_desc.pending) {
			len = no_os_min(extra->read_desc.pending, NO_OS_MAX_BYTES);
			extra->read_desc.pending -= len;
			adi_uart_SubmitRxBuffer(
				(ADI_UART_HANDLE const)extra->uart_handler,
				(void *const)extra->read_desc.buff,
				(uint32_t const)len,
				len > 4 ? true : false);
			extra->read_desc.buff += len;
		} else {
			extra->read_desc.is_nonblocking = false;
			if (desc->callback)
				desc->callback(desc->callback_ctx, NO_OS_IRQ_READ_DONE,
					       NULL);
		}
		break;
	/* Write done */
	case ADI_UART_EVENT_TX_BUFFER_PROCESSED:
		if (extra->write_desc.pending) {
			len = no_os_min(extra->write_desc.pending, NO_OS_MAX_BYTES);
			extra->write_desc.pending -= len;
			adi_uart_SubmitTxBuffer(
				(ADI_UART_HANDLE const)extra->uart_handler,
				(void *const)extra->write_desc.buff,
				(uint32_t const)len,
				len > 4 ? true : false);
			extra->write_desc.buff += len;
		} else {
			extra->write_desc.is_nonblocking = false;
			if (desc->callback)
				desc->callback(desc->callback_ctx, NO_OS_IRQ_WRITE_DONE,
					       NULL);
		}
		break;
	default:
		extra->errors |= (uint32_t)buff;
		extra->read_desc.is_nonblocking = false;
		extra->write_desc.is_nonblocking = false;
		if (desc->callback)
			desc->callback(desc->callback_ctx, NO_OS_IRQ_ERROR, buff);
		break;
	}
}

/**
 * @brief Read data from UART. Blocking function.
 * @param desc:	Descriptor of the UART device
 * @param data:	Buffer where data will be read
 * @param bytes_number:	Number of bytes to be read. Between 1 and 1024
 * @return 0 in case of success, -1 otherwise.
 */
int32_t no_os_uart_read(struct no_os_uart_desc *desc, uint8_t *data,
			uint32_t bytes_number)
{
	struct no_os_aducm_uart_desc	*extra;
	uint32_t		errors;
	uint32_t		to_read;
	uint32_t		idx;

	if (!desc || !data)
		return -1;

	extra = desc->extra;
	if (bytes_number == 0) {
		errors = BAD_INPUT_PARAMETERS;
		goto failure;
	}

	/* Wait until a previously no_os_uart_read_nonblocking ends */
	while (extra->read_desc.is_nonblocking)
		;

	idx = 0;
	while (bytes_number) {
		to_read = no_os_min(bytes_number, NO_OS_MAX_BYTES);
		if (ADI_UART_SUCCESS != adi_uart_Read(
			    (ADI_UART_HANDLE const)extra->uart_handler,
			    (void *const)(data + idx),
			    (uint32_t const)to_read,
			    bytes_number > 4 ? true : false,
			    &errors))
			goto failure;
		bytes_number -= to_read;
		idx += to_read;
	}
	return idx;
failure:
	extra->errors |= errors;
	return -1;
}

/**
 * @brief Write data to UART. Blocking function.
 * @param desc:	Descriptor of the UART device
 * @param data:	Buffer with the data to be written
 * @param bytes_number:	Number of bytes to be written. Between 1 and 1024
 * @return 0 in case of success, -1 otherwise.
 */
int32_t no_os_uart_write(struct no_os_uart_desc *desc, const uint8_t *data,
			 uint32_t bytes_number)
{
	struct no_os_aducm_uart_desc	*extra;
	uint32_t		errors;
	uint32_t		to_write;
	uint32_t		idx;

	if (!desc || !data || !bytes_number)
		return -1;

	/* TODO: Add support for more than 1024 bytes */

	extra = desc->extra;
	/* Wait until a previously no_os_uart_write_nonblocking ends */
	while (extra->write_desc.is_nonblocking)
		;

	idx = 0;
	while (bytes_number) {
		to_write = no_os_min(bytes_number, NO_OS_MAX_BYTES);
		if (ADI_UART_SUCCESS != adi_uart_Write(
			    (ADI_UART_HANDLE const)extra->uart_handler,
			    (void *const)(data + idx),
			    (uint32_t const)to_write,
			    bytes_number > 4 ? true : false,
			    &errors))
			goto failure;
		bytes_number -= to_write;
		idx += to_write;
	}
	return idx;

failure:
	extra->errors |= errors;
	return -1;
}

/**
 * @brief Submit reading buffer to the UART driver.
 *
 * Buffer is used until bytes_number bytes are read.
 * @param desc:	Descriptor of the UART device
 * @param data:	Buffer where data will be read
 * @param bytes_number:	Number of bytes to be read
 * @return 0 in case of success, -1 otherwise.
 */
int32_t no_os_uart_read_nonblocking(struct no_os_uart_desc *desc, uint8_t *data,
				    uint32_t bytes_number)
{
	struct no_os_aducm_uart_desc	*extra;
	uint32_t		to_read;

	if (!desc || !data || !bytes_number)
		return -1;
	extra = desc->extra;

	/* Driver can not submit an other buffer when there is already one */
	if (extra->read_desc.is_nonblocking)
		return -1;
	extra->read_desc.is_nonblocking = true;

	to_read = no_os_min(bytes_number, NO_OS_MAX_BYTES);
	extra->read_desc.pending = bytes_number - to_read;
	extra->read_desc.buff = data + to_read;
	/* The following submits until bytes_number are don in the interrupt. */
	adi_uart_SubmitRxBuffer(
		(ADI_UART_HANDLE const)extra->uart_handler,
		(void *const)data,
		(uint32_t const)to_read,
		to_read > 4 ? true : false);

	return 0;
}

/**
 * @brief Submit writting buffer to the UART driver.
 *
 * Data from the buffer is sent over the UART, the function returns immediately.
 * @param desc:	Descriptor of the UART device
 * @param data:	Buffer where data will be written
 * @param bytes_number:	Number of bytes to be written
 * @return 0 in case of success, -1 otherwise.
 */
int32_t no_os_uart_write_nonblocking(struct no_os_uart_desc *desc,
				     const uint8_t *data,
				     uint32_t bytes_number)
{
	struct no_os_aducm_uart_desc	*extra;
	uint32_t		to_write;

	if (!desc || !data || !bytes_number)
		return -1;
	extra = desc->extra;

	/* Driver can not submit an other buffer when there is already one */
	if (extra->write_desc.is_nonblocking)
		return -1;
	extra->write_desc.is_nonblocking = true;

	to_write = no_os_min(bytes_number, NO_OS_MAX_BYTES);
	extra->write_desc.pending = bytes_number - to_write;
	extra->write_desc.buff = (uint8_t *)data + to_write;
	/* The following submits until bytes_number are don in the interrupt. */
	adi_uart_SubmitTxBuffer(
		(ADI_UART_HANDLE const)extra->uart_handler,
		(void *const)data,
		(uint32_t const)to_write,
		to_write > 4 ? true : false);

	return 0;
}

/**
 * @brief Initialize the UART communication peripheral.
 *
 * To configure the UART, the user must set the extra parameter from param with
 * a pointer to the configured platform specific structure.
 * \ref aducm_uart_init_param .\n
 * @param desc:  Descriptor of the UART device used in the call of the drivers
 * functions
 * @param param: Descriptor used to configure the UART device
 * @return 0 in case of success, -1 otherwise.
 */
int32_t no_os_uart_init(struct no_os_uart_desc **desc,
			struct no_os_uart_init_param *param)
{
	ADI_UART_RESULT			uart_ret;
	struct no_os_aducm_uart_desc		*aducm_desc;
	struct aducm_uart_init_param	*aducm_init_param;

	if (!desc || !param || !(param->extra) ||
	    param->device_id >= NO_OS_NUM_UART_DEVICES || //
	    initialized[param->device_id] != 0) //Already initialized
		return -1;

	initialized[param->device_id] = 1;
	*desc = alloc_desc_mem();
	if (!(*desc))
		return -1;
	aducm_desc = (*desc)->extra;
	aducm_init_param = param->extra;

	(*desc)->baud_rate = param->baud_rate;
	(*desc)->device_id = param->device_id;
	/* aducm_desc->read_desc and aducm_desc->write_desc are 0 already */

	uart_ret = adi_uart_Open(param->device_id, ADI_UART_DIR_BIDIRECTION,
				 aducm_desc->adi_uart_buffer,
				 ADI_UART_BIDIR_MEMORY_SIZE,
				 &aducm_desc->uart_handler);
	if (uart_ret != ADI_UART_SUCCESS)
		goto failure;

	/* Think about the option of implementing default configuration */
	uart_ret = adi_uart_SetConfiguration(aducm_desc->uart_handler,
					     aducm_init_param->parity,
					     aducm_init_param->stop_bits,
					     aducm_init_param->word_length);
	if (uart_ret != ADI_UART_SUCCESS)
		goto failure;

	/* Configure baud rate */
	uint32_t i, freq;
	for (i = 0; i < NO_OS_BAUDS_NB; i++)
		if (baud_rates_26MHz[i].baud_rate == param->baud_rate)
			break;
	adi_pwr_GetClockFrequency(ADI_CLOCK_PCLK, &freq);
	if (i == NO_OS_BAUDS_NB || freq != NO_OS_CLK_FREQ)
		goto failure;
	uart_ret = adi_uart_ConfigBaudRate(aducm_desc->uart_handler,
					   baud_rates_26MHz[i].div_c,
					   baud_rates_26MHz[i].div_m,
					   baud_rates_26MHz[i].div_n,
					   baud_rates_26MHz[i].osr);
	if (uart_ret != ADI_UART_SUCCESS)
		goto failure;

	adi_uart_RegisterCallback(aducm_desc->uart_handler, no_os_uart_callback,
				  *desc);

	return 0;
failure:
	free_desc_mem(*desc);
	*desc = NULL;
	return -1;
}

/**
 * @brief Free the resources allocated by \ref no_os_uart_init().
 * @param desc: Descriptor of the UART device
 * @return 0 in case of success, -1 otherwise.
 */
int32_t no_os_uart_remove(struct no_os_uart_desc *desc)
{
	struct no_os_aducm_uart_desc *aducm_desc;

	if (desc == NULL || desc->extra == NULL)
		return -1;

	initialized[desc->device_id] = 0;

	aducm_desc = desc->extra;
	adi_uart_Close(aducm_desc->uart_handler);
	free_desc_mem(desc);

	return 0;
}

/**
 * @brief Free the resources allocated by \ref no_os_uart_init().
 * @param desc: Descriptor of the UART device
 * @return 0 in case of success, -1 otherwise.
 */
uint32_t no_os_uart_get_errors(struct no_os_uart_desc *desc)
{
	struct no_os_aducm_uart_desc *extra;

	if (!desc)
		return -1;
	extra = desc->extra;
	uint32_t ret = extra->errors;
	extra->errors = 0;

	return ret;
}

