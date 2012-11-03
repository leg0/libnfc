#include <avr/io.h>
#include <inttypes.h>
#include <assert.h>

#include "avr_spi.h"


// Valid values for SPI_BITORDER
#define SPI_LSBFIRST 1
#define SPI_MSBFIRST 0


#if (defined(__AVR_AT90USB82__) || defined(__AVR_AT90USB162__))
#  define SPI_DDRx     DDRB
#  define SPI_SS_PIN   PORTB0
#  define SPI_SCK_PIN  PORTB1
#  define SPI_MOSI_PIN PORTB2
#  define SPI_MISO_PIN PORTB3
#elif (defined(__AVR_ATmega48__) || defined(_AVR_ATmega88__) || defined(__AVR_ATmega168__) || defined(__AVR_ATmega328__) || defined(__AVR_ATmega328P__))
#  define SPI_DDRx     DDRB
#  define SPI_SS_PIN   PORTB2
#  define SPI_SCK_PIN  PORTB5
#  define SPI_MOSI_PIN PORTB3
#  define SPI_MISO_PIN PORTB4
#else
#  error unknown processor - add to spi.h
#endif


static void avr_spi_setup()
{
    // specify pin directions for SPI pins on port B
    SPI_DDRx |= (1<<SPI_MOSI_PIN)
             |  (1<<SPI_SCK_PIN)
             |  (1<<SPI_SS_PIN); // output
    SPI_DDRx &= ~(1<<SPI_MISO_PIN); // input

    SPCR = (1<<SPIE) // interrupt enabled
         | (1<<SPE) // enable SPI
         | (SPI_BITORDER << DORD) // LSB or MSB
         | (1 <<MSTR) // Master
         | (SPI_CPOL << CPOL)
         | (SPI_CPHA << CPHA)
         | (SPI_SPR1 << SPR1)
         | (SPI_SPR0 << SPR0);
    SPSR = (SPI_SPI2X << SPI2X);
}

static void avr_spi_disable()
{
    SPCR = 0;
}

static uint8_t avr_spi_transceive_byte(uint8_t out)
{
    SPDR = out;
    while (!(SPSR & (1<<SPIF)));
    return SPDR;
}

static uint8_t avr_spi_last_received(uint8_t data)
{
    SPDR = data;
    return SPDR;
}


typedef struct avr_spi
{
	int isOpen;
} avr_spi;

avr_spi port = 
{
	.isOpen = 0
};

avr_spi_handle avr_spi_open(const char * pcPortName)
{
	if (port.isOpen) return NULL;

	avr_spi_setup();
	return &port;
}

void    avr_spi_close(avr_spi_handle h)
{
	assert(h == &port);
	assert(port.isOpen);

	avr_spi_disable();

	port.isOpen = 0;
}

void    avr_spi_flush_input(avr_spi_handle h)
{
	assert(h == &port);
	assert(port.isOpen);
}

void    avr_spi_set_speed(avr_spi_handle h, const uint32_t uiPortSpeed)
{
	assert(h == &port);
	assert(port.isOpen);
}

uint32_t avr_spi_get_speed(avr_spi_handle h)
{
	assert(h == &port);
	assert(port.isOpen);
	return ~0;
}

int     avr_spi_receive(avr_spi_handle h, uint8_t* pbtRx, const size_t szRx, void* abort_p, int timeout)
{
	assert(h == &port);
	assert(port.isOpen);

	for (size_t i = 0; i < szRx; ++i)
	{
		pbtRx[i] = avr_spi_transceive_byte(0xff);
	}
}

int     avr_spi_send(avr_spi_handle h, const uint8_t* pbtTx, const size_t szTx, int timeout)
{
	assert(h == &port);
	assert(port.isOpen);

	for (size_t i = 0; i < szTx; ++i)
	{
		avr_spi_transceive_byte(pbtTx[i]);
	}
}

char** avr_spi_list_ports(void)
{
	static char p[] = "spi";
	static char* q = p;
	return &q;
}

