/*
    Library handling communication on a HDMI CEC bus

    Author: https://github.com/m-fudala
*/

#ifndef HDMI_CEC_H_
#define HDMI_CEC_H_

#include <avr/io.h>
#include <avr/interrupt.h>

#include "ATmega32u4_libraries/gpio_library/gpio.h"
#include "ATmega32u4_libraries/external_interrupt_library/external_interrupt.h"

#define START_LOW_TIME 7400     // 3.7 ms
#define START_TIME 9000    // 4.5 ms
#define ONE_LOW_TIME 1200       // 0.6 ms
#define ZERO_LOW_TIME 3000      // 1.5 ms
#define BIT_TIME 4800      // 2.4 ms
#define WAIT_FOR_ACK 3400       // 1.7 ms
#define TOLERANCE 400           // 0.4 ms

#define CEC_RX_BUFFER_SIZE 16   // max CEC message length

Pin CEC_bus;
Pin debug;

enum CEC_direction {
    INITIATOR,
    FOLLOWER
};

typedef struct CEC {
    volatile unsigned char bit_sent : 1;
    volatile unsigned char bus_direction : 1; 
} CEC;

CEC CEC_status;

typedef struct CEC_Rx {
    volatile unsigned char buffer[CEC_RX_BUFFER_SIZE];

    struct {
        volatile unsigned char start_detected : 1;
        volatile unsigned char bytes_read : 4;
        volatile unsigned char current_bit : 4;
        volatile unsigned char message_ended : 1;
        volatile unsigned char ack_detected : 1;
    } status;
} CEC_Rx;

CEC_Rx Rx;

void cec_init();

void set_initiator();

void set_follower();

void send_start();

void send_bit(unsigned char bit);

void insert_EOM(unsigned char message_complete);

void wait_for_ack();

void send_bytes(unsigned char *message, unsigned char no_of_bytes);

void send_ack();

void bus_interrupt_handler();

#endif