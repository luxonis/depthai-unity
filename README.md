# OAK For Unity

[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](https://opensource.org/licenses/MIT)&nbsp;
<img src="https://img.shields.io/badge/unity-2020.3.20f1-green.svg?style=flat-square" alt="unity 2020.3.20f1">
&nbsp;
<img src="https://img.shields.io/badge/unity-2021.2.7f1-green.svg?style=flat-square" alt="unity 2021.2.7f1">
&nbsp;
<img src="https://img.shields.io/badge/unity-URP-green.svg?style=flat-square" alt="unity URP">
&nbsp;
<img src="https://img.shields.io/badge/unity-HDRP-green.svg?style=flat-square" alt="unity HDRP">
&nbsp;
<img src="https://img.shields.io/badge/unity-Builtin-green.svg?style=flat-square" alt="unity Builtin">
&nbsp;

## Power Your Unity Projects with Advanced Spatial AI Using OAK 
### Elevate Unity Development with OAK: Advanced AI and Computer Vision

<p align="center">
  <img src="docs/img/oak_playground.gif" width="100%" />
  <br>

### Versatile Application Support 
Enables a broad range of applications, from interactive installations and games to health and sports monitoring, VR experiences, and more

<p align="center">
  <img src="docs/img/depthai-unity-plugin-face-detector.gif" width="100%" />
  <br>


### Ready-to-Use AI Models 
Provides a high-level API with a variety of pretrained models for rapid deployment of AI features like face and emotion recognition, and object detection.

<p align="center">
  <img src="docs/img/emotion-example.png" width="100%" />
  <br>


### Comprehensive Sensor Data Access
Provides access to OAK device streams including RGB, mono images, depth, and IMU data, enhancing the sensory input for Unity developers

<p align="center">
  <img src="docs/img/streams.png" width="100%" />
  <br>


### Streamlined Integration
Easy installation from the GitHub repository with Unity project example, and coming soon on the Unity Asset Store.


# Table of Contents
- [Get started](#get-started)
  - [Windows 10/11](#windows-1011)
  - [MacOS](#macos)
  - [Linux](#linux)

- [Examples](#examples)

- [Build C++ library](#build-c-library)

- [Unity bridge - python](#unity-bridge-python)

- [Known issues](#known-issues)

- [Roadmap](#roadmap)

- [Contributions](#contributions-and-acknowledgments)

- [Community projects](#community-projects)

- [License](#license)

# Get started

This repository contains Unity project with examples, inside the folder `OAKForUnity`

Easy way to start is explore the examples inside this project. Just need Unity and OAK device.

- Clone the repository to location on your local machine
  ```shell
  git clone https://github.com/luxonis/depthai-unity.git
  ```

- Install Unity (recommend latest 2021.3.x LTS)

## Windows 10/11

- If you're using Windows 10/11 you don't need to install anything special to have OAK device up and running.

## MacOS

- libusb1 development package (MacOS & Linux only)
  ```shell
  brew install libusb
  ```
- OpenCV 4.5
  ```shell
  brew install opencv@4
  ```

(see below how to compile the C++ library)

## Linux

### Ubuntu

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

if you get any message related to udev rules try:

```shell
echo 'SUBSYSTEM=="usb", ATTRS{idVendor}=="03e7", MODE="0666"' | sudo tee /etc/udev/rules.d/80-movidius.rules
sudo udevadm control --reload-rules && sudo udevadm trigger
``````

---

Steps:

- Open Unity project under folder `OAKForUnity`
- Click on menu option "OAK For Unity" -> "Example scenes"
- Hit play

**Important: Connect OAK device using USB3 cable for optimal experience**

# Examples

Unity project `OAKForUnity` includes following examples. Each example has its own unity scene and C# script showing how to use the results from the pipeline and has its own C++ pipeline.

Unity scenes could be found under folder `Plugins/OAKForUnity/Example scenes/`

and C++ pipelines under folder `src/`

## Playground
Main menu to explore all the examples

Go to menu on top: "OAK For Unity"->"Example scenes" and hit play

## Streams 
Access to device camera streams, including stereo, depth and disparity

## Point cloud / VFX
Point cloud generation from depth

## Face Detection
Face detector model running on OAK

## Face Emotion
Face detector model running on OAK

## Object Detection
Object detection using tiny yolo model

## Head Pose
Head pose estimation

## Hand Traking (Blaze hands through Unity Bridge)
Hand tracking using excellent python repo from geaxgx

# Build C++ library

## How it works
Unity standard plugin mechanism (usual in other platforms) is based on dynamic library interface between C# and C++ (depthai-core library) in this case.

We provide some prebuild libraries with the unity project, but also full source code under `src/` folder to build yourself.

## How to integrate your own pipelines

In case you want to extend the unity project or your own project with your own pipelines, you need to develop C++ depthai pipeline, C# interface and compile dynamic library.

### Windows 10/11
```shell
  git submodule update --init --recursive
  cmake -S. -Bbuild -D'BUILD_SHARED_LIBS=ON'
  cmake --build build --config Release --parallel 12
  mkdir OAKForUnity/URP/Assets/Plugins/OAKForUnity/NativePlugin/Windows
  cp build/Release/depthai-unity.dll OAKForUnity/URP/Assets/Plugins/OAKForUnity/NativePlugin/Windows
  cp build/depthai-core/Release/depthai-*.dll OAKForUnity/URP/Assets/Plugins/OAKForUnity/NativePlugin/Windows
  ```

### MacOS (dylib)
```shell
  git submodule update --init --recursive
  cmake -S. -Bbuild -D'BUILD_SHARED_LIBS=ON'
  cmake --build build --config Release --parallel 8
  mkdir OAKForUnity/URP/Assets/Plugins/OAKForUnity/NativePlugin/macOS
  cp build/libdepthai-unity.dylib OAKForUnity/URP/Assets/Plugins/OAKForUnity/NativePlugin/macOS
  cp build/depthai-core/libdepthai-* OAKForUnity/URP/Assets/Plugins/OAKForUnity/NativePlugin/macOS
  ```
### Linux (.so)
```shell
  git submodule update --init --recursive
  cmake -S. -Bbuild -D'BUILD_SHARED_LIBS=ON'
  cmake --build build --config Release --parallel 4
  mkdir -p OAKForUnity/URP/Assets/Plugins/OAKForUnity/NativePlugin/Linux
  cp build/libdepthai-unity.so OAKForUnity/URP/Assets/Plugins/OAKForUnity/NativePlugin/Linux
  cp build/depthai-core/libdepthai-* OAKForUnity/URP/Assets/Plugins/OAKForUnity/NativePlugin/Linux
  ```


We provide a small framework, taking care of unity life-cycle, so it's easy to extend the interface with your specifc use cases and custom models.

## How to integrate on your own unity project

Unity project includes main menu to navigate throught the examples. In case you want to integrate the core of the plugin with your own project, it's easy to export the main framework as .unitypackage.

We're currently working to provide the plugin through Scopely and Unity AssetStore so would be much easier to add the plugin to your project.

# Unity Bridge (python)

Since we released the initial version of the plugin, we got many feedback about the pain points to develop with it. One main recurrent feedback is the need to develop the pipeline in C++ making slow the developing cycle. We know also because custom projects with customers (our own dogfood).

DepthAI library comes also in Python flavour, so many community projects are available only on Python. Also developing with python is much more convenient, even only as prototype, as you don't need to compile and reload dynamic libraries.

In the future we want to explore also the creation of C# wrapper around depthai-core C++ library.

Unity Bridge is simple TCP socket bridge between Unity C# (using Netly) and Python, enabling reliable client/server approach:

- Reliable client/server approach based on TCP socket, similar to other unity python integrations like ROS-TCP connector

- Allows faster dev interations without C++ implementation and compilation

- Allows to integrate very easly community projects only available on Python

- Allows to develop prototype and after validation, develop the pipeline in C++ if you prefer to pack in dynamic library

- Allows to deploy application on other client platforms (lightweight, not supported like VR, mobile, ...) thanks to the client-server architecture - for example, external VR apps

## How it works

Unity app would act as client, DepthAI python app would act as server.

**Remember to start the server before playing the unity scene**

In this initial version, client is expecting to request image and results to the server.

On Unity side, we decided to rely on Netly framework as it's production ready and bullet-proof on many projects during time.

It's integrated on the same framework that request results from dynamic library, so it's very similar to integrate and even allow compatibility between the standard C++ and python modes in the future.

## Installation

Requirements are very similar to run any depthai python application.

For python we recommend to use conda environment:

`` conda create -n depthai-unity-env python==3.10 -y ``

`` cd unity_bridge ``

`` python -m pip install -r requirements.txt ``

## How to integrate external projects

Usually DepthAI application runs on main loop (while) where oak camera pipeline is running and returning results from AI models.

### python server app

We used the [color camera preview example](https://github.com/luxonis/depthai-python/blob/main/examples/ColorCamera/rgb_preview.py) from the python repository to illustrate the process and you can find under `unity_bridge/test_unity_bridge.py`

- Initialize unity bridge
- Prepare data serialization
- Send data back to Unity

We prepared dynamic serialization to make more easy send data back to unity from python.

``python test_unity_bridge.py``

Starts python server with OAK color camera preview, waiting for client to connect.

### unity client app

Inside the unity project, you will find new folders under `Example Scenes` and `Scripts` called `UnityBridge`

Open scene called `Test` and hit play. You should see oak color camera preview and some placeholder results from python.

**Important: start server first before starting client**

**Important: remember OAK devices can only run one pipeline at time, so it's not possible to run C++ examples if python server app is running at same time, so remember stop server app, in case you want to run other apps**

## Hand Tracking - depthai_hand_tracker by geaxgx

start server:
``python .\depthai_hand_tracking_unity_bridge.py --use_world_landmarks --gesture`` 

unity client scene: HandTracking.unity under folder `Example Scenes/UnityBridge`

# What's new

# Known issues
- If you're using OAK-1 (don't have stereo depth support) you need to disable depth on the examples, to prevent crash. UseDepth = false; config.confidenceThreshold = 0;

- If you just use the precompiled depthai-unity library inside Unity, be sure you're using latest version.
# Roadmap
Help build the roadmap checking out our **[Roadmap discussion](https://github.com/luxonis/depthai-unity/issues/1)** and feel free to explain about your use case.

# Contributions and Acknowledgments

First of all, Special thanks to **@sliwowitz** and **@onuralpszr** for their contribution and patience with Linux support !

## Community Projects

Are you building spatial app using OAK For Unity? Please DM and will be a pleasure to add a reference here

- **jbb-kryo** is building Unity app with some support for HoloLens2 and MKRT. Take a look here: https://github.com/kryotech-ltd/depthai-unity/tree/mkrt-hl2-update


## Acknowledgments

- Point cloud VFX examples are based on great work by [Keijiro Takahashi](https://github.com/keijiro/)

- Unity bridge uses [Netly]() for TCP socket communication.

## How to contribute in this repository

Everyone is more than welcome to contribute on this repository.

Contribution guide:
- fork the repository
- create new feature/bug branch
- make, commit and push your changes
- open pull request (PR) for development branch

After Your Pull Request is submitted, the project maintainers will review your PR. They might request some changes. Keep an eye on your GitHub notifications and be responsive to feedback.
Once the PR is approved and passes all checks, a maintainer will merge it into the development branch.

# Compatibility

| Platform     | Unity       | Render Pipeline |
| ------------ | ----------- | --------------- |
| Windows      | 2021.2.7f1  | ALL             |
| MacOS        | 2021.2.7f1  | ALL             |
| Linux        | 2021.3.22f1 | ALL (tested URP)|

## Related links

- [Unity forum](https://forum.unity.com/threads/oak-for-unity-spatial-ai-meets-the-power-of-unity.1205764/)
- [Youtube playlist](https://youtu.be/CSFOZLBV2RA?list=PLFzqMMJPSNSbsHp7QeJpOHrZu_1BAdDms)

# License
OAK For Unity is licensed under MIT License. See [LICENSE](LICENSE.md) for the full license text.