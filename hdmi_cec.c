#include "hdmi_cec.h"

void cec_init() {
    cli();

    cec_address = 0x4;

    // init pin
    pin_init(&CEC_bus, &PORTE, PE6, INPUT);

    pin_init(&debug, &PORTC, PC6, OUTPUT);

    // init timer 3
    TCNT3 = 0;
    TIMSK3 |= _BV(TOIE3);
    TCCR3B |= _BV(CS31);

    // init interrupt 6
    int_init(INT6, bus_interrupt_handler, ANY_EDGE);
    int_enable(INT6);

    sei();
}

void bus_low() {
    pin_set_direction(&CEC_bus, OUTPUT);
    pin_write(&CEC_bus, LOW);
}

void bus_release() {
    pin_set_direction(&CEC_bus, INPUT);
}

void send_start() {
    int_disable(INT6);

    TIFR1 |= _BV(OCF1A) | _BV(OCF1B);

    OCR1A = START_LOW_TIME;
    OCR1B = START_TIME;

    TIMSK1 |= _BV(OCIE1A) | _BV(OCIE1B);

    TCNT1 = 0;
    TCCR1B |= _BV(CS11);

    // Tx.send_debug = 'S';

    bus_low();
}

void send_bytes(unsigned char *message, unsigned char no_of_bytes) {
    Tx.bytes = message;
    Tx.no_of_bytes = no_of_bytes;
    
    send_start();
}

ISR (TIMER1_COMPA_vect) {   // counting time until the bus should go high again
    Rx.status.bus_idle_overflow = 0;
    TCNT3 = 0;

    bus_release();
    pin_write(&debug, LOW);

    // Tx.send_debug = 'R';

    if (Tx.status.ack_expected) {
        if (pin_read(&CEC_bus) && !((Tx.bytes[0] & 0xF) == 0xF)) {
            TCCR1B &= ~_BV(CS11);
            TIMSK1 &= ~(_BV(OCIE1A) | _BV(OCIE1B));

            Tx.status.bytes_sent = 0;

            int_enable(INT6);
        }

        Tx.status.ack_expected = 0;
    }

    if (Rx.status.ack_detected) {
        // Tx.send_debug = 'F';
        TCCR1B &= ~_BV(CS11);
        TIMSK1 &= ~(_BV(OCIE1A) | _BV(OCIE1B));

        Rx.status.ack_detected = 0;

        EIFR |= _BV(INT6);
        int_enable(INT6);
    }
}

ISR (TIMER1_COMPB_vect) {   // counting until start/bit ends
    if ((Tx.status.bytes_sent == 0) && (Tx.status.bits_sent == 0)) {
        OCR1B = BIT_TIME;
    }

    // data bits
    if (Tx.status.bits_sent < 8) {
        if ((Tx.bytes[Tx.status.bytes_sent] & (0x80 >> Tx.status.bits_sent))) {
            OCR1A = ONE_LOW_TIME;
        } else {
            OCR1A = ZERO_LOW_TIME;
        }
    }

    // eom bit
    if (Tx.status.bits_sent == 8) {
        ++Tx.status.bytes_sent;

        if (Tx.status.bytes_sent == Tx.no_of_bytes) {
            OCR1A = ONE_LOW_TIME;
        } else {
            OCR1A = ZERO_LOW_TIME;
        }
    }

    // Tx.send_debug = Tx.status.bits_sent + 1;

    if (Tx.status.bits_sent == 10) {
        TCCR1B &= ~_BV(CS11);
        TIMSK1 &= ~(_BV(OCIE1A) | _BV(OCIE1B));

        Tx.status.bytes_sent = 0;
        Tx.status.bits_sent = 0;

        int_enable(INT6);
    } else if (Tx.status.bits_sent == 9) {      // ack bit
        OCR1A = ONE_LOW_TIME;

        Tx.status.bits_sent = 0;
        Tx.status.ack_expected = 1;

        TCNT1 = 0;

        bus_low();

        if (Tx.status.bytes_sent == Tx.no_of_bytes) {
            Tx.status.bits_sent = 10;
        }
    } else if (Tx.status.bits_sent < 9) {
        ++Tx.status.bits_sent;

        TCNT1 = 0;

        bus_low();
    }
}

void send_ack() {
    TIFR1 |= _BV(OCF1A) | _BV(OCF1B);
    EIFR |= _BV(INT6);

    TCCR1B &= ~_BV(CS11);

    OCR1A = ZERO_LOW_TIME;

    TIMSK1 |= _BV(OCIE1A);
    TCCR1B |= _BV(CS11);

    pin_write(&debug, HIGH);

    int_disable(INT6);
    bus_low();
}

unsigned char check_addressing(unsigned char header_block) {
    if ((header_block & 0xF) == 0xF) {
        return BROADCAST;
    } else if ((header_block & 0xF) == cec_address) {
        return DIRECT;
    } else {
        return INVALID;
    }
}

void bus_interrupt_handler() {
    switch (pin_read(&CEC_bus)) {
        case LOW: {
            TCCR1B |= _BV(CS11);
            TCNT1 = 0;

            if (Rx.status.current_bit == 9) {
                // Rx.send_debug = TIMSK1;
                Rx.status.ack_detected = 1;

                if (Rx.status.bytes_read == 1) {
                    Rx.status.addressing = check_addressing(Rx.buffer[0]);
                }

                switch (Rx.status.addressing) {
                    case DIRECT: {
                       send_ack();

                        if (Rx.status.eom_detected) {
                            Rx.status.message_received = 1;
                        }
                        // Rx.send_debug = 'D';

                        break;
                    }

                    case BROADCAST: {
                        if (Rx.status.eom_detected) {
                            Rx.status.message_received = 1;
                        }
                        // Rx.send_debug = 'B';

                        break;
                    }

                    case INVALID: {
                        Rx.status.start_detected = 0;
                        Rx.status.message_received = 0;
                        // Rx.send_debug = 'X';

                        break;
                    }
                }

                Rx.status.current_bit = 0;
            }

            // Rx.send_debug = 'L';
            break;
        }

        case HIGH: {
            Rx.status.bus_idle_overflow = 0;
            TCNT3 = 0;

            if ((TCNT1 > START_LOW_TIME - TOLERANCE) &&
                    (TCNT1 < START_LOW_TIME + TOLERANCE)) {

                Rx.status.start_detected = 1;
                Rx.status.eom_detected = 0;
                Rx.status.current_bit = 0;
                Rx.status.bytes_read = 0;

                // Rx.send_debug = 'S';
                return;
            } else if ((TCNT1 > ONE_LOW_TIME - TOLERANCE) &&
                    (TCNT1 < ONE_LOW_TIME + TOLERANCE)) {
                
                // Rx.send_debug = Rx.status.current_bit;
                if (Rx.status.start_detected) {
                    if (Rx.status.ack_detected &&
                            (Rx.status.addressing == BROADCAST)) {
                        Rx.status.ack_detected = 0;

                        return;     // ignore ACK in broadcast
                    }

                    if (Rx.status.current_bit < 8) {
                        Rx.buffer[Rx.status.bytes_read] |=
                            (0x80 >> Rx.status.current_bit);
                        // Rx.send_debug = '1';
                    } else {
                        Rx.status.eom_detected = 1;
                        TCCR1B &= ~_BV(CS11);
                        // Rx.send_debug = 'Y';
                    }
                }
            } else if ((TCNT1 > ZERO_LOW_TIME - TOLERANCE) &&
                    (TCNT1 < ZERO_LOW_TIME + TOLERANCE)) {
                
                // Rx.send_debug = Rx.status.current_bit + 1;
                if (Rx.status.start_detected  && (Rx.status.current_bit < 8)) {
                    Rx.buffer[Rx.status.bytes_read] &=
                        ~(0x80 >> Rx.status.current_bit);
                        // Rx.send_debug = '0';
                } else {
                    Rx.status.ack_detected = 1;
                    // Rx.send_debug = 'N';
                }
            } else {
                // Rx.send_debug = 'E';
                Rx.status.start_detected = 0;
                return;
            }

            if (Rx.status.current_bit == 7) {
                ++Rx.status.bytes_read;
                // Rx.send_debug = 'K';
            }

            ++Rx.status.current_bit;

            break;
        }
    }
}

ISR (TIMER3_OVF_vect) {
    Rx.status.bus_idle_overflow = 1;
}
