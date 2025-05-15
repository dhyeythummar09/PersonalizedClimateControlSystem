const mqtt = require("mqtt");
const mysql = require("mysql2");

// MQTT Configuration - EMQX Cloud WSS
const mqttUrl = "wss://<YOUR-EMQX-ENDPOINT>:<PORT>/mqtt";
const options = {
  username: "<MQTT_USERNAME>",
  password: "<MQTT_PASSWORD>",
  connectTimeout: 4000,
  reconnectPeriod: 2000,
};

// Heat Index Calculation
function calculateHeatIndex(tempC, humidity) {
  const T = tempC;
  const RH = humidity;

  const HI = -8.78469475556 
    + 1.61139411 * T 
    + 2.33854883889 * RH 
    - 0.14611605 * T * RH 
    - 0.012308094 * T * T 
    - 0.0164248277778 * RH * RH 
    + 0.002211732 * T * T * RH 
    + 0.00072546 * T * RH * RH 
    - 0.000003582 * T * T * RH * RH;

  return HI;
}

const topic = "esp32/sensors"; 

// MySQL Configuration
const db = mysql.createConnection({
  host: "<MYSQL_HOST>",
  user: "<MYSQL_USER>",
  password: "<MYSQL_PASSWORD>",
  database: "<MYSQL_DATABASE>"
});

// MySQL Connection
db.connect((err) => {
  if (err) throw err;
  console.log("✅ Connected to MySQL database");
});

// Create table if not exists
const createTableQuery = `
  CREATE TABLE IF NOT EXISTS sensor_data (
    id INT AUTO_INCREMENT PRIMARY KEY,
    temperature FLOAT,
    humidity FLOAT,
    co2_level INT,
    feels_like FLOAT,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP
  )
`;
db.query(createTableQuery, (err) => {
  if (err) throw err;
  console.log("✅ Table checked/created");
});

// Connect to MQTT
const client = mqtt.connect(mqttUrl, options);

client.on("connect", () => {
  console.log("✅ Connected to MQTT broker");
  client.subscribe(topic);
});

client.on("message", (topic, message) => {
  try {
    const data = JSON.parse(message.toString());
    console.log("📩 Received data:", data);

    const temperature = parseFloat(data.temperature);
    const humidity = parseFloat(data.humidity);
    const co2 = parseInt(data.air_quality);

    if (isNaN(temperature) || isNaN(humidity) || isNaN(co2)) {
      return console.error("❌ Invalid data received:", data);
    }

    const feelsLike = calculateHeatIndex(temperature, humidity);

    const query = `
      INSERT INTO sensor_data (temperature, humidity, co2_level, feels_like)
      VALUES (?, ?, ?, ?)
    `;

    db.query(query, [temperature, humidity, co2, feelsLike], (err, results) => {
      if (err) return console.error("❌ MySQL Insert Error:", err);
      console.log("📥 Data inserted with ID:", results.insertId);
      console.log("📊 Values:", { temperature, humidity, co2, feelsLike });
    });
  } catch (err) {
    console.error("❌ MQTT Message Error:", err);
    console.error("Message received was:", message.toString());
  }
});

client.on("error", (err) => {
  console.error("❌ MQTT Connection Error:", err);
});

// Graceful shutdown
process.on('SIGINT', () => {
  console.log('Closing connections...');
  client.end();
  db.end();
  process.exit();
});
