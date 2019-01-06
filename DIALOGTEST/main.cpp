#pragma once

#include <string>
#include <iostream>
#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <CommCtrl.h>
#include <mmsystem.h>

#include "resource.h"
#include "kinect.h"
#include "tirout.h"
#include "d3d.h"
#include "NPClient.h"
#include "UDPSender.h"

#pragma comment(linker, \
  "\"/manifestdependency:type='Win32' "\
  "name='Microsoft.Windows.Common-Controls' "\
  "version='6.0.0.0' "\
  "processorArchitecture='*' "\
  "publicKeyToken='6595b64144ccf1df' "\
  "language='*'\"")

#pragma comment(lib, "ComCtl32.lib")

LVCOLUMN LvCol;
LVITEM LvItem;
char text[1024];

float lerptime = 0;
float lerprate = 0.001;
T6DOF t6out;
HWND hDlg;
HWND hPrev;
HANDLE pThread;
HANDLE oThread;


float Gravity = 1.9;



DWORD lastupdate = 0;
bool kDown = false;
bool bOutRunning = true;
bool bRunning = true;
bool bPreview = true;

char	*pStr, strPath[255], strTemp[255];
void LoadProfile();
void StartPreviewLoop();
void StartProcessLoop();
bool InitializeOutput();

void InstallDLL()
{

		//install dll
	char dllPath[FILENAME_MAX];
	GetModuleFileName (NULL, strPath, 255);
	pStr = strrchr(strPath, '\\');
	if (pStr != NULL)
		*(++pStr)='\0'; 
	sprintf(dllPath,"%s%s",strPath, "NPClient.dll");


	FILE * pfile;
	pfile = fopen (dllPath,"w+b");
	fwrite (rawData2, sizeof(rawData2), 1, pfile);
	fclose(pfile);


	HKEY hKey;
	LPCTSTR sk = TEXT("Software\\NaturalPoint\\NaturalPoint\\NPClient Location");

	LONG openRes = RegOpenKeyEx(HKEY_CURRENT_USER, sk, 0, KEY_ALL_ACCESS , &hKey);


	LPCTSTR value = TEXT("Path");
	LPCTSTR data = strPath;
					

	LONG setRes = RegSetValueEx (hKey, value, 0, REG_SZ, (LPBYTE)data, strlen(data)+1);
	LONG closeOut = RegCloseKey(hKey);
}
void LoadCombo()
{
	SendMessage(GetDlgItem(hDlg,IDC_COMBO1),CB_RESETCONTENT, 0,0);

	for(int i=0; i<PDATA.NumProfiles;i++){
		SendMessage(GetDlgItem(hDlg,IDC_COMBO1),CB_ADDSTRING, 0,(LPARAM)PDATA.Profiles[i].name);
	}

	 SendMessage(GetDlgItem(hDlg,IDC_COMBO1), CB_SETCURSEL, PDATA.LastProfile, 0);
	 LoadProfile();

}

void UpdateCurrentProfile()
{
	CurrentProfile.Output = SendMessage(GetDlgItem(hDlg, IDC_COMBO_OUTPUT), CB_GETCURSEL, 0, 0);

	DWORD dwAddr;
	SendMessage(GetDlgItem(hDlg, IDC_IPADDRESS), IPM_GETADDRESS, 0, (LPARAM)(LPDWORD)&dwAddr);
	CurrentProfile.IP1 = FIRST_IPADDRESS((LPARAM)dwAddr);
	CurrentProfile.IP2 = SECOND_IPADDRESS((LPARAM)dwAddr);
	CurrentProfile.IP3 = THIRD_IPADDRESS((LPARAM)dwAddr);
	CurrentProfile.IP4 = FOURTH_IPADDRESS((LPARAM)dwAddr);

	HWND hPort = GetDlgItem(hDlg, IDC_EDIT_PORT);
	int len = GetWindowTextLength(hPort) + 1;
	if (len > 1)
	{
		std::string s;
		s.reserve(len);
		GetWindowText(hPort, const_cast<char*>(s.c_str()), len);
		int iPort = std::stoi(s);
		CurrentProfile.Port1 = LOBYTE(iPort);
		CurrentProfile.Port2 = HIBYTE(iPort);
	}

	CurrentProfile.OutputRaw = SendMessage(GetDlgItem(hDlg, IDC_CHECK_OUTPUT_RAW), BM_GETCHECK, 0, 0);
}

void SaveProfile()
{
	
	char pName[64]={0}; 
	GetWindowText(GetDlgItem(hDlg,IDC_COMBO1),pName, 64);
	if(strlen(pName)==0){MessageBox(NULL, "Must enter something.", "KINECT2TIR", 0); return;}
	int pIndex=-1;
	//check if name exists
	for(int i = 0;i<999;i++){
		if (strcmp(pName, PDATA.Profiles[i].name)==0){ pIndex=i;break;}
	}
	//doesnt exist
	if(pIndex==-1) { pIndex=PDATA.NumProfiles;PDATA.NumProfiles++;}
	
	UpdateCurrentProfile();

	//copy into buffer
	strcpy(CurrentProfile.name, pName);
	memcpy(&PDATA.Profiles[pIndex], &CurrentProfile, sizeof(P));
	
	////save profile buffer to file
	char proPath[FILENAME_MAX];
	GetModuleFileName (NULL, strPath, 255);
	pStr = strrchr(strPath, '\\');
	if (pStr != NULL)
		*(++pStr)='\0'; 
	sprintf(proPath,"%s%s",strPath, "profiles.ktp");

	FILE * pfile;
	pfile = fopen (proPath,"w+b");
	fwrite (&PDATA, sizeof(PDATA), 1, pfile);
	fclose(pfile);

	PDATA.LastProfile = pIndex;
	LoadCombo();
}
void DeleteProfile()
{
	char pName[64]={0}; 
	GetWindowText(GetDlgItem(hDlg,IDC_COMBO1),pName, 64);
	if(strlen(pName)==0){MessageBox(NULL, "Nothing to delete.", "KINECT2TIR", 0); return;}
	DWORD pIndex=-1;
	//check if name exists
	for(int i = 0;i<999;i++){
		if (strcmp(pName, PDATA.Profiles[i].name)==0){ pIndex=i;break;}
	}
	//doesnt exist
	if(pIndex==-1) {return;}

	//loop and move elements minus one if i > target
	for(int i = 0;i<998;i++){
		if(i>pIndex)memcpy(&PDATA.Profiles[i-1], &PDATA.Profiles[i], sizeof(P));
	}


	PDATA.NumProfiles--;
	LoadCombo();
}
void LoadProfile()
{

	char pName[64]={0}; 
	int cursel =  SendMessage(GetDlgItem(hDlg,IDC_COMBO1), CB_GETCURSEL, 0, 0);
	PDATA.LastProfile = cursel;

		memcpy(&CurrentProfile, &PDATA.Profiles[cursel], sizeof(P));
		SendMessage(GetDlgItem(hDlg,IDC_SLIDER1), TBM_SETPOS, 1, CurrentProfile.Accel);
		SendMessage(GetDlgItem(hDlg,IDC_SLIDER2), TBM_SETPOS, 1, CurrentProfile.Damping);
		SendMessage(GetDlgItem(hDlg,IDC_SLIDER3), TBM_SETPOS, 1, CurrentProfile.AngleX);
		SendMessage(GetDlgItem(hDlg,IDC_SLIDER4), TBM_SETPOS, 1, CurrentProfile.TransX);
		SendMessage(GetDlgItem(hDlg,IDC_SLIDER5), TBM_SETPOS, 1, CurrentProfile.Filter);

		SendMessage(GetDlgItem(hDlg,IDC_CHECK1), BM_SETCHECK, CurrentProfile.iPitch,0);
		SendMessage(GetDlgItem(hDlg,IDC_CHECK2), BM_SETCHECK,  CurrentProfile.iYaw,0);
		SendMessage(GetDlgItem(hDlg,IDC_CHECK3), BM_SETCHECK,  CurrentProfile.iX,0);
		SendMessage(GetDlgItem(hDlg,IDC_CHECK4), BM_SETCHECK,  CurrentProfile.iY,0);
		SendMessage(GetDlgItem(hDlg,IDC_CHECK5), BM_SETCHECK,  CurrentProfile.iZ,0);

		SendMessage(GetDlgItem(hDlg, IDC_COMBO_OUTPUT), CB_SETCURSEL, CurrentProfile.Output, 0);
		if (CurrentProfile.IP1 | CurrentProfile.IP2 | CurrentProfile.IP3 | CurrentProfile.IP4)
		{
			LPARAM lpAdr = MAKEIPADDRESS(CurrentProfile.IP1, CurrentProfile.IP2, CurrentProfile.IP3, CurrentProfile.IP4);
			SendMessage(GetDlgItem(hDlg, IDC_IPADDRESS), IPM_SETADDRESS, 0, lpAdr);
		}
		if (CurrentProfile.Port1 | CurrentProfile.Port2)
		{
			SetWindowText(GetDlgItem(hDlg, IDC_EDIT_PORT), std::to_string(MAKEWORD(CurrentProfile.Port1, CurrentProfile.Port2)).c_str());
		}

		SendMessage(GetDlgItem(hDlg, IDC_CHECK_OUTPUT_RAW), BM_SETCHECK, CurrentProfile.OutputRaw, 0);
		//Beep(100,100);

		////save profile buffer to file
		char proPath[FILENAME_MAX];
		GetModuleFileName (NULL, strPath, 255);
		pStr = strrchr(strPath, '\\');
		if (pStr != NULL)
			*(++pStr)='\0'; 
		sprintf(proPath,"%s%s",strPath, "profiles.ktp");

		FILE * pfile;
		pfile = fopen (proPath,"w+b");
		fwrite (&PDATA, sizeof(PDATA), 1, pfile);
		fclose(pfile);

}

void LoadProfiles()
{
	//load profile buffer from file

	char proPath[FILENAME_MAX];
	GetModuleFileName (NULL, strPath, 255);
	pStr = strrchr(strPath, '\\');
	if (pStr != NULL)
		*(++pStr)='\0'; 
	sprintf(proPath,"%s%s",strPath, "profiles.ktp");

	FILE * pfile;
	pfile = fopen (proPath,"rb");
	if(pfile){
		fseek(pfile, 0L, SEEK_END);
		DWORD sz = ftell(pfile);
		fseek(pfile, 0L, SEEK_SET);

		if(sz>0){
		fread (&PDATA, sizeof(PDATA), 1, pfile);
		}
		fclose(pfile);
	}
	
	LoadCombo();
}

INT_PTR CALLBACK DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
		case WM_ACTIVATEAPP:
            {
                //if(wParam)
					//if(D3DDevice)
                    //D3DDevice->Reset(&d3dpp);
			}
			break;
		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDCANCEL:
					{
						SendMessage(hDlg, WM_CLOSE, 0, 0);
						return TRUE;
						break;
					}

				case IDC_BUTTON1:
					{
						memcpy(&KDATACENTER,&KDATACURRENT,sizeof(KDATA));
						break;
					}
				case IDC_BUTTON2:
					{
						InstallDLL();
						break;
					}
				case IDC_BUTTON3:
					{
						SaveProfile();
						break;
					}

				case IDC_BUTTON4:
					{
						DeleteProfile();
						break;
					}
				case IDC_BUTTON5:
					{
						bRunning=0;
						initialize();
						bRunning=1;
						StartProcessLoop();
						StartPreviewLoop();
						
						break;
					}
				case IDC_COMBO1:
					{
						switch(HIWORD(wParam)) // Find out what message it was
						{
							case CBN_SELCHANGE:
								{
									LoadProfile();
									break;
								}
						}
						break;
					}
				case IDC_COMBO_OUTPUT:
					{
						switch (HIWORD(wParam))
						{
							case CBN_SELCHANGE:
								{
									UpdateCurrentProfile();
									InitializeOutput();
									break;
								}
						}
						break;
					}
				case IDC_CHECK_OUTPUT_RAW:
					{
						UpdateCurrentProfile();
						break;
					}
				case IDC_CHECK_PREVIEW:
					{
						bPreview = SendMessage(GetDlgItem(hDlg, IDC_CHECK_PREVIEW), BM_GETCHECK, 0, 0);
						if (bPreview)
						{
							StartPreviewLoop();
						}
						break;
					}
			}
			break;

		case WM_CLOSE:
			//if(MessageBox(hDlg, TEXT("Close the program?"), TEXT("Close"),
			//  MB_ICONQUESTION | MB_YESNO) == IDYES)
			//{
			bOutRunning = false;
			//WaitForSingleObject(oThread,INFINITE);
			bRunning = false;
			//WaitForSingleObject(pThread,INFINITE);
			
			Sleep(500);
			Cleanup();
			DestroyWindow(hDlg);
			//}
			//return TRUE;
			break;

		case WM_DESTROY:
			//bRunning = false;
			PostQuitMessage(0);
			return TRUE;
			break;
	}

	return FALSE;
}
DWORD WINAPI OutputLoop( LPVOID lpParam ) //thread: waits for window
{
	timeBeginPeriod(1);
	//SendMessage(GetDlgItem(hDlg,IDC_SLIDER1), TBM_SETPOS, 0, 0);
	//SendMessage(GetDlgItem(hDlg,IDC_SLIDER2), TBM_SETPOS, 0, 0);
	//UpdateDeltaTime();

	//initialize timer
	QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSec);
	secondsPerCount = 1.0 / (double)countsPerSec;

	while(bOutRunning){

		if (CurrentProfile.Output == 0 || CurrentProfile.OutputRaw)
		{
			amult = 0.5*(CurrentProfile.AngleX*0.1 + 1.0);
			tmult = 0.1*(CurrentProfile.TransX*0.1 + 1.0);

			//output to game
			if (CurrentProfile.iPitch)
				t6out.position.pitch = -(KDATACURRENT.fPitch - KDATACENTER.fPitch)*amult*10.0;
			else
				t6out.position.pitch = (KDATACURRENT.fPitch - KDATACENTER.fPitch)*amult*10.0;

			t6out.position.roll = 0;//(KDATACURRENT.fRoll-KDATACENTER.fRoll)*amult;

			if (CurrentProfile.iYaw)
				t6out.position.yaw = (KDATACURRENT.fYaw - KDATACENTER.fYaw)*amult*10.0;
			else
				t6out.position.yaw = -(KDATACURRENT.fYaw - KDATACENTER.fYaw)*amult*10.0;

			if (CurrentProfile.iX)
				t6out.position.x = -(KDATACURRENT.fX - KDATACENTER.fX)*tmult*0.8;//KDATASMOOTH.fX*tmult;
			else
				t6out.position.x = (KDATACURRENT.fX - KDATACENTER.fX)*tmult*0.8;//KDATASMOOTH.fX*tmult;

			if (CurrentProfile.iY)
				t6out.position.y = (KDATACURRENT.fY - KDATACENTER.fY)*tmult*0.8;//KDATASMOOTH.fY*tmult;
			else
				t6out.position.y = -(KDATACURRENT.fY - KDATACENTER.fY)*tmult*0.8;//KDATASMOOTH.fY*tmult;

			if (CurrentProfile.iZ)
				t6out.position.z = -(KDATACURRENT.fZ - KDATACENTER.fZ)*tmult*0.8;//KDATASMOOTH.fZ*tmult;
			else
				t6out.position.z = (KDATACURRENT.fZ - KDATACENTER.fZ)*tmult*0.8;//KDATASMOOTH.fZ*tmult;
		}
		else
		{
			t6out.position.pitch = KDATA.fPitch;
			t6out.position.roll = KDATA.fRoll;
			t6out.position.yaw = KDATA.fYaw;
			t6out.position.x = KDATA.fX;
			t6out.position.y = KDATA.fY;
			t6out.position.z = KDATA.fZ;
		}

		if (CurrentProfile.Output == 0)
		{
			sendHeadposeToGame(&t6out);
		}
		else if (CurrentProfile.Output == 1)
		{
			SendUDP(&t6out);
		}
		//sprintf(text, "LERP: %.4f \r\nSMOOTH: %.4f \r\nIN: %.4f %.4f %.4f \r\nOUT: %.4f %.4f %.4f\r\nPHYS: %.4f DIFF: %.4f", (float)HScroll1,(float)HScroll2,KDATA.fPitch,KDATA.fRoll,KDATA.fYaw,KDATASMOOTH.fPitch,KDATASMOOTH.fRoll,KDATASMOOTH.fYaw,KDATACURRENT.fYaw);
		//SetWindowText(GetDlgItem(hDlg,IDC_EDIT1),text);
		Sleep(10);
	}
	return 0;
}

DWORD WINAPI PreviewLoop( LPVOID lpParam )
{
	while(bRunning && bPreview)
	{
		//gui stuff
		//Render();



		//CurrentProfile.iPitch = SendMessage(GetDlgItem(hDlg,IDC_CHECK1), BM_GETCHECK, 0, 0);
		//Update the delta time 
		if(lastupdate<GetTickCount())
		{
			lastupdate=GetTickCount()+100;
			//recenter
			if(GetAsyncKeyState(VK_F12)){
				if(kDown==false){
					memcpy(&KDATACENTER,&KDATACURRENT,sizeof(KDATA));
					kDown=true;
				}
			}else{kDown=false;}
			//update scrollbar
			HScroll1 = (float)SendMessage(GetDlgItem(hDlg,IDC_SLIDER1), TBM_GETPOS, 0, 0);
			HScroll2 = (float)SendMessage(GetDlgItem(hDlg,IDC_SLIDER2), TBM_GETPOS, 0, 0);
			HScroll3 = (float)SendMessage(GetDlgItem(hDlg,IDC_SLIDER3), TBM_GETPOS, 0, 0);
			HScroll4 = (float)SendMessage(GetDlgItem(hDlg,IDC_SLIDER4), TBM_GETPOS, 0, 0);
			HScroll5 = (float)SendMessage(GetDlgItem(hDlg,IDC_SLIDER5), TBM_GETPOS, 0, 0);

			//update current profile
			CurrentProfile.Accel = HScroll1;
			CurrentProfile.Damping = HScroll2;
			CurrentProfile.AngleX = HScroll3;
			CurrentProfile.AngleY = HScroll3;
			CurrentProfile.AngleZ = HScroll3;
			CurrentProfile.TransX = HScroll4;
			CurrentProfile.TransY = HScroll4;
			CurrentProfile.TransZ = HScroll4;
			CurrentProfile.Filter = HScroll5;
			CurrentProfile.iPitch = SendMessage(GetDlgItem(hDlg,IDC_CHECK1), BM_GETCHECK, 0, 0);
			CurrentProfile.iYaw = SendMessage(GetDlgItem(hDlg,IDC_CHECK2), BM_GETCHECK, 0, 0);
			CurrentProfile.iX = SendMessage(GetDlgItem(hDlg,IDC_CHECK3), BM_GETCHECK, 0, 0);
			CurrentProfile.iY = SendMessage(GetDlgItem(hDlg,IDC_CHECK4), BM_GETCHECK, 0, 0);
			CurrentProfile.iZ = SendMessage(GetDlgItem(hDlg,IDC_CHECK5), BM_GETCHECK, 0, 0);

		}

		//render crap
		Render();
		Sleep(1);
	}
	
	return 0;
}
DWORD WINAPI PhysicsLoop( LPVOID lpParam ) //thread: waits for window
{
	while(bRunning){

		UpdateDeltaTime();

		//Update the physics
		PhysicsApplyForce();
	}
	return 0;
}
DWORD WINAPI ProcessLoop( LPVOID lpParam ) //thread: waits for window
{
	while(bRunning){
		
		process();
		

		//if(g_means.size()>0){
		//	
		//	cout << g_means.size() << " " << flush;

		//	for(uint v=0;v<6;v++)
		//		cout << g_means[0][v] << " ";
		//	cout << endl;
		//}
		//g_frame_no++;

		

	}
	return 0;
}
void StartProcessLoop()
{
		DWORD dwThreadId, dwThrdParam = -1; 
		

		pThread = CreateThread( 
			NULL,                        // no security attributes 
			0,                           // use default stack size  
			ProcessLoop,                  // thread function 
			&dwThrdParam,                // argument to thread function 
			0,                           // use default creation flags 
			&dwThreadId);                // returns the thread identifier 
}

void StartPhysicsLoop()
{
		DWORD dwThreadId, dwThrdParam = -1; 
		

		pThread = CreateThread( 
			NULL,                        // no security attributes 
			0,                           // use default stack size  
			PhysicsLoop,                  // thread function 
			&dwThrdParam,                // argument to thread function 
			0,                           // use default creation flags 
			&dwThreadId);                // returns the thread identifier 
}
void StartPreviewLoop()
{
		DWORD dwThreadId, dwThrdParam = -1; 
		HANDLE hThread; 
		hThread = CreateThread( 
			NULL,                        // no security attributes 
			0,                           // use default stack size  
			PreviewLoop,                  // thread function 
			&dwThrdParam,                // argument to thread function 
			0,                           // use default creation flags 
			&dwThreadId);                // returns the thread identifier 
}
void StartOutputLoop()
{
		DWORD dwThreadId, dwThrdParam = -1; 


		oThread = CreateThread( 
			NULL,                        // no security attributes 
			0,                           // use default stack size  
			OutputLoop,                  // thread function 
			&dwThrdParam,                // argument to thread function 
			0,                           // use default creation flags 
			&dwThreadId);                // returns the thread identifier 
}
void DrawBitmap(HWND hWnd)
{
	HBITMAP hBitmap;

	// From File:
	//hBitmap = (HBITMAP)LoadImage(NULL, "myimage.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);

	// From Resource:
	//hBitmap = LoadBitmap(MAKEINTRESOURCE(IDB_MYBMP));

	//Ok we just loaded the .BMP...

	BITMAP BMP;
	GetObject(preview, sizeof(BMP), &BMP); // Here we get the BMP header info.

	HDC BMPDC = CreateCompatibleDC(NULL); // This will hold the BMP image itself.

	HDC hDC = GetDC(hWnd);
	SelectObject(BMPDC, preview); // Put the image into the DC.

	//BitBlt(hDC, 0, 0, BMP.bmWidth, BMP.bmHeight, BMPDC, 0, 0, SRCCOPY); // Finally, Draw it
	BitBlt(hDC, 0, 0, 100, 100, BMPDC, 0, 0, SRCCOPY); // Finally, Draw it

	ReleaseDC(hWnd,hDC);

	// Don't forget to clean up!
	DeleteDC(BMPDC);
	DeleteObject(hBitmap);

}

bool InitializeOutput()
{
	int curOutput = SendMessage(GetDlgItem(hDlg, IDC_COMBO_OUTPUT), CB_GETCURSEL, 0, 0);
	if (curOutput == 0)
	{
		CloseUDPSender();

		return CreateMapping();
	}
	else if (curOutput == 1)
	{
		DestroyMapping();
		
		return InitUDPSender(
			MAKELONG(MAKEWORD(CurrentProfile.IP1, CurrentProfile.IP2), MAKEWORD(CurrentProfile.IP3, CurrentProfile.IP4)),
			MAKEWORD(CurrentProfile.Port1, CurrentProfile.Port2));
	}
	return false;
}

int WINAPI _tWinMain(HINSTANCE hInst, HINSTANCE h0, LPTSTR lpCmdLine, int nCmdShow)
{
  //HWND hDlg;
  MSG msg;
  BOOL ret;

  InitCommonControls();

  hDlg = CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_DIALOG1), 0, DialogProc, 0);

  HWND hComboBox = GetDlgItem(hDlg, IDC_COMBO_OUTPUT);
  ComboBox_AddString(hComboBox, _T("TrackIR"));
  ComboBox_AddString(hComboBox, _T("UDP"));

  SendMessage(GetDlgItem(hDlg, IDC_IPADDRESS), IPM_SETADDRESS, 0, MAKEIPADDRESS(127, 0, 0, 1));
  SetWindowText(GetDlgItem(hDlg, IDC_EDIT_PORT), _T("5550"));

  SendMessage(GetDlgItem(hDlg, IDC_CHECK_PREVIEW), BM_SETCHECK, 1, 0);

  initialize();


  HICON t = LoadIcon(GetModuleHandle( NULL ), MAKEINTRESOURCE(IDR_MAINFRAME)); 
  SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)t);
  SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)t);

  ShowWindow(hDlg, nCmdShow);


  StartProcessLoop();

  StartOutputLoop();

  StartPreviewLoop();

  StartPhysicsLoop();

  LoadProfiles();

  InitializeOutput();

  while((ret = GetMessage(&msg, 0, 0, 0)) != 0) {

    if(ret == -1)
      return -1;

    if(!IsDialogMessage(hDlg, &msg)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }




  }

  DestroyMapping();
  CloseUDPSender();

  return 0;
}





		//smoother = 8;
		//if (smoother>800)smoother=800;
		////smooth data (moving average)
		//memcpy(&KDATATEMP[rptr], &KDATA , sizeof(KDATA));
		//
		//rptr++;
		//if(rptr>=smoother) rptr=0;

		//ZeroMemory(&KDATATEMP[99], sizeof(KDATA));
		//int ptr = 0;
		//for(ptr; ptr < smoother;ptr++)
		//{
		//	KDATATEMP[999].fPitch += KDATATEMP[ptr].fPitch;
		//	KDATATEMP[999].fYaw += KDATATEMP[ptr].fYaw;
		//	KDATATEMP[999].fRoll += KDATATEMP[ptr].fRoll;
		//	KDATATEMP[999].fX += KDATATEMP[ptr].fX;
		//	KDATATEMP[999].fY += KDATATEMP[ptr].fY;
		//	KDATATEMP[999].fZ += KDATATEMP[ptr].fZ;
		//}
		//KDATATEMP[999].fPitch/=(float)smoother;
		//KDATATEMP[999].fYaw/=(float)smoother;
		//KDATATEMP[999].fRoll/=(float)smoother;
		//KDATATEMP[999].fX/=(float)smoother;
		//KDATATEMP[999].fY/=(float)smoother;
		//KDATATEMP[999].fZ/=(float)smoother;




		//if(lerptime > 1){
		//	memcpy(&KDATATEMP[997], &KDATATEMP[998], sizeof(KDATA));
		//	memcpy(&KDATATEMP[998], &KDATATEMP[999], sizeof(KDATA));

		//	lerptime = 0;
		//}

		//lerptime+=lerprate;

		////KDATASMOOTH.fPitch = KDATATEMP[97].fPitch + ((KDATATEMP[98].fPitch - KDATATEMP[97].fPitch) * lerptime);
		////KDATASMOOTH.fYaw = KDATATEMP[97].fYaw + ((KDATATEMP[98].fYaw - KDATATEMP[97].fYaw) * lerptime);
		////KDATASMOOTH.fRoll = KDATATEMP[97].fRoll + ((KDATATEMP[98].fRoll - KDATATEMP[97].fRoll) * lerptime);
		////KDATASMOOTH.fX = KDATATEMP[97].fX + ((KDATATEMP[98].fX - KDATATEMP[97].fX) * lerptime);
		////KDATASMOOTH.fY = KDATATEMP[97].fY + ((KDATATEMP[98].fY - KDATATEMP[97].fY) * lerptime);
		////KDATASMOOTH.fZ = KDATATEMP[97].fZ + ((KDATATEMP[98].fZ - KDATATEMP[97].fZ) * lerptime);

		//float multt = 0.5;

		//KDATASMOOTH.fPitch = (int)(KDATATEMP[997].fPitch*multt) + (((int)(KDATATEMP[998].fPitch*multt) - (int)(KDATATEMP[997].fPitch*multt)) * lerptime);
		//KDATASMOOTH.fYaw = (int)(KDATATEMP[997].fYaw*multt) + (((int)(KDATATEMP[998].fYaw*multt) - (int)(KDATATEMP[997].fYaw*multt)) * lerptime);
		//KDATASMOOTH.fRoll = (int)(KDATATEMP[997].fRoll*multt) + (((int)(KDATATEMP[998].fRoll*multt) - (int)(KDATATEMP[997].fRoll*multt)) * lerptime);
		//KDATASMOOTH.fX = (int)(KDATATEMP[997].fX*multt) + (((int)(KDATATEMP[998].fX*multt) - (int)(KDATATEMP[997].fX*multt)) * lerptime);
		//KDATASMOOTH.fY = (int)(KDATATEMP[997].fY*multt) + (((int)(KDATATEMP[998].fY*multt) - (int)(KDATATEMP[997].fY*multt)) * lerptime);
		//KDATASMOOTH.fZ = (int)(KDATATEMP[997].fZ*multt) + (((int)(KDATATEMP[998].fZ*multt) - (int)(KDATATEMP[997].fZ*multt)) * lerptime);