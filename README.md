# Vardan's Browser 🌐

A fast, lightweight, privacy-focused desktop web browser engineered in pure **C**.

Built by embedding the open-source **Chromium Embedded Framework (CEF)** directly into a native Win32 window subsystem, providing high-performance modern web compatibility (HTML5, CSS3, JavaScript, WebGL) without the memory bloat of Electron.

---

## Features

* **Pure C Core:** Built and compiled with GCC using native Win32 APIs and CEF C-API.
* **Modern Web Engine:** Full Chromium rendering pipeline (Blink engine & V8 JavaScript runtime).
* **Smart Omnibox:** Directly loads URLs or routes search queries seamlessly.
* **Privacy Shields:** Built-in tracker and telemetry suppression flags.
* **Zero Bloat:** Minimalist desktop interface designed for maximum viewport speed.

---

## Project Structure

```text
├── src/
│   ├── browser.c        # Win32 GUI window & Chromium C-API integration
│   ├── main.c           # CLI search engine entrypoint
│   ├── http.c / http.h  # Network request engine
│   └── parser.c / .h    # Search result parser
├── include/             # Headers
├── build_browser.bat    # 1-Click compiler script for the GUI browser
├── build.bat            # CLI engine build script
└── Makefile             # Cross-platform makefile
```

---

## How to Build & Run

### Prerequisites
* Windows 10/11 (64-bit)
* GCC compiler (via MSYS2 MinGW-w64 / UCRT64)

### 1-Click Build
```cmd
build_browser.bat
```

### Launch Browser
```cmd
.\bin\vardan_browser.exe
```

---

## License
MIT License
