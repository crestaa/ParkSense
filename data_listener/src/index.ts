import * as mqtt from 'mqtt';
import { Client } from 'pg';

// Database configuration
const dbConfig = {
  host: process.env.POSTGRES_HOST || 'database',
  database: process.env.POSTGRES_DB || 'iot_data',
  user: process.env.POSTGRES_USER || 'iot_user',
  password: process.env.POSTGRES_PASSWORD
};

// MQTT configuration
const mqttConfig = {
  host: process.env.MQTT_BROKER?.split(':')[0] || 'mqtt-broker',
  port: parseInt(process.env.MQTT_BROKER?.split(':')[1] || '1883'),
  username: process.env.MQTT_USERNAME,
  password: process.env.MQTT_PASSWORD
};

interface Measurement {
  sensor_id: string;  // Now part of the JSON payload
  temperature?: number;
  humidity?: number;
  distance?: number;
}

async function setupDatabase() {
  const client = new Client(dbConfig);
  await client.connect();
  
  // Load and execute initialization script
  const initScript = `
    -- Create SENSORS table
    CREATE TABLE IF NOT EXISTS sensors (
        id VARCHAR(255) PRIMARY KEY,
        latitude FLOAT NOT NULL,
        longitude FLOAT NOT NULL
    );

    -- Create MEASUREMENTS table
    CREATE TABLE IF NOT EXISTS measurements (
        id SERIAL PRIMARY KEY,
        sensor_id VARCHAR(255) NOT NULL,
        distance FLOAT,
        temperature FLOAT,
        humidity FLOAT,
        timestamp TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
        FOREIGN KEY (sensor_id) REFERENCES sensors(id)
    );

    -- Create indexes
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
  
  // Setup database
  const dbClient = await setupDatabase();
  console.log('Database connected and initialized');
  
  // Connect to MQTT broker
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
      const data = JSON.parse(message.toString()) as Measurement;
      
      if (!data.sensor_id) {
        throw new Error('Missing sensor_id in message payload');
      }
      
      // Insert data into measurements table
      const query = `
        INSERT INTO measurements 
        (sensor_id, temperature, humidity, distance) 
        VALUES ($1, $2, $3, $4)
      `;
      
      await dbClient.query(query, [
        data.sensor_id,
        data.temperature || null,
        data.humidity || null,
        data.distance || null
      ]);
      
      console.log('Measurement saved for sensor:', data.sensor_id);
    } catch (error) {
      console.error('Error processing message:', error);
    }
  });
  
  // Error handling
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