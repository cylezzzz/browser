# NovaBrowse (MVP) — Native C++20 Desktop Browser Shell (Qt 6 + QtWebEngine)

This is a **production-minded MVP** of a native desktop browser shell:
- **Qt 6 Widgets** UI (Chrome-inspired layout: tabs, omnibox, toolbar, side panel)
- **QtWebEngine** for real page rendering (Chromium embedded)
- Local services in C++:
  - Web search (DuckDuckGo HTML scraping)
  - Page reader extraction + analyzer
  - DeepSearch skeleton (entity extraction + graph-ish view)
  - Local AI via **Ollama** HTTP API (no cloud)
- Persistence: **SQLite3 + FTS5**
- Build: **CMake + vcpkg (manifest mode)**

> Notes:
> - This is **not** a Chromium/Gecko clone. Rendering is delegated to QtWebEngine.
> - No paywall/captcha/geoblock bypass code. Fetch pipeline uses rate limiting and avoids cookie persistence.

## Quick Start (Windows 10/11)

### Prerequisites
1) Install **Qt 6** (including **QtWebEngine**) via Qt Online Installer.
   - Recommended: Qt 6.6+ (MSVC 2022 64-bit kit)
2) Install **Visual Studio 2022** with C++ Desktop workload
3) Install **vcpkg** and enable manifest mode

### Build (PowerShell)
```powershell
git clone <your repo url> NovaBrowse
cd NovaBrowse

# Set this to your vcpkg path
$env:VCPKG_ROOT="C:\dev\vcpkg"

cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake" -DVCPKG_FEATURE_FLAGS=manifests
cmake --build build --config Release
```

### Run
```powershell
.\build\Release\NovaBrowse.exe
```

### Ollama
Install Ollama, run:
```powershell
ollama serve
ollama pull llama3.2
```
Then in NovaBrowse: Settings -> AI model/host.

