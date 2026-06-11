/*
 * cmd_exec.h — VCMD 执行层
 *
 *  将 Vector_Cmd_t 翻译为实际的 WriteIO 调用
 *  所有 IO 操作统一在这里，状态机不直接调 BsmRelay
 */

#ifndef APP_RAMVECTOR_INC_CMD_EXEC_H_
#define APP_RAMVECTOR_INC_CMD_EXEC_H_

#include "ram_vector.h"

void CmdExec_Init(void);
void CmdExec_Execute(Vector_Cmd_t cmd);

#endif /* APP_RAMVECTOR_INC_CMD_EXEC_H_ */
