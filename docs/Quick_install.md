# Quick installation instructions

## Requirements

### Windows 10

- Clone the repository to location on your local machine
  ```shell
  git clone https://github.com/luxonis/depthai-unity.git
  ```
- Unity 2021.2.7f

- If you're in Windows 10 you don't need to install anything special to have OAK device up and running.

### MacOS

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
  brew install opencv@4.5.4
  ```

### Linux

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

## Next Steps

**[OAK For Unity](../OAKForUnity/README.md)**

Everything related to OAK For Unity package, projects and demo scenes.

This repo contains precompiled versions of depthai-unity library lowering any entry barrier, so you don't need to deal with C/C++ compilation.

**[Prebuild demos](../prebuild_demos/README.md)**

If you want to try some of our demos before opening Unity
