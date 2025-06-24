#ifndef __STATEMACHINE_H
#define __STATEMACHINE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
typedef enum {
    Lock = 0,
    Idle = 1,       // Machine Hardware Idle
    Ready = 2,      // Machine Hardware Ready
    Running = 3,      // Machine Hardware Error
    Emergency = 4,     // Machine Hardware Merge
    Complete = 5,
    SYS_INIT = 6,
} MachineState;



typedef enum {
    twinkle = 0,        // Machine LED twinkle
    Green = 1,          // Machine LED Green
    Red = 2,            // Machine LED Red
    Yellow = 3,         // Machine LED Yellow
    Led_OFF = 4,        // Machine LED OFF
} LEDState;


typedef enum {
    PressDown = 0,      // Machine is normal press
    PressUp = 1,        // Machine is Error and Stop
} EStopButtonState;

typedef enum {
    PressFall = 0,      // Machine is single Falling press
    PressRise = 1,      // Machine is single Rising press
} StartButtonState;

typedef enum {
    Trigger = 0,        // Machine is unsafe trigger
    UnTriggered = 1,    // Machine is safe
} SafeSensorState;

typedef enum {
    Door_Closed = 0,          // Cylinder is Down
    Door_Opened = 1,         // Cylinder is Up
    Door_Mid = 2,           // Cylinder is Mid          
} CylinderState;

typedef struct Action_Map {
    MachineState Machine;
    LEDState led;
    EStopButtonState eStopButton;
    StartButtonState startButton;
    SafeSensorState  safeSensor;
    CylinderState cylinder;
} MachineAction;

uint8_t StateMachine_Input();
uint8_t Init_Action(uint8_t in_01_08, uint8_t in_09_16, uint8_t out_01_08, uint8_t out_09_16);
uint8_t Lock_Action(uint8_t in_01_08, uint8_t in_09_16, uint8_t out_01_08, uint8_t out_09_16);
uint8_t Idle_Action(uint8_t in_01_08, uint8_t in_09_16, uint8_t out_01_08, uint8_t out_09_16);
uint8_t Ready_Action(uint8_t in_01_08, uint8_t in_09_16, uint8_t out_01_08, uint8_t out_09_16);
uint8_t Running_Action(uint8_t in_01_08, uint8_t in_09_16, uint8_t out_01_08, uint8_t out_09_16);
uint8_t Complete_Action(uint8_t in_01_08, uint8_t in_09_16, uint8_t out_01_08, uint8_t out_09_16);
uint8_t Emerge_Action(uint8_t in_01_08, uint8_t in_09_16, uint8_t out_01_08, uint8_t out_09_16);
uint8_t Release_detection(uint8_t in_01_08, uint8_t in_09_16, uint8_t out_01_08, uint8_t out_09_16);
uint8_t Door_detection(uint8_t in_01_08, uint8_t in_09_16, uint8_t out_01_08, uint8_t out_09_16);
uint8_t showStatus();
uint8_t showDoorStatus();

#endif /* __STATEMACHINE_H */