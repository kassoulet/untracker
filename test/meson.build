# Test suite for stem extractor

# Find dependencies (same as main project)
sndfile_dep = dependency('sndfile', version: '>=1.0.0')

test_main = executable('test_main', 'test.cpp',
  dependencies: [sndfile_dep],
  link_args: ['-std=c++17'],
  install: false
)

# Define tests
# The test requires a module file, so we'll just build it
# The user needs to run it manually with a module file