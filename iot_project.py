import time
import paho.mqtt.client as mqtt

# MQTT settings
BROKER = "broker.hivemq.com"
PORT = 1883
TOPIC = "iot/sensor/data"

def on_connect(client, userdata, flags, rc):
    print("Connected with result code " + str(rc))
    client.subscribe(TOPIC)

def on_message(client, userdata, msg):
    print(f"Received message: {msg.payload.decode()} on topic {msg.topic}")

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

client.connect(BROKER, PORT, 60)

# Simulate sensor data publishing
def publish_sensor_data():
    while True:
        # Simulate sensor reading (e.g., temperature)
        sensor_value = 25.5  # Replace with actual sensor read
        client.publish(TOPIC, f"Temperature: {sensor_value}°C")
        time.sleep(5)  # Publish every 5 seconds

# Start the loop
client.loop_start()
publish_sensor_data()