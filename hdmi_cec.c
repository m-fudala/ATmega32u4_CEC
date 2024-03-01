#include "hdmi_cec.h"

void cec_init() {
    CEC_status.bit_sent = 1;
    CEC_status.bus_direction = FOLLOWER;

    cli();

    // init pin
    pin_init(&CEC_bus, &PORTE, PE6, INPUT);
    // pin_write(&CEC_bus, HIGH);

    // init timer 1
    TIMSK1 |= _BV(OCIE1A) | _BV(OCIE1B);

    // init interrupt 6
    int_init(INT6, bus_interrupt_handler, FALLING_EDGE);
    int_enable(INT6);

    sei();
}

void set_initiator() {
    int_disable(INT6);

    pin_set_direction(&CEC_bus, INPUT_PULLUP);  // intermediate step
    pin_set_direction(&CEC_bus, OUTPUT);

    CEC_status.bus_direction = INITIATOR;
}

void set_follower() {
    pin_set_direction(&CEC_bus, INPUT);

    int_enable(INT6);

    CEC_status.bus_direction = FOLLOWER;
}

void send_start() {
    set_initiator();

    OCR1A = START_LOW_TIME;
    OCR1B = START_HIGH_TIME;

    CEC_status.bit_sent = 0;

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

    CEC_status.bit_sent = 0;

    TCNT1 = 0;
    TCCR1B |= _BV(CS11);

    pin_write(&CEC_bus, LOW);
}

void send_byte(unsigned char byte) {
    send_start();

    while (!CEC_status.bit_sent);

    for (unsigned char bit_id = 0; bit_id < 8; ++bit_id) {
        send_bit((byte & (0x80 >> bit_id)) >> (7 - bit_id));

        while (!CEC_status.bit_sent);
    }

    send_bit(1);

    while (!CEC_status.bit_sent);

    send_bit(1);

    while (!CEC_status.bit_sent);
}

void bus_interrupt_handler() {

}

ISR (TIMER1_COMPA_vect) {
    pin_write(&CEC_bus, HIGH);
}

ISR (TIMER1_COMPB_vect) {
    TCCR1B &= ~_BV(CS11);

    CEC_status.bit_sent = 1;
}