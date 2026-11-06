#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include/capi/cef_app_capi.h"
#include "include/capi/cef_client_capi.h"
#include "include/capi/cef_browser_capi.h"
#include "include/capi/cef_life_span_handler_capi.h"
#include "include/capi/cef_command_line_capi.h"
#include "include/cef_api_hash.h"
#include "include/internal/cef_types_win.h"

// Control IDs
#define IDC_NAV_BACK    101
#define IDC_NAV_FWD     102
#define IDC_NAV_RELOAD  103
#define IDC_NAV_URL     104
#define IDC_NAV_GO      105
#define IDC_NAV_SHIELD  106

#define TOOLBAR_HEIGHT  44

// Global browser instance
static cef_browser_t *g_browser = NULL;
static HWND g_hwnd_main = NULL;
static HWND g_hwnd_url = NULL;

// -------------------------------------------------------------
// Reference Counting Helpers
// -------------------------------------------------------------
typedef struct {
    cef_app_t app;
    volatile LONG ref_count;
} RefApp;

typedef struct {
    cef_client_t client;
    volatile LONG ref_count;
} RefClient;

typedef struct {
    cef_life_span_handler_t handler;
    volatile LONG ref_count;
} RefLifeSpan;

static void CEF_CALLBACK ref_add_ref(cef_base_ref_counted_t *base) {
    volatile LONG *ref = (volatile LONG *)((char *)base + base->size);
    InterlockedIncrement(ref);
}

static int CEF_CALLBACK ref_release(cef_base_ref_counted_t *base) {
    volatile LONG *ref = (volatile LONG *)((char *)base + base->size);
    LONG c = InterlockedDecrement(ref);
    return (c == 0);
}

static int CEF_CALLBACK ref_has_one(cef_base_ref_counted_t *base) {
    volatile LONG *ref = (volatile LONG *)((char *)base + base->size);
    return (*ref == 1);
}

static int CEF_CALLBACK ref_has_at_least_one(cef_base_ref_counted_t *base) {
    volatile LONG *ref = (volatile LONG *)((char *)base + base->size);
    return (*ref >= 1);
}

#define INIT_REF(obj, type) do { \
    memset(obj, 0, sizeof(*(obj))); \
    (obj)->ref_count = 1; \
    (obj)->type.base.size = sizeof((obj)->type); \
    (obj)->type.base.add_ref = ref_add_ref; \
    (obj)->type.base.release = ref_release; \
    (obj)->type.base.has_one_ref = ref_has_one; \
    (obj)->type.base.has_at_least_one_ref = ref_has_at_least_one; \
} while(0)

// -------------------------------------------------------------
// App Handler: Command Line Tuning & Privacy Switches
// -------------------------------------------------------------
static void CEF_CALLBACK on_before_command_line_processing(
    cef_app_t *self,
    const cef_string_t *process_type,
    cef_command_line_t *command_line) {

    if (!process_type || process_type->length == 0) {
        cef_string_t s_nosandbox = {0};
        cef_string_t s_disext = {0};
        cef_string_t s_disgpu = {0};

        cef_string_from_utf8("no-sandbox", 10, &s_nosandbox);
        cef_string_from_utf8("disable-extensions", 18, &s_disext);
        cef_string_from_utf8("disable-gpu", 11, &s_disgpu);

        command_line->append_switch(command_line, &s_nosandbox);
        command_line->append_switch(command_line, &s_disext);
        command_line->append_switch(command_line, &s_disgpu);

        cef_string_clear(&s_nosandbox);
        cef_string_clear(&s_disext);
        cef_string_clear(&s_disgpu);
    }
}

static RefApp g_app;

static void init_app(void) {
    INIT_REF(&g_app, app);
    g_app.app.on_before_command_line_processing = on_before_command_line_processing;
}

// -------------------------------------------------------------
// LifeSpan & Client Handlers
// -------------------------------------------------------------
static void CEF_CALLBACK on_after_created(cef_life_span_handler_t *self, cef_browser_t *browser) {
    g_browser = browser;
    browser->base.add_ref(&browser->base);
}

static void CEF_CALLBACK on_before_close(cef_life_span_handler_t *self, cef_browser_t *browser) {
    if (g_browser) {
        g_browser = NULL;
    }
}

static RefLifeSpan g_lifespan;
static RefClient g_client;

static cef_life_span_handler_t* CEF_CALLBACK get_life_span_handler(cef_client_t *self) {
    return &g_lifespan.handler;
}

static void init_client(void) {
    INIT_REF(&g_lifespan, handler);
    g_lifespan.handler.on_after_created = on_after_created;
    g_lifespan.handler.on_before_close = on_before_close;

    INIT_REF(&g_client, client);
    g_client.client.get_life_span_handler = get_life_span_handler;
}

// -------------------------------------------------------------
// URL Navigation & Search Dispatcher
// -------------------------------------------------------------
static void navigate_to(const char *input) {
    if (!g_browser || !input || strlen(input) == 0) return;

    char target_url[2048] = {0};

    if (strncmp(input, "http://", 7) == 0 || strncmp(input, "https://", 8) == 0) {
        snprintf(target_url, sizeof(target_url), "%s", input);
    } else if (strchr(input, '.') && !strchr(input, ' ')) {
        snprintf(target_url, sizeof(target_url), "https://%s", input);
    } else {
        char query_encoded[1024] = {0};
        size_t qi = 0;
        for (size_t i = 0; i < strlen(input) && qi < sizeof(query_encoded) - 4; i++) {
            if (input[i] == ' ') {
                query_encoded[qi++] = '+';
            } else {
                query_encoded[qi++] = input[i];
            }
        }
        snprintf(target_url, sizeof(target_url), "https://duckduckgo.com/?q=%s", query_encoded);
    }

    cef_string_t cef_url = {0};
    cef_string_from_utf8(target_url, strlen(target_url), &cef_url);

    cef_frame_t *frame = g_browser->get_main_frame(g_browser);
    if (frame) {
        frame->load_url(frame, &cef_url);
    }

    cef_string_clear(&cef_url);
}

// -------------------------------------------------------------
// Win32 GUI Window Procedure
// -------------------------------------------------------------
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            // Navigation Buttons
            CreateWindow("BUTTON", "<", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                         10, 6, 32, 30, hwnd, (HMENU)IDC_NAV_BACK, NULL, NULL);

            CreateWindow("BUTTON", ">", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                         46, 6, 32, 30, hwnd, (HMENU)IDC_NAV_FWD, NULL, NULL);

            CreateWindow("BUTTON", "R", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                         82, 6, 32, 30, hwnd, (HMENU)IDC_NAV_RELOAD, NULL, NULL);

            // Shield status badge
            CreateWindow("STATIC", "[Shields UP]", WS_CHILD | WS_VISIBLE | SS_CENTER,
                         120, 11, 95, 22, hwnd, (HMENU)IDC_NAV_SHIELD, NULL, NULL);

            // Omnibox (URL & Search)
            g_hwnd_url = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "https://duckduckgo.com",
                                        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                                        223, 7, 500, 28, hwnd, (HMENU)IDC_NAV_URL, NULL, NULL);

            CreateWindow("BUTTON", "Search / Go", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                         731, 6, 100, 30, hwnd, (HMENU)IDC_NAV_GO, NULL, NULL);

            return 0;
        }

        case WM_COMMAND: {
            int wmId = LOWORD(wParam);
            if (wmId == IDC_NAV_BACK && g_browser) {
                g_browser->go_back(g_browser);
            } else if (wmId == IDC_NAV_FWD && g_browser) {
                g_browser->go_forward(g_browser);
            } else if (wmId == IDC_NAV_RELOAD && g_browser) {
                g_browser->reload(g_browser);
            } else if (wmId == IDC_NAV_GO || (wmId == IDC_NAV_URL && HIWORD(wParam) == 1)) {
                char buffer[1024] = {0};
                GetWindowText(g_hwnd_url, buffer, sizeof(buffer) - 1);
                navigate_to(buffer);
            }
            return 0;
        }

        case WM_SIZE: {
            int width = LOWORD(lParam);
            int height = HIWORD(lParam);

            if (g_hwnd_url) {
                int url_width = width - 345;
                if (url_width < 150) url_width = 150;
                SetWindowPos(g_hwnd_url, NULL, 223, 7, url_width, 28, SWP_NOZORDER);
                SetWindowPos(GetDlgItem(hwnd, IDC_NAV_GO), NULL, 231 + url_width, 6, 100, 30, SWP_NOZORDER);
            }

            if (g_browser) {
                cef_browser_host_t *host = g_browser->get_host(g_browser);
                if (host) {
                    HWND browser_hwnd = host->get_window_handle(host);
                    if (browser_hwnd) {
                        SetWindowPos(browser_hwnd, NULL, 0, TOOLBAR_HEIGHT,
                                     width, height - TOOLBAR_HEIGHT, SWP_NOZORDER);
                    }
                }
            }
            return 0;
        }

        case WM_ERASEBKGND:
            return 1;

        case WM_DESTROY: {
            cef_quit_message_loop();
            PostQuitMessage(0);
            return 0;
        }
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// -------------------------------------------------------------
// Application Entry Point
// -------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // 1. Initialize CEF API contract
    cef_api_hash(CEF_API_VERSION, 0);

    init_app();

    cef_main_args_t main_args;
    main_args.instance = hInstance;

    int exit_code = cef_execute_process(&main_args, &g_app.app, NULL);
    if (exit_code >= 0) {
        return exit_code;
    }

    init_client();

    cef_settings_t settings;
    memset(&settings, 0, sizeof(settings));
    settings.size = sizeof(cef_settings_t);
    settings.no_sandbox = 1;
    settings.multi_threaded_message_loop = 0;
    settings.log_severity = LOGSEVERITY_WARNING;

    char root_cache[MAX_PATH];
    char profile_cache[MAX_PATH];
    GetFullPathNameA("profile", MAX_PATH, root_cache, NULL);
    GetFullPathNameA("profile\\cache", MAX_PATH, profile_cache, NULL);
    CreateDirectoryA(root_cache, NULL);
    CreateDirectoryA(profile_cache, NULL);

    cef_string_from_utf8(root_cache, strlen(root_cache), &settings.root_cache_path);
    cef_string_from_utf8(profile_cache, strlen(profile_cache), &settings.cache_path);

    if (!cef_initialize(&main_args, &settings, &g_app.app, NULL)) {
        MessageBox(NULL, "Failed to initialize Vardan's Browser engine.", "Vardan's Browser", MB_ICONERROR);
        return 1;
    }

    cef_string_clear(&settings.root_cache_path);
    cef_string_clear(&settings.cache_path);

    // Register Win32 Class
    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = "VardanBrowserWindow";

    RegisterClassEx(&wc);

    // Create Main Browser Window: Clean title "Vardan's Browser"
    g_hwnd_main = CreateWindowEx(
        WS_EX_APPWINDOW,
        "VardanBrowserWindow",
        "Vardan's Browser",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT, 1200, 800,
        NULL, NULL, hInstance, NULL
    );

    if (!g_hwnd_main) {
        cef_shutdown();
        return 1;
    }

    ShowWindow(g_hwnd_main, nCmdShow);
    UpdateWindow(g_hwnd_main);

    RECT rect;
    GetClientRect(g_hwnd_main, &rect);

    cef_window_info_t window_info;
    memset(&window_info, 0, sizeof(window_info));
    window_info.size = sizeof(cef_window_info_t);
    window_info.style = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_TABSTOP;
    window_info.parent_window = g_hwnd_main;
    window_info.bounds.x = 0;
    window_info.bounds.y = TOOLBAR_HEIGHT;
    window_info.bounds.width = rect.right;
    window_info.bounds.height = rect.bottom - TOOLBAR_HEIGHT;

    cef_browser_settings_t browser_settings;
    memset(&browser_settings, 0, sizeof(browser_settings));
    browser_settings.size = sizeof(cef_browser_settings_t);

    const char *start_url = "https://duckduckgo.com";
    cef_string_t cef_start_url = {0};
    cef_string_from_utf8(start_url, strlen(start_url), &cef_start_url);

    cef_browser_host_create_browser(
        &window_info,
        &g_client.client,
        &cef_start_url,
        &browser_settings,
        NULL,
        NULL
    );

    cef_string_clear(&cef_start_url);

    cef_run_message_loop();
    cef_shutdown();
    return 0;
}
