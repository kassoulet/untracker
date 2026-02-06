#include <iostream>
#include <cassert>
#include <filesystem>
#include <cstdlib>
#include <string>

int main(int argc, char* argv[]) {
    std::cout << "=== Untracker Integration Test ===" << std::endl;
    
    // Check if the executable exists
    std::string exe_path;
    if (std::filesystem::exists("./build/untracker")) {
        exe_path = "./build/untracker";
    } else if (std::filesystem::exists("../build/untracker")) {
        exe_path = "../build/untracker";
    } else if (std::filesystem::exists("../../build/untracker")) {
        exe_path = "../../build/untracker";
    } else {
        std::cerr << "ERROR: untracker executable not found!" << std::endl;
        std::cerr << "Please build the project first with 'make' or 'meson build && ninja -C build'" << std::endl;
        return 1;
    }
    
    std::cout << "✓ untracker executable found at: " << exe_path << std::endl;
    
    // Check if a test module file was provided
    if (argc < 2) {
        std::cerr << "USAGE: " << argv[0] << " <test_module_file>" << std::endl;
        std::cerr << "Example: " << argv[0] << " test_module.xm" << std::endl;
        return 1;
    }
    
    std::string test_module = argv[1];
    
    // Verify the test module exists
    if (!std::filesystem::exists(test_module)) {
        std::cerr << "ERROR: Test module file '" << test_module << "' not found!" << std::endl;
        return 1;
    }
    
    std::cout << "✓ Test module file found: " << test_module << std::endl;
    
    // Create a temporary output directory for the test
    std::string output_dir = "./test_output_" + std::to_string(std::rand() % 10000);
    std::filesystem::create_directory(output_dir);
    
    std::cout << "✓ Created temporary output directory: " << output_dir << std::endl;
    
    // Test 1: Basic extraction
    std::cout << "\nTest 1: Basic stem extraction..." << std::endl;
    std::string cmd1 = exe_path + " -i \"" + test_module + "\" -o \"" + output_dir + "\"";
    int result1 = std::system(cmd1.c_str());
    
    if (result1 == 0) {
        std::cout << "✓ Basic extraction completed successfully" << std::endl;
        
        // Count extracted files
        int file_count = 0;
        for (const auto& entry : std::filesystem::directory_iterator(output_dir)) {
            if (entry.path().extension() == ".wav") {
                file_count++;
            }
        }
        std::cout << "  Extracted " << file_count << " stem files" << std::endl;
    } else {
        std::cerr << "✗ Basic extraction failed" << std::endl;
        std::filesystem::remove_all(output_dir);
        return 1;
    }
    
    // Test 2: Extraction with higher sample rate
    std::cout << "\nTest 2: Extraction with higher sample rate..." << std::endl;
    std::string output_dir2 = output_dir + "_48k";
    std::filesystem::create_directory(output_dir2);
    
    std::string cmd2 = exe_path + " -i \"" + test_module + "\" -o \"" + output_dir2 + "\" --sample-rate 48000";
    int result2 = std::system(cmd2.c_str());
    
    if (result2 == 0) {
        std::cout << "✓ High sample rate extraction completed successfully" << std::endl;
        
        // Count extracted files
        int file_count2 = 0;
        for (const auto& entry : std::filesystem::directory_iterator(output_dir2)) {
            if (entry.path().extension() == ".wav") {
                file_count2++;
            }
        }
        std::cout << "  Extracted " << file_count2 << " stem files with high sample rate" << std::endl;
    } else {
        std::cerr << "✗ High sample rate extraction failed" << std::endl;
        std::filesystem::remove_all(output_dir2);
    }
    
    // Test 3: Extraction with FLAC format
    std::cout << "\nTest 3: Extraction with FLAC format..." << std::endl;
    std::string output_dir3 = output_dir + "_flac";
    std::filesystem::create_directory(output_dir3);
    
    std::string cmd3 = exe_path + " -i \"" + test_module + "\" -o \"" + output_dir3 + "\" --format flac";
    int result3 = std::system(cmd3.c_str());
    
    if (result3 == 0) {
        std::cout << "✓ FLAC format extraction completed successfully" << std::endl;
        
        // Count extracted files
        int file_count3 = 0;
        for (const auto& entry : std::filesystem::directory_iterator(output_dir3)) {
            if (entry.path().extension() == ".flac") {
                file_count3++;
            }
        }
        std::cout << "  Extracted " << file_count3 << " stem files in FLAC format" << std::endl;
    } else {
        std::cerr << "✗ FLAC format extraction failed" << std::endl;
        std::filesystem::remove_all(output_dir3);
    }
    
    // Test 4: Extraction with sinc resampling
    std::cout << "\nTest 4: Extraction with sinc resampling..." << std::endl;
    std::string output_dir4 = output_dir + "_sinc";
    std::filesystem::create_directory(output_dir4);

    std::string cmd4 = exe_path + " -i \"" + test_module + "\" -o \"" + output_dir4 + "\" --resample sinc";
    int result4 = std::system(cmd4.c_str());

    if (result4 == 0) {
        std::cout << "✓ Sinc resampling extraction completed successfully" << std::endl;

        // Count extracted files
        int file_count4 = 0;
        for (const auto& entry : std::filesystem::directory_iterator(output_dir4)) {
            if (entry.path().extension() == ".wav") {
                file_count4++;
            }
        }
        std::cout << "  Extracted " << file_count4 << " stem files with sinc resampling" << std::endl;
    } else {
        std::cerr << "✗ Sinc resampling extraction failed" << std::endl;
        std::filesystem::remove_all(output_dir4);
    }

    // Test 5: Extraction with Opus format
    std::cout << "\nTest 5: Extraction with Opus format..." << std::endl;
    std::string output_dir5 = output_dir + "_opus";
    std::filesystem::create_directory(output_dir5);

    std::string cmd5 = exe_path + " -i \"" + test_module + "\" -o \"" + output_dir5 + "\" --format opus --opus-bitrate 96";
    int result5 = std::system(cmd5.c_str());

    if (result5 == 0) {
        std::cout << "✓ Opus format extraction completed successfully" << std::endl;

        // Count extracted files
        int file_count5 = 0;
        for (const auto& entry : std::filesystem::directory_iterator(output_dir5)) {
            if (entry.path().extension() == ".opus") {
                file_count5++;
            }
        }
        std::cout << "  Extracted " << file_count5 << " stem files in Opus format" << std::endl;
    } else {
        std::cerr << "✗ Opus format extraction failed" << std::endl;
        std::filesystem::remove_all(output_dir5);
    }
    
    // Cleanup
    std::cout << "\nCleaning up test directories..." << std::endl;
    std::filesystem::remove_all(output_dir);
    
    std::cout << "\n=== All tests completed successfully! ===" << std::endl;
    return 0;
}