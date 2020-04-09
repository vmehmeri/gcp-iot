# gcp-iot
Google Cloud IoT Projects

## Quantified Desk
Uses arduino mkr1000 and supersonic sensor to measure standing desk height, sending information on sitting and standing durations to Google Cloud IoT Core and visualizing it on a real-time Dashboard (Python application backend and Python/Javascript frontend using Pusher API).

It also detects state changes (sitting -> standing and standing -> sitting) and logs them in a table displayed on the webpage.

![Sample visualization](https://lh3.googleusercontent.com/-tp2-5dIozVsXVUMPe6DpJnaaKTMX99GvJYOQz4Sen3-1Klc61sHjapQ8OD54iu_Y333_7Hil2pSAra3D_iG-x_VNRvhBY-EXfzEPgB24f6asTIcTTBmk7uTF9-vqTsafs5L2qJwbaOiT-vNA16n7p7mSgegY69Yu7oLWdcrKtX9X9eB9oiglJz66Gul-Tjjrb_LyyU_0aw3Tbh9Aiyog4_kgzbmL7ag3ash57njI44Lj6_6-nTzCZx02YXIZQdCINgqX_qW1s3iolBXy8Ab28HZr64tDAmxhnRSgOjoD1tjzujMCOIJgxJC40JLGuCrqA6ApaU0X5bjpLNzDfmoxytB1EqAswwebDkbwvLgkkr6f5JXVlZea76Pclmcen_shk0WIDBEiIpJighX0bipgZqOtIYc_yspTlfc9Mg7_qQHDzLzfivEvhREeRsbCz7Xt0g_WRBXmgHb3PTlGJ6X7LSz7ha3kib2NNe3uECfv3OgScUHVB9YAATv4oYMLq1haAdi1mZS2bmqMKzAVMEakFKYVYwMjy-N974mVwMG2ZwUrW_OzNBEnF4wEvIb6VA3D25bN7sANW7YAGxluEbzm0E7YlBd8QoA4qVFccsqE1ugABwIvOO6fbYjncUDpFILaloXDJifY-c03SbPC06KYhKSXVGkAoXxLKNUBM5N2g8UGtdgQf8rPg5daczXA_AOKjmWJYNFNezgcHURZ7IQrZ2oRjXbQS7IldmY4IlifwYq-EnNF_wvk34=w1247-h931-no)

Arduino project setup is available in this link:

https://www.hackster.io/vmehmeri/quantified-desk-iot-6647bf

(The project above mentions Azure IoT Hub, but the Arduino setup is the same)

### Requirements
As a pre-requisite, a Google Cloud IoT Core must be setup with a Cloud Pub/Sub Topic and Subscription for the device's telemetry data streaming.
Your Arduino device will also require SSL certificates for connecting to IoT core's MQTT endpoint.

- **Instructions for setting up Google Cloud IoT Core:**
  https://cloud.google.com/iot/docs/quickstart

- **Instructions for updating Firmware and adding SSL certificate to device:**
  https://www.arduino.cc/en/Tutorial/FirmwareUpdater

This project was developed using Python 3.7.

Create a virtual environment with version 3.7 and install the packages below.

#### Backend:

- google-cloud-pubsub
- pusher

#### Frontend:

- pusher
- flask

All can be installed with pip

#### Arduino Libraries:
- Ultrasonic 
- WiFi101 
- RTCZero
- WiFiUdp
- MQTT
- CloudIoTCore
- CloudIoTCoreMqtt
- jwt

All those libraries are already available in Arduino Web Editor. The latest code can also be accessed as a Web Editor sketch through the link below:

[Arduino Web Sketch](https://create.arduino.cc/editor/vmehmeri/5ba7a532-8c2b-4a67-be11-bf0912d3dc7b/preview "Arduino Web Sketch")


### Usage
First edit **config.py** and configure your connection details and secret keys. You will need to create an account in Pusher and an application (it's free)

**config.py (replace values):**
```
from pusher import Pusher

pusher = Pusher(
    app_id='APP_ID',
    key = "KEY",
    secret = "SECRET",
    cluster = "CLUSTER",
    ssl=True
)

## GCP Config
project_id = "PROJECT_ID"
topic_name = "projects/PROJECT_ID/topics/TOPIC"
subscription_name = "SUBSCRIPTION_NAME"
```

**Start frontend**
```
    python app.py
```

**On a separate terminal, start backend**
```
    python device_broker.py
```

Next open a browser pointing to localhost:5000 and you should see your dashboard.
