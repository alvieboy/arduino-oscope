#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__


/* Serial commands we support */
#define COMMAND_PING           0x3E
#define COMMAND_GET_VERSION    0x40
#define COMMAND_START_SAMPLING 0x41
#define COMMAND_SET_TRIGGER    0x42
#define COMMAND_SET_HOLDOFF    0x43
#define COMMAND_SET_TRIGINVERT 0x44
#define COMMAND_SET_VREF       0x45
#define COMMAND_SET_PRESCALER  0x46
#define COMMAND_GET_PARAMETERS 0x47
#define COMMAND_VERSION_REPLY  0x80
#define COMMAND_BUFFER_SEG     0x81
#define COMMAND_PARAMETERS_REPLY 0x87
/* Parameters are:
 buf[0] = triggerLevel;
 buf[1] = holdoffSamples;
 buf[2] = adcref;
 buf[3] = prescale;
 */
#define COMMAND_PONG           0xE3
#define COMMAND_ERROR          0xFF

#endif
