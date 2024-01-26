/** Basic implementation of TCP socket client using Netly
 *
 *
 * 
 */

using UnityEngine;
using System;
using System.IO;
using System.Net.Sockets;
using System.Threading;
using Netly.Core;
using TcpClient = Netly.TcpClient;

// Based on TCP Client example provided by Netly

public class TcpClientBehaviour : MonoBehaviour
{
    public string host;
    public int port;
    
    private TcpClient client;
    private Thread clientThread;

    private Texture2D _texture;
    private string _json;
    private byte[] _pendingImageData = null;
    private byte[] _pendingJsonData = null;
   
    private bool _connected;
    
    int bytesRead;
    MemoryStream jsonMemoryStream = new MemoryStream();
    MemoryStream memoryStream = new MemoryStream();

    private int _getJson = 1;
    void Start()
    {
        _connected = false;
        // Initialize the client and connect in a separate thread
        client = new TcpClient(framing: false); 
        
        _texture = new Texture2D(2, 2);
        _json = "";
        
        client.OnOpen(() =>
        {
            Debug.Log("Client connected");
            client.ToData("DATA");
            _connected = true;
        });

        client.OnClose(() =>
        {
            Debug.Log("Client disconnected");
        });

        client.OnError((Exception exception) =>
        {
            Debug.LogError("Connection error: " + exception.Message);
        });

        client.OnData((byte[] data) =>
        {
            // Handle incoming data
            // Remember to marshal this call back to the main thread if you're updating Unity objects
            //Debug.Log("DATA: "+data.Length+" 0:"+data[0]);
            
            // parse json results
            if (_getJson == 1)
            {
                if (data.Length == 32768)
                {
                    jsonMemoryStream.Write(data, 0, data.Length);
                }
                else
                {
                    jsonMemoryStream.Write(data, 0, data.Length);
                    byte[] totalData = jsonMemoryStream.ToArray();
                    _pendingJsonData = new byte[totalData.Length];
                    Array.Copy(totalData, 0, _pendingJsonData, 0, _pendingJsonData.Length);
                    
                    jsonMemoryStream.SetLength(0);
                    jsonMemoryStream.Position = 0;
                    _getJson = 2;

                }
            }
            else if (_getJson == 2)
            {
                // parse image
                if (data.Length == 32768)
                {
                    memoryStream.Write(data, 0, data.Length);
                }
                else
                {
                    memoryStream.Write(data, 0, data.Length);

                    byte[] totalData = memoryStream.ToArray();

                    _pendingImageData = new byte[totalData.Length];
                    Array.Copy(totalData, 0, _pendingImageData, 0, _pendingImageData.Length);

                    memoryStream.SetLength(0);
                    memoryStream.Position = 0;
                    client.ToData("DATA");
                    _getJson = 0;
                }
            }
        });

        client.OnEvent((string name, byte[] data) =>
        {
            Debug.Log("Event received: " + name);
        });

        client.OnModify((Socket socket) =>
        {
            // Modify socket before opening connection
        });

    }

    public bool InitUB()
    {
        clientThread = new Thread(() => client.Open(new Host(host, port)));
        clientThread.Start();
        return true;
    }
    private void Update()
    {
        if (_pendingImageData != null)
        {
            if (_texture.LoadImage(_pendingImageData))
            {
                _json = System.Text.Encoding.UTF8.GetString(_pendingJsonData);
            }
            else
            {
                Debug.LogError("Failed to create texture from received image data");
            }
            _pendingImageData = null; // Clear the data after processing
            _pendingJsonData = null;
            _getJson = 1;
        }
    }

    public string GetResults(out Texture2D texture)
    {
        texture = _texture;
        return _json;
    }
    
    void OnDestroy()
    {
        // Close the client
        if (client != null)
        {
            client.Close();
        }
        if (clientThread != null && clientThread.IsAlive)
        {
            clientThread.Abort();
        }
    }
}