/**
 * @file mjk_platform_win32.c
 * @brief Implementación nativa de plataforma para Windows (Win32, XInput, WaveOut).
 * 
 * PROYECTO: Mighty Bomb Jack - Source Port Educacional en C
 * FILOSOFÍA: Cero dependencias externas. Soporte nativo para mandos Xbox Series y teclado.
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <xinput.h>
#include <mmsystem.h>
#include "mjk_platform.h"
#include <stdio.h>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "xinput.lib")

#define WINDOW_SCALE 3 // Ventana a 768x720 (escala entera 3x de 256x240)

static HWND       g_hwnd = NULL;
static HDC        g_hdc_mem = NULL;
static HBITMAP    g_hbitmap = NULL;
static uint32_t*  g_pixel_buffer = NULL;
static bool       g_is_running = true;

// Estado de teclado
static uint8_t    g_keyboard_mask = 0;

// Estado de audio WaveOut
static HWAVEOUT   g_hWaveOut = NULL;
#define WAVE_BLOCKS 4
#define SAMPLES_PER_BLOCK 735
static WAVEHDR    g_waveHeaders[WAVE_BLOCKS];
static int16_t    g_waveBuffers[WAVE_BLOCKS][SAMPLES_PER_BLOCK];
static int        g_currentWaveBlock = 0;

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_KEYDOWN: {
            switch (wParam) {
                case VK_UP:    case 'W': g_keyboard_mask |= MJK_BTN_UP; break;
                case VK_DOWN:  case 'S': g_keyboard_mask |= MJK_BTN_DOWN; break;
                case VK_LEFT:  case 'A': g_keyboard_mask |= MJK_BTN_LEFT; break;
                case VK_RIGHT: case 'D': g_keyboard_mask |= MJK_BTN_RIGHT; break;
                case VK_SPACE: case 'Z': case 'J': g_keyboard_mask |= MJK_BTN_A; break;
                case 'X':      case 'K': g_keyboard_mask |= MJK_BTN_B; break;
                case VK_RETURN:          g_keyboard_mask |= MJK_BTN_START; break;
                case VK_SHIFT:           g_keyboard_mask |= MJK_BTN_SELECT; break;
                case VK_ESCAPE: PostQuitMessage(0); break;
            }
            return 0;
        }

        case WM_KEYUP: {
            switch (wParam) {
                case VK_UP:    case 'W': g_keyboard_mask &= ~MJK_BTN_UP; break;
                case VK_DOWN:  case 'S': g_keyboard_mask &= ~MJK_BTN_DOWN; break;
                case VK_LEFT:  case 'A': g_keyboard_mask &= ~MJK_BTN_LEFT; break;
                case VK_RIGHT: case 'D': g_keyboard_mask &= ~MJK_BTN_RIGHT; break;
                case VK_SPACE: case 'Z': case 'J': g_keyboard_mask &= ~MJK_BTN_A; break;
                case 'X':      case 'K': g_keyboard_mask &= ~MJK_BTN_B; break;
                case VK_RETURN:          g_keyboard_mask &= ~MJK_BTN_START; break;
                case VK_SHIFT:           g_keyboard_mask &= ~MJK_BTN_SELECT; break;
            }
            return 0;
        }

        case WM_CLOSE:
        case WM_DESTROY:
            g_is_running = false;
            if (g_hWaveOut) {
                waveOutReset(g_hWaveOut);
                for (int i = 0; i < WAVE_BLOCKS; ++i) {
                    if (g_waveHeaders[i].dwFlags & WHDR_PREPARED) {
                        waveOutUnprepareHeader(g_hWaveOut, &g_waveHeaders[i], sizeof(WAVEHDR));
                    }
                }
                waveOutClose(g_hWaveOut);
                g_hWaveOut = NULL;
            }
            PostQuitMessage(0);
            return 0;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rc;
            GetClientRect(hwnd, &rc);
            StretchBlt(hdc, 0, 0, rc.right, rc.bottom, g_hdc_mem, 0, 0, MJK_SCREEN_WIDTH, MJK_SCREEN_HEIGHT, SRCCOPY);
            EndPaint(hwnd, &ps);
            return 0;
        }
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

bool Win32_InitPlatform(void) {
    HINSTANCE hInstance = GetModuleHandle(NULL);

    WNDCLASSEX wc = {sizeof(WNDCLASSEX)};
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = "MjkWindowClass";

    if (!RegisterClassEx(&wc)) {
        return false;
    }

    int client_w = MJK_SCREEN_WIDTH * WINDOW_SCALE;
    int client_h = MJK_SCREEN_HEIGHT * WINDOW_SCALE;

    RECT rc = {0, 0, client_w, client_h};
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

    g_hwnd = CreateWindowEx(
        0,
        "MjkWindowClass",
        "Mighty Bomb Jack - Native Source Port (PC)",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left, rc.bottom - rc.top,
        NULL, NULL, hInstance, NULL
    );

    if (!g_hwnd) return false;

    // Crear DIBSection para volcado directo de píxeles 32-bit RGBA
    HDC hdc_win = GetDC(g_hwnd);
    g_hdc_mem = CreateCompatibleDC(hdc_win);

    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = MJK_SCREEN_WIDTH;
    bmi.bmiHeader.biHeight = -MJK_SCREEN_HEIGHT; // Top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    g_hbitmap = CreateDIBSection(hdc_win, &bmi, DIB_RGB_COLORS, (void**)&g_pixel_buffer, NULL, 0);
    SelectObject(g_hdc_mem, g_hbitmap);
    ReleaseDC(g_hwnd, hdc_win);

    // Inicializar WaveOut para sonido PCM a 44.1 kHz
    WAVEFORMATEX wfx = {0};
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = 1; // Mono
    wfx.nSamplesPerSec = MJK_AUDIO_SAMPLE_RATE;
    wfx.wBitsPerSample = 16;
    wfx.nBlockAlign = wfx.nChannels * (wfx.wBitsPerSample / 8);
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

    if (waveOutOpen(&g_hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR) {
        g_hWaveOut = NULL;
        fprintf(stderr, "[AVISO]: No se pudo abrir dispositivo WaveOut. Continuando sin audio.\n");
    }

    for (int i = 0; i < WAVE_BLOCKS; ++i) {
        memset(&g_waveHeaders[i], 0, sizeof(WAVEHDR));
        g_waveHeaders[i].lpData = (LPSTR)g_waveBuffers[i];
        g_waveHeaders[i].dwBufferLength = SAMPLES_PER_BLOCK * sizeof(int16_t);
    }

    return true;
}

uint64_t Win32_GetTimeMicroseconds(void) {
    static LARGE_INTEGER freq;
    static BOOL initialized = FALSE;
    if (!initialized) {
        QueryPerformanceFrequency(&freq);
        initialized = TRUE;
    }
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return (uint64_t)((counter.QuadPart * 1000000) / freq.QuadPart);
}

void Win32_PollInputs(MjkInputState* out_inputs) {
    if (!out_inputs) return;

    // Procesar mensajes de ventana de Windows
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    uint8_t buttons = g_keyboard_mask;

    // Consultar mando de Xbox Series vía XInput (Controller 0)
    XINPUT_STATE xstate;
    if (XInputGetState(0, &xstate) == ERROR_SUCCESS) {
        WORD wButtons = xstate.Gamepad.wButtons;

        // D-Pad del mando Xbox
        if (wButtons & XINPUT_GAMEPAD_DPAD_UP)    buttons |= MJK_BTN_UP;
        if (wButtons & XINPUT_GAMEPAD_DPAD_DOWN)  buttons |= MJK_BTN_DOWN;
        if (wButtons & XINPUT_GAMEPAD_DPAD_LEFT)  buttons |= MJK_BTN_LEFT;
        if (wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) buttons |= MJK_BTN_RIGHT;

        // Stick analógico izquierdo mapeado a direcciones con zona muerta
        SHORT thumb_x = xstate.Gamepad.sThumbLX;
        SHORT thumb_y = xstate.Gamepad.sThumbLY;
        if (thumb_x < -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) buttons |= MJK_BTN_LEFT;
        if (thumb_x >  XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) buttons |= MJK_BTN_RIGHT;
        if (thumb_y < -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) buttons |= MJK_BTN_DOWN;
        if (thumb_y >  XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) buttons |= MJK_BTN_UP;

        // Botones de acción del mando Xbox Series
        if (wButtons & XINPUT_GAMEPAD_A)     buttons |= MJK_BTN_A;      // Botón A (verde) -> Salto
        if (wButtons & XINPUT_GAMEPAD_B)     buttons |= MJK_BTN_B;      // Botón B (rojo) -> Cofres/Acción
        if (wButtons & XINPUT_GAMEPAD_START) buttons |= MJK_BTN_START;  // Botón Menú -> Start
        if (wButtons & XINPUT_GAMEPAD_BACK)  buttons |= MJK_BTN_SELECT; // Botón Vista -> Select
    }

    out_inputs->held = buttons;
}

void Win32_PresentFramebuffer(const uint32_t* framebuffer) {
    if (!g_pixel_buffer || !framebuffer) return;

    // Copiar píxeles del núcleo al buffer DIBSection
    memcpy(g_pixel_buffer, framebuffer, MJK_SCREEN_WIDTH * MJK_SCREEN_HEIGHT * sizeof(uint32_t));

    // Forzar repintado con escalado por hardware GDI
    HDC hdc_win = GetDC(g_hwnd);
    RECT rc;
    GetClientRect(g_hwnd, &rc);
    StretchBlt(hdc_win, 0, 0, rc.right, rc.bottom, g_hdc_mem, 0, 0, MJK_SCREEN_WIDTH, MJK_SCREEN_HEIGHT, SRCCOPY);
    ReleaseDC(g_hwnd, hdc_win);
}

void Win32_QueueAudioSamples(const int16_t* samples, uint32_t count) {
    if (!g_hWaveOut || !samples || count == 0) return;

    WAVEHDR* currentHeader = &g_waveHeaders[g_currentWaveBlock];

    // Si el bloque sigue activamente en cola de reproducción, esperar brevemente
    if (currentHeader->dwFlags & WHDR_INQUEUE) {
        int timeout_ms = 2;
        while ((currentHeader->dwFlags & WHDR_INQUEUE) && timeout_ms-- > 0) {
            Sleep(1);
        }
        if (currentHeader->dwFlags & WHDR_INQUEUE) {
            // El dispositivo de audio sigue ocupado: descartar este frame para no congelar la ventana
            return;
        }
    }

    // Si el encabezado fue preparado en una iteración previa, desarmarlo
    if (currentHeader->dwFlags & WHDR_PREPARED) {
        waveOutUnprepareHeader(g_hWaveOut, currentHeader, sizeof(WAVEHDR));
    }

    uint32_t copy_count = count < SAMPLES_PER_BLOCK ? count : SAMPLES_PER_BLOCK;
    memcpy(g_waveBuffers[g_currentWaveBlock], samples, copy_count * sizeof(int16_t));

    currentHeader->lpData = (LPSTR)g_waveBuffers[g_currentWaveBlock];
    currentHeader->dwBufferLength = copy_count * sizeof(int16_t);
    currentHeader->dwFlags = 0;

    if (waveOutPrepareHeader(g_hWaveOut, currentHeader, sizeof(WAVEHDR)) == MMSYSERR_NOERROR) {
        waveOutWrite(g_hWaveOut, currentHeader, sizeof(WAVEHDR));
    }

    g_currentWaveBlock = (g_currentWaveBlock + 1) % WAVE_BLOCKS;
}

bool Win32_IsRunning(void) {
    return g_is_running;
}
