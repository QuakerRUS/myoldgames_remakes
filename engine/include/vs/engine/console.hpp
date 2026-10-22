#pragma once

namespace vs {

// Our programs are WIN32 (GUI) executables without a console. Command-line modes
// (--roundtrip, --selftest, ...) call this first: when stdout is not redirected to a file or a
// pipe, stdout/stderr are attached to the console of the parent process (cmd, PowerShell,
// Git Bash) so printf / std::cout output is visible. No-op elsewhere or when redirected.
void attach_parent_console();

}  // namespace vs
