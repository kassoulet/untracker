/*
BSD 3-Clause License

Copyright (c) 2026, Gautier Portet

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <libopenmpt/libopenmpt.hpp>
#include <libopenmpt/libopenmpt_ext.hpp>
#include <map>
#include <memory>
#include <sndfile.hh>
#include <string>
#include <vector>

struct AudioOptions {
  int sample_rate = 44100;
  int channels = 2;             // Stereo (will be adjusted to 1 if stereo separation is 0)
  int interpolation_filter = 4; // cubic interpolation (sinc-like)
  int stereo_separation =
      100; // stereo separation in percent [0,200], default 100
  std::string output_format = "wav"; // wav, flac, opus, vorbis
  int bit_depth = 16;                // for lossless formats
  int opus_bitrate = 128;            // kbps for opus
  int vorbis_quality = 5;            // 0-10 for vorbis
};

class StemExtractor {
private:
  std::unique_ptr<openmpt::module_ext> mod;
  std::string input_path;
  AudioOptions options;

public:
  explicit StemExtractor(const std::string &path, const AudioOptions &opts = {})
      : input_path(path), options(opts) {
    std::ifstream file(input_path, std::ios::binary);
    if (!file.is_open()) {
      throw std::runtime_error("Could not open input file: " + path);
    }

    // Load the module using module_ext for advanced features
    mod = std::make_unique<openmpt::module_ext>(file);

    // Adjust channels based on stereo separation: if 0, use mono
    AudioOptions adjusted_opts = options;
    if (adjusted_opts.stereo_separation == 0) {
      adjusted_opts.channels = 1;  // Mono when stereo separation is 0
    }

    // Set up audio parameters
    mod->set_render_param(openmpt::module::RENDER_INTERPOLATIONFILTER_LENGTH,
                          adjusted_opts.interpolation_filter);
    mod->set_render_param(openmpt::module::RENDER_STEREOSEPARATION_PERCENT,
                          adjusted_opts.stereo_separation);
    
    // Update the stored options with adjusted values
    const_cast<AudioOptions&>(options) = adjusted_opts;
  }

  void extractStems(const std::string &output_dir) {
    // Get the number of instruments
    int num_instruments = mod->get_num_instruments();
    std::cout << "Found " << num_instruments << " instruments." << std::endl;

    // If no instruments, try using samples instead
    bool using_samples = false;
    if (num_instruments == 0) {
      // Some formats use samples instead of instruments
      // We'll need to determine the number of samples differently
      std::cout << "No instruments found, checking for samples..." << std::endl;

      // For formats like MOD, we should use the number of samples instead
      int num_samples = mod->get_num_samples();
      if (num_samples > 0) {
        num_instruments = num_samples;
        using_samples = true;
        std::cout << "Using " << num_instruments
                  << " samples instead of instruments." << std::endl;
      } else {
        // If get_num_samples() doesn't work, try to get it from the module type
        std::string mod_type = mod->get_metadata("type");
        if (mod_type.find("MOD") != std::string::npos) {
          // For MOD files, we'll try to get the number of samples by checking
          // pattern data This is a more complex approach but should work for
          // most MOD files We'll try to check if there are samples by looking
          // at the module structure For now, let's try a different approach -
          // check if we can get sample info through the interactive interface
          num_instruments = 31; // Standard MOD files have up to 31 samples
          using_samples = true;
          std::cout << "Assuming MOD format with up to " << num_instruments
                    << " samples." << std::endl;
        } else {
          // Fallback to channel count if all else fails
          num_instruments = mod->get_num_channels();
          std::cout << "Falling back to " << num_instruments << " channels."
                    << std::endl;
        }
      }
    }

    std::vector<std::string> names;

    // Get instrument names or sample names depending on the file type
    if (using_samples) {
      try {
        // Use the proper libopenmpt API to get all sample names at once
        names = mod->get_sample_names();
      } catch (...) {
        // If getting sample names fails, initialize with empty strings
        names.assign(num_instruments, "");
      }
    } else {
      // For formats that use instruments, get instrument names normally
      names = mod->get_instrument_names();
    }

    openmpt::ext::interactive *interactive =
        static_cast<openmpt::ext::interactive *>(
            mod->get_interface(openmpt::ext::interactive_id));

    if (!interactive) {
      std::cerr << "Interactive interface not available, cannot extract stems."
                << std::endl;
      return;
    }

    // Mute all instruments/samples initially (once)
    for (int i = 0; i < num_instruments; ++i) {
      try {
        interactive->set_instrument_mute_status(i, true);
      } catch (const std::exception &e) {
        std::cout << "Warning: Could not mute instrument/sample " << i << ": "
                  << e.what() << std::endl;
      }
    }

    // Extract module name without extension (once)
    std::string module_name =
        input_path.substr(input_path.find_last_of("/\\") + 1);
    size_t dot_pos = module_name.find_last_of(".");
    if (dot_pos != std::string::npos) {
      module_name.resize(dot_pos);
    }

    // Create module-specific output directory (once)
    std::string module_output_dir =
        output_dir + "/" + sanitize_filename(module_name);
    std::filesystem::create_directories(module_output_dir);

    // Reuse audio buffer to avoid repeated allocations
    // Buffer size increased for better rendering throughput
    const int BUFFER_SIZE = 65536;
    std::vector<float> buffer(BUFFER_SIZE * options.channels);

    for (int idx = 0; idx < num_instruments; ++idx) {
      // Determine the name for this instrument/sample/channel
      std::string name;
      if (static_cast<size_t>(idx) < names.size() && !names[idx].empty()) {
        name = names[idx];
      } else {
        if (using_samples) {
          name = "sample_" + std::to_string(idx + 1);
        } else {
          name = "instrument_" + std::to_string(idx + 1);
        }
      }

      std::cout << "Processing " << (using_samples ? "sample" : "instrument")
                << " " << idx << ": " << name << std::endl;

      // Unmute only the current instrument/sample
      try {
        interactive->set_instrument_mute_status(idx, false);
      } catch (const std::exception &e) {
        std::cout << "Warning: Could not unmute instrument/sample " << idx
                  << ": " << e.what() << std::endl;
      }

      // Reset playback position
      mod->set_position_seconds(0.0);

      // Create output filename in format:
      // {module_output_dir}/{instrument_number}-{instrument_name}.{format}
      // Format instrument number with leading zeros (001, 002, etc.)
      std::string instrument_number =
          "000" +
          std::to_string(idx + 1); // +1 to start from 001 instead of 000
      instrument_number =
          instrument_number.substr(instrument_number.length() - 3);

      std::string output_filename;
      if (!name.empty()) {
        output_filename = module_output_dir + "/" + instrument_number + "-" +
                          sanitize_filename(name) + "." + options.output_format;
      } else {
        output_filename = module_output_dir + "/" + instrument_number + "." +
                          options.output_format;
      }

      // Open output sound file with appropriate format
      SF_INFO sf_info;
      sf_info.samplerate = options.sample_rate;
      sf_info.channels = options.channels;

      // Set format based on user selection
      if (options.output_format == "wav") {
        sf_info.format =
            SF_FORMAT_WAV |
            (options.bit_depth == 16 ? SF_FORMAT_PCM_16 : SF_FORMAT_PCM_24);
      } else if (options.output_format == "flac") {
        sf_info.format =
            SF_FORMAT_FLAC |
            (options.bit_depth == 16 ? SF_FORMAT_PCM_16 : SF_FORMAT_PCM_24);
      } else if (options.output_format == "vorbis") {
        sf_info.format = SF_FORMAT_OGG | SF_FORMAT_VORBIS;
      } else if (options.output_format == "opus") {
        sf_info.format = SF_FORMAT_OGG | SF_FORMAT_OPUS;
      } else {
        // Default to WAV if format is not recognized
        sf_info.format =
            SF_FORMAT_WAV |
            (options.bit_depth == 16 ? SF_FORMAT_PCM_16 : SF_FORMAT_PCM_24);
        std::cout << "Unknown format '" << options.output_format
                  << "', defaulting to WAV." << std::endl;
      }

      bool has_any_audio = false;

      // First pass: check for audio with interpolation disabled (faster)
      mod->set_render_param(openmpt::module::RENDER_INTERPOLATIONFILTER_LENGTH,
                            1); // Nearest neighbor (no interpolation)
      mod->set_position_seconds(0.0);

      while (true) {
        std::vector<float> check_buffer(BUFFER_SIZE * options.channels);
        int samples_read =
            (options.channels == 1)
                ? mod->read(options.sample_rate, BUFFER_SIZE,
                            check_buffer.data())
            : (options.channels == 2)
                ? mod->read_interleaved_stereo(options.sample_rate, BUFFER_SIZE,
                                               check_buffer.data())
                : mod->read_interleaved_quad(options.sample_rate, BUFFER_SIZE,
                                             check_buffer.data());

        if (samples_read == 0) {
          break;
        }

        // Check if this buffer contains any non-silent samples
        for (int i = 0; i < samples_read * options.channels; ++i) {
          if (check_buffer[i] != 0.0f) { // Check for exact zero
            has_any_audio = true;
            break; // Exit early once we find audio
          }
        }

        if (has_any_audio) {
          break; // Stop checking once we know there's audio
        }

        double current_pos = mod->get_position_seconds();
        double duration = mod->get_duration_seconds();
        if (current_pos >= duration * 0.99) { // Allow slight tolerance
          break;
        }
      }

      // Restore the original interpolation setting
      mod->set_render_param(openmpt::module::RENDER_INTERPOLATIONFILTER_LENGTH,
                            options.interpolation_filter);

      if (!has_any_audio) {
        std::cout << "Skipping silent stem: " << output_filename << std::endl;
        // Mute back the current instrument/sample before continuing
        try {
          interactive->set_instrument_mute_status(idx, true);
        } catch (...) {
        }
        continue; // Skip this instrument/sample if it produces no audio
      }

      // Second pass: render with proper interpolation since we know there's
      // audio
      mod->set_position_seconds(0.0);

      // Only create the output file if we know there's audio to write
      SNDFILE *outfile = sf_open(output_filename.c_str(), SFM_WRITE, &sf_info);
      if (!outfile) {
        std::cerr << "Could not create output file: " << output_filename
                  << " - " << sf_strerror(nullptr) << std::endl;
        // Mute back the current instrument/sample before continuing
        try {
          interactive->set_instrument_mute_status(idx, true);
        } catch (...) {
        }
        continue;
      }

      while (true) {
        std::vector<float> write_buffer(BUFFER_SIZE * options.channels);
        int samples_read =
            (options.channels == 1)
                ? mod->read(options.sample_rate, BUFFER_SIZE,
                            write_buffer.data())
            : (options.channels == 2)
                ? mod->read_interleaved_stereo(options.sample_rate, BUFFER_SIZE,
                                               write_buffer.data())
                : mod->read_interleaved_quad(options.sample_rate, BUFFER_SIZE,
                                             write_buffer.data());

        if (samples_read == 0) {
          break;
        }

        // Write to output file
        sf_count_t frames_written =
            sf_writef_float(outfile, write_buffer.data(), samples_read);
        if (frames_written != samples_read) {
          std::cerr << "Error writing to output file: " << sf_strerror(outfile)
                    << std::endl;
          sf_close(outfile);
          std::filesystem::remove(output_filename);
          break;
        }
      }

      sf_close(outfile);
      std::cout << "Extracted stem: " << output_filename << std::endl;

      // Mute back the current instrument/sample for the next iteration
      try {
        interactive->set_instrument_mute_status(idx, true);
      } catch (...) {
      }
    }
  }

private:
  std::string sanitize_filename(const std::string &name) {
    if (name.empty()) {
      return "unknown";
    }
    std::string sanitized = name;
    for (char &c : sanitized) {
      if (c == '<' || c == '>' || c == ':' || c == '"' || c == '/' ||
          c == '\\' || c == '|' || c == '?' || c == '*') {
        c = '_';
      }
      // Also replace spaces with underscores for cleaner filenames
      if (c == ' ') {
        c = '_';
      }
    }
    // Prevent path traversal
    if (sanitized == "." || sanitized == "..") {
      return "_";
    }
    return sanitized;
  }
};

// Helper function to parse command line arguments
AudioOptions parseArguments(int argc, const char *const argv[],
                            std::string &input_file, std::string &output_dir) {
  AudioOptions opts;

  // Parse arguments
  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];

    if (arg == "-i" && i + 1 < argc) {
      input_file = argv[++i];
    } else if (arg == "-o" && i + 1 < argc) {
      output_dir = argv[++i];
    } else if (arg == "--sample-rate" && i + 1 < argc) {
      opts.sample_rate = std::stoi(argv[++i]);
      if (opts.sample_rate < 8000 || opts.sample_rate > 192000) {
        throw std::runtime_error("Invalid sample rate: " +
                                 std::to_string(opts.sample_rate));
      }
    } else if (arg == "--channels" && i + 1 < argc) {
      opts.channels = std::stoi(argv[++i]);
      if (opts.channels != 1 && opts.channels != 2 && opts.channels != 4) {
        throw std::runtime_error(
            "Invalid channels: " + std::to_string(opts.channels) +
            " (only 1, 2, 4 supported)");
      }
    } else if (arg == "--resample" && i + 1 < argc) {
      std::string resample_method = argv[++i];
      if (resample_method == "linear")
        opts.interpolation_filter = 2;
      else if (resample_method == "cubic")
        opts.interpolation_filter = 4;
      else if (resample_method == "sinc" || resample_method == "8tap")
        opts.interpolation_filter = 8;
      else if (resample_method == "nearest")
        opts.interpolation_filter = 1;
      else {
        std::cout << "Unknown resampling method: " << resample_method
                  << ", using sinc (8-tap)" << std::endl;
        opts.interpolation_filter = 8; // Default to sinc
      }
    } else if (arg == "--format" && i + 1 < argc) {
      opts.output_format = argv[++i];
    } else if (arg == "--bit-depth" && i + 1 < argc) {
      opts.bit_depth = std::stoi(argv[++i]);
      if (opts.bit_depth != 16 && opts.bit_depth != 24) {
        throw std::runtime_error(
            "Invalid bit depth: " + std::to_string(opts.bit_depth) +
            " (only 16, 24 supported)");
      }
    } else if (arg == "--opus-bitrate" && i + 1 < argc) {
      opts.opus_bitrate = std::stoi(argv[++i]);
      if (opts.opus_bitrate < 16 || opts.opus_bitrate > 512) {
        throw std::runtime_error(
            "Invalid opus bitrate: " + std::to_string(opts.opus_bitrate) +
            " (16-512 supported)");
      }
    } else if (arg == "--vorbis-quality" && i + 1 < argc) {
      opts.vorbis_quality = std::stoi(argv[++i]);
      if (opts.vorbis_quality < 0 || opts.vorbis_quality > 10) {
        throw std::runtime_error(
            "Invalid vorbis quality: " + std::to_string(opts.vorbis_quality) +
            " (0-10 supported)");
      }
    } else if (arg == "--stereo-separation" && i + 1 < argc) {
      opts.stereo_separation = std::stoi(argv[++i]);
      if (opts.stereo_separation < 0 || opts.stereo_separation > 200) {
        throw std::runtime_error("Invalid stereo separation: " +
                                 std::to_string(opts.stereo_separation) +
                                 " (0-200 supported)");
      }
    } else if (arg == "--help") {
      std::cout << "Usage: " << argv[0] << " [OPTIONS]\n";
      std::cout << "Options:\n";
      std::cout
          << "  -i INPUT_FILE              Input module file (required)\n";
      std::cout << "  -o OUTPUT_DIR              Output directory (required)\n";
      std::cout
          << "  --sample-rate RATE         Sample rate (default: 44100)\n";
      std::cout
          << "  --channels NUM             Number of channels (default: 2)\n";
      std::cout << "  --resample METHOD          Resampling method: nearest, "
                   "linear, cubic, sinc (default: sinc)\n";
      std::cout << "  --format FORMAT            Output format: wav, flac, "
                   "vorbis, opus (default: wav)\n";
      std::cout << "  --bit-depth DEPTH          Bit depth for lossless "
                   "formats (16 or 24, default: 16)\n";
      std::cout << "  --vorbis-quality LEVEL     Vorbis quality level (0-10, "
                   "default: 5)\n";
      std::cout << "  --stereo-separation PERCENT Stereo separation in percent "
                   "(0-200, default: 100)\n";
      std::cout << "  --help                     Show this help\n";
      std::cout << "\nSupported input formats: MOD, XM, IT, S3M, and other "
                   "tracker formats supported by libopenmpt\n";
      std::cout << "Supported output formats: WAV, FLAC, Vorbis, Opus\n";
      exit(0);
    }
  }

  // Set default sample rate to 48000 for opus format if not explicitly set by user
  if (opts.output_format == "opus" && opts.sample_rate == 44100) {
    opts.sample_rate = 48000;  // Opus default sample rate
  }

  return opts;
}

int main(int argc, char *argv[]) {
  try {
    std::string input_file, output_dir;
    AudioOptions opts = parseArguments(argc, argv, input_file, output_dir);

    if (input_file.empty() || output_dir.empty()) {
      std::cerr << "Usage: " << argv[0]
                << " -i <input_module_file> -o <output_directory> [OPTIONS]"
                << std::endl;
      std::cerr << "Run with --help for full options list." << std::endl;
      return 1;
    }

    StemExtractor extractor(input_file, opts);
    extractor.extractStems(output_dir);
    std::cout << "Stem extraction completed successfully!" << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}