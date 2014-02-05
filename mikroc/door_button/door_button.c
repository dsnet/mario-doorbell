// Copyright 2009, Joe Tsai. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE.md file.

/*
Project name:
    Mario Doorbell Project - Door button
Description:
    This firmware code implements the button for the doorbell. The idea is to
    have a button that, when pressed, increments a two digit 7-Segment display.
    Normally, for every press, the Mario coin sound will be played as the
    doorbell ringer. For every 10 presses, the 1-Up sound will be played, and
    for every 100 presses, the mushroom power-up sound will be played.
    The signal to determine which sound to be played is sent using USART.
    Since this component is placed outside the door as the button interface, as
    few components as possible were used to reduce cost in the unfortunate event
    that someone stole the doorbell button.
Configuration:
    Microcontroller:   PIC16F628A
    Oscillator:        INT_RC, 4.00 MHz
    External modules:  7-Segment Display
    Compiler:          MikroC 8.0
Notes:
    The reason why hardware USART was not used is because the sound generator
    that this device connects to was originally a PIC16F88 which could not
    support SPI and USART simultaneously. Thus, USART was not originally the
    planned method of communication between the two. In a freak accident, all
    the PIC16F88 chips I had were destroyed, so I had to settle with a different
    chip that happened to support both USART and SPI together. However, by this
    time, the PCB for this part had already been laid out and made.
*/


/* Global constants */
const unsigned short LO_SEGMENT[10] = {
    0x02, 0x8F, 0x44, 0x50, 0x98, 0x11, 0x01, 0x5A, 0x00, 0x10,
};
const unsigned short HI_SEGMENT[10] = {
    0xDF, 0xDA, 0x44, 0x50, 0x98, 0x11, 0x01, 0x5A, 0x00, 0x10,
};
enum sound { COIN, COIN_1UP, COIN_MUSHROOM, ITS_MARIO, OUTTA_TIME, DOWN_PIPE };


/* Global variables */
unsigned short ring_type;
unsigned short lo_num;
unsigned short hi_num;
unsigned short toggle;
unsigned short press;


// Interrupt vector
void interrupt() {
    // If timer timeout interrupt
    if (INTCON.T0IF) {
        // Toggle between the digits on the 7-segment display
        if (toggle == 0) {
            PORTB.F1 = 1; // Disable PMOS
            PORTB.F2 = 0; // Enable PMOS
            PORTA = LO_SEGMENT[lo_num];
            toggle = 1;
        } else {
            PORTB.F2 = 1; // Disable PMOS
            PORTB.F1 = 0; // Enable PMOS
            PORTA = HI_SEGMENT[hi_num];
            toggle = 0;
        }

        INTCON.T0IE = 1; // Enable timer interrupt
        INTCON.T0IF = 0; // Clear timer interrupt flag
    }

    // If external interrupt
    if (INTCON.INTF) {
        press = 1; // Set press flag

        INTCON.INTE = 0; // Disable external interrupt
        INTCON.INTF = 0; // Clear external interrupt flag
    }
}


// Main routine
void main() {
    // Define settings
    PORTA = 0x00;
    PORTB = 0x00;
    TRISA = 0x00;
    TRISB = 0x21;
    CMCON = 0x07; //Disable analog comparators
    OPTION_REG = 0x04;

    // Initialize variables
    hi_num = 0;
    lo_num = 0;
    toggle = 0;
    press = 0;

    // Initialize software UART
    soft_uart_init(PORTB, 5, 4, 9615, 0);

    // Set up interrupts
    INTCON.T0IE = 1;
    INTCON.T0IF = 0;
    INTCON.PEIE = 1;
    INTCON.INTE = 1;
    INTCON.GIE  = 1;

    // Continue forever
    while (1) {
        // Poll for the press flag
        if (press) {
            INTCON.INTE = 0; // Disable external interrupt
            press = 0; // Clear the press flag

            // Make sure the button is still pressed and not a spurious bounce
            delay_ms(25);
            if (PORTB.F0 == 0) {
                // Increment
                lo_num++;
                if (lo_num == 10) {
                    lo_num = 0;

                    hi_num++;
                    if (hi_num == 10) {
                        hi_num = 0;
                    }
                }

                // Determine ring type
                if (lo_num == 0) {
                    if (hi_num == 0) {
                        ring_type = COIN_MUSHROOM;
                    } else {
                        ring_type = COIN_1UP;
                    }
                } else {
                    ring_type = COIN;
                }

                // Send software UART signal
                INTCON.GIE = 0;
                soft_uart_write(ring_type);
                INTCON.GIE = 1;

                // Set delay until next allowable button press
                switch (ring_type) {
                case COIN:          delay_ms(125); break;
                case COIN_1UP:      delay_ms(675); break;
                case COIN_MUSHROOM: delay_ms(875); break;
                }
            }

            // Re-enable external interrupts
            INTCON.INTF = 0; // Clear external interrupt flag
            INTCON.INTE = 1; // Enable external interrupt
        }
    }
}
