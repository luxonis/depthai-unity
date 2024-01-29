using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.SceneManagement;
using System.Runtime.InteropServices;
using System;
using System.Threading;

namespace OAKForUnity
{
    public class UIMenuManager : MonoBehaviour
    {
        // Check device connected
        [DllImport("depthai-unity", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr GetAllDevices();

        // Scenes list and titles for menu
        public List<string> scenesList;
        public List<string> titlesFLList;
        public List<string> titlesSLList;

        // UI binding to manage the demo menu
        [Header("UI Binding")] public TMPro.TextMeshProUGUI titleText;
        public TMPro.TextMeshProUGUI oakPlaygroundText;
        public TMPro.TextMeshProUGUI connectedDeviceText;
        public GameObject burgerButton;
        public GameObject leftButton;
        public GameObject rightButton;
        public GameObject demoMenu;
        public GameObject overlay;
        public GameObject loader;
        public GameObject loaderBackground;
        public GameObject cameraObject;
        
        private int _currentScene;
        private bool _mainMenu;
        private bool _deviceConnected;
        
        // Check device connected in different thread to avoid UI freeze
        private Thread _worker;
        private void OnEnable()
        {
            // Listen for new scene loaded
            SceneManager.sceneLoaded += OnSceneLoaded;
        }

        // Start is called before the first frame update
        void Start()
        {
            // main menu scene, in fact loading Matrix2 VFX but not the first real demo
            _currentScene = -1;
            _mainMenu = true;
            _deviceConnected = false;
            SceneManager.LoadScene("Matrix2VFX", LoadSceneMode.Additive);
            // play with camera to avoid error between scenes
            cameraObject.SetActive(false);
        }

        // Update is called once per frame
        void Update()
        {
            // Update connected device text when thread finishes
            if (_deviceConnected) connectedDeviceText.text = "OAK device"+"\n"+"is connected";
            else connectedDeviceText.text = "OAK device"+"\n"+"not connected";
        }

        // Before removing loader check device is running
        IEnumerator WaitForDeviceRunning()
        {
            GameObject[] OAKDevices;
            OAKDevices = GameObject.FindGameObjectsWithTag("OAKDevice");

            bool atLeastOneDeviceRunning = false;
            int attempts = 0;

            while (!atLeastOneDeviceRunning || attempts > 25)
            {
                foreach (GameObject OAKDevice in OAKDevices)
                {
                    if (OAKDevice.GetComponent<OAKDevice>().IsDeviceRunning()) atLeastOneDeviceRunning = true;
                }

                if (!atLeastOneDeviceRunning) yield return new WaitForSeconds(1);
                attempts++;
            }

            HideLoader();
        }

        // When new scene is loaded remove loader when device is running
        void OnSceneLoaded(Scene scene, LoadSceneMode mode)
        {
            if (scene.name == "HandTracking") HideLoader(); 
            else StartCoroutine(WaitForDeviceRunning());
        }
        
        // UI helpers. FadeOutText and ScrollUp menu
        private IEnumerator FadeOutText(float timeSpeed, TMPro.TextMeshProUGUI text)
        {
            text.color = new Color(text.color.r, text.color.g, text.color.b, 1);
            while (text.color.a > 0.0f)
            {
                text.color = new Color(text.color.r, text.color.g, text.color.b,
                    text.color.a - (Time.deltaTime * timeSpeed));
                yield return null;
            }
        }

        /*
         * Ease Scroll Up
         */
        private IEnumerator ScrollUp(float seconds, GameObject go, int easeType)
        {
            var transf = go.GetComponent<RectTransform>();
            
            transf.anchoredPosition = new Vector2(0.0f, -670.0f);

            var elapsedTime = 0.0f;
            
            while (elapsedTime < seconds)
            {
                elapsedTime += Time.deltaTime;
                transf.anchoredPosition = new Vector2(0.0f, -EaseInQuad(31.0f,670.0f,(float)(seconds-elapsedTime)/seconds));
                yield return null;
            }
        }

        /*
         * Linear Scroll up
         */
        private IEnumerator ScrollUp(float timeSpeed, GameObject go)
        {
            var transf = go.GetComponent<RectTransform>();
            
            transf.anchoredPosition = new Vector2(0.0f, -670.0f);
            
            // linear
            while (transf.anchoredPosition.y < -31.0f)
            {
                transf.anchoredPosition = new Vector2(0.0f, transf.anchoredPosition.y + (Time.deltaTime * timeSpeed));
                yield return null;
            }
        }

        // Open Scene
        IEnumerator OpenSceneAsync(int numScene)
        {
            CloseMenu();
            ShowLoader();
            
            ShowTitle(numScene);
            yield return new WaitForSeconds(1);

            if (_mainMenu)
            {
                _mainMenu = false;
                SceneManager.UnloadSceneAsync("Matrix2VFX");
                _currentScene = numScene;
                StartCoroutine(LoadAsyncScene(scenesList[_currentScene]));
            }
            else
            {
                SceneManager.UnloadSceneAsync(scenesList[_currentScene]);
                _currentScene = numScene;
                StartCoroutine(LoadAsyncScene(scenesList[_currentScene]));
            }

            StartCoroutine(FadeOutText(1.0f, titleText));
            StartCoroutine(FadeOutText(1.0f, oakPlaygroundText));

            yield return null;
        }

        
        public void OpenScene(int numScene)
        {
            StartCoroutine(OpenSceneAsync(numScene));
        }

        // UI helpers. Show and hide loader. Show title.
        void ShowLoader()
        {
            cameraObject.SetActive(true);
            loaderBackground.SetActive(true);
            loader.SetActive(true);
        }

        void HideLoader()
        {
            loader.SetActive(false);
            loaderBackground.SetActive(false);
            cameraObject.SetActive(false);
        }

        void ShowTitle(int numScene)
        {
            titleText.color = new Color(titleText.color.r, titleText.color.g, titleText.color.b, 1);
            oakPlaygroundText.color = new Color(oakPlaygroundText.color.r, oakPlaygroundText.color.g, oakPlaygroundText.color.b, 1);
            titleText.text = titlesFLList[numScene] + "\n" + titlesSLList[numScene];
        }
        
        // Open next scene
        IEnumerator OpenNextSceneAsync()
        {
            ShowLoader();
            
            _currentScene++;
            if (_currentScene > scenesList.Count - 1) _currentScene = 0;

            ShowTitle(_currentScene);
            yield return new WaitForSeconds(1);
            
            if (_mainMenu)
            {
                _mainMenu = false;
                SceneManager.UnloadSceneAsync("Matrix2VFX");
            }
            else
            {
                if (_currentScene == 0)
                {
                    SceneManager.UnloadSceneAsync(scenesList[^1]);
                }
                else
                {
                    SceneManager.UnloadSceneAsync(scenesList[_currentScene - 1]);
                }
            }

            StartCoroutine(LoadAsyncScene(scenesList[_currentScene]));
            
            StartCoroutine(FadeOutText(1.0f, titleText));
            StartCoroutine(FadeOutText(1.0f, oakPlaygroundText));

            yield return null;
        }
        
        public void OpenNextScene()
        {
            StartCoroutine(OpenNextSceneAsync());
        }

        // Open previous scene
        IEnumerator OpenPrevSceneAsync()
        {
            ShowLoader();

            _currentScene--;
            if (_currentScene < 0) _currentScene = scenesList.Count-1;

            ShowTitle(_currentScene);
            yield return new WaitForSeconds(1);
            
            if (_mainMenu)
            {
                _mainMenu = false;
                SceneManager.UnloadSceneAsync("Matrix2VFX");
            }
            else
            {
                if (_currentScene == scenesList.Count-1)
                {
                    SceneManager.UnloadSceneAsync(scenesList[0]);
                }
                else
                {
                    SceneManager.UnloadSceneAsync(scenesList[_currentScene + 1]);
                }
            }

            StartCoroutine(LoadAsyncScene(scenesList[_currentScene]));
            
            StartCoroutine(FadeOutText(1.0f, titleText));
            StartCoroutine(FadeOutText(1.0f, oakPlaygroundText));
            
            yield return null;
        }
        public void OpenPrevScene()
        {
            StartCoroutine(OpenPrevSceneAsync());
        }

        IEnumerator LoadAsyncScene(string sceneName)
        {
            // The Application loads the Scene in the background as the current Scene runs.
            // This is particularly good for creating loading screens.
            // You could also load the Scene by using sceneBuildIndex. In this case Scene2 has
            // a sceneBuildIndex of 1 as shown in Build Settings.

            var OAKDevices = GameObject.FindGameObjectsWithTag("OAKDevice");

            foreach (var OAKDevice in OAKDevices)
            {
                OAKDevice.GetComponent<OAKDevice>().FinishDeviceThread();
            }

            yield return new WaitForSeconds(2);
            
            var asyncLoad = SceneManager.LoadSceneAsync(sceneName,LoadSceneMode.Additive);

            // Wait until the asynchronous scene fully loads
            while (!asyncLoad.isDone)
            {
                yield return null;
            }
        }

        // Just check if there is oak device connected or not
        bool CheckForDevice()
        {
            string devicesJson = Marshal.PtrToStringAnsi(GetAllDevices());
            if (devicesJson == "null") return false;

            return true;
        }

        private void CheckForDeviceWorker()
        {
            // Check for devices
            _deviceConnected = CheckForDevice();
        }
        
        // Open demo menu
        public void OpenMenu()
        {
            _worker = new Thread(CheckForDeviceWorker);
            _worker.Start();
            
            // Remove buttons and texts
            burgerButton.SetActive(false);
            leftButton.SetActive(false);
            rightButton.SetActive(false);
            titleText.gameObject.SetActive(false);
            oakPlaygroundText.gameObject.SetActive(false);
        
            // activate overlay
            overlay.SetActive(true);
            
            // scroll up demo menu
            demoMenu.SetActive(true);
            
            // linear
            //StartCoroutine(ScrollUp(1000.0f, demoMenu));
            // easeQuadIn
            StartCoroutine(ScrollUp(0.5f, demoMenu, 0));
        }

        // Close demo menu
        public void CloseMenu()
        {
            // put menu on starting position
            var transf = demoMenu.GetComponent<RectTransform>();
            transf.anchoredPosition = new Vector2(0.0f, -670.0f);
            
            // Remove buttons and texts
            burgerButton.SetActive(true);
            leftButton.SetActive(true);
            rightButton.SetActive(true);
            titleText.gameObject.SetActive(true);
            oakPlaygroundText.gameObject.SetActive(true);
        
            // activate overlay
            overlay.SetActive(false);
            
            // scroll up demo menu
            demoMenu.SetActive(false);
        }

        /*
         * Helper. Open website.
         */
        public void OpenWebsite()
        {
            Application.OpenURL("https://shop.luxonis.com/?aff=7");
        }
        
        public static float EaseInQuad(float start, float end, float value)
        {
            end -= start;
            return end * value * value + start;
        }
    }
    
}