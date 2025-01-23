// CMake generated file. Do Not Edit.

#pragma once

namespace pangolin {

  // Forward declarations
  bool RegisterWinWindowFactory();


  inline bool RegisterFactoriesWindowInterface() {
    bool success = true;
    success &= RegisterWinWindowFactory();
    return success;
  }


} // pangolin
