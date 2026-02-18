#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <process.h>
#include <windows.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <d3d11.h>
#include <dxgi1_2.h>

#include "logger.h"
#include "font.h"
#include "encoder.h"
#include "ui.h"
#include "threading.h"

#ifndef GL_BGRA
#define GL_BGRA 0x80E1
#endif
#ifndef GL_FRAMEBUFFER
#define GL_FRAMEBUFFER 0x8D40
#endif
#ifndef GL_COLOR_ATTACHMENT0
#define GL_COLOR_ATTACHMENT0 0x8CE0
#endif
#ifndef GL_FRAMEBUFFER_COMPLETE
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5
#endif

typedef struct {
    ID3D11Device*           device;
    ID3D11DeviceContext*    context;
    IDXGIOutputDuplication* duplication;
    ID3D11Texture2D*        staging_tex;
    UINT                    tex_width;
    UINT                    tex_height;
} CaptureState;

typedef struct {
    float x, y;
    float width, height;
    float scale;
    float opacity;
} RenderSource;

static RenderSource desktop_source = {
    .x      = 0,
    .y      = 0,
    .width  = 1920.0f,
    .height = 1080.0f,
    .scale  = 0.5f,
    .opacity = 1.0f
};

static CaptureState g_cap   = {0};
static SharedState  g_state = {0};
static GLuint       g_fbo, g_canvas_tex;

static HANDLE g_capture_thread = NULL;
static HANDLE g_encoder_thread = NULL;

static int create_duplication(void) {
    if (g_cap.duplication) {
        g_cap.duplication->lpVtbl->Release(g_cap.duplication);
        g_cap.duplication = NULL;
    }

    IDXGIDevice*  dxgi_device = NULL;
    IDXGIAdapter* adapter     = NULL;
    IDXGIOutput*  output      = NULL;
    IDXGIOutput1* output1     = NULL;

    g_cap.device->lpVtbl->QueryInterface(
        g_cap.device, &IID_IDXGIDevice, (void**)&dxgi_device);
    dxgi_device->lpVtbl->GetParent(
        dxgi_device, &IID_IDXGIAdapter, (void**)&adapter);
    adapter->lpVtbl->EnumOutputs(adapter, 0, &output);
    output->lpVtbl->QueryInterface(
        output, &IID_IDXGIOutput1, (void**)&output1);

    HRESULT hr = output1->lpVtbl->DuplicateOutput(
        output1, (IUnknown*)g_cap.device, &g_cap.duplication);

    output1->lpVtbl->Release(output1);
    output->lpVtbl->Release(output);
    adapter->lpVtbl->Release(adapter);
    dxgi_device->lpVtbl->Release(dxgi_device);

    if (FAILED(hr)) {
        log_error("DuplicateOutput failed: 0x%08X", hr);
        return 0;
    }
    return 1;
}

static int init_capture(void) {
    D3D_FEATURE_LEVEL fl;
    HRESULT hr = D3D11CreateDevice(
        NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0,
        NULL, 0, D3D11_SDK_VERSION,
        &g_cap.device, &fl, &g_cap.context);
    if (FAILED(hr)) {
        log_error("D3D11CreateDevice failed: 0x%08X", hr);
        return 0;
    }
    return 1;
}

static int capture_frame(unsigned char* out_data, int width, int height) {
    if (!g_cap.duplication) return 0;

    IDXGIResource*          res  = NULL;
    DXGI_OUTDUPL_FRAME_INFO info = {0};

    HRESULT hr = g_cap.duplication->lpVtbl->AcquireNextFrame(
        g_cap.duplication, 33, &info, &res);

    if (hr == DXGI_ERROR_WAIT_TIMEOUT) return 0;

    if (hr == DXGI_ERROR_ACCESS_LOST) {
        log_error("Desktop duplication access lost, recreating...");
        create_duplication();
        return 0;
    }

    if (FAILED(hr)) {
        log_error("AcquireNextFrame failed: 0x%08X", hr);
        return 0;
    }

    if (info.LastPresentTime.QuadPart == 0) {
        res->lpVtbl->Release(res);
        g_cap.duplication->lpVtbl->ReleaseFrame(g_cap.duplication);
        return 0;
    }

    ID3D11Texture2D* tex = NULL;
    hr = res->lpVtbl->QueryInterface(res, &IID_ID3D11Texture2D, (void**)&tex);
    if (FAILED(hr) || !tex) {
        res->lpVtbl->Release(res);
        g_cap.duplication->lpVtbl->ReleaseFrame(g_cap.duplication);
        return 0;
    }

    D3D11_TEXTURE2D_DESC acquired_desc;
    tex->lpVtbl->GetDesc(tex, &acquired_desc);

    if (!g_cap.staging_tex ||
        g_cap.tex_width  != acquired_desc.Width ||
        g_cap.tex_height != acquired_desc.Height)
    {
        if (g_cap.staging_tex) {
            g_cap.staging_tex->lpVtbl->Release(g_cap.staging_tex);
            g_cap.staging_tex = NULL;
        }
        D3D11_TEXTURE2D_DESC desc  = {0};
        desc.Width                 = acquired_desc.Width;
        desc.Height                = acquired_desc.Height;
        desc.MipLevels             = 1;
        desc.ArraySize             = 1;
        desc.Format                = acquired_desc.Format;
        desc.SampleDesc.Count      = 1;
        desc.Usage                 = D3D11_USAGE_STAGING;
        desc.BindFlags             = 0;
        desc.CPUAccessFlags        = D3D11_CPU_ACCESS_READ;
        desc.MiscFlags             = 0;
        hr = g_cap.device->lpVtbl->CreateTexture2D(
            g_cap.device, &desc, NULL, &g_cap.staging_tex);
        if (FAILED(hr)) {
            log_error("CreateTexture2D (staging) failed: 0x%08X", hr);
            tex->lpVtbl->Release(tex);
            res->lpVtbl->Release(res);
            g_cap.duplication->lpVtbl->ReleaseFrame(g_cap.duplication);
            return 0;
        }
        g_cap.tex_width  = acquired_desc.Width;
        g_cap.tex_height = acquired_desc.Height;
    }

    g_cap.context->lpVtbl->CopyResource(
        g_cap.context,
        (ID3D11Resource*)g_cap.staging_tex,
        (ID3D11Resource*)tex);

    tex->lpVtbl->Release(tex);
    res->lpVtbl->Release(res);
    g_cap.duplication->lpVtbl->ReleaseFrame(g_cap.duplication);

    D3D11_MAPPED_SUBRESOURCE mapped = {0};
    hr = g_cap.context->lpVtbl->Map(
        g_cap.context, (ID3D11Resource*)g_cap.staging_tex,
        0, D3D11_MAP_READ, 0, &mapped);
    if (FAILED(hr)) {
        log_error("Map failed: 0x%08X", hr);
        return 0;
    }

    const unsigned char* src   = (const unsigned char*)mapped.pData;
    unsigned char*       dst   = out_data;
    int copy_h = (int)g_cap.tex_height < height ? (int)g_cap.tex_height : height;
    int copy_w = (int)g_cap.tex_width  < width  ? (int)g_cap.tex_width  : width;

    for (int row = 0; row < copy_h; row++) {
        memcpy(dst, src, (size_t)copy_w * 4);
        src += mapped.RowPitch;
        dst += (size_t)width * 4;
    }

    g_cap.context->lpVtbl->Unmap(
        g_cap.context, (ID3D11Resource*)g_cap.staging_tex, 0);

    return 1;
}

static void init_shared_state(int w, int h) {
    g_state.width         = w;
    g_state.height        = h;
    g_state.running       = true;

    g_state.frame_buffer  = malloc((size_t)w * h * 4);
    // g_state.encode_buffer = malloc((size_t)w * h * 4);

    // if (!g_state.frame_buffer || !g_state.encode_buffer) {
    //     log_error("Failed to allocate shared buffers");
    //     exit(1);
    // }

    memset(g_state.frame_buffer,  0, (size_t)w * h * 4);
    // memset(g_state.encode_buffer, 0, (size_t)w * h * 4);

    InitializeCriticalSection(&g_state.lock);
    InitializeConditionVariable(&g_state.data_ready);
}

static unsigned __stdcall capture_thread_func(void* arg) {
    (void)arg;

    if (!create_duplication()) {
        log_error("capture_thread: failed to create duplication");
        return 1;
    }

    unsigned char* temp_buf =
        malloc((size_t)g_state.width * g_state.height * 4);
    if (!temp_buf) return 1;

    while (g_state.running) {
        if (capture_frame(temp_buf, g_state.width, g_state.height)) {
            EnterCriticalSection(&g_state.lock);
            memcpy(g_state.frame_buffer, temp_buf,
                   (size_t)g_state.width * g_state.height * 4);
            g_state.has_new_frame = true;
            LeaveCriticalSection(&g_state.lock);
            WakeAllConditionVariable(&g_state.data_ready);
        }
    }

    free(temp_buf);
    return 0;
}

static unsigned __stdcall encoder_thread_func(void* arg) {
    (void)arg;
    unsigned char* local_buf =
        malloc((size_t)g_state.width * g_state.height * 4);
    if (!local_buf) return 1;

    while (1) {
        EnterCriticalSection(&g_state.lock);
        while (g_state.running && !g_state.encoder_has_work)
            SleepConditionVariableCS(
                &g_state.data_ready, &g_state.lock, INFINITE);
        if (!g_state.running) {
            LeaveCriticalSection(&g_state.lock);
            break;
        }
        // memcpy(local_buf, g_state.encode_buffer,
               // (size_t)g_state.width * g_state.height * 4);
        g_state.encoder_has_work = false;
        LeaveCriticalSection(&g_state.lock);
        encode_frame(local_buf);
    }

    free(local_buf);
    return 0;
}

static void init_compositor(int w, int h) {
    glGenFramebuffers(1, &g_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, g_fbo);

    glGenTextures(1, &g_canvas_tex);
    glBindTexture(GL_TEXTURE_2D, g_canvas_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, g_canvas_tex, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        log_error("Framebuffer incomplete");

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

GLuint pbos[2];
int pbo_index = 0;

void init_pbos(int w, int h) {
    int data_size = w * h * 4;
    glGenBuffers(2, pbos);

    for (size_t i = 0; i < 2; ++i)
    {
        glBindBuffer(GL_PIXEL_PACK_BUFFER, pbos[i]);
        glBufferData(GL_PIXEL_PACK_BUFFER, data_size, NULL, GL_STREAM_READ);
    }
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
}

static void flip_bgra_vertical(unsigned char* buf, int width, int height) {
    size_t         row_bytes = (size_t)width * 4;
    unsigned char* tmp       = malloc(row_bytes);
    if (!tmp) return;
    for (int top = 0, bot = height - 1; top < bot; top++, bot--) {
        memcpy(tmp,                   buf + top * row_bytes, row_bytes);
        memcpy(buf + top * row_bytes, buf + bot * row_bytes, row_bytes);
        memcpy(buf + bot * row_bytes, tmp,                   row_bytes);
    }
    free(tmp);
}

int main(void) {
    if (!glfwInit()) return -1;

    const int screen_w = 1920, screen_h = 1080;
    const int window_w = 1280, window_h = 720;

    GLFWwindow* window = glfwCreateWindow(
        window_w, window_h, "Castr Engine", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    log_info("GL version: %s", glGetString(GL_VERSION));

    Font main_font;
    font_init(&main_font, "C:/Windows/Fonts/Arial.ttf", 32.0f);

    if (!init_capture()) {
        log_error("Capture init failed");
        glfwTerminate();
        return -1;
    }

    init_encoder("recording.mkv", screen_w, screen_h);
    init_shared_state(screen_w, screen_h);
    init_compositor(screen_w, screen_h);
    init_pbos(screen_w, screen_h);

    unsigned cap_tid, enc_tid;
    g_capture_thread = (HANDLE)_beginthreadex(
        NULL, 0, capture_thread_func, NULL, 0, &cap_tid);
    g_encoder_thread = (HANDLE)_beginthreadex(
        NULL, 0, encoder_thread_func, NULL, 0, &enc_tid);

    GLuint desktop_tex;
    glGenTextures(1, &desktop_tex);
    glBindTexture(GL_TEXTURE_2D, desktop_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, screen_w, screen_h,
                 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    const size_t   frame_bytes = (size_t)screen_w * screen_h * 4;
    unsigned char* upload_buf  = malloc(frame_bytes);
    unsigned char* encoder_buf = malloc(frame_bytes);
    if (!upload_buf || !encoder_buf) {
        log_error("Failed to allocate upload/encoder buffers");
        return -1;
    }


    float render_w = desktop_source.width - desktop_source.scale;
    float render_h = desktop_source.height - desktop_source.scale;
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ui_update(window);

        if (ui.mouse_down) {
            if (ui.dragging_item == 0 && ui.mouse_clicked) {
                if (ui.mouse_x >= desktop_source.x && ui.mouse_x <= desktop_source.x + render_w &&
                    ui.mouse_y >= desktop_source.y && ui.mouse_y <= desktop_source.y + render_h) {
                    ui.dragging_item = 1;
                }
            }

            if (ui.dragging_item == 1) {
                float delta_x = (float)(ui.mouse_x - ui.last_mouse_x);
                float delta_y = (float)(ui.mouse_y - ui.last_mouse_y);

                desktop_source.x += delta_x;
                desktop_source.y += delta_y;
            }
        }
        bool is_hovering_desktop = (ui.mouse_x >= desktop_source.x && ui.mouse_x <= desktop_source.x + render_w &&
                            ui.mouse_y >= desktop_source.y && ui.mouse_y <= desktop_source.y + render_h);

        bool has_frame = false;
        EnterCriticalSection(&g_state.lock);
        if (g_state.has_new_frame) {
            memcpy(upload_buf, g_state.frame_buffer, frame_bytes);
            g_state.has_new_frame = false;
            has_frame = true;
        }
        LeaveCriticalSection(&g_state.lock);

        if (has_frame) {
            glBindTexture(GL_TEXTURE_2D, desktop_tex);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, screen_w, screen_h, GL_BGRA, GL_UNSIGNED_BYTE, upload_buf);
            glBindFramebuffer(GL_FRAMEBUFFER, g_fbo);
            glViewport(0, 0, screen_w, screen_h);
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            glOrtho(0, screen_w, screen_h, 0, -1, 1);
            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();

            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, desktop_tex);
            glPushMatrix();
            glTranslatef(desktop_source.x, desktop_source.y, 0);
            glScalef(desktop_source.scale, desktop_source.scale, 1.0f);
            glBegin(GL_QUADS);
            glTexCoord2f(0, 1); glVertex2f(0, 0);
            glTexCoord2f(1, 1); glVertex2f(desktop_source.width, 0);
            glTexCoord2f(1, 0); glVertex2f(desktop_source.width, desktop_source.height);
            glTexCoord2f(0, 0); glVertex2f(0, desktop_source.height);
            glEnd();
            glPopMatrix();

            if (ui.dragging_item == 1 || is_hovering_desktop) {
                glDisable(GL_TEXTURE_2D);
                glColor3f(0.0f, 0.1f, 0.0f);
                glLineWidth(2.0f);
                glBegin(GL_LINE_LOOP);
                glVertex2f(desktop_source.x, desktop_source.y);
                glVertex2f(desktop_source.x + render_w, desktop_source.y);
                glVertex2f(desktop_source.x + render_w, desktop_source.y + render_h);
                glVertex2f(desktop_source.x, desktop_source.y + render_h);
                glEnd();
                glLineWidth(1.0f);
                glEnable(GL_TEXTURE_2D);
            }

            if (ui_button(1, &main_font, "MOVE", 20, 20, 180, 60))
                desktop_source.x += 10;

            glDisable(GL_TEXTURE_2D);

            pbo_index = (pbo_index + 1) % 2;
            int next_index = (pbo_index + 1) % 2;

            glBindBuffer(GL_PIXEL_PACK_BUFFER, pbos[pbo_index]);
            glReadPixels(0, 0, screen_w, screen_h, GL_BGRA, GL_UNSIGNED_BYTE, 0);

            glBindBuffer(GL_PIXEL_PACK_BUFFER, pbos[next_index]);
            void* ptr = glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
            if (ptr) {
                memcpy(encoder_buf, ptr, frame_bytes);
                glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
            }
            glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            flip_bgra_vertical(encoder_buf, screen_w, screen_h);

            EnterCriticalSection(&g_state.lock);
            g_state.encoder_has_work = true;
            WakeAllConditionVariable(&g_state.data_ready);
            LeaveCriticalSection(&g_state.lock);
        }

        glViewport(0, 0, window_w, window_h);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(-1, 1, -1, 1, -1, 1);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, g_canvas_tex);
        glBegin(GL_QUADS);
            glTexCoord2f(0, 0); glVertex2f(-1, -1);
            glTexCoord2f(1, 0); glVertex2f( 1, -1);
            glTexCoord2f(1, 1); glVertex2f( 1,  1);
            glTexCoord2f(0, 1); glVertex2f(-1,  1);
        glEnd();
        glDisable(GL_TEXTURE_2D);

        glfwSwapBuffers(window);

        if (!has_frame) Sleep(1);
    }

    EnterCriticalSection(&g_state.lock);
    g_state.running = false;
    WakeAllConditionVariable(&g_state.data_ready);
    LeaveCriticalSection(&g_state.lock);

    HANDLE threads[2] = { g_capture_thread, g_encoder_thread };
    WaitForMultipleObjects(2, threads, TRUE, 3000);
    CloseHandle(g_capture_thread);
    CloseHandle(g_encoder_thread);

    free(upload_buf);
    free(encoder_buf);
    cleanup_encoder();
    free(g_state.frame_buffer);
    // free(g_state.encode_buffer);
    DeleteCriticalSection(&g_state.lock);

    if (g_cap.staging_tex) g_cap.staging_tex->lpVtbl->Release(g_cap.staging_tex);
    if (g_cap.duplication) g_cap.duplication->lpVtbl->Release(g_cap.duplication);
    if (g_cap.context)     g_cap.context->lpVtbl->Release(g_cap.context);
    if (g_cap.device)      g_cap.device->lpVtbl->Release(g_cap.device);

    glDeleteTextures(1, &desktop_tex);
    glDeleteTextures(1, &g_canvas_tex);
    glDeleteFramebuffers(1, &g_fbo);
    glfwTerminate();
    return 0;
}
