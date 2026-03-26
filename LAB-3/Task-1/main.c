#include <xc.h>

#define _XTAL_FREQ 4000000    // 4MHz Crystal Frequency

// Global Variables ---
unsigned int overflow_count = 0; // Shared variable to track timer overflows

// --- Interrupt Service Routine (ISR) ---
void __interrupt() ISR(void) {
    // Check if Timer0 Interrupt Flag (TMR0IF) is set
    if (INTCONbits.TMR0IF) {

        /* RELOAD MATH:
        * To get a 50ms interval at 4MHz with 1:256 prescaler:
        1. Instruction Clock = 4MHz / 4 = 1MHz (1 tick = 1us)
        2. Prescaler 1:256 means timer ticks every 256us.
        3. 50ms / 256us = 195 ticks.
        4. Preload = 256 - 195 = 61.
        */

        TMR0 = 61;    // Reload the timer for consistent 50ms intervals

        overflow_count++;    // Increment the software counter

        // 50ms * 20 = 1000ms (1 Second)
        if (overflow_count >= 20) {
            PORTBbits.RB0 = ~PORTBbits.RB0; // Toggle Background LED
            overflow_count = 0;    // Reset software counter
        }

        INTCONbits.TMR0IF = 0;    // CLEAR the flag to allow future interrupts
    }
}

int main(void) {
    // 1. I/O Initialization
    TRISBbits.TRISB0 = 0;    // Set RB0 as output (Background LED)
    TRISDbits.TRISD0 = 0;    // Set RD0 as output (Foreground LED)

    PORTB = 0x00;    // Turn off all LEDs initially
    PORTD = 0x00;

    // 2. Timer0 Configuration (OPTION_REG)
    OPTION_REGbits.T0CS = 0;    // Select Internal Instruction Clock (Timer Mode)
    OPTION_REGbits.PSA = 0;      // Assign Prescaler to Timer0 (not WDT)
    OPTION_REGbits.PS = 0b111;   // Set Prescaler to 1:256

    TMR0 = 61;    // Initial preload for 50ms

    // 3. Interrupt Configuration (INTCON)
    INTCONbits.TMR0IE = 1;    // Enable Timer0 Overflow Interrupt
    INTCONbits.GIE = 1;       // Enable Global Interrupts (Master Switch)

    while(1) {
        /* FOREGROUND TASK:
        * This LED blinks independently. Even though we use __delay_ms here,
        * the Timer0 hardware continues counting in the background.
        */

        PORTDbits.RD0 = 1;
        __delay_ms(100);
        PORTDbits.RD0 = 0;
        __delay_ms(100);
    }
}