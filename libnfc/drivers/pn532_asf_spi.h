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
 */

/**
 * @file pn532_asf_spi.h
 * @brief Driver for PN532 devices over SPI bus using ASF.
 */

#if !defined(DRIVER_PN532_ASF_SPI_ENABLED)
#  error "You need to define DRIVER_PN532_ASF_SPI_ENABLED in order to compile this file!"
#endif

#include <nfc/nfc-types.h>

extern const struct nfc_driver pn532_asf_spi_driver;
