﻿// C/C++ Common
#include <cstdio>
#include <cmath>
// Windows
#include <windows.h>
#include <tchar.h>
// DirectX
#include <d3d11.h>
#include <d3dx10math.h>
// Others...
#include <dtprintf.h>
// Shaders
#include "perspective.vs.h"		// 頂点シェーダ (3 軸線表示用)
#include "lambert.vs.h"			// 頂点シェーダ (モデル表示用)
#include "constant.ps.h"		// ピクセルシェーダ


#define COM_SAFE_RELEASE( p ) { if(p) { (p)->Release(); (p) = NULL; } }


TCHAR *g_className  = _T( "013_MikuMikuDanceLambert" );
TCHAR *g_windowName = g_className;



// ウィンドウプロシージャ
LRESULT CALLBACK WndProc( HWND hWnd, UINT mes, WPARAM wParam, LPARAM lParam )
{
	switch ( mes )
	{
	case WM_DESTROY:
		PostQuitMessage( 0 );
		return 0;
	}
	return DefWindowProc( hWnd, mes, wParam, lParam );
}



// WinMain
int APIENTRY _tWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow )
{
	HRESULT hr;


	// ウィンドウクラスを登録
	WNDCLASSEX wcex = {
		sizeof( WNDCLASSEX ),			// cbSize
		CS_HREDRAW | CS_VREDRAW,		// style
		WndProc,						// lpfnWndProc
		0,								// cbClsExtra
		0,								// cbWndExtra
		hInstance,						// hInstance
		NULL,							// hIcon
		NULL,							// hCursor
		( HBRUSH )( COLOR_WINDOW + 1 ),	// hbrBackGround
		NULL,							// lpszMenuName
		g_className,					// lpszClassName
		NULL							// hIconSm
	};
	if ( ! RegisterClassEx( &wcex ) )
	{
		MessageBox( NULL, _T( "失敗: RegisterClassEx()" ), _T( "エラー" ), MB_OK | MB_ICONERROR );
		return 0;
	}
	dtprintf( _T( "RegisterClassEx: ok\n" ) );


	// ウィンドウサイズを計算
	RECT r = { 0, 0, 800, 450 };   // 800x450 (16:9)
	if ( ! AdjustWindowRect( &r, WS_OVERLAPPEDWINDOW, FALSE ) )
	{
		MessageBox( NULL, _T( "失敗: AdjustWindowRect()" ), _T( "エラー" ),  MB_OK | MB_ICONERROR );
		return 0;
	}
	dtprintf( _T( "AdjustWindowRect: ok (%d, %d)－(%d, %d)\n" ), r.left, r.top, r.right, r.bottom );


	// ウィンドウ生成
	HWND hWnd;
	hWnd = CreateWindow( g_className,
	                     g_windowName,
	                     WS_OVERLAPPEDWINDOW,
	                     CW_USEDEFAULT,
	                     0,
	                     r.right - r.left,
	                     r.bottom - r.top,
	                     NULL,
	                     NULL,
	                     hInstance,
	                     NULL );
	if ( hWnd == NULL )
	{
		MessageBox( NULL, _T( "失敗: CreateWindow()" ), _T( "エラー" ), MB_OK | MB_ICONERROR );
		return 0;
	}
	dtprintf( _T( "CreateWindow: ok\n" ) );


	// ウィンドウ表示
	ShowWindow(hWnd, nCmdShow);
	dtprintf( _T( "ShowWindow: ok\n" ) );



	// スワップチェイン設定
	DXGI_SWAP_CHAIN_DESC scDesc = {
		{
			1280,									// BufferDesc.Width
			720,									// BufferDesc.Height
			{
				60,									// BufferDesc.RefreshRate.Numerator
				1									// BufferDesc.RefreshRate.Denominator
			},
			DXGI_FORMAT_R16G16B16A16_FLOAT,			// BufferDesc.Format
			DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,	// BufferDesc.ScanlineOrdering
			DXGI_MODE_SCALING_CENTERED				// BufferDesc.Scaling
		},
		{
			1,										// SampleDesc.Count
			0										// SampleDesc.Quality
		},
		DXGI_USAGE_RENDER_TARGET_OUTPUT,			// BufferUsage
		1,											// BufferCount
		hWnd,										// OutputWindow
		TRUE,										// Windowed
		DXGI_SWAP_EFFECT_DISCARD,					// SwapEffect
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH		// Flags
	};

	// Direct3D11 デバイス・デバイスコンテキスト・スワップチェーンを生成
	ID3D11Device        * pDevice        = NULL;
	ID3D11DeviceContext * pDeviceContext = NULL;
	IDXGISwapChain      * pSwapChain     = NULL;
	D3D_FEATURE_LEVEL     feature;
	hr = D3D11CreateDeviceAndSwapChain( NULL,
	                                    D3D_DRIVER_TYPE_HARDWARE,
	                                    NULL,
	                                    0,
	                                    NULL,
	                                    0,
	                                    D3D11_SDK_VERSION,
	                                    &scDesc,
	                                    &pSwapChain,
	                                    &pDevice,
	                                    &feature,
	                                    &pDeviceContext );
	if ( FAILED( hr ) )
	{
		MessageBox( NULL, _T( "失敗: D3D11CreateDeviceAndSwapChain()" ), _T( "エラー" ), MB_OK | MB_ICONERROR );
		return 0;
	}
	dtprintf( _T( "D3D11CreateDeviceAndSwapChain: ok (pDevice: 0x%p, pDeviceContext: 0x%p, pSwapChain: 0x%p, feature: 0x%4x)\n" ),
	          pDevice,
	          pDeviceContext,
	          pSwapChain,
	          ( int ) feature );


	// バックバッファテクスチャを取得
	ID3D11Texture2D * pBackBuffer = NULL;
	hr = pSwapChain->GetBuffer( 0, __uuidof( pBackBuffer ), reinterpret_cast< void ** >( &pBackBuffer ) );
	if ( FAILED( hr ) )
	{
		MessageBox( NULL, _T( "失敗: IDXGISwapChain::GetBuffer()" ), _T( "エラー" ), MB_OK | MB_ICONERROR );
		return 0;
	}
	dtprintf( _T( "IDXGISwapChain::GetBuffer: ok (pBackBuffer: 0x%p)\n" ), pBackBuffer );


	// レンダーターゲットビューを生成
	ID3D11RenderTargetView * pRenderTargetView = NULL;
	hr = pDevice->CreateRenderTargetView( pBackBuffer, NULL, &pRenderTargetView );
	if ( FAILED( hr ) )
	{
		MessageBox( NULL, _T( "失敗: ID3D11Device::CreateRenderTargetView()" ), _T( "エラー" ), MB_OK | MB_ICONERROR );
		return 0;
	}
	dtprintf( _T( "ID3D11Device::CreateRenderTargetView: ok (pRenderTargetView: 0x%p)\n" ), pRenderTargetView );


	// デプス・ステンシルバッファとなるテクスチャを生成
	D3D11_TEXTURE2D_DESC depthStencilBufferDesc = {
		1280,						// Width
		720,						// Height
		1,							// MipLevels
		1,							// ArraySize
		DXGI_FORMAT_D32_FLOAT,		// Format
		{
			1,						// SampleDesc.Count
			0						// SampleDesc.Quality
		},
		D3D11_USAGE_DEFAULT,		// Usage
		D3D11_BIND_DEPTH_STENCIL,	// BindFlags
		0,							// CPUAccessFlags
		0							// MiscFlags
	};

	ID3D11Texture2D * pDepthStencilBuffer = NULL;
	hr = pDevice->CreateTexture2D( &depthStencilBufferDesc, NULL, &pDepthStencilBuffer );
	if ( FAILED( hr ) )
	{
		MessageBox( NULL, _T( "失敗: ID3D11Device::CreateTexture2D()" ), _T( "エラー" ), MB_OK | MB_ICONERROR );
		return 0;
	}
	dtprintf( _T( "ID3D11Device::CreateTexture2D: ok (pDepthStencilBuffer: 0x%p)\n" ), pDepthStencilBuffer );

	// デプス・ステンシルビューを生成
	ID3D11DepthStencilView * pDepthStencilView = NULL;
	hr = pDevice->CreateDepthStencilView( pDepthStencilBuffer, NULL, &pDepthStencilView );
	if ( FAILED( hr ) )
	{
		MessageBox( NULL, _T( "失敗: ID3D11Device::CreateDepthStencilView()" ), _T( "エラー" ), MB_OK | MB_ICONERROR );
		return 0;
	}
	dtprintf( _T( "ID3D11Device::CreateDepthStencilView: ok (pDepthStencilView: 0x%p)\n" ), pDepthStencilView );


	// レンダーターゲットビューとデプス・ステンシルビューをバインド
	ID3D11RenderTargetView * pRenderTargetViews[] = { pRenderTargetView };
	pDeviceContext->OMSetRenderTargets( 1, pRenderTargetViews, pDepthStencilView );
	dtprintf( _T( "ID3D11DeviceContext::OMSetRenderTargets: ok\n" ) );

	// バックバッファはもうここでは使わない
	COM_SAFE_RELEASE( pBackBuffer );


	// ビューポートをバインド
	D3D11_VIEWPORT viewport = {
		   0.0f,		// TopLeftX
		   0.0f,		// TopLeftY
		1280.0f,		// Width
		 720.0f,		// Height
		   0.0f,		// MinDepth
		   1.0f			// MaxDepth
	};
	pDeviceContext->RSSetViewports( 1, &viewport );
	dtprintf( _T( "ID3D11DeviceContext::RSSetViewports: ok\n" ) );



	// MikuMikuDance 用モデルデータ読み込み
	// fopen の引数 path/to/pmd_file を、モデルデータファイルに変更してください。
	FILE *fp;
	unsigned int   vertices_count = 0;
	unsigned int   faces_count    = 0;
	float        * pmdVertices    = NULL;
	unsigned int * pmdIndices     = NULL;

	if ( ( fp = fopen( "path/to/pmd_file", "rb" ) ) == NULL )
	{
		MessageBox( NULL, _T( "失敗: fopen()" ), _T( "エラー" ), MB_OK | MB_ICONERROR );
		return 0;
	}
	// 頂点数
	fseek( fp, 283, SEEK_SET );
	fread( &vertices_count, sizeof( unsigned int ), 1, fp );
	// 頂点データ用メモリ確保・読み込み
	pmdVertices = new float[ 6 * vertices_count ];
	for( unsigned int i = 0; i < vertices_count; ++i )
	{
		fread( pmdVertices + 6 * i, sizeof( float ), 6, fp );
		pmdVertices[ 6 * i + 2 ] *= -1.0f;
		pmdVertices[ 6 * i + 5 ] *= -1.0f;

		// そのほかのデータは読み飛ばす
		fseek( fp, 38 - sizeof( float ) * 6, SEEK_CUR );
	}
	// 面数
	fread( &faces_count, sizeof( unsigned int ), 1, fp );
	faces_count /= 3;
	// 面データ用メモリ確保・読み込み
	pmdIndices = new unsigned int[ 3 * faces_count ];
	for( unsigned int i = 0; i < faces_count; ++i )
	{
		// モデルデータの面頂点インデックスは 16bit なので
		// いったん 16bit で読み込み 32bit 化して代入
		unsigned short indices[ 3 ] = { 0 };
		fread( indices, sizeof( unsigned short ), 3, fp );

		pmdIndices[ 3 * i + 0 ] = indices[ 0 ];
		pmdIndices[ 3 * i + 1 ] = indices[ 1 ];
		pmdIndices[ 3 * i + 2 ] = indices[ 2 ];
	}

	fclose( fp );



	// モデル表示用の頂点バッファを生成
	D3D11_BUFFER_DESC pmdVertexBufferDesc = {
		sizeof( float ) * 6 * vertices_count,	// ByteWidth
		D3D11_USAGE_DEFAULT,					// Usage
		D3D11_BIND_VERTEX_BUFFER,				// BindFlags
		0,										// CPUAccessFlags
		0,										// MiscFlags
		0										// StructureByteStride
	};
	D3D11_SUBRESOURCE_DATA pmdVertexResourceData = { pmdVertices };

	ID3D11Buffer * pPmdVertexBuffer = NULL;
	hr = pDevice->CreateBuffer( &pmdVertexBufferDesc, &pmdVertexResourceData, &pPmdVertexBuffer );
	if ( FAILED( hr ) )
	{
		MessageBox( NULL, _T( "失敗: ID3D11Device::CreateBuffer()" ), _T( "エラー" ), MB_OK | MB_ICONERROR );
		return 0;
	}
	dtprintf( _T( "ID3D11Device::CreateBuffer: ok (pPmdVertexBuffer: 0x%p)\n" ), pPmdVertexBuffer );


	// モデル表示用のインデックスバッファを生成
	D3D11_BUFFER_DESC pmdIndexBufferDesc = {
		sizeof( unsigned int ) * 3 * faces_count,	// ByteWidth
		D3D11_USAGE_DEFAULT,						// Usage
		D3D11_BIND_INDEX_BUFFER,					// BindFlags
		0,											// CPUAccessFlags
		0,											// MiscFlags
		0											// StructureByteStride
	};
	D3D11_SUBRESOURCE_DATA pmdIndexResourceData = { pmdIndices };

	ID3D11Buffer * pPmdIndexBuffer = NULL;
	hr = pDevice->CreateBuffer( &pmdIndexBufferDesc, &pmdIndexResourceData, &pPmdIndexBuffer );
	if ( FAILED( hr ) )
	{
		MessageBox( NULL, _T( "失敗: ID3D11Device::CreateBuffer()" ), _T( "エラー" ), MB_OK | MB_ICONERROR );
		return 0;
	}
	dtprintf( _T( "ID3D11Device::CreateBuffer: ok (pPmdIndexBuffer: 0x%p)\n" ), pPmdIndexBuffer );

	// モデル表示用のインデックスバッファをバインド
	pDeviceContext->IASetIndexBuffer( pPmdIndexBuffer, DXGI_FORMAT_R32_UINT, 0 );
	dtprintf( _T( "ID3D11DeviceContext::IASetIndexBuffer: ok\n" ) );


	// 3 軸線の頂点データ
	float triaxialVertices[ 12 ][ 7 ] = {
	//     Xaxis   Yaxis   Zaxis  赤     緑     青     Alpha
		{   0.0f,   0.0f,   0.0f,  1.0f,  0.0f,  0.0f,  1.0f },   // x 軸正方向
		{  10.0f,   0.0f,   0.0f,  1.0f,  0.0f,  0.0f,  1.0f },
		{   0.0f,   0.0f,   0.0f,  0.2f,  0.0f,  0.0f,  1.0f },   // x 軸負方向
		{ -10.0f,   0.0f,   0.0f,  0.2f,  0.0f,  0.0f,  1.0f },
		{   0.0f,   0.0f,   0.0f,  0.0f,  1.0f,  0.0f,  1.0f },   // y 軸正方向
		{   0.0f,  20.0f,   0.0f,  0.0f,  1.0f,  0.0f,  1.0f },
		{   0.0f,   0.0f,   0.0f,  0.0f,  0.2f,  0.0f,  1.0f },   // y 軸負方向
		{   0.0f, -20.0f,   0.0f,  0.0f,  0.2f,  0.0f,  1.0f },
		{   0.0f,   0.0f,   0.0f,  0.0f,  0.0f,  1.0f,  1.0f },   // z 軸正方向
		{   0.0f,   0.0f,  10.0f,  0.0f,  0.0f,  1.0f,  1.0f },
		{   0.0f,   0.0f,   0.0f,  0.0f,  0.0f,  0.2f,  1.0f },   // z 軸負方向
		{   0.0f,   0.0f, -10.0f,  0.0f,  0.0f,  0.2f,  1.0f }
	};

	// 3 軸線の頂点バッファを生成
	D3D11_BUFFER_DESC triaxialVertexBufferDesc = {
		sizeof( triaxialVertices ),		// ByteWidth
		D3D11_USAGE_DEFAULT,			// Usage
		D3D11_BIND_VERTEX_BUFFER,		// BindFlags
		0,								// CPUAccessFlags
		0,								// MiscFlags
		0								// StructureByteStride
	};
	D3D11_SUBRESOURCE_DATA triaxialVertexResourceData = { triaxialVertices };

	ID3D11Buffer * pTriaxialVertexBuffer = NULL;
	hr = pDevice->CreateBuffer( &triaxialVertexBufferDesc, &triaxialVertexResourceData, &pTriaxialVertexBuffer );
	if ( FAILED( hr ) )
	{
		MessageBox( NULL, _T( "失敗: ID3D11Device::CreateBuffer()" ), _T( "エラー" ), MB_OK | MB_ICONERROR );
		return 0;
	}
	dtprintf( _T( "ID3D11Device::CreateBuffer: ok (pTriaxialVertexBuffer: 0x%p)\n" ), pTriaxialVertexBuffer );


	// 頂点シェーダ用の定数バッファを作成
	D3D11_BUFFER_DESC VSConstantBufferDesc = {
		sizeof( D3DXMATRIX ) * 3 +
		 sizeof( D3DXVECTOR4 ) * 5,		// ByteWidth
		D3D11_USAGE_DYNAMIC,			// Usage
		D3D11_BIND_CONSTANT_BUFFER,		// BindFlags
		D3D11_CPU_ACCESS_WRITE,			// CPUAccessFlags
		0,								// MiscFlags
		0								// StructureByteStride
	};

	ID3D11Buffer * pVSConstantBuffer = NULL;
	hr = pDevice->CreateBuffer( &VSConstantBufferDesc, NULL, &pVSConstantBuffer );
	if ( FAILED( hr ) )
	{
		MessageBox( NULL, _T( "失敗: ID3D11Device::CreateBuffer()" ), _T( "エラー" ), MB_OK | MB_ICONERROR );
		return 0;
	}
	dtprintf( _T( "ID3D11Device::CreateBuffer: ok (pVSConstantBuffer: 0x%p)\n" ), pVSConstantBuffer );

	// 定数バッファをバインド
	pDeviceContext->VSSetConstantBuffers( 0, 1, &pVSConstantBuffer );
	dtprintf( _T( "ID3D11DeviceContext::VSSetConstantBuffers: ok\n" ) );


	// 3 軸線表示用の頂点シェーダを作成
	ID3D11VertexShader * pTriaxialVertexShader = NULL;
	hr = pDevice->CreateVertexShader( g_vs_perspective, sizeof( g_vs_perspective ), NULL, &pTriaxialVertexShader );
	if ( FAILED( hr ) )
	{
		MessageBox( NULL, _T( "失敗: ID3D11Device::CreateVertexShader()" ), _T( "エラー" ), MB_OK | MB_ICONERROR );
		return 0;
	}
	dtprintf( _T( "ID3D11Device::CreateVertexShader: ok (pTriaxialVertexShader: 0x%p)\n" ), pTriaxialVertexShader );

	// モデル表示用の頂点シェーダを作成
	ID3D11VertexShader * pPmdVertexShader = NULL;
	hr = pDevice->CreateVertexShader( g_vs_lambert, sizeof( g_vs_lambert ), NULL, &pPmdVertexShader );
	if ( FAILED( hr ) )
	{
		MessageBox( NULL, _T( "失敗: ID3D11Device::CreateVertexShader()" ), _T( "エラー" ), MB_OK | MB_ICONERROR );
		return 0;
	}
	dtprintf( _T( "ID3D11Device::CreateVertexShader: ok (pPmdVertexShader: 0x%p)\n" ), pPmdVertexShader );

	// ピクセルシェーダを作成
	ID3D11PixelShader * pPixelShader = NULL;
	hr = pDevice->CreatePixelShader( g_ps_constant, sizeof( g_ps_constant ), NULL, &pPixelShader );
	if ( FAILED( hr ) )
	{
		MessageBox( NULL, _T( "失敗: ID3D11Device::CreatePixelShader()" ), _T( "エラー" ), MB_OK | MB_ICONERROR );
		return 0;
	}
	dtprintf( _T( "ID3D11Device::CreatePixelShader: ok (pPixelShader: 0x%p)\n" ), pPixelShader );

	// シェーダをバインド
	pDeviceContext->VSSetShader( NULL, NULL, 0 );
//	pDeviceContext->VSSetShader( pVertexShader, NULL, 0 );
//	dtprintf( _T( "ID3D11DeviceContext::VSSetShader: ok\n" ) );
	pDeviceContext->PSSetShader( pPixelShader, NULL, 0 );
	dtprintf( _T( "ID3D11DeviceContext::PSSetShader: ok\n" ) );
	pDeviceContext->GSSetShader( NULL, NULL, 0 );
	pDeviceContext->HSSetShader( NULL, NULL, 0 );
	pDeviceContext->DSSetShader( NULL, NULL, 0 );


	// 3 軸線表示用の入力レイアウトを作成
	D3D11_INPUT_ELEMENT_DESC triaxialVerticesDesc[] = {
		{ "IN_POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,               D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "IN_COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, sizeof(float)*3, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	ID3D11InputLayout * pTriaxialInputLayout = NULL;
	hr = pDevice->CreateInputLayout( triaxialVerticesDesc, 2, g_vs_perspective, sizeof( g_vs_perspective ), &pTriaxialInputLayout );
	if ( FAILED( hr ) )
	{
		MessageBox( NULL, _T( "失敗: ID3D11Device::CreateInputLayout()" ), _T( "エラー" ), MB_OK | MB_ICONERROR );
		return 0;
	}
	dtprintf( _T( "ID3D11Device::CreateInputLayout: ok (pTriaxialInputLayout: 0x%p)\n" ), pTriaxialInputLayout );

	// モデル表示用の入力レイアウトを作成
	D3D11_INPUT_ELEMENT_DESC pmdVerticesDesc[] = {
		{ "IN_POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,               D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "IN_NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, sizeof(float)*3, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	ID3D11InputLayout * pPmdInputLayout = NULL;
	hr = pDevice->CreateInputLayout( pmdVerticesDesc, 2, g_vs_lambert, sizeof( g_vs_lambert ), &pPmdInputLayout );
	if ( FAILED( hr ) )
	{
		MessageBox( NULL, _T( "失敗: ID3D11Device::CreateInputLayout()" ), _T( "エラー" ), MB_OK | MB_ICONERROR );
		return 0;
	}
	dtprintf( _T( "ID3D11Device::CreateInputLayout: ok (pPmdInputLayout: 0x%p)\n" ), pPmdInputLayout );


	// ラスタライザステートを生成
	D3D11_RASTERIZER_DESC rasterizerStateDesc = {
		D3D11_FILL_SOLID,		// FillMode
//		D3D11_FILL_WIREFRAME,	// FillMode (ワイヤーフレーム表示)
		D3D11_CULL_BACK,		// CullMode
//		D3D11_CULL_NONE,		// CullMode (カリングなし)
		FALSE,					// FrontCounterClockwise
//		TRUE,					// FrontCounterClockwise
		0,						// DepthBias
		0.0f,					// DepthBiasClamp
		0.0f,					// SlopeScaledDepthBias
		TRUE,					// DepthClipEnable
		FALSE,					// ScissorEnable
		FALSE,					// MultisampleEnable
		FALSE					// AntialiasedLineEnable
	};

	ID3D11RasterizerState * pRasterizerState = NULL;
	hr = pDevice->CreateRasterizerState( &rasterizerStateDesc, &pRasterizerState );
	if ( FAILED( hr ) )
	{
		MessageBox( NULL, _T( "失敗: ID3D11Device::CreateRasterizerState()" ), _T( "エラー" ), MB_OK | MB_ICONERROR );
		return 0;
	}
	dtprintf( _T( "ID3D11Device::CreateRasterizerState: ok (pRasterizerState: 0x%p)\n" ), pRasterizerState );

	// ラスタライザステートをバインド
	pDeviceContext->RSSetState( pRasterizerState );
	dtprintf( _T( "ID3D11DeviceContext::RSSetState: ok\n" ) );


	// デプス・ステンシルステートを生成
	D3D11_DEPTH_STENCIL_DESC depthStencilStateDesc = {
		TRUE,								// DepthEnable
		D3D11_DEPTH_WRITE_MASK_ALL,			// DepthWriteMask
		D3D11_COMPARISON_LESS,				// DepthFunc
		FALSE,								// StencilEnable
		D3D11_DEFAULT_STENCIL_READ_MASK,	// StencilReadMask
		D3D11_DEFAULT_STENCIL_WRITE_MASK,	// StencilWriteMask
		{
			D3D11_STENCIL_OP_KEEP,			// FrontFace.StencilFailOp
			D3D11_STENCIL_OP_KEEP,			// FrontFace.StencilDepthFailOp
			D3D11_STENCIL_OP_KEEP,			// FrontFace.StencilPassOp
			D3D11_COMPARISON_ALWAYS			// FrontFace.StencilFunc
		},
		{
			D3D11_STENCIL_OP_KEEP,			// BackFace.StencilFailOp
			D3D11_STENCIL_OP_KEEP,			// BackFace.StencilDepthFailOp
			D3D11_STENCIL_OP_KEEP,			// BackFace.StencilPassOp
			D3D11_COMPARISON_ALWAYS			// BackFace.StencilFunc
		}
	};

	ID3D11DepthStencilState * pDepthStencilState = NULL;
	hr = pDevice->CreateDepthStencilState( &depthStencilStateDesc, &pDepthStencilState );
	if ( FAILED( hr ) )
	{
		MessageBox( NULL, _T( "失敗: ID3D11Device::CreateDepthStencilState()" ), _T( "エラー" ), MB_OK | MB_ICONERROR );
		return 0;
	}
	dtprintf( _T( "ID3D11Device::CreateDepthStencilState: ok (pDepthStencilState: 0x%p)\n" ), pDepthStencilState );

	// デプス・ステンシルステートをバインド
	pDeviceContext->OMSetDepthStencilState( pDepthStencilState, 0 );
	dtprintf( _T( "ID3D11DeviceContext::OMSetDepthStencilState: ok\n" ) );



	MSG msg;

	while ( 1 )
	{
		// メッセージを取得
		if ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
		{
			if ( msg.message == WM_QUIT )
			{
				dtprintf( _T( "PeekMessage: WM_QUIT\n" ) );
				break;
			}
			// メッセージ処理
			DispatchMessage( &msg );
		}
		else
		{
			HRESULT hr;

			static unsigned int count = 0;

			float theta = ( count++ / 200.0f ) * ( 3.141593f / 2.0f );


			// World-View-Projection 行列をそれぞれ生成
			D3DXMATRIX world, view, projection;

			D3DXMatrixIdentity( &world );

			const D3DXVECTOR3 eye( 15.0f * 1.414214f * -cosf( theta ), 15.0f, 15.0f * 1.414214f * sinf( theta ) );
			const D3DXVECTOR3 at( 0.0f, 8.0f, 0.0f );
			const D3DXVECTOR3 up( 0.0f, 1.0f, 0.0f );
			D3DXMatrixLookAtRH( &view, &eye, &at, &up );

			D3DXMatrixPerspectiveFovRH( &projection, 3.141593f / 4.0f, 1280.0f / 720.0f, 1.0f, 10000.0f );


			// 頂点シェーダ用定数バッファへアクセス
			D3D11_MAPPED_SUBRESOURCE mapped;
			hr = pDeviceContext->Map( pVSConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped );
			if ( SUCCEEDED( hr ) )
			{
				D3DXMATRIX * mapped_m = static_cast< D3DXMATRIX * >( mapped.pData );

				mapped_m[0] = world;
				mapped_m[1] = view;
				mapped_m[2] = projection;

				D3DXVECTOR4 * mapped_v = reinterpret_cast< D3DXVECTOR4 * >( mapped_m + 3 );
				// 光源の位置ベクトル
				mapped_v[0].x = 1.0f;
				mapped_v[0].y = 0.5f;
				mapped_v[0].z = 1.0f;
				mapped_v[0].w = 0.0f;
				// 環境光の色
				mapped_v[1].x = 0.3f;
				mapped_v[1].y = 0.3f;
				mapped_v[1].z = 0.3f;
				mapped_v[1].w = 1.0f;
				// 環境光反射係数
				mapped_v[2].x = 1.0f;
				mapped_v[2].y = 1.0f;
				mapped_v[2].z = 1.0f;
				mapped_v[2].w = 1.0f;
				// 拡散反射光の色
				mapped_v[3].x = 0.7f;
				mapped_v[3].y = 0.7f;
				mapped_v[3].z = 0.7f;
				mapped_v[3].w = 1.0f;
				// 拡散反射光反射係数
				mapped_v[4].x = 1.0f;
				mapped_v[4].y = 1.0f;
				mapped_v[4].z = 1.0f;
				mapped_v[4].w = 1.0f;

				// 後始末
				pDeviceContext->Unmap( pVSConstantBuffer, 0 );
			}


			// レンダーターゲットビューをクリア
			const float clear[ 4 ] = { 0.0f, 0.25f, 0.5f, 1.0f };	// RGBA
			pDeviceContext->ClearRenderTargetView( pRenderTargetView, clear );

			// デプス・ステンシルビューをクリア
			pDeviceContext->ClearDepthStencilView( pDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0 );


			///// 3 軸線の描画

			// 頂点バッファをバインド
			UINT strides[] = { sizeof( float ) * 7 };
			UINT offsets[] = { 0 };
			pDeviceContext->IASetVertexBuffers( 0, 1, &pTriaxialVertexBuffer, strides, offsets );

			// プリミティブタイプを設定
			pDeviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_LINELIST );

			// 頂点シェーダをバインド
			pDeviceContext->VSSetShader( pTriaxialVertexShader, NULL, 0 );

			// 入力レイアウトをバインド
			pDeviceContext->IASetInputLayout( pTriaxialInputLayout );

			// 描画
			pDeviceContext->Draw( 12, 0 );


			///// モデルの描画

			// 頂点バッファをバインド
			strides[0] = sizeof( float ) * 6;
			pDeviceContext->IASetVertexBuffers( 0, 1, &pPmdVertexBuffer, strides, offsets );

			// プリミティブタイプを設定
			pDeviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

			// 頂点シェーダをバインド
			pDeviceContext->VSSetShader( pPmdVertexShader, NULL, 0 );

			// 入力レイアウトをバインド
			pDeviceContext->IASetInputLayout( pPmdInputLayout );

			// 描画
			pDeviceContext->DrawIndexed( 3 * faces_count, 0, 0 );


			pSwapChain->Present( 1, 0 );

			// ちょっとだけ待つ
			Sleep( 5 );
		}
	}



	// シェーダをアンバインド
	pDeviceContext->VSSetShader( NULL, NULL, 0 );
	pDeviceContext->PSSetShader( NULL, NULL, 0 );

	// デバイス・リソース解放
	COM_SAFE_RELEASE( pDepthStencilState );
	COM_SAFE_RELEASE( pRasterizerState );
	COM_SAFE_RELEASE( pPmdInputLayout );
	COM_SAFE_RELEASE( pTriaxialInputLayout );
	COM_SAFE_RELEASE( pPixelShader );
	COM_SAFE_RELEASE( pPmdVertexShader );
	COM_SAFE_RELEASE( pTriaxialVertexShader );
	COM_SAFE_RELEASE( pVSConstantBuffer );
	COM_SAFE_RELEASE( pTriaxialVertexBuffer );
	COM_SAFE_RELEASE( pPmdIndexBuffer );
	COM_SAFE_RELEASE( pPmdVertexBuffer );
	COM_SAFE_RELEASE( pDepthStencilView );
	COM_SAFE_RELEASE( pDepthStencilBuffer );
	COM_SAFE_RELEASE( pRenderTargetView );
	COM_SAFE_RELEASE( pSwapChain );
	COM_SAFE_RELEASE( pDeviceContext );
	COM_SAFE_RELEASE( pDevice );

	// メモリ解放
	delete [] pmdVertices;
	delete [] pmdIndices;


	return msg.wParam;
}
