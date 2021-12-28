using System;
using UnityEditor;
using UnityEngine;
using System.Runtime.InteropServices;
using SimpleJSON;

namespace OAKForUnity
{
    public class OAKDeviceManagerWindow : EditorWindow
    {

        [DllImport("depthai-unity", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr GetAllDevices();

        // Scroll position
        private Vector2 _scrollPos = Vector2.zero;
        // device information
        private string _devicesJson = "";
        private string _devices = "";

        [MenuItem("OAK For Unity/OAK Device Manager")]
        public static void ShowWindow()
        {
            OAKDeviceManagerWindow w = EditorWindow.GetWindow<OAKDeviceManagerWindow>();
            GUIContent titleContent = new GUIContent("OAK Device Manager");
            w.titleContent = titleContent;
            w.Show();
        }

        void OnGUI()
        {
            GUILayout.BeginVertical();
            _scrollPos = GUILayout.BeginScrollView(_scrollPos, false, true);
            Texture banner =
                AssetDatabase.LoadAssetAtPath<Texture>("Assets/Plugins/OAKForUnity/Editor/Resources/banner.png");

            // Press Check devices button
            if (GUILayout.Button("Check devices"))
            {
                _devices = "";
                _devicesJson = Marshal.PtrToStringAnsi(GetAllDevices());
                if (_devicesJson != "" && _devicesJson != "null")
                {
                    var obj = JSON.Parse(_devicesJson);
                    _devices += "Device ID\t\t\tDevice State\t\t\tDevice Name\n";
                    // foreach device parse information
                    foreach (JSONNode arr in obj)
                    {
                        string deviceId = arr["deviceId"];
                        string deviceName = arr["deviceName"];
                        string deviceState = arr["deviceState"];

                        // build line with device information
                        _devices = _devices + deviceId + "\t\t" + deviceState + "\t\t\t" + deviceName + "\n";
                    }
                }
            }

            GUILayout.TextArea(_devicesJson == "null" ? "NO AVAILABLE DEVICES" : _devices, EditorStyles.boldLabel);

            GUILayout.Box(banner);

            GUILayout.EndScrollView();
            GUILayout.EndVertical();
        }
    }
}