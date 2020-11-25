#include <d3dx9.h>
//#include <mmsystem.h>
//#include <strsafe.h>
#include <windows.h>

struct CUSTOMVERTEX {
  FLOAT x, y, z;  // The untransformed, 3D position for the vertex
  DWORD color;    // The vertex color
};

// Our custom FVF, which describes our custom vertex structure
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ | D3DFVF_DIFFUSE)

struct context {
  LPDIRECT3D9 d3d9_pointer;
  LPDIRECT3DDEVICE9 device_pointer;
  LPDIRECT3DVERTEXBUFFER9 triangle_vertices_buffer;
  CUSTOMVERTEX triangle_vertices[3];
};

struct context* window_handle_to_context(HWND window_handle) {
  return (struct context*)GetWindowLongPtr(window_handle, 0);
}

HRESULT InitD3D(HWND window_handle) {
  struct context* the_context = window_handle_to_context(window_handle);
  // Create the D3D object.
  if (NULL == (the_context->d3d9_pointer = Direct3DCreate9(D3D_SDK_VERSION)))
    return E_FAIL;

  // Set up the structure used to create the D3DDevice
  D3DPRESENT_PARAMETERS d3dpp;
  ZeroMemory(&d3dpp, sizeof(d3dpp));
  d3dpp.Windowed = TRUE;
  d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
  d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;

  // Create the D3DDevice
  if (FAILED(the_context->d3d9_pointer->CreateDevice(
          D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, window_handle,
          D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp,
          &the_context->device_pointer))) {
    return E_FAIL;
  }

  // Turn off culling, so we see the front and back of the triangle
  the_context->device_pointer->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

  // Turn off D3D lighting, since we are providing our own vertex colors
  the_context->device_pointer->SetRenderState(D3DRS_LIGHTING, FALSE);

  return S_OK;
}

HRESULT InitGeometry(HWND window_handle) {
  struct context* the_context = window_handle_to_context(window_handle);

  // Initialize three vertices for rendering a triangle

  // Create the vertex buffer.
  if (FAILED(the_context->device_pointer->CreateVertexBuffer(
          3 * sizeof(CUSTOMVERTEX), 0, D3DFVF_CUSTOMVERTEX, D3DPOOL_DEFAULT,
          &the_context->triangle_vertices_buffer, NULL))) {
    return E_FAIL;
  }

  // Fill the vertex buffer.
  VOID* pVertices;
  if (FAILED(the_context->triangle_vertices_buffer->Lock(
          0, sizeof(the_context->triangle_vertices), (void**)&pVertices, 0)))
    return E_FAIL;
  memcpy(pVertices, the_context->triangle_vertices,
         sizeof(the_context->triangle_vertices));
  the_context->triangle_vertices_buffer->Unlock();

  return S_OK;
}

VOID Cleanup(HWND window_handle) {
  struct context* the_context = window_handle_to_context(window_handle);

  if (the_context->triangle_vertices_buffer != NULL)
    the_context->triangle_vertices_buffer->Release();

  if (the_context->device_pointer != NULL)
    the_context->device_pointer->Release();

  if (the_context->d3d9_pointer != NULL) the_context->d3d9_pointer->Release();
}

// Configure the world, view, projection matrices.
// The world rotates and objects are stationary as of now
VOID SetupMatrices(HWND window_handle) {
  struct context* the_context = window_handle_to_context(window_handle);
  // For our world matrix, we will just rotate the object about the y-axis.
  D3DXMATRIX matWorld;

  // Set up the rotation matrix to generate 1 full rotation (2*PI radians)
  // every 1000 ms. To avoid the loss of precision inherent in very high
  // floating point numbers, the system time is modulated by the rotation
  // period before conversion to a radian angle.
  UINT iTime = timeGetTime() % 1000;
  FLOAT fAngle = iTime * (2.0f * D3DX_PI) / 1000.0f;
  D3DXMatrixRotationY(&matWorld, fAngle);
  // set the world transformation to matWorld
  the_context->device_pointer->SetTransform(D3DTS_WORLD, &matWorld);

  // Set up our view matrix. A view matrix can be defined given an eye point,
  // a point to lookat, and a direction for which way is up. Here, we set the
  // eye five units back along the z-axis and up three units, look at the
  // origin, and define "up" to be in the y-direction.
  D3DXVECTOR3 vEyePt(0.0f, 3.0f, -5.0f);
  D3DXVECTOR3 vLookatPt(0.0f, 0.0f, 0.0f);
  D3DXVECTOR3 vUpVec(0.0f, 1.0f, 0.0f);
  D3DXMATRIX matView;
  D3DXMatrixLookAtLH(&matView, &vEyePt, &vLookatPt, &vUpVec);
  // set teh current view transformation for this device
  the_context->device_pointer->SetTransform(D3DTS_VIEW, &matView);

  // For the projection matrix, we set up a perspective transform (which
  // transforms geometry from 3D view space to 2D viewport space, with
  // a perspective divide making objects smaller in the distance). To build
  // a perpsective transform, we need the field of view (1/4 pi is common),
  // the aspect ratio, and the near and far clipping planes (which define at
  // what distances geometry should be no longer be rendered).
  D3DXMATRIX matProj;
  D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI / 4, 1.0f, 1.0f, 100.0f);
  the_context->device_pointer->SetTransform(D3DTS_PROJECTION, &matProj);
}

void update_a_color(HWND window_handle) {
  struct context* the_context = window_handle_to_context(window_handle);
  the_context->triangle_vertices[0].color += 100;

  VOID* pVertices;
  the_context->triangle_vertices_buffer->Lock(
      0, sizeof(the_context->triangle_vertices), (void**)&pVertices, 0);
  memcpy(pVertices, the_context->triangle_vertices,
         sizeof(the_context->triangle_vertices));
  the_context->triangle_vertices_buffer->Unlock();
}

VOID Render(HWND window_handle) {
  struct context* the_context = window_handle_to_context(window_handle);
  // Clear the backbuffer to a black color
  the_context->device_pointer->Clear(0, NULL, D3DCLEAR_TARGET,
                                     D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);

  update_a_color(window_handle);

  // Begin the scene
  if (SUCCEEDED(the_context->device_pointer->BeginScene())) {
    // Setup the world, view, and projection matrices
    SetupMatrices(window_handle);

    // Render the vertex buffer contents
    the_context->device_pointer->SetStreamSource(
        0, the_context->triangle_vertices_buffer, 0, sizeof(CUSTOMVERTEX));
    the_context->device_pointer->SetFVF(D3DFVF_CUSTOMVERTEX);
    the_context->device_pointer->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 1);

    // End the scene
    the_context->device_pointer->EndScene();
  }

  // Present the backbuffer contents to the display
  the_context->device_pointer->Present(NULL, NULL, NULL, NULL);
}

LRESULT WINAPI MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  switch (msg) {
    case WM_DESTROY:
      Cleanup(hWnd);
      PostQuitMessage(0);
      return 0;
  }

  return DefWindowProc(hWnd, msg, wParam, lParam);
}

#define WINDOW_CLASS_NAME "D3D Tutorial"
#define WINDOW_TITLE "D3D Tutorial 03: Matrices"
#define WINDOW_START_X_POSITION 400
#define WINDOW_START_Y_POSITION 200
#define WINDOW_WIDTH 256
#define WINDOW_HEIGHT 256
int main(int argc, char** argv) {
  struct context* the_context = NULL;

  // Register the window class
  int hwnd_bytes_extra = sizeof(struct context*);
  WNDCLASSEX wc = {
      sizeof(WNDCLASSEX),    CS_CLASSDC, MsgProc, 0L,   hwnd_bytes_extra,
      GetModuleHandle(NULL), NULL,       NULL,    NULL, NULL,
      WINDOW_CLASS_NAME,     NULL};
  RegisterClassEx(&wc);

  // Create the application's window
  HWND window_handle =
      CreateWindow(WINDOW_CLASS_NAME, WINDOW_TITLE, WS_OVERLAPPEDWINDOW,
                   WINDOW_START_X_POSITION, WINDOW_START_Y_POSITION,
                   WINDOW_WIDTH, WINDOW_HEIGHT, NULL, NULL, wc.hInstance, NULL);

  // Make room for our context and save it in the window instance
  the_context = (struct context*)malloc(sizeof(struct context));
  SetWindowLongPtr(window_handle, 0, (LONG_PTR)the_context);

  the_context->triangle_vertices[0] = {
      -1.0f,
      -1.0f,
      0.0f,
      0xffff0000,
  };
  the_context->triangle_vertices[1] = {
      1.0f,
      -1.0f,
      0.0f,
      0xff0000ff,
  };
  the_context->triangle_vertices[2] = {
      0.0f,
      1.0f,
      0.0f,
      0xffffffff,
  };

  // Initialize Direct3D
  if (SUCCEEDED(InitD3D(window_handle))) {
    // Create the scene geometry
    if (SUCCEEDED(InitGeometry(window_handle))) {
      // Show the window
      ShowWindow(window_handle, SW_SHOWDEFAULT);
      UpdateWindow(window_handle);

      // Enter the message loop
      MSG msg;
      ZeroMemory(&msg, sizeof(msg));
      while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
          TranslateMessage(&msg);
          DispatchMessage(&msg);
        } else
          Render(window_handle);
      }
    }
  }

  UnregisterClass(WINDOW_CLASS_NAME, wc.hInstance);
  free(the_context);
  return 0;
}
