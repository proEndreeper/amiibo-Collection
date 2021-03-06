#include <stdio.h>
#include <cstdio>
#include <string.h>
#include <string>
#include <vector>
#include <fstream>
#include <3ds.h>
#include <inttypes.h>
#include "amiiboData.hpp"

#define NUM_THREADS 2

#define STACKSIZE (4 * 1024)

Result ret=0;
u32 pos;
FILE *f;
size_t tmpsize;

NFC_TagState prevstate, curstate;
NFC_AmiiboConfig amiiboconfig;
PrintConsole topScreen, bottomScreen;
bool dataLoaded = 0;
bool scan = 1;

void nfc()
{

    consoleSelect(&bottomScreen);
    ret = nfcGetTagState(&curstate);
    if(R_FAILED(ret))
    {
        printf("nfcGetTagState() failed.\n");
    }
    if(curstate!=prevstate)
    {
        printf("TagState changed from %d to %d.\n", prevstate, curstate);
        prevstate = curstate;
    }

    if(curstate==NFC_TagState_Uninitialized)
    {

        nfcInit(NFC_OpType_NFCTag);

    }

    else if(curstate==NFC_TagState_ScanningStopped)
    {

        	ret = nfcStartScanning(NFC_STARTSCAN_DEFAULTINPUT);
            if(R_FAILED(ret))
            {
                printf("nfcStartScanning() failed.\n");
            }
            else
            {
                printf("Currently Scanning please tap an amiibo\n");
            }

            dataLoaded = 0;

    }

    else if(curstate==NFC_TagState_InRange)
    {

                ret = nfcLoadAmiiboData();
				if(R_FAILED(ret))
				{
					printf("nfcLoadAmiiboData() failed.\n");
				}

				dataLoaded = 0;

    }

    else if(curstate==NFC_TagState_DataReady&&!dataLoaded)
    {

        memset(&amiiboconfig, 0, sizeof(NFC_AmiiboConfig));
        ret = nfcGetAmiiboConfig(&amiiboconfig);
        if(R_FAILED(ret))
        {
            printf("nfcGetAmiiboConfig() failed.\n");
        }

        printf("Data loaded and printed on top screen.\n");

        consoleSelect(&topScreen);
        consoleInit(GFX_TOP, NULL);

        printf("Collection : %s\n\n", getCollection(amiiboconfig).c_str());

        printf("Character : %s\n\n", getCharacter(amiiboconfig).c_str());

        printf("Variant : %s\n\n", getVariant(amiiboconfig).c_str());

        printf("Color : %s\n\n", getColor(amiiboconfig).c_str());

        printf("Series : %s\n\n", getSeries(amiiboconfig).c_str());

        printf("Amiibo ID : %04x\n\n", amiiboconfig.amiiboID);

        printf("Type : : %s\n\n", getType(amiiboconfig).c_str());

        dataLoaded = 1;


    }

    else if(curstate==NFC_TagState_OutOfRange)
    {

        printf("NFC tag is now out of range");
        dataLoaded = 0;
        nfcStopScanning();

    }

}

void nfcLoop(void *arg)
{
    while(scan)
    {
        nfc();
        svcSleepThread(2000000000);
    }
}

int main()
{
	// Initialize services
	gfxInitDefault();

	consoleInit(GFX_TOP, &topScreen);
	consoleInit(GFX_BOTTOM, &bottomScreen);
    consoleSelect(&bottomScreen);

	Result rc = romfsInit();

	if (rc)
		printf("romfsInit: %08lX\n", rc);
	else
	{
		printf("romfs Init Successful!\n");
	}
	s32 prio;

	rc = svcGetThreadPriority(&prio, CUR_PROCESS_HANDLE);

	if (rc)
		printf("PriorityInit: %08lX (%08lX)\n", prio, rc);
	else
	{
		printf("Priority Init Successful! %08lX\n", prio);
	}

	//Load database

	loadData();

	Thread nfcThread;

	nfcThread = threadCreate(nfcLoop,0, STACKSIZE, 0x3F,-2,true);

	svcSetThreadPriority(prio-1,threadGetHandle(nfcThread));

	// Main loop
	while (aptMainLoop())
	{
		gspWaitForVBlank();
		hidScanInput();

		u32 kDown = hidKeysDown();
		if (kDown & KEY_START) {
			printf("Exiting...\n");
			break; // break in order to return to hbmenu
		}

		gfxFlushBuffers();
		gfxSwapBuffers();
	}
	printf("Waiting for NFC...\n");
	scan=0;
	nfcStopScanning();
	threadFree(nfcThread);

	printf("Waiting for GFX...\n");
	gfxExit();
	return 0;
}
