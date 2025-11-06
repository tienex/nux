/*++
    Module Name:

        simple_triangle.c

    Abstract:

        Simple Direct3D 9 example that renders a colored triangle.
        Demonstrates basic D3D9 usage with the ananke implementation.

    Environment:

        C99 compatible.
--*/

#include <ananke/types.h>
#include <ananke/d3d9.h>
#include <stdio.h>

/* --------------------------------------------------------------- */
/*  Vertex structure                                               */
/* --------------------------------------------------------------- */

typedef struct _VERTEX {
    FLOAT x, y, z;      /* Position */
    UINT32 color;       /* Color (D3DCOLOR) */
} VERTEX;

/* --------------------------------------------------------------- */
/*  Main program                                                   */
/* --------------------------------------------------------------- */

int main(int argc, char **argv)
{
    IDirect3D9 *d3d = NULL;
    IDirect3DDevice9 *device = NULL;
    IDirect3DVertexBuffer9 *vertexBuffer = NULL;
    IDirect3DVertexDeclaration9 *vertexDecl = NULL;
    D3DPRESENT_PARAMETERS d3dpp;
    D3DVIEWPORT9 viewport;
    HRESULT hr;
    VERTEX *vertices;
    int i;

    printf("Direct3D 9 Simple Triangle Example\n");
    printf("===================================\n\n");

    /* Create D3D9 */
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (!d3d) {
        printf("ERROR: Failed to create Direct3D 9 interface\n");
        return 1;
    }
    printf("Direct3D 9 interface created successfully\n");

    /* Setup presentation parameters */
    RtlZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.BackBufferWidth = 800;
    d3dpp.BackBufferHeight = 600;
    d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
    d3dpp.BackBufferCount = 1;
    d3dpp.SwapEffect = 1;  /* D3DSWAPEFFECT_DISCARD */
    d3dpp.Windowed = 1;
    d3dpp.EnableAutoDepthStencil = 1;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;

    /* Create device */
    hr = IDirect3D9_CreateDevice(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, NULL,
                                 D3DCREATE_HARDWARE_VERTEXPROCESSING,
                                 &d3dpp, &device);
    if (FAILED(hr)) {
        printf("ERROR: Failed to create D3D9 device (hr=0x%08X)\n", hr);
        IUnknown_Release((IUnknown*)d3d);
        return 1;
    }
    printf("D3D9 device created successfully\n");

    /* Setup viewport */
    viewport.X = 0;
    viewport.Y = 0;
    viewport.Width = 800;
    viewport.Height = 600;
    viewport.MinZ = 0.0f;
    viewport.MaxZ = 1.0f;
    IDirect3DDevice9_SetViewport(device, &viewport);
    printf("Viewport set to %dx%d\n", viewport.Width, viewport.Height);

    /* Create vertex buffer */
    hr = IDirect3DDevice9_CreateVertexBuffer(device, 3 * sizeof(VERTEX),
                                            D3DUSAGE_WRITEONLY, 0,
                                            D3DPOOL_MANAGED, &vertexBuffer, NULL);
    if (FAILED(hr)) {
        printf("ERROR: Failed to create vertex buffer (hr=0x%08X)\n", hr);
        goto cleanup;
    }
    printf("Vertex buffer created\n");

    /* Fill vertex buffer */
    hr = IDirect3DVertexBuffer9_Lock(vertexBuffer, 0, 0, (void**)&vertices, 0);
    if (SUCCEEDED(hr)) {
        /* Triangle vertices (counter-clockwise) */
        vertices[0].x = 0.0f;
        vertices[0].y = 0.5f;
        vertices[0].z = 0.5f;
        vertices[0].color = D3DCOLOR_ARGB(255, 255, 0, 0); /* Red */

        vertices[1].x = 0.5f;
        vertices[1].y = -0.5f;
        vertices[1].z = 0.5f;
        vertices[1].color = D3DCOLOR_ARGB(255, 0, 255, 0); /* Green */

        vertices[2].x = -0.5f;
        vertices[2].y = -0.5f;
        vertices[2].z = 0.5f;
        vertices[2].color = D3DCOLOR_ARGB(255, 0, 0, 255); /* Blue */

        IDirect3DVertexBuffer9_Unlock(vertexBuffer);
        printf("Vertex buffer filled with triangle data\n");
    }

    /* Create vertex declaration */
    D3DVERTEXELEMENT9 vertexElements[] = {
        {0, 0,  D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_UBYTE4, 0, D3DDECLUSAGE_COLOR, 0},
        D3DDECL_END()
    };

    hr = IDirect3DDevice9_CreateVertexDeclaration(device, vertexElements, &vertexDecl);
    if (SUCCEEDED(hr)) {
        printf("Vertex declaration created\n");
    }

    /* Setup render states */
    IDirect3DDevice9_SetRenderState(device, D3DRS_CULLMODE, D3DCULL_NONE);
    IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING, 0);  /* Disable lighting */
    IDirect3DDevice9_SetRenderState(device, D3DRS_ZENABLE, 1);   /* Enable depth test */
    printf("Render states configured\n");

    /* Set stream source and vertex declaration */
    IDirect3DDevice9_SetStreamSource(device, 0, vertexBuffer, 0, sizeof(VERTEX));
    IDirect3DDevice9_SetVertexDeclaration(device, vertexDecl);

    /* Render frames (simplified - normally this would be a loop) */
    printf("\nRendering triangle...\n");
    for (i = 0; i < 1; i++) {
        /* Begin scene */
        hr = IDirect3DDevice9_BeginScene(device);
        if (SUCCEEDED(hr)) {
            /* Clear backbuffer */
            IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
                                 D3DCOLOR_ARGB(255, 64, 64, 128), 1.0f, 0);

            /* Draw triangle */
            IDirect3DDevice9_DrawPrimitive(device, D3DPT_TRIANGLELIST, 0, 1);

            /* End scene */
            IDirect3DDevice9_EndScene(device);

            /* Present */
            IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
        }
    }

    printf("Rendering complete\n");

cleanup:
    /* Cleanup */
    if (vertexDecl) {
        IUnknown_Release((IUnknown*)vertexDecl);
        printf("Vertex declaration released\n");
    }
    if (vertexBuffer) {
        IUnknown_Release((IUnknown*)vertexBuffer);
        printf("Vertex buffer released\n");
    }
    if (device) {
        IUnknown_Release((IUnknown*)device);
        printf("Device released\n");
    }
    if (d3d) {
        IUnknown_Release((IUnknown*)d3d);
        printf("D3D9 interface released\n");
    }

    printf("\nExample completed successfully!\n");
    return 0;
}
