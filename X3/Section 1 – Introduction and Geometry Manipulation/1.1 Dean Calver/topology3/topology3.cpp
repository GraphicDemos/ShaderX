//--------------------------------------------------------------------------------------
// File: Topology4.cpp
//
//--------------------------------------------------------------------------------------
#include "dxstdafx.h"
#include "resource.h"
#include "VertexMapCreator.h"

#define DEBUG_VS   // Uncomment this line to debug vertex shaders 
#define DEBUG_PS   // Uncomment this line to debug pixel shaders 


//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
ID3DXFont*              g_pFont = NULL;         // Font for drawing text
ID3DXSprite*            g_pTextSprite = NULL;   // Sprite for batching draw text calls
ID3DXEffect*            g_pEffect = NULL;       // D3DX effect interface
ID3DXEffect*            g_pEffect2 = NULL;      // D3DX effect interface
CModelViewerCamera      g_Camera;               // A model viewing camera
bool                    g_bShowHelp = true;     // If true, it renders the UI control text
CDXUTDialog             g_HUD;                  // dialog for standard controls
CDXUTDialog             g_SampleUI;             // dialog for sample specific controls

bool					g_softwareVS30 = true;
ID3DXMesh*				g_teapotMesh = 0;
IDirect3DTexture9*		g_positionTexture = 0;
IDirect3DTexture9*		g_normalTexture = 0;
IDirect3DVertexBuffer9*	g_indexVertexStream = 0;
IDirect3DVertexDeclaration9* g_indexVSDecl = 0;
int						g_numTriangles = 0;
IDirect3DVertexBuffer9*	g_123VertexStream = 0;
IDirect3DIndexBuffer9*	g_123IndexStream = 0;
float					g_vertexMapSizes[2] = {0};

IDirect3DTexture9*		g_topoMapTexture = 0;
float					g_topoMapSizes[2] = {0};
IDirect3DVertexBuffer9*	g_teapotCounterStream = 0;

IDirect3DTexture9*		g_rtFaceNormalTexture = 0;

//--------------------------------------------------------------------------------------
// UI control IDs
//--------------------------------------------------------------------------------------
#define IDC_TOGGLEFULLSCREEN    1
#define IDC_VIEWWHITEPAPER      2
#define IDC_TOGGLEREF           3
#define IDC_CHANGEDEVICE        4



//--------------------------------------------------------------------------------------
// Forward declarations 
//--------------------------------------------------------------------------------------
bool    CALLBACK IsDeviceAcceptable( D3DCAPS9* pCaps, D3DFORMAT AdapterFormat, D3DFORMAT BackBufferFormat, bool bWindowed );
void    CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, const D3DCAPS9* pCaps );
HRESULT CALLBACK OnCreateDevice( IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc );
HRESULT CALLBACK OnResetDevice( IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc );
void    CALLBACK OnFrameMove( IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime );
void    CALLBACK OnFrameRender( IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime );
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing );
void    CALLBACK KeyboardProc( UINT nChar, bool bKeyDown, bool bAltDown  );
void    CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl );
void    CALLBACK OnLostDevice();
void    CALLBACK OnDestroyDevice();

void    InitApp();
HRESULT LoadMesh( IDirect3DDevice9* pd3dDevice, WCHAR* strFileName, ID3DXMesh** ppMesh );
void    RenderText();
void RenderFaceNormalMap( IDirect3DDevice9* pd3dDevice );


//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
INT WINAPI WinMain( HINSTANCE, HINSTANCE, LPSTR, int )
{
    // Set the callback functions. These functions allow the sample framework to notify
    // the application about device changes, user input, and windows messages.  The 
    // callbacks are optional so you need only set callbacks for events you're interested 
    // in. However, if you don't handle the device reset/lost callbacks then the sample 
    // framework won't be able to reset your device since the application must first 
    // release all device resources before resetting.  Likewise, if you don't handle the 
    // device created/destroyed callbacks then the sample framework won't be able to 
    // recreate your device resources.
    DXUTSetCallbackDeviceCreated( OnCreateDevice );
    DXUTSetCallbackDeviceReset( OnResetDevice );
    DXUTSetCallbackDeviceLost( OnLostDevice );
    DXUTSetCallbackDeviceDestroyed( OnDestroyDevice );
    DXUTSetCallbackMsgProc( MsgProc );
    DXUTSetCallbackKeyboard( KeyboardProc );
    DXUTSetCallbackFrameRender( OnFrameRender );
    DXUTSetCallbackFrameMove( OnFrameMove );

    // Show the cursor and clip it when in full screen
    DXUTSetCursorSettings( true, true );

    InitApp();

    // Initialize the sample framework and create the desired Win32 window and Direct3D 
    // device for the application. Calling each of these functions is optional, but they
    // allow you to set several options which control the behavior of the framework.
    DXUTInit( true, true, true ); // Parse the command line, handle the default hotkeys, and show msgboxes
    DXUTCreateWindow( L"Topology3" );
//    DXUTCreateDevice( D3DADAPTER_DEFAULT, true, 640, 480, IsDeviceAcceptable, ModifyDeviceSettings );
    DXUTCreateDevice( D3DADAPTER_DEFAULT, true, 320, 240, IsDeviceAcceptable, ModifyDeviceSettings );

    // Pass control to the sample framework for handling the message pump and 
    // dispatching render calls. The sample framework will call your FrameMove 
    // and FrameRender callback when there is idle time between handling window messages.
    DXUTMainLoop();

    // Perform any application-level cleanup here. Direct3D device resources are released within the
    // appropriate callback functions and therefore don't require any cleanup code here.

    return DXUTGetExitCode();
}


//--------------------------------------------------------------------------------------
// Initialize the app 
//--------------------------------------------------------------------------------------
void InitApp()
{
    // Initialize dialogs
    g_HUD.SetCallback( OnGUIEvent ); int iY = 10; 
    g_HUD.AddButton( IDC_TOGGLEFULLSCREEN, L"Toggle full screen", 35, iY += 24, 125, 22 );
    g_HUD.AddButton( IDC_TOGGLEREF, L"Toggle REF (F3)", 35, iY += 24, 125, 22 );
    g_HUD.AddButton( IDC_CHANGEDEVICE, L"Change device (F2)", 35, iY += 24, 125, 22 );
}


//--------------------------------------------------------------------------------------
// Called during device initialization, this code checks the device for some 
// minimum set of capabilities, and rejects those that don't pass by returning false.
//--------------------------------------------------------------------------------------
bool CALLBACK IsDeviceAcceptable( D3DCAPS9* pCaps, D3DFORMAT AdapterFormat, 
                                  D3DFORMAT BackBufferFormat, bool bWindowed )
{
    // Skip backbuffer formats that don't support alpha blending
    IDirect3D9* pD3D = DXUTGetD3DObject(); 
    if( FAILED( pD3D->CheckDeviceFormat( pCaps->AdapterOrdinal, pCaps->DeviceType,
                    AdapterFormat, D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING, 
                    D3DRTYPE_TEXTURE, BackBufferFormat ) ) )
        return false;
	// Check for vertex texture support we reque
    if( FAILED( pD3D->CheckDeviceFormat( pCaps->AdapterOrdinal, pCaps->DeviceType,
                    AdapterFormat, (D3DUSAGE_QUERY_VERTEXTEXTURE | D3DUSAGE_SOFTWAREPROCESSING), 
                    D3DRTYPE_TEXTURE, D3DFMT_A32B32G32R32F ) ) )
        return false;

    return true;
}


//--------------------------------------------------------------------------------------
// This callback function is called immediately before a device is created to allow the 
// application to modify the device settings. The supplied pDeviceSettings parameter 
// contains the settings that the framework has selected for the new device, and the 
// application can make any desired changes directly to this structure.  Note however that 
// the sample framework will not correct invalid device settings so care must be taken 
// to return valid device settings, otherwise IDirect3D9::CreateDevice() will fail.  
//--------------------------------------------------------------------------------------
void CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, const D3DCAPS9* pCaps )
{
    // If device doesn't support HW T&L or doesn't support 1.1 vertex shaders in HW 
    // then switch to SWVP.
    if( (pCaps->DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) == 0 ||
         pCaps->VertexShaderVersion < D3DVS_VERSION(3,0) )
    {
        pDeviceSettings->BehaviorFlags = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
    }
    else
    {
        pDeviceSettings->BehaviorFlags = D3DCREATE_HARDWARE_VERTEXPROCESSING;
    }

    // This application is designed to work on a pure device by not using 
    // IDirect3D9::Get*() methods, so create a pure device if supported and using HWVP.
    if ((pCaps->DevCaps & D3DDEVCAPS_PUREDEVICE) != 0 && 
        (pDeviceSettings->BehaviorFlags & D3DCREATE_HARDWARE_VERTEXPROCESSING) != 0 )
        pDeviceSettings->BehaviorFlags |= D3DCREATE_PUREDEVICE;

    // Debugging vertex shaders requires either REF or software vertex processing 
    // and debugging pixel shaders requires REF.  
#ifdef DEBUG_VS
    if( pDeviceSettings->DeviceType != D3DDEVTYPE_REF )
    {
        pDeviceSettings->BehaviorFlags &= ~D3DCREATE_HARDWARE_VERTEXPROCESSING;
        pDeviceSettings->BehaviorFlags &= ~D3DCREATE_PUREDEVICE;
        pDeviceSettings->BehaviorFlags |= D3DCREATE_SOFTWARE_VERTEXPROCESSING;
    }
#endif
#ifdef DEBUG_PS
    pDeviceSettings->DeviceType = D3DDEVTYPE_REF;
#endif
}

//--------------------------------------------------------------------------------------
// This callback function will be called immediately after the Direct3D device has been 
// created, which will happen during application initialization and windowed/full screen 
// toggles. This is the best location to create D3DPOOL_MANAGED resources since these 
// resources need to be reloaded whenever the device is destroyed. Resources created  
// here should be released in the OnDestroyDevice callback. 
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnCreateDevice( IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc )
{
    HRESULT hr;

    // Initialize the font
    V_RETURN( D3DXCreateFont( pd3dDevice, 15, 0, FW_BOLD, 1, FALSE, DEFAULT_CHARSET, 
                         OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, 
                         L"Arial", &g_pFont ) );

    // Define DEBUG_VS and/or DEBUG_PS to debug vertex and/or pixel shaders with the 
    // shader debugger. Debugging vertex shaders requires either REF or software vertex 
    // processing, and debugging pixel shaders requires REF.  The 
    // D3DXSHADER_FORCE_*_SOFTWARE_NOOPT flag improves the debug experience in the 
    // shader debugger.  It enables source level debugging, prevents instruction 
    // reordering, prevents dead code elimination, and forces the compiler to compile 
    // against the next higher available software target, which ensures that the 
    // unoptimized shaders do not exceed the shader model limitations.  Setting these 
    // flags will cause slower rendering since the shaders will be unoptimized and 
    // forced into software.  See the DirectX documentation for more information about 
    // using the shader debugger.
    DWORD dwShaderFlags = 0;
    #ifdef DEBUG_VS
        dwShaderFlags |= D3DXSHADER_FORCE_VS_SOFTWARE_NOOPT;
    #endif
    #ifdef DEBUG_PS
        dwShaderFlags |= D3DXSHADER_FORCE_PS_SOFTWARE_NOOPT;
    #endif

	D3DCAPS9 fakecaps;
	V_RETURN( pd3dDevice->GetDeviceCaps( &fakecaps ) );

	D3DCAPS9 caps;
	V_RETURN( DXUTGetD3DObject()->GetDeviceCaps( fakecaps.AdapterOrdinal, fakecaps.DeviceType, &caps ) );

	if( caps.VertexShaderVersion >= D3DVS_VERSION(3,0) )
	{
		g_softwareVS30 = false;
	} else
	{
		g_softwareVS30 = true;
	}

    // Read the D3DX effect file
    WCHAR str[MAX_PATH];
    V_RETURN( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, L"Topology1.fx" ) );


    // If this fails, there should be debug output as to 
    // they the .fx file failed to compile
    V_RETURN( D3DXCreateEffectFromFile( pd3dDevice, str, NULL, NULL, dwShaderFlags, 
                                        NULL, &g_pEffect, NULL ) );

	V_RETURN( D3DXCreateTeapot( pd3dDevice, &g_teapotMesh, 0 ) );
//	V_RETURN( D3DXCreateBox( pd3dDevice, 1,1,1, &g_teapotMesh, 0 ) );
//	V_RETURN( D3DXCreateSphere( pd3dDevice, 1,10,10, &g_teapotMesh, 0 ) );

	// not we assume the vertexmap texture have the same dimensions
	V_RETURN( VertexMapCreator::ConvertXToVertexMap( 
		pd3dDevice, g_teapotMesh, VertexMapCreator::VC_POSITION, g_positionTexture,
		g_vertexMapSizes ) );
	V_RETURN( VertexMapCreator::ConvertXToVertexMap( 
		pd3dDevice, g_teapotMesh, VertexMapCreator::VC_NORMAL, g_normalTexture,
		g_vertexMapSizes ) );

	V_RETURN( VertexMapCreator::ConvertXIndexToVertexStream( pd3dDevice, g_teapotMesh, g_indexVertexStream ) );
	V_RETURN( VertexMapCreator::Create123VertexStream( pd3dDevice, g_123VertexStream ) );
	V_RETURN( VertexMapCreator::Create123IndexStream( pd3dDevice, g_123IndexStream ) );

	V_RETURN( VertexMapCreator::ConvertXIndexToFaceTopologyMap( pd3dDevice, g_teapotMesh, g_topoMapTexture, g_topoMapSizes ) );
	V_RETURN( VertexMapCreator::CreateCounterVertexStream( pd3dDevice, g_teapotCounterStream, g_teapotMesh->GetNumFaces()*3 ) );

	g_numTriangles = g_teapotMesh->GetNumFaces();

	D3DVERTEXELEMENT9 indexDecl[] = 
	{
		{ 0, 0, D3DDECLTYPE_FLOAT1, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
		{ 1, 0, D3DDECLTYPE_FLOAT1, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 1 },
		D3DDECL_END(),
	};

	V_RETURN( pd3dDevice->CreateVertexDeclaration( indexDecl, &g_indexVSDecl ) );
	// Setup the camera's view parameters
    D3DXVECTOR3 vecEye(0.0f, 0.0f, -5.0f);
    D3DXVECTOR3 vecAt (0.0f, 0.0f, -0.0f);
    g_Camera.SetViewParams( &vecEye, &vecAt );

    return S_OK;
}


//--------------------------------------------------------------------------------------
// This function loads the mesh and ensures the mesh has normals; it also optimizes the 
// mesh for the graphics card's vertex cache, which improves performance by organizing 
// the internal triangle list for less cache misses.
//--------------------------------------------------------------------------------------
HRESULT LoadMesh( IDirect3DDevice9* pd3dDevice, WCHAR* strFileName, ID3DXMesh** ppMesh )
{
    ID3DXMesh* pMesh = NULL;
    WCHAR str[MAX_PATH];
    HRESULT hr;

    // Load the mesh with D3DX and get back a ID3DXMesh*.  For this
    // sample we'll ignore the X file's embedded materials since we know 
    // exactly the model we're loading.  See the mesh samples such as
    // "OptimizedMesh" for a more generic mesh loading example.
    V_RETURN( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, strFileName ) );

    V_RETURN( D3DXLoadMeshFromX(str, D3DXMESH_MANAGED, pd3dDevice, NULL, NULL, NULL, NULL, &pMesh) );

    DWORD *rgdwAdjacency = NULL;

    // Make sure there are normals which are required for lighting
    if( !(pMesh->GetFVF() & D3DFVF_NORMAL) )
    {
        ID3DXMesh* pTempMesh;
        V( pMesh->CloneMeshFVF( pMesh->GetOptions(), 
                                  pMesh->GetFVF() | D3DFVF_NORMAL, 
                                  pd3dDevice, &pTempMesh ) );
        V( D3DXComputeNormals( pTempMesh, NULL ) );

        SAFE_RELEASE( pMesh );
        pMesh = pTempMesh;
    }

    // Optimize the mesh for this graphics card's vertex cache 
    // so when rendering the mesh's triangle list the vertices will 
    // cache hit more often so it won't have to re-execute the vertex shader 
    // on those vertices so it will improve perf.     
    rgdwAdjacency = new DWORD[pMesh->GetNumFaces() * 3];
    if( rgdwAdjacency == NULL )
        return E_OUTOFMEMORY;
    V( pMesh->ConvertPointRepsToAdjacency(NULL, rgdwAdjacency) );
    V( pMesh->OptimizeInplace(D3DXMESHOPT_VERTEXCACHE, rgdwAdjacency, NULL, NULL, NULL) );
    delete []rgdwAdjacency;

    *ppMesh = pMesh;

    return S_OK;
}


//--------------------------------------------------------------------------------------
// This callback function will be called immediately after the Direct3D device has been 
// reset, which will happen after a lost device scenario. This is the best location to 
// create D3DPOOL_DEFAULT resources since these resources need to be reloaded whenever 
// the device is lost. Resources created here should be released in the OnLostDevice 
// callback. 
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnResetDevice( IDirect3DDevice9* pd3dDevice, 
                                const D3DSURFACE_DESC* pBackBufferSurfaceDesc )
{
    HRESULT hr;

    if( g_pFont )
        V_RETURN( g_pFont->OnResetDevice() );
    if( g_pEffect )
        V_RETURN( g_pEffect->OnResetDevice() );

    // Create a sprite to help batch calls when drawing many lines of text
    V_RETURN( D3DXCreateSprite( pd3dDevice, &g_pTextSprite ) );

    // Setup the camera's projection parameters
    float fAspectRatio = pBackBufferSurfaceDesc->Width / (FLOAT)pBackBufferSurfaceDesc->Height;
    g_Camera.SetProjParams( D3DX_PI/4, fAspectRatio, 0.1f, 1000.0f );
    g_Camera.SetWindow( pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height );

    g_HUD.SetLocation( pBackBufferSurfaceDesc->Width-170, 0 );
    g_HUD.SetSize( 170, 170 );
    g_SampleUI.SetLocation( pBackBufferSurfaceDesc->Width-170, pBackBufferSurfaceDesc->Height-350 );
    g_SampleUI.SetSize( 170, 300 );

    return S_OK;
}

float g_offNum = 0;
//--------------------------------------------------------------------------------------
// This callback function will be called once at the beginning of every frame. This is the
// best location for your application to handle updates to the scene, but is not 
// intended to contain actual rendering calls, which should instead be placed in the 
// OnFrameRender callback.  
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove( IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime )
{
    // Update the camera's position based on user input 
    g_Camera.FrameMove( fElapsedTime );

	static float elapTime = 0;
	elapTime += fElapsedTime;
	if( elapTime > 0.4f )
	{
		elapTime = 0;
		g_offNum++;
		if( g_offNum >= 20 )
		{
			g_offNum = 0;
		}
	}

}


//--------------------------------------------------------------------------------------
// This callback function will be called at the end of every frame to perform all the 
// rendering calls for the scene, and it will also be called if the window needs to be 
// repainted. After this function has returned, the sample framework will call 
// IDirect3DDevice9::Present to display the contents of the next buffer in the swap chain
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameRender( IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime )
{
    HRESULT hr;
    D3DXMATRIXA16 mWorld;
    D3DXMATRIXA16 mView;
    D3DXMATRIXA16 mProj;
    D3DXMATRIXA16 mWorldViewProjection;
    
    // Clear the render target and the zbuffer 
    V( pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB(0, 45, 50, 170), 1.0f, 0) );

    // Render the scene
    if( SUCCEEDED( pd3dDevice->BeginScene() ) )
    {
        // Get the projection & view matrix from the camera class
        mWorld = *g_Camera.GetWorldMatrix();
        mProj = *g_Camera.GetProjMatrix();
        mView = *g_Camera.GetViewMatrix();

        mWorldViewProjection = mWorld * mView * mProj;

        // Update the effect's variables.  Instead of using strings, it would 
        // be more efficient to cache a handle to the parameter by calling 
        // ID3DXEffect::GetParameterByName
        V( g_pEffect->SetMatrix( "g_mWorldViewProjection", &mWorldViewProjection ) );
        V( g_pEffect->SetMatrix( "g_mWorld", &mWorld ) );
        V( g_pEffect->SetFloat( "g_fTime", (float)fTime ) );
		V( g_pEffect->SetTexture( "g_positionVertexMap", g_positionTexture ) );
		V( g_pEffect->SetTexture( "g_normalVertexMap", g_normalTexture ) );
		V( g_pEffect->SetTexture( "g_uvVertexMap", g_normalTexture ) ); // TODO
		float vertSize[] = { (1.f / g_vertexMapSizes[0]), (1.f / g_vertexMapSizes[0]) * (1.f / g_vertexMapSizes[1] ) };
		V( g_pEffect->SetFloatArray( "g_vertexMapAddrConsts", vertSize, 2 ) );

		V( g_pEffect->SetTexture( "g_faceTopoMap", g_topoMapTexture ) );
		float topoSize[] = { (1.f / g_topoMapSizes[0]), (1.f / g_topoMapSizes[0]) * (1.f / g_topoMapSizes[1] ) };
		V( g_pEffect->SetFloatArray( "g_topoMapAddrConsts", topoSize, 2 ) );

		float OnArray[20];
		for( unsigned int i=0;i < 20;i++)
		{
			OnArray[i] = 1.f;
		}

		OnArray[ (unsigned int)(g_offNum) ] = 0.f;

		V( g_pEffect->SetFloatArray( "g_offArray", OnArray, 20 ) );

		unsigned int iPasses;
		g_pEffect->Begin( &iPasses, 0 );

		for( unsigned int i = 0;i < iPasses;i++)
		{
			g_pEffect->BeginPass( i );
			V( pd3dDevice->SetVertexDeclaration( g_indexVSDecl ) );
			// this streams is a indexed stream repeated n times
			V( pd3dDevice->SetStreamSource( 0, g_123VertexStream, 0, sizeof(float) ) );
			V( pd3dDevice->SetStreamSourceFreq( 0, g_numTriangles | D3DSTREAMSOURCE_INDEXEDDATA ) ); 
			// this is updated once every instance (per triangle)
			V( pd3dDevice->SetStreamSource( 1, g_teapotCounterStream, 0, sizeof(float) ) );
			V( pd3dDevice->SetStreamSourceFreq( 1, 1 | D3DSTREAMSOURCE_INSTANCEDATA ) );

			// for some weird reason vertex divided REQUIRES indexed primitve's now!!!
			V( pd3dDevice->SetIndices( g_123IndexStream ) );
			V( pd3dDevice->DrawIndexedPrimitive( D3DPT_TRIANGLELIST, 0, 0, 3, 0, 1 ) );

			g_pEffect->EndPass();
		}

		g_pEffect->End();

		V( pd3dDevice->SetStreamSourceFreq( 0, 1 ) ); // reset
		V( pd3dDevice->SetStreamSourceFreq( 1, 1 ) ); // reset
        RenderText();

        V( g_HUD.OnRender( fElapsedTime ) );
        V( g_SampleUI.OnRender( fElapsedTime ) );

        V( pd3dDevice->EndScene() );
    }
}

//-----------------------------------------------------------------------------
// Name: BaseLight ApplyFullScreenPixelShader
// Desc: Render a full screen quad with whatever settings are currently set
//-----------------------------------------------------------------------------
void ApplyFullScreenPixelShader( IDirect3DDevice9* pd3dDevice, const unsigned int width, const unsigned int height )
{
	// render pixel shader quad to combine the render target
	struct
	{
		float x,y,z,w;
		float u0,v0;
	} quad[4] = 
	{ 
		{ (float)0,		(float)0,		0.1f,1,		0+(1.f/width)/2.f, 0+(1.f/height)/2.f, }, 
		{ (float)width, (float)0,		0.1f,1,		1+(1.f/width)/2.f, 0+(1.f/height)/2.f, }, 
		{ (float)0,		(float)height,	0.1f,1,		0+(1.f/width)/2.f, 1+(1.f/height)/2.f }, 
		{ (float)width, (float)height,	0.1f,1,		1+(1.f/width)/2.f, 1+(1.f/height)/2.f, } 
	};

	pd3dDevice->SetFVF( D3DFVF_XYZRHW | D3DFVF_TEX1);
	pd3dDevice->DrawPrimitiveUP( D3DPT_TRIANGLESTRIP, 2, quad, sizeof(quad[0])  );
}

void RenderFaceNormalMap( IDirect3DDevice9* pd3dDevice )
{
	HRESULT hr;
	V( g_pEffect2->SetTexture( "g_positionVertexMap", g_positionTexture ) );
	float vertSize[] = { (1.f / g_vertexMapSizes[0]), (1.f / g_vertexMapSizes[0]) * (1.f / g_vertexMapSizes[1] ) };
	V( g_pEffect2->SetFloatArray( "g_vertexMapAddrConsts", vertSize, 2 ) );

	V( g_pEffect2->SetTexture( "g_faceTopoMap", g_topoMapTexture ) );

	IDirect3DSurface9* surface;

	V( g_rtFaceNormalTexture->GetSurfaceLevel(0, &surface ) );
	pd3dDevice->SetRenderTarget(0, surface );
	surface->Release();

	unsigned int iPasses;
	g_pEffect2->Begin( &iPasses, 0 );

	for( unsigned int i = 0;i < iPasses;i++)
	{
		g_pEffect2->BeginPass( i );	
		ApplyFullScreenPixelShader( pd3dDevice, (unsigned int)g_topoMapSizes[0], 
												(unsigned int)g_topoMapSizes[1] );
		g_pEffect2->EndPass();
	}

	g_pEffect2->End();
}

//--------------------------------------------------------------------------------------
// Render the help and statistics text. This function uses the ID3DXFont interface for 
// efficient text rendering.
//--------------------------------------------------------------------------------------
void RenderText()
{
	return;
    // The helper object simply helps keep track of text position, and color
    // and then it calls pFont->DrawText( m_pSprite, strMsg, -1, &rc, DT_NOCLIP, m_clr );
    // If NULL is passed in as the sprite object, then it will work however the 
    // pFont->DrawText() will not be batched together.  Batching calls will improves performance.
    const D3DSURFACE_DESC* pd3dsdBackBuffer = DXUTGetBackBufferSurfaceDesc();
    CDXUTTextHelper txtHelper( g_pFont, g_pTextSprite, 15 );

    // Output statistics
    txtHelper.Begin();
    txtHelper.SetInsertionPos( 5, 5 );
    txtHelper.SetForegroundColor( D3DXCOLOR( 1.0f, 1.0f, 0.0f, 1.0f ) );
    txtHelper.DrawTextLine( DXUTGetFrameStats() );
    txtHelper.DrawTextLine( DXUTGetDeviceStats() );

    txtHelper.SetForegroundColor( D3DXCOLOR( 1.0f, 1.0f, 1.0f, 1.0f ) );
    txtHelper.DrawTextLine( L"Put whatever misc status here" );
    
    // Draw help
    if( g_bShowHelp )
    {
        txtHelper.SetInsertionPos( 10, pd3dsdBackBuffer->Height-15*6 );
        txtHelper.SetForegroundColor( D3DXCOLOR( 1.0f, 0.75f, 0.0f, 1.0f ) );
        txtHelper.DrawTextLine( L"Controls (F1 to hide):" );

        txtHelper.SetInsertionPos( 40, pd3dsdBackBuffer->Height-15*5 );
        txtHelper.DrawTextLine( L"Blah: X\n"
                                L"Quit: ESC" );
    }
    else
    {
        txtHelper.SetInsertionPos( 10, pd3dsdBackBuffer->Height-15*2 );
        txtHelper.SetForegroundColor( D3DXCOLOR( 1.0f, 1.0f, 1.0f, 1.0f ) );
        txtHelper.DrawTextLine( L"Press F1 for help" );
    }
    txtHelper.End();
}


//--------------------------------------------------------------------------------------
// Before handling window messages, the sample framework passes incoming windows 
// messages to the application through this callback function. If the application sets 
// *pbNoFurtherProcessing to TRUE, then the sample framework will not process this message.
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing )
{
    // Give the dialogs a chance to handle the message first
    *pbNoFurtherProcessing = g_HUD.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;
    *pbNoFurtherProcessing = g_SampleUI.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

    // Pass all remaining windows messages to camera so it can respond to user input
    g_Camera.HandleMessages( hWnd, uMsg, wParam, lParam );

    return 0;
}


//--------------------------------------------------------------------------------------
// As a convenience, the sample framework inspects the incoming windows messages for
// keystroke messages and decodes the message parameters to pass relevant keyboard
// messages to the application.  The framework does not remove the underlying keystroke 
// messages, which are still passed to the application's MsgProc callback.
//--------------------------------------------------------------------------------------
void CALLBACK KeyboardProc( UINT nChar, bool bKeyDown, bool bAltDown )
{
    if( bKeyDown )
    {
        switch( nChar )
        {
            case VK_F1: g_bShowHelp = !g_bShowHelp; break;
        }
    }
}


//--------------------------------------------------------------------------------------
// Handles the GUI events
//--------------------------------------------------------------------------------------
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl )
{
    switch( nControlID )
    {
        case IDC_TOGGLEFULLSCREEN: DXUTToggleFullScreen(); break;
        case IDC_TOGGLEREF:        DXUTToggleREF(); break;
        case IDC_CHANGEDEVICE:     DXUTSetShowSettingsDialog( !DXUTGetShowSettingsDialog() ); break;
    }
}


//--------------------------------------------------------------------------------------
// This callback function will be called immediately after the Direct3D device has 
// entered a lost state and before IDirect3DDevice9::Reset is called. Resources created
// in the OnResetDevice callback should be released here, which generally includes all 
// D3DPOOL_DEFAULT resources. See the "Lost Devices" section of the documentation for 
// information about lost devices.
//--------------------------------------------------------------------------------------
void CALLBACK OnLostDevice()
{
    if( g_pFont )
        g_pFont->OnLostDevice();
    if( g_pEffect )
        g_pEffect->OnLostDevice();
    SAFE_RELEASE( g_pTextSprite );
}


//--------------------------------------------------------------------------------------
// This callback function will be called immediately after the Direct3D device has 
// been destroyed, which generally happens as a result of application termination or 
// windowed/full screen toggles. Resources created in the OnCreateDevice callback 
// should be released here, which generally includes all D3DPOOL_MANAGED resources. 
//--------------------------------------------------------------------------------------
void CALLBACK OnDestroyDevice()
{
	SAFE_RELEASE( g_rtFaceNormalTexture );
	SAFE_RELEASE( g_teapotCounterStream );
	SAFE_RELEASE( g_topoMapTexture );
	SAFE_RELEASE( g_123IndexStream );
	SAFE_RELEASE( g_123VertexStream );
	SAFE_RELEASE( g_indexVSDecl );
	SAFE_RELEASE( g_indexVertexStream );
	SAFE_RELEASE( g_normalTexture );
	SAFE_RELEASE( g_positionTexture );
	SAFE_RELEASE( g_teapotMesh );
	SAFE_RELEASE( g_pEffect2 );
    SAFE_RELEASE( g_pEffect );
    SAFE_RELEASE( g_pFont );
}

