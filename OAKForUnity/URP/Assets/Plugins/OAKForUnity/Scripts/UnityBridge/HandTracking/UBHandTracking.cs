using System;
using System.Runtime.InteropServices;
using UnityEngine;
using SimpleJSON;

namespace OAKForUnity
{
    public class UBHandTracking : PredefinedBase
    {
        // For future compatibility between UB and standard C++ plugin
        
        //Lets make our calls from the Plugin
        //[DllImport("depthai-unity", CallingConvention = CallingConvention.Cdecl)]
        /*
        * Pipeline creation based on streams template
        *
        * @param config pipeline configuration 
        * @returns pipeline 
        */
        //private static extern bool InitUBTest (in PipelineConfig config);

        //[DllImport("depthai-unity", CallingConvention = CallingConvention.Cdecl)]
        /*
        * Pipeline results
        *
        * @param frameInfo camera images pointers
        * ................
        * @returns Json with results or information about device availability. 
        */    
        //private static extern IntPtr UBTestResults(out FrameInfo frameInfo, bool getPreview, int width, int height,  ...., int deviceNum);

        [Header("Results")] 
        public Texture2D colorTexture;
        public string ubHandTrackingResults;
        public int countData;
        
        // private attributes
        private Color32[] _colorPixel32;
        private GCHandle _colorPixelHandle;
        private IntPtr _colorPixelPtr;

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
            // Init dataPath to load body pose NN model
            _dataPath = Application.dataPath;
            
            InitTexture();

            // Init FrameInfo. Only need it in case memcpy data ptr on plugin lib.
            frameInfo.colorPreviewData = _colorPixelPtr;

            countData = -1;
        }

        // Prepare Pipeline Configuration and call pipeline init implementation
        protected override bool InitDevice()
        {
            // For future compatibility between UB and standard C++ plugin

            // Color camera
            /*config.colorCameraFPS = cameraFPS;
            config.colorCameraResolution = (int) rgbResolution;
            config.colorCameraInterleaved = Interleaved;
            config.colorCameraColorOrder = (int) ColorOrderV;
            ....
            */
            
            deviceRunning = false;
            if (useUnityBridge)
            {
                deviceRunning = tcpClientBehaviour.InitUB();
            }
            /*else
            {
                // Plugin lib init pipeline implementation
                deviceRunning = InitUBTest(config);
            }*/

            // Check if was possible to init device with pipeline. Base class handles replay data if possible.
            if (!deviceRunning)
                Debug.LogError(
                    "Was not possible to initialize UB Hand Tracking. Check you have available devices on OAK For Unity -> Device Manager and check you setup correct deviceId if you setup one.");

            return deviceRunning;
        }

        // Get results from pipeline
        protected override void GetResults()
        {
            // if not doing replay
            if (!device.replayResults)
            {
                if (useUnityBridge)
                {
                    ubHandTrackingResults = tcpClientBehaviour.GetResults(out colorTexture);
                }
                /*else
                {
                    // Plugin lib pipeline results implementation
                    results = Marshal.PtrToStringAnsi(UBTestResults(out frameInfo, GETPreview, 300, 300,
                        UseDepth, ..., retrieveSystemInformation,
                        useIMU,
                        useSpatialLocator, (int) device.deviceNum));
                }*/
            }
            // if replay read results from file
            else
            {
                ubHandTrackingResults = device.results;
            }
        }
        
        // Process results from pipeline
        protected override void ProcessResults()
        {
            // If not replaying data
            if (!device.replayResults)
            {
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

            if (string.IsNullOrEmpty(ubHandTrackingResults)) return;

            // EXAMPLE HOW TO PARSE INFO
            var json = JSON.Parse(ubHandTrackingResults);
            var arr1 = json["res2"]["arr1"];

            if (countData == -1)
            {
                countData = (int)arr1[0];
            }
            else
            {
                if (countData+2 < arr1[0]) Debug.LogError("MISSING DATA "+countData+ " "+arr1[0]);
                countData = arr1[0];
            }
        }
    }
}