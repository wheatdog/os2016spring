#include "printtime.h"
#include "psjf.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/wait.h>

typedef unsigned int uint;

typedef struct
{
    char Name[32];
    uint ReadyTime;
    uint ExecTime;

    int Recorded;
    struct timespec Time[2];

    int Pipefd[2];
    pid_t Pid;
} raw_input;

typedef struct
{
    raw_input **Data;
    int Head;
    int ElementCount;
} ready_queue;

/******************************************************************************/
/* Debug Utilities for this file                                              */
/******************************************************************************/
/* #define DEBUG */
#ifdef DEBUG
#include <assert.h>
#include <wdb/wd_common.h>

static void
PrintRawInput(int InputCount, raw_input *InputArray)
{
    for (int Index = 0; Index < InputCount; ++Index)
    {
        raw_input *Input = InputArray + Index;
        DebugPrint("%s\t%d\t%d\n", Input->Name, Input->ReadyTime, Input->ExecTime);
    }
}

/******************************************************************************/
#else
/******************************************************************************/

#define PrintRawInput(...)
#define DebugPrint(...)
#define assert(...)

/******************************************************************************/
#endif
/******************************************************************************/

static int
CompareByExecTime(const void *A, const void *B)
{
    uint AExecTime = (*(raw_input **)A)->ExecTime;
    uint BExecTime = (*(raw_input **)B)->ExecTime;

    if (AExecTime < BExecTime) return -1;
    else if (AExecTime > BExecTime) return 1;
    return 0;
}

static int
CompareByReadyTime(const void *A, const void *B)
{
    uint AReadyTime = ((raw_input *)A)->ReadyTime;
    uint BReadyTime = ((raw_input *)B)->ReadyTime;

    if (AReadyTime < BReadyTime) return -1;
    else if (AReadyTime > BReadyTime) return 1;
    return 0;
}

static void
WaitForNextProcess(int Counter)
{
    for (int Index = 0; Index < Counter; ++Index)
    {
        volatile unsigned long i; for(i=0;i<1000000UL;i++);
    }
}

static raw_input *
NextDataIn(ready_queue *Queue)
{
    return Queue->Data[Queue->Head + Queue->ElementCount];
}

static void
ChangePriority(pid_t Pid, int Priority)
{
    struct sched_param ProcessParam;
    ProcessParam.sched_priority = Priority;
    sched_setscheduler(Pid, SCHED_FIFO, &ProcessParam);
}

int
PSJF()
{
    ChangePriority(0, 98);

    int ProcessCount;
    scanf("%d", &ProcessCount);

    raw_input *Raw = (raw_input *)malloc(ProcessCount*sizeof(raw_input));
    for (int Index = 0; Index < ProcessCount; ++Index)
    {
        scanf("%s%d%d", Raw[Index].Name, &Raw[Index].ReadyTime, &Raw[Index].ExecTime);
        Raw[Index].Recorded = 0;
        pipe(Raw[Index].Pipefd);
    }

    qsort(Raw, ProcessCount, sizeof(raw_input), CompareByReadyTime);
    PrintRawInput(ProcessCount, Raw);

    ready_queue ReadyQueue = {};
    ReadyQueue.Data = (raw_input **)malloc(ProcessCount*sizeof(raw_input *));
    for (int Index = 0; Index < ProcessCount; ++Index)
    {
        ReadyQueue.Data[Index] = Raw + Index;
    }

    // NOTE(wheatdog): Start processing
    uint TimeCounter = 0;
    int UnReadyCount = ProcessCount;
    int FinishedCount = 0;
    raw_input *Running = 0;

    while (FinishedCount < ProcessCount)
    {
        if (!Running)
        {
            uint WaitTime = NextDataIn(&ReadyQueue)->ReadyTime - TimeCounter;
            WaitForNextProcess(WaitTime);
            TimeCounter += WaitTime;
        }

        if (UnReadyCount && (NextDataIn(&ReadyQueue)->ReadyTime == TimeCounter))
        {
            pid_t ChildPid = fork();
            if (ChildPid < 0)
            {
                fprintf(stderr, "fork error\n");
                exit(0);
            }
            else if (ChildPid != 0)
            {
                NextDataIn(&ReadyQueue)->Pid = ChildPid;
                --UnReadyCount;
                ++ReadyQueue.ElementCount;
            }
            else
            {
                raw_input *NewProcess = NextDataIn(&ReadyQueue);
                dup2(NewProcess->Pipefd[0], 0);
                char Buffer[128];
                snprintf(Buffer, sizeof(Buffer), "%d", NewProcess->ExecTime);
                if (execl("./child", "./child", NewProcess->Name, Buffer, 0) < 0)
                {
                    fprintf(stderr, "exec error\n");
                }
            }
        }

        qsort(ReadyQueue.Data + ReadyQueue.Head,
              ReadyQueue.ElementCount, sizeof(raw_input *), CompareByExecTime);
        Running = ReadyQueue.Data[ReadyQueue.Head];

        int NeedWait = 1;
        uint ThisExecTime = Running->ExecTime;

        if (UnReadyCount)
        {
            uint NextReadyTime = NextDataIn(&ReadyQueue)->ReadyTime;
            if (NextReadyTime < (TimeCounter + ThisExecTime))
            {
                NeedWait = 0;
                ThisExecTime = NextReadyTime - TimeCounter;
            }
        }

        if (!Running->Recorded)
        {
            gettime(&Running->Time[0]);
            Running->Recorded = 1;
        }

        DebugPrint("[%6d] %s run %d\n", TimeCounter, Running->Name, ThisExecTime);

        char Buffer[128] = {};
        snprintf(Buffer, sizeof(Buffer), "%d ", ThisExecTime);
        write(Running->Pipefd[1], Buffer, strlen(Buffer));
        ChangePriority(Running->Pid, 99);
        TimeCounter += ThisExecTime;
        Running->ExecTime -= ThisExecTime;

        if (NeedWait)
        {
            int Status;
            if (waitpid(Running->Pid, &Status, 0) < 0)
            {
                printf("wait error\n");
                exit(0);
            }

            gettime(&Running->Time[1]);
            print_result(Running->Pid, Running->Time);

            ++ReadyQueue.Head;
            ++FinishedCount;
            --ReadyQueue.ElementCount;

            if (!ReadyQueue.ElementCount)
            {
                Running = 0;
            }
        }
    }

    free(ReadyQueue.Data);
    free(Raw);
    return 0;
}
