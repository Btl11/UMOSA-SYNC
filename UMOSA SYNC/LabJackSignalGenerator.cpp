#include <stdio.h>
#include <Windows.h>
#include <math.h>
#include <stdlib.h>
#include <cmath>
#include <cinttypes>
#include <LabJackUD.h>
#include "LabJackSignalGenerator.h"

#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
const double PI = 3.141592653589793238463;






signalGenerator::signalGenerator() {
	QueryPerformanceFrequency(&Frequency);
	QueryPerformanceCounter(&startingTime);
	QueryPerformanceCounter(&now);
	QueryPerformanceCounter(&elapsedMicroseconds);
	Timercallback = &create_sin;
}



void signalGenerator::startSignal(int delta) {
	if (timerID == 0) {
		timerID = timeSetEvent(delta, 0, Timercallback, (DWORD_PTR)this, TIME_PERIODIC);
		updateDelta = delta;
	}
}
void signalGenerator::killTimer() {
	timeKillEvent(timerID);
}

void signalGenerator::sendMessage(uint64_t m, uint8_t length) {
	message = m;
	Mlength = length;
	sendingIndex = -1;
	commandFlag = sendFlag::SENDING;
}


void signalGenerator::setSignalAmplitude(float a) { amplitude = a; }
void signalGenerator::setSigFrequency(uint32_t f) { signalFrequency = f; }
void signalGenerator::setHandle(LJ_HANDLE handle) { lngHandle = handle; } //Register LabJack communication handle
void signalGenerator::setIdleAmpl(double a) { idleAmpl = a; } //Set amplitude of signal when not sending a message
void signalGenerator::setStartSequence(uint8_t seq, uint8_t len) { startSequence = seq; startSequenceLen = len; } //Set start bits and the number of start bits
void signalGenerator::setStopSequence(uint8_t seq, uint8_t len) { stopSequence = seq; stopSequenceLen = len; }//Set stop bits and the number of stop bits

double signalGenerator::modulateSignal()
{
	static double prevPhase = 0;
	if (sinPhase < prevPhase) sendingIndex++; //When phase jumps back to 0, the next symbol should be sent
	prevPhase = sinPhase;
	if (sendingIndex < 0) return idleAmpl * signal;
	double output;
	uint64_t symbol = 0;
	uint64_t mask = 0;
	//Get correct bit, based on messageStage and sendingIndex
	switch (messageStage) {
	case sendStage::STARTSEQUENCE:
		if (sendingIndex + 1 > startSequenceLen) messageStage = sendStage::MESSAGE;
		else {
			mask = (1ULL << (startSequenceLen - 1));
			symbol = ((uint64_t)startSequence << (sendingIndex)) & mask;
			break;
		}
	case sendStage::MESSAGE:
		if (sendingIndex + 1 > startSequenceLen + Mlength) messageStage = sendStage::STOPSEQUENCE;
		else {
			mask = (1ULL << (Mlength - 1));
			symbol = (message << (sendingIndex - startSequenceLen)) & mask;
			break;
		}
	case sendStage::STOPSEQUENCE:
		if (sendingIndex + 1 > startSequenceLen + Mlength + stopSequenceLen) {
			messageStage = sendStage::STARTSEQUENCE;
			commandFlag = sendFlag::IDLE;
			prevPhase = 0;
		}
		else {
			mask = (1ULL << (stopSequenceLen - 1));
			symbol = ((uint64_t)stopSequence << (sendingIndex - startSequenceLen - Mlength)) & mask;
			break;
		}
	}
	//Determine the output based on the bit
	if (symbol == 0) output = 0;
	else output = signal;
	return output;
}


void signalGenerator::calculateTiming()
{
	//Calculates delta between signal updates and determines jitter and mean delay
	QueryPerformanceCounter(&now);
	elapsedMicroseconds.QuadPart = now.QuadPart - startingTime.QuadPart;
	elapsedMicroseconds.QuadPart *= 1000000;
	elapsedMicroseconds.QuadPart /= Frequency.QuadPart;
	QueryPerformanceCounter(&startingTime);
	delay = (int)elapsedMicroseconds.QuadPart - ((static_cast<long long>(updateDelta) * 1000));
	total_delay += abs(delay);
	diff_delay = abs(delay - last_delay);
	total_diff_delay += diff_delay;
	calls++;
	last_delay = (int)delay;
	mean_delay = (int)total_delay / calls;
	jitter = (int)total_diff_delay / calls;
}

void CALLBACK signalGenerator::create_sin(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
	signalGenerator* classpointer = (signalGenerator*)dwUser;
	LJ_HANDLE lngHandle = classpointer->lngHandle;
	double sinFrequency = classpointer->signalFrequency;
	double amplitude = classpointer->amplitude;

	//calculate delta
	classpointer->calculateTiming();
	classpointer->d_theta = (double)(classpointer->elapsedMicroseconds.QuadPart / 1000000.0) * 2.0 * PI * sinFrequency;
	classpointer->sinPhase = std::fmod((classpointer->sinPhase + classpointer->d_theta), 2.0 * PI);
	//calculate carrier
	classpointer->signal = sin(classpointer->sinPhase);
	if (classpointer->commandFlag == signalGenerator::sendFlag::SENDING) {
		classpointer->outputSignal = amplitude * classpointer->modulateSignal();
	}
	else {
		classpointer->outputSignal = classpointer->idleAmpl * classpointer->signal;
	}
	ePut(lngHandle, LJ_ioTDAC_COMMUNICATION, LJ_chTDAC_UPDATE_DACA, classpointer->outputSignal, 0);
}

