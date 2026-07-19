# Cognitive Primitives Summary

Integration of the CoGWXP-OS9 cognitive layer with c9o-mach, at the scope
that is executable and falsifiable today.

## What Landed

| Deliverable | Path |
|---|---|
| Fixed-point PLN truth-value formulas | `tests/include/cognitive/cogpln.h` |
| Minimal atomspace with Mach wire format | `tests/include/cognitive/cogspace.h` |
| Deterministic QEMU boot test | `tests/test-cognitive.c` |
| Validation script | `scripts/validate-cognitive-primitives.sh` |
| Integration architecture and graduation path | `docs/cogwxp-integration.md` |

## Commands

```
# Boot the cognitive test in QEMU (from a configured build directory)
make run-cognitive

# Full validation: compile, boot, consistency checks
./scripts/validate-cognitive-primitives.sh build-i686
```

## Verified Behavior

The boot test asserts exact integers on bare Mach: an atomspace arena
obtained from `vm_allocate`; PLN deduction over the Socrates syllogism
yielding strength exactly 7800; revision yielding exactly 6450 with
confidence 476; one atom packed into a `mach_msg`, sent and received on a
Mach port, and verified field for field; attention ranking and decay with
exact outcomes.

## Honest Status

This is a primitives proof on bare Mach, not OpenCog on Mach. There is no
pattern matcher, no inference chaining at scale, no persistence, no 9P
endpoint, and no agent system. Zero kernel source files were changed. The
Phase 3 graduation (a `cognitive` MIG subsystem, number 5400 reserved) is
specified in `docs/cogwxp-integration.md` and is unstarted. CoGWXP-OS9
remains a read-only reference corpus; no code was copied from it.
