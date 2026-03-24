#include <xc.h>
#include <stdint.h>

#define _XTAL_FREQ 4000000

// ---------------- GLOBAL VARIABLES ----------------

// Capture values
volatile uint16_t previous_capture = 0;
volatile uint16_t current_capture = 0;

// Overflow counter (counts Timer1 rollovers)
volatile uint16_t overflow_count = 0;

// Final 32-bit ticks
volatile uint32_t total_ticks = 0;

// Flag to indicate new measurement
volatile unsigned char new_data_ready = 0;


// ---------------- INTERRUPT SERVICE ROUTINE ----------------
void __interrupt() ISR()
{
    // ---------- TIMER1 OVERFLOW ----------
    if (PIR1bits.TMR1IF)
    {
        overflow_count++;        // count overflow
        PIR1bits.TMR1IF = 0;     // clear flag
    }

    // ---------- CCP1 CAPTURE ----------
    if (PIR1bits.CCP1IF)
    {
        // Read captured value
        current_capture = ((uint16_t)CCPR1H << 8) | CCPR1L;

        uint16_t delta;

        // -------- 16-bit subtraction --------
        delta = current_capture - previous_capture;

        // -------- 32-bit correction logic --------
        if (current_capture < previous_capture)
        {
            // one overflow already handled in subtraction
            total_ticks = ((uint32_t)(overflow_count - 1) * 65536UL) + delta;
        }
        else
        {
            total_ticks = ((uint32_t)overflow_count * 65536UL) + delta;
        }

        // Reset for next measurement
        overflow_count = 0;
        previous_capture = current_capture;

        new_data_ready = 1;      // signal main loop

        PIR1bits.CCP1IF = 0;     // clear flag
    }
}


// ---------------- MAIN FUNCTION ----------------
void main(void)
{
    // -------- PORT CONFIG --------
    TRISC2 = 1;     // CCP1 input
    TRISD0 = 0;     // LED output

    RD0 = 0;

    // -------- TIMER1 CONFIG --------
    T1CON = 0x01;   // Timer1 ON, prescaler 1:1

    // -------- CCP1 CONFIG --------
    CCP1CON = 0x05; // Capture mode, every rising edge

    // -------- INTERRUPTS --------
    PIE1bits.TMR1IE = 1;   // enable Timer1 overflow interrupt
    PIE1bits.CCP1IE = 1;   // enable CCP1 interrupt

    INTCONbits.PEIE = 1;   // peripheral interrupt enable
    INTCONbits.GIE = 1;    // global interrupt enable

    // -------- MAIN LOOP --------
    while (1)
    {
        if (new_data_ready)
        {
            new_data_ready = 0;

            // -------- 10Hz CHECK --------
            // Expected = 100,000 ticks
            if (total_ticks >= 99900 && total_ticks <= 100100)
            {
                RD0 = 1;   // LED ON (frequency lock)
            }
            else
            {
                RD0 = 0;   // LED OFF
            }
        }
    }
}