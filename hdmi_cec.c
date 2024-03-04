#include "hdmi_cec.h"

void cec_init() {
    cli();

    // init pin
    pin_init(&CEC_bus, &PORTE, PE6, INPUT);
    Tx.status.bus_direction = FOLLOWER;

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

    Tx.status.bus_direction = INITIATOR;
}

void set_follower() {
    pin_set_direction(&CEC_bus, INPUT);
    Tx.status.bus_direction = FOLLOWER;

    int_enable(INT6);
}

void send_start() {
    set_initiator();

    OCR1A = START_LOW_TIME;
    OCR1B = START_TIME;

    TCNT1 = 0;
    TCCR1B |= _BV(CS11);

    pin_write(&CEC_bus, LOW);
}

void send_bytes(unsigned char *message, unsigned char no_of_bytes) {
    Tx.bytes = message;
    Tx.no_of_bytes = no_of_bytes;
    
    send_start();   
}

ISR (TIMER1_COMPA_vect) {   // counting time until the bus should go high again
    pin_write(&CEC_bus, HIGH);
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

    if (Tx.status.bits_sent == 10) {
        TCCR1B &= ~_BV(CS11);

        Tx.status.bytes_sent = 0;
    } else if (Tx.status.bits_sent == 9) {      // ack bit
        OCR1A = ZERO_LOW_TIME;  // ack by initiator

        Tx.status.bits_sent = 0;

        TCNT1 = 0;

        pin_write(&CEC_bus, LOW);

        if (Tx.status.bytes_sent == Tx.no_of_bytes) {
            Tx.status.bits_sent = 10;
        }
    } else if (Tx.status.bits_sent < 9) {
        ++Tx.status.bits_sent;

        TCNT1 = 0;

        pin_write(&CEC_bus, LOW);
    }
}

void send_ack() {
    // set_initiator();

    // TCNT1 = 0;

    // pin_write(&CEC_bus, LOW);       // not cool, better implementation needed
    // while(TCNT1 < ZERO_LOW_TIME);   //
    //                                 //
    // pin_write(&CEC_bus, HIGH);      //
    // while(TCNT1 < BIT_TIME);        //

    // set_follower();
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

            Rx.send_debug = 'L';
            break;
        }

        case HIGH:{
            if ((TCNT1 > START_LOW_TIME - TOLERANCE) &&
                    (TCNT1 < START_LOW_TIME + TOLERANCE)) {
                pin_write(&debug, HIGH);

                Rx.status.start_detected = 1;
                Rx.status.message_ended = 0;

                Rx.send_debug = 'S';
                break;
            } else if ((TCNT1 > ONE_LOW_TIME - TOLERANCE) &&
                    (TCNT1 < ONE_LOW_TIME + TOLERANCE)) {
                if (Rx.status.start_detected) {
                    pin_toggle(&debug);

                    if (Rx.status.current_bit < 8) {
                        Rx.buffer[Rx.status.bytes_read] |
                            (0x80 >> Rx.status.current_bit);
                        Rx.send_debug = '1';
                    } else {
                        Rx.status.message_ended = 1;
                        TCCR1B &= ~_BV(CS11);
                        Rx.send_debug = 'Y';
                    }
                }
            } else if ((TCNT1 > ZERO_LOW_TIME - TOLERANCE) &&
                    (TCNT1 < ZERO_LOW_TIME + TOLERANCE)) {
                Rx.send_debug = 4;
                pin_toggle(&debug);
                        
                if (Rx.status.start_detected  && (Rx.status.current_bit < 8)) {
                    Rx.buffer[Rx.status.bytes_read] &
                        ~(0x80 >> Rx.status.current_bit);
                        Rx.send_debug = '0';
                } else {
                    Rx.status.ack_detected = 1;
                    Rx.send_debug = 'N';
                }
            } else {
                Rx.send_debug = 'E';
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
