# Project Governance

## Overview

The c9o-mach project (GNU Mach microkernel) follows a phased development model
with clear roles and responsibilities. This document describes how the project
is governed, how decisions are made, and how contributors can participate.

## Roles

### Maintainers

Maintainers have full commit access and are responsible for:
- Reviewing and merging pull requests
- Managing releases and version tags
- Setting project direction and roadmap
- Resolving disputes in technical discussions
- Ensuring code quality and CI/CD health

### Contributors

Anyone who submits a pull request, files an issue, or participates in
discussions. Contributors are expected to:
- Follow the [Contributing Guidelines](CONTRIBUTING.md)
- Adhere to the [Security Policy](SECURITY.md)
- Respect the phased development model
- Write tests for new functionality
- Keep PRs focused and reasonably sized

### Reviewers

Experienced contributors who regularly review PRs. Reviewers provide:
- Technical feedback on code changes
- Architecture and design guidance
- Mentorship for new contributors

## Decision Making

### Technical Decisions

- **Phase 1-2 changes** (bug fixes, small improvements): A single maintainer
  approval is sufficient.
- **Phase 3 changes** (new features, architecture changes): Require discussion
  in an issue before implementation. Two maintainer approvals needed.
- **Phase 4 changes** (experimental R&D): Must include a design document in
  `docs/` and be discussed in an issue. Two maintainer approvals needed.

### Consensus Process

1. Proposals are made via GitHub Issues with the appropriate phase label.
2. Discussion period: minimum 3 days for Phase 3+, 1 day for Phase 1-2.
3. If consensus is reached, the proposer (or assignee) implements the change.
4. If consensus cannot be reached, maintainers make the final decision.

## Development Phases

The project follows a structured four-phase development model:

| Phase | Focus | Approval |
|-------|-------|----------|
| Phase 1 | Foundation & Quick Wins | 1 maintainer |
| Phase 2 | Subsystem Improvements | 1 maintainer + tests |
| Phase 3 | New Features & Architecture | 2 maintainers + design doc |
| Phase 4 | Experimental R&D | 2 maintainers + design doc + hypothesis |

## Release Process

1. Release candidates are tagged from the `develop` branch.
2. A release checklist is opened as an issue.
3. CI must pass on all architectures (i686, x86_64).
4. Static analysis must show zero critical findings.
5. At least one maintainer must approve the release.
6. Releases are tagged on `master` with semantic versioning.

## Code of Conduct

All participants are expected to be respectful and constructive. We follow the
[Contributor Covenant](https://www.contributor-covenant.org/) code of conduct.

## Amendments

This governance document can be amended by consensus of the maintainers. Major
changes should be discussed in an issue with a minimum 7-day discussion period.
