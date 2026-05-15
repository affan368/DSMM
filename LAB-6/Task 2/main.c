#include <xc.h>
#include <stdint.h>

// Configuration Bits: Standard XT Oscillator, Watchdog Timer Off
#pragma config FOSC = XT, WDTE = OFF, PWRTE = OFF, BOREN = ON, LVP = OFF

#define _XTAL_FREQ 4000000

// Function Prototypes for Modular Design
void System_Init(void);
uint16_t ADC_Read(unsigned char channel);
void PWM_SetDuty(uint16_t duty);

void main(void) {
    uint16_t throttle_value; // 16-bit container for 10-bit data

    System_Init(); // Initialize ADC and PWM Peripherals

    while(1) {
        // Step 1: Capture Throttle Position (0 to 1023)
        throttle_value = ADC_Read(0); // Fixed: removed "channel:"

        /* Step 2: Industrial Safety Clamp
        * Our PWM is set for 4kHz, meaning PR2 = 249.
        * Max PWM Resolution = (PR2 + 1) * 4 = 1000 steps.
        * Since the ADC can go up to 1023, we must clamp values > 1000
        * to prevent duty cycle register overflow/glitches.
        */
        if (throttle_value > 1000) {
            throttle_value = 1000;
        }

        // Step 3: Actuate Motor
        // Send the digitized throttle value directly to the PWM hardware
        PWM_SetDuty(throttle_value); // Fixed: removed "duty:"

        __delay_ms(10); // Loop stability delay
    }
}

void System_Init(void) {
    // ADC Setup
    // ADCON1: Bit 7=1 (Right Justified), Bits 3:0=0000 (All Pins Analog)
    ADCON1 = 0b10000000;
    // ADCON0: Bits 7:6=01 (Fosc/8), Bit 0=1 (ADC Module Power ON)
    ADCON0 = 0b01000001;

    // PWM Setup (Reference Lab 5)
    TRISCbits.TRISC2 = 0; // Set CCP1 (RC2) as output
    T2CONbits.T2CKPS = 0b00; // Timer2 Prescaler 1:1
    PR2 = 249; // Set Period for exactly 4kHz
    T2CONbits.TMR2ON = 1; // Fixed: was TMR20N
    CCP1CON = 0b00001100; // CCP1 in PWM mode
}

// --- Hardware Functional Logic ---
uint16_t ADC_Read(unsigned char channel) {
    // Select ADC Channel by clearing and then setting CHS bits in ADCON0
    ADCON0 &= 0b11000101; // Clearing the CHS bits (bits 5:3)
    ADCON0 |= (channel << 3); // Set the desired channel (0-7) by shifting left into position

    /* Acquisition Time: The internal sample-and-hold capacitor
    * needs physical time to charge to the sensor voltage.
    */
    __delay_us(20);

    ADCON0bits.GO = 1; // Start conversion
    while(ADCON0bits.GO); // Wait for hardware to clear GO bit (Done)

    // Combine 8-bit result registers into a single 16-bit value
    // Since we used Right Justification, ADRESH holds the top 2 bits.
    return ((uint16_t)(ADRESH << 8) + ADRESL);
}

void PWM_SetDuty(uint16_t duty) {
    /*
    * To achieve 10-bit resolution, the duty cycle is split:
    * 1. The 8 Most Significant Bits go to CCPR1L.
    * 2. The 2 Least Significant Bits go to CCP1CON bits 5 and 4.
    */
    CCPR1L = (unsigned char)(duty >> 2); // Shift right to drop bottom 2 bits

    // Mask out bits 5 and 4, then OR in the new bottom 2 bits of the duty value
    CCP1CON = (unsigned char)((CCP1CON & 0xCF) | ((duty & 0x0003) << 4));
}