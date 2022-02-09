/**
 * Author:    Bart Lammers
 * Contact:   btlammers2@gmail.com
 * Created:   02.09.2022
 * 
 **/

#include <stdio.h>
#include <Windows.h>
#include <LabJackUD.h>
#include "LabJackSignalGenerator.h"

void CALLBACK print(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
    // Callbackfunction to periodically print delay and jitter information
    signalGenerator* classpointer = (signalGenerator*)dwUser;
    printf("delay: %i microseconds\n", classpointer->mean_delay);
    printf("jitter: %i microseconds\n", classpointer->jitter);

}


void ErrorHandler(LJ_ERROR lngErrorcode, long lngLineNumber, long lngIteration)
{
    //LabJack error handler function
    char err[255];

    if (lngErrorcode != LJE_NOERROR)
    {
        ErrorToString(lngErrorcode, err);
        printf("Error number = %d\n", lngErrorcode);
        printf("Error string = %s\n", err);
        printf("Source line number = %d\n", lngLineNumber);
        printf("Iteration = %d\n\n", lngIteration);
        if (lngErrorcode > LJE_MIN_GROUP_ERROR)
        {
            //Quit if this is a group error.
            (void)getchar();
            exit(0);
        }
    }
}

LJ_HANDLE initLabJac(double pinNum)
{
    //Make connection with LabJack and return the labJack handle
    LJ_HANDLE lngHandle = 0;

    LJ_ERROR lngErrorcode = OpenLabJack(LJ_dtU3, LJ_ctUSB, "1", 1, &lngHandle);
    ErrorHandler(lngErrorcode, __LINE__, 0);
    //Specify where the LJTick-DAC is plugged in.
    //This is just setting a parameter in the driver, and not actually talking
    //to the hardware, and thus executes very fast.
    lngErrorcode = ePut(lngHandle, LJ_ioPUT_CONFIG, LJ_chTDAC_SCL_PIN_NUM, pinNum, 0);
    ErrorHandler(lngErrorcode, __LINE__, 0);
    lngErrorcode = AddRequest(lngHandle, LJ_ioPUT_CONFIG, LJ_chI2C_SPEED_ADJUST, 0, 0, 0);
    ErrorHandler(lngErrorcode, __LINE__, 0);
    //Execute the configuration requests.
    GoOne(lngHandle);
    return lngHandle;
}



int main()
{

    //Set higher priority class to reduce jitter
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
    if (timeBeginPeriod(1) == TIMERR_NOERROR) {
        printf("succes setting timeBeginPeriod\n");
    }
    else {
        printf("error setting timeBeginPeriod\n");
    }
    MSG msg;
    double pinNum = 4;
    LJ_HANDLE lngHandle = 0;

    lngHandle = initLabJac(pinNum);
    
    //Set settings for instance of signalGenerator
    signalGenerator umosaGen;
    umosaGen.setSignalAmplitude(1);
    umosaGen.setSigFrequency(32);
    umosaGen.setHandle(lngHandle);
    umosaGen.setIdleAmpl(1);
    umosaGen.setStartSequence(0b10, 2);
    umosaGen.setStopSequence(0b01, 2);
    umosaGen.startSignal(1);

    timeSetEvent(3000, 10, (LPTIMECALLBACK)&print, (DWORD_PTR)&umosaGen, TIME_PERIODIC);

    while (true) {
        (void)getchar();
        printf("Sending\n");
        umosaGen.sendMessage(16909515400900422314, 64);
    }


    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    umosaGen.killTimer();

    return 0;
}

