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
using System.Text;
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
    private const string DELIMITER = "<<END_OF_JSON>>";
    private const string DELIMITER_END = "<<END>>";
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
            //Debug.Log("GET JSON: "+_getJson);
            //Debug.Log("DATA: "+data.Length+" 0:"+data[0]);
            
            byte[] delimiterBytes = Encoding.ASCII.GetBytes(DELIMITER); 
            byte[] delimiterEndBytes = Encoding.ASCII.GetBytes(DELIMITER_END);
            
            if (data == delimiterBytes) 
            {
            	_getJson=2;
            }
            
            if (_getJson == 1)
			{
			    jsonMemoryStream.Write(data, 0, data.Length);

			    // Check if the delimiter is in the received data
			    int delimiterIndex = FindDelimiterIndex(jsonMemoryStream.ToArray(), delimiterBytes);
			    if (delimiterIndex >= 0) // Delimiter found
			    {
					// Split the data at the delimiter
					int jsonPartLength = delimiterIndex;
					int imagePartStartIndex = delimiterIndex + delimiterBytes.Length;

					// Extract JSON data
					_pendingJsonData = new byte[jsonPartLength];
					Array.Copy(jsonMemoryStream.ToArray(), 0, _pendingJsonData, 0, jsonPartLength);
					// Prepare for image data reception
					jsonMemoryStream.SetLength(0); // Reset the JSON memory stream
					jsonMemoryStream.Position = 0;

					if (data.Length > imagePartStartIndex) // If there's image data following the delimiter
					{
					    // Write the initial part of the image data to the memoryStream
					    memoryStream.Write(data, imagePartStartIndex, data.Length - imagePartStartIndex);
					}

					_getJson = 2; // Move to image parsing state
			    }
			}
			else if (_getJson == 2)
			{
			    // Parse image
			    memoryStream.Write(data, 0, data.Length);
                
                int delimiterEndIndex = FindDelimiterIndex(memoryStream.ToArray(), delimiterEndBytes);
                if (delimiterEndIndex >= 0) // Delimiter found
			    {
                    byte[] totalData = memoryStream.ToArray();
					_pendingImageData = new byte[totalData.Length];
					Array.Copy(totalData, 0, _pendingImageData, 0, _pendingImageData.Length-delimiterEndBytes.Length);

					memoryStream.SetLength(0);
					memoryStream.Position = 0;
					_getJson = 0; // Reset or move to the next state as needed
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

    // Helper method to find the delimiter in the data
    int FindDelimiterIndex(byte[] data, byte[] delimiter)
    {
        for (int i = 0; i < data.Length - delimiter.Length+1; i++)
        {
            bool match = true;
            for (int j = 0; j < delimiter.Length; j++)
            {
                if (data[i + j] != delimiter[j])
                {
                    match = false;
                    break;
                }
            }
            if (match) return i;
        }
        return -1; // Delimiter not found
    }
    
    public bool InitUB()
    {
        clientThread = new Thread(() => client.Open(new Host(host, port)));
        clientThread.Start();
        return true;
    }
    private void Update()
    {
        if (_pendingImageData != null && _getJson == 0)
        {
            if (!_texture.LoadImage(_pendingImageData))
            {
                Debug.LogError("Failed to create texture from received image data");
            }
            _json = System.Text.Encoding.UTF8.GetString(_pendingJsonData);
            _pendingImageData = null; // Clear the data after processing
            _pendingJsonData = null;
            _getJson = 1;
            client.ToData("DATA");
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
