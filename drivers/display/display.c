/***************************************************************************//**
 *   @file   display.c
 *   @brief  Implementation of display Driver.
 *   @author Andrei Porumb (andrei.porumb@analog.com)
********************************************************************************
 * Copyright 2021(c) Analog Devices, Inc.
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
/***************************** Include Files **********************************/
/******************************************************************************/

#include "display.h"
#include "ssd_1306.h"

/***************************************************************************//**
 * @brief Initializes the display peripheral.
 *
 * @param device     - The device structure.
 * @param init_param - The structure that contains the device initial
 * 		               parameters.
 *
 * @return status - Initialization status.
 *                  Example: -1 - Initialization failed;
 *                            0 - Initialization succeeded.
*******************************************************************************/
int32_t display_init(struct display_dev **device,
		     const struct display_init_param *param)
{
	struct display_dev *dev;
	uint32_t ret;

	dev = (struct display_dev *)malloc(sizeof(*dev));
	if (!dev)
		return FAILURE;
	ret = gpio_get(&dev->reset_pin, param->reset_pin);
	if (ret != SUCCESS)
		return FAILURE;
	ret = gpio_get(&dev->dc_pin, param->dc_pin);
	if (ret != SUCCESS)
		return FAILURE;
	ret = spi_init(&dev->spi_desc, param->spi_ip);
	if (ret != SUCCESS)
		return FAILURE;
	ret = gpio_direction_output(dev->dc_pin, DC_CMD);
	if (ret != SUCCESS)
		return FAILURE;
	ret = gpio_direction_output(dev->reset_pin, RST_OFF);
	if (ret != SUCCESS)
		return FAILURE;
	dev->cols_nb = param->cols_nb;
	dev->rows_nb = param->rows_nb;
	dev->controller_ops = param->controller_ops;

	ret = dev->controller_ops->init(dev);

	*device = dev;

	return ret;
}

/****************************************************************************//**
 * @brief Frees the resources allocated by display_init().
 *
 * @param dev - The device structure.
 *
 * @return SUCCESS in case of success, negative error code otherwise.
*******************************************************************************/
int32_t display_remove(struct display_dev *device)
{
	int32_t ret;

	ret = spi_remove(device->spi_desc);

	ret |= gpio_remove(device->dc_pin);
	ret |= gpio_remove(device->reset_pin);

	free(device);

	return ret;
}

/***************************************************************************//**
 * @brief Turns display on/off.
 *
 * @param device - The device structure.
 *        on_off - DISP_ON
 *                 DISP_OFF
 *
 * @return SUCCESS in case of success, negative error code otherwise.
*******************************************************************************/
int32_t display_on_off(struct display_dev *device, uint8_t on_off)
{

	return device->controller_ops->display_on_off(device, on_off);
}

/***************************************************************************//**
 * @brief Moves cursor to desired position.
 *
 * @param device - The device structure.
 *        row    - row
 *        column - column
 *
 * @return SUCCESS in case of success, negative error code otherwise.
*******************************************************************************/
int32_t display_move_cursor(struct display_dev *device, uint8_t row,
			    uint8_t column)
{

	return device->controller_ops->move_cursor(device, row, column);
}

/***************************************************************************//**
 * @brief Prints character at selected position.
 *
 * @param device - The device structure.
 *        row    - row
 *        column - column
 *        ascii  - corresponding number to ascii table
 *
 * @return SUCCESS in case of success, negative error code otherwise.
*******************************************************************************/
int32_t display_print_ascii(struct display_dev *device, uint8_t ascii,
			    uint8_t row, uint8_t column)
{

	return device->controller_ops->display_print_by_ascii(device, ascii, row,
			column);
}

/***************************************************************************//**
 * @brief Clears data on display.
 *
 * @param device - The device structure.
 *
 * @return SUCCESS in case of success, negative error code otherwise.
*******************************************************************************/
int32_t display_clear(struct display_dev *device)
{
	int32_t ret = SUCCESS;
	uint8_t i, j;

	for(i = 0; i < device->rows_nb; i++)
		for(j = 0; j < device->cols_nb; j++)
			ret |= display_print_ascii(device, ' ', i, j);

	return ret;
}

/***************************************************************************//**
 * @brief Prints char string at selected position.
 *
 * @param device - The device structure.
 *        row    - row
 *        column - column
 *        *msg   - char string pointer
 *
 * @return SUCCESS in case of success, negative error code otherwise.
*******************************************************************************/
int32_t display_print_string(struct display_dev *device, char *msg,
			     uint8_t row, uint8_t column)
{
	int32_t ret = SUCCESS;
	int32_t len;
	int32_t i, j;
	int32_t r = row;
	int32_t c = column;

	len = strlen(msg);
	for(i = 0; i < len; i++) {
		if(r < device->rows_nb) {
			if(c < device->cols_nb) {
				ret |= display_print_ascii(device, msg[i], r, c);
				c++;
			} else {
				c=0U;
				r++;
				ret |= display_print_ascii(device, msg[i], r, c);
				c++;
			}
		}
	}

	return ret;
}
