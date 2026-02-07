# Bolt's Journal - Critical Learnings

## 2026-03-05 - Initial Investigation
**Learning:** Tracker module rendering is a compute-intensive task that involves simulating a whole synthesizer engine. In `untracker.cpp`, the implementation was doing $O(N^2)$ muting operations and allocating buffers in every iteration, which significantly slowed down the stem extraction process.
**Action:** Move setup logic outside the loops and reuse buffers to minimize overhead.

## 2026-03-05 - Buffer Management & Loop Optimization
**Learning:** Increasing the buffer size from 4,096 to 65,536 for libopenmpt rendering significantly reduces the number of calls to the rendering engine and improves cache locality, leading to measurable performance gains. Reusing a single pre-allocated `std::vector<float>` instead of re-allocating it in every iteration of the rendering loop also noticeably reduced the CPU time spent in memory management.
**Action:** Always prefer large, reusable buffers for audio processing hot loops. Measured a ~50% reduction in User CPU time for standard modules.
