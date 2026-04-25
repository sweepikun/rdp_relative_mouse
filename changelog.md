### Version 0.3

+ Fixed first-move jump issue by properly initializing `lastX`/`lastY` on the first real mouse event.
+ Replaced coarse `LLMHF_INJECTED` check with precise `dwExtraInfo` signature matching to avoid accidentally blocking legitimate input.
+ Added giant-delta filtering to ignore cursor warps caused by games (e.g. Minecraft resetting the cursor to screen center).
+ Added `--debug` / `-d` command-line argument to both executables for easier troubleshooting.
+ Added GitHub Actions workflow for automated Windows builds and releases.
+ Documented Minecraft compatibility: Java Edition requires disabling **Raw Input** in mouse settings.

### Version 0.2

+ completely reimplemented using Windows hook; no longer depends on Interception library.
+ almost native experience
