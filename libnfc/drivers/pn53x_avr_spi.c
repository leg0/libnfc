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

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <sys/select.h>
#include <sys/types.h>
#include <errno.h>

#include <string.h>

#include <nfc/nfc.h>

#include "nfc-internal.h"
#include "chips/pn53x.h"
#include "chips/pn53x-internal.h"
#include "drivers/pn53x_avr_spi.h"

#define PN53X_AVR_SPI_DRIVER_NAME "pn53x_avr_spi"
#define LOG_CATEGORY "libnfc.driver.pn53x_avr_spi"

const struct pn53x_io pn53x_avr_spi_io;

static int
pn53x_avr_spi_bulk_read(struct pn53x_avr_spi_data *data, uint8_t abtRx[], const size_t szRx, const int timeout)
{
}

static int
pn53x_avr_spi_bulk_write(struct pn53x_avr_spi_data *data, uint8_t abtTx[], const size_t szTx, const int timeout)
{
}

struct pn53x_avr_spi_supported_device {
  const char *name;
};


static pn53x_avr_spi_model
pn53x_avr_spi_get_device_model(uint16_t vendor_id, uint16_t product_id)
{
}

int  pn53x_avr_spi_ack(nfc_device *pnd);

static size_t
pn53x_avr_spi_scan(nfc_connstring connstrings[], const size_t connstrings_len)
{
}

struct pn53x_avr_spi_descriptor {
  char *dirname;
  char *filename;
};

static int
pn53x_avr_spi_connstring_decode(const nfc_connstring connstring, struct pn53x_avr_spi_descriptor *desc)
{
}

bool
pn53x_avr_spi_get_usb_device_name(struct avr_spi_device *dev, avr_spi_handle *udev, char *buffer, size_t len)
{
  // copy "SPI to buffer
  return true;
}

static nfc_device *
pn53x_avr_spi_open(const nfc_connstring connstring)
{
      pnd->driver = &pn53x_avr_spi_driver;

      // HACK1: Send first an ACK as Abort command, to reset chip before talking to it:
      pn53x_avr_spi_ack(pnd);

  return pnd;
}

static void
pn53x_avr_spi_close(nfc_device *pnd)
{
}

#define PN53X_AVR_SPI_BUFFER_LEN (PN53x_EXTENDED_FRAME__DATA_MAX_LEN + PN53x_EXTENDED_FRAME__OVERHEAD)

static int
pn53x_avr_spi_send(nfc_device *pnd, const uint8_t *pbtData, const size_t szData, const int timeout)
{
  return NFC_SUCCESS;
}

#define AVR_SPI_TIMEOUT_PER_PASS 200
static int
pn53x_avr_spi_receive(nfc_device *pnd, uint8_t *pbtData, const size_t szDataLen, const int timeout)
{
  size_t len;
  off_t offset = 0;

  uint8_t  abtRxBuf[PN53X_AVR_SPI_BUFFER_LEN];
  int res;

  return len;
}

int
pn53x_avr_spi_ack(nfc_device *pnd)
{
  return pn53x_avr_spi_bulk_write(DRIVER_DATA(pnd), (uint8_t *) pn53x_ack_frame, sizeof(pn53x_ack_frame), -1);
}

int
pn53x_avr_spi_init(nfc_device *pnd)
{
  return NFC_SUCCESS;
}

static int
pn53x_avr_spi_set_property_bool(nfc_device *pnd, const nfc_property property, const bool bEnable)
{
  int res = 0;
  if ((res = pn53x_set_property_bool(pnd, property, bEnable)) < 0)
    return res;

  switch (DRIVER_DATA(pnd)->model) {
    case ASK_LOGO:
      if (NP_ACTIVATE_FIELD == property) {
        /* Switch on/off LED2 and Progressive Field GPIO according to ACTIVATE_FIELD option */
        log_put(LOG_CATEGORY, NFC_PRIORITY_TRACE, "Switch progressive field %s", bEnable ? "On" : "Off");
        if ((res = pn53x_write_register(pnd, PN53X_SFR_P3, _BV(P31) | _BV(P34), bEnable ? _BV(P34) : _BV(P31))) < 0)
          return NFC_ECHIP;
      }
      break;
    case SCM_SCL3711:
      if (NP_ACTIVATE_FIELD == property) {
        // Switch on/off LED according to ACTIVATE_FIELD option
        if ((res = pn53x_write_register(pnd, PN53X_SFR_P3, _BV(P32), bEnable ? 0 : _BV(P32))) < 0)
          return res;
      }
      break;
    case NXP_PN531:
    case NXP_PN533:
    case SONY_PN531:
    case SONY_RCS360:
    case UNKNOWN:
      // Nothing to do.
      break;
  }
  return NFC_SUCCESS;
}

static int
pn53x_avr_spi_abort_command(nfc_device *pnd)
{
  DRIVER_DATA(pnd)->abort_flag = true;
  return NFC_SUCCESS;
}

const struct pn53x_io pn53x_avr_spi_io = {
  .send       = pn53x_avr_spi_send,
  .receive    = pn53x_avr_spi_receive,
};

const struct nfc_driver pn53x_avr_spi_driver = {
  .name                             = PN53X_AVR_SPI_DRIVER_NAME,
  .scan                             = pn53x_avr_spi_scan,
  .open                             = pn53x_avr_spi_open,
  .close                            = pn53x_avr_spi_close,
  .strerror                         = pn53x_avr_spi_strerror,

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

  .target_init           = pn53x_target_init,
  .target_send_bytes     = pn53x_target_send_bytes,
  .target_receive_bytes  = pn53x_target_receive_bytes,
  .target_send_bits      = pn53x_target_send_bits,
  .target_receive_bits   = pn53x_target_receive_bits,

  .device_set_property_bool     = pn53x_avr_spi_set_property_bool,
  .device_set_property_int      = pn53x_set_property_int,
  .get_supported_modulation     = pn53x_get_supported_modulation,
  .get_supported_baud_rate      = pn53x_get_supported_baud_rate,
  .device_get_information_about = pn53x_get_information_about,

  .abort_command  = pn53x_avr_spi_abort_command,
  .idle  = pn53x_idle,
};
