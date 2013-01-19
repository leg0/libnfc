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
 * @file asf_spi.h
 * @brief SPI header using ASF (Atmel Software Foundation).
 */
#include <nfc/nfc-types.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct asf_spi asf_spi;
typedef asf_spi* asf_spi_handle;

typedef struct asf_spi_selector
{
    void (*select)();
    void (*deselect)();
} asf_spi_selector;

/// Opens the SPI bus. Currently only one bus is supported. If the bus is already open
/// this function fails.
///
/// @param name - reserved for future use.
///
/// @pre bus is not open
/// @post bus is open
///
/// @return a handle to the SPI bus 
///
asf_spi_handle asf_spi_open(const char *name);

/// Closes the opened bus.
///
/// @param h - a handle returned by avr_spi_open.
///
/// @pre h points to an opened bus
/// @post the bus is closed an can no longer be used.
/// 
void asf_spi_close(asf_spi_handle h);

/// Starts an SPI transaction (selects the device using selector). 
///
/// @param [in] selector - provides functions to select and deselect a device on the bus.
///
/// @pre there is no ongoing transaction.
/// @post there is an ongoing transaction.
/// @post a device on the bus is selected.
///
void asf_spi_begin_transaction(asf_spi_handle h, asf_spi_selector const* selector);

/// Ends the SPI transaction (deselects the device / sets SS high).
///
/// @pre there is an ongoing transaction.
/// @post there is no ongoing transaction.
/// @post no device in the bus is selected.
///
void asf_spi_end_transaction(asf_spi_handle h);

/// Transmit and receive a single byte. This must be called within transaction only. 
//
/// @param [in] h - bus handle returned by asf_spi_open.
/// @param [in] tx - a byte to transmit
///
/// @return byte received from the device.
///
uint8_t asf_spi_transceive_byte(asf_spi_handle h, uint8_t tx);

/// Receives a number of bytes from the device. This function must be called within 
/// a transaction only.
///
/// @param [out] pbtRx - buffer to write the received bytes to. Must hold at
///                      least szRx bytes.
/// @param [in] szRx - size of the receive buffer.
///
int asf_spi_receive(asf_spi_handle h, uint8_t *pbtRx, size_t szRx, void *abort_p, int timeout);

/// Sends a number of bytes to the device. This function must be called within a
/// transaction only.
///
/// @param [in] pbtTx - buffer that contains data to send. This buffer must hold at least
///                     szTx bytes
/// @param [in] szTx - number of bytes to send from the buffer.
///
/// @return ?
///
int asf_spi_send(asf_spi_handle h, const uint8_t *pbtTx, size_t szTx, int timeout);

/// ?
char** asf_spi_list_ports(void);
