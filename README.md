# DeepSDF Preprocessing

This is my personal modification to the code in the data preprocessing section of project [DeepSDF][1], which works for the latest version of [Pangolin (0.9.2)][2].

![chair](test/chair.png)

## Platform

Tested only on the Windows 11 64-bit platform.

## How to Use

Similar to the DeepSDF repository, the preprocessing code is in C++ and has the following requirements:

- [CLI11][3]
- [Pangolin][2]
- [nanoflann][4]
- [Eigen3][5]


To make it easier to configure the environment, requirements are in `DeepSDF_Preprocessing/third-party` directory. With these dependencies, the build process follows the standard CMake procedure:

```
cd path\to\DeepSDF_Proprecessing
mkdir build
cd build
cmake ..
make -j
```

Once this is done there should be two executables in the `DeepSDF_Preprocessing/bin` directory, one for surface sampling and one for SDF sampling. With the binaries, the dataset can be preprocessed using `preprocess_data.py`. To preprocess the mesh data for testing, run:

```
.\bin\Release\PreprocessMesh.exe -m test\chair.obj -o test\sample_chair.npz --ply test\sample_chair.ply
```

## What I've Modified

The following modifications are based on this [issue][6]:
* Replaced `pangolin::get` calls with `std::get` calls.
* Changed `prog.SetUniform("ttt", 1.0, 0, 0, 1);` to `prog.SetUniform("ttt", 1.0, 0.0, 0.0, 1.0);`.
* Updated the CMakeLists.txt file:
    * Added `set(CMAKE_CXX_STANDARD 17)`.
    * Used two header-only libraries: CLI11 and nanoflann.
    * Extended the `target_link_libraries` to `target_link_libraries(PreprocessMesh PRIVATE pango_core pango_display pango_geometry pango_glgeometry cnpy Eigen3::Eigen)`.

The following modifications are based on this [issue][7]:
* Commented out the `gl_PrimitiveID` variable declaration.

The following modifications are from my own changes:
* Added `#undef FAR` before `#include<zlib.h>` in `cnpy.h`.
* Added `#include <wtypes.h>` and `#define uint UINT` in `Utils.h`.
* Commented out the uniform names `ToWorld, slant_thr, ttt` because they are never used in shader.

Similar to the issues listed in [#19][8], [#25][9], and [#35][7], the `preprocess_data.py` script from [DeepSDF][1] failed to generate `.npz` files on my local development environment. To address this, I developed a simple batch processing tool using the C++17 standard, which includes the following features:
* Relies only on the C++ standard library and the [nlohmann][10] library.
* Processes data in batches (default: 8), and the batch size can be adjusted based on device performance and UI responsiveness.
* Requires compilation, unlike the Python script.

## Known Issues

The data can be preprocessed, but there are still some bugs. If anyone knows how to fix them, please open an issue. Here are the bugs:

`OpenGL Error: GL_INVALID_ENUM: An unacceptable value is specified for an enumerated argument. (1280)
In: path\to\DeepSDF_Proprecessing\third-party\pangolin\include\pangolin/gl/gl.hpp, line 202`

`Attribute name doesn't exist for program (normal)`

The OpenGL error does not interrupt execution; it only continuously triggers warnings. To reduce excessive error messages, I commented out the `cerr` section in the `inline GLint GlSlProgram::GetAttributeHandle` function within `glsl.hpp` during Pangolin configuration.
## License

DeepSDF_Preprocessing is relased under the MIT License. See the [LICENSE file][11] for more details.


[1]: https://github.com/facebookresearch/DeepSDF
[2]: https://github.com/stevenlovegrove/Pangolin
[3]: https://github.com/CLIUtils/CLI11
[4]: https://github.com/jlblancoc/nanoflann
[5]: https://eigen.tuxfamily.org
[6]: https://github.com/stevenlovegrove/Pangolin/issues/725
[7]: https://github.com/facebookresearch/DeepSDF/issues/35
[8]: https://github.com/facebookresearch/DeepSDF/issues/19
[9]: https://github.com/facebookresearch/DeepSDF/issues/25
[10]: https://github.com/nlohmann/json
[11]: https://github.com/Xiaoyanglib/DeepSDF_Preprocessing/blob/main/LICENSE

