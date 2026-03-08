/*-
 * BSD 2-Clause License
 *
 * Copyright (c) 2012-2018, Jan Breuer
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file   scpi-def.c
 * @date   Thu Nov 15 10:58:45 UTC 2012
 *
 * @brief  SCPI parser test
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scpi/scpi.h"
#include "scpi-def.h"
#include "scpi_switch.h"
#include "BsmRelay.h"



static scpi_result_t SCPI_ConfigureSwitch(scpi_t *context)
{
    int32_t number[2] = {0,0};
    uint16_t switch_id = 1;
    SCPI_CommandNumbers(context, number, 1, 1);
    if(number[0] != 1)
    {
        switch_id = number[0];
    }
    uint32_t switch_value = 0;
    if(!SCPI_ParamUInt32(context, &switch_value, true))
    {
        return SCPI_RES_ERR;
    }
    if(Switch_Write(switch_id, switch_value))
    {
        return SCPI_RES_ERR;
    }
    return SCPI_RES_OK;
}

static scpi_result_t SCPI_Configurebaudrate(scpi_t *context)
{
    uint32_t baudrate;
    if(!SCPI_ParamUInt32(context, &baudrate, true))
    {
        return SCPI_RES_ERR;
    }
        // Verify the input baudrate is 115200
    if (baudrate != 115200) {
        SCPI_ResultText(context, "ERROR: Only 115200 baudrate is supported");
        return SCPI_RES_ERR;
    }

    // First change UART3 baudrate to 9600 to match BSM default
    huart3.Init.BaudRate = 9600;
    if (HAL_UART_Init(&huart3) != HAL_OK)
    {
        return SCPI_RES_ERR;
    }

    // Send command to change BSM baudrate to 115200
    uint8_t set_baud_cmd[] = {0x01, 0x06, 0x00, 0x02, 0x04, 0x80, 0x2B, 0x6A};
    HAL_UART_Transmit(&huart3, set_baud_cmd, sizeof(set_baud_cmd), HAL_MAX_DELAY);

    // Small delay to ensure command is processed
    HAL_Delay(100);

    // Write settings to flash
    uint8_t write_flash_cmd[] = {0x01, 0x06, 0x00, 0x09, 0x00, 0x6F, 0x19, 0xE4};
    HAL_UART_Transmit(&huart3, write_flash_cmd, sizeof(write_flash_cmd), HAL_MAX_DELAY);
    HAL_Delay(100);
 
    // Perform warm restart
    uint8_t warm_restart_cmd[] = {0x01, 0x06, 0x00, 0x09, 0x00, 0x7B, 0x19, 0xEB};
    HAL_UART_Transmit(&huart3, warm_restart_cmd, sizeof(warm_restart_cmd), HAL_MAX_DELAY);
    HAL_Delay(1000); // Longer delay for restart to complete

    // Change UART3 baudrate to new value (115200)
    huart3.Init.BaudRate = 115200;
    if (HAL_UART_Init(&huart3) != HAL_OK)
    {
        return SCPI_RES_ERR;
    }

    // Verify the baudrate was changed by reading it back
    uint8_t read_baud_cmd[] = {0x01, 0x03, 0x00, 0x02, 0x00, 0x01, 0x25, 0xCA};
    uint8_t response[8];
    
    HAL_UART_Transmit(&huart3, read_baud_cmd, sizeof(read_baud_cmd), HAL_MAX_DELAY);
    
    // Wait for response (adjust timeout as needed)
    if (HAL_UART_Receive(&huart3, response, sizeof(response), 100) != HAL_OK)
    {
        return SCPI_RES_ERR;
    }

    // Check if response indicates 115200 baud (0x04 0x80)
    if (response[3] != 0x04 || response[4] != 0x80)
    {
        return SCPI_RES_ERR;
    }

    SCPI_ResultText(context, "115200 baudrate is enable");

    return SCPI_RES_OK;
}


static scpi_result_t SCPI_ReadSwitchState(scpi_t *context)
{
    int32_t number[2] = {0};
    uint16_t switch_id = 1;

    SCPI_CommandNumbers(context, number,1,1);
    if(number[0] !=1)
    {
        switch_id = number[0];
    }
    uint32_t mask = 0;
    Switch_Read(switch_id, &mask);
    SCPI_ResultInt32(context, mask);
    return SCPI_RES_OK;
}

scpi_choice_def_t link_source[] = {
    {"ERR", 0},
    {"Port1", 1},
    {"Port2", 2},
    {"Port3", 3},
    {"Port4", 4},
    {"Port5", 5},
    {"Port6", 6},
    {"Port7", 7},
    {"Port8", 8},
    {"Port9", 9},
    {"Port10",10},
    {"Port11",11},
    {"Port12",12},
    {"Port13",13},
    {"Port14",14},
    {"Port15",15},
    {"Port16",16},
    SCPI_CHOICE_LIST_END /* termination of option list */
};

static scpi_result_t SCPI_ConfigureLink(scpi_t *context)
{
    int32_t param;
    const char *name;
    if (!SCPI_ParamChoice(context, link_source, &param, TRUE)) {
        return SCPI_RES_ERR;
    }
    SCPI_ChoiceToName(link_source, param, &name);
    if(Link_Write(1, link_source[param].tag))
    {
        return SCPI_RES_ERR;
    }

    const char * name_read = "LINK ERR";
    uint32_t mask = 0;
    Link_Read(1, &mask);
    name_read = link_source[mask].name;
    if(!strcmp(name_read, name))
    {
        SCPI_ResultCharacters(context, name_read, strlen(name_read));
        context->first_output = 0;
    }
    else
    {
        return SCPI_RES_ERR;
    }
    return SCPI_RES_OK;
}

static scpi_result_t SCPI_ReadLinkState(scpi_t *context)
{
    const char *name = "LINK ERR";
    uint32_t mask = 0;
    Link_Read(1, &mask);
    name = link_source[mask].name;
    
    SCPI_ResultCharacters(context, name, strlen(name));
    
    return SCPI_RES_OK;
}

scpi_choice_def_t cylinder_source[] = {
    {"CLOSE", 0},
    {"OPEN", 1},
    {"CLOSING",2},
    {"OPENING",3},
    {"CLOSED",4},
    {"OPENED",5},
    {"CYL ERR",6},
    SCPI_CHOICE_LIST_END /* termination of option list */
};

static scpi_result_t SCPI_ConfigureCylinder(scpi_t *context)
{
    int32_t number[2] = {0,0};
    uint16_t cylinder_id = 1;
    SCPI_CommandNumbers(context, number, 1, 1);
    if(number[0] != 1)
    {
        cylinder_id = number[0];
    }
    int32_t param;
    const char *name;
    if (!SCPI_ParamChoice(context, cylinder_source, &param, TRUE)) {
        return SCPI_RES_ERR;
    }
    SCPI_ChoiceToName(cylinder_source, param, &name);
    if(Cylinder_Write(cylinder_id, cylinder_source[param]))
    {
        return SCPI_RES_ERR;
    }


    const char *name_read = "CYL ERR";
    scpi_choice_def_t status =Cylinder_Status(1);
    name_read = status.name;
    SCPI_ResultCharacters(context, name_read, strlen(name_read));
    context->first_output = 0;
    return SCPI_RES_OK;
}

static scpi_result_t SCPI_ReadCylinderState(scpi_t *context)
{
    const char *name = "CYL ERR";
    scpi_choice_def_t status =Cylinder_Status(1);
    name = status.name;
    SCPI_ResultCharacters(context, name, strlen(name));
    return SCPI_RES_OK;
}

scpi_choice_def_t lock_source[] = {
    {"UNLOCK", 0},
    {"LOCKED", 1},
    {"LOCK ERR",2},
    SCPI_CHOICE_LIST_END /* termination of option list */
};

static scpi_result_t SCPI_ConfigureLOCK(scpi_t *context)
{
    int32_t param;
    const char *name;
    if (!SCPI_ParamChoice(context, lock_source, &param, TRUE)) {
        return SCPI_RES_ERR;
    }
    SCPI_ChoiceToName(lock_source, param, &name);
    if(Lock_Write(lock_source[param]))
    {
        return SCPI_RES_ERR;
    }

    const char *name_read = "LOCK ERR";
    scpi_choice_def_t status =Lock_Status();
    name_read = status.name;
    SCPI_ResultCharacters(context, name_read, strlen(name_read));
    context->first_output = 0;
    return SCPI_RES_OK;
}

static scpi_result_t SCPI_ReadLOCKState(scpi_t *context)
{
    const char *name = "LOCK ERR";
    scpi_choice_def_t status =Lock_Status();
    name = status.name;
    SCPI_ResultCharacters(context, name, strlen(name));
    return SCPI_RES_OK;
}

scpi_choice_def_t led_source[] = {
    {"OFF", 0},
    {"GREEN", 1},
    {"RED", 2},
    {"YELLOW", 3},
    {"LED ERR", 4},
    SCPI_CHOICE_LIST_END /* termination of option list */
};

static scpi_result_t SCPI_ConfigureLED(scpi_t *context)
{
    int32_t param;
    const char *name;
    if (!SCPI_ParamChoice(context, led_source, &param, TRUE)) {
        return SCPI_RES_ERR;
    }
    SCPI_ChoiceToName(led_source, param, &name);
    if(LED_Write(led_source[param]))
    {
        return SCPI_RES_ERR;
    }

    const char *name_read = "LED ERR";
    scpi_choice_def_t status =LED_Status();
    name_read = status.name;

    SCPI_ResultCharacters(context, name_read, strlen(name_read));
    context->first_output = 0;
    return SCPI_RES_OK;
}

static scpi_result_t SCPI_ReadLEDState(scpi_t *context)
{
    const char *name = "LED ERR";
    scpi_choice_def_t status =LED_Status();
    name = status.name;
    SCPI_ResultCharacters(context, name, strlen(name));
    return SCPI_RES_OK;
}

scpi_choice_def_t sys_source[] = {
    {"LOCK", 0},
    {"IDLE", 1},
    {"READY", 2},
    {"RUNNING", 3},
    {"EMERGENCY", 4},
    {"COMPLETE",5},
    {"SYS ERR",6},
    SCPI_CHOICE_LIST_END /* termination of option list */
};
static scpi_result_t SCPI_ReadSystemState(scpi_t *context)
{
    const char *name = "SYS ERR";
    scpi_choice_def_t status =SYS_Status();
    name = status.name;
    SCPI_ResultCharacters(context, name, strlen(name));
    return SCPI_RES_OK;
}

const scpi_command_t scpi_commands[] = {
    /* IEEE Mandated Commands (SCPI std V1999.0 4.1.1) */
    {
        .pattern = "*CLS",
        .callback = SCPI_CoreCls,
    },
    {
        .pattern = "*IDN?",
        .callback = SCPI_CoreIdnQ,
    },
    {
        .pattern = "*RST",
        .callback = SCPI_CoreRst,
    },
    {
        .pattern = "*STB?",
        .callback = SCPI_CoreStbQ,
    },
    {
        .pattern = "*WAI",
        .callback = SCPI_CoreWai,
    },
    {
        .pattern = "*OPC?",
        .callback = SCPI_CoreOpcQ,
    },
    /* Required SCPI commands (SCPI std V1999.0 4.2.1) */
    {
        .pattern = "SYSTem:ERRor[:NEXT]?",
        .callback = SCPI_SystemErrorNextQ,
    },
    {
        .pattern = "SYSTem:ERRor:COUNt?",
        .callback = SCPI_SystemErrorCountQ,
    },
    {
        .pattern = "CONFigure:SWITch#",
        .callback = SCPI_ConfigureSwitch,
    },
    {
        .pattern = "CONFigure:BAUDrate",
        .callback = SCPI_Configurebaudrate,
    },
    {
        .pattern = "READ:SWITch#:STATe?",
        .callback = SCPI_ReadSwitchState,
    },
    {
        .pattern = "CONFigure:LINK",
        .callback = SCPI_ConfigureLink,
    },
    {
        .pattern = "READ:LINK:STATe?",
        .callback = SCPI_ReadLinkState,
    },
    {
        .pattern = "CONFigure:CYLInder#",
        .callback = SCPI_ConfigureCylinder,
    },
    {
        .pattern = "READ:CYLInder#:STATe?",
        .callback = SCPI_ReadCylinderState,
    },
    {
        .pattern = "CONFigure:LOCK",
        .callback = SCPI_ConfigureLOCK,
    },
    {
        .pattern = "READ:LOCK:STATe?",
        .callback = SCPI_ReadLOCKState,
    },
    {
        .pattern = "CONFigure:LED",
        .callback = SCPI_ConfigureLED,
    },
    {
        .pattern = "READ:LED:STATe?",
        .callback = SCPI_ReadLEDState,
    },
    {
        .pattern = "READ:SYSTem:STATe?",
        .callback = SCPI_ReadSystemState,
    },
    SCPI_CMD_LIST_END};

scpi_interface_t scpi_interface = {
    .error = SCPI_Error,
    .write = SCPI_Write,
    .control = SCPI_Control,
    .flush = SCPI_Flush,
    .reset = SCPI_Reset,
};

char scpi_input_buffer[SCPI_INPUT_BUFFER_LENGTH];
scpi_error_t scpi_error_queue_data[SCPI_ERROR_QUEUE_SIZE];

scpi_t scpi_context;
