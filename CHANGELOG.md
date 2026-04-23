# Changelog

All notable changes to the c9o-mach project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## 1.0.0 (2026-04-23)


### Features

* add best features from all sibling repositories ([0b136d4](https://github.com/cogpy/c9o-mach/commit/0b136d4d8fbd44ea6768a4540f1736cc888ec81f))
* add best features from all sibling repositories ([ff3ed8a](https://github.com/cogpy/c9o-mach/commit/ff3ed8aec006141e6af68eccbb8f73a8e97d7960))


### Bug Fixes

* Add missing mach/time_value.h include to kern/printf.h ([11b946e](https://github.com/cogpy/c9o-mach/commit/11b946e81e57351068ae625f2267f009acb3dd95))
* Remove uint64_t redefinition, use standard &lt;stdint.h&gt; ([ddae334](https://github.com/cogpy/c9o-mach/commit/ddae334343b85a137b1ac99cefcbd791481863ef))
* Replace conflicting typedef with conditional includes in lttng.h ([0ff9118](https://github.com/cogpy/c9o-mach/commit/0ff91187d8f158e01c2038ff5407e1b117ea30f3))
* Resolve conflicting typedef of uint64_t in include/mach/lttng.h ([c39739b](https://github.com/cogpy/c9o-mach/commit/c39739ba545afadf55615d92621add1d2d73162c))
* **scripts,ci:** correct build/test working directories and force-build semantics ([b02a5f1](https://github.com/cogpy/c9o-mach/commit/b02a5f13373b85295b7e689da89a5c84582dbc77))

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
