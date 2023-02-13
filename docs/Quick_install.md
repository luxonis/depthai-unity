# Quick installation instructions

## Requirements

### Windows 10 Installation

- Clone the repository to location on your local machine
  ```shell
  git clone https://github.com/luxonis/depthai-unity.git
  ```
- Unity 2021.2.7f

- If you're in Windows 10 you don't need to install anything special to have OAK device up and running.

### MacOS Installation

- Clone the repository to location on your local machine
  ```shell
  git clone https://github.com/luxonis/depthai-unity.git
  ```
- Unity 2021.2.7f

- libusb1 development package (MacOS & Linux only)
  ```shell
  brew install libusb
  ```
- OpenCV 4.5
  ```shell
  brew install opencv@4
  ```

### Linux Installation

- Requirements
  - libusb1 and OpenCV4(OpenCV 4.5 or above) development packages
  - cmake and gcc/gcc-c++ compiler
  - Unity 2021.2.7f

### Ubuntu

  ```shell
  sudo apt install libusb-1.0-0-dev libopencv-dev cmake build-essentials git-all
  ```

### Fedora 

  ```shell
  sudo dnf install libusb1-devel opencv-devel cmake gcc gcc-c++ git
  ```
  
#### Installation

- Clone the repository to location on your local machine
  ```shell
  git clone https://github.com/luxonis/depthai-unity.git
  cd depthai-unity
  git submodule update --init --recursive # get depthai submodules for compilation
  
  # Build depthai-core
  cd depthai-core
  cmake -S. -Bbuild -D'BUILD_SHARED_LIBS=ON'
  cmake --build build --config Debug --parallel 4 # Number can be change based on your CPU core
  cmake --build build --target install
  cd ..
  
  # Build depthai-unity
  cmake -S. -Bbuild -D'BUILD_SHARED_LIBS=ON'
  cmake --build build --config Debug --parallel 4
 
  # Copying files into unity project
  mkdir -p OAKForUnity/URP/Assets/Plugins/OAKForUnity/NativePlugin/Linux
  cp build/libdepthai-unity.so OAKForUnity/URP/Assets/Plugins/OAKForUnity/NativePlugin/Linux
  cp build/depthai-core/libdepthai-* OAKForUnity/URP/Assets/Plugins/OAKForUnity/NativePlugin/Linux
  
  # (Optinal) Convert mp4 into webm for linux (İt needs ffmpeg to do so)
  ffmpeg -i ./OAKForUnity/URP/Assets/Plugins/OAKForUnity/Textures/Playground/loader.mp4 -c:v libvpx -crf 30 -b:v 0 -b:a 128k -c:a libvorbis   ./OAKForUnity/URP/Assets/Plugins/OAKForUnity/Textures/Playground/loader.webm

  ```

## Next Steps

**[OAK For Unity](../OAKForUnity/README.md)**

Everything related to OAK For Unity package, projects and demo scenes.

This repo contains precompiled versions of depthai-unity library lowering any entry barrier, so you don't need to deal with C/C++ compilation.

**[Prebuild demos](../prebuild_demos/README.md)**

If you want to try some of our demos before opening Unity
