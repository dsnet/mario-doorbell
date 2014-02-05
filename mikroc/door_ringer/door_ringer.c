// Copyright 2009, Joe Tsai. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE.md file.

/*
Project name:
    Mario Doorbell Project - Door ringer
Description:
    As the counterpart to the doorbell button part of this project, this module
    will play the sound clip requested on the USART. In order to play the
    requested sound file, sound data is first retrieved from a EEPROM chip and
    then sent to a DAC chip. Unless the MCU is overclocked, the maximum sample
    rate that can be played is 22050 samples/second.
Configuration:
    Microcontroller:   PIC16F687
    Oscillator:        HS, 20.00 MHz
    External modules:  MCP4822 DAC, 25LC1024 EEPROM
    Compiler:          MikroC 8.0
Notes:
    A C++ program was written in order to easily convert multiple .wav files
    into a single Intel Hex file ready to load onto the EEPROM chip. Only 8-bit,
    monophonic, non-compressed wav files are playable. In addition, only rates
    of 8000, 11025, and 22050 samples/second are supported.
*/


/* Helper macros */
#define BYTE0(param) ((char *)&param)[0]
#define BYTE1(param) ((char *)&param)[1]
#define BYTE2(param) ((char *)&param)[2]
#define BYTE3(param) ((char *)&param)[3]


/* Global constants */
enum frequency { FREQ_8000, FREQ_11025, FREQ_22050 };
enum sound { COIN, COIN_1UP, COIN_MUSHROOM, ITS_MARIO, OUTTA_TIME, DOWN_PIPE };


/* Global variables */
unsigned short rx_data;
unsigned long wave_scan;


// Interrupt vector
void interrupt() {
    // If there is an external interrupt
    if (INTCON.INTF) {
        delay_ms(25); // Debounce delay

        // Determine the sound to be played (debugging purposes)
        rx_data = 0xFF;
        if (PORTA.F0 && PORTA.F1) {
            rx_data = ITS_MARIO;
        } else if (!PORTA.F0 && PORTA.F1) {
            rx_data = OUTTA_TIME;
        } else if (PORTA.F0 && !PORTA.F1) {
            rx_data = DOWN_PIPE;
        }

        INTCON.INTF = 0; // Clear interrupt flag
    }

    // If there is an unread byte
    if (PIR1.RCIF) {
        rx_data = usart_read();
        wave_scan = 0x80000000; // Stop on-going sounds
    }
}


// Function to play sound from the EEPROM
void play_sound(short rate, unsigned long offset, unsigned long length) {
    unsigned int wave_data;

    // Wake up EEPROM - with power-up delay
    PORTC.F0 = 1;
    PORTC.F0 = 0;
    spi_write(0xAB);
    PORTC.F0 = 1;
    delay_us(100);

    // Setup the EEPROM
    PORTC.F0 = 0;
    PORTC.F2 = 1; // Unhold EEPROM
    spi_write(0x03); // EEPROM read command
    spi_write(BYTE2(offset));
    spi_write(BYTE1(offset));
    spi_write(BYTE0(offset));

    // Process all bytes in sound file
    for (wave_scan = 0x000000; wave_scan < length; wave_scan++) {
        // Retrieve a byte of audio data
        PORTC.F2 = 1; // Unhold EEPROM
        wave_data = (SPI_Read(0x00) << 4); // Read EEPROM byte
        PORTC.F2 = 0; // Hold EEPROM

        // Write audio data to DAC
        PORTC.F1 = 0;
        spi_write(BYTE1(wave_data) | 0x10);
        spi_write(BYTE0(wave_data));
        PORTC.F1 = 1;

        // Set delays for different sampling rates
        switch (rate) {
            case FREQ_8000:  delay_us(0x65); break;
            case FREQ_11025: delay_us(0x41); break;
            case FREQ_22050: delay_us(0x12); break;
        }
    }

    // Set DAC voltage output to normalized level
    PORTC.F1 = 0;
    spi_write(0x18);
    spi_write(0x00);
    PORTC.F1 = 1;

    // Shutdown EEPROM - with power-down delay
    PORTC.F0 = 1;
    PORTC.F0 = 0;
    spi_write(0xB9);
    PORTC.F0 = 1;
    delay_us(100);
}


// Main routine
void main() {
    unsigned short _rx_data;

    // Initiate variables
    rx_data = 0xFF;
    wave_scan = 0xFFFFFFFF;

    // Disable ADC modules
    ANSEL = 0x00;
    ANSELH = 0x00;
    OPTION_REG = 0x40;
    TRISA = 0x07;
    WPUA = 0x07;

    // Setup SPI module
    TRISB = 0x00;
    TRISC = 0x00;
    PORTC.F0 = 1; // nCS for EEPROM
    PORTC.F1 = 1; // nCS for DAC
    PORTC.F2 = 1; // nHold for EEPROM
    TRISB.F6 = 0; // SCK is output
    TRISB.F4 = 1; // SDI is input
    TRISC.F7 = 0; // SDO is output
    spi_init_advanced(
        MASTER_OSC_DIV4, DATA_SAMPLE_MIDDLE, CLK_IDLE_LOW, LOW_2_HIGH
    );

    // Setup USART module with interrupts
    usart_init(9615);
    INTCON.PEIE = 1;
    INTCON.GIE = 1;
    INTCON.INTE = 1;
    PIE1.RCIE = 1;

    // Continue forever
    while (1) {
        _rx_data = rx_data;
        rx_data = 0xFF; // Clear the wave to play
        switch (_rx_data) {
        case COIN:
            play_sound(FREQ_22050, 0x000000, 0x0046BE);
            break;

        case COIN_1UP:
            play_sound(FREQ_22050, 0x000000, 0x000CEC);
            play_sound(FREQ_22050, 0x0046BE, 0x0042F0);
            break;

        case COIN_MUSHROOM:
            play_sound(FREQ_22050, 0x000000, 0x000CEC);
            play_sound(FREQ_22050, 0x0089AE, 0x005053);
            break;

        case ITS_MARIO:
            play_sound(FREQ_11025, 0x00DA01, 0x0050C9);
            break;

        case OUTTA_TIME:
            play_sound(FREQ_11025, 0x012ACA, 0x007DC1);
            break;

        case DOWN_PIPE:
            play_sound(FREQ_22050, 0x01A88B, 0x000FD2); delay_ms(85);
            play_sound(FREQ_22050, 0x01A88B, 0x000FD2); delay_ms(85);
            play_sound(FREQ_22050, 0x01A88B, 0x000FD2); delay_ms(85);
            break;
        }
    }
}
