# Contributing to OpenRF Platform

First of all, thank you for your interest in OpenRF Platform!

Whether you're reporting a bug, improving the documentation or contributing new RF protocol support, your help is greatly appreciated.

---

# Project Philosophy

OpenRF Platform is designed around a simple principle:

> **Keep the platform stable while allowing protocols to evolve independently.**

The RF gateway, REST API, MQTT interface and Home Assistant integration are considered the platform core. New protocol support should be implemented as independent decoder modules whenever possible.

---

# How You Can Help

Contributions are welcome in many forms, including:

- Bug reports
- Feature suggestions
- Documentation improvements
- Performance optimizations
- New RF protocol decoders
- Hardware compatibility improvements
- Testing and validation

---

# Reporting Bugs

When reporting an issue, please include as much information as possible:

- Hardware used
- Firmware version
- RF module
- Steps to reproduce
- Expected behavior
- Actual behavior
- Serial log (if available)
- Screenshots (if applicable)

---

# Coding Guidelines

Please try to keep contributions consistent with the existing project structure.

General guidelines:

- Keep code modular.
- Prefer readable code over clever code.
- Avoid breaking existing APIs.
- Maintain backward compatibility whenever possible.
- Keep platform-independent code separated from protocol-specific implementations.

---

# RF Protocol Modules

When adding support for a new RF protocol:

- Implement it as a separate decoder module.
- Avoid modifying the core RF infrastructure unless absolutely necessary.
- Keep protocol logic isolated from the gateway core.

The long-term goal is to allow new protocols to be added without affecting existing functionality.

---

# Pull Requests

Before submitting a Pull Request:

- Make sure the project builds successfully.
- Test your changes.
- Keep commits focused on a single feature or fix.
- Update documentation when necessary.

Small, focused Pull Requests are preferred over large changes.

---

# Discussions

If you are unsure about a feature or architectural change, please open an Issue or Discussion before starting major development.

---

# Thank You

Every contribution helps improve OpenRF Platform.

Whether it's a typo fix, a new protocol decoder or simply testing the firmware, your time and effort are sincerely appreciated.

Thank you for helping make OpenRF Platform better!