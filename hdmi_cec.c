#include "hdmi_cec.h"

void cec_init() {
    CEC_status.bit_sent = 1;

    cli();

    // init pin
    pin_init(&CEC_bus, &PORTE, PE6, INPUT);
    CEC_status.bus_direction = FOLLOWER;

    pin_init(&debug, &PORTC, PC6, OUTPUT);

    // init timer 1
    TIMSK1 |= _BV(OCIE1A) | _BV(OCIE1B);

    // init interrupt 6
    int_init(INT6, bus_interrupt_handler, ANY_EDGE);
    int_enable(INT6);

    sei();
}

void set_initiator() {
    int_disable(INT6);

    if (pin_read(&CEC_bus)) {
        pin_set_direction(&CEC_bus, INPUT_PULLUP);  // intermediate step
    }

    pin_set_direction(&CEC_bus, OUTPUT);

    CEC_status.bus_direction = INITIATOR;
}

void set_follower() {
    pin_set_direction(&CEC_bus, INPUT);
    CEC_status.bus_direction = FOLLOWER;

    int_enable(INT6);
}

void send_start() {
    set_initiator();

    OCR1A = START_LOW_TIME;
    OCR1B = START_TIME;

    CEC_status.bit_sent = 0;

    TCNT1 = 0;
    TCCR1B |= _BV(CS11);

    pin_write(&CEC_bus, LOW);

    while (!CEC_status.bit_sent);
}

void send_bit(unsigned char bit) {
    if (bit) {
        OCR1A = ONE_LOW_TIME;
    } else {
        OCR1A = ZERO_LOW_TIME;
    }

    OCR1B = BIT_TIME;

    CEC_status.bit_sent = 0;

    TCNT1 = 0;
    TCCR1B |= _BV(CS11);

    pin_write(&CEC_bus, LOW);

    while (!CEC_status.bit_sent);
}

void insert_EOM(unsigned char message_complete) {
    send_bit(message_complete);
}

void wait_for_ack() {
    send_bit(1);

    // wait for ACK
    set_follower();

    while(!Rx.status.ack_detected);

    set_initiator();
}

void send_bytes(unsigned char *message, unsigned char no_of_bytes) {
    send_start();

    for (unsigned char byte_id = 0; byte_id < no_of_bytes; ++byte_id) {
        Rx.status.ack_detected = 0;

        for (unsigned char bit_id = 0; bit_id < 8; ++bit_id) {
            send_bit((message[byte_id] & (0x80 >> bit_id)) >> (7 - bit_id));

            if (bit_id == 7) {
                insert_EOM(byte_id == (no_of_bytes - 1));

                wait_for_ack();
            }
        }
    }    
}

void send_ack() {
    set_initiator();

    TCNT1 = 0;

    pin_write(&CEC_bus, LOW);       // not cool, better implementation needed
    while(TCNT1 < ZERO_LOW_TIME);   //
                                    //
    pin_write(&CEC_bus, HIGH);      //
    while(TCNT1 < BIT_TIME);        //

    set_follower();
}

void bus_interrupt_handler() {
    switch (pin_read(&CEC_bus)) {
        case LOW: {
            TCCR1B |= _BV(CS11);

            if (Rx.status.current_bit == 9) {
                Rx.status.current_bit = 0;
                send_ack();
            } else {
                TCNT1 = 0;
            }

            break;
        }

        case HIGH:{
            if ((TCNT1 > START_LOW_TIME - TOLERANCE) &&
                    (TCNT1 < START_LOW_TIME + TOLERANCE)) {
                pin_write(&debug, HIGH);

                Rx.status.start_detected = 1;
                Rx.status.message_ended = 0;

                break;
            } else if ((TCNT1 > ONE_LOW_TIME - TOLERANCE) &&
                    (TCNT1 < ONE_LOW_TIME + TOLERANCE)) {
                if (Rx.status.start_detected) {
                    pin_toggle(&debug);

                    if (Rx.status.current_bit < 8) {
                        Rx.buffer[Rx.status.bytes_read] |
                            (0x80 >> Rx.status.current_bit);
                    } else {
                        Rx.status.message_ended = 1;
                        TCCR1B &= ~_BV(CS11);
                    }
                }
            } else if ((TCNT1 > ZERO_LOW_TIME - TOLERANCE) &&
                    (TCNT1 < ZERO_LOW_TIME + TOLERANCE)) {
                if (Rx.status.start_detected  && (Rx.status.current_bit < 8)) {
                    pin_toggle(&debug);
                    Rx.buffer[Rx.status.bytes_read] &
                        ~(0x80 >> Rx.status.current_bit);
                } else {
                    Rx.status.ack_detected = 1;
                }
            } else {
                Rx.status.start_detected = 0;
                break;
            }

            if (Rx.status.current_bit == 7) {
                ++Rx.status.bytes_read;
            }

            ++Rx.status.current_bit;

            break;
        }
    }
}

ISR (TIMER1_COMPA_vect) {   // counting time until the bus should go high again
    pin_write(&CEC_bus, HIGH);
}

ISR (TIMER1_COMPB_vect) {   // counting until start/bit ends
    TCCR1B &= ~_BV(CS11);

    CEC_status.bit_sent = 1;
}