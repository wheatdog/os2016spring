#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "fifo.h"
#include "sjf.h"
#include "psjf.h"
#include "rr.h"

#define ArrayCount(Array) sizeof(Array)/sizeof((Array)[0])

enum
{
    POLICY_FIFO = 0,
    POLICY_SJF = 1,
    POLICY_PSJF = 2,
    POLICY_RR = 3,
};

int main()
{
    printf("in main\n");
    char DesiredPolicy[5];
    scanf("%s", DesiredPolicy);

    char *Policies[] = {"FIFO", "SJF", "PSJF", "RR"};
    int Type = -1;
    for (int Index = 0; Index < ArrayCount(Policies); ++Index)
    {
        if (strcmp(Policies[Index], DesiredPolicy) == 0)
        {
            Type = Index;
            break;
        }
    }

    printf("Policy is %d\n", Type);

    if (Type < 0)
    {
        printf("No such policy.\n");
        exit(0);
    }

    switch(Type)
    {
        case POLICY_FIFO:
        {
            FIFO();
        } break;

        case POLICY_SJF:
        {
            SJF();
        } break;

        case POLICY_PSJF:
        {
            printf("test\n");
            PSJF();
        } break;

        case POLICY_RR:
        {
            RR();
        } break;
    }

    return 0;
}
