# Module Untracker

This program extracts individual stems from tracker module files (MOD, XM, IT, S3M, etc.) using libopenmpt. It works by soloing each instrument/sample and rendering the audio separately.

## Overview

The program uses libopenmpt's advanced features to:
1. Load module files in various tracker formats
2. Access individual instruments/samples through the interactive extension
3. Mute/unmute instruments programmatically
4. Render each instrument separately to create isolated stems
5. Apply customizable audio processing (bass boost, resampling)
6. Export to multiple audio formats (WAV, FLAC, OGG/Vorbis)

## Dependencies

- libopenmpt (OpenMPT C++ library) - Version 0.7.3 or later
- libsndfile (For audio file I/O)
- FLAC development libraries (optional, for FLAC support)
- Vorbis development libraries (optional, for OGG/Vorbis support)
- C++17 compatible compiler
- Meson build system and Ninja (for building with Meson/Ninja)

On Ubuntu/Debian systems, install dependencies with:
```bash
sudo apt-get install libopenmpt-dev libsndfile1-dev libflac-dev libvorbis-dev meson ninja-build
```

## Building

### Using Make (with Meson/Ninja):
```bash
make
```

### Using Meson/Ninja directly:
```bash
meson build
ninja -C build
```

## Usage

Basic usage:
```bash
./build/stem_extractor -i <input_module_file> -o <output_directory>
```

Example:
```bash
./build/stem_extractor -i song.xm -o ./stems/
```

Advanced usage with options:
```bash
# Extract with bass boost and custom resampling
./build/stem_extractor -i song.xm -o ./stems/ --bass-boost 200 --resample sinc --sample-rate 48000

# Extract in FLAC format with 24-bit depth
./build/stem_extractor -i song.xm -o ./stems/ --format flac --bit-depth 24

# Extract in OGG Vorbis format with high quality
./build/stem_extractor -i song.xm -o ./stems/ --format vorbis --vorbis-quality 9
```

### Available Options:
- `-i INPUT_FILE`: Input module file (required)
- `-o OUTPUT_DIR`: Output directory (required)
- `--sample-rate RATE`: Sample rate (default: 44100)
- `--channels NUM`: Number of channels (default: 2)
- `--bass-boost MB`: Bass boost in millibels (default: 0)
- `--resample METHOD`: Resampling method: nearest, linear, cubic, sinc (default: sinc)
- `--format FORMAT`: Output format: wav, flac, ogg, vorbis (default: wav)
- `--bit-depth DEPTH`: Bit depth for lossless formats (16 or 24, default: 16)
- `--vorbis-quality LEVEL`: Vorbis quality level (0-10, default: 5)

## Supported Formats

### Input Formats
The program supports all formats supported by libopenmpt, including:
- MOD (Amiga ProTracker)
- XM (FastTracker II)
- IT (Impulse Tracker)
- S3M (Scream Tracker 3)
- FAR (Farandole Composer)
- MED (OctaMED)
- MTM (MultiTracker Module)
- STM (Scream Tracker Music Interface Kit)
- Ultimate Soundtracker MOD files
- And many others

### Output Formats
- WAV (default) - Lossless, supports 16/24-bit
- FLAC - Lossless compression, supports 16/24-bit
- OGG/Vorbis - Lossy compression with variable quality settings
- OGG/Opus - Lossy compression with variable bitrate settings

## How It Works

The program:

1. **Loads the module**: Uses libopenmpt's `module_ext` class to load the module file
2. **Identifies instruments**: Queries the module for the number of instruments and their names
3. **Accesses interactive controls**: Uses the interactive extension to control instrument muting
4. **Applies audio processing**: Configures bass boost and resampling filters
5. **Solo processing**: For each instrument:
   - Mutes all instruments
   - Unmutes only the target instrument
   - Renders the entire module sequence to audio
   - Saves the result in the selected format
6. **Cleanup**: Restores all instruments to unmuted state

## Technical Details

### Audio Processing Options
- **Sample Rate**: Configurable (default: 44.1 kHz)
- **Channels**: Configurable (default: Stereo/2 channels)
- **Buffer Size**: Configurable (default: 4096 samples per chunk)
- **Bass Boost**: Adjustable in millibels (default: 0)
- **Resampling**: Multiple methods available:
  - Nearest neighbor (fastest, lowest quality)
  - Linear interpolation
  - Cubic interpolation
  - Sinc interpolation (8-tap, highest quality, default)
- **Bit Depth**: 16 or 24-bit for lossless formats

### Instrument Detection Strategy
The program attempts multiple strategies to detect instruments/samples:
1. First, it tries to get the number of instruments using `get_num_instruments()`
2. If no instruments are found, it attempts to get sample information
3. As a fallback, it uses the number of channels

### Filename Sanitization
Instrument names are sanitized to create valid filenames:
- Special characters (`< > : " / \ | ? *`) are replaced with underscores
- Spaces are replaced with underscores

## API Usage

The program leverages several libopenmpt features:

- `openmpt::module_ext`: Extended module class with additional functionality
- `openmpt::ext::interactive`: Interface for real-time control of playback
- `set_instrument_mute_status()`: Mutes/unmutes specific instruments
- `get_instrument_names()`: Retrieves instrument names from the module
- `read_interleaved_stereo()`: Reads stereo audio data from the module
- `set_render_param()`: Sets rendering parameters (gain, interpolation, etc.)

## Build System

The project now uses Meson/Ninja build system:
- `meson build` - Initializes the build directory
- `ninja -C build` - Builds the project
- `ninja -C build install` - Installs the executable

## Limitations

- Some older module formats may not support instrument-level muting
- Very large modules may consume significant memory during processing
- The quality of extracted stems depends on how instruments are arranged in the original module

## Troubleshooting

If the program fails to extract stems:
1. Verify the module file is valid and playable
2. Check that the file format supports instruments (some formats only have samples)
3. Ensure sufficient disk space for output files
4. Check that libopenmpt and required audio libraries are properly installed
5. Run with `--help` to verify command-line options are correct

## License

This program is provided as-is for educational purposes. The underlying libopenmpt library is released under the BSD license.