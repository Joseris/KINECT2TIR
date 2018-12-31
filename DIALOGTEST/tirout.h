#pragma once

#include "windows.h"
#include "common.h"

#define	NP_AXIS_MIN				-16383
#define	NP_AXIS_MAX				16383

typedef struct tagTrackIRData
{
	unsigned short wNPStatus;
	unsigned short wPFrameSignature;
	unsigned long  dwNPIOData;

	float fNPRoll;
	float fNPPitch;
	float fNPYaw;
	float fNPX;
	float fNPY;
	float fNPZ;
	float fNPRawX;
	float fNPRawY;
	float fNPRawZ;
	float fNPDeltaX;
	float fNPDeltaY;
	float fNPDeltaZ;
	float fNPSmoothX;
	float fNPSmoothY;
	float fNPSmoothZ;
} TRACKIRDATA, *LPTRACKIRDATA;

struct FTIRMemMap {
	// Emulators can check this
	int iRecordSize;
	TRACKIRDATA data;
	int Version;
	// Emulators should read these
	HANDLE RegisteredHandle;
	bool Transmission, Cursor;
	long int RequestFormat;
	long int ProgramProfileID;
	// Read/Write
	int LastError;
	int Param[16];
	unsigned short ClientNotify1, ClientNotify2;
	char Signature[400];
};
typedef FTIRMemMap * PFTIRMemMap;

HANDLE hFTIRMemMap;
FTIRMemMap *pMemData;
HANDLE hFTIRMutex;
bool didMapping=false;
static const char* FTIR_MM_DATA = "{0F98177E-0E5C-4F86-8837-229D19B1701D}";
static const char* FTIR_MUTEX = "FT_TIR_Mutex";


///load program

bool CreateMapping()
{
		hFTIRMemMap = CreateFileMappingA( INVALID_HANDLE_VALUE , 00 , PAGE_READWRITE , 0 , 
		                           sizeof( FTIRMemMap ), 
//		                           sizeof( TRACKIRDATA ) + sizeof( HANDLE ) + 100, 
								   (LPCSTR) FTIR_MM_DATA );

	if ( hFTIRMemMap != 0 ) {
		if ( (long) GetLastError == ERROR_ALREADY_EXISTS ) {
			///qDebug() << "FTIRCreateMapping says: FileMapping already exists!" << hFTIRMemMap;
			CloseHandle( hFTIRMemMap );
			hFTIRMemMap = 0;
		}
		else {
			//qDebug() << "FTIRCreateMapping says: FileMapping newly created!" << hFTIRMemMap;
		}
	}

	//
	// Open the FileMapping, Read/Write access
	//
	pMemData = 0;
	hFTIRMemMap = OpenFileMappingA( FILE_MAP_ALL_ACCESS , false , (LPCSTR) FTIR_MM_DATA );
	if ( ( hFTIRMemMap != 0 ) ) {
		//qDebug() << "FTIRCreateMapping says: FileMapping opened: " << hFTIRMemMap;
//		pMemData = (FTIRMemMap *) MapViewOfFile(hFTIRMemMap, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(TRACKIRDATA) + sizeof(hFTIRMemMap) + 100);
		pMemData = (FTIRMemMap *) MapViewOfFile(hFTIRMemMap, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(FTIRMemMap));
		if (pMemData != NULL) {
			//qDebug() << "FTIRCreateMapping says: View of File mapped: " << pMemData;
			pMemData->RegisteredHandle = 0;	// The game uses the handle, to send a message that the Program-Name was set!
		}
	    hFTIRMutex = CreateMutexA(NULL, false, FTIR_MUTEX);
	}
	else {
		//QMessageBox::information(0, "FaceTrackNoIR error", QString("FTIRServer Error! \n"));
		return false;
	}
	return true;
}
void DestroyMapping()
{
		if ( pMemData != NULL ) {
		UnmapViewOfFile ( pMemData );
	}
	
	if (hFTIRMutex != 0) {
		CloseHandle( hFTIRMutex );
	}
	hFTIRMutex = 0;
	
	if (hFTIRMemMap != 0) {
		CloseHandle( hFTIRMemMap );
	}
	hFTIRMemMap = 0;
}

float scale2AnalogLimits( float x, float min_x, float max_x ) {
double y;
double local_x;
	
	local_x = x;
	if (local_x > max_x) {
		local_x = max_x;
	}
	if (local_x < min_x) {
		local_x = min_x;
	}
	y = ( NP_AXIS_MAX * local_x ) / max_x;

	return (float) y;
}

void sendHeadposeToGame( T6DOF *headpose ) {
float virtPosX;
float virtPosY;
float virtPosZ;

float virtRotX;
float virtRotY;
float virtRotZ;

TRACKIRDATA localdata;

	//
	// Copy the Raw measurements directly to the client.
	//
	virtRotX = scale2AnalogLimits (headpose->position.pitch, -180.0f, 180.0f);
	virtRotY = scale2AnalogLimits (headpose->position.yaw, -180.0f, 180.0f);
	virtRotZ = scale2AnalogLimits (headpose->position.roll, -180.0f, 180.0f);

	virtPosX = scale2AnalogLimits (headpose->position.x * 10.0f, -500.0f, 500.0f);
	virtPosY = scale2AnalogLimits (headpose->position.y * 10.0f, -500.0f, 500.0f);
	virtPosZ = scale2AnalogLimits (headpose->position.z * 10.0f, -500.0f, 500.0f);

	//
	// Check if the pointer is OK and wait for the Mutex.
	// Use the setposition in the (special) DLL, to write the headpose-data.
	//
//	qDebug() << "FTIRCreateMapping says: sendHeadpose";
	if ( (pMemData != NULL) && (WaitForSingleObject(hFTIRMutex, 100) == WAIT_OBJECT_0) ) {
//		qDebug() << "FTIRCreateMapping says: Calling setposition" << setdata;

		//localdata.fNPX = virtPosX;
		//localdata.fNPY = virtPosY;
		//localdata.fNPZ = virtPosZ;
		//localdata.fNPRoll = virtRotZ;
		//localdata.fNPPitch = virtRotX;
		//localdata.fNPYaw = virtRotY;
		//localdata.wPFrameSignature = localdata.wPFrameSignature + 1;

		//setdata(&localdata);
		//
//		setposition ( virtPosX, virtPosY, virtPosZ, virtRotZ, virtRotX, virtRotY );
		pMemData->data.fNPX = virtPosX;
		pMemData->data.fNPY = virtPosY;
		pMemData->data.fNPZ = virtPosZ;
		pMemData->data.fNPRoll = virtRotZ;
		pMemData->data.fNPPitch = virtRotX;
		pMemData->data.fNPYaw = virtRotY;
		pMemData->data.wPFrameSignature +=1;
		if ((pMemData->data.wPFrameSignature < 0) || (pMemData->data.wPFrameSignature > 1000)){
			pMemData->data.wPFrameSignature = 0;
		}
		ReleaseMutex(hFTIRMutex);
	}
}

