/*
* This file contains basic streams pipeline and interface for Unity scene called "Streams"
* Main goal is to show basic streams of OAK Device: color camera, mono right and left cameras, depth and disparity
*/

using System;
using System.Runtime.InteropServices;
using UnityEngine;
using System.Collections.Generic;
using SimpleJSON;

namespace OAKForUnity
{
    public class DaiStreams : PredefinedBase
    {
        //Lets make our calls from the Plugin
        [DllImport("depthai-unity", CallingConvention = CallingConvention.Cdecl)]
        /*
        * Pipeline creation based on streams template
        *
        * @param config pipeline configuration 
        * @returns pipeline 
        */
        private static extern bool InitStreams(in PipelineConfig config);

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
        private static extern IntPtr StreamsResults(out FrameInfo frameInfo, bool getPreview, bool useDepth,
            bool retrieveInformation, bool useIMU, int deviceNum);

        
        // Editor attributes
        [Header("RGB Camera")] 
        public float cameraFPS = 30;
        public RGBResolution rgbResolution;
        private const bool Interleaved = false;
        private const ColorOrder ColorOrderV = ColorOrder.BGR;

        [Header("Mono Cameras")] 
        public MonoResolution monoResolution;

        [Header("Streams Configuration")] 
        public MedianFilter medianFilter;
        public bool useIMU = false;
        public bool retrieveSystemInformation = false;
        private const bool GETPreview = true;
        private const bool UseDepth = true;

        [Header("Streams Results")] 
        public Texture2D colorTexture;
        public Texture2D monoRTexture;
        public Texture2D monoLTexture;
        public Texture2D disparityTexture;
        public Texture2D depthTexture;
        public string streamsResults;
        public string systemInfo;

        // private attributes
        private Color32[] _colorPixel32;
        private GCHandle _colorPixelHandle;
        private IntPtr _colorPixelPtr;

        private Color32[] _monoRPixel32;
        private GCHandle _monoRPixelHandle;
        private IntPtr _monoRPixelPtr;

        private Color32[] _monoLPixel32;
        private GCHandle _monoLPixelHandle;
        private IntPtr _monoLPixelPtr;

        private Color32[] _disparityPixel32;
        private GCHandle _disparityPixelHandle;
        private IntPtr _disparityPixelPtr;

        private Color32[] _depthPixel32;
        private GCHandle _depthPixelHandle;
        private IntPtr _depthPixelPtr;

        // Init textures. Each PredefinedBase implementation handles textures. Decoupled from external viz (Canvas, VFX, ...)
        void InitTexture()
        {
            colorTexture = new Texture2D(1920, 1080, TextureFormat.ARGB32, false);
            _colorPixel32 = colorTexture.GetPixels32();
            //Pin pixel32 array
            _colorPixelHandle = GCHandle.Alloc(_colorPixel32, GCHandleType.Pinned);
            //Get the pinned address
            _colorPixelPtr = _colorPixelHandle.AddrOfPinnedObject();

            monoRTexture = new Texture2D(640, 400, TextureFormat.ARGB32, false);
            _monoRPixel32 = monoRTexture.GetPixels32();
            //Pin pixel32 array
            _monoRPixelHandle = GCHandle.Alloc(_monoRPixel32, GCHandleType.Pinned);
            //Get the pinned address
            _monoRPixelPtr = _monoRPixelHandle.AddrOfPinnedObject();

            monoLTexture = new Texture2D(640, 400, TextureFormat.ARGB32, false);
            _monoLPixel32 = monoLTexture.GetPixels32();
            //Pin pixel32 array
            _monoLPixelHandle = GCHandle.Alloc(_monoLPixel32, GCHandleType.Pinned);
            //Get the pinned address
            _monoLPixelPtr = _monoLPixelHandle.AddrOfPinnedObject();

            disparityTexture = new Texture2D(640, 400, TextureFormat.ARGB32, false);
            _disparityPixel32 = disparityTexture.GetPixels32();
            //Pin pixel32 array
            _disparityPixelHandle = GCHandle.Alloc(_disparityPixel32, GCHandleType.Pinned);
            //Get the pinned address
            _disparityPixelPtr = _disparityPixelHandle.AddrOfPinnedObject();

            depthTexture = new Texture2D(640, 400, TextureFormat.ARGB32, false);
            _depthPixel32 = depthTexture.GetPixels32();
            //Pin pixel32 array
            _depthPixelHandle = GCHandle.Alloc(_depthPixel32, GCHandleType.Pinned);
            //Get the pinned address
            _depthPixelPtr = _depthPixelHandle.AddrOfPinnedObject();
        }

        // Start. Init textures and frameInfo
        void Start()
        {
            InitTexture();

            // Init FrameInfo. Only need it in case memcpy data ptr on plugin lib.
            frameInfo.colorPreviewData = _colorPixelPtr;
            frameInfo.rectifiedRData = _monoRPixelPtr;
            frameInfo.rectifiedLData = _monoLPixelPtr;
            frameInfo.disparityData = _disparityPixelPtr;
            frameInfo.depthData = _depthPixelPtr;
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
            config.confidenceThreshold = 230;
            config.leftRightCheck = true;
            config.ispScaleF1 = 0;
            config.ispScaleF2 = 0;
            config.manualFocus = 0;
            config.deviceId = device.deviceId;
            config.deviceNum = (int) device.deviceNum;
            if (useIMU) config.freq = 400;
            if (retrieveSystemInformation) config.rate = 30.0f;
            config.medianFilter = (int) medianFilter;

            // Plugin lib init pipeline implementation
            deviceRunning = InitStreams(config);

            // Check if was possible to init device with pipeline. Base class handles replay data if possible.
            if (!deviceRunning)
                Debug.LogError(
                    "Was not possible to initialize Streams. Check you have available devices on OAK For Unity -> Device Manager and check you setup correct deviceId if you setup one.");

            return deviceRunning;
        }

        // Get results from pipeline
        protected override void GetResults()
        {
            // if not doing replay
            if (!device.replayResults)
            {
                // Plugin lib pipeline results implementation
                streamsResults = Marshal.PtrToStringAnsi(StreamsResults(out frameInfo, GETPreview, UseDepth, retrieveSystemInformation,
                    useIMU,
                    (int) device.deviceNum));
            }
            // if replay read results from file
            else
            {
                streamsResults = device.results;
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
                monoRTexture.SetPixels32(_monoRPixel32);
                monoRTexture.Apply();
                monoLTexture.SetPixels32(_monoLPixel32);
                monoLTexture.Apply();
                disparityTexture.SetPixels32(_disparityPixel32);
                disparityTexture.Apply();
                depthTexture.SetPixels32(_depthPixel32);
                depthTexture.Apply();
                
                // In case we're recording send data to unity device implementation
                if (!device.recordResults) return;
                List<Texture2D> textures = new List<Texture2D>()
                    {colorTexture, monoRTexture, monoLTexture, disparityTexture, depthTexture};
                List<string> nameTextures = new List<string>() {"color", "monor", "monol", "disparity", "depth"};

                device.Record("", textures, nameTextures);
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

                    if (device.textureNames[i] == "monor")
                    {
                        monoRTexture.SetPixels32(device.textures[i].GetPixels32());
                        monoRTexture.Apply();
                    }

                    if (device.textureNames[i] == "monol")
                    {
                        monoLTexture.SetPixels32(device.textures[i].GetPixels32());
                        monoLTexture.Apply();
                    }

                    if (device.textureNames[i] == "disparity")
                    {
                        disparityTexture.SetPixels32(device.textures[i].GetPixels32());
                        disparityTexture.Apply();
                    }

                    if (device.textureNames[i] == "depth")
                    {
                        depthTexture.SetPixels32(device.textures[i].GetPixels32());
                        depthTexture.Apply();
                    }
                }
            }

            if (string.IsNullOrEmpty(streamsResults)) return;

            // EXAMPLE HOW TO PARSE INFO
            var obj = JSON.Parse(streamsResults);
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