#pragma once
#include <LabJackUD.h>
#include <Windows.h>
#include <cstdint>

class signalGenerator
{
private:
    //Messaging flags 
    enum class sendFlag { SENDING, IDLE };
    enum class sendStage { STARTSEQUENCE, MESSAGE, STOPSEQUENCE };

    sendFlag commandFlag = sendFlag::IDLE;
    sendStage messageStage = sendStage::STARTSEQUENCE;

    // LabJack communication handle
    LJ_HANDLE lngHandle = 0;

    //Store callback function and its timerID to destroy it later
    LPTIMECALLBACK Timercallback = 0;
    UINT timerID = 0;
    int updateDelta = 0; //timer delta 

    // Signal settings and variables
    double signalFrequency = 32;
    double amplitude = 1;
    double idleAmpl = 1;
    double sinPhase = 0;
    double d_theta = 0;
    double signal = 0;
    double outputSignal = 0;

    //Timing variables(to monitor jitter and delay)
    LARGE_INTEGER startingTime, now, elapsedMicroseconds;
    LARGE_INTEGER Frequency;
    int total_delay = 0;
    long long calls = 0;
    int last_delay = 0;
    int total_diff_delay = 0;
    int delay = 0;
    int diff_delay = 0;

    //Messaging settings and variables
    uint64_t message = 0;
    uint8_t Mlength = 0;
    int8_t sendingIndex = -1;
    uint8_t startSequence = 0;
    uint8_t stopSequence = 0;
    uint8_t startSequenceLen = 0;
    uint8_t stopSequenceLen = 0;

    //Timing function and function that modulates message on carrier
    double modulateSignal();
    void calculateTiming();

public:
    //Timing performance variables (in Microseconds)
    int mean_delay = 0;
    int jitter = 0;

    signalGenerator();

    void startSignal(int delta); //start the signal with timer interval delta (ms)
    void killTimer(); //kill timer made with startsignal()
    void sendMessage(uint64_t m, uint8_t length);//send message m with length 'length'

    static void CALLBACK create_sin(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2);
    void setSignalAmplitude(float a); //Set amplitude when sending message
    void setSigFrequency(uint32_t f);//Set signal frequency 
    void setHandle(LJ_HANDLE handle);//Register LabJack communication handle
    void setIdleAmpl(double a); //Set amplitude of signal when not sending a message
    void setStartSequence(uint8_t seq, uint8_t len); //Set start bits and the number of start bits
    void setStopSequence(uint8_t seq, uint8_t len);//Set stop bits and the number of stop bits

};