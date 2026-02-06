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
  int channels = 2; // Stereo
  int interpolation_filter = 4;      // cubic interpolation (sinc-like)
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

    // Set up audio parameters
    mod->set_render_param(openmpt::module::RENDER_INTERPOLATIONFILTER_LENGTH,
                          options.interpolation_filter);
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
      // For MOD files and other formats that use samples, get sample names
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

      // Cast to interactive interface to access muting functions
      openmpt::ext::interactive *interactive =
          static_cast<openmpt::ext::interactive *>(
              mod->get_interface(openmpt::ext::interactive_id));

      if (!interactive) {
        std::cerr
            << "Interactive interface not available, cannot extract stems."
            << std::endl;
        return;
      }

      // Mute all instruments/samples initially
      // For MOD files and other formats that use samples, we'll try to mute
      // each instrument/sample Note: Some formats like classic MOD may not
      // support individual sample muting
      for (int i = 0; i < num_instruments; ++i) {
        try {
          interactive->set_instrument_mute_status(i, true);
        } catch (const std::exception &e) {
          // If muting fails for this index, it might be that the format doesn't
          // support individual instrument muting. For MOD files, we might need
          // to handle this differently.
          std::cout << "Warning: Could not mute instrument/sample " << i << ": "
                    << e.what() << std::endl;
          // Continue anyway - we'll try to extract what we can
        }
      }

      // Unmute only the current instrument/sample
      try {
        interactive->set_instrument_mute_status(idx, false);
      } catch (const std::exception &e) {
        std::cout << "Warning: Could not unmute instrument/sample " << idx
                  << ": " << e.what() << std::endl;
        // Continue anyway - we'll try to extract what we can
      }

      // Reset playback position
      mod->set_position_seconds(0.0);

      // Extract module name without extension
      std::string module_name =
          input_path.substr(input_path.find_last_of("/\\") + 1);
      size_t dot_pos = module_name.find_last_of(".");
      if (dot_pos != std::string::npos) {
        module_name.resize(dot_pos);
      }

      // Create module-specific output directory
      std::string module_output_dir =
          output_dir + "/" + sanitize_filename(module_name);
      std::filesystem::create_directories(module_output_dir);

      // Create output filename in format:
      // {module_output_dir}/{instrument_number}-{instrument_name}.{format}
      // Format instrument number with leading zeros (001, 002, etc.)
      std::string instrument_number =
          "000" +
          std::to_string(idx + 1); // +1 to start from 001 instead of 000
      // Take only the last 3 digits to ensure format like 001, 002, ..., 999
      instrument_number =
          instrument_number.substr(instrument_number.length() - 3);

      // Only include the instrument name if it's not empty
      std::string output_filename;
      if (!name.empty()) {
        output_filename = module_output_dir + "/" + instrument_number + "-" +
                          sanitize_filename(name) + "." + options.output_format;
      } else {
        // If no name, just use the number
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
      } else if (options.output_format == "ogg" ||
                 options.output_format == "vorbis") {
        sf_info.format = SF_FORMAT_OGG | SF_FORMAT_VORBIS;
      } else {
        // Default to WAV if format is not recognized
        sf_info.format =
            SF_FORMAT_WAV |
            (options.bit_depth == 16 ? SF_FORMAT_PCM_16 : SF_FORMAT_PCM_24);
        std::cout << "Unknown format '" << options.output_format
                  << "', defaulting to WAV." << std::endl;
      }

      // First, check if this instrument/sample produces any audio
      // by rendering the entire module to see if there's any non-zero data
      const int BUFFER_SIZE = 4096;
      std::vector<float> preview_buffer(BUFFER_SIZE * options.channels);
      bool has_any_audio = false;

      // Temporarily reset position to check for audio
      mod->set_position_seconds(0.0);

      while (true) {
        int samples_read = mod->read_interleaved_stereo(
            options.sample_rate, BUFFER_SIZE, preview_buffer.data());

        if (samples_read == 0) {
          break;
        }

        // Check if this buffer contains any non-silent samples
        for (int i = 0; i < samples_read * options.channels; ++i) {
          if (std::abs(preview_buffer[i]) >
              1e-9f) { // Using small epsilon instead of exact zero
            has_any_audio = true;
            break; // Found audio, no need to continue checking this buffer
          }
        }

        if (has_any_audio) {
          break; // Found audio, no need to continue checking the entire module
        }

        // Check if we've reached the end of the song
        double current_pos = mod->get_position_seconds();
        double duration = mod->get_duration_seconds();
        if (current_pos >= duration * 0.99) { // Allow slight tolerance
          break;
        }
      }

      // Reset position back to beginning for actual rendering
      mod->set_position_seconds(0.0);

      if (!has_any_audio) {
        std::cout << "Skipping silent stem: " << output_filename << std::endl;
        continue; // Skip this instrument/sample if it produces no audio
      }

      // Only create the output file if we know there's audio to write
      SNDFILE *outfile = sf_open(output_filename.c_str(), SFM_WRITE, &sf_info);
      if (!outfile) {
        std::cerr << "Could not create output file: " << output_filename
                  << " - " << sf_strerror(nullptr) << std::endl;
        continue;
      }

      // Only create the output file if we know there's audio to write
      // Render the module with only this instrument active
      std::vector<float> buffer(BUFFER_SIZE * options.channels);

      while (true) {
        int samples_read = mod->read_interleaved_stereo(
            options.sample_rate, BUFFER_SIZE, buffer.data());

        if (samples_read == 0) {
          break;
        }

        // Write to output file
        sf_count_t frames_written =
            sf_writef_float(outfile, buffer.data(), samples_read);
        if (frames_written != samples_read) {
          std::cerr << "Error writing to output file: " << sf_strerror(outfile)
                    << std::endl;
          break;
        }

        // Check if we've reached the end of the song
        double current_pos = mod->get_position_seconds();
        double duration = mod->get_duration_seconds();
        if (current_pos >= duration * 0.99) { // Allow slight tolerance
          break;
        }
      }

      sf_close(outfile);
      std::cout << "Extracted stem: " << output_filename << std::endl;
    }

    // Restore all instruments to unmuted state
    openmpt::ext::interactive *interactive =
        static_cast<openmpt::ext::interactive *>(
            mod->get_interface(openmpt::ext::interactive_id));

    if (interactive) {
      for (int i = 0; i < num_instruments; ++i) {
        interactive->set_instrument_mute_status(i, false);
      }
    }
  }

private:
  std::string sanitize_filename(const std::string &name) {
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
    return sanitized;
  }
};

// Helper function to parse command line arguments
AudioOptions parseArguments(int argc, char * const argv[], std::string &input_file,
                            std::string &output_dir) {
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
    } else if (arg == "--channels" && i + 1 < argc) {
      opts.channels = std::stoi(argv[++i]);
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
    } else if (arg == "--opus-bitrate" && i + 1 < argc) {
      opts.opus_bitrate = std::stoi(argv[++i]);
    } else if (arg == "--vorbis-quality" && i + 1 < argc) {
      opts.vorbis_quality = std::stoi(argv[++i]);
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
                   "ogg, vorbis (default: wav)\n";
      std::cout << "  --bit-depth DEPTH          Bit depth for lossless "
                   "formats (16 or 24, default: 16)\n";
      std::cout << "  --vorbis-quality LEVEL     Vorbis quality level (0-10, "
                   "default: 5)\n";
      std::cout << "  --help                     Show this help\n";
      std::cout << "\nSupported input formats: MOD, XM, IT, S3M, and other "
                   "tracker formats supported by libopenmpt\n";
      std::cout << "Supported output formats: WAV, FLAC, OGG/Vorbis\n";
      std::cout << "Note: Opus format is not directly supported by libsndfile, "
                   "use OGG/Vorbis instead.\n";
      exit(0);
    }
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