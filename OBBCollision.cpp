#include <Windows.h>
#include <mmsystem.h>
#include <d3dx9.h>
#pragma warning( disable : 4996 ) // disable deprecated warning 
#include <strsafe.h>
#pragma warning( default : 4996 )
#include <iostream>
using namespace std;
//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------
LPDIRECT3D9             g_pD3D = NULL; // Used to create the D3DDevice
LPDIRECT3DDEVICE9       g_pd3dDevice = NULL; // Our rendering device
class Objecttype
{
public:
    D3DXVECTOR3 position = { 0, 0, 0 };
    float rotationY = 0.f;
    D3DXVECTOR3 minPos;
    D3DXVECTOR3 maxPos;
    D3DXVECTOR3 centerPos;
    D3DXVECTOR3 axis[3];
    float axisLen[3];
};

struct MYINDEX
{
    WORD _0, _1, _2; // 인덱스는 WORD 형으로 나열하기 때문에 WORD 선언 
};

class OBBobject
{
public:
    Objecttype mObjecttype;
    D3DXVECTOR3 m_vertices[8];

    LPDIRECT3DVERTEXBUFFER9 mVB = nullptr; // Vertex Buffer
    LPDIRECT3DINDEXBUFFER9	mIB = nullptr; // Index Buffer

    void SetBoundingBox();

    HRESULT InitVB();
    HRESULT InitIB();

    void InitObject() { InitVB(); InitIB(); }
    void RenderBox();
    void UpdateBox(Objecttype targetBox);
    void Release();

    bool	obbCollide = false;

    void	CheckOBB(Objecttype targetBox);
};

OBBobject* Obb1;
OBBobject* Obb2;

void OBBobject::SetBoundingBox()
{
    mObjecttype.minPos[0] = m_vertices[0][0];
    mObjecttype.minPos[1] = m_vertices[0][1];
    mObjecttype.minPos[2] = m_vertices[0][2];
    mObjecttype.maxPos[0] = m_vertices[0][0];
    mObjecttype.maxPos[1] = m_vertices[0][1];
    mObjecttype.maxPos[2] = m_vertices[0][2];

    for (int i = 0; i < 8; ++i)
    {
        for (int j = 0; j < 3; ++j)
        {
            mObjecttype.minPos[j] = __min(mObjecttype.minPos[j], m_vertices[i][j]);
            mObjecttype.maxPos[j] = __max(mObjecttype.maxPos[j], m_vertices[i][j]);
        }
    }

}

HRESULT OBBobject::InitVB()
{
    // 박스의 8개 vertex를 만든다
    m_vertices[0] = { -1.f, +1.f, +1.f };	// v0
    m_vertices[1] = { +1.f, +1.f, +1.f };	// v1
    m_vertices[2] = { +1.f, +1.f, -1.f };	// v2
    m_vertices[3] = { -1.f, +1.f, -1.f };	// v3
    m_vertices[4] = { -1.f, -1.f, +1.f };	// v4
    m_vertices[5] = { +1.f, -1.f, +1.f };	// v5
    m_vertices[6] = { +1.f, -1.f, -1.f };	// v6
    m_vertices[7] = { -1.f, -1.f, -1.f };	// v7

    if (FAILED(g_pd3dDevice->CreateVertexBuffer(8 * sizeof(D3DXVECTOR3),
        0, D3DFVF_XYZ,
        D3DPOOL_DEFAULT, &mVB, NULL)))
    {
        return E_FAIL;
    }

    VOID* pVertices;
    if (FAILED(mVB->Lock(0, sizeof(m_vertices), (void**)&pVertices, 0)))
        return E_FAIL;
    memcpy(pVertices, m_vertices, sizeof(m_vertices));
    mVB->Unlock();

    SetBoundingBox();

    mObjecttype.centerPos = { 0.0f, 0.0f, 0.0f };

    mObjecttype.axisLen[0] = (mObjecttype.maxPos[0] - mObjecttype.minPos[0]) / 2; // x
    mObjecttype.axisLen[1] = (mObjecttype.maxPos[1] - mObjecttype.minPos[1]) / 2; // y
    mObjecttype.axisLen[2] = (mObjecttype.maxPos[2] - mObjecttype.minPos[2]) / 2; // z

    mObjecttype.axis[0] = { 1.f, 0.f, 0.f };
    mObjecttype.axis[1] = { 0.f, 1.f, 0.f };
    mObjecttype.axis[2] = { 0.f, 0.f, 1.f };

    return S_OK;
}

HRESULT OBBobject::InitIB()
{
    // 박스의 12개 면을 만든다
    MYINDEX	indices[] =
    {
        { 0, 1, 2 }, { 0, 2, 3 },	/// 윗면
        { 4, 6, 5 }, { 4, 7, 6 },	/// 아랫면
        { 0, 3, 7 }, { 0, 7, 4 },	/// 왼면
        { 1, 5, 6 }, { 1, 6, 2 },	/// 오른면
        { 3, 2, 6 }, { 3, 6, 7 },	/// 앞면
        { 0, 4, 5 }, { 0, 5, 1 }	/// 뒷면
    };

    /// 인덱스버퍼 생성
    /// WORD로 인덱스 버퍼를 선언했으므로 D3DFMT_INDEX16 플래그를 쓴다.
    if (FAILED(g_pd3dDevice->CreateIndexBuffer(12 * sizeof(MYINDEX), 0, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &mIB, NULL)))
    {
        return E_FAIL;
    }

    /// 인덱스버퍼를 값으로 채운다.
    /// 인덱스버퍼의 Lock()함수를 호출하여 포인터를 얻어온다.
    VOID* pIndices;
    if (FAILED(mIB->Lock(0, sizeof(indices), (void**)&pIndices, 0)))
        return E_FAIL;
    memcpy(pIndices, indices, sizeof(indices));
    mIB->Unlock();

    return S_OK;
}

void OBBobject::RenderBox()
{
    if (!g_pd3dDevice || !mVB || !mIB)
    {
        return;
    }

    D3DXMATRIXA16 matWorld, matRot, matMov;
    D3DXMatrixIdentity(&matWorld);
    D3DXMatrixIdentity(&matRot);
    D3DXMatrixIdentity(&matMov);

    D3DXMatrixRotationY(&matRot, mObjecttype.rotationY);
    D3DXMatrixTranslation(&matMov, mObjecttype.position[0], mObjecttype.position[1], mObjecttype.position[2]);
    D3DXMatrixMultiply(&matWorld, &matRot, &matMov);

    g_pd3dDevice->SetTransform(D3DTS_WORLD, &matWorld);

   
    SetBoundingBox();
    /// 정점버퍼의 삼각형을 그린다.
    /// 1. 정점정보가 담겨있는 정점버퍼를 출력 스트림으로 할당한다.
    g_pd3dDevice->SetStreamSource(0, mVB, 0, sizeof(D3DXVECTOR3));
    /// 2. D3D에게 정점쉐이더 정보를 지정한다. 대부분의 경우에는 FVF만 지정한다.
    g_pd3dDevice->SetFVF(D3DFVF_XYZ);
    /// 3. 인덱스버퍼를 지정한다.
    g_pd3dDevice->SetIndices(mIB);
    g_pd3dDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
    /// 4. DrawIndexedPrimitive()를 호출한다.
    g_pd3dDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 8, 0, 12);
}

void OBBobject::UpdateBox(Objecttype targetBox)
{
    CheckOBB(targetBox);
}

void OBBobject::Release()
{
    // 메모리 해제
}

void OBBobject::CheckOBB(Objecttype targetBox)
{
    double c[3][3];
    double absC[3][3];
    double d[3];
    double r0, r1, r;
    int i;
    const double cutoff = 0.999999;
    bool existsParallelPair = false;
    D3DXVECTOR3 diff = mObjecttype.centerPos - targetBox.centerPos;

    for (i = 0; i < 3; ++i)
    {
        c[0][i] = D3DXVec3Dot(&mObjecttype.axis[0], &targetBox.axis[i]);
        absC[0][i] = fabsf(c[0][i]);
        if (absC[0][i] > cutoff)
            existsParallelPair = true;
    }

    d[0] = D3DXVec3Dot(&diff, &mObjecttype.axis[0]);
    r = fabsf(d[0]);
    r0 = mObjecttype.axisLen[0];
    r1 = targetBox.axisLen[0] * absC[0][0] + targetBox.axisLen[1] * absC[0][1] + targetBox.axisLen[2] * absC[0][2];
    if (r > r0 + r1)
    {
        obbCollide = false;
        return;
    }
    for (i = 0; i < 3; ++i)
    {
        c[1][i] = D3DXVec3Dot(&mObjecttype.axis[1], &targetBox.axis[i]);
        absC[1][i] = fabsf(c[1][i]);
        if (absC[1][i] > cutoff)
            existsParallelPair = true;
    }
    d[1] = D3DXVec3Dot(&diff, &mObjecttype.axis[1]);
    r = fabsf(d[1]);
    r0 = mObjecttype.axisLen[1];
    r1 = targetBox.axisLen[0] * absC[1][0] + targetBox.axisLen[1] * absC[1][1] + targetBox.axisLen[2] * absC[1][2];
    if (r > r0 + r1)
    {
        obbCollide = false;
        return;
    }
    for (i = 0; i < 3; ++i)
    {
        c[2][i] = D3DXVec3Dot(&mObjecttype.axis[2], &targetBox.axis[i]);
        absC[2][i] = fabsf(c[2][i]);
        if (absC[2][i] > cutoff)
            existsParallelPair = true;
    }
    d[2] = D3DXVec3Dot(&diff, &mObjecttype.axis[2]);
    r = fabsf(d[2]);
    r0 = mObjecttype.axisLen[2];
    r1 = targetBox.axisLen[0] * absC[2][0] + targetBox.axisLen[1] * absC[2][1] + targetBox.axisLen[2] * absC[2][2];
    if (r > r0 + r1)
    {
        obbCollide = false;
        return;
    }
    r = fabsf(D3DXVec3Dot(&diff, &targetBox.axis[0]));
    r0 = mObjecttype.axisLen[0] * absC[0][0] + mObjecttype.axisLen[1] * absC[1][0] + mObjecttype.axisLen[2] * absC[2][0];
    r1 = targetBox.axisLen[0];
    if (r > r0 + r1)
    {
        obbCollide = false;
        return;
    }

    r = fabsf(D3DXVec3Dot(&diff, &targetBox.axis[1]));
    r0 = mObjecttype.axisLen[0] * absC[0][1] + mObjecttype.axisLen[1] * absC[1][1] + mObjecttype.axisLen[2] * absC[2][1];
    r1 = targetBox.axisLen[1];
    if (r > r0 + r1)
    {
        obbCollide = false;
        return;
    }

    r = fabsf(D3DXVec3Dot(&diff, &targetBox.axis[2]));
    r0 = mObjecttype.axisLen[0] * absC[0][2] + mObjecttype.axisLen[1] * absC[1][2] + mObjecttype.axisLen[2] * absC[2][2];
    r1 = targetBox.axisLen[2];
    if (r > r0 + r1)
    {
        obbCollide = false;
        return;
    }

    if (existsParallelPair == true)
    {
        obbCollide = true;
        return;
    }

    r = fabsf(d[2] * c[1][0] - d[1] * c[2][0]);
    r0 = mObjecttype.axisLen[1] * absC[2][0] + mObjecttype.axisLen[2] * absC[1][0];
    r1 = targetBox.axisLen[1] * absC[0][2] + targetBox.axisLen[2] * absC[0][1];
    if (r > r0 + r1)
    {
        obbCollide = false;
        return;
    }

    r = fabsf(d[2] * c[1][1] - d[1] * c[2][1]);
    r0 = mObjecttype.axisLen[1] * absC[2][1] + mObjecttype.axisLen[2] * absC[1][1];
    r1 = targetBox.axisLen[0] * absC[0][2] + targetBox.axisLen[2] * absC[0][0];
    if (r > r0 + r1)
    {
        obbCollide = false;
        return;
    }

    r = fabsf(d[2] * c[1][2] - d[1] * c[2][2]);
    r0 = mObjecttype.axisLen[1] * absC[2][2] + mObjecttype.axisLen[2] * absC[1][2];
    r1 = targetBox.axisLen[0] * absC[0][1] + targetBox.axisLen[1] * absC[0][0];
    if (r > r0 + r1)
    {
        obbCollide = false;
        return;
    }

    r = fabsf(d[0] * c[2][0] - d[2] * c[0][0]);
    r0 = mObjecttype.axisLen[0] * absC[2][0] + mObjecttype.axisLen[2] * absC[0][0];
    r1 = targetBox.axisLen[1] * absC[1][2] + targetBox.axisLen[2] * absC[1][1];
    if (r > r0 + r1)
    {
        obbCollide = false;
        return;
    }

    r = fabsf(d[0] * c[2][1] - d[2] * c[0][1]);
    r0 = mObjecttype.axisLen[0] * absC[2][1] + mObjecttype.axisLen[2] * absC[0][1];
    r1 = targetBox.axisLen[0] * absC[1][2] + targetBox.axisLen[2] * absC[1][0];
    if (r > r0 + r1)
    {
        obbCollide = false;
        return;
    }

    r = fabsf(d[0] * c[2][2] - d[2] * c[0][2]);
    r0 = mObjecttype.axisLen[0] * absC[2][2] + mObjecttype.axisLen[2] * absC[0][2];
    r1 = targetBox.axisLen[0] * absC[1][1] + targetBox.axisLen[1] * absC[1][0];
    if (r > r0 + r1)
    {
        obbCollide = false;
        return;
    }

    r = fabsf(d[1] * c[0][0] - d[0] * c[1][0]);
    r0 = mObjecttype.axisLen[0] * absC[1][0] + mObjecttype.axisLen[1] * absC[0][0];
    r1 = targetBox.axisLen[1] * absC[2][2] + targetBox.axisLen[2] * absC[2][1];
    if (r > r0 + r1)
    {
        obbCollide = false;
        return;
    }

    r = fabsf(d[1] * c[0][1] - d[0] * c[1][1]);
    r0 = mObjecttype.axisLen[0] * absC[1][1] + mObjecttype.axisLen[1] * absC[0][1];
    r1 = targetBox.axisLen[0] * absC[2][2] + targetBox.axisLen[2] * absC[2][0];
    if (r > r0 + r1)
    {
        obbCollide = false;
        return;
    }

    r = fabsf(d[1] * c[0][2] - d[0] * c[1][2]);
    r0 = mObjecttype.axisLen[0] * absC[1][2] + mObjecttype.axisLen[1] * absC[0][2];
    r1 = targetBox.axisLen[0] * absC[2][1] + targetBox.axisLen[1] * absC[2][0];
    if (r > r0 + r1)
    {
        obbCollide = false;
        return;
    }

    obbCollide = true;
    return;
}

void MoveBox(float x, float y, float z)
{
    Obb1->mObjecttype.position[0] += x;
    Obb1->mObjecttype.position[1] += y;
    Obb1->mObjecttype.position[2] += z;

    Obb1->mObjecttype.centerPos[0] += x;
    Obb1->mObjecttype.centerPos[1] += y;
    Obb1->mObjecttype.centerPos[2] += z;

    D3DXMATRIXA16 matMov;
    D3DXMatrixIdentity(&matMov);
    D3DXMatrixTranslation(&matMov, x, y, z);

    for (int i = 0; i < 8; ++i)
    {
        D3DXVec3TransformCoord(&Obb1->m_vertices[i], &Obb1->m_vertices[i], &matMov);
    }
}

void RotateBox()
{
    Obb1->mObjecttype.rotationY += 0.1;

    D3DXMATRIXA16 matRot, matMov;
    D3DXMatrixIdentity(&matRot);
    D3DXMatrixIdentity(&matMov);
    //D3DXMatrixTranslation(&matMov, -Obb1->mObjecttype.position[0], -Obb1->mObjecttype.position[1], -Obb1->mObjecttype.position[2]);
    D3DXMatrixRotationY(&matRot, 0.1); // 0.1
    //D3DXMatrixMultiply(&matRot, &matMov, &matRot); // matrot == - position * 0.1
    D3DXMatrixTranslation(&matMov, Obb1->mObjecttype.position[0], Obb1->mObjecttype.position[1], Obb1->mObjecttype.position[2]);
    D3DXMatrixMultiply(&matRot, &matRot, &matMov); // matrot == -positon * +position * 0.1

    for (int i = 0; i < 8; ++i)
    {
        D3DXVec3TransformCoord(&Obb1->m_vertices[i], &Obb1->m_vertices[i], &matRot);
    }

    for (int i = 0; i < 3; ++i)
    {
        D3DXVec3TransformNormal(&Obb1->mObjecttype.axis[i], &Obb1->mObjecttype.axis[i], &matRot);
    }
}
//-----------------------------------------------------------------------------
// Name: InitD3D()
// Desc: Initializes Direct3D
//-----------------------------------------------------------------------------
HRESULT InitD3D( HWND hWnd )
{
    // Create the D3D object.
    if( NULL == ( g_pD3D = Direct3DCreate9( D3D_SDK_VERSION ) ) )
        return E_FAIL;

    // Set up the structure used to create the D3DDevice. Since we are now
    // using more complex geometry, we will create a device with a zbuffer.
    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory( &d3dpp, sizeof( d3dpp ) );
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D16;

    // Create the D3DDevice
    if( FAILED( g_pD3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
                                      D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                                      &d3dpp, &g_pd3dDevice ) ) )
    {
        return E_FAIL;
    }

    // Turn off culling
    g_pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );

    // Turn on the zbuffer
    g_pd3dDevice->SetRenderState( D3DRS_ZENABLE, TRUE );

    return S_OK;
}

//-----------------------------------------------------------------------------
// Name: InitGeometry()
// Desc: Creates the scene geometry
//-----------------------------------------------------------------------------
HRESULT InitGeometry()
{
     Obb1 = new OBBobject();
     Obb2 = new OBBobject();
     Obb1->InitObject();
     Obb2->InitObject();
    return S_OK;
}

//-----------------------------------------------------------------------------
// Name: Cleanup()
// Desc: Releases all previously initialized objects
//-----------------------------------------------------------------------------
VOID Cleanup()
{
    if( g_pd3dDevice != NULL )
        g_pd3dDevice->Release();

    if( g_pD3D != NULL )
        g_pD3D->Release();
}

//-----------------------------------------------------------------------------
// Name: SetupMatrices()
// Desc: Sets up the world, view, and projection transform matrices.
//-----------------------------------------------------------------------------
float Ycampos = 0;
float Zcampos = -10;
VOID SetupMatrices()
{
    D3DXVECTOR3 vEyePt( 0.0f, Ycampos, Zcampos);
    D3DXVECTOR3 vLookatPt( 0.0f, 0.0f, 0.0f );
    D3DXVECTOR3 vUpVec( 0.0f, 1.0f, 0.0f );
    D3DXMATRIXA16 matView;
    D3DXMatrixLookAtLH( &matView, &vEyePt, &vLookatPt, &vUpVec );
    g_pd3dDevice->SetTransform( D3DTS_VIEW, &matView );
    D3DXMATRIXA16 matProj;
    D3DXMatrixPerspectiveFovLH( &matProj, D3DX_PI / 4, 1.0f, 1.0f, 100.0f );
    g_pd3dDevice->SetTransform( D3DTS_PROJECTION, &matProj );
}

//-----------------------------------------------------------------------------
// Name: SetupOBBCollision()
// Desc: Sets up the OBBCollision and materials for the scene.
//-----------------------------------------------------------------------------
VOID SetupOBBCollision()
{
    // Set up a material. The material here just has the diffuse and ambient
    // colors set to yellow. Note that only one material can be used at a time.
    D3DMATERIAL9 mtrl;
    ZeroMemory( &mtrl, sizeof( D3DMATERIAL9 ) );
    mtrl.Diffuse.r = mtrl.Ambient.r = 1.0f;
    mtrl.Diffuse.g = mtrl.Ambient.g = 1.0f;
    mtrl.Diffuse.b = mtrl.Ambient.b = 0.0f;
    mtrl.Diffuse.a = mtrl.Ambient.a = 1.0f;
    g_pd3dDevice->SetMaterial( &mtrl );

    // Set up a white, directional light, with an oscillating direction.
    // Note that many OBBCollision may be active at a time (but each one slows down
    // the rendering of our scene). However, here we are just using one. Also,
    // we need to set the D3DRS_LIGHTING renderstate to enable lighting
    D3DXVECTOR3 vecDir;
    D3DLIGHT9 light;
    ZeroMemory( &light, sizeof( D3DLIGHT9 ) );
    light.Type = D3DLIGHT_DIRECTIONAL;
    light.Diffuse.r = 1.0f;
    light.Diffuse.g = 1.0f;
    light.Diffuse.b = 1.0f;
    vecDir = D3DXVECTOR3( cosf( timeGetTime() / 350.0f ),
                          1.0f,
                          sinf( timeGetTime() / 350.0f ) );
    D3DXVec3Normalize( ( D3DXVECTOR3* )&light.Direction, &vecDir );
    light.Range = 1000.0f;
    g_pd3dDevice->SetLight( 0, &light );
    g_pd3dDevice->LightEnable( 0, TRUE );
    g_pd3dDevice->SetRenderState( D3DRS_LIGHTING, TRUE );

    // Finally, turn on some ambient light.
    g_pd3dDevice->SetRenderState( D3DRS_AMBIENT, 0x00202020 );
}
float xpos = 0;
float zpos = 0;
VOID Update()
{
   if (GetAsyncKeyState(VK_UP))
       Ycampos += 0.1f;
   if (GetAsyncKeyState(VK_DOWN))
       Ycampos -= 0.1f;
   if (GetAsyncKeyState(VK_LEFT))
       Zcampos += 0.1f;
   if (GetAsyncKeyState(VK_RIGHT))
       Zcampos -= 0.1f;
  
    if(GetAsyncKeyState('W'))
        MoveBox(0, 0, 0.1f);
    if(GetAsyncKeyState('S'))
        MoveBox(0, 0, -0.1f);
    if (GetAsyncKeyState('A'))
        MoveBox(-0.1f, 0, 0);
    if (GetAsyncKeyState('D'))
        MoveBox(0.1f, 0, 0);

    if(GetAsyncKeyState('E'))
        RotateBox();
    if (Obb1)
    {
        Obb1->UpdateBox(Obb2->mObjecttype);
    }
    if (Obb2)
    {
        Obb2->UpdateBox(Obb1->mObjecttype);
    }

    if (Obb1->obbCollide || Obb2->obbCollide)
    {
        cout << "OBBCOLLISION" << endl;
    }
    else
        cout << "NO" << endl;
}

//-----------------------------------------------------------------------------
// Name: Render()
// Desc: Draws the scene
//-----------------------------------------------------------------------------
VOID Render()
{
    // Clear the backbuffer and the zbuffer
    g_pd3dDevice->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
                         D3DCOLOR_XRGB( 0, 0, 255 ), 1.0f, 0 );

    // Begin the scene
    if( SUCCEEDED( g_pd3dDevice->BeginScene() ) )
    {
        // Setup the OBBCollision and materials
        SetupOBBCollision();
        // Setup the world, view, and projection matrices
        SetupMatrices();
        if (Obb1)
        {
            Obb1->RenderBox();
        }
        if (Obb2)
        {
            Obb2->RenderBox();
        }
        g_pd3dDevice->EndScene();
    }

    // Present the backbuffer contents to the display
    g_pd3dDevice->Present( NULL, NULL, NULL, NULL );
}

//-----------------------------------------------------------------------------
// Name: MsgProc()
// Desc: The window's message handler
//-----------------------------------------------------------------------------
LRESULT WINAPI MsgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch( msg )
    {
        case WM_DESTROY:
            Cleanup();
            PostQuitMessage( 0 );
            return 0;
    }

    return DefWindowProc( hWnd, msg, wParam, lParam );
}

//-----------------------------------------------------------------------------
// Name: WinMain()
// Desc: The application's entry point
//-----------------------------------------------------------------------------
//INT WINAPI wWinMain( HINSTANCE hInst, HINSTANCE, LPWSTR, INT )
int main(void)
{
    //UNREFERENCED_PARAMETER( hInst );

    // Register the window class
    WNDCLASSEX wc =
    {
        sizeof( WNDCLASSEX ), CS_CLASSDC, MsgProc, 0L, 0L,
        GetModuleHandle( NULL ), NULL, NULL, NULL, NULL,
        L"OBBCollision", NULL
    };
    RegisterClassEx( &wc );

    // Create the application's window
    HWND hWnd = CreateWindow( L"OBBCollision", L"OBBCollision",
                              WS_OVERLAPPEDWINDOW, 100, 100, 800, 800,
                              NULL, NULL, wc.hInstance, NULL );

    // Initialize Direct3D
    if( SUCCEEDED( InitD3D( hWnd ) ) )
    {
        // Create the geometry
        if( SUCCEEDED( InitGeometry() ) )
        {
            // Show the window
            ShowWindow( hWnd, SW_SHOWDEFAULT );
            UpdateWindow( hWnd );

            // Enter the message loop
            MSG msg;
            ZeroMemory( &msg, sizeof( msg ) );
            while( msg.message != WM_QUIT )
            {
                if( PeekMessage( &msg, NULL, 0U, 0U, PM_REMOVE ) )
                {
                    TranslateMessage( &msg );
                    DispatchMessage( &msg );
                }
                else
                {
                    Update();
                    Render();
                }
            }
        }
    }

    UnregisterClass( L"OBBCollision", wc.hInstance );
    return 0;
}
