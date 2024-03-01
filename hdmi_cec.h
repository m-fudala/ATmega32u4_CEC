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

#define START_LOW_TIME 7400
#define START_HIGH_TIME 9000
#define ONE_LOW_TIME 1200
#define ZERO_LOW_TIME 3000
#define BIT_HIGH_TIME 4800

enum CEC_direction {
    INITIATOR,
    FOLLOWER
};

typedef struct CEC {
    volatile unsigned char bit_sent : 1;
    volatile unsigned char bus_direction : 1; 
} CEC;

Pin CEC_bus;
CEC CEC_status;

void cec_init();

void set_initiator();

void set_follower();

void send_start();

void send_bit(unsigned char bit);

void send_byte(unsigned char byte);

void bus_interrupt_handler();

#endif