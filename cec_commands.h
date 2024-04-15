#ifndef CEC_COMMANDS_H_
#define CEC_COMMANDS_H_

// CEC opcodes
#define FEATURE_ABORT               0x00
#define IMAGE_VIEW_ON               0x04
#define SET_MENU_LANGUAGE           0x32
#define STANDBY                     0x36
#define USER_CONTROL_PRESSED        0x44
#define USER_CONTROL_RELEASED       0x45
#define GIVE_OSD_NAME               0x46
#define SET_OSD_NAME                0x47
#define SET_OSD_STRING              0x64
#define ACTIVE_SOURCE               0x82
#define GIVE_PHYSICAL_ADDRESS       0x83
#define REPORT_PHYSICAL_ADDRESS     0x84
#define DEVICE_VENDOR_ID            0x87
#define GIVE_DEVICE_VENDOR_ID       0x8C
#define GIVE_DEVICE_POWER_STATUS    0x8F
#define REPORT_POWER_STATUS         0x90
#define GET_MENU_LANGUAGE           0x91
#define CEC_VERSION                 0x9E
#define GET_CEC_VERSION             0x9F
#define ABORT                       0xFF

// user control codes
enum User_control_codes {
    SELECT,
    UP,
    DOWN,
    LEFT,
    RIGHT,
    EXIT = 0x0D,
    VOLUME_UP = 0x41,
    VOLUME_DOWN,
    MUTE
};

#define CEC_MAX_COMMAND_SIZE 16

typedef struct CEC_command {
    unsigned char header;
    unsigned char opcode;
    unsigned char data[14];
} CEC_command;

union Feature_abort {
    unsigned char data[2];
    
    unsigned char feature_opcode;
    unsigned char abort_reason;
};

union User_control_pressed {
    unsigned char data[1];
    
    unsigned char ui_command;
};

union Active_source {
    unsigned char data[2];
    
    unsigned char physical_address[2];
};

union Report_physical_address {
    unsigned char data[3];
    
    unsigned char physical_address[2];
    unsigned char device_type;
};

union Device_vendor_id {
    unsigned char data[3];
    
    unsigned char vendor_id[3];
};

union Report_power_status {
    unsigned char data[1];
    
    unsigned char power_status;
};

union Cec_version {
    unsigned char data[1];
    
    unsigned char cec_version;
};

#endif