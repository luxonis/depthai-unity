/*
 * This base class is helper and handles common things for any pipeline
 * like multithread support, initialization and replay
 */

using System;
using System.Collections;
using UnityEngine;
using System.Runtime.InteropServices;
using System.Threading;

namespace OAKForUnity
{
    public class PredefinedBase : MonoBehaviour
    {
        [DllImport("depthai-unity", CallingConvention = CallingConvention.Cdecl)]
        
        /*
         * Close device
         * @param deviceNum Device selection on unity dropdown
         */
        private static extern void DAICloseDevice(int deviceNum);

        /*
        * FrameInfo contains pointers to all the images available on OAK devices. Mirroring FrameInfo on plugin lib.
        *
        * monochrome camera images (right and left)
        * color camera image
        * color preview camera image
        * disparity image
        * depth image
        * rectified monochrome camera images (right and left)
        *
        * Data pointers could be used in two ways:
        *
        * 1. Allocate Texture2D on Unity, get the pointer and memcpy on plugin side
        * 2. Return pointer and LoadRawTextureData on Texture2D
        *
        */
        [StructLayout(LayoutKind.Sequential)]
        public struct FrameInfo
        {
            public int monoRWidth, monoRHeight;
            public int monoLWidth, monoLHeight;
            public int colorWidth, colorHeight;
            public int colorPreviewWidth, colorPreviewHeight;
            public int diparityWidth, disparityHeight;
            public int depthWidth, depthHeight;
            public int rectifiedRWidth, rectifiedRHeight;
            public int rectifiedLWidth, rectifiedLHeight;
            public System.IntPtr monoRData;
            public System.IntPtr monoLData;
            public System.IntPtr colorData;
            public System.IntPtr colorPreviewData;
            public System.IntPtr disparityData;
            public System.IntPtr depthData;
            public System.IntPtr rectifiedRData;
            public System.IntPtr rectifiedLData;
        }

        /*
        * PipelineConfig contains all the setup option for a pipeline. Mirroring PipelineConfig on plugin lib.
        */
        [StructLayout(LayoutKind.Sequential)]
        public struct PipelineConfig
        {
            // General config
            public int deviceNum;
            public string deviceId;

            // Color Camera
            public float colorCameraFPS;

            // 0: THE_1080_P, 1: THE_4_K, 2: THE_12_MP, 3: THE_13_MP
            public int colorCameraResolution;

            [MarshalAs(UnmanagedType.I1)] public bool colorCameraInterleaved;

            // 0: BGR, 1:RGB
            public int colorCameraColorOrder;
            public int previewSizeWidth, previewSizeHeight;

            public int ispScaleF1, ispScaleF2;
            public int manualFocus;

            // MonoR Camera
            public float monoRCameraFPS;
            public int monoRCameraResolution;

            // MonoL camera
            public float monoLCameraFPS;
            public int monoLCameraResolution;

            // Stereo
            public int confidenceThreshold;
            [MarshalAs(UnmanagedType.I1)] public bool leftRightCheck;
            [MarshalAs(UnmanagedType.I1)] public bool subpixel;
            [MarshalAs(UnmanagedType.I1)] public bool extendedDisparity;
            public int depthAlign;
            public int medianFilter;

            // NN models
            public string nnPath1;
            public string nnPath2;
            public string nnPath3;

            // System information
            public float rate;

            // IMU
            public int freq;
            public int batchReportThreshold;
            public int maxBatchReports;
        };

        // public enums
        
        // Indicates to work on standard Start/Update unity lifecycle or start pipeline in separate Thread
        public enum ProcessMode
        {
            Multithread,
            UnityThread,
        }

        // Color camera resolution
        public enum RGBResolution
        {
            THE_1080_P,
            THE_4_K,
            THE_12_MP,
            THE_13_MP,
        }

        // Color camera order
        public enum ColorOrder
        {
            BGR,
            RGB,
        }

        // Mono camera resolution
        public enum MonoResolution
        {
            THE_400_P,
            THE_720_P,
            THE_800_P,
            THE_480_P,
        }

        // Median filter for depth
        public enum MedianFilter
        {
            MEDIAN_OFF,
            KERNEL_3X3,
            KERNEL_5X5,
            KERNEL_7X7,
        }

        // public attributes
        
        // process mode: Multithread or UnityThread
        public ProcessMode processMode;

        // Unity device. Device is mandatory.
        public OAKDevice device { get; set; }
        
        // True if pipeline is running on device
        [HideInInspector] public bool deviceRunning;
        
        // Thread and events in case of multithread process mode
        private Thread _worker;
        private readonly AutoResetEvent _stopEvent = new AutoResetEvent(false);

        // Thread to finish device
        private Thread _finishDeviceWorker;
        
        // Path for models
        protected String _dataPath;
        
        // Pipeline configuration and FrameInfo objects
        public PipelineConfig config;
        public FrameInfo frameInfo;

        /*
         * Prepare Pipeline Configuration and call pipeline init implementation.
         * @returns True if device available and pipeline started, false otherwise.
         */
        protected virtual bool InitDevice()
        {
            return false;
        }

        /*
         * Get results from pipeline
         */
        protected virtual void GetResults()
        {
        }
        
        /*
         * Process results from pipeline
         */
        protected virtual void ProcessResults()
        {
        }

        /*
         * Call device and pipeline initialization implementation or replay
         * from multithread or unity thread
         */
        private void StartDevice()
        {
            // if device is in replay mode
            if (device.replayResults && device.pathToReplay != "")
            {
                Debug.Log("Starting replay.");
                device.StartReplay();
                deviceRunning = true;
            }
            // let's try to start device
            else if (!InitDevice())
            {
                Debug.LogWarning(
                    "Was not possible to initialize. Check you have available devices on OAK For Unity -> Device Manager and check you setup correct deviceId if you setup one.");
                // if there is no real oak device available check for replay
                if (device.pathToReplay == "") return;
                
                Debug.Log("Lets fallback on recorded replay.");
                deviceRunning = true;
                device.StartReplay();
            }
        }

       /*
        * ConnectDevice is public main entry to start device with pipeline
        * Called from unity device if start pipeline when play is defined
        * Could be called from start pipeline button on canvas ui (p.eg)
        */
        public void ConnectDevice()
        {
            // Set dataPath for loading NN model if need it
            _dataPath = Application.dataPath;
            
            // if multithread worker is in charge to start pipeline and get results
            if (processMode == ProcessMode.Multithread)
            {
                _stopEvent.Reset();
                _worker = new Thread(WaitForFrames)
                {
                    IsBackground = true
                };
                _worker.Start();
            }
            else
            {
                if (!deviceRunning) StartDevice();
            }
        }

        /*
         * Worker main process
         */
        private void WaitForFrames()
        {
            // wait for stop event
            while (!_stopEvent.WaitOne(0))
            {
                // if device is not running try to start device
                if (!deviceRunning)
                {
                    StartDevice();
                }
                // if device is running try to get results from pipeline
                else
                {
                    GetResults();
                }
            }
        }

        /*
         * Close device. Manage multithread and unity thread cases.
         * Main entry point when we want to stop device
         * Could be called from stop pipeline button on ui canvas (p.eg)
         * Is called automatically on application quit.
         *
         * It's important call this method to NOT leave the device is bad state
         */
        public void FinishDevice()
        {
            if (processMode == ProcessMode.UnityThread)
            {
                if (deviceRunning)
                {
                    deviceRunning = false;
                    DAICloseDevice((int) device.deviceNum);
                }
            }
            else
            {
                if (_worker != null)
                {
                    _stopEvent.Set();
                    _worker.Join();
                    deviceRunning = false;
                    DAICloseDevice((int) device.deviceNum);
                }
            }
        }

        /*
         * Finish device in separate thread.
         */
        private void FinishDeviceWorker()
        {
            DAICloseDevice((int)device.deviceNum);
            deviceRunning = false;
        }
        public void FinishDeviceThread()
        {
            if (processMode == ProcessMode.Multithread)
            {
                if (_worker != null)
                {
                    _stopEvent.Set();
                    _worker.Join();
                }
            }

            // Start worker to finish device
            _finishDeviceWorker = new Thread(FinishDeviceWorker);
            _finishDeviceWorker.Start();
        }
        
        void OnApplicationQuit()
        {
            FinishDevice();
        }

        // Update is called once per frame
        // For unity thread mode
        void Update()
        {
            if (deviceRunning)
            {
                if (processMode == ProcessMode.UnityThread)
                {
                    GetResults();
                }

                ProcessResults();
            }
        }
    }
}