#include <stdio.h>
#include <GLFW/glfw3.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <process.h>
#include <windows.h>
#include <stdbool.h>
#include "logger.h"
#include "encoder.h"
#include "threading.h"

#ifndef GL_BGRA
#define GL_BGRA 0x80E1
#endif

typedef struct {
    ID3D11Device* device;
    ID3D11DeviceContext* context;
    IDXGIOutputDuplication* duplication;
    ID3D11Texture2D* staging_tex;
} CaptureState;

CaptureState g_cap = {0};

int init_capture() {
    D3D_FEATURE_LEVEL fl;
    HRESULT hr = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION, &g_cap.device, &fl, &g_cap.context);
    
    IDXGIDevice* dxgiDevice = NULL;
    g_cap.device->lpVtbl->QueryInterface(g_cap.device, &IID_IDXGIDevice, (void**)&dxgiDevice);
    IDXGIAdapter* adapter = NULL;
    dxgiDevice->lpVtbl->GetParent(dxgiDevice, &IID_IDXGIAdapter, (void**)&adapter);
    IDXGIOutput* output = NULL;
    adapter->lpVtbl->EnumOutputs(adapter, 0, &output);
    IDXGIOutput1* output1 = NULL;
    output->lpVtbl->QueryInterface(output, &IID_IDXGIOutput1, (void**)&output1);

    hr = output1->lpVtbl->DuplicateOutput(output1, (IUnknown*)g_cap.device, &g_cap.duplication);
    if (FAILED(hr)) {
        log_error("Failed to duplicate output: %08X", hr);
        return 0;
    }
    return 1;
}

int capture_frame(unsigned char* out_data, int width, int height) {
    IDXGIResource* res = NULL;
    DXGI_OUTDUPL_FRAME_INFO info;
    HRESULT hr = g_cap.duplication->lpVtbl->AcquireNextFrame(g_cap.duplication, 100, &info, &res);
    
    if (hr == DXGI_ERROR_WAIT_TIMEOUT) return 0;
    if (FAILED(hr)) return 0;

    ID3D11Texture2D* tex = NULL;
    res->lpVtbl->QueryInterface(res, &IID_ID3D11Texture2D, (void**)&tex);

    if (!g_cap.staging_tex) {
        D3D11_TEXTURE2D_DESC desc;
        tex->lpVtbl->GetDesc(tex, &desc);
        desc.Usage = D3D11_USAGE_STAGING;
        desc.BindFlags = 0;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        desc.MiscFlags = 0;
        g_cap.device->lpVtbl->CreateTexture2D(g_cap.device, &desc, NULL, &g_cap.staging_tex);
    }

    g_cap.context->lpVtbl->CopyResource(g_cap.context, (ID3D11Resource*)g_cap.staging_tex, (ID3D11Resource*)tex);

    D3D11_MAPPED_SUBRESOURCE mapped;
    if (SUCCEEDED(g_cap.context->lpVtbl->Map(g_cap.context, (ID3D11Resource*)g_cap.staging_tex, 0, D3D11_MAP_READ, 0, &mapped))) {
        memcpy(out_data, mapped.pData, width * height * 4);
        g_cap.context->lpVtbl->Unmap(g_cap.context, (ID3D11Resource*)g_cap.staging_tex, 0);
    }

    tex->lpVtbl->Release(tex);
    res->lpVtbl->Release(res);
    g_cap.duplication->lpVtbl->ReleaseFrame(g_cap.duplication);
    return 1;
}

SharedState g_state = {0};

void init_shared_state(int w, int h) {
    g_state.width = w;
    g_state.height = h;
    g_state.running = true;
    g_state.frame_buffer = malloc(w * h * 4);
    InitializeCriticalSection(&g_state.lock);
    InitializeConditionVariable(&g_state.data_ready);
}

void cleanup_shared_state() {
    g_state.running = false;
    
    DeleteCriticalSection(&g_state.lock);
    
    if (g_state.frame_buffer) {
        free(g_state.frame_buffer);
        g_state.frame_buffer = NULL;
    }
}

unsigned __stdcall capture_thread_func(void* arg) {
    while (g_state.running) {
        static unsigned char* temp_buf = NULL;
        if (!temp_buf) temp_buf = malloc(g_state.width * g_state.height * 4);

        if (capture_frame(temp_buf, g_state.width, g_state.height)) {
            EnterCriticalSection(&g_state.lock);
            memcpy(g_state.frame_buffer, temp_buf, g_state.width * g_state.height * 4);
            g_state.has_new_frame = true; 
            g_state.encoder_has_work = true;
            LeaveCriticalSection(&g_state.lock);

            WakeAllConditionVariable(&g_state.data_ready);
        }
        Sleep(0);
    }
    return 0;
}

unsigned __stdcall encoder_thread_func(void* arg) {
    log_info("Encoder thread started.");

    unsigned char* local_enc_buf = malloc(g_state.width * g_state.height * 4);

    while (g_state.running) {
        EnterCriticalSection(&g_state.lock);

        while (g_state.running && !g_state.encoder_has_work) {
            SleepConditionVariableCS(&g_state.data_ready, &g_state.lock, INFINITE);
        }

        if (!g_state.running) {
            LeaveCriticalSection(&g_state.lock);
            break;
        }

        memcpy(local_enc_buf, g_state.frame_buffer, g_state.width * g_state.height * 4);
        g_state.encoder_has_work = false;

        LeaveCriticalSection(&g_state.lock);

        encode_frame(local_enc_buf);
    }

    free(local_enc_buf);
    log_info("Encoder thread exiting.");
    return 0;
}

int main(void) {
    log_info("Starting Castr....");

    if (!glfwInit()) {
        return -1;
    }

    int screen_w = 1920;
    int screen_h = 1080;

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Castr Engine - Multi-threaded", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    if (!init_capture()) {
        log_fatal("Capture init failed");
        return -1;
    }
    init_encoder("output.h264", screen_w, screen_h);

    init_shared_state(screen_w, screen_h);

    _beginthreadex(NULL, 0, capture_thread_func, NULL, 0, NULL);
    _beginthreadex(NULL, 0, encoder_thread_func, NULL, 0, NULL);

    GLuint tex_id;
    glGenTextures(1, &tex_id);
    glBindTexture(GL_TEXTURE_2D, tex_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, screen_w, screen_h, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        EnterCriticalSection(&g_state.lock);
        if (g_state.has_new_frame) {
            glBindTexture(GL_TEXTURE_2D, tex_id);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, screen_w, screen_h, GL_BGRA, GL_UNSIGNED_BYTE, g_state.frame_buffer);
            
            // encode_frame(g_state.frame_buffer);

            g_state.has_new_frame = false;
        }
        LeaveCriticalSection(&g_state.lock);

        glClear(GL_COLOR_BUFFER_BIT);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, tex_id);
        
        glBegin(GL_QUADS);
            glTexCoord2f(0, 1); glVertex2f(-1, -1);
            glTexCoord2f(1, 1); glVertex2f(1, -1);
            glTexCoord2f(1, 0); glVertex2f(1, 1);
            glTexCoord2f(0, 0); glVertex2f(-1, 1);
        glEnd();

        glfwSwapBuffers(window);
    }

    g_state.running = false;
    Sleep(100);
    cleanup_shared_state();
    glfwDestroyWindow(window);
    glfwTerminate();
    
    log_info("Castr shutdown cleanly.");
    return 0;
}