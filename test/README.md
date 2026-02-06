# Untracker Tests

This directory contains unit and integration tests for the untracker program.

## Test Suite

### test_main
- Performs full integration tests with actual module files
- Tests all extraction features
- Verifies different output formats
- Tests audio processing options

## Running Tests

### Integration Tests
```bash
# With a test module file
./build/test/test_main <path_to_module_file>

# Example:
./build/test/test_main test_module.xm
```

## Test Module

For integration tests, you need a module file (XM, MOD, IT, etc.). Place the module file in this directory or provide the path when running the integration test.

## Expected Results

After running the integration tests, you should see:
- Multiple stem files extracted in the output directory
- Different formats tested (WAV, FLAC)
- Audio processing options verified