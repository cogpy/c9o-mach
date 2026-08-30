# Boot and Kernel Modules

The boot process loads the Mach kernel and its modules, establishes bootstrap ports, and starts the initial user task.

## Boot Flow (Mermaid)
```mermaid
flowchart TD
  A[Bootloader] --> B[Load Mach kernel + modules]
  B --> C[Transfer control to kernel]
  C --> D[Initialize VM, IPC, devices, scheduler]
  D --> E[Create bootstrap ports]
  E --> F[Launch bootstrap task]
  F --> G[User space servers start (e.g., Hurd)]
```

## Modules
- Loadable modules provide drivers and kernel extensions.
- Configuration and compilation affect which drivers are in-kernel vs user-space.
- A multiboot module string is a line of the boot script: the first word names the program to run and the rest becomes its argument vector.
- A line whose first word starts with `#` is a comment, which is how data modules such as the root file system image on the c9o-mach install media are loaded without being executed, see `packaging-architecture.md`.

## Bootstrap
- Bootstrap task receives privileged ports to locate/start core servers.
- Syscall emulation support may be provided for compatibility layers.

## References
- Bootstrap: `https://www.gnu.org/software/hurd/gnumach-doc/Bootstrap.html`
- Bootloader/Modules: `https://www.gnu.org/software/hurd/gnumach-doc/Bootloader.html`, `https://www.gnu.org/software/hurd/gnumach-doc/Modules.html`
- Installing/Compilation/Configuration: `https://www.gnu.org/software/hurd/gnumach-doc/Installing.html`, `https://www.gnu.org/software/hurd/gnumach-doc/Compilation.html`, `https://www.gnu.org/software/hurd/gnumach-doc/Configuration.html`, `https://www.gnu.org/software/hurd/gnumach-doc/Cross_002dCompilation.html`
