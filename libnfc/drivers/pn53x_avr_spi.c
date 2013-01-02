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
#include <limits.h>

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

#define DRIVER_DATA(pnd) ((avr_spi_handle)(pnd->driver_data))

//
// Begin P70_IRQ 
//

// P70_IRQ is triggered by the PN53x when it has data available for the
// host.

#if defined(__AVR_ATmega328P__) // Arduino Uno Rev3
#  define PN532_P70_IRQ_PORT_OUT PORTD
#  define PN532_P70_IRQ_PORT_IN  PIND
#  define PN532_P70_IRQ_DDR      DDRD
#  define PN532_P70_IRQ_PIN      PORTD2
#  define PN532_P70_IRQ_VECT     INT0_vect
#  define PN532_P70_IRQ_INT      INT0
#  define PN532_P70_IRQ_MSK      EIMSK
#  define PN532_P70_IRQ_EICR     EICRA
#  define PN532_P70_IRQ_ISC1     ISC01
#  define PN532_P70_IRQ_ISC0     ISC00
#elif defined(__AVR_ATmega2560__) // Arduino Mega 2560
#  define PN532_P70_IRQ_PORT_OUT PORTE
#  define PN532_P70_IRQ_PORT_IN  PINE
#  define PN532_P70_IRQ_DDR      DDRE
#  define PN532_P70_IRQ_PIN      PORTE4
#  define PN532_P70_IRQ_VECT     INT4_vect
#  define PN532_P70_IRQ_INT      INT4
#  define PN532_P70_IRQ_MSK      EIMSK
#  define PN532_P70_IRQ_EICR     EICRB
#  define PN532_P70_IRQ_ISC1     ISC41
#  define PN532_P70_IRQ_ISC0     ISC40
#else
#  error "Don't know where the P70_IRQ is connected to"
#endif

// According to user manual, in SPI mode one of the following bytes must be sent
// before reading writing or asking status.
const uint8_t PN532_SPI_DATAWRITE = 0x01;
const uint8_t PN532_SPI_STATREAD = 0x02;
const uint8_t PN532_SPI_DATAREAD = 0x03;

const uint8_t PN532_SPI_READY = 0x01;

#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>

static uint8_t p70_irq_pin();

volatile bool p70_irq = true;
ISR(PN532_P70_IRQ_VECT)
{
    p70_irq = p70_irq_pin();
    //printf("interrupt: p70_irq=%d\n", p70_irq);
}


/// Waits for p70_irq to change to specified state.
///
/// @param [in] timeout_ms - number of milliseconds to wait for
///        state change.
///
/// @retval true if p70_irq is in required state.
/// @retval false if the wait timed out.
static bool p70_irq_wait(int timeout_ms)
{
    for (int i = 0; p70_irq == 1; ++i)
    {
        if (i >= timeout_ms)
            return false;
        _delay_ms(1);
    }
    
    return true;
}    

static void p70_irq_reset()
{
    p70_irq = true;
}

static void p70_irq_init()
{
    sei();

    // enable the interrupt
    PN532_P70_IRQ_MSK |= _BV(PN532_P70_IRQ_INT);
    
    // trigger interrupt on falling edge only
    PN532_P70_IRQ_EICR |= _BV(PN532_P70_IRQ_ISC1);
    PN532_P70_IRQ_EICR &= _BV(PN532_P70_IRQ_ISC0);
    
    // make the pin an input
    PN532_P70_IRQ_DDR &= ~_BV(PN532_P70_IRQ_PIN);

    p70_irq_reset();
}    

static uint8_t p70_irq_pin()
{
    return (PN532_P70_IRQ_PORT_IN & _BV(PN532_P70_IRQ_PIN)) != 0;
}

//
// End P70_IRQ
//

//
// Begin device select
//

static void pn53x_avr_spi_init_selector()
{
    // XXX: make this a avr device dependent.
    DDRB |= _BV(PORTB4);
    PORTB |= _BV(PORTB4);
}
    
static void pn53x_avr_spi_select()
{
    PORTB &= ~_BV(PORTB4);
}

static void pn53x_avr_spi_deselect()
{
    PORTB |= _BV(PORTB4);
}    

static avr_spi_selector const pn532_avr_spi_selector =
{
    .select = pn53x_avr_spi_select,
    .deselect = pn53x_avr_spi_deselect
};    

//
// End device select
//


static int pn53x_avr_spi_handshake(nfc_device *pnd);
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

    // TODO: selector should be passed to avr_spi_open, and stored in
    // driver_data somehow.
    pn53x_avr_spi_init_selector();
    
    const avr_spi_handle hSpi = avr_spi_open(connstring);
    if (hSpi == NULL)
    {
        return NULL;
    }

    p70_irq_init();
                    
    nfc_device* pnd = &the_avr_spi_device;
    pnd->driver = &pn53x_avr_spi_driver;
    pnd->driver_data = hSpi;
    pn53x_data_new(pnd, &pn53x_avr_spi_io);
    CHIP_DATA(pnd)->type = PN532;
    strncpy(pnd->name, PN53X_AVR_SPI_DRIVER_NAME, sizeof(pnd->name));
    strncpy(pnd->connstring, PN53X_AVR_SPI_DRIVER_NAME, sizeof(pnd->connstring));
    pnd->bCrc = true; // ?? don't know that
    pnd->bPar = true; // ?? don't know that
    pnd->bEasyFraming = false; // ?? don't know that
    pnd->bAutoIso14443_4 = false; // ?? don't know that
    pnd->btSupportByte = 0;
    pnd->last_error = 0;
       
    if (pn53x_avr_spi_handshake(pnd) == NFC_SUCCESS)
    {
        return pnd;
    }
    else
    {
        return NULL;
    }                
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

static uint8_t
pn53x_read_status(nfc_device* pnd)
{
    assert(pnd != NULL);
    
    return avr_spi_transceive_byte(pnd->driver_data, PN532_SPI_STATREAD);
}

static uint8_t
pn53x_read_byte(nfc_device* pnd)
{
    assert(pnd != NULL);
    
    return avr_spi_transceive_byte(pnd->driver_data, PN532_SPI_DATAREAD);
}
        
static bool
pn53x_avr_spi_wait_ready(nfc_device* pnd, const int timeout)
{
    return p70_irq_wait(timeout);
}    

/// This is a type for return values of the functions in this class. Just plain success/fail is
/// not enough - we need a reason of failure.
typedef enum ReturnValue
{
    Success,
    S_AckFrame,
    S_NackFrame,
    E_Fail,
    E_InsufficientBuffer,
    E_MultipleTags,
    E_DataChecksum,
    E_LengthChecksum,
    E_ApplicationError,
    E_ErrorFrame,
} ReturnValue;

/// @param [out] bytesRead - number of bytes written to rxBuffer.
static ReturnValue read_data(nfc_device* pnd, uint8_t* const rxBuffer, const uint8_t szRx, uint8_t* bytesRead)
{
    // TODO: optimize - read more than one byte at a time from SPI.
    
    assert(pnd != NULL);
    assert(rxBuffer != NULL);
    assert(bytesRead != NULL);
    
    // 00 00 FF <LEN> <LCS> D5 <CC+1> <optional output data> <DCS> 00
    typedef enum ReadState {
        //ReadingPreamble,
        ReadingStartCode1,
        ReadingStartCode2,
        ReadingLength,
        ReadingLcs,
        ReadingTfi,
        ReadingOutputData,
        ReadingDcs,
        ReadingPostamble,
        Done
    } ReadState;

    ReadState state = ReadingStartCode1;
    uint8_t length = 0;
    uint8_t* p = rxBuffer;
    uint8_t* end = rxBuffer + szRx;
    uint8_t dcs = 0xd5;
    while (state != Done)
    {
        if (p > end)
        {
            //printf("\nError: insufficient buffer\n");
            //printf("state=%d\n", state);
            return E_InsufficientBuffer;
        }
        uint8_t const b = *(p++) = pn53x_read_byte(pnd);
        //printf("%02X-", b);
        //printf("");
        _delay_ms(1); // XXX: why does it not work without this delay?

        switch (state)
        {
        case Done:
            // Reading should have ended by now!
            assert(false);
            break;
            
        case ReadingStartCode1:
            if (b == 0x00)
            {
                state = ReadingStartCode2;
            }
            else
            {
                p = rxBuffer;
                //Debug::println();
            }
            break;
        case ReadingStartCode2:
            if (b == 0xff)
            {
                state = ReadingLength;
            }
            else if (b == 0x00)
            {
                state = ReadingStartCode2;
                p = rxBuffer+1;
            }
            else
            {
                state = ReadingStartCode1;
                p = rxBuffer;
                //Debug::println();
            }
            break;
        case ReadingLength:
                length = b;
                if (length+PN53x_NORMAL_FRAME__OVERHEAD > szRx) 
                {                     
                    return E_InsufficientBuffer;
                }
                else
                {
                    *bytesRead = length+PN53x_NORMAL_FRAME__OVERHEAD;
                    state = ReadingLcs;
                    break;
                }                    
        case ReadingLcs:
            if (b == 0xff && length == 0)
            {
                *(p++) = pn53x_read_byte(pnd);
                *bytesRead = 6;
                return S_AckFrame;
            }
            else if (b == 0 && length == 0xff)
            {
                *(p++) = pn53x_read_byte(pnd);
                *bytesRead = 6;
                return S_NackFrame;
            }
            else if ((uint8_t)(b+length) != 0)
            {
                //Debug::println("Error: length checksum mismatch");
                return E_LengthChecksum;
            }
            else
            {
                state = ReadingTfi;
            }
            break;
        case ReadingTfi:
            if (b != 0xD5/*Tfi_Pn5xxToHost*/)
                return E_ErrorFrame;
            else
            {
                state = ReadingOutputData;
                --length; // TFI is included in length
            }
            break;
        case ReadingOutputData:
            if (length > 0)
            {
                dcs += b;
                --length;
                break;
            }
            else
            {
                state = ReadingDcs;
            }
            // no break
        case ReadingDcs:
            if ((uint8_t)(dcs + b) != 0)
            {
                //Debug::println("Error: data checksum error");
                return E_DataChecksum;
            }
            else
            {
                state = ReadingPostamble;
            }
            break;
        case ReadingPostamble:
            state = Done;
            break;
        }
    }

    puts("~");
    return Success;
}
    
static bool pn53x_avr_spi_wait_ack(nfc_device* pnd, const int timeout)
{    
    uint8_t ackBuffer[7];
    uint8_t bytesRead = 0;
    return S_AckFrame == read_data(pnd, ackBuffer, sizeof(ackBuffer), &bytesRead);
}
    
#define PN53X_AVR_SPI_BUFFER_LEN (PN53x_EXTENDED_FRAME__DATA_MAX_LEN + PN53x_EXTENDED_FRAME__OVERHEAD)

static void dump_buf(uint8_t const* buf, size_t size)
{
    assert(buf != NULL);
    
    for (size_t i = 0; i < size; ++i)
    {
        printf("%02X ", buf[i]);
    }        
}
    
static int
pn53x_avr_spi_send(nfc_device *pnd, const uint8_t *pbtData, const size_t szData, const int timeout)
{
    assert(pnd != NULL);
    assert(pbtData != NULL);
    assert(szData < 256);

    uint8_t const PN532_PREAMBLE = 0x00;
    uint8_t const PN532_STARTCODE1 = 0x00;
    uint8_t const PN532_STARTCODE2 = 0xFF;
    uint8_t const PN532_POSTAMBLE = 0x00;
    
    uint8_t const len = szData + 1; // Length of data and TFI.
    uint8_t const lcs = ~len + 1;
    assert(((len + lcs) & 0xFF) == 0);
    
    uint8_t const tfi = 0xD4; // Host-to-PN
    uint8_t const buf1[] = {PN532_PREAMBLE, PN532_STARTCODE1, PN532_STARTCODE2, len, lcs, tfi};

    uint8_t dcs = tfi;
    for (uint8_t i = 0; i < szData; ++i)
    {
        dcs += pbtData[i];
    }
    dcs = ~dcs + 1;
    uint8_t const buf2[] = { dcs, PN532_POSTAMBLE };
    
    //printf("%s(%d): Sending: ", __FILE__, __LINE__);
    //dump_buf(buf1, sizeof(buf1));
    //dump_buf(pbtData, szData);
    //dump_buf(buf2, sizeof(buf2));
    //puts("");
    
    // Send the command
    p70_irq_reset();
    avr_spi_begin_transaction(DRIVER_DATA(pnd), &pn532_avr_spi_selector);
        avr_spi_transceive_byte(DRIVER_DATA(pnd), PN532_SPI_DATAWRITE);
        avr_spi_send(DRIVER_DATA(pnd), buf1, sizeof(buf1), timeout);
        avr_spi_send(DRIVER_DATA(pnd), pbtData, szData,    timeout);
        avr_spi_send(DRIVER_DATA(pnd), buf2, sizeof(buf2), timeout);
    avr_spi_end_transaction(DRIVER_DATA(pnd));
    
    // Wait for ACK
    if (!p70_irq_wait(INT_MAX))
    {
        //printf("%s(%d): Timeout waiting for p70_irq\n", __FILE__, __LINE__);
        return NFC_ETIMEOUT;
    }
    p70_irq_reset();
    
    avr_spi_begin_transaction(DRIVER_DATA(pnd), &pn532_avr_spi_selector);
        uint8_t ack_buf[6];
        avr_spi_transceive_byte(DRIVER_DATA(pnd), PN532_SPI_DATAREAD);
        avr_spi_receive(DRIVER_DATA(pnd), ack_buf, sizeof(ack_buf), NULL, timeout);
    avr_spi_end_transaction(DRIVER_DATA(pnd));
    
    if (memcmp(ack_buf, pn53x_ack_frame, sizeof(ack_buf)) == 0)
    {
        return NFC_SUCCESS;
    }
    else
    {
        printf("%s(%d): Expected ACK, but got %02x %02x %02x %02x %02x %02x\n", __FILE__, __LINE__,
            ack_buf[0], ack_buf[1], ack_buf[2], ack_buf[3], ack_buf[4], ack_buf[5]);
            
        return NFC_EIO;
    }                
}

/// Receives number of bytes from the bus.
///
/// @retval negative - if the function fails.
/// @retval positive - number of bytes received.
static int
pn53x_avr_spi_receive(nfc_device *pnd, uint8_t *pbtData, const size_t szDataLen, const int timeout)
{
    assert(pnd != NULL);
    assert(pbtData != NULL);

    if (!p70_irq_wait(INT_MAX))
    {
        //printf("%s(%d): Timeout waiting for p70_irq\n", __FILE__, __LINE__);
        return NFC_ETIMEOUT;
    }        
    p70_irq_reset();
    
    // TODO: respect the timeout
    avr_spi_begin_transaction(pnd->driver_data, &pn532_avr_spi_selector);
        uint8_t rxBuffer[255] = {0};
        uint8_t bytesRead = 0;
        const ReturnValue res = read_data(pnd, rxBuffer, sizeof(rxBuffer), &bytesRead);
    avr_spi_end_transaction(pnd->driver_data);
    
    if (res == Success)
    {
        memcpy(pbtData, rxBuffer+6, szDataLen);
        //printf("%s(%d): Received %d bytes: ", __FILE__, __LINE__, bytesRead);
        //dump_buf(pbtData, min(szDataLen, bytesRead));
        //puts("");
        return szDataLen;
    }
    else
    {        
        return NFC_EIO;
    }        
}

static int
pn53x_avr_spi_handshake(nfc_device *pnd)
{
    assert(pnd != NULL);
    assert(pnd->driver_data != NULL);
    
    p70_irq_reset();
    avr_spi_begin_transaction(DRIVER_DATA(pnd), &pn532_avr_spi_selector);
    {
        for (int i = 0; i < 100; ++i) { _delay_ms(10); }
        const uint8_t cmd[] = {
            PN532_SPI_DATAWRITE, // indicate that we're going to send data
            0x00, // preamble
            0x00, 0xFF, // start code
            0x02, // length
            0x100-0x02, // length checksum
            0xD4, // host to PN
            GetFirmwareVersion, // command
            0x100-0xD4-GetFirmwareVersion, // data checksum
            0x00 // post amble
        };

        avr_spi_send(pnd->driver_data, cmd, sizeof(cmd), 1000);
    }        
    avr_spi_end_transaction(pnd->driver_data);
    
    if (!p70_irq_wait(INT_MAX))
    {
        printf("%s(%d): Timeout waiting for p70_irq\n", __FILE__, __LINE__);
        assert(false);
        return NFC_ETIMEOUT;
    }        

    p70_irq_reset();
    avr_spi_begin_transaction(pnd->driver_data, &pn532_avr_spi_selector);
    {
        //printf("p70_irq=%d\n", p70_irq_pin());
            
        uint8_t ack_buf[6];
        avr_spi_transceive_byte(pnd->driver_data, PN532_SPI_DATAREAD);
        avr_spi_receive(pnd->driver_data, ack_buf, sizeof(ack_buf), NULL, 1000);
        dump_buf(ack_buf, sizeof(ack_buf));
        if (memcmp(ack_buf, pn53x_ack_frame, sizeof(ack_buf)) == 0)
        {
            //printf("%s(%d): Got ACK frame!\n", __FILE__, __LINE__);
        }                
        else if (memcmp(ack_buf, pn53x_nack_frame, sizeof(ack_buf)) == 0)
        {
            //printf("%s(%d): Got NACK frame!\n", __FILE__, __LINE__);
            return NFC_EIO;
        }                
        else
        {
            //printf("%s(%d): Got some crap\n", __FILE__, __LINE__);
            return NFC_EIO;
        }                
    }
    avr_spi_end_transaction(pnd->driver_data);
    
    if (!p70_irq_wait(INT_MAX))
    {
        //printf("%s(%d): Timeout waiting for p70 irq:", __FILE__, __LINE__);
        return NFC_ETIMEOUT;
    }        
    p70_irq_reset();
    avr_spi_begin_transaction(pnd->driver_data, &pn532_avr_spi_selector);
        //printf("\n%s(%d): Reading response:", __FILE__, __LINE__);
        uint8_t ver_buf[32];
        avr_spi_transceive_byte(pnd->driver_data, PN532_SPI_DATAREAD);
        avr_spi_receive(pnd->driver_data, ver_buf, sizeof(ver_buf), NULL, 1000);
        //dump_buf(ver_buf, sizeof(ver_buf));
        //printf("%c", '\n');
    avr_spi_end_transaction(pnd->driver_data);

    printf("Handshake success!!!\n");
    return NFC_SUCCESS;
}

static int
pn53x_avr_spi_abort_command(nfc_device *pnd)
{
    //DRIVER_DATA(pnd)->abort_flag = true;
    return NFC_SUCCESS;
}

static const struct pn53x_io pn53x_avr_spi_io =
{
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
