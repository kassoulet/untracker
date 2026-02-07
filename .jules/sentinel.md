## 2026-05-22 - [Buffer Overflow in Audio Rendering]
**Vulnerability:** A heap buffer overflow was found in `untracker.cpp` where `mod->read_interleaved_stereo` was called even if the user specified `--channels 1`. This caused 2 channels of audio to be written into a buffer allocated for only 1 channel.
**Learning:** Hardcoded stereo rendering functions should not be used when the output channel count is user-configurable.
**Prevention:** Always use channel-aware rendering functions (like `read_interleaved`) and validate that the output buffer size matches the requested channel count multiplied by the frame count.
