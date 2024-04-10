/*
    Program turning an ATmega32u4 microcontroller into a HDMI Consumer
    Electronics Control capable device.

    https://github.com/m-fudala
*/

#include <util/delay.h>
#include "ATmega32u4_libraries/uart_library/uart.h"
#include "ATmega32u4_libraries/external_interrupt_library/external_interrupt.h"
#include "ATmega32u4_libraries/gpio_library/gpio.h"
#include "serial_commands.h"
#include "hdmi_cec.h"


void button_press();
volatile unsigned char send_message = 0;

int main()
{
    USBCON = 0;

    uart_start(BAUD_250K);

    unsigned char uart_rx_buffer[8];

    cec_init();

    // init interrupt 2
    Pin Button;
    pin_init(&Button, &PORTD, PD1, INPUT);
    int_init(INT1, button_press, FALLING_EDGE);
    int_enable(INT1);

    unsigned char message_to_be_sent = 0;

    while (1) {
        if (send_message) {
            send_message = 0;

            // send_bytes((unsigned char[1]){0x0F}, 1);
            send_bytes((unsigned char[3]){0x40, 0x9E, 0x04}, 3);
            // send_bytes((unsigned char[3]){0xFF, 0xFF, 0xFF}, 3);
        }

        if (message_to_be_sent) {
            send_start();

            message_to_be_sent = 0;
        }

        if (Rx.status.message_received) {
            // unsigned char send_ok[5] = "M:\r\n";

            // uart_send(send_ok,
            //         sizeof(send_ok) / sizeof(unsigned char) - 1);

            unsigned char termination[3] = "\r\n";

            uart_send((unsigned char *)Rx.buffer, Rx.status.bytes_read);

            uart_send(termination,
                    sizeof(termination) / sizeof(unsigned char) - 1);

            Rx.status.message_received = 0;

            // TODO: better response system

            if (Rx.buffer[1] == 0x8C) {     // give Vendor ID
                // TODO: waiting for appropiate time
                _delay_ms(12);

                // respond with Feature Abort
                Tx.bytes = (unsigned char[5]){0x40, 0x87, 0x01, 0x01, 0x01};
                Tx.no_of_bytes = 5;

                send_start();
            }
        }

        if (Rx.send_debug) {
            uart_send((unsigned char[1]){Rx.send_debug}, 1);

            Rx.send_debug = 0;
        }

        if (Tx.send_debug) {
            uart_send((unsigned char[1]){Tx.send_debug}, 1);

            Tx.send_debug = 0;
        }

        if (!check_message_readiness()) {
            uart_read(uart_rx_buffer, 8);
            unsigned char selected_command = 0;

            for (unsigned char command_no = 0; command_no < NO_OF_COMMANDS;
                    ++command_no) {
                unsigned char character_hits = 0;

                for (unsigned char character = 0;
                        character < commands[command_no].command_len;
                        ++ character) {
                    if (commands[command_no].command_str[character] ==
                            uart_rx_buffer[character]) {
                        ++character_hits;
                    }
                }

                if (character_hits == commands[command_no].command_len) {
                    selected_command = command_no + 1;
                }
            }

            switch (selected_command) {
                case INV_COMMAND: {
                    unsigned char send_nok[11] = "Inv comm\r\n";
                    uart_send(send_nok,
                            sizeof(send_nok) / sizeof(unsigned char) - 1);

                    break;
                }

                case SND: {
                    unsigned char send_ok[6] = "snd\r\n";

                    uart_send(send_ok,
                            sizeof(send_ok) / sizeof(unsigned char) - 1);

                    // send Image View On
                    Tx.bytes =
                        (unsigned char[2]){0x40, 0x04};
                    Tx.no_of_bytes = 2;

                    message_to_be_sent = 1;

                    break;
                }
            }
        }
    }

    return 0;
}

void button_press() {
    send_message = 1;
}
