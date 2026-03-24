
#include <stdint.h>
#include <xc.h>

#define _XTAL_FREQ 4000000
#pragma config FOSC = XT, WDTE = OFF, LVP = OFF, BOREN = ON

// Global variables for timestamp tracking
volatile uint16_t current_capture = 0;
volatile uint16_t previous_capture = 0;
volatile uint8_t overflow_count = 0;  // Counts Timer1 overflows between captures
volatile uint32_t total_ticks = 0;    // In case of overflow, total ticks exceed 65536 (16 bits) 
volatile uint8_t capture_flag = 0;    // uint8_t is equivalent to unsigned char, but more explicit about size

/*
In embedded C (especially with interrupts on PIC), volatile is a keyword that tells the compiler:
 "This variable can change unexpectedly (e.g., by hardware or interrupts), so don't optimize it away."
  Without it, the compiler might assume the variable never changes between accesses, leading to bugs 
  like stale data or infinite loops.

Best Practices
Always use volatile for variables shared between ISRs and main code.
For PIC: It's essential due to the 8-bit architecture and interrupt-driven design.
Alternatives: If not shared with interrupts, omit volatile to allow optimizations.
*/

void __interrupt() ISR(void) {
    if (PIR1bits.TMR1IF) {
        // Track Timer1 overflows so we can measure periods > 65535 ticks
        overflow_count++;
        PIR1bits.TMR1IF = 0; // Clear overflow flag
    }

    if (PIR1bits.CCP1IF) {
        // Read 16-bit captured value
        current_capture = (uint16_t)((CCPR1H << 8) | CCPR1L);

        // Calculate elapsed ticks, accounting for timer rollovers.
        // Apply conditional logic for 32-bit Total_Ticks (>65535):
        // - Condition A (Current < Previous): The 16-bit subtraction already utilized one overflow lap.
        //   Subtract 1 from overflow_count to prevent double-counting.
        // - Condition B (Current >= Previous): No rollover was consumed; use overflow_count as is.
        uint32_t delta = (uint16_t)(current_capture - previous_capture);
        if (current_capture < previous_capture) {
            // Condition A: Subtraction used one lap
            total_ticks = (uint32_t)(overflow_count - 1) * 65536UL + delta;
        } else {
            // Condition B: No lap used in subtraction
            total_ticks = (uint32_t)overflow_count * 65536UL + delta;
        }
        overflow_count = 0;  // Reset for next period

        previous_capture = current_capture;

        // Notify main loop
        capture_flag = 1;
        PIR1bits.CCP1IF = 0; // Clear flag
    }
}

int main(void) {
    // I/O Configuration
    TRISCbits.TRISC2 = 1;       // RC2/CCP1 as INPUT for Capture
    TRISDbits.TRISD0 = 0;       // RD0 as OUTPUT for LED
    PORTDbits.RD0 = 0;          // Initialize LED to OFF
    
    // Timer1 Configuration (1:1 Prescaler)
    T1CONbits.T1CKPS = 0b00;    
    T1CONbits.TMR1CS = 0;       
    T1CONbits.TMR1ON = 1;       

    // CCP1 Configuration (Capture mode, every rising edge)
    CCP1CON = 0x05;             

    // Interrupt Enabling
    PIR1bits.TMR1IF = 0;        // Clear any existing Timer1 overflow flag
    PIE1bits.TMR1IE = 1;        // Enable Timer1 overflow interrupt
    PIE1bits.CCP1IE = 1;        // CCP1 interrupt enable
    INTCONbits.PEIE = 1;        // Enable peripheral interrupts
    INTCONbits.GIE = 1;         // Enable global interrupts

    while(1) {
        if (capture_flag) {
            
            /* * IDEAL WORLD: 
             * 2 kHz => 500us period. At 1us per tick, period = 500 ticks.
             * if (signal_period == 500) { ... }
             * * REAL WORLD (Engineering Practice):
             * Crystals have tolerances and drift. We should allow a small margin 
             * of error (e.g., +/- 2 ticks, which is +/- 0.4% error margin).
             */
            
            if (total_ticks >= 498 && total_ticks <= 502) {
                PORTDbits.RD0 = 1;  // Turn ON LED (2 kHz match found)
            } else {
                PORTDbits.RD0 = 0;  // Turn OFF LED (Frequency mismatch)
            }
            
            capture_flag = 0; // Reset flag until next capture
        }
    }
}