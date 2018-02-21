/**
 * AS - the open source Automotive Software on https://github.com/parai
 *
 * Copyright (C) 2017  AS <parai@foxmail.com>
 *
 * This source code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by the
 * Free Software Foundation; See <http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt>.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 */
/* ============================ [ INCLUDES  ] ====================================================== */
#include "kernel_internal.h"
#include "asdebug.h"
/* ============================ [ MACROS    ] ====================================================== */
#define AS_LOG_OS 0
/* ============================ [ TYPES     ] ====================================================== */
/* ============================ [ DECLARES  ] ====================================================== */
/* ============================ [ DATAS     ] ====================================================== */
uint32 ISR2Counter;
/* ============================ [ LOCALS    ] ====================================================== */
/* ============================ [ FUNCTIONS ] ====================================================== */
void Os_PortActivate(void)
{
	/* get internal resource or NON schedule */
	RunningVar->priority = RunningVar->pConst->runPriority;

	ASLOG(OS, "%s(%d) is running\n", RunningVar->pConst->name,
			RunningVar->pConst->initPriority);

	OSPreTaskHook();

	CallLevel = TCL_TASK;
	Irq_Enable();

	RunningVar->pConst->entry();

	/* Should not return here */
	TerminateTask();
}

void Os_PortInit(void)
{
	ISR2Counter = 0;
}

void Os_PortInitContext(TaskVarType* pTaskVar)
{
	pTaskVar->context.sp = pTaskVar->pConst->pStack + pTaskVar->pConst->stackSize-4;
	pTaskVar->context.pc = Os_PortActivate;
}

void EnterISR(void)
{
	/* do nothing */
}

void LeaveISR(void)
{
	/* do nothing */
}
#ifdef USE_PTHREAD_SIGNAL
void Os_PortCallSignal(int sig, void (*handler)(int), void* sp)
{
	asAssert(NULL != handler);

	handler(sig);

	/* restore its previous stack */
	RunningVar->context.sp = sp;
}

void Os_PortExitSignalCall(void)
{
	Os_PortStartDispatch();
}

void Os_PortInstallSignal(TaskVarType* pTaskVar, int sig, void* handler)
{
	void* sp;
	uint32* stk;

	sp = pTaskVar->context.sp;
	stk = sp;

	*(--stk) = (uint32_t)Os_PortCallSignal;         /* entry point */
	*(--stk) = (uint32_t)Os_PortExitSignalCall;     /* lr */
	*(--stk) = 0xdeadbeef;                  /* r12 */
	*(--stk) = 0xdeadbeef;                  /* r11 */
	*(--stk) = 0xdeadbeef;                  /* r10 */
	*(--stk) = 0xdeadbeef;                  /* r9 */
	*(--stk) = 0xdeadbeef;                  /* r8 */
	*(--stk) = 0xdeadbeef;                  /* r7 */
	*(--stk) = 0xdeadbeef;                  /* r6 */
	*(--stk) = 0xdeadbeef;                  /* r5 */
	*(--stk) = 0xdeadbeef;                  /* r4 */
	*(--stk) = 0xdeadbeef;                  /* r3 */
	*(--stk) = (uint32_t)sp;                /* r2 */
	*(--stk) = (uint32_t)handler;           /* r1 */
	*(--stk) = (uint32_t)sig;               /* r0 : argument */
	/* cpsr SYSMODE(0x1F) */
	if ((uint32_t)Os_PortCallSignal & 0x01)
		*(--stk) = 0x1F | 0x20;          /* thumb mode */
	else
		*(--stk) = 0x1F;                 /* arm mode   */

	pTaskVar->context.sp = stk;
}
#endif
