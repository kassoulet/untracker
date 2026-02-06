project('untracker', 'cpp',
  version: '1.0',
  meson_version: '>=0.50.0',
  default_options: ['cpp_std=c++17'])

# Find dependencies
openmpt_dep = dependency('libopenmpt', version: '>=0.4')
sndfile_dep = dependency('sndfile', version: '>=1.0.0')

# Additional audio format dependencies
flac_dep = dependency('flac', required: false)
vorbisfile_dep = dependency('vorbisfile', required: false)

# Define executable
untracker = executable('untracker', 'untracker.cpp',
  dependencies: [openmpt_dep, sndfile_dep],
  link_args: ['-lstdc++fs'],  # Link filesystem library
  install: true
)

# Create a config header to check for optional dependencies
config_data = configuration_data()
config_data.set('HAVE_SNDFILE', true)
if flac_dep.found()
  config_data.set('HAVE_FLAC', true)
  message('FLAC support enabled')
else
  message('FLAC support not available')
endif
if vorbisfile_dep.found()
  config_data.set('HAVE_VORBIS', true)
  message('Vorbis support enabled')
else
  message('Vorbis support not available')
endif

# Write the config header
configure_file(
  output: 'config.h',
  configuration: config_data
)

# Add subdirectory for tests
subdir('test')