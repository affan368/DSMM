
#include <stdint.h>
#include <xc.h>

#define _XTAL_FREQ 4000000
#pragma config FOSC = XT, WDTE = OFF, LVP = OFF, BOREN = ON

uint16_t pulse_width = 0;
unsigned char edge_state = 0; // 0 = wait for press (falling), 1 = wait for release (rising)

void __interrupt() ISR(void) {
    if (INTCONbits.INTF) {
        if (edge_state == 0) {
            // --- FALLING EDGE (Button Pressed) ---
            T1CONbits.TMR1ON = 0;       // Ensure timer is off before modifying
            TMR1H = 0; TMR1L = 0;       // Reset timer registers
            PIR1bits.TMR1IF = 0;        // Clear hardware overflow flag (Crucial for avoiding the trap)
            
            T1CONbits.TMR1ON = 1;       // Start Timer
            OPTION_REGbits.INTEDG = 1;  // Reconfigure interrupt to trigger on Release
            edge_state = 1;             
        } else {
            // --- RISING EDGE (Button Released) ---
            T1CONbits.TMR1ON = 0;       // Stop Timer
            pulse_width = (uint16_t)((TMR1H << 8) | TMR1L); 
            
            /* TIMING LOGIC:
             * Target: > 500 ms. At 8us/tick, 500ms => 62,500 ticks.
             * Max Timer Capacity = 524 ms (65,535 ticks).
             * If TMR1IF == 1, the timer rolled over (exceeded 524 ms).
             * If TMR1IF == 0, check if the value is between 500 ms and 524 ms.
             */
            if (PIR1bits.TMR1IF == 1 || pulse_width > 62500) {
                PORTDbits.RD0 = 1;      // Long press confirmed
            } else {
                PORTDbits.RD0 = 0;      // Short press
            }

            OPTION_REGbits.INTEDG = 0;  // Reconfigure interrupt to trigger on Press
            edge_state = 0;             
        }
        INTCONbits.INTF = 0;            // Clear RB0 Interrupt Flag
    }
}

int main(void) {
    TRISBbits.TRISB0 = 1;       // RB0 as Input
    TRISDbits.TRISD0 = 0;       // RD0 as Output
    PORTDbits.RD0 = 0;          

    T1CONbits.T1CKPS = 0b11;    // Timer1 Prescaler = 1:8 (1 tick = 8 us at 4MHz)
    T1CONbits.TMR1CS = 0;       // Internal Instruction Clock

    OPTION_REGbits.INTEDG = 0;  // Initial state: trigger on falling edge
    INTCONbits.INTE = 1;        // Enable RB0 External Interrupt
    INTCONbits.GIE = 1;         // Enable Global Interrupts

    while(1) {
        // CPU remains free for background scheduling
    }
}