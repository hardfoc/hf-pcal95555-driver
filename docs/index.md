# HF-PCAL95555 Documentation

Welcome! This directory contains step-by-step guides for installing, building, and using the **HF-PCAL95555** library -- a hardware-agnostic C++ driver for the **PCA9555** and **PCAL9555A** 16-bit I/O expanders.

The driver auto-detects which chip variant is connected and enables features accordingly. Standard PCA9555 features work on both chips; PCAL9555A-only features (pull resistors, drive strength, input latch, interrupt mask/status, output mode) degrade gracefully on a PCA9555.

## Documentation Structure

### **Getting Started**

1. **[🛠️ Installation](installation.md)** – How to integrate the driver into your project
2. **[⚡ Quick Start](quickstart.md)** – Minimal working example to get you running
3. **[🔌 Hardware Setup](hardware_setup.md)** – Wiring diagrams and pin connections

### **Integration**

4. **[🔧 Platform Integration](platform_integration.md)** – Implement the CRTP I2C interface for your platform
5. **[⚙️ Configuration](configuration.md)** – Configuration options and Kconfig settings

### **Reference**

6. **[📖 API Reference](api_reference.md)** – Complete API documentation (43 methods)
7. **[💡 Examples](examples.md)** – Detailed example walkthroughs (comprehensive test + LED animation)

### **Troubleshooting**

8. **[🐛 Troubleshooting](troubleshooting.md)** – Common issues and solutions

---

## Quick Start Path

**New to PCAL95555?** Follow this recommended path:

1. Start with **[Installation](installation.md)** to integrate the driver
2. Follow **[Hardware Setup](hardware_setup.md)** to wire your I/O expander
3. Read **[Quick Start](quickstart.md)** for a minimal working example
4. Check **[Platform Integration](platform_integration.md)** to implement the I2C interface
5. Explore **[Examples](examples.md)** for more advanced usage

---

## Need Help?

- **🐛 Found a bug?** Check the [Troubleshooting](troubleshooting.md) guide
- **❓ Have questions?** Review the [API Reference](api_reference.md)
- **📝 Want to contribute?** See the contributing guidelines in the main README

---

**Navigation**
➡️ [Installation](installation.md)

