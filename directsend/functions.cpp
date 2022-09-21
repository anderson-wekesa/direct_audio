#include "sendmain.h"

#define BUFFER_SIZE 15000 //88200
char* pPrivateBuffer = (char*) malloc(BUFFER_SIZE / 2);

int nBytesSent;
char buffer[10];

IDirectSoundCapture8* pDSC8 = nullptr;
IDirectSoundCaptureBuffer* pDSCaptureBuffer = nullptr;
IDirectSoundCaptureBuffer8* pDSCaptureBuffer8 = nullptr;
IDirectSoundNotify8* pDSNotify8 = nullptr;

//HANDLE hPosHandles[2];


//------------------------------
#define cEvents  2
LPDIRECTSOUNDNOTIFY8 pDSNotify;
HANDLE rghEvent[cEvents] = { 0 };
DSBPOSITIONNOTIFY  rgdsbpn[cEvents];
//------------------------------



WSADATA wsaData;

WAVEFORMATEX wfCaptureWaveFormat = 
{
	WAVE_FORMAT_PCM, //Format Tag
	1, //No. of Channels
	44100, //Sample Rate
	88200, //Avg. Bytes per Second [Sample Rate * Block Align]
	2, //Block Align [(No. of Channels * Bits per Sample) / 8]
	16, //Bits per Sample
	0 //cbSize
};

DSCBUFFERDESC dsCaptureDesc = 
{
	sizeof(DSCBUFFERDESC),
	0,
	BUFFER_SIZE, //15,000 bytes
	0,
	&wfCaptureWaveFormat,
	0,
	NULL
};

HWND hAddressText, hPort, hConnectBtn, hDisconnectBtn;
LPSTR szTargetAddress = "192.168.0.14"; //nullptr;
LPSTR szTargetPort = "3773"; //nullptr;

SOCKET sOutSock;

struct addrinfo* pAddress = nullptr;

HANDLE hSendThread, hControlThread;


void InitializeAudio() //Initializes DirectSound
{
	
	//Create IDirectSoundCaptureCreate8 Interface Object
	if(FAILED(DirectSoundCaptureCreate8(NULL, &pDSC8, NULL)))
	{
		MessageBoxA(NULL, "Failed to DirectSound Capture Object!", "Error", MB_OK | MB_ICONERROR);
		return;
	}

	//Create Capture Buffer
	if(FAILED(pDSC8 -> CreateCaptureBuffer(&dsCaptureDesc, &pDSCaptureBuffer, NULL)))
	{
		MessageBoxA(NULL, "Failed to Create Capture Buffer!", "Error", MB_OK | MB_ICONERROR);
		return;
	}

	//Obtain IDirectSoundCaptureBuffer8 Interface
	if(FAILED(pDSCaptureBuffer -> QueryInterface(IID_IDirectSoundCaptureBuffer8, (LPVOID*)&pDSCaptureBuffer8)))
	{
		MessageBoxA(NULL, "Failed to Query for IDirectSoundCaptureBuffer8 Interface!", "Error", MB_OK | MB_ICONERROR);
		return;
	}

	//Query for the IDirectSoundNotify8 Interfaces
	if(FAILED(pDSCaptureBuffer8 -> QueryInterface(IID_IDirectSoundNotify8, (LPVOID*)&pDSNotify8)))
	{
		MessageBoxA(NULL, "Failed to Query for IDirectSoundNotify8 Interface!", "Error", MB_OK | MB_ICONERROR);
		return;
	}

	//Set Notification Positions at the Middle and End of the Buffer
	/*
	hPosHandles[0] = CreateEventA(NULL, TRUE, FALSE, "Half-Buffer");
	hPosHandles[1] = CreateEventA(NULL, TRUE, FALSE, "End-Buffer");
	*/



	//---------------------------------------------------
	for (int i = 0; i < cEvents; ++i)
	{
		rghEvent[i] = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (NULL == rghEvent[i])
		{
			MessageBoxA(NULL, "Failed to Create Events!", "Error", MB_OK | MB_ICONERROR);
		}
	}
	//----------------------------------------------------


	rgdsbpn[0].dwOffset = (BUFFER_SIZE / 2) - 1;
	rgdsbpn[0].hEventNotify = rghEvent[0];

	rgdsbpn[1].dwOffset = BUFFER_SIZE - 1;
	rgdsbpn[1].hEventNotify = rghEvent[1];

	rgdsbpn[2].dwOffset = DSBPN_OFFSETSTOP;
	rgdsbpn[2].hEventNotify = rghEvent[2];


	/*
	DSBPOSITIONNOTIFY dsPosNotifies[2] = 
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
	*/

	pDSNotify8->SetNotificationPositions(cEvents, rgdsbpn);
	pDSNotify8->Release();


	/*
	if(FAILED(pDSNotify8 -> SetNotificationPositions(2, dsPosNotifies))) //Why one Position ???????
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

	sOutSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	

	//Load UI
	HWND hAddressBox = CreateWindowExA(0, "Static", "Bytes Sent" /*"Target Address"*/, WS_CHILD | WS_VISIBLE, 10, 10, 110, 20, hWnd, NULL, NULL, NULL);
	hAddressText = CreateWindowExA(0, "Edit", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER, 130, 10, 200, 20, hWnd, NULL, NULL, NULL);
	HWND hPortBox = CreateWindowExA(0, "Static", "Target Port", WS_CHILD | WS_VISIBLE, 420, 10, 110, 20, hWnd, NULL, NULL, NULL);
	hPort = CreateWindowExA(0, "Edit", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER, 510, 10, 100, 20, hWnd, NULL, NULL, NULL);

	hConnectBtn = CreateWindowExA(0, "Button", "Connect", WS_CHILD | WS_VISIBLE, 25, 160, 90, 30, hWnd, (HMENU)BTN_CONNECT, 
		NULL, NULL);
	hDisconnectBtn = CreateWindowExA(0, "Button", "Disconnect", WS_CHILD | WS_VISIBLE | WS_DISABLED, 520, 160, 90, 30, hWnd,
		(HMENU)BTN_DISCONNECT, NULL, NULL);

}


void DirectConnect()
{
	EnableWindow(hConnectBtn, FALSE);
	EnableWindow(hDisconnectBtn, TRUE);
	/*
	int nAddressLength = GetWindowTextLengthA(hAddressText);
	int nPortLenght = GetWindowTextLengthA(hPort);
	GetWindowTextA(hAddressText, szTargetAddress, (nAddressLength + 1));
	GetWindowTextA(hAddressText, szTargetPort, (nPortLenght + 1));
	*/

	if(getaddrinfo(szTargetAddress, szTargetPort, NULL, &pAddress))
	{
		MessageBoxA(NULL, "Failed to Resolve Target Address and Port!", "Error", MB_OK | MB_ICONERROR);
		return;
	}

	if(connect(sOutSock, pAddress -> ai_addr, pAddress -> ai_addrlen))
	{
		MessageBoxA(NULL, "Failed to Connect to Target!", "Error", MB_OK | MB_ICONERROR);
		return;
	}

	//Start Notify/Sending Thread
	hSendThread = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)SendThread, NULL, NULL, NULL);
	if(hSendThread == NULL)
	{
		MessageBoxA(NULL, "Failed to Start Notify/Sending Thread!", "Error", MB_OK | MB_ICONERROR);
		return;
	}

	//Start Control Thread
	hControlThread = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)ControlThread, NULL, NULL, NULL);
	if (hControlThread == NULL)
	{
		MessageBoxA(NULL, "Failed to Start Control Thread!", "Error", MB_OK | MB_ICONERROR);
		return;
	}

	//Begin Audio Capture
	if(FAILED(pDSCaptureBuffer8 -> Start(DSCBSTART_LOOPING)))
	{
		MessageBoxA(NULL, "Failed to Start Audio Capture!", "Error", MB_OK | MB_ICONERROR);
		return;
	}
}

void DirectDisconnect()
{
	EnableWindow(hDisconnectBtn, FALSE);
	EnableWindow(hConnectBtn, TRUE);

	//pDSCaptureBuffer8 -> Stop();

	TerminateThread(hControlThread, 0);
	TerminateThread(hSendThread, 0);

}



void SendThread()
{
	void* pDSBuffer = nullptr;
	DWORD dwBufferSize;

	while(true)
	{
		DWORD dwResult = WaitForMultipleObjects(2, rghEvent /*hPosHandles*/, FALSE, INFINITE);

		switch (dwResult)
		{
			case (WAIT_OBJECT_0 + 0): //First half of Buffer is Full
			{ 
				//MessageBoxA(NULL, "First Half", "Check", MB_OK | MB_ICONINFORMATION);
				if(FAILED(pDSCaptureBuffer8 -> Lock(0, (BUFFER_SIZE / 2), &pDSBuffer, &dwBufferSize, NULL, 0, 0)))
				{
					MessageBoxA(NULL, "Failed to Lock Buffer 1/2!", "Error", MB_OK | MB_ICONERROR);
					return;
				}

				memcpy(pPrivateBuffer, pDSBuffer, (BUFFER_SIZE / 2));

				if(FAILED(pDSCaptureBuffer8 -> Unlock(pDSBuffer, dwBufferSize, NULL, 0)))
				{
					MessageBoxA(NULL, "Failed to Unlock Buffer 1/2", "Error", MB_OK | MB_ICONERROR);
					return;
				}

				ResetEvent(rghEvent[0]);
			}
			break;
			
			case (WAIT_OBJECT_0 + 1): //Second half of Buffer is Full
			{ 
				//MessageBoxA(NULL, "Second Half", "Check", MB_OK | MB_ICONINFORMATION);
				if(FAILED(pDSCaptureBuffer8 -> Lock((BUFFER_SIZE / 2), (BUFFER_SIZE / 2), &pDSBuffer, &dwBufferSize, NULL, 0, 0)))
				{
					MessageBoxA(NULL, "Failed to Lock Buffer 2/2!", "Error", MB_OK | MB_ICONERROR);
					return;
				}

				memcpy(pPrivateBuffer, pDSBuffer, (BUFFER_SIZE / 2));

				if(FAILED(pDSCaptureBuffer8 -> Unlock(pDSBuffer, dwBufferSize, NULL, 0)))
				{
					MessageBoxA(NULL, "Failed to Unlock Buffer 2/2!", "Error", MB_OK | MB_ICONERROR);
					return;
				}

				ResetEvent(rghEvent[1]);
			}
			break;
			
		}

		/*
		if(send(sOutSock, pPrivateBuffer, (BUFFER_SIZE / 2), NULL) < (BUFFER_SIZE / 2)) //Logic error here...
		{
			MessageBoxA(NULL, "Failed to Send Data!", "Error", MB_OK | MB_ICONERROR);
			return;
		}
		*/

		nBytesSent = send(sOutSock, pPrivateBuffer, (BUFFER_SIZE / 2), NULL);
		
	}

}



void ControlThread()
{
	while (true)
	{
		char buffer[10];
		itoa(nBytesSent, buffer, 10);
		//MessageBoxA(NULL, "Sent", buffer, MB_OK | MB_ICONINFORMATION);
		SetWindowTextA(hAddressText, buffer);
	}
}



void CleanUp()
{
	free(pPrivateBuffer);
	closesocket(sOutSock);
	WSACleanup();

	pDSNotify8 -> Release();
	pDSCaptureBuffer8 -> Release();
	pDSCaptureBuffer -> Release();
	pDSC8 -> Release();

}