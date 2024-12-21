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

async function setupDatabase() {
  const client = new Client(dbConfig);
  await client.connect();
  
  // Create table if it doesn't exist
  await client.query(`
    CREATE TABLE IF NOT EXISTS sensor_data (
      id SERIAL PRIMARY KEY,
      temperature FLOAT,
      humidity FLOAT,
      distance FLOAT,
      timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP
    );
  `);
  
  return client;
}

async function main() {
  console.log('Starting data listener service...');
  
  // Setup database
  const dbClient = await setupDatabase();
  console.log('Database connected and initialized');
  
  // Connect to MQTT broker with credentials
  const mqttClient = mqtt.connect({
    host: mqttConfig.host,
    port: mqttConfig.port,
    username: mqttConfig.username,
    password: mqttConfig.password
  });
  
  mqttClient.on('connect', () => {
    console.log('Connected to MQTT broker');
    mqttClient.subscribe('sensors/#', (err) => {
      if (err) {
        console.error('Subscription error:', err);
      } else {
        console.log('Subscribed to sensors/#');
      }
    });
  });
  
  mqttClient.on('message', async (topic, message) => {
    try {
      const data = JSON.parse(message.toString());
      
      // Insert data into database
      await dbClient.query(
        'INSERT INTO sensor_data (temperature, humidity, distance) VALUES ($1, $2, $3)',
        [data.temperature, data.humidity, data.distance]
      );
      
      console.log('Data saved:', data);
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