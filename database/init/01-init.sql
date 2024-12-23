CREATE TABLE IF NOT EXISTS sensors (
    id VARCHAR(255) PRIMARY KEY,
    latitude FLOAT NOT NULL,
    longitude FLOAT NOT NULL
);

CREATE TABLE IF NOT EXISTS measurements (
    id SERIAL PRIMARY KEY,
    sensor_id VARCHAR(255) NOT NULL,
    distance FLOAT,
    temperature FLOAT,
    humidity FLOAT,
    timestamp TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (sensor_id) REFERENCES sensors(id)
);

CREATE INDEX IF NOT EXISTS idx_measurements_timestamp 
    ON measurements(timestamp);

CREATE INDEX IF NOT EXISTS idx_measurements_sensor_id 
    ON measurements(sensor_id);



----------------------------------------------------------------------------
-- TEST DATA
INSERT INTO sensors (id, latitude, longitude)
VALUES 
    ('test', 45.408576, 11.886677),
    ('test2', 45.408630, 11.886841),
    ('test3', 45.408532, 11.886447);

INSERT INTO measurements (sensor_id, temperature, humidity, distance, timestamp)
VALUES 
    ('test', 12.2, 60.0, 18.2, CURRENT_TIMESTAMP - INTERVAL '1 hour'),
    ('test', 11.0, 58.4, 18.2, CURRENT_TIMESTAMP),
    ('test2', 6.0, 80.4, 48.2, CURRENT_TIMESTAMP - INTERVAL '1 minute'),
    ('test3', 15.3, 42.8, 122.4, CURRENT_TIMESTAMP - INTERVAL '2 minute');
    