#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

/******************************************************************************/
/* Debug Utility for this file                                                */
/******************************************************************************/
#define DEBUG
#ifdef DEBUG
#include <wdb/wd_common.h>

typedef struct
{
    char Name[32];
    unsigned int ReadyTime;
    unsigned int ExecTime;
} raw_input;

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
CompareByReadyTime(const void *A, const void *B)
{
    unsigned int AReadyTime = ((raw_input *)A)->ReadyTime;
    unsigned int BReadyTime = ((raw_input *)B)->ReadyTime;

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

int
main()
{
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
    }

    qsort(Raw, ProcessCount, sizeof(raw_input), CompareByReadyTime);
    PrintRawInput(ProcessCount, Raw);

    return 0;
}
