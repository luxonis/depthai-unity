/*
 * Unity device. Represents high level OAK device (not specific information about cameras, ..)
 * Designed to setup general parameters like assigning specific device number or device id (mxid) in case you run multiple OAK devices (example coming soon)
 * Also allows to select behaviour when hits play: start manually (with button calling pipeline ConnectDevice() p.eg), start automatically
 * with first available device or start automatically with device id (mxid)
 * For more information about OAK devices check https://docs.luxonis.com
 */
using UnityEngine;
using System.Collections.Generic;
using System.IO;

namespace OAKForUnity
{
    public class OAKDevice : MonoBehaviour
    {
        // public enums
        
        // device num allows to assign specific number to OAK device. Up to 10 devices.
        public enum DeviceNum
        {
            OAK_1,
            OAK_2,
            OAK_3,
            OAK_4,
            OAK_5,
            OAK_6,
            OAK_7,
            OAK_8,
            OAK_9,
        }
        
        // Start pipeline when play behaviours
        // NO: manual start (call pipeline ConnectDevice() from button on ui canvas p.eg)
        // WITH_FIRST_AVAILABLE_DEVICE: Picks first available device connected
        // WITH_DEVICE_ID: Init device with specific device id (mxid)
        public enum StartPipelineWhenPlay
        {
            NO,
            WITH_FIRST_AVAILABLE_DEVICE,
            WITH_DEVICE_ID,
        }

        // public attributes
        
        // Device num
        public DeviceNum deviceNum;
        // Device id (mxid)
        public string deviceId;
        
        [Header("Live Mode")] 
        public StartPipelineWhenPlay startPipelineWhenPlay;
        // List of pipelines. For now only supports one pipeline (first pipeline in the list). 
        // The idea is support more than one pipeline and custom pipelines (defined inside Unity)
        public List<PredefinedBase> pipelines;

        [Header("Record Results")] 
        // Enable recordResults and setup pathToRecord folder if you want to record results from a pipeline
        public bool recordResults;
        public string pathToRecord;

        // Replay results
        // if device is not available try to replay data
        // also useful for tutorials. Explain user how to interact using OAK device
        [Header("Replay Results")] 
        public bool replayResults;
        public string pathToReplay;

        [Tooltip("Number of saved frames starting with 0")]
        public int replayNumFrames;
        public float replayFPS;
        public bool replayInLoop;
        // replay textures and names
        public List<Texture2D> textures;
        public List<string> textureNames;
        // private attributes for replay
        private int _frame;
        private float _elapsedTime;
        // results from replay
        public string results { get; private set; }

        public void Start()
        {
            _frame = 0;

            // Texture List initialization
            textures = new List<Texture2D>(textureNames.Count);
            for (int i = 0; i < textureNames.Count; i++)
            {
                Texture2D texture = new Texture2D(2, 2, TextureFormat.ARGB32, false);
                textures.Add(texture);
            }

            // pipeline is mandatory
            if (pipelines.Count == 0) Debug.LogError("No pipeline defined.");
            else
            {
                if (recordResults && pathToRecord == "") Debug.LogError("No path to save recording.");
                if (replayResults && pathToReplay == "") Debug.LogError("No path to read recording.");

                // check conditions for replay. Disable record for safety.
                if (replayResults && pathToReplay != "" && replayNumFrames > 0)
                {
                    recordResults = false;
                }

                // set pipeline device
                pipelines[0].device = this;
                
                // if replay is set then ConnectDevice from here
                // no matter start pipeline
                if (replayResults && pathToReplay != "")
                {
                    ConnectDevice();
                }
                else if ((int) startPipelineWhenPlay > 0)
                {
                    switch ((int) startPipelineWhenPlay)
                    {
                        case 1:
                        case 2 when deviceId != "":
                            ConnectDevice();
                            break;
                        default:
                            Debug.LogError(
                                "To start pipeline when play with specific device, please specify device ID to avoid issues if you're running more than one device at same time. Check device IDs on OAK For Unity -> Device Manager (just copy and paste first value");
                            break;
                    }
                }
            }

            _elapsedTime = 0.0f;
        }

        /*
        * ConnectDevice is public main entry to start device with pipeline
        * Called from unity device if start pipeline when play is defined
        * Could be called from start pipeline button on canvas ui (p.eg)
        */
        public void ConnectDevice()
        {
            if (pipelines.Count == 0) Debug.LogError("No pipeline defined.");
            else
            {
                pipelines[0].ConnectDevice();
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
            if (pipelines.Count == 0) Debug.LogError("No pipeline defined.");
            else
            {
                pipelines[0].FinishDevice();
            }
        }

        /*
         * Close device in separate thread.
         * Not common case for single scene but useful for playground demo menu
         * or just in case to close device without freezing UI thread.
         */
        public void FinishDeviceThread()
        {
            if (pipelines.Count == 0) Debug.LogError("No pipeline defined.");
            else
            {
                pipelines[0].FinishDeviceThread();
            }
        }

        /*
         * True if device is running, false otherwise.
         */
        public bool IsDeviceRunning()
        {
            return pipelines[0].deviceRunning;
        }

        /*
         * Start replay if there is replay data
         */
        public void StartReplay()
        {
            if (pathToReplay != "" && replayNumFrames > 0)
            {
                recordResults = false;
                replayResults = true;
            }

            _elapsedTime = 0.0f;
        }

        /*
         * Stop replay
         */
        public void StopReplay()
        {
            replayResults = false;
        }
        
        /*
         * In case of replay just loading replay data
         * Decoupled from any pipeline. Specific pipeline needs to Process the data
         */
        public void Update()
        {
            if (!replayResults) return;
            
            _elapsedTime += Time.deltaTime;
            if (!(_elapsedTime > 1.0f / replayFPS)) return;
            
            _elapsedTime -= 1.0f / replayFPS;

            var finalPath = Application.dataPath + "/Plugins/OAKForUnity/Example Scenes/Recorded/" +
                            pathToReplay +
                            "/result_";

            var jsonPath = finalPath + _frame + ".json";

            // Load json results
            var reader = new StreamReader(jsonPath);
            results = reader.ReadToEnd();

            for (int i = 0; i < textureNames.Count; i++)
            {
                var imagePath = finalPath + textureNames[i] + "_" + _frame + ".png";
                // Load image
                var bytes = System.IO.File.ReadAllBytes(imagePath);

                textures[i].LoadImage(bytes);
            }

            _frame++;
            if (_frame > replayNumFrames)
            {
                if (replayInLoop) _frame = 0;
                else StopReplay();
            }
        }

        /*
         * Record pipeline results
         *
         * @param json with specific results
         * @param textures list
         * @param textures names list
         */
        public void Record(string json, List<Texture2D> textures, List<string> textureNames)
        {
            var finalPath = Application.dataPath + "/Plugins/OAKForUnity/Example Scenes/Recorded/" + pathToRecord +
                            "/result_";

            var jsonPath = finalPath + _frame + ".json";
            // save json
            var writer = new StreamWriter(jsonPath, false);
            writer.Write(json);
            writer.Close();

            for (int i = 0; i < textures.Count; i++)
            {
                var imagePath = finalPath + textureNames[i] + "_" + _frame + ".png";
                // save image
                var bytes = textures[i].EncodeToPNG();
                System.IO.File.WriteAllBytes(imagePath, bytes);
            }

            // free resources
            Resources.UnloadUnusedAssets();
            _frame++;
        }

        /*
         * Record pipeline results and one texture
         */
        public void Record(string json, Texture2D tex)
        {
            var finalPath = Application.dataPath + "/Plugins/OAKForUnity/Example Scenes/Recorded/" + pathToRecord +
                            "/result_" + _frame;

            var jsonPath = finalPath + ".json";
            var imagePath = finalPath + ".png";

            // save json
            var writer = new StreamWriter(jsonPath, false);
            writer.Write(json);
            writer.Close();

            // save image
            var bytes = tex.EncodeToPNG();
            System.IO.File.WriteAllBytes(imagePath, bytes);

            // free resources
            Resources.UnloadUnusedAssets();
            _frame++;
        }
    }
}