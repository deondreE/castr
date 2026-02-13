#include <stdio.h>
#include <GLFW/glfw3.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include "logger.h"

#define GL_BGRA 0x80E1

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

int main(void) {
    //  return -1;
    // }if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    //     log_fatal("Failed to initialize GLAD");
       
    GLFWwindow* window;

    log_info("Starting Castr....");

    if (!glfwInit()) {
        return -1;
    }

    int screen_w = 1920;
    int screen_h = 1080;

    window = glfwCreateWindow(screen_w, screen_h, "Castr Window", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    if (!init_capture()) {
        log_fatal("Capture init failed");
        return 0;
    }

    unsigned char* buffer = malloc(screen_w * screen_h * 4);

      GLuint tex_id;
    glGenTextures(1, &tex_id);
    glBindTexture(GL_TEXTURE_2D, tex_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, screen_w, screen_h, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);

    while (!glfwWindowShouldClose(window)) {
        if (capture_frame(buffer, screen_w, screen_h)) {
            glBindTexture(GL_TEXTURE_2D, tex_id);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, screen_w, screen_h, GL_BGRA, GL_UNSIGNED_BYTE, buffer);
        }
        
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
        glfwPollEvents();
    }

    free(buffer);
    glfwTerminate();
    return 0;
}
