#include <stdio.h>
#include <Windows.h>
#define INTERVAL 1
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

LARGE_INTEGER startingTime, now, elapsedMicroseconds;
LARGE_INTEGER Frequency;
int total_delay = 0;
int calls = 0;
int last_delay = 0;
int total_diff_delay = 0;
int mean_delay = 0;
int jitter = 0;
void CALLBACK f(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{

    QueryPerformanceCounter(&now);
    elapsedMicroseconds.QuadPart = now.QuadPart - startingTime.QuadPart;
    elapsedMicroseconds.QuadPart *= 1000000;
    elapsedMicroseconds.QuadPart /= Frequency.QuadPart;
    QueryPerformanceCounter(&startingTime);
    int delay = elapsedMicroseconds.QuadPart - (INTERVAL * 1000);
    total_delay += abs(delay);
    int diff_delay = abs(delay - last_delay);
    total_diff_delay += diff_delay;
    calls++;
    last_delay = delay;
    mean_delay = total_delay / calls;
    jitter = total_diff_delay / calls;
}

void CALLBACK print(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2) {
    printf("delay: %i\n", mean_delay);
    printf("jitter: %i\n", jitter);
}


int main()
{
    MSG msg;
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
    if (timeBeginPeriod(1) == TIMERR_NOERROR) {
        printf("succes setting timeBeginPeriod\n");
    }
    else {
        printf("error setting timeBeginPeriod\n");
    }
    QueryPerformanceFrequency(&Frequency);
    QueryPerformanceCounter(&startingTime);
    timeSetEvent(INTERVAL, 0, (LPTIMECALLBACK)&f, 0, TIME_PERIODIC);
    timeSetEvent(3000, 10, (LPTIMECALLBACK)&print, 0, TIME_PERIODIC);
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

