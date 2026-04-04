# Security Policy

## Supported Versions

| Version | Supported          |
| ------- | ------------------ |
| master  | :white_check_mark: |
| develop | :white_check_mark: |
| < 1.8   | :x:                |

## Reporting a Vulnerability

The GNU Mach (c9o-mach) project takes security seriously. If you discover a
security vulnerability, please report it responsibly.

### How to Report

1. **Do NOT open a public GitHub issue** for security vulnerabilities.
2. Email your report to the project maintainers with the subject line:
   `[SECURITY] c9o-mach vulnerability report`
3. Include the following in your report:
   - Description of the vulnerability
   - Steps to reproduce the issue
   - Potential impact assessment
   - Suggested fix (if any)

### What to Expect

- **Acknowledgment**: Within 48 hours of your report.
- **Assessment**: We will evaluate the severity and impact within 7 days.
- **Resolution**: Critical vulnerabilities will be patched within 30 days.
- **Disclosure**: We follow coordinated disclosure. We will work with you to
  determine an appropriate disclosure timeline.

### Scope

The following are in scope for security reports:

- **Kernel memory corruption** (buffer overflows, use-after-free, etc.)
- **IPC security bypasses** (unauthorized port access, message tampering)
- **Privilege escalation** via kernel interfaces
- **Virtual memory isolation failures**
- **Device driver vulnerabilities** in included drivers
- **Denial of service** via kernel resource exhaustion
- **Boot security issues** (secure boot bypass, module loading)

### Out of Scope

- Vulnerabilities in user-space programs not included in this repository
- Issues requiring physical access to the machine
- Social engineering attacks
- Denial of service via network flooding (network stack is in user space)

## Security Best Practices for Contributors

1. **Memory Safety**: Always validate buffer sizes and use bounds-checked
   operations. Use `copyinmsg`/`copyoutmsg` for user-kernel data transfer.
2. **Input Validation**: Validate all inputs from user space at the system
   call boundary.
3. **Integer Overflow**: Check for integer overflow in size calculations,
   especially in `vm_allocate` and IPC message handling.
4. **Locking Discipline**: Follow the established lock ordering to prevent
   deadlocks. Document lock requirements in function headers.
5. **Static Analysis**: Run `cppcheck` and `clang-analyzer` before submitting
   patches. The CI pipeline enforces this automatically.
6. **Stack Usage**: Keep kernel stack usage minimal. Use `scripts/checkstack.sh`
   to verify stack depth.

## Security Scanning

This project employs automated security scanning:

- **Static Analysis**: cppcheck and clang static analyzer run on every PR
- **Coverity Scan**: Periodic deep static analysis via Coverity
- **CI Security Job**: Automated scanning for common vulnerability patterns,
  hardcoded credentials, and unsafe function usage
