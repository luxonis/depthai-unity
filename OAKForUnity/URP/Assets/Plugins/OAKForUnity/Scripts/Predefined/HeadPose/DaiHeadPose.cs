/*
* This file contains face emotion pipeline and interface for Unity scene called "Face Emotion"
* Main goal is to show how to use 2-stage NN model like face emotion inside Unity
*/

using System;
using System.Runtime.InteropServices;
using UnityEngine;
using System.Collections.Generic;
using SimpleJSON;

namespace OAKForUnity
{
    public class DaiHeadPose : PredefinedBase
    {
        //Lets make our calls from the Plugin
        [DllImport("depthai-unity", CallingConvention = CallingConvention.Cdecl)]
        /*
        * Pipeline creation based on streams template
        *
        * @param config pipeline configuration 
        * @returns pipeline 
        */
        private static extern bool InitHeadPose(in PipelineConfig config);

        [DllImport("depthai-unity", CallingConvention = CallingConvention.Cdecl)]
        /*
        * Pipeline results
        *
        * @param frameInfo camera images pointers
        * @param getPreview True if color preview image is requested, False otherwise. Requires previewSize in pipeline creation.
        * @param drawBestFaceInPreview True to draw face rectangle in the preview image
        * @param drawAllFacesInPreview True to draw all detected faces in the preview image
        * @param faceScoreThreshold Normalized score to filter face detections
        * @param useDepth True if depth information is requested, False otherwise. Requires confidenceThreshold in pipeline creation.
        * @param retrieveInformation True if system information is requested, False otherwise. Requires rate in pipeline creation.
        * @param useIMU True if IMU information is requested, False otherwise. Requires freq in pipeline creation.
        * @param deviceNum Device selection on unity dropdown
        * @returns Json with results or information about device availability. 
        */    
        private static extern IntPtr HeadPoseResults(out FrameInfo frameInfo, bool getPreview, int width, int height, bool drawBestFaceInPreview, bool drawAllFacesInPreview, float faceScoreThreshold, bool useDepth, bool retrieveInformation, bool useIMU, int deviceNum);

        
        // Editor attributes
        [Header("RGB Camera")] 
        public float cameraFPS = 30;
        public RGBResolution rgbResolution;
        private const bool Interleaved = false;
        private const ColorOrder ColorOrderV = ColorOrder.BGR;

        [Header("Mono Cameras")] 
        public MonoResolution monoResolution;

        [Header("Face Emotion Configuration")] 
        public MedianFilter medianFilter;
        public bool useIMU = false;
        public bool retrieveSystemInformation = false;
        public bool drawBestFaceInPreview;
        public bool drawAllFacesInPreview;
        public float faceScoreThreshold; 
        private const bool GETPreview = true;
        private const bool UseDepth = true;

        [Header("Head Pose Results")] 
        public Texture2D colorTexture;
        public string headPoseResults;
        public string systemInfo;
        public GameObject cube;
        
        // private attributes
        private Color32[] _colorPixel32;
        private GCHandle _colorPixelHandle;
        private IntPtr _colorPixelPtr;

        private float oldHeadPoseYaw;
        private float oldHeadPoseRoll;
        private float oldHeadPosePitch;
        
        // Init textures. Each PredefinedBase implementation handles textures. Decoupled from external viz (Canvas, VFX, ...)
        void InitTexture()
        {
            colorTexture = new Texture2D(300, 300, TextureFormat.ARGB32, false);
            _colorPixel32 = colorTexture.GetPixels32();
            //Pin pixel32 array
            _colorPixelHandle = GCHandle.Alloc(_colorPixel32, GCHandleType.Pinned);
            //Get the pinned address
            _colorPixelPtr = _colorPixelHandle.AddrOfPinnedObject();
        }

        // Start. Init textures and frameInfo
        void Start()
        {
            oldHeadPoseYaw = 0.0f;
            oldHeadPoseRoll = 0.0f;
            oldHeadPosePitch = 0.0f;
            
            // Init dataPath to load face detector NN model
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
            config.previewSizeHeight = 300;
            config.previewSizeWidth = 300;
            
            // Mono camera
            config.monoLCameraResolution = (int) monoResolution;
            config.monoRCameraResolution = (int) monoResolution;

            // Depth
            // Need it for depth
            /*config.confidenceThreshold = 230;
            config.leftRightCheck = true;
            if (rgbResolution == RGBResolution.THE_800_P)
            {
                config.ispScaleF1 = 1;
                config.ispScaleF2 = 2;
            }
            else
            {
                config.ispScaleF1 = 2;
                config.ispScaleF2 = 3;
            }*/
            config.manualFocus = 130;
            //config.depthAlign = 1; // RGB align
            config.subpixel = false;
            config.deviceId = device.deviceId;
            config.deviceNum = (int) device.deviceNum;
            if (useIMU) config.freq = 400;
            if (retrieveSystemInformation) config.rate = 30.0f;
            config.medianFilter = (int) medianFilter;
            
            // 2-stage NN model
            config.nnPath1 = _dataPath +
                             "/Plugins/OAKForUnity/Models/face-detection-retail-0004_openvino_2021.2_4shave.blob";

            config.nnPath2 = _dataPath +
                             "/Plugins/OAKForUnity/Models/head-pose-estimation-adas-0001_openvino_2021.2_4shave.blob";

            // Plugin lib init pipeline implementation
            deviceRunning = InitHeadPose(config);

            // Check if was possible to init device with pipeline. Base class handles replay data if possible.
            if (!deviceRunning)
                Debug.LogError(
                    "Was not possible to initialize Head Pose. Check you have available devices on OAK For Unity -> Device Manager and check you setup correct deviceId if you setup one.");

            return deviceRunning;
        }

        // Get results from pipeline
        protected override void GetResults()
        {
            // if not doing replay
            if (!device.replayResults)
            {
                // Plugin lib pipeline results implementation
                headPoseResults = Marshal.PtrToStringAnsi(HeadPoseResults(out frameInfo, GETPreview, 300,300,drawBestFaceInPreview, drawAllFacesInPreview, faceScoreThreshold, UseDepth, retrieveSystemInformation,
                    useIMU,
                    (int) device.deviceNum));
            }
            // if replay read results from file
            else
            {
                headPoseResults = device.results;
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

            if (string.IsNullOrEmpty(headPoseResults)) return;

            // EXAMPLE HOW TO PARSE INFO
            
            var obj = JSON.Parse(headPoseResults);
            int centerx = 0;
            int centery = 0;
            
            float headPoseYaw = 0.0f;
            float headPoseRoll = 0.0f;
            float headPosePitch = 0.0f;
            
            
            if (obj != null)
            {
                centerx = obj["best"]["xcenter"];
                centery = obj["best"]["ycenter"];
            }
            if (centerx == 0 && centery == 0) {}
            else
            {
                // record results
                if (device.recordResults)
                {
                    List<Texture2D> textures = new List<Texture2D>()
                        {colorTexture};
                    List<string> nameTextures = new List<string>() {"color"};

                    device.Record(headPoseResults, textures, nameTextures);
                }
                
                // PROCESS HEAD POSE RESULTS
                headPoseYaw = obj["headPoses"][0]["yaw"];
                headPoseRoll = obj["headPoses"][0]["roll"];
                headPosePitch = obj["headPoses"][0]["pitch"];

                if (headPoseYaw != 0 && headPosePitch != 0 && headPoseRoll != 0)
                {
                    //cube.transform.Rotate((headPosePitch - oldHeadPosePitch), -(headPoseYaw - oldHeadPoseYaw), (headPoseRoll - oldHeadPoseRoll), Space.Self);
                    cube.transform.Rotate(0f, -(headPoseYaw - oldHeadPoseYaw)*1.5f, 0f, Space.Self);
                    
                    oldHeadPoseRoll = headPoseRoll;
                    oldHeadPoseYaw = headPoseYaw;
                    oldHeadPosePitch = headPosePitch;
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