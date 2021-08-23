/***************************************************************************//**
 *   @file   ssd_1306.h
 *   @brief  Header file for ssd_1306 Driver.
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

#ifndef SSD_1306_H
#define SSD_1306_H

/******************************************************************************/
/***************************** Include Files **********************************/
/******************************************************************************/

#include <stdint.h>
#include <stdlib.h>
#include "gpio.h"
#include "spi.h"
#include "error.h"

/* @defines for ssd_1306 pins signal level */
#define VDD_ON     0U   //p-channel MOSFET
#define VDD_OFF    1U
#define VBAT_ON    0U   //p-channel MOSFET
#define VBAT_OFF   1U
#define RST_ON     0U
#define RST_OFF    1U
#define DC_DATA    1U
#define DC_CMD     0U

/******************************************************************************/
/*************************** Types Declarations *******************************/
/******************************************************************************/

typedef struct ssd_1306_dev {
	/* SPI */
	spi_desc	             *spi_desc;
	/** DC pin */
	struct gpio_desc	     *dc_pin;
	/** RESET pin */
	struct gpio_desc	     *reset_pin;
	/** VBAT pin */
	struct gpio_desc	     *vbat_pin;
	/** VDD pin */
	struct gpio_desc	     *vdd_pin;
} ssd_1306_dev;

typedef struct ssd_1306_init_param {
	/* SPI */
	spi_init_param	         *spi_ip;
	/** DC pin */
	struct gpio_init_param	 *dc_pin;
	/** RESET pin */
	struct gpio_init_param	 *reset_pin;
	/** VBAT pin */
	struct gpio_init_param   *vbat_pin;
	/** VDD pin */
	struct gpio_init_param   *vdd_pin;
} ssd_1306_init_param;

/******************************************************************************/
/************************ Functions Declarations ******************************/
/******************************************************************************/

/* Initialize the ssd_1306 peripheral. */
int32_t ssd_1306_init(struct ssd_1306_dev **device,
		 const struct ssd_1306_init_param *param);

/* Free the resources allocated by ssd_1306_init(). */
int32_t ssd_1306_remove(struct ssd_1306_dev *device);


/* Initialize the ssd_1306 peripheral for display operation. */
int32_t ssd_1306_display_init(struct ssd_1306_dev *device);
#endif
