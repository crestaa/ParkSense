const express = require('express');
const { Client } = require('pg');
const path = require('path');

const app = express();
const port = process.env.PORT || 3000;

// Database configuration
const dbConfig = {
  host: process.env.POSTGRES_HOST || 'database',
  database: process.env.POSTGRES_DB || 'iot_data',
  user: process.env.POSTGRES_USER || 'iot_user',
  password: process.env.POSTGRES_PASSWORD
};

// Create database client
const dbClient = new Client(dbConfig);

app.use(express.json());

// Serve static files from the public directory
app.use(express.static(path.join(__dirname, 'public')));

// Enable CORS for development
app.use((req, res, next) => {
  res.header('Access-Control-Allow-Origin', '*');
  res.header('Access-Control-Allow-Headers', 'Origin, X-Requested-With, Content-Type, Accept');
  res.header('Access-Control-Allow-Methods', 'GET, POST, DELETE');
  next();
});

// Get all sensors
app.get('/api/sensors', async (req, res) => {
  try {
    const result = await dbClient.query(
      'SELECT id AS sensor_id, latitude, longitude FROM sensors ORDER BY id'
    );
    res.json(result.rows);
  } catch (error) {
    console.error('Database error:', error);
    res.status(500).json({ error: 'Internal server error' });
  }
});

// Add new sensor
app.post('/api/sensors', async (req, res) => {
  const { sensor_id, latitude, longitude } = req.body;

  // Validate input
  if (!sensor_id || latitude === undefined || longitude === undefined) {
    return res.status(400).json({ message: 'All fields are required' });
  }

  try {
    // Check if sensor_id already exists
    const existing = await dbClient.query(
      'SELECT id FROM sensors WHERE id = $1',
      [sensor_id]
    );

    if (existing.rows.length > 0) {
      return res.status(409).json({ message: 'Sensor ID already exists' });
    }

    // Insert new sensor
    await dbClient.query(
      'INSERT INTO sensors (id, latitude, longitude) VALUES ($1, $2, $3)',
      [sensor_id, latitude, longitude]
    );

    res.status(201).json({ message: 'Sensor added successfully' });
  } catch (error) {
    console.error('Database error:', error);
    res.status(500).json({ message: 'Failed to add sensor' });
  }
});

// Delete sensor
app.delete('/api/sensors/:id', async (req, res) => {
  const sensorId = req.params.id;

  try {
    // First delete all measurements for this sensor due to foreign key constraint
    await dbClient.query(
      'DELETE FROM measurements WHERE sensor_id = $1',
      [sensorId]
    );

    // Then delete the sensor
    const result = await dbClient.query(
      'DELETE FROM sensors WHERE id = $1',
      [sensorId]
    );

    if (result.rowCount === 0) {
      return res.status(404).json({ message: 'Sensor not found' });
    }

    res.json({ message: 'Sensor deleted successfully' });
  } catch (error) {
    console.error('Database error:', error);
    res.status(500).json({ message: 'Failed to delete sensor' });
  }
});

// Get all measurements
app.get('/api/measurements', async (req, res) => {
  try {
    const result = await dbClient.query(
      'SELECT m.sensor_id, m.temperature, m.humidity, m.distance, m.timestamp ' +
      'FROM measurements m ' +
      'JOIN sensors s ON m.sensor_id = s.id ' +
      'ORDER BY m.timestamp DESC LIMIT 100'
    );
    res.json(result.rows);
  } catch (error) {
    console.error('Database error:', error);
    res.status(500).json({ error: 'Internal server error' });
  }
});

// Get latest sensor readings
app.get('/api/latest', async (req, res) => {
  try {
    const result = await dbClient.query(
      'SELECT m.* FROM measurements m ' +
      'JOIN sensors s ON m.sensor_id = s.id ' +
      'ORDER BY m.timestamp DESC LIMIT 1'
    );
    res.json(result.rows[0] || {});
  } catch (error) {
    console.error('Database error:', error);
    res.status(500).json({ error: 'Internal server error' });
  }
});

// Get historical data
app.get('/api/history', async (req, res) => {
  try {
    const { hours = 24 } = req.query;
    const result = await dbClient.query(
      'SELECT m.* FROM measurements m ' +
      'JOIN sensors s ON m.sensor_id = s.id ' +
      'WHERE m.timestamp > NOW() - INTERVAL $1 HOUR ' +
      'ORDER BY m.timestamp DESC',
      [hours]
    );
    res.json(result.rows);
  } catch (error) {
    console.error('Database error:', error);
    res.status(500).json({ error: 'Internal server error' });
  }
});

// Get latest measurements for all sensors
app.get('/api/latest-all', async (req, res) => {
  try {
    const result = await dbClient.query(
      `WITH LatestMeasurements AS (
        SELECT DISTINCT ON (sensor_id)
          sensor_id, distance, temperature, humidity, timestamp
        FROM measurements
        ORDER BY sensor_id, timestamp DESC
      )
      SELECT 
        l.*
      FROM LatestMeasurements l
      JOIN sensors s ON l.sensor_id = s.id
      ORDER BY s.id`
    );
    res.json(result.rows);
  } catch (error) {
    console.error('Database error:', error);
    res.status(500).json({ error: 'Internal server error' });
  }
});


// Function to try connecting to the database
async function connectWithRetry(maxAttempts = 5, delay = 5000) {
  for (let attempt = 1; attempt <= maxAttempts; attempt++) {
    try {
      await dbClient.connect();
      console.log('Connected to database');
      return true;
    } catch (err) {
      console.log(`Connection attempt ${attempt}/${maxAttempts} failed:`, err.message);
      if (attempt === maxAttempts) {
        throw err;
      }
      await new Promise(resolve => setTimeout(resolve, delay));
    }
  }
}

// Start server
async function startServer() {
  try {
    // Try to connect to the database with retries
    await connectWithRetry();
    
    app.listen(port, () => {
      console.log(`Web server listening on port ${port}`);
    });
  } catch (error) {
    console.error('Failed to start server:', error);
    process.exit(1);
  }
}

// Handle shutdown
process.on('SIGTERM', async () => {
  console.log('Shutting down...');
  await dbClient.end();
  process.exit(0);
});

startServer().catch(console.error);