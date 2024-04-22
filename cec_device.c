/*
    Program turning an ATmega32u4 microcontroller into a HDMI Consumer
    Electronics Control capable device.

    https://github.com/m-fudala
*/

#include <stdio.h>
#include <util/delay.h>
#include "ATmega32u4_libraries/uart_library/uart.h"
#include "ATmega32u4_libraries/external_interrupt_library/external_interrupt.h"
#include "ATmega32u4_libraries/gpio_library/gpio.h"
#include "serial_commands.h"
#include "hdmi_cec.h"
#include "cec_commands.h"


void button_press();

int main()
{
    USBCON = 0;

    uart_start(BAUD_250K);

    unsigned char uart_rx_buffer[8];

    cec_init();

    unsigned char message_to_be_sent = 0;

    while (1) {
        if (message_to_be_sent) {
            message_to_be_sent = 0;
            
            send_start();
        }

        if (Rx.status.message_received) {
            Rx.status.message_received = 0;

            CEC_command received_command = {
                .header = Rx.buffer[0],
                .opcode = Rx.buffer[1]
            };

            if (((received_command.header & 0xF) == 0x4)
                    && (Rx.status.bytes_read > 1)) {

                // buffer used to send debug with received messages
                // 4 characters at the start, 3 characters for every byte plus
                // a space and termination at the end
                unsigned char buffer[4 + Rx.status.bytes_read * 3 + 3];

                sprintf(buffer, "Rec:");

                for (unsigned char i = 0; i < Rx.status.bytes_read; ++i) {
                    sprintf(buffer, "%s %02X", buffer, Rx.buffer[i]);
                }

                sprintf(buffer, "%s\r\n", buffer);

                uart_send(buffer, 4 + Rx.status.bytes_read * 3 + 3 - 1);
            }

            switch (received_command.opcode) {
                case GIVE_OSD_NAME: {
                    // set 'TV PC' as OSD name
                    Tx.bytes =
                        (unsigned char[7]){0x40, SET_OSD_NAME,
                        'T', 'V', ' ', 'P', 'C'};
                    Tx.no_of_bytes = 7;

                    // TODO: waiting for appropiate time
                    _delay_ms(12);

                    send_start();

                    break;
                }

                case GIVE_PHYSICAL_ADDRESS: {
                    // respond with static address
                    Tx.bytes =
                        (unsigned char[5]){0x40, REPORT_PHYSICAL_ADDRESS,
                        0x10, 0x00, 0x04};
                    Tx.no_of_bytes = 5;

                    // TODO: waiting for appropiate time
                    _delay_ms(12);

                    send_start();

                    break;
                }

                case GIVE_DEVICE_VENDOR_ID: {
                    // respond with some ID
                    Tx.bytes =
                        (unsigned char[5]){0x40, DEVICE_VENDOR_ID,
                        0x01, 0x01, 0x01};
                    Tx.no_of_bytes = 5;

                    // TODO: waiting for appropiate time
                    _delay_ms(12);

                    send_start();

                    break;
                }
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

                case STB: {
                    unsigned char res[] = "Switching to standby\r\n";

                    uart_send(res, sizeof(res) / sizeof(unsigned char) - 1);

                    // send Standby to TV
                    Tx.bytes =
                        (unsigned char[4]){0x4F, STANDBY};
                    Tx.no_of_bytes = 2;

                    message_to_be_sent = 1;

                    break;
                }

                case IV: {
                    unsigned char res[] = "Turning image ON\r\n";

                    uart_send(res, sizeof(res) / sizeof(unsigned char) - 1);

                    // send Image View On
                    Tx.bytes =
                        (unsigned char[4]){0x40, IMAGE_VIEW_ON};
                    Tx.no_of_bytes = 2;

                    message_to_be_sent = 1;

                    break;
                }

                case AS: {
                    unsigned char res[] = "Changing source\r\n";

                    uart_send(res, sizeof(res) / sizeof(unsigned char) - 1);

                    // send Active Source
                    // send static address
                    Tx.bytes =
                        (unsigned char[4]){0x4F, ACTIVE_SOURCE, 0x10, 0x00};
                    Tx.no_of_bytes = 4;

                    message_to_be_sent = 1;

                    break;
                }
            }
        }
    }

    return 0;
}
