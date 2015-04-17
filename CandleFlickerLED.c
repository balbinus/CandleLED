/**
 * CandleFlickerLED.c forked from https://github.com/cpldcpu/CandleLEDhack.
 *
 * Emulates a candleflicker-LED on an AVR microcontroller.
 */

#ifndef F_CPU
#ifdef __AVR_ATtiny13__
#define F_CPU 9600000
#else
#define F_CPU 16000000
#endif
#endif

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>

/**
 * This is a 32-bit long LFSR. The point is to decrease repetition in the
 * sequences, but it takes a toll on performance (slightly) and on program/
 * memory size.(see http://www.ece.cmu.edu/~koopman/lfsr/index.html on LFSRs).
 * 
 * Values are inverted so the LFSR also works with zero initialization.
 */
#define LFSR_FEEDBACK_TERM 0x7FFFF159

// Led connected to PB0
#define LEDPORT PORTB
#define LEDDDR  DDRB
#define LEDPIN  PB0

// Select only the lowest byte
#define low_byte(x) ((uint8_t) x)

int main(void)
{
    cli();
	
    // Set output pin direction
	LEDDDR |= _BV(LEDPIN);	            // LED is connected to PB0
    
    // Timer/Counter Control Register A
#ifdef __INVERTED_PWM
    TCCR0A = _BV(COM0A1) | _BV(COM0A0)  // Inverted PWM (0 at start, 1 on match)
#else
    TCCR0A = _BV(COM0A1)                // "Normal" PWM (1 at start, 0 on match)
#endif
           | _BV(WGM01)  | _BV(WGM00);  // Fast PWM mode 0x00-0xFF then overflow
    // Timer/Counter Control Register B
    TCCR0B = _BV(CS01) | _BV(CS00);     // Counter started, f/8
    // Timer/Counter Interrupt Mask Register
    TIMSK0 = _BV(TOIE0);                // Timer/Counter0 Overflow Int Enable
    OCR0A = 0;
    
    sei();
    
    while (1);
}

/**
 * ISR triggered on Timer0 overflow.
 * 
 * Overflow of the timer = frame counter increment
 */
ISR(TIM0_OVF_vect)
{
    static uint8_t  FRAME_CTR  = 0;
    static uint32_t RAND       = 0;

    FRAME_CTR++;
    FRAME_CTR &= 0x1F;
    
    /**
     * Generate a new random brightness value at the bottom of each frame, and
     * if the number we've generated is deemed invalid, we retry up to three
     * times to make a new one (& 0x07).
     * 
     * Bad values are those whose bits 2 and 3 (0b1100) are not set. These
     * values will be too low for our flicker to work.
     * 
     * In the end, we clamp the 5-bit random value to 4 bits so that 50% of the
     * time, the LED is full on.
     * 
     * (uint8_t) conversion (via low_byte()) is here to simplify asm code and
     * compare only on the byte we need. GCC can't see this optimization somehow.
     */
    if (FRAME_CTR == 0 || (((FRAME_CTR & 0x07) == 0) && ((low_byte(RAND) & 0x18) == 0)))
    {
        if ((RAND & 1) == 1) RAND >>= 1;
        else RAND = (RAND >> 1) ^ LFSR_FEEDBACK_TERM;
    }

    /**
     * Top of a frame: set the new PWM value from the generated randomness.
     */
    if (FRAME_CTR == 0x1F)
    {
        OCR0A = (low_byte(RAND) & 0x10) > 0 ? 0xFF : (low_byte(RAND) << 4) | 0xF;
    }
}
