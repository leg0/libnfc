
# set the include path found by configure
AM_CPPFLAGS = $(all_includes) $(LIBNFC_CFLAGS)

noinst_LTLIBRARIES = libnfcbuses.la
libnfcbuses_la_SOURCES = uart.c uart.h
libnfcbuses_la_CFLAGS = -I$(top_srcdir)/libnfc

EXTRA_DIST = uart_posix.c uart_win32.c

