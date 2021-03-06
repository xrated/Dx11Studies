﻿// C/C++ Common
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
#include "perspective.vs.h"   // 頂点シェーダ
#include "constant.ps.h"      // ピクセルシェーダ


#define COM_SAFE_RELEASE( p ) { if(p) { (p)->Release(); (p) = NULL; } }


TCHAR *g_className  = _T( "すみすみは俺の嫁" );
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



	// 立方体の頂点データ
	float cubeVertices[ 8 ][ 7 ] = {
	//    Xaxis  Yaxis  Zaxis  赤     緑     青     Alpha
		{ -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  0.0f,  1.0f },   // 手前左上
		{  0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f },   // 手前右上
		{  0.5f, -0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f },   // 手前右下
		{ -0.5f, -0.5f,  0.5f,  0.0f,  1.0f,  1.0f,  1.0f },   // 手前左下
		{ -0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f },   // 奥左上
		{  0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  1.0f,  1.0f },   // 奥右上
		{  0.5f, -0.5f, -0.5f,  1.0f,  1.0f,  0.0f,  1.0f },   // 奥右下
		{ -0.5f, -0.5f, -0.5f,  1.0f,  1.0f,  1.0f,  1.0f }    // 奥左下
	};

	// 立方体の頂点バッファを生成
	D3D11_BUFFER_DESC cubeVertexBufferDesc = {
		sizeof( cubeVertices ),		// ByteWidth
		D3D11_USAGE_DEFAULT,		// Usage
		D3D11_BIND_VERTEX_BUFFER,	// BindFlags
		0,							// CPUAccessFlags
		0,							// MiscFlags
		0							// StructureByteStride
	};
	D3D11_SUBRESOURCE_DATA cubeVertexResourceData = { cubeVertices };

	ID3D11Buffer * pCubeVertexBuffer = NULL;
	hr = pDevice->CreateBuffer( &cubeVertexBufferDesc, &cubeVertexResourceData, &pCubeVertexBuffer );
	if ( FAILED( hr ) )
	{
		MessageBox( NULL, _T( "失敗: ID3D11Device::CreateBuffer()" ), _T( "エラー" ), MB_OK | MB_ICONERROR );
		return 0;
	}
	dtprintf( _T( "ID3D11Device::CreateBuffer: ok (pCubeVertexBuffer: 0x%p)\n" ), pCubeVertexBuffer );


	// 立方体のインデックスデータ
	unsigned int cubeIndices[] = {
		0, 1, 2, 0, 2, 3,   // 手前
		4, 0, 3, 4, 3, 7,   // 左
		1, 5, 6, 1, 6, 2,   // 右
		0, 4, 5, 0, 5, 1,   // 上
		2, 6, 7, 2, 7, 3,   // 下
		5, 4, 7, 5, 7, 6    // 裏
	};


	// 立方体のインデックスバッファを生成
	D3D11_BUFFER_DESC cubeIndexBufferDesc = {
		sizeof( cubeIndices ),			// ByteWidth
		D3D11_USAGE_DEFAULT,			// Usage
		D3D11_BIND_INDEX_BUFFER,		// BindFlags
		0,								// CPUAccessFlags
		0,								// MiscFlags
		0								// StructureByteStride
	};
	D3D11_SUBRESOURCE_DATA cubeIndexResourceData = { cubeIndices };

	ID3D11Buffer * pCubeIndexBuffer = NULL;
	hr = pDevice->CreateBuffer( &cubeIndexBufferDesc, &cubeIndexResourceData, &pCubeIndexBuffer );
	if ( FAILED( hr ) )
	{
		MessageBox( NULL, _T( "失敗: ID3D11Device::CreateBuffer()" ), _T( "エラー" ), MB_OK | MB_ICONERROR );
		return 0;
	}
	dtprintf( _T( "ID3D11Device::CreateBuffer: ok (pCubeIndexBuffer: 0x%p)\n" ), pCubeIndexBuffer );

	// 立方体のインデックスバッファをバインド
	pDeviceContext->IASetIndexBuffer( pCubeIndexBuffer, DXGI_FORMAT_R32_UINT, 0 );
	dtprintf( _T( "ID3D11DeviceContext::IASetIndexBuffer: ok\n" ) );


	// 3 軸線の頂点データ
	float triaxialVertices[ 12 ][ 7 ] = {
	//    Xaxis  Yaxis  Zaxis  赤     緑     青     Alpha
		{  0.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f },   // x 軸正方向
		{  3.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f },
		{  0.0f,  0.0f,  0.0f,  0.2f,  0.0f,  0.0f,  1.0f },   // x 軸負方向
		{ -3.0f,  0.0f,  0.0f,  0.2f,  0.0f,  0.0f,  1.0f },
		{  0.0f,  0.0f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f },   // y 軸正方向
		{  0.0f,  3.0f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f },
		{  0.0f,  0.0f,  0.0f,  0.0f,  0.2f,  0.0f,  1.0f },   // y 軸負方向
		{  0.0f, -3.0f,  0.0f,  0.0f,  0.2f,  0.0f,  1.0f },
		{  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  1.0f,  1.0f },   // z 軸正方向
		{  0.0f,  0.0f,  3.0f,  0.0f,  0.0f,  1.0f,  1.0f },
		{  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.2f,  1.0f },   // z 軸負方向
		{  0.0f,  0.0f, -3.0f,  0.0f,  0.0f,  0.2f,  1.0f }
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
		sizeof( D3DXMATRIX ) * 3,		// ByteWidth
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


	// 頂点シェーダを作成
	ID3D11VertexShader * pVertexShader = NULL;
	hr = pDevice->CreateVertexShader( g_vs_perspective, sizeof( g_vs_perspective ), NULL, &pVertexShader );
	if ( FAILED( hr ) )
	{
		MessageBox( NULL, _T( "失敗: ID3D11Device::CreateVertexShader()" ), _T( "エラー" ), MB_OK | MB_ICONERROR );
		return 0;
	}
	dtprintf( _T( "ID3D11Device::CreateVertexShader: ok (pVertexShader: 0x%p)\n" ), pVertexShader );

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
	pDeviceContext->VSSetShader( pVertexShader, NULL, 0 );
	dtprintf( _T( "ID3D11DeviceContext::VSSetShader: ok\n" ) );
	pDeviceContext->PSSetShader( pPixelShader, NULL, 0 );
	dtprintf( _T( "ID3D11DeviceContext::PSSetShader: ok\n" ) );
	pDeviceContext->GSSetShader( NULL, NULL, 0 );
	pDeviceContext->HSSetShader( NULL, NULL, 0 );
	pDeviceContext->DSSetShader( NULL, NULL, 0 );


	// 入力エレメント記述子
	D3D11_INPUT_ELEMENT_DESC verticesDesc[] = {
		{ "IN_POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,               D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "IN_COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, sizeof(float)*3, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	// 入力レイアウトを生成
	ID3D11InputLayout * pInputLayout = NULL;
	hr = pDevice->CreateInputLayout( verticesDesc, 2, g_vs_perspective, sizeof( g_vs_perspective ), &pInputLayout );
	if ( FAILED( hr ) )
	{
		MessageBox( NULL, _T( "失敗: ID3D11Device::CreateInputLayout()" ), _T( "エラー" ), MB_OK | MB_ICONERROR );
		return 0;
	}
	dtprintf( _T( "ID3D11Device::CreateInputLayout: ok (pInputLayout: 0x%p)\n" ), pInputLayout );

	// 入力レイアウトをバインド
	pDeviceContext->IASetInputLayout( pInputLayout );
	dtprintf( _T( "ID3D11DeviceContext::IASetInputLayout: ok\n" ) );


	// ラスタライザステートを生成
	D3D11_RASTERIZER_DESC rasterizerStateDesc = {
		D3D11_FILL_SOLID,		// FillMode
//		D3D11_FILL_WIREFRAME,	// FillMode (ワイヤーフレーム表示)
//		D3D11_CULL_BACK,		// CullMode
		D3D11_CULL_NONE,		// CullMode (カリングなし)
		FALSE,					// FrontCounterClockwise
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

			const D3DXVECTOR3 eye( 1.8f * 1.414214f * -cosf( theta ), 1.8f, 1.8f * 1.414214f * sinf( theta ) );
			const D3DXVECTOR3 at( 0.0f, 0.0f, 0.0f );
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

			// 描画
			pDeviceContext->Draw( 12, 0 );


			///// 立方体の描画

			// 頂点バッファをバインド
			pDeviceContext->IASetVertexBuffers( 0, 1, &pCubeVertexBuffer, strides, offsets );

			// プリミティブタイプを設定
			pDeviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

			// 描画
			pDeviceContext->DrawIndexed( 36, 0, 0 );


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
	COM_SAFE_RELEASE( pInputLayout );
	COM_SAFE_RELEASE( pPixelShader );
	COM_SAFE_RELEASE( pVertexShader );
	COM_SAFE_RELEASE( pVSConstantBuffer );
	COM_SAFE_RELEASE( pTriaxialVertexBuffer );
	COM_SAFE_RELEASE( pCubeIndexBuffer );
	COM_SAFE_RELEASE( pCubeVertexBuffer );
	COM_SAFE_RELEASE( pDepthStencilView );
	COM_SAFE_RELEASE( pDepthStencilBuffer );
	COM_SAFE_RELEASE( pRenderTargetView );
	COM_SAFE_RELEASE( pSwapChain );
	COM_SAFE_RELEASE( pDeviceContext );
	COM_SAFE_RELEASE( pDevice );


	return msg.wParam;
}
