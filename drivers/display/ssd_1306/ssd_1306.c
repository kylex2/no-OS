/***************************************************************************//**
 *   @file   ssd_1306.c
 *   @brief  Implementation of ssd_1306 Driver.
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

#include "ssd_1306.h"

/***************************************************************************//**
 * @brief Initializes the communication peripheral.
 *
 * @param device     - The device structure.
 * @param init_param - The structure that contains the device initial
 * 		               parameters.
 *
 * @return status - Initialization status.
 *                  Example: -1 - Initialization failed;
 *                            0 - Initialization succeeded.
*******************************************************************************/
int32_t ssd_1306_init(struct ssd_1306_dev **device,
		 const struct ssd_1306_init_param *param)
{
	struct ssd_1306_dev *dev;
	uint32_t ret;

	dev = (struct ssd_1306_dev *)malloc(sizeof(*dev));
	if (!dev)
		return -1;

	ret = gpio_get(&dev->reset_pin, param->reset_pin);
	if (ret != SUCCESS)
		return FAILURE;
	ret = gpio_get(&dev->vbat_pin, param->vbat_pin);
	if (ret != SUCCESS)
		return FAILURE;
	ret = gpio_get(&dev->vdd_pin, param->vdd_pin);
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
	ret = gpio_direction_output(dev->vbat_pin, VBAT_ON);
	if (ret != SUCCESS)
		return FAILURE;
	ret = gpio_direction_output(dev->vdd_pin, VDD_ON);
	if (ret != SUCCESS)
		return FAILURE;
	ret = gpio_direction_output(dev->reset_pin, RST_OFF);
	if (ret != SUCCESS)
		return FAILURE;

	*device = dev;

	return ret;
}

/***************************************************************************//**
 * @brief Free the resources allocated by ssd_1306_init().
 *
 * @param dev - The device structure.
 *
 * @return SUCCESS in case of success, negative error code otherwise.
*******************************************************************************/
int32_t ssd_1306_remove(struct ssd_1306_dev *device)
{
	int32_t ret;

	ret = spi_remove(device->spi_desc);

	ret |= gpio_remove(device->dc_pin);
	ret |= gpio_remove(device->reset_pin);
	ret |= gpio_remove(device->vbat_pin);
	ret |= gpio_remove(device->vdd_pin);

	free(device);

	return ret;
}

/***************************************************************************//**
 * @brief Initializes ssd_1306 for display screening.
 *
 * @param dev - The device structure.
 *
 * @return SUCCESS in case of success, negative error code otherwise.
*******************************************************************************/
int32_t ssd_1306_display_init(struct ssd_1306_dev *device)
{
	int32_t ret = SUCCESS;
	uint8_t command[3];
	// initial pin state
	ret |= gpio_set_value(device->reset_pin, RST_OFF);
	ret |= gpio_set_value(device->dc_pin, DC_CMD);
	ret |= gpio_set_value(device->vdd_pin, VDD_ON);
	ret |= gpio_set_value(device->vbat_pin, VBAT_ON);
	usleep(2000U);

	command[0] = 0xAE;
	ret |= spi_write_and_read(device->spi_desc, command, 1U);
	ret |= gpio_set_value(device->reset_pin, RST_ON); //apply reset
	usleep(2000U);
	ret |= gpio_set_value(device->reset_pin, RST_OFF); //normal mode
	usleep(2000U);
	// charge pump
	command[0] = 0x8D;
	command[1] = 0x14;
	ret |= spi_write_and_read(device->spi_desc, command, 2U);
	// pre-charge
	command[0] = 0xD9;
	command[1] = 0xF1;
	ret |= spi_write_and_read(device->spi_desc, command, 2U);
	// set contrast
	command[0] = 0x81;
	command[1] = 0xFF;
	ret |= spi_write_and_read(device->spi_desc, command, 2U);
	// set segment remap
	command[0] = 0xA0;
	ret |= spi_write_and_read(device->spi_desc, command, 1U);
	// set scan direction
	command[0] = 0xC0;
	ret |= spi_write_and_read(device->spi_desc, command, 1U);
	// set COM pin
	command[0] = 0xDA;
	command[1] = 0x00;
	ret |= spi_write_and_read(device->spi_desc, command, 2U);

	command[0] = 0xA4;	// show written memory on screen
	ret |= spi_write_and_read(device->spi_desc, command, 1U);

	command[0] = 0x20;	// memory addressing mode
	command[1] = 0x00;	// horizontal addressing
	ret |= spi_write_and_read(device->spi_desc, command, 2U);

	return ret;
}
