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
#include "display.h"

const struct display_controller_ops ssd1306_ops = {
	.init = &ssd_1306_init,
	.display_on_off = &ssd_1306_display_on_off,
	.move_cursor = &ssd_1306_move_cursor,
	.display_print_by_ascii = &ssd_1306_print_ascii
};

/* ASCII to bitmap */
uint64_t chr_map[128] = {0x0000000000000000,
			 0x0000000000000000,
			 0x0000000000000000,
			 0x0000000000000000,
			 0x0000000000000000,
			 0x0000000000000000,
			 0x0000000000000000,
			 0x0000000000000000,
			 0x0000000000000000,
			 0x0000000000000000,
			 0x0000000000000000,
			 0x0000000000000000,
			 0x0000000000000000,
			 0x0000000000000000,
			 0x0000000000000000,
			 0x0000000000000000,
			 0x0000000000000000,
			 0x0000000000000000,
			 0x0000000000000000,
			 0x0000000000000000,
			 0x0000000000000000,
			 0x0000000000000000,
			 0x0000000000000000,
			 0x0000000000000000,
			 0x0000000000000000,
			 0x0000000000000000,
			 0x0000000000000000,
			 0x0000000000000000,
			 0x0000000000000000,
			 0x0000000000000000,
			 0x0000000000000000,
			 0x0000000000000000,
			 0x0000000000000000,
			 0x0000005f00000000,
			 0x0000030003000000,
			 0x643c26643c262400,
			 0x2649497f49493200,
			 0x4225120824522100,
			 0x20504e5522582800,
			 0x0000000300000000,
			 0x00001c2241000000,
			 0x00000041221c0000,
			 0x0015150e0e151500,
			 0x0008083e08080000,
			 0x0000005030000000,
			 0x0008080808080000,
			 0x0000004000000000,
			 0x4020100804020100,
			 0x003e4141413e0000,
			 0x0000417f40000000,
			 0x00426151496e0000,
			 0x0022414949360000,
			 0x001814127f100000,
			 0x0027494949710000,
			 0x003c4a4948700000,
			 0x004321110d030000,
			 0x0036494949360000,
			 0x00060949291e0000,
			 0x0000001200000000,
			 0x0000005230000000,
			 0x0000081414220000,
			 0x0014141414141400,
			 0x0000221414080000,
			 0x0002015905020000,
			 0x3e415d554d512e00,
			 0x407c4a094a7c4000,//A
			 0x417f494949493600,//B
			 0x1c22414141412200,//C
			 0x417f414141221c00,
			 0x417f49495d416300,
			 0x417f49091d010300,
			 0x1c224149493a0800,
			 0x417f0808087f4100,
			 0x0041417F41410000,
			 0x304041413F010100,
			 0x417f080c12614100,
			 0x417f414040406000,
			 0x417f420c427f4100,
			 0x417f420c117f0100,
			 0x1c22414141221c00,
			 0x417f490909090600,
			 0x0c12212161524c00,
			 0x417f090919694600,
			 0x6649494949493300,
			 0x0301417f41010300,
			 0x013f4140413f0100,
			 0x010f3140310f0100,
			 0x011f6114611f0100,
			 0x4141360836414100,
			 0x0103447844030100,
			 0x4361514945436100,
			 0x00007f4141000000,
			 0x0102040810204000,
			 0x000041417f000000,
			 0x0004020101020400,
			 0x0040404040404000,
			 0x0001020000000000,
			 0x00344a4a4a3c4000,
			 0x00413f4848483000,
			 0x003c424242240000,
			 0x00304848493f4000,
			 0x003c4a4a4a2c0000,
			 0x0000487e49090000,
			 0x00264949493f0100,
			 0x417f480444784000,
			 0x0000447d40000000,
			 0x000040443d000000,
			 0x417f101824424200,
			 0x0040417f40400000,
			 0x427e027c027e4000,
			 0x427e4402427c4000,
			 0x003c4242423c0000,
			 0x00417f4909090600,
			 0x00060909497f4100,
			 0x00427e4402020400,
			 0x00644a4a4a360000,
			 0x00043f4444200000,
			 0x00023e4040227e40,
			 0x020e3240320e0200,
			 0x021e6218621e0200,
			 0x4262140814624200,
			 0x0143453805030100,
			 0x004662524a466200,
			 0x0000083641000000,
			 0x0000007f00000000,
			 0x0000004136080000,
			 0x0018080810101800,
			 0xAA55AA55AA55AA55
			};

/***************************************************************************//**
 * @brief Initializes ssd_1306 for display screening.
 *
 * @param dev - The device structure.
 *
 * @return SUCCESS in case of success, negative error code otherwise.
*******************************************************************************/
int32_t ssd_1306_init(struct display_dev *device)
{
	int32_t	ret;
	uint8_t command[3];
	// initial pin state
	ret = gpio_set_value(device->reset_pin, RST_OFF);
	if (ret != SUCCESS)
		return FAILURE;
	ret = gpio_set_value(device->dc_pin, DC_CMD);
	if (ret != SUCCESS)
		return FAILURE;
	usleep(2000U);

	command[0] = 0xAE;
	ret = spi_write_and_read(device->spi_desc, command, 1U);
	if (ret != SUCCESS)
		return FAILURE;
	ret = gpio_set_value(device->reset_pin, RST_ON);
	if (ret != SUCCESS)
		return FAILURE;
	usleep(2000U);
	ret = gpio_set_value(device->reset_pin, RST_OFF);
	if (ret != SUCCESS)
		return FAILURE;
	usleep(2000U);
	// charge pump
	command[0] = 0x8D;
	command[1] = 0x14;
	ret = spi_write_and_read(device->spi_desc, command, 2U);
	if (ret != SUCCESS)
		return FAILURE;
	// pre-charge
	command[0] = 0xD9;
	command[1] = 0xF1;
	ret = spi_write_and_read(device->spi_desc, command, 2U);
	if (ret != SUCCESS)
		return FAILURE;
	// set contrast
	command[0] = 0x81;
	command[1] = 0xFF;
	ret = spi_write_and_read(device->spi_desc, command, 2U);
	if (ret != SUCCESS)
		return FAILURE;
	// set segment remap
	command[0] = 0xA0;
	ret = spi_write_and_read(device->spi_desc, command, 1U);
	if (ret != SUCCESS)
		return FAILURE;
	// set scan direction
	command[0] = 0xC0;
	ret = spi_write_and_read(device->spi_desc, command, 1U);
	if (ret != SUCCESS)
		return FAILURE;
	// set COM pin
	command[0] = 0xDA;
	command[1] = 0x00;
	ret = spi_write_and_read(device->spi_desc, command, 2U);
	if (ret != SUCCESS)
		return FAILURE;
	// show written memory on screen
	command[0] = 0xA4;
	ret = spi_write_and_read(device->spi_desc, command, 1U);
	if (ret != SUCCESS)
		return FAILURE;
	command[0] = 0x20;	// memory addressing mode
	command[1] = 0x00;	// horizontal addressing
	ret = spi_write_and_read(device->spi_desc, command, 2U);

	return ret;
}

/***************************************************************************//**
 * @brief  ssd_1306 turns display on/off.
 *
 * @param dev    - The device structure
 *        on_off - Display state
 *
 * @return SUCCESS in case of success, negative error code otherwise.
*******************************************************************************/
int32_t ssd_1306_display_on_off(struct display_dev *device, uint8_t on_off)
{
	int32_t	ret;
	uint8_t command;

	ret = gpio_set_value(device->dc_pin, DC_CMD);
	if (ret != SUCCESS)
		return FAILURE;
	/* AF = on, AE = OFF */
	command = (on_off == DISP_ON) ? 0xAF : 0xAE;
	ret = spi_write_and_read(device->spi_desc, &command, 1U);

	return ret;
}

/***************************************************************************//**
 * @brief  Moves cursor to desired row/column.
 *
 * @param dev    - The device structure
 *        row    - row
 *        column - column
 *
 * @return SUCCESS in case of success, negative error code otherwise.
*******************************************************************************/
int32_t ssd_1306_move_cursor(struct display_dev *device, uint8_t row,
			     uint8_t column)
{
	int32_t	ret;
	uint8_t command[3];
	ret = gpio_set_value(device->dc_pin, DC_CMD);
	if (ret != SUCCESS)
		return FAILURE;
	command[0] = 0x21;
	command[1] = column*8;
	command[2] = device->cols_nb * 8 - 1U;
	ret = spi_write_and_read(device->spi_desc, &command, 3U);
	if (ret != SUCCESS)
		return FAILURE;
	command[0] = 0x22;
	command[1] = row;
	command[2] = device->rows_nb - 1U;
	ret = spi_write_and_read(device->spi_desc, &command, 3U);

	return ret;
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
int32_t ssd_1306_print_ascii(struct display_dev *device, uint8_t ascii,
			     uint8_t row, uint8_t column)
{
	int32_t ret;
	int8_t ch[8];
	int8_t i;

	ret = ssd_1306_move_cursor(device, row, column);
	if (ret != SUCCESS)
		return FAILURE;
	ret = gpio_set_value(device->dc_pin, DC_DATA);
	if (ret != SUCCESS)
		return FAILURE;
	for(i = 0; i < 8; i++) {
		ch[7-i] = (chr_map[ascii] >> (8*i)) & 0xFF;
	}
	ret = spi_write_and_read(device->spi_desc, ch, 8U);

	return ret;
}
