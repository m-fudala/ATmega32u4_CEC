#ifndef SERIAL_COMMANDS_H_
#define SERIAL_COMMANDS_H_

#define MAX_COMMAND_SIZE (3 + 1)
#define NO_OF_COMMANDS 1

enum Commands_index {
    INV_COMMAND,
    SND
};

typedef struct Command {
    unsigned char command_str[MAX_COMMAND_SIZE];
    unsigned char command_len;
} Command;

Command commands[1] = {
    {"SND", 3}
};

#endif