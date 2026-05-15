/*
 * Description:
 *   Hardware PWM motor speed controller using the CCP1 module.
 *   - PWM Frequency : 4 kHz  (PR2 = 249, Prescaler = 1)
 *   - Resolution    : 1000 steps (10-bit effective via PR2+1)*4 = 1000)
 *   - Button A (RB0): Increase speed by 10% (100 steps)
 *   - Button B (RB1): Decrease speed by 10% (100 steps)
 *   - Motor output  : RC2 / CCP1 pin
 *
 * Calculations:
 *   FOSC = 4 MHz  =>  TOSC = 0.25 us
 *   PWM Period = (PR2 + 1) x 4 x TOSC x Prescaler
 *   250 us = (PR2 + 1) x 4 x 0.25 us x 1
 *   PR2 = 249
 *
 *   Max 10-bit value = (PR2 + 1) x 4 = 250 x 4 = 1000
 *   10% increment    = 1000 / 10 = 100
 */

#include <xc.h>
#include <stdint.h>

/* ── Configuration Bits ─────────────────────────────────────────────── */
#define _XTAL_FREQ 4000000
#pragma config FOSC = XT, WDTE = OFF, LVP = OFF, BOREN = ON

/* ── Constants ──────────────────────────────────────────────────────── */
#define DUTY_MAX        1000u   /* 100% — (PR2+1)*4 = 250*4             */
#define DUTY_MIN        0u      /* 0%                                   */
#define DUTY_STEP       100u    /* 10% per button press                 */
#define DEBOUNCE_MS     15u     /* Software debounce delay (ms)         */

/* ── Macro: write 10-bit duty value to CCPR1L + CCP1CON<5:4> ───────── */
/*
 *  The 10-bit duty is split:
 *    Upper 8 bits  -> CCPR1L        (bits 9:2 of duty_reg, i.e. >> 2)
 *    Lower 2 bits  -> CCP1CON<5:4>  (bits 1:0 of duty_reg, shifted to pos 5:4)
 *
 *  We preserve the lower nibble of CCP1CON (mode bits CCP1M3:CCP1M0)
 *  by masking with 0xCF before ORing in the new lower bits.
 */
#define SET_DUTY(d) \
    do { \
        CCPR1L  = (unsigned char)((d) >> 2); \
        CCP1CON = (unsigned char)((CCP1CON & 0xCF) | (((d) & 0x0003u) << 4)); \
    } while(0)

/* ── Helper: millisecond delay using __delay_ms (non-blocking sections) */
static void delay_ms(uint8_t ms)
{
    while (ms--) {
        __delay_ms(1);
    }
}

/* ════════════════════════════════════════════════════════════════════ */
void main(void)
{
    /* ── 1. GPIO Configuration ──────────────────────────────────────── */
    TRISCbits.TRISC2 = 0;   /* RC2/CCP1 as Output (motor drive)        */
    TRISBbits.TRISB0 = 1;   /* RB0 as Input  (Speed UP  button)        */
    TRISBbits.TRISB1 = 1;   /* RB1 as Input  (Speed DOWN button)       */

    /* Enable pull-ups on Port B (RBPU bit in OPTION_REG, active LOW)   */
    OPTION_REGbits.nRBPU = 0;

    /* ── 2. Timer2 Configuration (sets PWM Frequency) ──────────────── */
    T2CONbits.T2CKPS = 0b00;   /* Prescaler 1:1                        */
    PR2               = 249;    /* Period register -> 4 kHz             */
    T2CONbits.TMR2ON  = 1;      /* Start Timer2                         */

    /* ── 3. CCP1 Configuration (PWM mode, duty = 0% to start) ──────── */
    /*
     *  CCP1CON = 0b00001100
     *    CCP1M3:CCP1M0 = 1100  -> PWM mode
     *    DC1B1 :DC1B0  = 00    -> lower 2 duty bits = 0
     */
    CCPR1L  = 0;            /* Upper 8 bits of duty = 0                 */
    CCP1CON = 0b00001100;   /* PWM mode, lower duty bits = 0            */

    /* ── 4. Runtime variables ───────────────────────────────────────── */
    uint16_t duty_reg = 0;  /* Current duty (0 – 1000)                  */

    /* ── 5. Main Loop ───────────────────────────────────────────────── */
    while (1)
    {
        /*
         * Buttons are active-LOW (10k pull-up to Vcc).
         * PORTBbits.RB0 == 0  =>  button pressed.
         */

        /* ── Speed UP: RB0 pressed ──────────────────────────────────── */
        if (PORTBbits.RB0 == 0)
        {
            delay_ms(DEBOUNCE_MS);          /* Debounce                 */

            if (PORTBbits.RB0 == 0)         /* Still pressed?           */
            {
                if (duty_reg <= (DUTY_MAX - DUTY_STEP))
                    duty_reg += DUTY_STEP;  /* Increment by 10%         */
                else
                    duty_reg = DUTY_MAX;    /* Clamp to 100%            */

                SET_DUTY(duty_reg);         /* Update hardware registers */

                /* Wait for button release to avoid repeated increments  */
                while (PORTBbits.RB0 == 0);
                delay_ms(DEBOUNCE_MS);      /* Release debounce          */
            }
        }

        /* ── Speed DOWN: RB1 pressed ────────────────────────────────── */
        if (PORTBbits.RB1 == 0)
        {
            delay_ms(DEBOUNCE_MS);          /* Debounce                  */

            if (PORTBbits.RB1 == 0)         /* Still pressed?            */
            {
                if (duty_reg >= (DUTY_MIN + DUTY_STEP))
                    duty_reg -= DUTY_STEP;  /* Decrement by 10%          */
                else
                    duty_reg = DUTY_MIN;    /* Clamp to 0%               */

                SET_DUTY(duty_reg);         /* Update hardware registers  */

                /* Wait for button release                               */
                while (PORTBbits.RB1 == 0);
                delay_ms(DEBOUNCE_MS);      /* Release debounce           */
            }
        }

        /*
         * CPU is completely FREE here.
         * You can add sensor reads, UART comms, PID loops, etc.
         * The hardware CCP module maintains the PWM with zero CPU load.
         */
    }
}