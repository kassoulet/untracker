# Untracker - Render Stems from Tracker Modules

```
               __
  __ __  _____/  |_____________    ____ |  | __ ___________
 |  |  \/    \   __\_  __ \__  \ _/ ___\|  |/ // __ \_  __ \
 |  |  /   |  \  |  |  | \// __ \\  \___|    <\  ___/|  | \/
 |____/|___|  /__|  |__|  (____  /\___  >__|_ \\___  >__|
            \/                 \/     \/     \/    \/
```


This program renders individual stems from tracker module files (MOD, XM, IT, S3M, etc.) using libopenmpt.

## Dependencies

- libopenmpt (OpenMPT C++ library)
- libsndfile (For audio file I/O)
- FLAC development libraries (optional, for FLAC support)
- Vorbis development libraries (optional, for OGG/Vorbis support)
- C++17 compatible compiler
- Meson build system and Ninja (for building with Meson/Ninja)

On Ubuntu/Debian systems, install dependencies with:
```bash
sudo apt-get install libopenmpt-dev libsndfile1-dev libflac-dev libvorbis-dev meson ninja-build
```

On other distributions, use the equivalent package manager.

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
./build/untracker -i <input_module_file> -o <output_directory>
```

Example:
```bash
./build/untracker -i song.xm -o ./stems/
```

Advanced usage with options:
```bash
# Extract with custom resampling
./build/untracker -i song.xm -o ./stems/ --resample sinc --sample-rate 48000

# Extract in FLAC format with 24-bit depth
./build/untracker -i song.xm -o ./stems/ --format flac --bit-depth 24

# Extract in OGG Vorbis format with high quality
./build/untracker -i song.xm -o ./stems/ --format vorbis --vorbis-quality 9

# Extract in OGG Opus format with custom bitrate
./build/untracker -i song.xm -o ./stems/ --format opus --opus-bitrate 192
```

### Available Options:
- `-i INPUT_FILE`: Input module file (required)
- `-o OUTPUT_DIR`: Output directory (required)
- `--sample-rate RATE`: Sample rate (default: 44100)
- `--channels NUM`: Number of channels (default: 2)
- `--resample METHOD`: Resampling method: nearest, linear, cubic, sinc (default: sinc)
- `--format FORMAT`: Output format: wav, flac, ogg, vorbis, opus (default: wav)
- `--bit-depth DEPTH`: Bit depth for lossless formats (16 or 24, default: 16)
- `--opus-bitrate BITRATE`: Bitrate for Opus format in kbps (default: 128)
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
