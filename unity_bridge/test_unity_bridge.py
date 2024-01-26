from unity_bridge import UnityBridge, TestObject
import json 
import os
import cv2
import depthai as dai
import time

# Unity Bridge Configuration
# Example usage in the main application
address = ('127.0.0.1', 12347)
unity_bridge = UnityBridge(address)
unity_bridge.start()

test_object = TestObject(result="Success")
test_object.field1 = "Field1"

test_object2 = TestObject(result="Success 2")

# Prepare data for serialization
names = ['res1','res2']
objects = [test_object,test_object2]
configs = [['result','field1'],['result','field2','arr1']]  # List of fields to serialize for each object


# ColorCamera preview example from depthai-python
# https://github.com/luxonis/depthai-python/blob/main/examples/ColorCamera/rgb_preview.py

# Create pipeline
pipeline = dai.Pipeline()

# Define source and output
camRgb = pipeline.create(dai.node.ColorCamera)
xoutRgb = pipeline.create(dai.node.XLinkOut)

xoutRgb.setStreamName("rgb")

# Properties
camRgb.setPreviewSize(300, 300)
camRgb.setInterleaved(False)
camRgb.setColorOrder(dai.ColorCameraProperties.ColorOrder.RGB)

# Linking
camRgb.preview.link(xoutRgb.input)

# Connect to device and start pipeline
with dai.Device(pipeline) as device:

    print('Connected cameras:', device.getConnectedCameraFeatures())
    # Print out usb speed
    print('Usb speed:', device.getUsbSpeed().name)
    # Bootloader version
    if device.getBootloaderVersion() is not None:
        print('Bootloader version:', device.getBootloaderVersion())
    # Device name
    print('Device name:', device.getDeviceName(), ' Product name:', device.getProductName())

    # Output queue will be used to get the rgb frames from the output defined above
    qRgb = device.getOutputQueue(name="rgb", maxSize=4, blocking=False)

    while True:
        inRgb = qRgb.get()  # blocking call, will wait until a new data has arrived

        # Retrieve 'bgr' (opencv format) frame
        cv2.imshow("rgb", inRgb.getCvFrame())

        # Unity Bridge part
        test_object2.arr1 = [unity_bridge.count]
        # Send data back to Unity
        unity_bridge.send(inRgb.getCvFrame(), names, objects, configs)


        if cv2.waitKey(1) == ord('q'):
            break

unity_bridge.close()