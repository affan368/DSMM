#include <xc.h>

#define _XTAL_FREQ 4000000

unsigned int t0_count = 0;    // Timer0 overflow counter
unsigned int t1_count = 0;    // Timer1 overflow counter

void __interrupt() ISR()
{
    if(INTCONbits.TMR0IF)     // Timer0 interrupt flag
    {
        TMR0 = 0;             // reload Timer0
        t0_count++;

        if(t0_count >= 15)
        {
            RB0 = !RB0;       // toggle LED1
            t0_count = 0;
        }

        INTCONbits.TMR0IF = 0; // clear flag
    }

    if(PIR1bits.TMR1IF)       // Timer1 interrupt flag
    {
        TMR1H = 0;            // reload Timer1 high
        TMR1L = 0;            // reload Timer1 low
        t1_count++;

        if(t1_count >= 4)
        {
            RB2 = !RB2;       // toggle LED3
            t1_count = 0;
        }

        PIR1bits.TMR1IF = 0;  // clear flag
    }
}

int main(void)
{
    TRISB0 = 0;               // RB0 output
    TRISB2 = 0;               // RB2 output
    TRISD0 = 0;               // RD0 output

    PORTB = 0;                // clear PORTB
    PORTD = 0;                // clear PORTD

    OPTION_REGbits.T0CS = 0;  // internal clock
    OPTION_REGbits.PSA = 0;   // prescaler to Timer0
    OPTION_REGbits.PS0 = 1;
    OPTION_REGbits.PS1 = 1;
    OPTION_REGbits.PS2 = 1;   // prescaler 1:256

    TMR0 = 0;                 // Timer0 start

    T1CONbits.TMR1CS = 0;     // internal clock
    T1CONbits.T1CKPS0 = 1;
    T1CONbits.T1CKPS1 = 1;    // prescaler 1:8
    T1CONbits.TMR1ON = 1;     // enable Timer1

    TMR1H = 0;
    TMR1L = 0;                // Timer1 start

    INTCONbits.GIE = 1;       // global interrupt enable
    INTCONbits.TMR0IE = 1;    // Timer0 interrupt enable

    INTCONbits.PEIE = 1;      // peripheral interrupt enable
    PIE1bits.TMR1IE = 1;      // Timer1 interrupt enable

    while(1)
    {
        RD0 = 1;              // LED2 ON
        __delay_ms(100);

        RD0 = 0;              // LED2 OFF
        __delay_ms(100);
    }
}