#include "recvmain.h"

#define BUFFER_SIZE 15000

IDirectSound8* pDS8 = nullptr;
IDirectSoundBuffer* pDSBuffer = nullptr;
IDirectSoundBuffer8* pDSBuffer8 = nullptr;

//IDirectSoundNotify8* pDSNotify8 = nullptr;

HWND hListenBtn, hDisconnectBtn, hPeakBox;
HANDLE hPlayThread, hControlThread;
int nBytesRecv;

WSADATA wsaData;
SOCKET sInSock;

struct addrinfo* pLocalInfo = nullptr;

char* pPrivateBuffer = (char*) malloc(BUFFER_SIZE / 2);


WAVEFORMATEX wfWaveFormat = 
{
	WAVE_FORMAT_PCM, //Format Tag
	1, //No. of Channels
	44100, //Sample Rate
	88200, //Avg. Bytes per Second
	2, //Block Align
	16, //Bits per Sample
	0 //cbSize
};

DSBUFFERDESC dsBufferDesc = 
{
	sizeof(DSBUFFERDESC),
	DSBCAPS_GLOBALFOCUS | DSBCAPS_CTRLPOSITIONNOTIFY,
	(BUFFER_SIZE / 2), //15,000 bytes
	0,
	&wfWaveFormat,
	0,
	NULL
};

/*
HANDLE hPosHandles[1] = 
{
	CreateEvent(NULL, TRUE, FALSE, L"Half-Buffer"),
	//CreateEvent(NULL, TRUE, FALSE, L"Full-Buffer")
};
*/

void InitializeAudio(HWND hWnd) //Initializes DirectSound
{
	
	if(FAILED(DirectSoundCreate8(NULL, &pDS8, NULL)))
	{
		MessageBoxA(NULL, "Failed to Create DirectSound Object!", "Error", MB_OK | MB_ICONERROR);
		return;
	}
	
	if(FAILED(pDS8 -> SetCooperativeLevel(hWnd, DSSCL_PRIORITY)))
	{
		MessageBoxA(NULL, "Failed to Set Cooperative Level!", "Error", MB_OK | MB_ICONERROR);
		return;
	}

	if(FAILED(pDS8 -> CreateSoundBuffer(&dsBufferDesc, &pDSBuffer, NULL)))
	{
		MessageBoxA(NULL, "Failed to Create Sound Buffer!", "Error", MB_OK | MB_ICONERROR);
		return;
	}

	if(FAILED(pDSBuffer -> QueryInterface(IID_IDirectSoundBuffer8, (LPVOID*)&pDSBuffer8)))
	{
		MessageBoxA(NULL, "Failed to Query for IDirectSoundBuffer8 Interface!", "Error", MB_OK | MB_ICONERROR);
		return;
	}

	/*

	//Set Play Buffer Notifications
	DSBPOSITIONNOTIFY dsNotificationPos[1] = 
	{
		{
			(BUFFER_SIZE / 2),
			hPosHandles[0]
		},

		{
			BUFFER_SIZE,
			hPosHandles[1]
		}

	};

	if(FAILED(pDSBuffer8 -> QueryInterface(IID_IDirectSoundNotify8, (LPVOID*)&pDSNotify8)))
	{
		MessageBoxA(NULL, "Failed to Query for IDirectSoundNotify8 Interface!", "Error", MB_OK | MB_ICONERROR);
		return;
	}

	if(FAILED(pDSNotify8 -> SetNotificationPositions(1, dsNotificationPos)))
	{
		MessageBoxA(NULL, "Failed to Set Notification Positions!", "Error", MB_OK | MB_ICONERROR);
		return;
	}
	*/
	
}


void Initialize(HWND hWnd) //Initializes UI and WinSock
{
	
	//Initialize WindSock
	if(WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
		MessageBoxA(NULL, "Failed to Initialize WinSock!", "Error", MB_OK | MB_ICONERROR);
		return;
	}

	struct addrinfo hints = {0};

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

	if(getaddrinfo(NULL, LOCAL_PORT, &hints, &pLocalInfo))
	{
		MessageBoxA(NULL, "Failed to Resolve Target Address and Port!", "Error", MB_OK | MB_ICONERROR);
		return;
	}

	sInSock = socket(pLocalInfo -> ai_family, pLocalInfo -> ai_socktype, pLocalInfo -> ai_protocol);

	if(bind(sInSock, pLocalInfo -> ai_addr, pLocalInfo -> ai_addrlen))
	{
		MessageBoxA(NULL, "Failed to bind Local Port!", "Error", MB_OK | MB_ICONERROR);
		return;
	}
	

	//Load UI
	hListenBtn = CreateWindowExA(0, "Button", "Listen", WS_CHILD | WS_VISIBLE, 25, 160, 90, 30, hWnd, (HMENU)BTN_LISTEN, 
		NULL, NULL);
	hDisconnectBtn = CreateWindowExA(0, "Button", "Disconnect", WS_CHILD | WS_VISIBLE | WS_DISABLED, 520, 160, 90, 30, hWnd,
		(HMENU)BTN_DISCONNECT, NULL, NULL);
	hPeakBox = CreateWindowExA(0, "Edit", NULL, WS_CHILD | WS_VISIBLE, 100, 60, 90, 30, hWnd,
		NULL, NULL, NULL);

}


void DirectListen()
{
	EnableWindow(hListenBtn, FALSE);
	EnableWindow(hDisconnectBtn, TRUE);
	CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)ListenThread, NULL, NULL, NULL);
	hPlayThread = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)PlayThread, NULL, NULL, NULL);
	//CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)PlayThread, NULL, NULL, NULL);
	hControlThread = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)ControlThread, NULL, NULL, NULL);
	
}

void DirectDisconnect()
{
	EnableWindow(hDisconnectBtn, FALSE);
	EnableWindow(hListenBtn, TRUE);

	//pDSBuffer8 -> Stop();
	TerminateThread(hControlThread, 0);
	TerminateThread(hPlayThread, 0);
}



void ListenThread()
{
	void* pSoundBuffer = nullptr;
	DWORD dwBufferSize;


	if(listen(sInSock, 1))
	{
		MessageBoxA(NULL, "Failed to Listen from Socket!", "Error", MB_OK | MB_ICONERROR);
		return;
	}

	SOCKET sDirectSock = accept(sInSock, NULL, NULL);
	if(sDirectSock == INVALID_SOCKET)
	{
		MessageBoxA(NULL, "Failed to Accept Connection!", "Error", MB_OK | MB_ICONERROR);
		return;
	}

	while(true)
	{
		/*
		if(recv(sDirectSock, pPrivateBuffer, (BUFFER_SIZE / 2), 0) == 0)
		{
			MessageBoxA(NULL, "Failed to Recieve Data!", "Error", MB_OK | MB_ICONERROR);
			return;
		}
		*/

		nBytesRecv = recv(sDirectSock, pPrivateBuffer, (BUFFER_SIZE / 2), 0);
		if((nBytesRecv == 0) || (nBytesRecv == -1))
		{
			TerminateThread(hPlayThread, 0);
		}

		//Move Recived Data to DirectSound Buffer
		if(FAILED(pDSBuffer8 -> Lock(0, (BUFFER_SIZE / 2), &pSoundBuffer, &dwBufferSize, NULL, NULL, NULL)))
		{
			MessageBoxA(NULL, "Failed to Lock Buffer 1!", "Error", MB_OK | MB_ICONERROR);
			return;
		}

		memcpy(pSoundBuffer, pPrivateBuffer, (BUFFER_SIZE / 2));

		if(FAILED(pDSBuffer8 -> Unlock(pSoundBuffer, dwBufferSize, NULL, 0)))
		{
			MessageBoxA(NULL, "Failed to Unlock Buffer 1!", "Error", MB_OK | MB_ICONERROR);
			return;
		}
	/*	
	if(FAILED(pDSBuffer8 -> Play(0, 0, 0)))
	{
		MessageBoxA(NULL, "Failed to Play Buffer!", "Error", MB_OK | MB_ICONERROR);
		return;
	}
	*/
	}

	return;

}


void PlayThread()
{
	while(true)
	{
		if(FAILED(pDSBuffer8 -> Play(0, 0, 0)))
		{
			MessageBoxA(NULL, "Failed to Play Buffer!", "Error", MB_OK | MB_ICONERROR);
			return;
		}
	}
}

void ControlThread()
{
	char buffer[10];

	while(true)
	{
		_itoa(nBytesRecv, buffer, 10);
		SetWindowTextA(hPeakBox, buffer);
	}
}

/*
void PlayThread()
{

	void* pSoundBuffer = nullptr;
	DWORD dwBufferSize;

	while(true)
	{
		DWORD dwResult = WaitForMultipleObjects(1, hPosHandles, FALSE, INFINITE);

		switch(dwResult)
		{
			case WAIT_OBJECT_0: //First Half of Buffer is Filled
			{
				if(FAILED(pDSBuffer8 -> Lock(0, (BUFFER_SIZE / 2), &pSoundBuffer, &dwBufferSize, NULL, NULL, NULL)))
				{
					MessageBoxA(NULL, "Failed to Lock Buffer 1!", "Error", MB_OK | MB_ICONERROR);
					return;
				}

				memcpy(pPrivateBuffer, pSoundBuffer, (BUFFER_SIZE / 2));

				if(FAILED(pDSBuffer8 -> Unlock(pSoundBuffer, dwBufferSize, NULL, 0)))
				{
					MessageBoxA(NULL, "Failed to Unlock Buffer 1!", "Error", MB_OK | MB_ICONERROR);
					return;
				}

				if(FAILED(pDSBuffer8 -> Play(0, 0, 0)))
				{
					MessageBoxA(NULL, "Failed to Play Buffer!", "Error", MB_OK | MB_ICONERROR);
					return;
				}

			}
			break;
			/*
			case WAIT_OBJECT_0 + 1: //Second Half of Buffer is Filled
			{
				if(FAILED(pDSBuffer8 -> Lock((BUFFER_SIZE / 2), (BUFFER_SIZE / 2), &pSoundBuffer, &dwBufferSize, NULL, NULL, NULL)))
				{
					MessageBoxA(NULL, "Failed to Lock Buffer 2!", "Error", MB_OK | MB_ICONERROR);
					return;
				}

				memcpy(pPrivateBuffer, pSoundBuffer, (BUFFER_SIZE / 2));

				if(FAILED(pDSBuffer8 -> Unlock(pSoundBuffer, dwBufferSize, NULL, 0)))
				{
					MessageBoxA(NULL, "Failed to Unlockn Buffer 2!", "Error", MB_OK | MB_ICONERROR);
					return;
				}

				if(FAILED(pDSBuffer8 -> Play(0, 0, 0)))
				{
					MessageBoxA(NULL, "Failed to Play Buffer!", "Error", MB_OK | MB_ICONERROR);
					return;
				}
			}
			break; 
		}
	}

}
*/

void CleanUp()
{
	
	free(pPrivateBuffer);
	closesocket(sInSock);
	WSACleanup();

	//pDSNotify8 -> Release();
	pDSBuffer8 -> Release();
	pDSBuffer -> Release();
	pDS8 -> Release();
	
}
