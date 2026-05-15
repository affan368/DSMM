#include <xc.h>
#include <stdint.h>
#define _XTAL_FREQ 4000000

void ADC_Init() {
    ADCON1 = 0b10000000; // Right Justified, All Analog
    ADCON0 = 0b01000001; // Fosc/8, Channel 0, ADC ON
}

uint16_t ADC_Read() {
    __delay_us(20);    // Acquisition Time (changed to double underscore)
    ADCON0bits.GO = 1;   // Start
    while(ADCON0bits.GO); // Wait
    return (uint16_t)((uint16_t)ADRESH << 8) | (uint16_t)ADRESL;
}

void main() {
    ADC_Init();
    TRISD = 0x00;
    while(1) {
    uint16_t val = ADC_Read();
    PORTD = 0; // Clear
    if(val > 307) PORTDbits.RD0 = 1;
    if(val > 614) PORTDbits.RD1 = 1;
    if(val > 921) PORTDbits.RD2 = 1;
    }
}