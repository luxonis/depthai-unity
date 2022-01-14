# Build depthai-unity library

Expand platform to build depthai-unity library for:

<details><summary>Windows</summary>

## Requirements (if you didn't check before)

- Clone the repository to location on your local machine
  ```shell
  git clone https://github.com/luxonis/depthai-unity.git
  ```
- Unity 2021.2.7f

- If you're in Windows 10 you don't need to install anything special to have OAK device up and running.

## Build depthai-unity.dll (Coming soon)

</details>

<details><summary>MacOS</summary>

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

## Build depthai-unity.bundle

- Make sure submodules are updated

  ```shell
  git submodule update --init --recursive
  ```

- Build depthai-core library
  ```shell
  cd depthai-core
  ```
  Configure, build and install
  ```shell
  cmake -S. -Bbuild -D'BUILD_SHARED_LIBS=ON'
  cmake --build build --config Release --parallel 8
  cmake --build build --target install
  ```
- Build depthai-unity bundle

  Open xcode project located on folder xcode/depthai-unity/

  If you don't have Apple developer account, setup `signing certificate` to `Sign to Run Locally`

  Build product (cmd+B)

  Find .bundle file: Product -> Show Build Folder in Finder

  Copy depthai-unity.bundle from Products/Debug/ folder to repository folder OAKForUnity/URP/Assets/Plugins/OAKForUnity/NativePlugin/MacOS/

</details>

<details><summary>Linux</summary>

## Requirements (if you didn't check before)

- Clone the repository to location on your local machine
  ```shell
  git clone https://github.com/luxonis/depthai-unity.git
  ```
- Unity 2021.2.7f

- libusb1 development package (MacOS & Linux only)
  ```shell
  sudo apt install libusb-1.0-0-dev
  ```
- OpenCV 4.5
  ```shell
  sudo apt install libopencv-dev
  ```

## Build libdepthai-unity.so (Coming soon)

  </details>
