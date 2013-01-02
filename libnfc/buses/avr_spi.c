#include "avr_spi.h"

#include <nfc/nfc.h>

#include <avr/io.h>
#include <inttypes.h>
#include <assert.h>

// Valid values for SPI_BITORDER
#define SPI_LSBFIRST 1
#define SPI_MSBFIRST 0

#ifndef SPI_CPOL
#  define SPI_CPOL 0
#endif

#ifndef SPI_CPHA
#  define SPI_CPHA 0
#endif

// Important: When selecting values for SPR0, SPR1 and SPI2X, make sure that the
// SPI frequency is within the range of PN53x. PN532 User Manual p6.2.5 "SPI
// communication details" says that PN532 is able to communicate at up to 5MHz.
#ifndef SPI_SPR1
#  define SPI_SPR1 0
#endif

#ifndef SPI_SPR0
#  define SPI_SPR0 0
#endif

#ifndef SPI_SPI2X
#  define SPI_SPI2X 0
#endif

#ifndef SPI_BITORDER
#  define SPI_BITORDER SPI_LSBFIRST
#endif

#if defined(__AVR_AT90USB82__) \
        || defined(__AVR_AT90USB162__) \
        || defined(__AVR_ATmega2560__)
#  define SPI_DDRx     DDRB
#  define SPI_PORT     PORTB
#  define SPI_SS_PIN   PORTB0
#  define SPI_SCK_PIN  PORTB1
#  define SPI_MOSI_PIN PORTB2
#  define SPI_MISO_PIN PORTB3
#elif defined(__AVR_ATmega48__) \
        || defined(_AVR_ATmega88__) \
        || defined(__AVR_ATmega168__) \
        || defined(__AVR_ATmega328__) \
        || defined(__AVR_ATmega328P__)
#  define SPI_DDRx     DDRB
#  define SPI_PORT     PORTB
#  define SPI_SS_PIN   PORTB2
#  define SPI_SCK_PIN  PORTB5
#  define SPI_MOSI_PIN PORTB3
#  define SPI_MISO_PIN PORTB4
#else
    // This is either not an AVR, or is an AVR that is not yet supported.
#  error Unknown processor or not an AVR.
#endif


static void avr_spi_setup()
{  
    // specify pin directions for SPI pins on port B
    SPI_DDRx |=  _BV(SPI_MOSI_PIN)  // output
             |   _BV(SPI_SCK_PIN)   // output
             |   _BV(SPI_SS_PIN);   // output
    SPI_DDRx &= ~_BV(SPI_MISO_PIN); // input

    SPI_PORT &= ~_BV(SPI_SCK_PIN);  // low
    SPI_PORT &= ~_BV(SPI_MOSI_PIN); // low
    SPI_PORT |=  _BV(SPI_SS_PIN);   // high
    
//    SPCR = 0 // (1<<SPIE) // interrupt not enabled
//         | _BV(SPE) // enable SPI
//         | (SPI_BITORDER << DORD) // LSB or MSB
//         | _BV(MSTR) // Master
//         | (SPI_CPOL << CPOL)
//         | (SPI_CPHA << CPHA)
//         | (SPI_SPR1 << SPR1)
//         | (SPI_SPR0 << SPR0);
//    SPSR = (SPI_SPI2X << SPI2X);
    
    uint8_t const SPI_MODE0 = 0x00;
    uint8_t const SPI_MODE_MASK = 0x0C;
    uint8_t const SPI_CLOCK_MASK = 0x03;
    uint8_t const SPI_CLOCK_DIV4 = 0x00;
    uint8_t const SPI_2XCLOCK_MASK = 0x01;
    
    SPCR = _BV(MSTR) | _BV(DORD) | _BV(SPE) | SPI_MODE0 | (SPI_CLOCK_DIV4 & SPI_CLOCK_MASK);
    SPSR  = (SPSR & ~SPI_2XCLOCK_MASK) | ((SPI_CLOCK_DIV4 >> 2) & SPI_2XCLOCK_MASK);
}

static void avr_spi_disable()
{
    SPCR &= ~_BV(SPE);
}

typedef struct avr_spi
{
    bool isOpen;
    
    /// When no device on the bus is selected, this field is NULL, otherwise it points
    /// to a selector object that provides functions to select/deselect the device.
    avr_spi_selector const* sel;
} avr_spi;

avr_spi port =
{
    .isOpen = false,
    .sel = NULL
};

avr_spi_handle avr_spi_open(const char * name)
{
    if (port.isOpen) return NULL;

    port.isOpen = true;
    port.sel = NULL;
    avr_spi_setup();
    return &port;
}

void avr_spi_close(avr_spi_handle h)
{
    assert(h == &port);
    assert(port.isOpen);

    avr_spi_disable();

    port.isOpen = false;
}

void avr_spi_begin_transaction(avr_spi_handle h, avr_spi_selector const* sel)
{
    assert(h == &port);
    assert(h->isOpen);
    assert(h->sel == NULL);
    assert(sel != NULL);

    h->sel = sel;
    sel->select();
}

void avr_spi_end_transaction(avr_spi_handle h)
{
    assert(h == &port);
    assert(h->isOpen);
    assert(h->sel != NULL);

    h->sel->deselect();
    h->sel = NULL;
}

uint8_t avr_spi_transceive_byte(avr_spi_handle h, uint8_t out)
{
    assert(h == &port);
    assert(h->isOpen);
    assert(h->sel != NULL);
    
    SPDR = out;
    while (!(SPSR & (1<<SPIF)));
    return SPDR;
}

int avr_spi_receive(avr_spi_handle h, uint8_t* pbtRx, const size_t szRx, void* abort_p, int timeout)
{
    assert(h == &port);
    assert(h->isOpen);
    assert(h->sel != NULL);

    for (size_t i = 0; i < szRx; ++i)
    {
        const uint8_t PN532_SPI_DATAREAD = 0x03;
        pbtRx[i] = avr_spi_transceive_byte(h, PN532_SPI_DATAREAD);
    }

    return szRx;
}

int avr_spi_send(avr_spi_handle h, const uint8_t* pbtTx, const size_t szTx, int timeout)
{
    assert(h == &port);
    assert(h->isOpen);
    assert(h->sel != NULL);
    assert(pbtTx != NULL);

    for (size_t i = 0; i < szTx; ++i)
    {
        avr_spi_transceive_byte(h, pbtTx[i]);
    }
    printf("%c", '\n');

    return szTx;
}

char** avr_spi_list_ports(void)
{
    static char p[] = "spi";
    static char* q = p;
    return &q;
}
