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

-- Create index on timestamp for better query performance
CREATE INDEX IF NOT EXISTS idx_measurements_timestamp 
    ON measurements(timestamp);

-- Create index on sensor_id for better join performance
CREATE INDEX IF NOT EXISTS idx_measurements_sensor_id 
    ON measurements(sensor_id);