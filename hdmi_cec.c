#include "hdmi_cec.h"

Pin CEC_bus;

void cec_init() {
    cli();

    // init pin
    pin_init(&CEC_bus, &PORTE, PE6, OUTPUT);
    pin_write(&CEC_bus, HIGH);

    // init timer 1 
    TCCR1B |= _BV(CS11);
    // TIMSK1 |= _BV(OCIE1A) | _BV(OCIE1B);

    sei();
}

void send_start() {
    TCNT1 = 0;

    OCR1A = START_LOW_TIME;
    OCR1B = START_HIGH_TIME;

    TIMSK1 |= _BV(OCIE1A) | _BV(OCIE1B);

    pin_write(&CEC_bus, LOW);
}

ISR (TIMER1_COMPA_vect) {
    pin_write(&CEC_bus, HIGH);
}

ISR (TIMER1_COMPB_vect) {
    pin_write(&CEC_bus, LOW);

    TCNT1 = 0;
    TIMSK1 &= ~(_BV(OCIE1A) | _BV(OCIE1B));
}