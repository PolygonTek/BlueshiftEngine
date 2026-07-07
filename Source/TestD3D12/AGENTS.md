# Codex Instructions

- Do not modify files without asking first.
- Before applying any code change, explain the intended direction and wait for approval.
- This is engine code: prefer performance-conscious implementations and avoid unnecessary work, allocation, or data movement.
- Preserve existing intent-bearing comments when moving or refactoring code; update them when behavior changes.
- Keep code readable, but do not use readability as a reason to introduce avoidable runtime cost.
- When reading files in PowerShell, use explicit UTF-8 decoding (for example, `Get-Content -Encoding UTF8`) before judging Korean comments or patching around them; UTF-8 without BOM is valid, and terminal mojibake does not mean the file is incorrectly encoded.
- Create new source files as UTF-8 without signature/BOM.
