/*
MIT License

Copyright(c) 2025 Xiaoyang Yu

Permission is hereby granted, free of charge, to any person obtaining a copy
of this softwareand associated documentation files(the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions :

The above copyright noticeand this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <string>
#include <vector>

// Use the nlohmann JSON library
#include "nlohmann/json.hpp"

namespace fs = std::filesystem;
using json = nlohmann::json;

#define ERROR_NO_MESH "Error: No mesh found in directory: "
#define ERROR_MULTIPLE_MESH "Error: Multiple mesh files found in directory: "

struct PathParams {
    fs::path data_dir;          // Directory containing preprocessed data
    fs::path source_dir;        // Directory containing data to be processed
    std::string source_name;    // Data source name (defaults to the directory name of source_dir)
    fs::path split_filename;    // JSON split file
};

// Get data source map filename stored under data_dir
fs::path get_data_source_map_filename(const fs::path& data_dir) {
    return data_dir / "data_source_map.json";
}

// Append data source information to the map file
void append_data_source_map(const fs::path& data_dir, const std::string& name,
    const fs::path& source) {
    fs::path map_filename = get_data_source_map_filename(data_dir);
    std::cout << "Data sources stored to " << map_filename << std::endl;
    json data_source_map;
    if (fs::exists(map_filename)) {
        std::ifstream ifs(map_filename);
        ifs >> data_source_map;
    }
    if (data_source_map.contains(name)) {
        if (data_source_map[name] != fs::absolute(source).string()) {
            throw std::runtime_error("Cannot add data with the same name and a different source.");
        }
    }
    else {
        data_source_map[name] = fs::absolute(source).string();
        std::ofstream ofs(map_filename);
        ofs << data_source_map.dump(2);
    }
}

// Find mesh files in given directory (extensions: .obj, .ply, .off)
// Throw exception if none found or multiple found
fs::path findMeshInDirectory(const fs::path& dir) {
    std::vector<fs::path> mesh_files;
    std::vector<std::string> extensions = { ".obj", ".ply", ".off" };
    for (const auto& entry : fs::directory_iterator(dir)) {
        if (entry.is_regular_file()) {
            fs::path p = entry.path();
            for (const auto& ext : extensions) {
                if (p.extension() == ext) {
                    mesh_files.push_back(p.filename());
                    break;
                }
            }
        }
    }
    if (mesh_files.empty()) {
        throw std::runtime_error(std::string(ERROR_NO_MESH) + dir.string());
    }
    if (mesh_files.size() > 1) {
        throw std::runtime_error(std::string(ERROR_MULTIPLE_MESH) + dir.string());
    }
    return mesh_files.front();
}

// Call external executable to process mesh data
void process_mesh(const fs::path& mesh_filepath, const fs::path& target_filepath,
    const fs::path& executable,
    const std::vector<std::string>& additional_args) {
    // executable -m mesh_filepath -o target_filepath ...
    std::string command = executable.string() + 
        " -m " + mesh_filepath.string() +
        " -o " + target_filepath.string();
    for (const auto& arg : additional_args) {
        command += " " + arg;
    }
    std::cout << "Executing command: " << command << std::endl;
    int ret = std::system(command.c_str());
    if (ret != 0) {
        std::cerr << "Command failed with return code: " << ret << std::endl;
    }
}

// Main function
int main() {
    try {
        // ========== Parameter settings (modify as needed) ==========
        PathParams params;
        params.data_dir = fs::path("path\\to\\deepsdf\\data");        // Directory containing preprocessed data
        params.source_dir = fs::path("path\\to\\dataset");      // Directory containing data to be processed
        // Data source name (defaults to the directory name of source_dir)
        params.source_name = params.source_dir.filename().string();
        params.split_filename = fs::path("path\\to\\deepsdf\\examples\\splits\\sv2_sofas_train.json");  // JSON split file

        bool skip = false;           // Whether to skip already processed meshes
        bool test_sampling = false;  // Whether to generate test SDF samples (only valid for sdf mode)
        bool surface_sampling = false;  // Whether to use mesh surface sampling (otherwise SDF sampling)
        bool visual_sampling = false;   // Whether to enable visual sampling (exports a .ply file)
        int batch_size = 8;         // Number of batch size

        fs::path deepsdf_dir = "path\\to\\deepsdf"//fs::current_path();  // Assume current directory is deep_sdf root
        fs::path executable;
        fs::path subdir;
        std::string extension;
        std::vector<std::string> additional_general_args;
        if (surface_sampling) {
            executable = deepsdf_dir / "bin" / "SampleVisibleMeshSurface.exe";
            subdir = "surface_samples";
            extension = ".ply";
        }
        else {
            executable = deepsdf_dir / "bin" / "PreprocessMesh.exe";
            subdir = "sdf_samples";
            extension = ".npz";
            if (test_sampling) {
                additional_general_args.push_back("-t");
            }
        }
        // ===================================================

        // Read JSON split file
        std::ifstream split_ifs(params.split_filename);
        if (!split_ifs) {
            std::cerr << "Error opening split file: " << params.split_filename << std::endl;
            return 1;
        }
        json split;
        split_ifs >> split;

        if (!split.contains(params.source_name)) {
            std::cerr << "Source name not found in split file: " << params.source_name << std::endl;
            return 1;
        }
        json class_directories = split[params.source_name];

        // Target directory: data_dir/subdir/source_name
        fs::path dest_dir = params.data_dir / subdir / params.source_name;
        std::cout << "Preprocessing data from " << params.source_dir << " and placing the results in " << dest_dir
            << std::endl;
        fs::create_directories(dest_dir);

        // Create normalization parameter directory for surface sampling mode
        fs::path normalization_param_dir;
        if (surface_sampling) {
            normalization_param_dir = params.data_dir / "normalization_params" / params.source_name;
            fs::create_directories(normalization_param_dir);
        }

        // Write data source information to the map file
        append_data_source_map(params.data_dir, params.source_name, params.source_dir);

        // Define task structure to store information for each mesh to process
        struct Task {
            fs::path mesh_filepath;
            fs::path target_filepath;
            std::vector<std::string> specific_args;
        };
        std::vector<Task> tasks;

        // Traverse each class directory
        for (auto& item : class_directories.items()) {
            std::string class_dir = item.key();
            json instance_dirs = item.value();

            fs::path class_path = params.source_dir / class_dir;
            std::cout << "Processing " << instance_dirs.size() << " instances of class " << class_dir << std::endl;
            fs::path target_class_dir = dest_dir / class_dir;
            fs::create_directories(target_class_dir);

            // Process each instance under the class
            for (const auto& instance : instance_dirs) {
                std::string instance_dir = instance.get<std::string>();
                fs::path shape_dir = class_path / instance_dir / "models";
                fs::path processed_filepath = target_class_dir / (instance_dir + extension);
                if (skip && fs::exists(processed_filepath)) {
                    std::cout << "Skipping " << processed_filepath << std::endl;
                    continue;
                }
                try {
                    // Find mesh file (only one valid file allowed in directory)
                    fs::path mesh_filename = findMeshInDirectory(shape_dir);
                    fs::path mesh_filepath = shape_dir / mesh_filename;
                    std::vector<std::string> specific_args;
                    if (surface_sampling) {
                        fs::path normalization_param_target_dir = normalization_param_dir / class_dir;
                        fs::create_directories(normalization_param_target_dir);
                        fs::path normalization_param_filename = normalization_param_target_dir / (instance_dir + ".npz");
                        specific_args.push_back("-n \"" + normalization_param_filename.string() + "\"");
                    }
                    if (visual_sampling) {
                        fs::path visual_filepath = target_class_dir / (instance_dir + ".ply");
                        specific_args.push_back("--ply " + visual_filepath.string());
                    }
                    tasks.push_back({ mesh_filepath, processed_filepath, specific_args });
                }
                catch (const std::exception& ex) {
                    std::cerr << ex.what() << " for instance " << instance_dir << std::endl;
                }
            }
        }

        // Use std::async for processing (batch size)
        std::vector<std::future<void>> futures;
        size_t task_count = tasks.size();

        for (size_t i = 0; i < task_count; i++) {
            std::vector<std::string> args = tasks[i].specific_args;
            args.insert(args.end(), additional_general_args.begin(), additional_general_args.end());

            futures.push_back(std::async(std::launch::async, process_mesh,
                tasks[i].mesh_filepath, tasks[i].target_filepath, executable, args));

            // Wait for all tasks in current batch to complete
            if (futures.size() >= batch_size || i == task_count - 1) {
                for (auto& f : futures) {
                    f.get();  
                }
                futures.clear();  // Clear futures for next batch
                std::cout << "Progress: " << (i + 1) << "/" << task_count << std::endl;
            }
        }

        return 0;
    }
    catch (const std::exception& ex) {
        std::cerr << "Exception caught: " << ex.what() << std::endl;
        return 1;
    }
}