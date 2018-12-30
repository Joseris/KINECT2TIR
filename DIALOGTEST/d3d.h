#include <d3d9.h>
#include <d3dx9.h>
#include "skullocc.h"
#define SAFE_RELEASE(p) { if ( (p) ) { (p)->Release(); (p) = 0; } }
#define SAFE_DELETE(a) if( (a) != NULL ) delete (a); (a) = NULL;
// Globals
HWND              hwnd       = NULL;
IDirect3DDevice9* D3DDevice = NULL;
D3DDISPLAYMODE d3ddm = {0};
D3DPRESENT_PARAMETERS d3dpp = {0};

//DX meshes
ID3DXMesh* PlatformMesh;
ID3DXMesh* BoxMesh;
ID3DXMesh* CylinderMesh;
ID3DXMesh* SphereMesh;


LPD3DXMESH          g_pMesh = NULL; // Our mesh object in sysmem
D3DMATERIAL9*       g_pMeshMaterials = NULL; // Materials for our mesh
LPDIRECT3DTEXTURE9* g_pMeshTextures = NULL; // Textures for our mesh
DWORD               g_dwNumMaterials = 0L;   // Number of mesh materials
D3DVIEWPORT9 vp = {0};
D3DXVECTOR3 d1,d2;
D3DXMATRIX World;
D3DXMATRIX Trans;

//x, y, z and color
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ | D3DFVF_DIFFUSE)
#define D3DFVF_CUSTOMPOINT (D3DFVF_XYZ | D3DFVF_NORMAL)

D3DMATERIAL9 mtrl;
D3DMATERIAL9 mtrl_pose;




//structures
struct D3DVERTEX
{
	float fX,
		  fY,
		  fZ;
	DWORD dwColor;
};
struct D3DPOINT{
	
    float x, y, z;
    float nx, ny, nz;

};

//vertex buffer for triangle (NEW)
LPDIRECT3DVERTEXBUFFER9 pTriangleVB = NULL;
//pointer to vertex data (NEW)
VOID* pData;
	D3DPOINT plist[500000];

	DWORD BACKCOLOR = {0};//GetSysColor(COLOR_BTNFACE);
DWORD FORECOLOR = {0};//GetSysColor(COLOR_BTNTEXT);

void Setup(void);
//void Cleanup(void);
void Render(void);
void InitializeDirect3D(void);

float ScreenWidth  = 420;
float ScreenHeight = 168;
bool init = false;
void Setup( void )
{

	BACKCOLOR = 0xffbbbbbb;
	FORECOLOR = 0xffbbbbbb;
	//BACKCOLOR = GetSysColor(COLOR_BTNFACE) | 0xff000000;
	//FORECOLOR = //GetSysColor(COLOR_BTNTEXT) | 0xff000000;



	LPD3DXBUFFER pD3DXMtrlBuffer;

	D3DXLoadMeshFromXInMemory(rawData,sizeof(rawData), D3DXMESH_SYSTEMMEM,
                             D3DDevice, NULL,
                             &pD3DXMtrlBuffer,NULL, &g_dwNumMaterials,
                             &g_pMesh );
    // We need to extract the material properties and texture names from the 
    // pD3DXMtrlBuffer
    D3DXMATERIAL* d3dxMaterials = ( D3DXMATERIAL* )pD3DXMtrlBuffer->GetBufferPointer();
	
    g_pMeshMaterials = new D3DMATERIAL9[g_dwNumMaterials];
    if( g_pMeshMaterials == NULL )
        return;
    g_pMeshTextures = new LPDIRECT3DTEXTURE9[g_dwNumMaterials];
    if( g_pMeshTextures == NULL )
        return;

    for( DWORD i = 0; i < g_dwNumMaterials; i++ )
    {
        // Copy the material
        g_pMeshMaterials[i] = d3dxMaterials[i].MatD3D;

        // Set the ambient color for the material (D3DX does not do this)
        g_pMeshMaterials[i].Ambient = g_pMeshMaterials[i].Diffuse;

        g_pMeshTextures[i] = NULL;
        if( d3dxMaterials[i].pTextureFilename != NULL &&
            lstrlenA( d3dxMaterials[i].pTextureFilename ) > 0 )
        {
            // Create the texture
            if( FAILED( D3DXCreateTextureFromFileA( D3DDevice,
                                                    d3dxMaterials[i].pTextureFilename,
                                                    &g_pMeshTextures[i] ) ) )
            {
                // If texture is not in current folder, try parent folder
                const CHAR* strPrefix = "..\\";
                CHAR strTexture[MAX_PATH];
                strcpy_s( strTexture, MAX_PATH, strPrefix );
                strcat_s( strTexture, MAX_PATH, d3dxMaterials[i].pTextureFilename );
                // If texture is not in current folder, try parent folder
                if( FAILED( D3DXCreateTextureFromFileA( D3DDevice,
                                                        strTexture,
                                                        &g_pMeshTextures[i] ) ) )
                {
                    MessageBox( NULL, "Could not find texture map", "Mesh", MB_OK );
                }
            }
        }
    }

    // Done with the material buffer
	
    pD3DXMtrlBuffer->Release();


//generate normals
	   LPD3DXMESH pTempMesh = NULL;
	   DWORD dwFVF = D3DFVF_CUSTOMPOINT;

       g_pMesh->CloneMeshFVF( g_pMesh->GetOptions(), dwFVF, D3DDevice, &pTempMesh);

        DWORD dwOldFVF = 0;
        dwOldFVF = g_pMesh->GetFVF();
        g_pMesh->Release();
        g_pMesh = pTempMesh;
		
        // Compute normals if they are being requested and
        // the old mesh does not have them.
        if( !(dwOldFVF & D3DFVF_NORMAL) && dwFVF & D3DFVF_NORMAL )
        {
            D3DXComputeNormals( g_pMesh, NULL );
        }
    



	//Create the platform mesh
	D3DXCreateBox(D3DDevice, 20.0f, 1.0f, 20.0f, &PlatformMesh, 0);

	//Create the sphere mesh
	D3DXCreateSphere(D3DDevice, 30.0f, 20, 20, &SphereMesh, 0);

	//Create the cylinder mesh
	D3DXCreateCylinder(D3DDevice, 15.0f, 15.0f, 100.0f, 20, 20, &CylinderMesh, 0);

	//Set the View matrix
	D3DXMATRIX View;
	D3DXVECTOR3 Position(0.0f, 0.0f, -10.0f);
	D3DXVECTOR3 Target(0.0f, 0.0f, 0.0f);
	D3DXVECTOR3 Up(0.0f, -1.0f, 0.0f);
	D3DXMatrixLookAtLH(&View, &Position, &Target, &Up);
	D3DDevice->SetTransform(D3DTS_VIEW, &View);

	//Set the Projection matrix
	D3DXMATRIX Proj;
	//D3DXMatrixPerspectiveFovLH( &Proj, D3DX_PI /4, ScreenWidth / ScreenHeight, 1.0f, 10000.0f );
	D3DXMatrixPerspectiveFovLH( &Proj, D3DX_PI /4, 200.0f / 240.0f, 1.0f, 10000.0f );
	//D3DXMatrixPerspectiveFovLH( &Proj, 0.3333,ScreenWidth / ScreenHeight, 1.0f, 10000.0f );
	//D3DXMatrixPerspectiveLH( &Proj, 2, 1, 1.0f, 10000.0f );
	D3DDevice->SetTransform(D3DTS_PROJECTION, &Proj);

    //Turn on lighting and set an ambient value
    D3DDevice->SetRenderState( D3DRS_LIGHTING, TRUE );
    D3DDevice->SetRenderState( D3DRS_AMBIENT,D3DCOLOR_COLORVALUE( 0.2f, 0.2f, 0.2f, 1.0f ));
	//D3DDevice->SetRenderState( D3DRS_AMBIENT, 0xffffffff );

	//Create a simple DX directional light
    D3DLIGHT9 light0;
    ZeroMemory( &light0, sizeof(D3DLIGHT9) );
    light0.Type = D3DLIGHT_DIRECTIONAL;
    light0.Direction = D3DXVECTOR3( 0.5f, -0.8f, 0.7f );
    light0.Diffuse = D3DXCOLOR(0xffffffff  );//1.0f;
    //light0.Diffuse.g = 1.0f;
    //light0.Diffuse.b = 1.0f;
    //light0.Diffuse.a = 1.0f;

	//Set the light
    D3DDevice->SetLight( 0, &light0 );
    D3DDevice->LightEnable( 0, TRUE );




	//Create a material

    ZeroMemory( &mtrl, sizeof(D3DMATERIAL9) );
	ZeroMemory( &mtrl_pose, sizeof(D3DMATERIAL9) );
    mtrl.Diffuse = D3DXCOLOR(FORECOLOR  );//  .r = 1.0f;
    //mtrl.Diffuse.g = 1.0f;
    //mtrl.Diffuse.b = 1.0f;
    //mtrl.Diffuse.a = 1.0f;
    mtrl.Ambient = D3DXCOLOR(FORECOLOR );//1.0f;
    //mtrl.Ambient.g = 1.0f;
    //mtrl.Ambient.b = 1.0f;
    //mtrl.Ambient.a = 1.0f;
    mtrl_pose.Diffuse.r = 0.0f;
    mtrl_pose.Diffuse.g = 1.0f;
    mtrl_pose.Diffuse.b = 0.0f;
    mtrl_pose.Diffuse.a = 1.0f;
    mtrl_pose.Ambient.r = 0.0f;
    mtrl_pose.Ambient.g = 1.0f;
    mtrl_pose.Ambient.b = 0.0f;
    mtrl_pose.Ambient.a = 1.0f;



	//Set some render states to determine texture quality
	D3DDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	D3DDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	D3DDevice->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);


	D3DDevice->SetRenderState(D3DRS_ZENABLE,D3DZB_TRUE);
	D3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	D3DDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
	D3DDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	D3DDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);




}

void Cleanup( bool releasedevice = true )
{
	
    if( g_pMeshTextures )
    {
        for( DWORD i = 0; i < g_dwNumMaterials; i++ )
        {
            SAFE_RELEASE( g_pMeshTextures[i] )
               
        }
        SAFE_DELETE( g_pMeshTextures);
		g_dwNumMaterials=0;
    }
    if( g_pMeshMaterials )
    {
		SAFE_DELETE(g_pMeshMaterials);
	}
	//Release the Meshes
	SAFE_RELEASE(PlatformMesh);
	SAFE_RELEASE(BoxMesh);
	//BoxMesh->Release();
	SAFE_RELEASE(CylinderMesh);
	SAFE_RELEASE(SphereMesh);
	SAFE_RELEASE(g_pMesh);

	//Release the Direct3D device
	UINT references = 0;
	if(releasedevice){
		if(D3DDevice)
		 references = D3DDevice->Release();
	}
}
void Reset()
{

}
void InitializeDirect3D( HWND hwnd )
{
	IDirect3D9* D3D = NULL;

    D3D = Direct3DCreate9( D3D_SDK_VERSION );

	if( !D3D )
	{
		if( D3D != NULL )
			D3D->Release();

		::MessageBox(0, "Direct3DCreate9() - Failed", 0, 0);
		return;
	}

    

    if( FAILED( D3D->GetAdapterDisplayMode( D3DADAPTER_DEFAULT, &d3ddm ) ) )
	{
		if( D3D != NULL )
			D3D->Release();

		::MessageBox(0, "GetAdapterDisplayMode() - Failed", 0, 0);
		return;
	}

	HRESULT hr;

	if( FAILED( hr = D3D->CheckDeviceFormat( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, 
												d3ddm.Format, D3DUSAGE_DEPTHSTENCIL,
												D3DRTYPE_SURFACE, D3DFMT_D16 ) ) )
	{
		if( hr == D3DERR_NOTAVAILABLE )
		{
			if( D3D != NULL )
				D3D->Release();

			::MessageBox(0, "CheckDeviceFormat() - Failed", 0, 0);
			return;
		}
	}

	D3DCAPS9 d3dCaps;

	if( FAILED( D3D->GetDeviceCaps( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &d3dCaps ) ) )
	{
		if( D3D != NULL )
			D3D->Release();

		::MessageBox(0, "GetDeviceCaps() - Failed", 0, 0);
		return;
	}

	DWORD dwBehaviorFlags = 0;

	// Use hardware vertex processing if supported, otherwise default to software 
	if( d3dCaps.VertexProcessingCaps != 0 )
		dwBehaviorFlags |= D3DCREATE_HARDWARE_VERTEXPROCESSING;
	else
		dwBehaviorFlags |= D3DCREATE_SOFTWARE_VERTEXPROCESSING;

	// All system checks passed, create the device


	memset(&d3dpp, 0, sizeof(d3dpp));

    d3dpp.BackBufferFormat       = d3ddm.Format;
	d3dpp.SwapEffect             = D3DSWAPEFFECT_DISCARD;
	d3dpp.Windowed               = TRUE;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
    d3dpp.PresentationInterval   = D3DPRESENT_INTERVAL_ONE;

    if( FAILED( D3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd,
                                      dwBehaviorFlags, &d3dpp, &D3DDevice ) ) )
	{
		if( D3D != NULL )
			D3D->Release();

		::MessageBox(0, "CreateDevice() - Failed", 0, 0);
		return;
	}

	// No longer needed, release it
	 if( D3D != NULL )
        D3D->Release();
}
void RenderGameHead( void )
{

	vp.X = 400;
	vp.Y = 0;
	vp.Width = 202;
	vp.Height = 262;
	D3DDevice->SetViewport(&vp);

	D3DDevice->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,  D3DXCOLOR(BACKCOLOR ), 1.0f, 0 );




	D3DXMatrixIdentity(&World);
	D3DXMatrixRotationX(&Trans, -(KDATACURRENT.fPitch-KDATACENTER.fPitch)*amult   *angsens);
	D3DXMatrixMultiply(&World,&World,&Trans);
	D3DXMatrixRotationY(&Trans, -(KDATACURRENT.fYaw-KDATACENTER.fYaw)*amult   *angsens);
	D3DXMatrixMultiply(&World,&World,&Trans);
	D3DXMatrixRotationZ(&Trans, -(KDATACURRENT.fRoll-KDATACENTER.fRoll)*amult   *angsens+3.14);
	D3DXMatrixMultiply(&World,&World,&Trans);
	
	World._43 = 20+(KDATACURRENT.fZ-KDATACENTER.fZ)*tmult   *movesens; //inout
	World._42 = 1+(KDATACURRENT.fY-KDATACENTER.fY)*tmult   *movesens; ; //updwwn
	World._41 = 0+(KDATACURRENT.fX-KDATACENTER.fX)*tmult   *movesens; ; //leftright

	D3DDevice->SetTransform(D3DTS_WORLD, &World);
	D3DDevice->SetFVF(NULL); //set vertex format
	D3DDevice->SetMaterial( &mtrl );
    g_pMesh->DrawSubset( 0 );



}
void RenderActualHead( void )
{

	vp.X = 200;
	vp.Y = 0;
	vp.Width = 202;
	vp.Height = 262;
	D3DDevice->SetViewport(&vp);

	D3DDevice->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DXCOLOR(BACKCOLOR ), 1.0f, 0 );

	D3DXMatrixIdentity(&World);
	D3DXMatrixRotationX(&Trans, -KDATA.fPitch*angsens);
	D3DXMatrixMultiply(&World,&World,&Trans);
	D3DXMatrixRotationY(&Trans, -KDATA.fYaw*angsens);
	D3DXMatrixMultiply(&World,&World,&Trans);
	D3DXMatrixRotationZ(&Trans, -KDATA.fRoll*angsens+3.14);
	D3DXMatrixMultiply(&World,&World,&Trans);
	

	World._43 = 20+ KDATA.fZ*movesens; //inout
	World._42 = 1+ KDATA.fY*movesens; //updwwn
	World._41 = 0+ KDATA.fX*movesens; //leftright
	D3DDevice->SetTransform(D3DTS_WORLD, &World);
	D3DDevice->SetFVF(NULL); //set vertex format
	D3DDevice->SetMaterial( &mtrl );
    g_pMesh->DrawSubset( 0 );
	


}
void RenderPoints( void )
{

	vp.X = 0;
	vp.Y = 0;
	vp.Width = 200;
	vp.Height = 262;
	D3DDevice->SetViewport(&vp);
	D3DDevice->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DXCOLOR(BACKCOLOR ), 1.0f, 0 );


    D3DDevice->SetFVF(D3DFVF_CUSTOMPOINT); //set vertex format
	D3DDevice->SetMaterial( &mtrl );


	D3DXMatrixIdentity(&World);
	//World._41 = 800;
	D3DDevice->SetTransform(D3DTS_WORLD, &World);
	
	DWORD pcount = 0;



	for(int y = 0; y < g_im3D.rows-1; y++)
	{

		const Vec3f* Mi = g_im3D.ptr<Vec3f>(y);
		const Vec3f* Mi1 = g_im3D.ptr<Vec3f>(y+1);

		for(int x = 0; x < g_im3D.cols-1; x++){

			if( Mi[x][2] <= 0 || Mi[x][2] <= 0 )
				continue;

			d1[0] = Mi[x][0] - Mi1[x][0];// v1 - v2;
			d1[1] = Mi[x][1] - Mi1[x][1];
			d1[2] = Mi[x][2] - Mi1[x][2];

			d2[0] = Mi[x+1][0] - Mi1[x][0];// v1 - v2;
			d2[1] = Mi[x+1][1] - Mi1[x][1];
			d2[2] = Mi[x+1][2] - Mi1[x][2];

			D3DXVECTOR3 norm;
			D3DXVec3Cross(&norm,&d2,&d1);
			plist[pcount].x = Mi[x][0];
			plist[pcount].y = Mi[x][1];
			plist[pcount].z = Mi[x][2];
			plist[pcount].nx = norm.x;
			plist[pcount].ny = norm.y;
			plist[pcount].nz = norm.z;
			pcount++;
		}
	}

	D3DDevice->SetMaterial( &mtrl );
	D3DDevice->DrawPrimitiveUP(D3DPT_POINTLIST, pcount, plist, sizeof(D3DPOINT));



	//draw head poses
	//if(g_means.size()>0){

		D3DDevice->SetMaterial( &mtrl_pose );
		float mult = 0.0174532925f;

		//for(unsigned int i=0;i<g_means.size();++i){

			D3DXVECTOR3 head_center( KDATA.fX, KDATA.fY, KDATA.fZ+1000 );
			

			D3DXMatrixIdentity(&World);
			D3DXMatrixRotationYawPitchRoll(&World,  mult*KDATA.fYaw,mult*KDATA.fPitch, mult*KDATA.fRoll);
			World._41 = head_center.x;
			World._42 = head_center.y;
			World._43 = head_center.z;
			

			
			D3DXMatrixTranslation(&Trans,0,0,-50);
			D3DXMatrixMultiply(&Trans,&Trans,&World);
			D3DDevice->SetTransform(D3DTS_WORLD, &Trans);
			HRESULT ar = D3DDevice->TestCooperativeLevel();
			SphereMesh->DrawSubset(0);
			D3DXMatrixTranslation(&Trans,0,0,-120);
			D3DXMatrixMultiply(&Trans,&Trans,&World);
			D3DDevice->SetTransform(D3DTS_WORLD, &Trans);
			CylinderMesh->DrawSubset(0);
		//}

	//}



}

extern HWND hDlg;
void Render( void )
{
	if(!init){
		InitializeDirect3D(GetDlgItem(hDlg,IDC_STATIC6));
		Setup();
		init=true;
	}
	//if(GetForegroundWindow() != hDlg) return;
	HRESULT hr=D3DDevice->TestCooperativeLevel();
	if(hr == D3DERR_DEVICELOST || hr == D3DERR_DEVICENOTRESET)
	{
		Cleanup(false);
		if(FAILED(hr = D3DDevice->Reset(&d3dpp)) )
		{
			return;
		}else{

			Setup();
		}
	}
	
    D3DDevice->BeginScene();
	
	RenderPoints();
	RenderActualHead();
	RenderGameHead();

	D3DDevice->EndScene();
	D3DDevice->Present( NULL, NULL, NULL, NULL );
    




}