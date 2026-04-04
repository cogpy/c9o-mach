// Commitlint configuration
// Inspired by: marduk-ml-sdk's conventional commit enforcement
//
// Enforces conventional commit format for automated changelog generation.
// Format: type(scope): description
//
// Types: feat, fix, perf, refactor, docs, ci, test, build, chore
// Scopes: kern, vm, ipc, device, i386, x86_64, aarch64, build, ci, docs

module.exports = {
  extends: ['@commitlint/config-conventional'],
  rules: {
    'type-enum': [
      2,
      'always',
      [
        'feat',     // New feature
        'fix',      // Bug fix
        'perf',     // Performance improvement
        'refactor', // Code refactoring
        'docs',     // Documentation
        'ci',       // CI/CD changes
        'test',     // Test changes
        'build',    // Build system changes
        'chore',    // Maintenance
        'revert',   // Revert previous commit
      ],
    ],
    'scope-enum': [
      1, // Warning only - scopes are suggested but not required
      'always',
      [
        'kern',     // Kernel core
        'vm',       // Virtual memory
        'ipc',      // Inter-process communication
        'device',   // Device drivers
        'i386',     // i386 architecture
        'x86_64',   // x86_64 architecture
        'aarch64',  // ARM64 architecture
        'build',    // Build system
        'ci',       // CI/CD pipeline
        'docs',     // Documentation
        'tests',    // Test infrastructure
        'ddb',      // Kernel debugger
        'linux',    // Linux compatibility layer
      ],
    ],
    'subject-max-length': [2, 'always', 72],
    'body-max-line-length': [1, 'always', 100],
  },
};
