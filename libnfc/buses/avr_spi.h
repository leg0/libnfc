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
void avr_spi_close(avr_spi_handle h);
//void avr_spi_flush_input(avr_spi_handle h);

/// Starts an SPI transaction (selects the device / sets SS low). 
///
/// @pre there is no ongoing transaction.
/// @post there is an ongoing transaction.
///
void avr_spi_begin_transaction(avr_spi_handle h);

/// Ends the SPI transaction (deselects the device / sets SS high).
///
/// @pre there is an ongoing transaction.
/// @post there is no ongoing transaction.
///
void avr_spi_end_transaction(avr_spi_handle h);

/// Transmit and receive a single byte. This must be called within transaction only. 
//
/// @param [in] h - bus handle returned by avr_spi_open.
/// @param [in] tx - a byte to transmit
///
/// @return byte received from the device.
///
uint8_t avr_spi_transceive_byte(avr_spi_handle h, uint8_t tx);

/// Receives a number of bytes from the device. This function must be called within 
/// a transaction only.
///
/// @param [out] pbtRx - buffer to write the received bytes to. Must hold at
///                      least szRx bytes.
/// @param [in] szRx - size of the receive buffer.
///
int avr_spi_receive(avr_spi_handle h, uint8_t *pbtRx, size_t szRx, void *abort_p, int timeout);

/// Sends a number of bytes to the device. This function must be called within a
/// transaction only.
int avr_spi_send(avr_spi_handle h, const uint8_t *pbtTx, size_t szTx, int timeout);

char** avr_spi_list_ports(void);
