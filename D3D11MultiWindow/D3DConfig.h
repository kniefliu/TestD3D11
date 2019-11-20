#pragma once 

#define USE_POPUP 1
#define USE_GDI_CHILD_WINDOW 0

/* Need to fix the problem that D3D11 Window use flip, can't have child window */
#define USE_D3D11_FLIP 1
