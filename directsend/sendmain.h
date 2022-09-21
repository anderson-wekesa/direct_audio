#pragma once

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <dsound.h>

#define BTN_CONNECT 1
#define BTN_DISCONNECT 2

void InitializeAudio();
void Initialize(HWND hWnd);

void DirectConnect();
void DirectDisconnect();
void CleanUp();

void SendThread();
void ControlThread();