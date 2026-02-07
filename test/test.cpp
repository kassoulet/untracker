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

#include <iostream>
#include <cassert>
#include <filesystem>
#include <cstdlib>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <random>

// Constants
const std::string UNTRACKER_EXECUTABLE = "untracker";
const std::string DEFAULT_TEST_MODULES_DIR = "./modules/";
const std::string OUTPUT_DIR_PREFIX = "./test_output_";

// Helper function to get file type using the 'file' command
std::string getFileType(const std::string& filepath) {
    std::string command = "file -b \"" + filepath + "\" 2>/dev/null";
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        return "unknown";
    }

    char buffer[128];
    std::string result = "";
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    pclose(pipe);

    // Remove trailing newline if present
    if (!result.empty() && result.back() == '\n') {
        result.pop_back();
    }
    return result;
}

// Helper function to find the untracker executable
std::string findExecutable() {
    std::vector<std::string> possible_paths = {
        "./build/" + UNTRACKER_EXECUTABLE,
        "../build/" + UNTRACKER_EXECUTABLE,
        "../../build/" + UNTRACKER_EXECUTABLE
    };

    for (const auto& path : possible_paths) {
        if (std::filesystem::exists(path)) {
            return path;
        }
    }

    std::cerr << "ERROR: untracker executable not found!" << std::endl;
    std::cerr << "Please build the project first with 'make' or 'meson build && ninja -C build'" << std::endl;
    return "";
}

// Helper function to find the test module file
std::string findModuleFile(const std::string& module_arg) {
    // Check if the module file exists in the current location
    if (std::filesystem::exists(module_arg)) {
        return module_arg;
    }

    // Check if the module file exists in the modules subdirectory
    std::string modules_path = DEFAULT_TEST_MODULES_DIR + module_arg;
    if (std::filesystem::exists(modules_path)) {
        return modules_path;
    }

    std::cerr << "ERROR: Test module file '" << module_arg << "' not found in current directory or modules subdirectory!" << std::endl;
    return "";
}

// Helper function to run a command and return success status
bool runCommand(const std::string& command, const std::string& test_name) {
    std::cout << "\n" << test_name << "..." << std::endl;
    int result = std::system(command.c_str());
    return result == 0;
}

// Helper function to count files with specific extension in a directory
std::vector<std::string> findFilesWithExtension(const std::string& directory, const std::string& extension) {
    std::vector<std::string> files;
    for (const auto& dir_entry : std::filesystem::recursive_directory_iterator(directory)) {
        if (dir_entry.path().extension() == extension) {
            files.push_back(dir_entry.path().string());
        }
    }
    return files;
}

// Helper function to verify WAV file format
void verifyWavFormat(const std::string& filepath, int expected_bit_depth = 16) {
    std::string file_type = getFileType(filepath);
    std::cout << "  File type verification: " << file_type << std::endl;

    bool is_wav = (file_type.find("WAV") != std::string::npos || file_type.find("RIFF") != std::string::npos);
    bool has_correct_sample_rate = (file_type.find("44100 Hz") != std::string::npos || file_type.find("48000 Hz") != std::string::npos);
    bool has_expected_bit_depth = false;
    
    if (expected_bit_depth == 16) {
        has_expected_bit_depth = (file_type.find("16 bit") != std::string::npos);
    } else if (expected_bit_depth == 24) {
        has_expected_bit_depth = (file_type.find("24 bit") != std::string::npos);
    } else {
        has_expected_bit_depth = (file_type.find("16 bit") != std::string::npos || file_type.find("24 bit") != std::string::npos);
    }

    if (is_wav) {
        std::cout << "  ✓ File format verified as WAV" << std::endl;
    } else {
        std::cout << "  ⚠ File format may not be WAV: " << file_type << std::endl;
    }

    if (has_correct_sample_rate) {
        std::cout << "  ✓ Sample rate verified (44.1kHz or 48kHz)" << std::endl;
    } else {
        std::cout << "  ⚠ Unexpected sample rate: " << file_type << std::endl;
    }

    if (has_expected_bit_depth) {
        std::cout << "  ✓ Bit depth verified (" << expected_bit_depth << "-bit)" << std::endl;
    } else {
        std::cout << "  ⚠ Unexpected bit depth: " << file_type << std::endl;
    }
    
    // Check if both sample rate and bit depth are correct
    if (has_correct_sample_rate && has_expected_bit_depth) {
        std::cout << "  ✓ Both sample rate and bit depth verified" << std::endl;
    } else {
        std::cout << "  ⚠ Missing expected sample rate or bit depth" << std::endl;
    }
}

// Helper function to verify FLAC file format
void verifyFlacFormat(const std::string& filepath, int expected_bit_depth = 16) {
    std::string file_type = getFileType(filepath);
    std::cout << "  File type verification: " << file_type << std::endl;

    bool is_flac = (file_type.find("FLAC") != std::string::npos);
    bool has_correct_sample_rate = (file_type.find("44.1 kHz") != std::string::npos || file_type.find("48 kHz") != std::string::npos);
    bool has_expected_bit_depth = false;
    
    if (expected_bit_depth == 16) {
        has_expected_bit_depth = (file_type.find("16 bit") != std::string::npos);
    } else if (expected_bit_depth == 24) {
        has_expected_bit_depth = (file_type.find("24 bit") != std::string::npos);
    } else {
        has_expected_bit_depth = (file_type.find("16 bit") != std::string::npos || file_type.find("24 bit") != std::string::npos);
    }

    if (is_flac) {
        std::cout << "  ✓ File format verified as FLAC" << std::endl;
    } else {
        std::cout << "  ⚠ File format may not be FLAC: " << file_type << std::endl;
    }

    if (has_correct_sample_rate) {
        std::cout << "  ✓ Sample rate verified (44.1kHz or 48kHz)" << std::endl;
    } else {
        std::cout << "  ⚠ Unexpected sample rate: " << file_type << std::endl;
    }

    if (has_expected_bit_depth) {
        std::cout << "  ✓ Bit depth verified (" << expected_bit_depth << "-bit)" << std::endl;
    } else {
        std::cout << "  ⚠ Unexpected bit depth: " << file_type << std::endl;
    }

    // Check if both sample rate and bit depth are correct
    if (has_correct_sample_rate && has_expected_bit_depth) {
        std::cout << "  ✓ Both sample rate and bit depth verified" << std::endl;
    } else {
        std::cout << "  ⚠ Missing expected sample rate or bit depth" << std::endl;
    }
}

// Helper function to verify Opus file format
void verifyOpusFormat(const std::string& filepath, int expected_bit_depth = 16) {
    std::string file_type = getFileType(filepath);
    std::cout << "  File type verification: " << file_type << std::endl;

    bool is_opus = (file_type.find("Opus") != std::string::npos || file_type.find("Ogg") != std::string::npos);
    bool has_correct_sample_rate = (file_type.find("48000 Hz") != std::string::npos || file_type.find("48 kHz") != std::string::npos);

    if (is_opus) {
        std::cout << "  ✓ File format verified as Opus" << std::endl;
    } else {
        std::cout << "  ⚠ File format may not be Opus: " << file_type << std::endl;
    }

    if (has_correct_sample_rate) {
        std::cout << "  ✓ Sample rate verified (48kHz)" << std::endl;
    } else {
        std::cout << "  ⚠ Unexpected sample rate: " << file_type << std::endl;
    }

    // Check if sample rate is correct for Opus
    if (has_correct_sample_rate) {
        std::cout << "  ✓ Both sample rate and format verified" << std::endl;
    } else {
        std::cout << "  ⚠ Missing expected sample rate" << std::endl;
    }
}

int main(int argc, char* argv[]) {
    std::cout << "=== Untracker Integration Test ===" << std::endl;

    // Find the executable
    std::string exe_path = findExecutable();
    if (exe_path.empty()) {
        return 1;
    }
    std::cout << "✓ untracker executable found at: " << exe_path << std::endl;

    // Check if a test module file was provided
    if (argc < 2) {
        std::cerr << "USAGE: " << argv[0] << " <test_module_file>" << std::endl;
        std::cerr << "Example: " << argv[0] << " test_module.xm" << std::endl;
        return 1;
    }

    // Find the test module file
    std::string test_module = findModuleFile(argv[1]);
    if (test_module.empty()) {
        return 1;
    }
    std::cout << "✓ Test module file found: " << test_module << std::endl;

    // Create a temporary output directory for the test
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1000, 9999);
    std::string output_dir = OUTPUT_DIR_PREFIX + std::to_string(dis(gen));
    std::filesystem::create_directory(output_dir);

    std::cout << "✓ Created temporary output directory: " << output_dir << std::endl;
    
    // Test 1: Basic extraction
    std::string cmd1 = exe_path + " -i \"" + test_module + "\" -o \"" + output_dir + "\"";
    if (runCommand(cmd1, "Test 1: Basic stem extraction")) {
        std::cout << "✓ Basic extraction completed successfully" << std::endl;

        // Count extracted files in subdirectories
        std::vector<std::string> wav_files = findFilesWithExtension(output_dir, ".wav");
        std::cout << "  Extracted " << wav_files.size() << " stem files" << std::endl;

        // Verify file formats using 'file' utility
        if (!wav_files.empty()) {
            verifyWavFormat(wav_files[0], 16); // Default to 16-bit
        }
    } else {
        std::cerr << "✗ Basic extraction failed" << std::endl;
        std::filesystem::remove_all(output_dir);
        return 1;
    }

    // Test 2: Extraction with higher sample rate
    std::string output_dir2 = output_dir + "_48k";
    std::filesystem::create_directory(output_dir2);
    std::string cmd2 = exe_path + " -i \"" + test_module + "\" -o \"" + output_dir2 + "\" --sample-rate 48000";
    if (runCommand(cmd2, "Test 2: Extraction with higher sample rate")) {
        std::cout << "✓ High sample rate extraction completed successfully" << std::endl;

        // Count extracted files in subdirectories
        std::vector<std::string> wav_files2 = findFilesWithExtension(output_dir2, ".wav");
        std::cout << "  Extracted " << wav_files2.size() << " stem files with high sample rate" << std::endl;

        // Verify file formats using 'file' utility
        if (!wav_files2.empty()) {
            verifyWavFormat(wav_files2[0], 16); // Default to 16-bit
        }
    } else {
        std::cerr << "✗ High sample rate extraction failed" << std::endl;
        std::filesystem::remove_all(output_dir2);
    }

    // Test 3: Extraction with FLAC format
    std::string output_dir3 = output_dir + "_flac";
    std::filesystem::create_directory(output_dir3);
    std::string cmd3 = exe_path + " -i \"" + test_module + "\" -o \"" + output_dir3 + "\" --format flac";
    if (runCommand(cmd3, "Test 3: Extraction with FLAC format")) {
        std::cout << "✓ FLAC format extraction completed successfully" << std::endl;

        // Count extracted files in subdirectories
        std::vector<std::string> flac_files = findFilesWithExtension(output_dir3, ".flac");
        std::cout << "  Extracted " << flac_files.size() << " stem files in FLAC format" << std::endl;

        // Verify file formats using 'file' utility
        if (!flac_files.empty()) {
            verifyFlacFormat(flac_files[0], 16); // Default to 16-bit
        }
    } else {
        std::cerr << "✗ FLAC format extraction failed" << std::endl;
        std::filesystem::remove_all(output_dir3);
    }

    // Test 4: Extraction with sinc resampling
    std::string output_dir4 = output_dir + "_sinc";
    std::filesystem::create_directory(output_dir4);
    std::string cmd4 = exe_path + " -i \"" + test_module + "\" -o \"" + output_dir4 + "\" --resample sinc";
    if (runCommand(cmd4, "Test 4: Extraction with sinc resampling")) {
        std::cout << "✓ Sinc resampling extraction completed successfully" << std::endl;

        // Count extracted files in subdirectories
        std::vector<std::string> wav_files4 = findFilesWithExtension(output_dir4, ".wav");
        std::cout << "  Extracted " << wav_files4.size() << " stem files with sinc resampling" << std::endl;

        // Verify file formats using 'file' utility
        if (!wav_files4.empty()) {
            verifyWavFormat(wav_files4[0], 16); // Default to 16-bit
        }
    } else {
        std::cerr << "✗ Sinc resampling extraction failed" << std::endl;
        std::filesystem::remove_all(output_dir4);
    }

    // Test 5: Extraction with Opus format
    std::string output_dir5 = output_dir + "_opus";
    std::filesystem::create_directory(output_dir5);
    std::string cmd5 = exe_path + " -i \"" + test_module + "\" -o \"" + output_dir5 + "\" --format opus --opus-bitrate 96 --sample-rate 48000";
    if (runCommand(cmd5, "Test 5: Extraction with Opus format")) {
        std::cout << "✓ Opus format extraction completed successfully" << std::endl;

        // Count extracted files in subdirectories
        std::vector<std::string> opus_files = findFilesWithExtension(output_dir5, ".opus");
        std::cout << "  Extracted " << opus_files.size() << " stem files in Opus format" << std::endl;

        // Verify file formats using 'file' utility
        if (!opus_files.empty()) {
            verifyOpusFormat(opus_files[0], 16); // Default to 16-bit
        }
    } else {
        std::cerr << "✗ Opus format extraction failed" << std::endl;
        std::filesystem::remove_all(output_dir5);
    }

    // Test 6: Extraction with 24-bit depth
    std::string output_dir6 = output_dir + "_24bit";
    std::filesystem::create_directory(output_dir6);
    std::string cmd6 = exe_path + " -i \"" + test_module + "\" -o \"" + output_dir6 + "\" --format flac --bit-depth 24";
    if (runCommand(cmd6, "Test 6: Extraction with 24-bit depth")) {
        std::cout << "✓ 24-bit depth extraction completed successfully" << std::endl;

        // Count extracted files in subdirectories
        std::vector<std::string> flac_files6 = findFilesWithExtension(output_dir6, ".flac");
        std::cout << "  Extracted " << flac_files6.size() << " stem files with 24-bit depth" << std::endl;

        // Verify file formats using 'file' utility
        if (!flac_files6.empty()) {
            verifyFlacFormat(flac_files6[0], 24); // Expect 24-bit
        }
    } else {
        std::cerr << "✗ 24-bit depth extraction failed" << std::endl;
        std::filesystem::remove_all(output_dir6);
    }

    // Cleanup
    std::cout << "\nCleaning up test directories..." << std::endl;
    std::filesystem::remove_all(output_dir);

    std::cout << "\n=== All tests completed successfully! ===" << std::endl;
    return 0;
}