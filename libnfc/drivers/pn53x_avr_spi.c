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
 */

/**
 * @file pn53x_avr_spi.c
 * @brief Driver for PN53x using SPI and running on AVR
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif // HAVE_CONFIG_H

#include <assert.h>
//#include <stdio.h>
//#include <stdlib.h>
#include <inttypes.h>

#include <string.h>

#include <nfc/nfc.h>

#include "nfc-internal.h"
#include "chips/pn53x.h"
#include "chips/pn53x-internal.h"
#include "drivers/pn53x_avr_spi.h"
#include "buses/avr_spi.h"

#if !defined(DRIVER_PN53X_AVR_SPI_ENABLED)
#  error "You need to define DRIVER_PN53X_AVR_SPI_ENABLED in order to compile this file!"
#endif

#define PN53X_AVR_SPI_DRIVER_NAME "pn53x_avr_spi"
#define LOG_CATEGORY "libnfc.driver.pn53x_avr_spi"

static int pn53x_avr_spi_ack(nfc_device *pnd);
static const struct pn53x_io pn53x_avr_spi_io;

/**
 * @brief List opened devices
 *
 * @param connstring array of nfc_connstring where found device's connection strings will be stored.
 * @param connstrings_len size of connstrings array.
 * @return number of devices found.
 */
static size_t
pn53x_avr_spi_scan(nfc_connstring connstrings[], const size_t connstrings_len)
{
    strcpy(connstrings[0], PN53X_AVR_SPI_DRIVER_NAME);
    return 1;
}

/**
 * @return the opened device
 */
static nfc_device *
pn53x_avr_spi_open(const nfc_connstring connstring)
{
    if (strcmp(connstring, PN53X_AVR_SPI_DRIVER_NAME) != 0)
    {
        // can't open if it's not PN53X_AVR_SPI_DRIVER_NAME.
        return NULL;
    }

    // This is the one and only AVR SPI device that can be open. For now.
    // Add support for multiple devices if there is need for it.
    static nfc_device the_avr_spi_device;

    const avr_spi_handle hSpi = avr_spi_open(connstring);
    if (hSpi == NULL)
    {
        return NULL;
    }

    nfc_device* pnd = &the_avr_spi_device;
    pnd->driver = &pn53x_avr_spi_driver;
    pnd->driver_data = hSpi;
    pn53x_data_new(pnd, &pn53x_avr_spi_io);
    strncpy(pnd->name, PN53X_AVR_SPI_DRIVER_NAME, sizeof(pnd->name));
    strncpy(pnd->connstring, PN53X_AVR_SPI_DRIVER_NAME, sizeof(pnd->connstring));
    pnd->bCrc = true; // ?? don't know that
    pnd->bPar = true; // ?? don't know that
    pnd->bEasyFraming = false; // ?? don't know that
    pnd->bAutoIso14443_4 = false; // ?? don't know that
    pnd->btSupportByte = 0;
    pnd->last_error = 0;

    // HACK1: Send first an ACK as Abort command, to reset chip before talking to it:
    pn53x_avr_spi_ack(pnd);

    return pnd;
}

static void
pn53x_avr_spi_close(nfc_device *pnd)
{
    assert(pnd != NULL);

    avr_spi_close(pnd->driver_data);
    pn53x_data_free(pnd);
    pnd->driver_data = NULL;
    pnd->driver = NULL;
}

#define PN53X_AVR_SPI_BUFFER_LEN (PN53x_EXTENDED_FRAME__DATA_MAX_LEN + PN53x_EXTENDED_FRAME__OVERHEAD)

static int
pn53x_avr_spi_send(nfc_device *pnd, const uint8_t *pbtData, const size_t szData, const int timeout)
{
    assert(pnd != NULL);
    return avr_spi_send(pnd->driver_data, pbtData, szData, timeout);
}

#define AVR_SPI_TIMEOUT_PER_PASS 200
static int
pn53x_avr_spi_receive(nfc_device *pnd, uint8_t *pbtData, const size_t szDataLen, const int timeout)
{
    assert(pnd != NULL);
    assert(pbtData != NULL);

    return avr_spi_receive(pnd->driver_data, pbtData, szDataLen, NULL, timeout);
}

static int
pn53x_avr_spi_ack(nfc_device *pnd)
{
    // TODO: send ACK frame
	return 0;
}

static int
pn53x_avr_spi_abort_command(nfc_device *pnd)
{
    //DRIVER_DATA(pnd)->abort_flag = true;
    return NFC_SUCCESS;
}

static const struct pn53x_io pn53x_avr_spi_io = {
    .send       = pn53x_avr_spi_send,
    .receive    = pn53x_avr_spi_receive
};

const struct nfc_driver pn53x_avr_spi_driver = {
    .name                             = PN53X_AVR_SPI_DRIVER_NAME,
    .scan                             = pn53x_avr_spi_scan,
    .open                             = pn53x_avr_spi_open,
    .close                            = pn53x_avr_spi_close,
    .strerror                         = pn53x_strerror,

    .initiator_init                   = pn53x_initiator_init,
    .initiator_init_secure_element    = NULL, // No secure-element support
    .initiator_select_passive_target  = pn53x_initiator_select_passive_target,
    .initiator_poll_target            = pn53x_initiator_poll_target,
    .initiator_select_dep_target      = pn53x_initiator_select_dep_target,
    .initiator_deselect_target        = pn53x_initiator_deselect_target,
    .initiator_transceive_bytes       = pn53x_initiator_transceive_bytes,
    .initiator_transceive_bits        = pn53x_initiator_transceive_bits,
    .initiator_transceive_bytes_timed = pn53x_initiator_transceive_bytes_timed,
    .initiator_transceive_bits_timed  = pn53x_initiator_transceive_bits_timed,
    .initiator_target_is_present      = pn53x_initiator_target_is_present,

    .target_init                      = pn53x_target_init,
    .target_send_bytes                = pn53x_target_send_bytes,
    .target_receive_bytes             = pn53x_target_receive_bytes,
    .target_send_bits                 = pn53x_target_send_bits,
    .target_receive_bits              = pn53x_target_receive_bits,

    .device_set_property_bool         = pn53x_set_property_bool,
    .device_set_property_int          = pn53x_set_property_int,
    .get_supported_modulation         = pn53x_get_supported_modulation,
    .get_supported_baud_rate          = pn53x_get_supported_baud_rate,
    .device_get_information_about     = pn53x_get_information_about,

    .abort_command                    = pn53x_avr_spi_abort_command,
    .idle                             = pn53x_idle
};
