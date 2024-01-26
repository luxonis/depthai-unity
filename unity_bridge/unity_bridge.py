import socket
import json
import cv2
import _thread as thread
import time

class UnityBridge:
    def __init__(self, address):
        self.address = address
        self.socket = None
        self.running = False
        self.image = None
        self.names = None
        self.objects = None
        self.configs = None
        self.data = None
        self.count = 0

    def start(self):
        """ Start the networking thread. """
        self.running = True
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        self.socket.bind(self.address) 
        print("Server Started!")
        self.socket.listen(10)
        thread.start_new_thread(self._run, ())

    def send(self, image, names, objects, configs):
        self.image = image
        self.names = names
        self.objects = objects
        self.configs = configs

    def close(self):
        """ Close the socket connection. """
        self.running = False
        if self.socket:
            self.socket.close()

    def _serialize_objects(self, key_names, objects, configs):
        # Ensure that key_names, objects, and configs have the same length
        if not (len(key_names) == len(objects) == len(configs)):
            raise ValueError("Length of key_names, objects, and configs must be the same.")

        if len(key_names) != len(set(key_names)):
            raise ValueError("Key names must be unique.")

        # Initialize a dictionary to store the serialized data
        serialized_data = {}#{key: [] for key in key_names}

        for obj, config, key_name in zip(objects, configs, key_names):
            serialized_obj = {field: getattr(obj, field) for field in config if hasattr(obj, field)}
            serialized_data[key_name] = serialized_obj #.append(serialized_obj)

        return serialized_data


    def client(self,conn, addr):
        while True:

            # Wait for DATA command

            try:
                data = bytearray()
                while len(data) < 4:
                    packet = conn.recv(4)
                    if not packet:
                        break
                    if len(packet) == 0:
                        break

                    data.extend(packet)

            except:
                self._error(conn, addr)
                break

            if not data.decode('utf-8'):
                continue

            # Received DATA command

            self.data = self._serialize_objects(self.names, self.objects,self.configs)
            self._send_data(conn, self.image,self.data)

            time.sleep(0.05) 

        conn.close()
        print('Disconnected ', addr)

    def _error(self,conn, addr):
        try:
            print("Error.")
        except:
            print(addr, "Disconnected.")

    def _run(self):
        """ The main loop for the networking thread. """
        while True:
            print("Listening...")
            try:
                conn, addr = self.socket.accept()
                print('Connected with ', addr)
                thread.start_new_thread(self.client, (conn, addr))
            except:
                break


    def _send_data(self, conn, image, data):
        """ Send the image and serialized data over the socket. """
        ret, encoded_image = cv2.imencode('.jpg', image)
        if not ret:
            print("Could not encode image")
            return

        image_data = encoded_image.tobytes()
        json_data = json.dumps(data).encode('utf-8')

        try:
            conn.sendall(json_data)
            conn.sendall(image_data)
            self.count = self.count + 1
        except socket.error as e:
            print(f"Error sending data: {e}")


class TestObject:
    def __init__(self, result):
        self.result = result
        self.field1 = None
        self.arr1 = []
