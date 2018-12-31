#define USE_MS_SKD  //use MS sdk


#ifndef USE_MS_SKD
	#include <XnOS.h>
	#include <XnCppWrapper.h>
	#include <XnCodecIDs.h>
	using namespace xn;
#else
	#include <d2d1.h>
	#include "NuiApi.h"
	//#include "DepthBasics.h"
#endif

#include "CRForestEstimator.h"
#include <algorithm>
#include <vector>
#include <opencv2/imgproc/imgproc.hpp>

using namespace std;
using namespace cv;


float cAcceleration = 0.0002;  //0.0001
float cTolerance = 2.0;
float cMaxAngleVelocity =1.5;
float cMaxVelocity =5.5;
float fInertialMass = 0.1;

float cDamping = 0.65;


// Path to trees
string g_treepath;
// Number of trees
int g_ntrees;
// Patch width
int g_p_width;
// Patch height
int g_p_height;
//maximum distance form the sensor - used to segment the person
int g_max_z = 0;
//head threshold - to classify a cluster of votes as a head
int g_th = 300;
//threshold for the probability of a patch to belong to a head
float g_prob_th = 1.0f;
//threshold on the variance of the leaves
float g_maxv = 400.f;
//stride (how densely to sample test patches - increase for higher speed)
int g_stride = 10;
//radius used for clustering votes into possible heads
float g_larger_radius_ratio = 1.f;
//radius used for mean shift
float g_smaller_radius_ratio = 5.f;
//
int g_frame_no = 0;
//opengl window size
int w,h;
//pointer to the actual estimator
HBITMAP preview;
CRForestEstimator* g_Estimate;
//input 3D image
Mat g_im3D, g_imD;
//input image size
int g_im_w = 640;
int g_im_h = 480;
//kinect's frame rate
int g_fps = 30;

#ifndef USE_MS_SKD
	XnUInt64 g_focal_length;
	XnDouble g_pixel_size;
	xn::Context g_Context;
	xn::DepthGenerator g_DepthGenerator;
	DepthMetaData g_depthMD;
	XnStatus g_RetVal;
#else
    INuiSensor*             g_pNuiSensor;
	HANDLE                  g_pDepthStreamHandle;
    HANDLE                  g_hNextDepthFrameEvent;
#endif
 //   INuiSensor*             g_pNuiSensor;
	//HANDLE                  g_pDepthStreamHandle;
 //   HANDLE                  g_hEvNuiProcessStop;
	//HANDLE                  g_hNextDepthFrameEvent;
	//HANDLE                  g_hNextVideoFrameEvent;
	//HANDLE                  g_hNextSkeletonEvent;

bool g_first_rigid = true;
bool g_show_votes = false;
bool g_draw_triangles = false;
bool g_draw = true;

std::vector< cv::Vec<float,POSE_SIZE> > g_means; //outputs
std::vector< cv::Vec<float,POSE_SIZE> > g_meansdone; //outputs
std::vector< std::vector< const Vote* > > g_clusters; //full clusters of votes
std::vector< Vote > g_votes; //all votes returned by the forest

typedef struct KINECTDATA
{
	float fRoll;
	float fPitch;
	float fYaw;
	float fX;
	float fY;
	float fZ;

} KINECTDATA;

KINECTDATA KDATA = {0};
KINECTDATA KDATACENTER = {0};
KINECTDATA KDATACURRENT = {0};
KINECTDATA KVELOCITY = {0};
KINECTDATA KFORCE = {0};
KINECTDATA KDIFF = {0};



KINECTDATA KDATASMOOTH = {0};
KINECTDATA KDATAR = {0};
KINECTDATA KDATATEMP[1000] = {0};
DWORD smoother = 100;
DWORD rptr = 0;


//DWORD NumProfiles = 0;

struct P
{
	char name[64];
	float Accel;
	float Filter;
	float Damping;
	float AngleX;
	float AngleY;
	float AngleZ;
	float TransX;
	float TransY;
	float TransZ;
	char iPitch;
	char iYaw;
	char iX;
	char iY;
	char iZ;
	unsigned char Output;
	unsigned char Port1;
	unsigned char Port2;
	unsigned char IP1;
	unsigned char IP2;
	unsigned char IP3;
	unsigned char IP4;
	float res4;
	float res5;
	float res6;
	float res7;
	float res8;
	float res9;
	float res10;
	float res11;
	float res12;
};

struct PFILE
{
	DWORD NumProfiles;
	DWORD LastProfile;
	P Profiles[1000];
};

PFILE PDATA;
//P Profiles[1000];
P CurrentProfile;



float HScroll1;
float HScroll2;
float HScroll3;
float HScroll4;
float HScroll5;
float amult = 4.0;
float tmult = 0.05;
double angsens = 0.03;
double movesens = 0.03;

//Timer
DWORD CurrentTime;
DWORD LastTime;
//double DeltaTime;


//high resolution delta
double DeltaTime;
__int64 currentTime;
__int64 prevTime;
__int64 countsPerSec;
double secondsPerCount;

float cap(float in, float max)
{
	float f = in;

	if(abs(f) > max){
		if(in>0) f = max;
		if(in<0) f = -max;
	}

	return f;

}

double FMTSLD(int value, bool flip = false)
{
	if(flip)
		return ((100-value)*0.01+0.5);
	else
		return value*0.01+0.5;

}


// load config file
void loadConfig(const char* filename) {

	ifstream in(filename);
	string dummy;

	if(in.is_open()) {

		// Path to trees
		in >> dummy;
		in >> g_treepath;

		// Number of trees
		in >> dummy;
		in >> g_ntrees;

		in >> dummy;
		in >> g_maxv;

		in >> dummy;
		in >> g_larger_radius_ratio;

		in >> dummy;
		in >> g_smaller_radius_ratio;

		in >> dummy;
		in >> g_stride;

		in >> dummy;
		in >> g_max_z;

		in >> dummy;
		in >> g_th;


	} else {
		cerr << "File not found " << filename << endl;
		exit(-1);
	}
	in.close();

	cout << endl << "------------------------------------" << endl << endl;
	cout << "Estimation:       " << endl;
	cout << "Trees:            " << g_ntrees << " " << g_treepath << endl;
	cout << "Stride:           " << g_stride << endl;
	cout << "Max Variance:     " << g_maxv << endl;
	cout << "Max Distance:     " << g_max_z << endl;
	cout << "Head Threshold:   " << g_th << endl;

	cout << endl << "------------------------------------" << endl << endl;

}
bool initialize(){
	//char path[255] = ;
	loadConfig("treeconfig.ini");

	g_Estimate =  new CRForestEstimator();
	if( !g_Estimate->load_forest(g_treepath.c_str(), g_ntrees) ){
		//WriteLogFile("Kinect: could not read forest!");
	}
	
	//std::cout << "initializing kinect... " << endl;
#ifdef USE_MS_SKD

	int iSensorCount = 0;
    if ( NuiGetSensorCount(&iSensorCount) < 0 )
		return false;

    // Look at each Kinect sensor
    for (int i = 0; i < iSensorCount; ++i)
    {
        // Create the sensor so we can check status, if we can't create it, move on to the next
        if ( NuiCreateSensorByIndex(i, &g_pNuiSensor) < 0 ) continue;
      
		// Get the status of the sensor, and if connected, then we can initialize it
        if( 0== g_pNuiSensor->NuiStatus() ){
            g_pNuiSensor = g_pNuiSensor;
            break;
        }

        // This sensor wasn't OK, so release it since we're not using it
        g_pNuiSensor->Release();
    }

    if (NULL != g_pNuiSensor)
    {
        // Initialize the Kinect and specify that we'll be using depth
        if ( g_pNuiSensor->NuiInitialize(NUI_INITIALIZE_FLAG_USES_DEPTH) >= 0 )
        {
            // Create an event that will be signaled when depth data is available
            g_hNextDepthFrameEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

            // Open a depth image stream to receive depth frames
            g_pNuiSensor->NuiImageStreamOpen(
                NUI_IMAGE_TYPE_DEPTH,
                NUI_IMAGE_RESOLUTION_640x480,
                0,
                2,
                g_hNextDepthFrameEvent,
                &g_pDepthStreamHandle);
        }
    }
	else
        return false;
#else
	// Initialize context object
	g_RetVal = g_Context.Init();

	g_RetVal = g_DepthGenerator.Create(g_Context);
	if (g_RetVal != XN_STATUS_OK)
	{
		printf("Failed creating DEPTH generator %s\n", xnGetStatusString(g_RetVal));
		MessageBox(NULL, "Failed creating DEPTH generator. Did you install OpenNI and all its modules Correctly?",NULL,NULL);
		return false;
	}


	XnMapOutputMode outputMode;
	outputMode.nXRes = g_im_w;
	outputMode.nYRes = g_im_h;
	outputMode.nFPS = g_fps;
	g_RetVal = g_DepthGenerator.SetMapOutputMode(outputMode);
	if (g_RetVal != XN_STATUS_OK){
		printf("Failed setting the DEPTH output mode %s\n", xnGetStatusString(g_RetVal));
		return false;
	}

	g_RetVal = g_Context.StartGeneratingAll();
	if (g_RetVal != XN_STATUS_OK){
		printf("Failed starting generating all %s\n", xnGetStatusString(g_RetVal));
		return false;
	}
#endif
	g_im3D.create(g_im_h,g_im_w,CV_32FC3);
	g_imD.create(g_im_h,g_im_w,CV_16UC1);

	return true;

}

HBITMAP ConvertIplImage2HBITMAP(IplImage* pImage)
{
      IplImage* image = (IplImage*)pImage;
      bool imgConverted = false;
      /*if(pImage->nChannels!=3)
      {
            IplImage* imageCh3 = cvCreateImage(cvGetSize(pImage),
                  8, 3);
            if(pImage->nChannels==1)
                  cvCvtColor(pImage, imageCh3, CV_GRAY2RGB);
            image = imageCh3;

            imgConverted = true;
      }*/

      int bpp = image->nChannels * 8;
      assert(image->width >= 0 && image->height >= 0 &&
            (bpp == 8 || bpp == 24 || bpp == 32));
      CvMat dst;
      void* dst_ptr = 0;
      HBITMAP hbmp = NULL;
      unsigned char buffer[sizeof(BITMAPINFO) + 255*sizeof(RGBQUAD)];
      BITMAPINFO* bmi = (BITMAPINFO*)buffer;
      BITMAPINFOHEADER* bmih = &(bmi->bmiHeader);

      ZeroMemory(bmih, sizeof(BITMAPINFOHEADER));
      bmih->biSize = sizeof(BITMAPINFOHEADER);
      bmih->biWidth = image->width;
      bmih->biHeight = image->origin ? abs(image->height) :
            -abs(image->height);
      bmih->biPlanes = 1;
      bmih->biBitCount = bpp;
      bmih->biCompression = BI_RGB;

      if (bpp == 8) {
            RGBQUAD* palette = bmi->bmiColors;
            int i;
            for (i = 0; i < 256; i++) {
                  palette[i].rgbRed = palette[i].rgbGreen = palette
                        [i].rgbBlue
                        = (BYTE)i;
                  palette[i].rgbReserved = 0;
            }
      }

      hbmp = CreateDIBSection(NULL, bmi, DIB_RGB_COLORS, &dst_ptr, 0,
            0);
      cvInitMatHeader(&dst, image->height, image->width, CV_8UC3,
            dst_ptr, (image->width * image->nChannels + 3) & -4);
      cvConvertImage(image, &dst, image->origin ? CV_CVTIMG_FLIP : 0);

      if(imgConverted)
            cvReleaseImage(&image);

      return hbmp;
}
bool read_data( ){

#ifdef USE_MS_SKD

	const int eventCount = 1;
    void* hEvents[eventCount];
	hEvents[0] = g_hNextDepthFrameEvent;

	// Check to see if we have either a message (by passing in QS_ALLINPUT)
	// Or a Kinect event (hEvents)
	// Update() will check for Kinect events individually, in case more than one are signalled

	DWORD dwEvent = MsgWaitForMultipleObjects(eventCount, hEvents, FALSE, INFINITE, QS_ALLINPUT);

	// Check if this is an event we're waiting on and not a timeout or message
	if (WAIT_OBJECT_0 == dwEvent)
	{
		if (NULL == g_pNuiSensor)
			return false;

		if ( WAIT_OBJECT_0 == WaitForSingleObject(g_hNextDepthFrameEvent, 0) ) {

			NUI_IMAGE_FRAME imageFrame;
			HRESULT hr = g_pNuiSensor->NuiImageStreamGetNextFrame(	g_pDepthStreamHandle,
																	100,
																	&imageFrame );

			if ( FAILED( hr ) ) return false;
		
			INuiFrameTexture * pTexture = imageFrame.pFrameTexture;
			NUI_LOCKED_RECT LockedRect;
			pTexture->LockRect( 0, &LockedRect, NULL, 0 );
			if ( 0 != LockedRect.Pitch )
			{
				g_imD.setTo(0);

				DWORD frameWidth, frameHeight;
				NuiImageResolutionToSize( imageFrame.eResolution, frameWidth, frameHeight );
        
				// draw the bits to the bitmap
				//BYTE * rgbrun = m_depthRGBX;
				const USHORT * pBufferRun = (const USHORT *)LockedRect.pBits;

				// end pixel is start + width*height - 1
				const USHORT * pBufferEnd = pBufferRun + (frameWidth * frameHeight);

				int pixel = 0;
				while ( pBufferRun < pBufferEnd )
				{
					USHORT depth     = *pBufferRun;
					USHORT realDepth = NuiDepthPixelToDepth(depth);
         
					int x = pixel%640;
					int y = floor(float(pixel)/640.f);
					
					g_imD.at<int16_t>(y,x) = realDepth;

					++pBufferRun;
					++pixel;
				}
			}
			
			// We're done with the texture so unlock it
			pTexture->UnlockRect(0);

			// Release the frame
			g_pNuiSensor->NuiImageStreamReleaseFrame(g_pDepthStreamHandle, &imageFrame);
			
		}
	}
#else
		// Wait for new data to be available
	//MessageBox(NULL, "wait data",NULL,NULL);
	g_RetVal = g_Context.WaitAndUpdateAll();
	if (g_RetVal != XN_STATUS_OK)
	{
		printf("Failed updating data: %s\n", xnGetStatusString(g_RetVal));
		return false;
	}

	// Take current depth map
	g_DepthGenerator.GetMetaData(g_depthMD);
#endif

	//int i = 7;
	//Mat src;
	//Mat dst;

	//src.create(g_im_h,g_im_w,CV_32FC3);
	//dst.create(g_im_h,g_im_w,CV_32FC3);
	//g_imD.convertTo(src,CV_32FC3);

	//bilateralFilter ( src, dst, i, i*2, i/2 );
	//dst.convertTo(g_imD,CV_16UC1);


	int valid_pixels = 0;
	float d = 0.f;

	//generate 3D image
	for(int y = 0; y < g_im3D.rows; y++)
	{
		Vec3f* Mi = g_im3D.ptr<Vec3f>(y);
		for(int x = 0; x < g_im3D.cols; x++){

#ifdef USE_MS_SKD
			d = float( g_imD.at<int16_t>(y,x) );
#else
			d = (float)g_depthMD(x,y);
#endif


			if ( d < g_max_z && d > 0 ){

				valid_pixels++;

				Mi[x][0] = ( float(d * (x - 320)) * 0.0017505f );
				Mi[x][1] = ( float(d * (y - 240)) * 0.0017505f );
				Mi[x][2] = d;

			}
			else
				Mi[x] = 0;

		}
	}
	//this part is to set the camera position, depending on what's in the scene
	if (g_first_rigid ) {

		if( valid_pixels > 10000){ //wait for something to be in the image

			// calculate gravity center
			Vec3f gravity(0,0,0);
			int count = 0;
			for(int y=0;y<g_im3D.rows;++y){
				const Vec3f* Mi = g_im3D.ptr<Vec3f>(y);
				for(int x=0;x<g_im3D.cols;++x){

					if( Mi[x][2] > 0 ) {

						gravity = gravity + Mi[x];
						count++;
					}
				}
			}

			float maxDist = 0;
			if(count > 0) {

				gravity = (1.f/(float)count)*gravity;

				for(int y=0;y<g_im3D.rows;++y){
					const Vec3f* Mi = g_im3D.ptr<Vec3f>(y);
					for(int x=0;x<g_im3D.cols;++x){

						if( Mi[x][2] > 0 ) {

							maxDist = MAX(maxDist,(float)norm( Mi[x]-gravity ));
						}
					}
				}
			}

			//g_camera.resetview( math_vector_3f(gravity[0],gravity[1],gravity[2]), maxDist );
			//g_camera.rotate_180();
			g_first_rigid = false;
		}
	}

	//IplImage myImage = g_im3D;
	//preview = ConvertIplImage2HBITMAP(&myImage);

	return true;
}

bool process() {

	if( read_data() ){

		g_means.clear();
		g_votes.clear();
		g_clusters.clear();

		//do the actual estimation
		g_Estimate->estimate( 	g_im3D,
									g_means,
									g_clusters,
									g_votes,
									g_stride,
									g_maxv,
									g_prob_th,
									g_larger_radius_ratio,
									g_smaller_radius_ratio,
									false,
									g_th
								);

		//get raw data
		for(unsigned int i=0;i<g_means.size();++i) {
			KDATA.fPitch = g_means[i][3];
			KDATA.fYaw  = g_means[i][4];
			KDATA.fRoll = g_means[i][5];
			KDATA.fX = g_means[i][0];
			KDATA.fY = g_means[i][1];
			KDATA.fZ = g_means[i][2]-1000;
		}
		



		return true;
	}
	else
		return false;

}
void UpdateDeltaTime()
{
	QueryPerformanceCounter((LARGE_INTEGER*)&currentTime);
	DeltaTime = (currentTime - prevTime)*secondsPerCount;
	prevTime=currentTime;

	//DeltaTime=0.0001;

	//CurrentTime = timeGetTime();
	//DeltaTime = ((double)CurrentTime - (double)LastTime) * 0.001f;
	//LastTime = timeGetTime();
}

void  PhysicsApplyForce()
{
	KDIFF.fYaw = KDATACURRENT.fYaw-KDATA.fYaw;
	KDIFF.fPitch = KDATACURRENT.fPitch-KDATA.fPitch;
	KDIFF.fRoll = KDATACURRENT.fRoll-KDATA.fRoll;
	KDIFF.fX = KDATACURRENT.fX-KDATA.fX;
	KDIFF.fY = KDATACURRENT.fY-KDATA.fY;
	KDIFF.fZ = KDATACURRENT.fZ-KDATA.fZ;

	
	//if(KDIFF.fYaw < 0)
	//{
		//do acceleration
		KVELOCITY.fYaw+=	-KDIFF.fYaw		*(cAcceleration*(abs(KDIFF.fYaw)	*cTolerance*FMTSLD(CurrentProfile.Filter))* FMTSLD(CurrentProfile.Accel)) ;
		KVELOCITY.fPitch+=	-KDIFF.fPitch	*(cAcceleration*(abs(KDIFF.fPitch)	*cTolerance*FMTSLD(CurrentProfile.Filter))* FMTSLD(CurrentProfile.Accel))  ;
		KVELOCITY.fRoll+=	-KDIFF.fRoll	*(cAcceleration*(abs(KDIFF.fRoll)	*cTolerance*FMTSLD(CurrentProfile.Filter))* FMTSLD(CurrentProfile.Accel))  ;
		KVELOCITY.fX+=		-KDIFF.fX		*(cAcceleration*(abs(KDIFF.fX)		*cTolerance*FMTSLD(CurrentProfile.Filter))* FMTSLD(CurrentProfile.Accel))  ;
		KVELOCITY.fY+=		-KDIFF.fY		*(cAcceleration*(abs(KDIFF.fY)		*cTolerance*FMTSLD(CurrentProfile.Filter))* FMTSLD(CurrentProfile.Accel))  ;
		KVELOCITY.fZ+=		-KDIFF.fZ		*(cAcceleration*(abs(KDIFF.fZ)		*cTolerance*FMTSLD(CurrentProfile.Filter))* FMTSLD(CurrentProfile.Accel))  ;
	//}else{
		//do decceleration
		//KVELOCITY.fYaw-=KDIFF.fYaw  *cDecceleration*DeltaTime;
	//}




    //if(KDIFF.fYaw>0)
		//KVELOCITY.fYaw-=KDIFF.fYaw  *cDecceleration*DeltaTime;


	KVELOCITY.fYaw*=	cDamping*FMTSLD(CurrentProfile.Damping, true);
	KVELOCITY.fPitch*=	cDamping*FMTSLD(CurrentProfile.Damping, true);
	KVELOCITY.fRoll*=	cDamping*FMTSLD(CurrentProfile.Damping, true);
	KVELOCITY.fX*=	cDamping*FMTSLD(CurrentProfile.Damping, true);
	KVELOCITY.fY*=	cDamping*FMTSLD(CurrentProfile.Damping, true);
	KVELOCITY.fZ*=	cDamping*FMTSLD(CurrentProfile.Damping, true);

	KVELOCITY.fYaw=cap(KVELOCITY.fYaw, cMaxAngleVelocity);
	KVELOCITY.fPitch=cap(KVELOCITY.fPitch, cMaxAngleVelocity);
	KVELOCITY.fRoll=cap(KVELOCITY.fRoll, cMaxAngleVelocity);
	KVELOCITY.fX=cap(KVELOCITY.fX, cMaxVelocity);
	KVELOCITY.fY=cap(KVELOCITY.fY, cMaxVelocity);
	KVELOCITY.fZ=cap(KVELOCITY.fZ, cMaxVelocity);

	//KVELOCITY.fYaw*=0.9998;
	//KVELOCITY.fPitch*=0.9998;
	//KVELOCITY.fRoll*=0.9998;
	//KVELOCITY.fX*=0.9998;
	//KVELOCITY.fY*=0.9998;
	//KVELOCITY.fZ*=0.9998;

	//

	KDATACURRENT.fYaw+=KVELOCITY.fYaw;
	KDATACURRENT.fPitch+=KVELOCITY.fPitch;
	KDATACURRENT.fRoll+=KVELOCITY.fRoll;
	KDATACURRENT.fX+=KVELOCITY.fX;
	KDATACURRENT.fY+=KVELOCITY.fY;
	KDATACURRENT.fZ+=KVELOCITY.fZ;

	Sleep(10);
	//if(KDATACURRENT.fYaw<0)KDATACURRENT.fYaw=0;
	
	//if(KDIFF.fYaw>0)
	//	KDATACURRENT.fYaw-=Gravity*DeltaTime;
	//else
	//	KDATACURRENT.fYaw+=Gravity*DeltaTime;
}