#pragma once
/*-
 * Public platform independent Near Field Communication (NFC) library
 *
 * Copyright (C) 2012 Ahti Legonkov
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 */

/**
 * @file avr_spi.h
 * @brief AVR SPI header
 */

#  include <stdio.h>
#  include <string.h>
#  include <stdlib.h>


#  include <nfc/nfc-types.h>

typedef struct avr_spi avr_spi;
typedef avr_spi* avr_spi_handle;

avr_spi_handle avr_spi_open(const char *pcPortName);
void    avr_spi_close(avr_spi_handle h);
void    avr_spi_flush_input(avr_spi_handle h);

void    avr_spi_set_speed(avr_spi_handle h, uint32_t uiPortSpeed);
uint32_t avr_spi_get_speed(avr_spi_handle h);

int     avr_spi_receive(avr_spi_handle h, uint8_t *pbtRx, size_t szRx, void *abort_p, int timeout);
int     avr_spi_send(avr_spi_handle h, const uint8_t *pbtTx, size_t szTx, int timeout);

char** avr_spi_list_ports(void);
