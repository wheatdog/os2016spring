#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "fifo.h"
#include "sjf.h"
#include "psjf.h"

#define ArrayCount(Array) sizeof(Array)/sizeof((Array)[0])

enum
{
    POLICY_FIFO,
    POLICY_SJF,
    POLICY_PSJF,
    POLICY_RR,
};

int main()
{
    char DesiredPolicy[5];
    scanf("%s", DesiredPolicy);

    char *Policies[] = {"FIFO", "SJF", "PSJF", "RR"};
    int Type = -1;
    for (int Index = 0; Index < ArrayCount(Policies); ++Index)
    {
        if (strcmp(Policies[Index], DesiredPolicy))
        {
            Type = Index;
            break;
        }
    }

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
            PSJF();
        } break;

        case POLICY_RR:
        {
            break;
        } break;
    }

    return 0;
}
