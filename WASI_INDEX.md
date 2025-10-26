# WASI-SDK for ONNXRuntime - Documentation Index

Welcome to the WASI-SDK build system for ONNXRuntime! This index will help you find the right documentation for your needs.

## 🚀 Getting Started (Choose Your Path)

### I'm new to WASI and want to build quickly
→ **Start here**: [WASI_QUICK_REFERENCE.md](WASI_QUICK_REFERENCE.md)
- One-page quick start guide
- Essential commands only
- Common troubleshooting

### I want comprehensive instructions
→ **Start here**: [README_WASI.md](README_WASI.md)
- Complete setup guide
- Detailed build options
- Integration examples
- Extensive troubleshooting

### I want to understand the technical details
→ **Start here**: [WASI_SDK_MIGRATION.md](WASI_SDK_MIGRATION.md)
- Technical migration details
- What changed and why
- Feature comparison
- Build system internals

## 📋 Before You Build

### Verify Your Setup
```bash
./verify_wasi_setup.sh
```

This script checks:
- ✓ WASI-SDK installation
- ✓ Build tools (CMake, make, etc.)
- ✓ WASI runtimes (optional)
- ✓ Required files

## 📚 Documentation Overview

| Document | Purpose | Audience |
|----------|---------|----------|
| [README_WASI.md](README_WASI.md) | Complete guide | All users |
| [WASI_QUICK_REFERENCE.md](WASI_QUICK_REFERENCE.md) | Quick reference | Experienced users |
| [WASI_SDK_MIGRATION.md](WASI_SDK_MIGRATION.md) | Technical details | Developers |
| [WASI_ARCHITECTURE.md](WASI_ARCHITECTURE.md) | Architecture diagrams | Architects/Developers |
| [WASI_CHANGES_SUMMARY.md](WASI_CHANGES_SUMMARY.md) | List of changes | Contributors |
| [WASI_INDEX.md](WASI_INDEX.md) | This file | Everyone |

## 🔧 Build Scripts

| Script | Purpose | When to Use |
|--------|---------|-------------|
| `verify_wasi_setup.sh` | Verify setup | Before first build |
| `build_wasi_simple.sh` | Quick build | Most common use |
| `build_wasi.sh` | Detailed build | Advanced configuration |
| Manual CMake | Full control | Custom builds |

## 🗂️ File Structure

```
onnxruntime-bailey/
│
├── 📖 Documentation (Start Here!)
│   ├── WASI_INDEX.md                  ← You are here
│   ├── README_WASI.md                 ← Complete guide
│   ├── WASI_QUICK_REFERENCE.md        ← Quick start
│   ├── WASI_SDK_MIGRATION.md          ← Technical details
│   ├── WASI_ARCHITECTURE.md           ← Architecture
│   └── WASI_CHANGES_SUMMARY.md        ← Changes list
│
├── 🔧 Build Scripts
│   ├── verify_wasi_setup.sh           ← Verify before build
│   ├── build_wasi_simple.sh           ← Simple build
│   └── build_wasi.sh                  ← Detailed build
│
└── ⚙️ Configuration
    └── cmake/
        ├── wasi-sdk.cmake             ← Toolchain file
        ├── onnxruntime_webassembly.cmake ← WASI config
        └── CMakeLists.txt             ← Main CMake
```

## 🎯 Common Tasks

### Task: First Time Setup
1. Read [README_WASI.md § Prerequisites](README_WASI.md#prerequisites)
2. Download and install WASI-SDK
3. Set `WASI_SDK_PATH` environment variable
4. Run `./verify_wasi_setup.sh`
5. Run `./build_wasi_simple.sh`

### Task: Quick Build
```bash
export WASI_SDK_PATH=/path/to/wasi-sdk
./build_wasi_simple.sh
```

### Task: Debug Build
```bash
./build_wasi_simple.sh -DCMAKE_BUILD_TYPE=Debug
```

### Task: Custom Build
See [README_WASI.md § Build Options](README_WASI.md#build-options)

### Task: Run the Output
See [README_WASI.md § Running WASI Binaries](README_WASI.md#running-wasi-binaries)

### Task: Integrate in My App
See [README_WASI.md § Integration Examples](README_WASI.md#integration-examples)

### Task: Troubleshooting
See [README_WASI.md § Troubleshooting](README_WASI.md#troubleshooting)

## 🆚 WASI vs Emscripten

| Use Case | Recommended Build |
|----------|------------------|
| Server-side inference | ✅ WASI-SDK |
| Cloud functions | ✅ WASI-SDK |
| Edge computing | ✅ WASI-SDK |
| Portable applications | ✅ WASI-SDK |
| Browser with WebGPU | ⚠️ Emscripten |
| Browser with JSEP | ⚠️ Emscripten |
| Traditional web apps | ⚠️ Emscripten |

For detailed comparison, see [WASI_ARCHITECTURE.md § Feature Comparison](WASI_ARCHITECTURE.md#feature-comparison-matrix)

## 🔍 Finding Information

### "How do I install WASI-SDK?"
→ [README_WASI.md § Prerequisites](README_WASI.md#prerequisites)

### "What build options are available?"
→ [README_WASI.md § Build Options](README_WASI.md#build-options)
→ [WASI_QUICK_REFERENCE.md § Common Build Options](WASI_QUICK_REFERENCE.md#common-build-options)

### "How do I run the .wasm file?"
→ [README_WASI.md § Running WASI Binaries](README_WASI.md#running-wasi-binaries)

### "What changed from Emscripten?"
→ [WASI_SDK_MIGRATION.md § Key Changes](WASI_SDK_MIGRATION.md#key-changes)
→ [WASI_CHANGES_SUMMARY.md](WASI_CHANGES_SUMMARY.md)

### "Why is my build failing?"
→ [README_WASI.md § Troubleshooting](README_WASI.md#troubleshooting)
→ Run `./verify_wasi_setup.sh`

### "How does the build system work?"
→ [WASI_ARCHITECTURE.md](WASI_ARCHITECTURE.md)

### "What files were modified?"
→ [WASI_CHANGES_SUMMARY.md § Modified Files](WASI_CHANGES_SUMMARY.md#modified-files)

### "How do I integrate this in Node.js?"
→ [README_WASI.md § In Node.js](README_WASI.md#in-nodejs)

### "How do I integrate this in Python?"
→ [README_WASI.md § Python with wasmtime](README_WASI.md#python-with-wasmtime)

### "Can I use this in a browser?"
→ [README_WASI.md § In Browser](README_WASI.md#in-browser)

### "What's not supported?"
→ [WASI_SDK_MIGRATION.md § Not Supported](WASI_SDK_MIGRATION.md#not-supported-with-wasi-sdk)
→ [README_WASI.md § Features Not Supported](README_WASI.md#features-not-supported-in-wasi)

## 🐛 Troubleshooting Quick Links

| Problem | Solution |
|---------|----------|
| WASI_SDK_PATH not set | [Quick Reference § Troubleshooting](WASI_QUICK_REFERENCE.md#troubleshooting-quick-fixes) |
| Build fails | [README § Troubleshooting § Build Issues](README_WASI.md#build-issues) |
| Runtime crashes | [README § Troubleshooting § Runtime Issues](README_WASI.md#runtime-issues) |
| Slow performance | [README § Troubleshooting § Performance Issues](README_WASI.md#performance-issues) |
| Unknown import error | [README § Troubleshooting § Runtime Issues](README_WASI.md#runtime-issues) |

## 📊 Build Process Flow

```
Setup → Verify → Build → Test → Integrate
  ↓       ↓        ↓       ↓       ↓
[ENV]  [verify] [script] [run]  [app]
```

For detailed flow, see [WASI_ARCHITECTURE.md § Build Flow Diagram](WASI_ARCHITECTURE.md#build-flow-diagram)

## 🎓 Learning Path

### Beginner (Just want to build)
1. [WASI_QUICK_REFERENCE.md](WASI_QUICK_REFERENCE.md)
2. Run `./verify_wasi_setup.sh`
3. Run `./build_wasi_simple.sh`
4. Done! 🎉

### Intermediate (Want to customize)
1. [README_WASI.md § Build Options](README_WASI.md#build-options)
2. [README_WASI.md § Build Variants](README_WASI.md#build-variants)
3. Experiment with different flags

### Advanced (Want to integrate)
1. [README_WASI.md § Integration Examples](README_WASI.md#integration-examples)
2. [WASI_ARCHITECTURE.md § Integration Patterns](WASI_ARCHITECTURE.md#integration-patterns)
3. Build your application

### Expert (Want to modify)
1. [WASI_SDK_MIGRATION.md](WASI_SDK_MIGRATION.md)
2. [WASI_ARCHITECTURE.md](WASI_ARCHITECTURE.md)
3. [WASI_CHANGES_SUMMARY.md](WASI_CHANGES_SUMMARY.md)
4. Study the CMake files

## 🔗 External Resources

- **WASI**: https://wasi.dev/
- **WASI-SDK**: https://github.com/WebAssembly/wasi-sdk
- **Wasmtime**: https://wasmtime.dev/
- **Wasmer**: https://wasmer.io/
- **ONNXRuntime**: https://onnxruntime.ai/

## 💡 Tips

- Always run `./verify_wasi_setup.sh` before building
- Use `build_wasi_simple.sh` for most cases
- Keep WASI_SDK_PATH in your `~/.bashrc`
- Check documentation updates in newer WASI-SDK versions
- Test with wasmtime first (most compatible)

## 🤝 Contributing

If you improve the WASI build:
1. Update relevant documentation
2. Test with `verify_wasi_setup.sh`
3. Update [WASI_CHANGES_SUMMARY.md](WASI_CHANGES_SUMMARY.md)
4. Submit a pull request

## ⚖️ License

ONNXRuntime is licensed under the MIT License.

## 📮 Getting Help

1. Check the [Troubleshooting](README_WASI.md#troubleshooting) section
2. Run `./verify_wasi_setup.sh` for diagnostics
3. Review [Common Issues](README_WASI.md#troubleshooting)
4. Check ONNXRuntime GitHub issues
5. Ask on ONNXRuntime forums/discussions

---

**Quick Start**: `export WASI_SDK_PATH=/path/to/wasi-sdk && ./verify_wasi_setup.sh && ./build_wasi_simple.sh`

**Questions?** Start with [README_WASI.md](README_WASI.md) or [WASI_QUICK_REFERENCE.md](WASI_QUICK_REFERENCE.md)
