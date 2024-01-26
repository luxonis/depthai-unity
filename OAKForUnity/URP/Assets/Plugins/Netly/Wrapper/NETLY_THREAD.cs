using UnityEngine;

#if UNITY_EDITOR
using UnityEditor;
#endif
namespace Netly.Abstract
{
    [AddComponentMenu("NETLY THREAD")]
    public class NETLY_THREAD : MonoBehaviour
    {
        private static NETLY_THREAD _Self { get; set; } = null;
        private static bool _IsQuit { get; set; } = false;


        private void Awake()
        {
            Application.quitting += () =>
            {
                _IsQuit = true;
            };

            if (_Self == null || _Self == this)
            {
                _Self = this;
                transform.position = Vector3.zero;
                transform.rotation = Quaternion.identity;
                gameObject.transform.parent = null;
                gameObject.name = "NETLY [RUNTIME]";
                DontDestroyOnLoad(gameObject);
            }
            else
            {
                Destroy(gameObject);
            }
        }

        private void Update()
        {
            Netly.Core.MainThread.Automatic = false;
            Netly.Core.MainThread.Clean();
        }

        private void OnDisable()
        {
            Destroy(gameObject);
        }

        private void OnDestroy()
        {
            if (_Self != null && _Self == this)
            {
                _Self = null;
            }

            GenerateNew();
        }

        private static void GenerateNew()
        {
            if (Application.isPlaying && _IsQuit == false)
            {
                var o = new GameObject("NETLY RUNTIME");
                o.isStatic = true;
                var s = o.AddComponent<NETLY_THREAD>();
            }
        }

        [RuntimeInitializeOnLoadMethod(RuntimeInitializeLoadType.AfterSceneLoad)]
        static void OnAfterSceneLoad()
        {
            GenerateNew();
        }

    }

#if UNITY_EDITOR
    [CustomEditor(typeof(NETLY_THREAD))]
    public class NETLY_THREAD_EDITOR : Editor
    {
        private NETLY_THREAD self => (NETLY_THREAD)target;

        public override void OnInspectorGUI()
        {
            GUILayout.Box("Netly main thread dispacher");
            self.gameObject.SetActive(true);

            if (!Application.isPlaying)
            {
                DestroyImmediate(self.gameObject);
            }
        }
    }
#endif
}