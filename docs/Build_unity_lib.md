# Build depthai-unity library

Expand platform to build depthai-unity library for:

<details><summary>Windows 10/11</summary>

## Requirements (if you didn't check before)

- Clone the repository to location on your local machine
  ```shell
  git clone https://github.com/luxonis/depthai-unity.git
  ```
- Unity 2021.2.7f

- If you're in Windows you don't need to install anything special to have OAK device up and running.

## Build depthai-unity.dll

  ```shell
  git submodule update --init --recursive
  cmake -G"Visual Studio 15 2017 Win64" -S. -Bbuild -D'BUILD_SHARED_LIBS=ON'
  cmake --build build --config Release --parallel 12
  mkdir OAKForUnity/URP/Assets/Plugins/OAKForUnity/NativePlugin/Windows
  cp build/Release/depthai-unity.dll OAKForUnity/URP/Assets/Plugins/OAKForUnity/NativePlugin/Windows
  cp build/depthai-core/Release/depthai-*.dll OAKForUnity/URP/Assets/Plugins/OAKForUnity/NativePlugin/Windows
  ```

</details>

<details><summary>MacOS Intel/Apple silicon</summary>

## Requirements (if you didn't check before)

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

## Build libdepthai-unity.dylib

  ```shell
  git submodule update --init --recursive
  cmake -S. -Bbuild -D'BUILD_SHARED_LIBS=ON'
  cmake --build build --config Release --parallel 8
  mkdir OAKForUnity/URP/Assets/Plugins/OAKForUnity/NativePlugin/macOS
  cp build/libdepthai-unity.dylib OAKForUnity/URP/Assets/Plugins/OAKForUnity/NativePlugin/macOS
  cp build/depthai-core/libdepthai-* OAKForUnity/URP/Assets/Plugins/OAKForUnity/NativePlugin/macOS
  ```

- Build depthai-unity bundle (Coming soon)

</details>

<details><summary>Linux Ubuntu/Fedora</summary>

## Requirements (if you didn't check before)

- Clone the repository to location on your local machine
  ```shell
  git clone https://github.com/luxonis/depthai-unity.git
  ```
- Unity 2021.3.22f1

### Ubuntu (tested on Ubuntu 20.04.5 LTS)

- libusb1 development package (MacOS & Linux only)
  ```shell
  sudo apt install libusb-1.0-0-dev cmake git-all
  ```
- OpenCV 4.5
  ```shell
  sudo apt install libopencv-dev
  ```

### Fedora

  ```shell
  sudo dnf install libusb1-devel opencv-devel cmake gcc gcc-c++ git
  ```

## Build libdepthai-unity.so

  ```shell
  git submodule update --init --recursive
  cmake -S. -Bbuild -D'BUILD_SHARED_LIBS=ON'
  cmake --build build --config Release --parallel 4
  mkdir -p OAKForUnity/URP/Assets/Plugins/OAKForUnity/NativePlugin/Linux
  cp build/libdepthai-unity.so OAKForUnity/URP/Assets/Plugins/OAKForUnity/NativePlugin/Linux
  cp build/depthai-core/libdepthai-* OAKForUnity/URP/Assets/Plugins/OAKForUnity/NativePlugin/Linux
  ```

</details>
