/*
* This file contains object detector pipeline based on tiny yolo v4 and interface for Unity scene called "Object Detector"
* Main goal is to show how to use basic NN model like object detector inside Unity
*/

using System;
using System.Runtime.InteropServices;
using UnityEngine;
using System.Collections.Generic;
using SimpleJSON;

namespace OAKForUnity
{
    public class DaiObjectDetector : PredefinedBase
    {
        //Lets make our calls from the Plugin
        [DllImport("depthai-unity", CallingConvention = CallingConvention.Cdecl)]
        /*
        * Pipeline creation based on streams template
        *
        * @param config pipeline configuration 
        * @returns pipeline 
        */
        private static extern bool InitObjectDetector(in PipelineConfig config);

        [DllImport("depthai-unity", CallingConvention = CallingConvention.Cdecl)]
        /*
        * Pipeline results
        *
        * @param frameInfo camera images pointers
        * @param getPreview True if color preview image is requested, False otherwise. Requires previewSize in pipeline creation.
        * @param useDepth True if depth information is requested, False otherwise. Requires confidenceThreshold in pipeline creation.
        * @param retrieveInformation True if system information is requested, False otherwise. Requires rate in pipeline creation.
        * @param useIMU True if IMU information is requested, False otherwise. Requires freq in pipeline creation.
        * @param deviceNum Device selection on unity dropdown
        * @returns Json with results or information about device availability. 
        */    
        private static extern IntPtr ObjectDetectorResults(out FrameInfo frameInfo, bool getPreview,float objectScoreThreshold, bool useDepth, bool retrieveInformation, bool useIMU, int deviceNum);

        
        // Editor attributes
        [Header("RGB Camera")] 
        public float cameraFPS = 30;
        public RGBResolution rgbResolution;
        private const bool Interleaved = false;
        private const ColorOrder ColorOrderV = ColorOrder.BGR;

        [Header("Mono Cameras")] 
        public MonoResolution monoResolution;

        [Header("Object Detector Configuration")] 
        public MedianFilter medianFilter;
        public bool useIMU = false;
        public bool retrieveSystemInformation = false;
        public float detectionScoreThreshold; 
        private const bool GETPreview = true;
        private const bool UseDepth = true;

        [Header("Object Detector Results")] 
        public Texture2D colorTexture;
        public string objectDetectorResults;
        public string systemInfo;

        [Header("Objects")] 
        public GameObject apple;
        
        // private attributes
        private Color32[] _colorPixel32;
        private GCHandle _colorPixelHandle;
        private IntPtr _colorPixelPtr;

        // Init textures. Each PredefinedBase implementation handles textures. Decoupled from external viz (Canvas, VFX, ...)
        void InitTexture()
        {
            colorTexture = new Texture2D(416, 416, TextureFormat.ARGB32, false);
            _colorPixel32 = colorTexture.GetPixels32();
            //Pin pixel32 array
            _colorPixelHandle = GCHandle.Alloc(_colorPixel32, GCHandleType.Pinned);
            //Get the pinned address
            _colorPixelPtr = _colorPixelHandle.AddrOfPinnedObject();
        }

        // Start. Init textures and frameInfo
        void Start()
        {
            // Init dataPath to load object detector NN model
            _dataPath = Application.dataPath;
            
            InitTexture();

            // Init FrameInfo. Only need it in case memcpy data ptr on plugin lib.
            frameInfo.colorPreviewData = _colorPixelPtr;
        }

        // Prepare Pipeline Configuration and call pipeline init implementation
        protected override bool InitDevice()
        {
            // Color camera
            config.colorCameraFPS = cameraFPS;
            config.colorCameraResolution = (int) rgbResolution;
            config.colorCameraInterleaved = Interleaved;
            config.colorCameraColorOrder = (int) ColorOrderV;
            // Need it for color camera preview
            config.previewSizeHeight = 416;
            config.previewSizeWidth = 416;
            
            // Mono camera
            config.monoLCameraResolution = (int) monoResolution;
            config.monoRCameraResolution = (int) monoResolution;

            // Depth
            // Need it for depth
            config.confidenceThreshold = 255;
            config.leftRightCheck = true;
            config.ispScaleF1 = 0;
            config.ispScaleF2 = 0;
            config.manualFocus = 130;
            config.depthAlign = 1; // RGB align
            config.subpixel = false;
            config.deviceId = device.deviceId;
            config.deviceNum = (int) device.deviceNum;
            if (useIMU) config.freq = 400;
            if (retrieveSystemInformation) config.rate = 30.0f;
            config.medianFilter = (int) medianFilter;
            
            // Object NN model
            config.nnPath1 = _dataPath +
                             "/Plugins/OAKForUnity/Models/yolo-v4-tiny-tf_openvino_2021.4_6shave.blob";
            
            // Plugin lib init pipeline implementation
            deviceRunning = InitObjectDetector(config);

            // Check if was possible to init device with pipeline. Base class handles replay data if possible.
            if (!deviceRunning)
                Debug.LogError(
                    "Was not possible to initialize Object Detector. Check you have available devices on OAK For Unity -> Device Manager and check you setup correct deviceId if you setup one.");

            return deviceRunning;
        }

        // Get results from pipeline
        protected override void GetResults()
        {
            // if not doing replay
            if (!device.replayResults)
            {
                // Plugin lib pipeline results implementation
                objectDetectorResults = Marshal.PtrToStringAnsi(ObjectDetectorResults(out frameInfo, GETPreview, detectionScoreThreshold, UseDepth, retrieveSystemInformation, useIMU, (int) device.deviceNum));
            }
            // if replay read results from file
            else
            {
                objectDetectorResults = device.results;
            }
        }

        // Process results from pipeline
        protected override void ProcessResults()
        {
            // If not replaying data
            if (!device.replayResults)
            {
                // Apply textures
                colorTexture.SetPixels32(_colorPixel32);
                colorTexture.Apply();
            }
            // if replaying data
            else
            {
                // Apply textures but get them from unity device implementation
                for (int i = 0; i < device.textureNames.Count; i++)
                {
                    if (device.textureNames[i] == "color")
                    {
                        colorTexture.SetPixels32(device.textures[i].GetPixels32());
                        colorTexture.Apply();
                    }
                }
            }

            if (string.IsNullOrEmpty(objectDetectorResults)) return;
            
            var obj = JSON.Parse(objectDetectorResults);
            int centerx = 0;
            int centery = 0;
            
            if (obj != null)
            {
                // record results
                if (device.recordResults)
                {
                    List<Texture2D> textures = new List<Texture2D>()
                        {colorTexture};
                    List<string> nameTextures = new List<string>() {"color"};

                    device.Record(objectDetectorResults, textures, nameTextures);
                }

                float bestAppleScore = 10000000.0f;;
                int bestAppleDepthx = 0;
                int bestAppleDepthy = 0;
                int bestAppleDepthz = 0;

                foreach (JSONNode detection in obj["objects"])
                {
                    // look for person and apple
                    if (detection["label"] != "apple") continue;
                    if (!(detection["score"] < bestAppleScore)) continue;
                    
                    bestAppleScore = detection["score"];
                    bestAppleDepthx = detection["X"];
                    bestAppleDepthy = detection["Y"];
                    bestAppleDepthz = detection["Z"];
                }

                if (bestAppleDepthx == 0 && bestAppleDepthy == 0 && bestAppleDepthz == 0) {}
                else
                {
                    // move apple object
                    // Normalize 3D position of object regarding the camera to the Unity scene depending your use case / design / needs
                    apple.transform.localPosition = new Vector3((float)bestAppleDepthx/100.0f,(float)bestAppleDepthy/100.0f,(float)bestAppleDepthz/100.0f);
                }
                
            }
            
            if (!retrieveSystemInformation || obj == null) return;
            
            float ddrUsed = obj["sysinfo"]["ddr_used"];
            float ddrTotal = obj["sysinfo"]["ddr_total"];
            float cmxUsed = obj["sysinfo"]["cmx_used"];
            float cmxTotal = obj["sysinfo"]["ddr_total"];
            float chipTempAvg = obj["sysinfo"]["chip_temp_avg"];
            float cpuUsage = obj["sysinfo"]["cpu_usage"];
            systemInfo = "Device System Information\nddr used: "+ddrUsed+"MiB ddr total: "+ddrTotal+" MiB\n"+"cmx used: "+cmxUsed+" MiB cmx total: "+cmxTotal+" MiB\n"+"chip temp avg: "+chipTempAvg+"\n"+"cpu usage: "+cpuUsage+" %";
        }
    }
}