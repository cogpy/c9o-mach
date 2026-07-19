# Changelog

All notable changes to the c9o-mach project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.2](https://github.com/cogpy/c9o-mach/compare/v1.0.1...v1.0.2) (2026-04-26)


### Bug Fixes

* **ci:** fix 5 CI build bugs (pci macros, lock.h macros.h, elf32 linker, user-qemu.mk, format strings) ([23635ac](https://github.com/cogpy/c9o-mach/commit/23635ac05e6ead2c77da0128eb9afd2bc97972ec))
* **ci:** resolve 5 CI build failures (pci config macros, lock.h self-containment, elf32 linker, USER_TESTS list, format specifiers) ([08d91e7](https://github.com/cogpy/c9o-mach/commit/08d91e7b08d80e2a29dee0b4cee39f1e1be2f9cc))

## [1.0.1](https://github.com/cogpy/c9o-mach/compare/v1.0.0...v1.0.1) (2026-04-24)


### Bug Fixes

* **ci,mig:** build migcom with 32-bit TARGET_CC per target arch ([fb38813](https://github.com/cogpy/c9o-mach/commit/fb388135e33dfe6b3a9114b9d2c8bb2c4a0a5bd9))
* **ci,mig:** match migcom TARGET_CC to kernel arch, not user arch ([a0f9b89](https://github.com/cogpy/c9o-mach/commit/a0f9b8980f3533a619e6863ab473c19d61e58cb7))
* **ci:** run extended test loop from build-${matrix.arch} directory ([7c40264](https://github.com/cogpy/c9o-mach/commit/7c402646b3ab0347e2241d0795d2c3d7af14118d))
* **ci:** run extended test loop from build-${matrix.arch}; unblock MIG, tests, and latent build bugs ([15c09b3](https://github.com/cogpy/c9o-mach/commit/15c09b3c462410f78aa6145646c3baf021e6646a))
* **kern,linux:** resolve latent build/link errors unmasked by MIG fix ([4637006](https://github.com/cogpy/c9o-mach/commit/46370066d96b4035d89e206acc1a3c7dc75d5059))
* **linux/pci:** guard pci_find_capability against malformed cap chains ([84ac195](https://github.com/cogpy/c9o-mach/commit/84ac19548b5d38d7a3dfd4c45775705d74d06230))
* **linux:** restore pci_find_capability in pci.c; add &lt;linux/types.h&gt; ([d8f7f58](https://github.com/cogpy/c9o-mach/commit/d8f7f5883292a080c33dbb4db5b8efd24b0e8934))
* **tests,kern:** wire USER_MIG + dedupe testlib enum + drop kern include ([c62ddb3](https://github.com/cogpy/c9o-mach/commit/c62ddb36b7797f977759bb0b3e0ce9e4cfbbb610))
* **tests:** add -I$(srcdir) to TESTCFLAGS so kern/* internal headers resolve ([0444c78](https://github.com/cogpy/c9o-mach/commit/0444c783b609de6f2f40b11052a3b529f4c12d58))
* **tests:** drop -ftrivial-auto-var-init=pattern (GCC 12+ only) ([eee10c4](https://github.com/cogpy/c9o-mach/commit/eee10c4ad0861f841c298d70be2f8384b70ed21b))
* **tests:** extend TESTCFLAGS include path for kernel-internal headers ([63ee978](https://github.com/cogpy/c9o-mach/commit/63ee978e9860e5f195a53999498950f7d7e20bc9))
* **tests:** include i386 headers in TESTCFLAGS for x86_64 builds ([2d4dd30](https://github.com/cogpy/c9o-mach/commit/2d4dd300fc3dd39b1d7c7e6c80641a82ec276a73))

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
