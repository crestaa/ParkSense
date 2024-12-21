import express from 'express';
import { Client } from 'pg';

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

// Enable CORS for development
app.use((req, res, next) => {
  res.header('Access-Control-Allow-Origin', '*');
  res.header('Access-Control-Allow-Headers', 'Origin, X-Requested-With, Content-Type, Accept');
  next();
});

// Get latest sensor readings
app.get('/api/latest', async (req, res) => {
  try {
    const result = await dbClient.query(
      'SELECT * FROM sensor_data ORDER BY timestamp DESC LIMIT 1'
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
      'SELECT * FROM sensor_data WHERE timestamp > NOW() - INTERVAL $1 HOUR ORDER BY timestamp DESC',
      [hours]
    );
    res.json(result.rows);
  } catch (error) {
    console.error('Database error:', error);
    res.status(500).json({ error: 'Internal server error' });
  }
});

// Start server
async function startServer() {
  try {
    await dbClient.connect();
    console.log('Connected to database');
    
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