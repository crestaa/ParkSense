import * as mqtt from 'mqtt';
import { Client } from 'pg';

const dbConfig = {
  host: process.env.POSTGRES_HOST || 'database',
  database: process.env.POSTGRES_DB || 'iot_data',
  user: process.env.POSTGRES_USER || 'iot_user',
  password: process.env.POSTGRES_PASSWORD
};

const mqttConfig = {
  host: process.env.MQTT_BROKER?.split(':')[0] || 'mqtt-broker',
  port: parseInt(process.env.MQTT_BROKER?.split(':')[1] || '1883'),
  username: process.env.MQTT_USERNAME,
  password: process.env.MQTT_PASSWORD
};

interface SensorData {
  t: number;    // temperature
  h: number;    // humidity
  d: number;    // distance
  m: string;    // sensor_id (MAC address)
  b: number;    // boot_count (not used)
}

async function setupDatabase() {
  const client = new Client(dbConfig);
  await client.connect();
  
  // initialization script
  const initScript = `
    CREATE TABLE IF NOT EXISTS measurements (
        id SERIAL PRIMARY KEY,
        sensor_id VARCHAR(255) NOT NULL,
        distance FLOAT,
        temperature FLOAT,
        humidity FLOAT,
        timestamp TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
    );

    CREATE INDEX IF NOT EXISTS idx_measurements_timestamp 
        ON measurements(timestamp);
    CREATE INDEX IF NOT EXISTS idx_measurements_sensor_id 
        ON measurements(sensor_id);
  `;
  
  await client.query(initScript);
  console.log('Database schema initialized');
  
  return client;
}

async function main() {
  console.log('Starting data listener service...');
  
  const dbClient = await setupDatabase();
  console.log('Database connected and initialized');
  
  const mqttClient = mqtt.connect({
    host: mqttConfig.host,
    port: mqttConfig.port,
    username: mqttConfig.username,
    password: mqttConfig.password
  });
  
  mqttClient.on('connect', () => {
    console.log('Connected to MQTT broker');
    mqttClient.subscribe('sensors', (err) => {
      if (err) {
        console.error('Subscription error:', err);
      } else {
        console.log('Subscribed to sensors topic');
      }
    });
  });
  
  mqttClient.on('message', async (_topic, message) => {
    try {
      const data = JSON.parse(message.toString()) as SensorData;
      
      if (!data.m) {
        throw new Error('Missing sensor_id (m) in message payload');
      }
      
      const query = `
        INSERT INTO measurements 
        (sensor_id, temperature, humidity, distance) 
        VALUES ($1, $2, $3, $4)
      `;
      
      // convert undefined to null
      const params = [
        data.m ?? null,                                        // sensor_id
        typeof data.t !== 'undefined' ? data.t : null,        // temperature
        typeof data.h !== 'undefined' ? data.h : null,        // humidity
        typeof data.d !== 'undefined' ? data.d : null         // distance
      ];

      await dbClient.query(query, params);
      
      console.log(`Measurement saved - Sensor: ${data.m}, Temp: ${data.t}°C, Humidity: ${data.h}%, Distance: ${data.d}m`);
    } catch (error) {
      console.error('Error processing message:', error);
    }
  });
  
  mqttClient.on('error', (error) => {
    console.error('MQTT error:', error);
  });
  
  process.on('SIGTERM', async () => {
    console.log('Shutting down...');
    await dbClient.end();
    mqttClient.end();
  });
}

main().catch(console.error);