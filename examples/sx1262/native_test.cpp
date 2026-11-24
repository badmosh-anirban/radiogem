#include <RadioLib.h>
#include "PiHal.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <csignal>

static volatile bool running = true;

static void sigHandler(int signum) {
    (void)signum;
    running = false;
}

int main(int argc, char** argv) {
    signal(SIGINT, sigHandler);
    signal(SIGTERM, sigHandler);

    bool rxMode = false;
    uint8_t spiChannel = 1;
    uint32_t nssPin = 7;
    uint32_t dio1Pin = 27;
    uint32_t rstPin = 22;
    uint32_t busyPin = 24;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--rx") == 0) {
            rxMode = true;
        } else if (strcmp(argv[i], "--tx") == 0) {
            rxMode = false;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("Usage: %s [--tx|--rx] [spi_channel] [nss] [dio1] [rst] [busy]\n", argv[0]);
            printf("Defaults: channel=%u nss=%u dio1=%u rst=%u busy=%u\n",
                   spiChannel, nssPin, dio1Pin, rstPin, busyPin);
            return 0;
        }
    }

    printf("=========================================\n");
    printf(" RadioLib SX1262 Native C++ Test for RPi\n");
    printf(" Mode: %s\n", rxMode ? "RECEIVER (RX)" : "TRANSMITTER (TX)");
    printf(" Pins: SPI_CH=%u NSS=%u DIO1=%u RST=%u BUSY=%u\n",
           spiChannel, nssPin, dio1Pin, rstPin, busyPin);
    printf(" Radio: 866.5 MHz | 125 kHz | SF7 | CR 4/5 | 10 dBm\n");
    printf("=========================================\n");

    bool isHwCs = (spiChannel == 1 && nssPin == 7) || (spiChannel == 0 && nssPin == 8);
    uint32_t modNss = isHwCs ? RADIOLIB_NC : nssPin;

    PiHal* hal = new PiHal(spiChannel);
    Module* mod = new Module(hal, modNss, dio1Pin, rstPin, busyPin);
    SX1262 radio(mod);

    printf("[SX1262] Initializing ... ");
    fflush(stdout);

    // 866.5 MHz, 125 kHz BW, SF7, CR 5 (4/5), private sync word 0x12, 10 dBm, preamble 8
    int16_t state = radio.begin(866.5f, 125.0f, 7, 5, RADIOLIB_SX126X_SYNC_WORD_PRIVATE, 10, 8, 1.6f, false);
    if (state != RADIOLIB_ERR_NONE) {
        printf("FAILED! (code %d)\n", state);
        if (state == RADIOLIB_ERR_CHIP_NOT_FOUND) {
            printf("Tip: Chip not found (-2). Check SPI bus, NSS (CE) pin, and 3.3V power.\n");
        } else if (state == RADIOLIB_ERR_SPI_CMD_TIMEOUT || state == RADIOLIB_ERR_SPI_CMD_INVALID) {
            printf("Tip: SPI command error (%d). If your module uses an XTAL instead of TCXO, set TCXO voltage to 0.\n", state);
        }
        delete mod;
        delete hal;
        return 1;
    }
    printf("SUCCESS!\n");

    int counter = 0;
    while (running) {
        if (!rxMode) {
            char payload[64];
            snprintf(payload, sizeof(payload), "Hello from Pi C++ #%d", counter++);
            printf("[TX] Sending: \"%s\" ... ", payload);
            fflush(stdout);

            state = radio.transmit(payload);
            if (state == RADIOLIB_ERR_NONE) {
                printf("done!\n");
            } else {
                printf("FAILED (code %d)\n", state);
            }
            hal->delay(1500);
        } else {
            printf("[RX] Listening for packets (timeout 5s) ... ");
            fflush(stdout);

            uint8_t buffer[256] = { 0 };
            state = radio.receive(buffer, 0, 5000);
            if (state == RADIOLIB_ERR_NONE) {
                size_t len = radio.getPacketLength(false);
                printf("Received %zu bytes!\n", len);
                printf("  Payload: \"%.*s\"\n", (int)len, (char*)buffer);
                printf("  RSSI:    %.1f dBm\n", radio.getRSSI());
                printf("  SNR:     %.1f dB\n", radio.getSNR());
            } else if (state == RADIOLIB_ERR_RX_TIMEOUT) {
                printf("timeout (no packet)\n");
            } else {
                printf("error (%d)\n", state);
            }
        }
    }

    printf("\nShutting down...\n");
    delete mod;
    delete hal;
    return 0;
}
