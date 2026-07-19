# GNU Mach Architecture Documentation

This documentation set synthesizes the official GNU Mach manual into a practical, systems-architecture-oriented guide, enriched with Mermaid and PlantUML diagrams.

- Audience: kernel and systems engineers, OS researchers, and contributors to GNU Hurd/Mach.
- Sources: Official GNU Mach documentation (see References below).

## Index
- Architecture Overview: `architecture-overview.md`
- Inter-Process Communication (IPC): `ipc.md`
- Virtual Memory: `virtual-memory.md`
- External Memory Management: `external-memory-management.md`
- Threads and Tasks: `threads-and-tasks.md`
- Scheduling and Processor Sets: `scheduling-and-processors.md`
- Device Interface: `device-interface.md`
- Boot and Kernel Modules: `boot-and-modules.md`
- Kernel Debugger and Profiling: `debugger-and-profiling.md`
- Host Interface: `host-interface.md`
- CoGWXP-OS9 Integration: `cogwxp-integration.md`

## Diagram Conventions
- Mermaid blocks are fenced as:
  ```mermaid
  graph TD
  A --> B
  ```
- PlantUML blocks are fenced as:
  ```plantuml
  @startuml
  A --> B
  @enduml
  ```

## References
- GNU Mach Manual: `https://www.gnu.org/software/hurd/gnumach-doc/index.html`
- Key sections used:
  - Introduction, Features, Overview, History
  - Installing, Bootstrap, Bootloader, Modules
  - Inter-Process Communication, Messaging Interface, Port Manipulation
  - Virtual Memory Interface, External Memory Management
  - Threads and Tasks, Scheduling, Processor/Processor Sets
  - Device Interface
  - Host Interface
  - Kernel Debugger, Profiling
