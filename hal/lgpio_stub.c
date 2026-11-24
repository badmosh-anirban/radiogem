/*
 * lgpio_stub.c — Build-time-only linker stub for liblgpio.
 *
 * We do NOT #include <lgpio.h> here — lgpio.h pulls in <linux/gpio.h> which
 * has incompatible GPIO v2 structs on the old kernel headers inside the
 * manylinux_2_28 image.  Instead we declare just the symbols that radiogem
 * actually links against as plain C functions with no header dependency.
 *
 * At runtime on the Raspberry Pi, the real liblgpio (apt install liblgpio-dev)
 * is loaded via dlopen; this stub is never shipped to the Pi.
 */
#include <stdint.h>

/* Suppress "no previous declaration" warnings */
#pragma GCC diagnostic ignored "-Wmissing-declarations"

/* ── GPIO chip ─────────────────────────────────────────────── */
int lgGpiochipOpen(int d)                              { return -1; }
int lgGpiochipClose(int h)                             { return  0; }

/* ── GPIO pin control ──────────────────────────────────────── */
int lgGpioClaimInput(int h, int f, int g)              { return  0; }
int lgGpioClaimOutput(int h, int f, int g, int v)      { return  0; }
int lgGpioClaimAlert(int h, int f, int t, int g, int nb){ return 0; }
int lgGpioFree(int h, int g)                           { return  0; }
int lgGpioRead(int h, int g)                           { return  0; }
int lgGpioWrite(int h, int g, int v)                   { return  0; }
int lgGpioGetMode(int h, int g)                        { return  0; }
int lgGpioSetDebounce(int h, int g, int us)            { return  0; }
int lgGpioSetWatchdog(int h, int g, int us)            { return  0; }
int lgGpioSetBannedState(int h, int g, int b)          { return  0; }

/* ── GPIO alerts / callbacks ───────────────────────────────── */
/* lgGpioAlertsFunc_t = void (*)(int, void*, void*) — declare inline */
typedef void (*lgGpioAlertsFunc_t)(int, void*, void*);
int  lgGpioSetAlertsFunc(int h, int g, lgGpioAlertsFunc_t f, void *u){ return 0; }
void lgGpioSetSamplesFunc(lgGpioAlertsFunc_t f, void *u)             {          }

/* ── SPI ───────────────────────────────────────────────────── */
int lgSpiOpen(int dev, int chan, int baud, int flags)  { return -1; }
int lgSpiClose(int h)                                  { return  0; }
int lgSpiRead(int h, char *rx, int c)                  { return  0; }
int lgSpiWrite(int h, const char *tx, int c)           { return  0; }
int lgSpiXfer(int h, const char *tx, char *rx, int c)  { return  0; }

/* ── Misc ──────────────────────────────────────────────────── */
const char *lgErrorText(int e)                         { return ""; }
int  lgVersion(void)                                   { return  0; }
