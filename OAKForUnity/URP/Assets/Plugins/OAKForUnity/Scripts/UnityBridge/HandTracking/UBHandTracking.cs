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
        public GameObject light;
        public int countData;
        [Header("Hand 0")]
        public Vector3[] landmarks;
        public GameObject[] skeleton;
        public GameObject[] cylinders;
        public Vector2[] connections;
        [Header("Hand 1")]
        public Vector3[] landmarks1;
        public GameObject[] skeleton1;
        public GameObject[] cylinders1;
        public Vector2[] connections1;

        private float _oldRotation;
        
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
            _oldRotation = 0f;
            landmarks = new Vector3[21];
            landmarks1 = new Vector3[21];
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
            var arr2 = json["res2"]["arr1"];

            if (countData == -1)
            {
                countData = (int)arr2[0];
            }
            else
            {
                if (countData+2 < arr2[0]) Debug.LogError("MISSING DATA "+countData+ " "+arr2[0]);
                countData = arr2[0];
            }
            
            // PROCESS HANDS INFO
            var hand0 = json["hand_0"];
            var hand1 = json["hand_1"];

            // PLACE LEFT AND RIGHT HAND
            if (hand0["label"] == "left")
            {
                hand0 = json["hand_1"];
                hand1 = json["hand_0"];
            }
            
            // TODO: Create method to manage each hand
            if (hand0 != null)
            {
                if (hand0["gesture"] == "FIST")
                {
                    float rotation = (float) hand0["rotation"];
                    rotation *= 0.1f;
                    light.transform.Rotate(Vector3.right, rotation);
                }

            }
            
            var arr = hand0["world_landmarks"];

            for (int i = 0; i < 21; i++)
            {
                landmarks[i] = Vector3.zero;
            }

            if (arr == null)
            {
                for (int i = 0; i < 21; i++)
                {
                    skeleton[i].SetActive(false);
                    cylinders[i].SetActive(false);
                }
            }
            
            int index = 0;
            foreach(JSONNode obj in arr)
            {
                float x = 0.0f,y = 0.0f,z = 0.0f;
                float kx = 0.0f, ky = 0.0f;

                x = obj[0];
                y = obj[1]*-1.0f;
                z = obj[2];

                landmarks[index] = new Vector3(x,y,z);
                if (x!=0 && y!=0 && z!=0) 
                {
                    skeleton[index].SetActive(true);
                    skeleton[index].transform.position = landmarks[index];
                }
        
                index++;
            }

            bool allZero = true;
            for (int i=0; i<21; i++)
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
                for (int i = 0; i<21; i++) if (landmarks[i] == Vector3.zero) skeleton[i].SetActive(false);

                // place dots connections
                for (int i=0; i<21; i++)
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
            
            // HAND 1
            
            // PROCESS HANDS INFO
            //var hand1 = json["hand_1"];

            if (hand1 != null)
            {
                if (hand1["gesture"] == "FIST")
                {
                    float rotation = (float) hand1["rotation"];
                    //
                }
            }
            
            var arr1 = hand1["world_landmarks"];

            for (int i = 0; i<21; i++) landmarks1[i] = Vector3.zero;

            if (arr1 == null)
            {
                for (int i = 0; i < 21; i++)
                {
                    skeleton1[i].SetActive(false);
                    cylinders1[i].SetActive(false);
                }
            }

            
            index = 0;
            foreach(JSONNode obj in arr1)
            {
                float x = 0.0f,y = 0.0f,z = 0.0f;
                float kx = 0.0f, ky = 0.0f;

                x = obj[0]+0.5f;
                y = obj[1]*-1.0f;
                z = obj[2];

                landmarks1[index] = new Vector3(x,y,z);
                if (x!=0 && y!=0 && z!=0) 
                {
                    skeleton1[index].SetActive(true);
                    skeleton1[index].transform.position = landmarks1[index];
                }
        
                index++;
            }

            bool allZero1 = true;
            for (int i=0; i<21; i++)
            {
                if (landmarks1[i]!=Vector3.zero) 
                {
                    allZero1 = false;
                    break;
                }
            }

            // Update skeleton and movement
            if (!allZero1)
            {
                for (int i = 0; i<21; i++) if (landmarks1[i] == Vector3.zero) skeleton1[i].SetActive(false);

                // place dots connections
                for (int i=0; i<21; i++)
                {
                    int s = (int)connections1[i].x;
                    int e = (int)connections1[i].y;
                    
                    if (landmarks1[s] != Vector3.zero && landmarks1[e]!=Vector3.zero)
                    {
                        cylinders1[i].SetActive(true);
                        PlaceConnection(skeleton1[s],skeleton1[e],cylinders1[i]);
                    }
                    else cylinders1[i].SetActive(false);
                }
            }
        }
    }
}