#ifndef SERIAL_COMMANDS_H_
#define SERIAL_COMMANDS_H_

#define MAX_COMMAND_SIZE (3 + 1)
#define NO_OF_COMMANDS 7

enum Commands_index {
    INV_COMMAND,
    STB,
    IV,
    AS,
    VU,
    VD,
    MT,
    OSD
};

typedef struct Command {
    unsigned char command_str[MAX_COMMAND_SIZE];
    unsigned char command_len;
} Command;

Command commands[NO_OF_COMMANDS] = {
    {"STB", 3},
    {"IV", 2},
    {"AS", 2},
    {"VU", 2},
    {"VD", 2},
    {"MT", 2},
    {"OSD", 3}
};

#endif