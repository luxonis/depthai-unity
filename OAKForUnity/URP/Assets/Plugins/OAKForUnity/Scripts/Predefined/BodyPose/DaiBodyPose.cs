/*
* This file contains body pose detector pipeline and interface for Unity scene called "Body Pose"
* Main goal is to show how to use basic NN model like body pose inside Unity. It's using MoveNet body pose model.
*/

using System;
using System.Runtime.InteropServices;
using UnityEngine;
using System.Collections.Generic;
using SimpleJSON;

namespace OAKForUnity
{
    public class DaiBodyPose : PredefinedBase
    {
        //Lets make our calls from the Plugin
        [DllImport("depthai-unity", CallingConvention = CallingConvention.Cdecl)]
        /*
        * Pipeline creation based on streams template
        *
        * @param config pipeline configuration 
        * @returns pipeline 
        */
        private static extern bool InitBodyPose(in PipelineConfig config);

        [DllImport("depthai-unity", CallingConvention = CallingConvention.Cdecl)]
        /*
        * Pipeline results
        *
        * @param frameInfo camera images pointers
        * @param getPreview True if color preview image is requested, False otherwise. Requires previewSize in pipeline creation.
        * @param width Unity preview image canvas width
        * @param height Unity preview image canvas height
        * @param useDepth True if depth information is requested, False otherwise. Requires confidenceThreshold in pipeline creation.
        * @param drawBodyPoseInPreview True to draw body landmakrs in the preview image
        * @param bodyLandmarkScoreThreshold Normalized score to filter body pose keypoints detections
        * @param retrieveInformation True if system information is requested, False otherwise. Requires rate in pipeline creation.
        * @param useIMU True if IMU information is requested, False otherwise. Requires freq in pipeline creation.
        * @param deviceNum Device selection on unity dropdown
        * @returns Json with results or information about device availability. 
        */    
        private static extern IntPtr BodyPoseResults(out FrameInfo frameInfo, bool getPreview, int width, int height,  bool useDepth, bool drawBodyPoseInPreview, float bodyLandmarkScoreThreshold, bool retrieveInformation, bool useIMU, int deviceNum);

        
        // Editor attributes
        [Header("RGB Camera")] 
        public float cameraFPS = 30;
        public RGBResolution rgbResolution;
        private const bool Interleaved = true;
        private const ColorOrder ColorOrderV = ColorOrder.BGR;

        [Header("Mono Cameras")] 
        public MonoResolution monoResolution;

        [Header("Body Pose Configuration")] 
        public MedianFilter medianFilter;
        public bool useIMU = false;
        public bool retrieveSystemInformation = false;
        public bool drawBodyPoseInPreview;
        public float bodyLandmarkThreshold; 
        private const bool GETPreview = true;
        private const bool UseDepth = true;

        [Header("Body Pose Results")] 
        public Texture2D colorTexture;
        public string bodyPoseResults;
        public string systemInfo;
        public Vector3[] landmarks;
        public GameObject[] skeleton;
        public GameObject[] cylinders;
        public Vector2[] connections;
         
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
            landmarks = new Vector3[17];
            
            // Init dataPath to load body pose NN model
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
            config.previewSizeHeight = 192; // 192 for lightning model, 256 for thunder model
            config.previewSizeWidth = 192;
            
            // Mono camera
            config.monoLCameraResolution = (int) monoResolution;
            config.monoRCameraResolution = (int) monoResolution;

            // Depth
            // Need it for depth
            config.confidenceThreshold = 230;
            config.leftRightCheck = true;
            config.ispScaleF1 = 2;
            config.ispScaleF2 = 3;
            config.manualFocus = 130;
            config.depthAlign = 1; // RGB align
            config.subpixel = true;
            config.deviceId = device.deviceId;
            config.deviceNum = (int) device.deviceNum;
            if (useIMU) config.freq = 400;
            if (retrieveSystemInformation) config.rate = 30.0f;
            config.medianFilter = (int) medianFilter;
            
            // Body Pose NN model
            config.nnPath1 = _dataPath +
                             "/Plugins/OAKForUnity/Models/movenet_singlepose_lightning_3.blob";
            
            // Plugin lib init pipeline implementation
            deviceRunning = InitBodyPose(config);

            // Check if was possible to init device with pipeline. Base class handles replay data if possible.
            if (!deviceRunning)
                Debug.LogError(
                    "Was not possible to initialize Body Pose. Check you have available devices on OAK For Unity -> Device Manager and check you setup correct deviceId if you setup one.");

            return deviceRunning;
        }

        // Get results from pipeline
        protected override void GetResults()
        {
            // if not doing replay
            if (!device.replayResults)
            {
                // Plugin lib pipeline results implementation
                bodyPoseResults = Marshal.PtrToStringAnsi(BodyPoseResults(out frameInfo, GETPreview, 300, 300, UseDepth, drawBodyPoseInPreview, bodyLandmarkThreshold, retrieveSystemInformation,
                    useIMU,
                    (int) device.deviceNum));
            }
            // if replay read results from file
            else
            {
                bodyPoseResults = device.results;
            }
        }

        void PlaceConnection(GameObject sp1, GameObject sp2, GameObject cyl)
        {
            Vector3 v3Start = sp1.transform.position;
            Vector3 v3End = sp2.transform.position;
     
            cyl.transform.position = (v3End - v3Start)/2.0f + v3Start;
     
            Vector3 v3T = cyl.transform.localScale; 
            v3T.y = (v3End - v3Start).magnitude/2; 
        
            cyl.transform.localScale = v3T;
     
            cyl.transform.rotation = Quaternion.FromToRotation(Vector3.up, v3End - v3Start);
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

            if (string.IsNullOrEmpty(bodyPoseResults)) return;

            // EXAMPLE HOW TO PARSE INFO
            var json = JSON.Parse(bodyPoseResults);
            var arr = json["landmarks"];

            for (int i = 0; i<17; i++) landmarks[i] = Vector3.zero;
            
            foreach(JSONNode obj in arr)
            {
                int index = -1;
                float x = 0.0f,y = 0.0f,z = 0.0f;
                float kx = 0.0f, ky = 0.0f;

                index = obj["index"];
                x = obj["location.x"];
                y = obj["location.y"];
                z = obj["location.z"];

                kx = obj["xpos"];
                ky = obj["ypos"];

                if (index != -1) 
                {
                    landmarks[index] = new Vector3(x/1000,y/1000,z/1000);
                    if (x!=0 && y!=0 && z!=0) 
                    {
                        skeleton[index].SetActive(true);
                        skeleton[index].transform.position = landmarks[index];
                    }
                }
            }

            bool allZero = true;
            for (int i=0; i<17; i++)
            {
                if (landmarks[i]!=Vector3.zero) 
                {
                    allZero = false;
                    break;
                }
            }

            // Update skeleton and movement
            if (!allZero)
            {
                for (int i = 0; i<17; i++) if (landmarks[i] == Vector3.zero) skeleton[i].SetActive(false);

                // place dots connections
                for (int i=0; i<16; i++)
                {
                    int s = (int)connections[i].x;
                    int e = (int)connections[i].y;
                    
                    if (landmarks[s] != Vector3.zero && landmarks[e]!=Vector3.zero)
                    {
                        cylinders[i].SetActive(true);
                        PlaceConnection(skeleton[s],skeleton[e],cylinders[i]);
                    }
                    else cylinders[i].SetActive(false);
                }
            }
            
            if (!retrieveSystemInformation || json == null) return;
            
            float ddrUsed = json["sysinfo"]["ddr_used"];
            float ddrTotal = json["sysinfo"]["ddr_total"];
            float cmxUsed = json["sysinfo"]["cmx_used"];
            float cmxTotal = json["sysinfo"]["ddr_total"];
            float chipTempAvg = json["sysinfo"]["chip_temp_avg"];
            float cpuUsage = json["sysinfo"]["cpu_usage"];
            systemInfo = "Device System Information\nddr used: "+ddrUsed+"MiB ddr total: "+ddrTotal+" MiB\n"+"cmx used: "+cmxUsed+" MiB cmx total: "+cmxTotal+" MiB\n"+"chip temp avg: "+chipTempAvg+"\n"+"cpu usage: "+cpuUsage+" %";
        }
    }
}