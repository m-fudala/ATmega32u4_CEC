#include "hdmi_cec.h"

Pin CEC_bus;

void cec_init() {
    bit_sent = 1;

    cli();

    // init pin
    pin_init(&CEC_bus, &PORTE, PE6, OUTPUT);
    pin_write(&CEC_bus, HIGH);

    // init timer 1
    TIMSK1 |= _BV(OCIE1A) | _BV(OCIE1B);

    sei();
}

void send_start() {
    OCR1A = START_LOW_TIME;
    OCR1B = START_HIGH_TIME;

    bit_sent = 0;

    TCNT1 = 0;
    TCCR1B |= _BV(CS11);

    pin_write(&CEC_bus, LOW);
}

void send_bit(unsigned char bit) {
    if (bit) {
        OCR1A = ONE_LOW_TIME;
    } else {
        OCR1A = ZERO_LOW_TIME;
    }

    OCR1B = BIT_HIGH_TIME;

    bit_sent = 0;

    TCNT1 = 0;
    TCCR1B |= _BV(CS11);

    pin_write(&CEC_bus, LOW);
}

void send_byte(unsigned char byte) {
    send_start();

    while (!bit_sent);

    for (unsigned char bit_id = 0; bit_id < 8; ++bit_id) {
        send_bit((byte & (0x80 >> bit_id)) >> (7 - bit_id));

        while (!bit_sent);
    }

    send_bit(1);

    while (!bit_sent);

    send_bit(1);

    while (!bit_sent);
}

ISR (TIMER1_COMPA_vect) {
    pin_write(&CEC_bus, HIGH);
}

ISR (TIMER1_COMPB_vect) {
    TCCR1B &= ~_BV(CS11);

    bit_sent = 1;
}