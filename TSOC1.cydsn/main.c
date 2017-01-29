#include "project.h"

int main(void)
{
    PWM_1_Start();

    while(1)
    {
        CySysPmSleep();
    }
}
