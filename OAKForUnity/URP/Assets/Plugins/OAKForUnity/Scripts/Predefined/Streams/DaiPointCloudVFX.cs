/*
 * This file contains cloud point VFX pipeline and interface for Unity scenes called "PointCloudVFX","MatrixVFX","Matrix2VFX"
 * Main goal is to display depth as point cloud using Visual Effect Graph
 * This work is based on Keijiro Takahashi https://github.com/keijiro/DepthAITestbed
 */

using System;
using System.Runtime.InteropServices;
using UnityEngine;
using System.Collections.Generic;
using SimpleJSON;

namespace OAKForUnity
{
    public class DaiPointCloudVFX : PredefinedBase
    {
        //Lets make our calls from the Plugin
        [DllImport("depthai-unity", CallingConvention = CallingConvention.Cdecl)]
        private static extern bool InitPointCloudVFX(PipelineConfig config);

        [DllImport("depthai-unity", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr PointCloudVFXResults(out FrameInfo frameInfo, bool getPreview, bool useDepth,
            bool retrieveInformation, bool useIMU, int deviceNum);


        // public attributes
        
        [Header("RGB Camera")] public float cameraFPS = 30;
        public RGBResolution rgbResolution;
        private const bool Interleaved = false;
        private const ColorOrder ColorOrderV = ColorOrder.BGR;

        [Header("Mono Cameras")] 
        public MonoResolution monoResolution;

        [Header("Point Cloud VFX Configuration")]
        [Tooltip("by default Kernel 7x7")]
        public MedianFilter medianFilter;

        public bool useIMU = false;
        public bool retrieveSystemInformation = false;
        private const bool GETPreview = false;
        private const bool UseDepth = true;

        [Header("Point Cloud VFX Results")] 
        public Texture2D monoRTexture;
        public Texture2D depthTexture;
        public string pointCloudVFXResults;
        public string systemInfo;

        // private attributes
        
        private Color32[] _monoRPixel32;
        private GCHandle _monoRPixelHandle;
        private IntPtr _monoRPixelPtr;

        private Color32[] _depthPixel32;
        private GCHandle _depthPixelHandle;
        private IntPtr _depthPixelPtr;

        /*
         * Init textures. In this case we allocate them but copy data in unity side with loadrawdata
         */
        void InitTexture()
        {
            monoRTexture = new Texture2D
                (640, 400, TextureFormat.R8, false)
                {filterMode = FilterMode.Point};

            depthTexture = new Texture2D
                (640, 400, TextureFormat.R16, false)
                {filterMode = FilterMode.Point};
        }

        // Init textures. In this case we don't assign pointers previous copy
        void Start()
        {
            InitTexture();
        }

        // Prepare Pipeline Configuration and call pipeline init implementation
        protected override bool InitDevice()
        {
            // Color camera
            config.colorCameraFPS = cameraFPS;
            config.colorCameraResolution = (int) rgbResolution;
            config.colorCameraInterleaved = Interleaved;
            config.colorCameraColorOrder = (int) ColorOrderV;
            
            // we don't need preview color
            config.previewSizeHeight = 0;
            config.previewSizeWidth = 0;

            // Mono camera
            config.monoLCameraResolution = (int) monoResolution;
            config.monoRCameraResolution = (int) monoResolution;

            // Depth
            config.confidenceThreshold = 245;
            config.leftRightCheck = true;
            config.subpixel = true;
            config.ispScaleF1 = 0;
            config.ispScaleF2 = 0;
            config.manualFocus = 0;
            config.deviceId = device.deviceId;
            config.deviceNum = (int) device.deviceNum;
            config.medianFilter = (int) medianFilter;

            // Plugin lib init pipeline implementation
            deviceRunning = InitPointCloudVFX(config);
            
            // Check if was possible to init device with pipeline. Base class handles replay data if possible.
            if (!deviceRunning)
                Debug.LogError(
                    "Was not possible to initialize Point Cloud VFX. Check you have available devices on OAK For Unity -> Device Manager and check you setup correct deviceId if you setup one.");

            return deviceRunning;
        }

        // Get results from pipeline
        protected override void GetResults()
        {
            // if not doing replay
            if (!device.replayResults)
            {
                // Plugin lib pipeline results implementation
                pointCloudVFXResults = Marshal.PtrToStringAnsi(PointCloudVFXResults(out frameInfo, GETPreview, UseDepth,
                    retrieveSystemInformation, useIMU,
                    (int) device.deviceNum));
            }
            else
            {
                pointCloudVFXResults = device.results;
            }
        }

        // Process results from pipeline
        protected override void ProcessResults()
        {
            // If not replaying data
            if (!device.replayResults)
            {
                // Load texture data from plugin pointer
                if (frameInfo.rectifiedRData != IntPtr.Zero)
                {
                    monoRTexture.LoadRawTextureData(frameInfo.rectifiedRData, 640 * 400);
                    monoRTexture.Apply();
                }
                
                // Load texture data from plugin pointer
                if (frameInfo.depthData != IntPtr.Zero)
                {
                    depthTexture.LoadRawTextureData(frameInfo.depthData, 640 * 400 * 2);
                    depthTexture.Apply();
                }

                // If recording data
                if (!device.recordResults) return;
                
                List<Texture2D> textures = new List<Texture2D>() {monoRTexture, depthTexture};
                List<string> nameTextures = new List<string>() {"monor", "depth"};

                device.Record("", textures, nameTextures);
            }
            // replay data
            else
            {
                for (int i = 0; i < device.textureNames.Count; i++)
                {
                    if (device.textureNames[i] == "monor")
                    {
                        monoRTexture.SetPixels32(device.textures[i].GetPixels32());
                        monoRTexture.Apply();
                    }

                    if (device.textureNames[i] == "depth")
                    {
                        depthTexture.SetPixels32(device.textures[i].GetPixels32());
                        depthTexture.Apply();
                    }

                }
            }

            if (string.IsNullOrEmpty(pointCloudVFXResults)) return;

            // EXAMPLE HOW TO PARSE INFO
            var obj = JSON.Parse(pointCloudVFXResults);
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