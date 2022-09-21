#pragma once

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <dsound.h>

void Initialize(HWND hWnd);

#define LOCAL_PORT "3773"

#define BTN_LISTEN 1
#define BTN_DISCONNECT 2

void InitializeAudio(HWND hWnd);
void Initialize(HWND hWnd);

void DirectListen();
void DirectDisconnect();
void CleanUp();

void ListenThread();
void PlayThread();
void ControlThread();