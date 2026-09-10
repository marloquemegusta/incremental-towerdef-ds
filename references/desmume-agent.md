# Bundled DeSmuME agent

The skill owns the headless DeSmuME runtime, pinned source, GDB overlay,
scenario runner, input contract, and evidence format. Projects provide only
their own ROMs and scenarios.

Use these entry points from the skill directory:

```powershell
.\scripts\build-runtime.ps1
.\scripts\run-scenario.ps1 -RomPath <rom> -ScenarioPath <scenario> -OutputPath <artifacts>
.\scripts\validate-gdb.ps1 -RomPath <rom>
```

Each scenario output contains a manifest, event log, emulator log, and declared
PNG captures. Read the manifest first, then correlate events and captures with
the hypothesis under test. A changed screen or hash is not a correctness claim
without a targeted assertion.

The bundled runtime is headless and audio-disabled. It validates deterministic
input, state transitions, traces, memory/GDB diagnostics, and visual evidence;
it does not validate physical audio, DSi timing, or hardware compatibility.
