#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>
#include "printtime.h"

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
//#define DEBUG
#include <wdb/wd_common.h>
#ifdef DEBUG

void
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

#define PrintRawInput

/******************************************************************************/
#endif
/******************************************************************************/

int
CompareByExecTime(const void *A, const void *B)
{
    uint AExecTime = (*(raw_input **)A)->ExecTime;
    uint BExecTime = (*(raw_input **)B)->ExecTime;

    if (AExecTime < BExecTime) return -1;
    else if (AExecTime > BExecTime) return 1;
    return 0;
}

int
CompareByReadyTime(const void *A, const void *B)
{
    uint AReadyTime = ((raw_input *)A)->ReadyTime;
    uint BReadyTime = ((raw_input *)B)->ReadyTime;

    if (AReadyTime < BReadyTime) return -1;
    else if (AReadyTime > BReadyTime) return 1;
    return 0;
}

void
WaitForNextProcess(int Counter)
{
    for (int Index = 0; Index < Counter; ++Index)
    {
        volatile unsigned long i; for(i=0;i<1000000UL;i++);
    }
}

raw_input *
GetDataFromReadyQueue(ready_queue *Queue, int Count)
{
    return Queue->Data[Queue->Head + Count];
}

int
main()
{
    struct sched_param ThisProcessParam;
    ThisProcessParam.sched_priority = 98;
    sched_setscheduler(0, SCHED_FIFO, &ThisProcessParam);

    char SchedulingPolicy[5];
    scanf("%s", SchedulingPolicy);

    assert(SchedulingPolicy[0] == 'P' &&
           SchedulingPolicy[1] == 'S' &&
           SchedulingPolicy[2] == 'J' &&
           SchedulingPolicy[3] == 'F' &&
           SchedulingPolicy[4] == '\0');

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

    // NOTE(wheatdog): Init ReadyQueue

    ready_queue ReadyQueue;
    ReadyQueue.Data = (raw_input **)malloc(ProcessCount*sizeof(raw_input *));
    ReadyQueue.Head = 0;
    ReadyQueue.ElementCount = 0;
    for (int Index = 0; Index < ProcessCount; ++Index)
    {
        ReadyQueue.Data[Index] = Raw + Index;
    }

    // NOTE(wheatdog): Start processing

    uint TimeCounter = 0;
    int FinishedProcessCount = 0;
    raw_input *Running = 0;

    while (FinishedProcessCount < ProcessCount)
    {
        if (!Running)
        {
            DebugPrint("h: %d \tc:%d\n", ReadyQueue.Head, ReadyQueue.ElementCount);
            if (((FinishedProcessCount + ReadyQueue.ElementCount) != ProcessCount))
            {
                uint WaitTime =
                    GetDataFromReadyQueue(&ReadyQueue, ReadyQueue.ElementCount)->ReadyTime -
                    TimeCounter;
                TimeCounter += WaitTime;
                WaitForNextProcess(WaitTime);
            }
        }

        // NOTE(wheatdog): Spawn new process

        if (((FinishedProcessCount + ReadyQueue.ElementCount) != ProcessCount))
        {
            pid_t ChildPid = fork();
            if (ChildPid < 0)
            {
                fprintf(stderr, "fork error\n");
                exit(0);
            }
            else if (ChildPid != 0)
            {
                GetDataFromReadyQueue(&ReadyQueue, ReadyQueue.ElementCount)->Pid = ChildPid;
                ++ReadyQueue.ElementCount;
            }
            else
            {
                raw_input *NewProcess = GetDataFromReadyQueue(&ReadyQueue,
                                                              ReadyQueue.ElementCount);
                dup2(NewProcess->Pipefd[0], 0);
                char Buffer[128];
                snprintf(Buffer, sizeof(Buffer), "%d", NewProcess->ExecTime);
                if (execl("./child", "./child", NewProcess->Name, Buffer, 0) < 0)
                {
                    fprintf(stderr, "exec error\n");
                }
            }
        }

        // NOTE(wheatdog): Decide who to run, if there is time gap between next
        // process, remember to set Running = 0 after writing

        // NOTE(wheatdog): Sort ReadyQueue and choose the top one

        qsort(ReadyQueue.Data + ReadyQueue.Head, ReadyQueue.ElementCount,
              sizeof(raw_input *), CompareByExecTime);

        Running = ReadyQueue.Data[ReadyQueue.Head];

        int NeedWait = 1;
        uint ThisExecTime = Running->ExecTime;

        if (((FinishedProcessCount + ReadyQueue.ElementCount) != ProcessCount))
        {
            uint NextReadyTime = GetDataFromReadyQueue(&ReadyQueue, ReadyQueue.ElementCount)->ReadyTime;
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

        TimeCounter += ThisExecTime;
        Running->ExecTime -= ThisExecTime;

        // NOTE(wheatdog): Send data to Running
        char Buffer[128] = {};
        snprintf(Buffer, sizeof(Buffer), "%d", ThisExecTime);
        write(Running->Pipefd[1], Buffer, sizeof(Buffer));

        struct sched_param Param;
        Param.sched_priority = 99;
        sched_setscheduler(Running->Pid, SCHED_FIFO, &Param);

        if (NeedWait)
        {
            int Status;
            if (waitpid(Running->Pid, &Status, 0) < 0)
            {
                printf("wait error\n");
                exit(0);
            }
            gettime(&Running->Time[1]);
            ++ReadyQueue.Head;
            --ReadyQueue.ElementCount;
            ++FinishedProcessCount;
            print_result(Running->Pid, Running->Time);
            Running = 0;
        }
    }

    free(ReadyQueue.Data);
    free(Raw);
    return 0;
}
