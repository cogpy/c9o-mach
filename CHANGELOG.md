# Changelog

All notable changes to the c9o-mach project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- SECURITY.md with vulnerability reporting policy (inspired by cogpwsh)
- GOVERNANCE.md with project governance structure (inspired by cogpwsh)
- Pull request template with phase-based checklist (inspired by firedrake)
- Enhanced CI/CD with dual-compiler testing, Valgrind, and SLSA3 provenance
  (inspired by diod, cogfernos, llama2.c)
- Claude AI-powered code review workflow (inspired by marduk-ml-sdk)
- Coverity Scan static analysis workflow (inspired by conman)
- Release-please automated versioning (inspired by marduk-ml-sdk)
- Commitlint for conventional commit enforcement (inspired by marduk-ml-sdk)
- Doxygen API documentation generation (inspired by lustre)
- Stack analysis script for kernel stack depth checking (inspired by lustre)
- Phased test execution script (inspired by cogpwsh, cogfernos)
- Valgrind memory testing script for CI (inspired by diod)
- QEMU boot configuration examples (inspired by guile-daemon-zero)
- Kernel module development examples (inspired by guile-daemon-zero)
- This CHANGELOG.md file (inspired by marduk-ml-sdk, cogpwsh)

### Changed
- Enhanced CI/CD pipeline with dual compiler (gcc + clang) matrix
- Expanded build matrix to include minimal and debug configurations

## [1.8] - Previous Release

### Features
- GNU Mach microkernel with IPC, VM, scheduling, and device support
- Multi-architecture support: i386, x86_64, aarch64
- LTTng-style kernel tracing
- Enhanced GDB stub for debugging
- Performance analysis framework
- QEMU-based test infrastructure
- Static analysis integration (cppcheck, clang-analyzer)
- Comprehensive documentation (35+ files)
