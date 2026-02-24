#include "DFVOC.h"

/// @brief Creates a new VOC sensor object
/// @param Name The device name
/// @param I2C_bus The I2C bus attached to the sensor
/// @param ens160_address Address of the ENS160 sensor
/// @param bme280_address Address of the BME280 sensor
/// @param ConfigFile The file name to store settings in
DFVOC::DFVOC(String Name, TwoWire* I2C_bus, uint8_t ens160_address, uint8_t bme280_address, String ConfigFile) : ens160_sensor(I2C_bus, ens160_address), bme280_sensor(I2C_bus, bme280_address), Sensor(Name) {
	config_path = "/settings/sen/" + ConfigFile;
	i2c_bus = I2C_bus;
}

/// @brief Creates a new VOC sensor object
/// @param Name The device name
/// @param sda SDA pin to use for I2C bus
/// @param scl SCL pin to use for I2C bus
/// @param I2C_bus The I2C bus attached to the sensor
/// @param ens160_address Address of the ENS160 sensor
/// @param bme280_address Address of the BME280 sensor
/// @param ConfigFile The file name to store settings in
DFVOC::DFVOC(String Name, int sda, int scl, TwoWire* I2C_bus, uint8_t ens160_address, uint8_t bme280_address, String ConfigFile) : ens160_sensor(I2C_bus, ens160_address), bme280_sensor(I2C_bus, bme280_address), Sensor(Name) {
	config_path = "/settings/sen/" + ConfigFile;
	i2c_bus = I2C_bus;
	scl_pin = scl;
	sda_pin = sda;
}

/// @brief Starts the VOC sensor
/// @return True on success
bool DFVOC::begin() {
	Description.parameterQuantity = 7;
	Description.type = "Multi Function Environmental Module";
	Description.parameters = {"AQI", "TVOC", "eCO2", "Temperature", "Humidity", "Pressure", "Altitude"};
	Description.units = {"level", "ppb", "ppm", "C", "%RH", "hPa", "m"};
	values.resize(Description.parameterQuantity);

	bool result = false;
	// Create settings directory if necessary
	if (!checkConfig(config_path)) {
		// Set defaults
		result = setConfig(getConfig(), true);
	} else {
		// Load settings
		result = setConfig(Storage::readFile(config_path), false);
	}

	if (result) {
		// Start I2C bus if not started
		if (scl_pin > -1 && sda_pin > -1) {
			result = i2c_bus->begin(sda_pin, scl_pin);
		} else {
			result = i2c_bus->begin();
		}
	}

	if (result) {
		// Initialize BME280
		bme280_sensor.reset();
		if (bme280_sensor.begin() != DFRobot_BME280_IIC::eStatusOK) {
			return false;
		}
		
		// Initialize ENS160
		if (ens160_sensor.begin() != NO_ERR) {
			return false;
		}

		// Set ENS160 to standard mode
		ens160_sensor.setPWRMode(ENS160_STANDARD_MODE);
	}

	return result;
}

/// @brief Takes a measurement
/// @return True on success
bool DFVOC::takeMeasurement() {
	// Check ENS160 status
	uint8_t status = ens160_sensor.getENS160Status();
	if (status != 0) {
		if (status == 1) {
			Logger.println("Sensor still warming up. Wait up to 3 minutes from power on and try again.");
		} else {
			Logger.println("Initial sensor burn-in. Leave sensor on for one hour to complete first-use burn in.");
		}
		return false;
	}

	// Get temperature and humidity from BME280 for ENS160 compensation
	float temperature = bme280_sensor.getTemperature();
	float humidity = bme280_sensor.getHumidity();

	// Set temperature and humidity compensation for ENS160
	ens160_sensor.setTempAndHum(temperature, humidity);

	values[0] = ens160_sensor.getAQI();
	values[1] = ens160_sensor.getTVOC();
	values[2] = ens160_sensor.getECO2();
	values[3] = temperature;
	values[4] = humidity;
	uint32_t pressure = bme280_sensor.getPressure();
	values[5] = ((double)pressure) / 100;
	values[6] = bme280_sensor.calAltitude(current_config.pressureSeaLevel, pressure);
	return true;
}

/// @brief Gets the current config
/// @return A JSON string of the config
String DFVOC::getConfig() {
	// Allocate the JSON document
	JsonDocument doc;
	// Assign current values
	doc["Name"] = Description.name;
	doc["pressureSeaLevel"] = current_config.pressureSeaLevel;

	// Create string to hold output
	String output;
	// Serialize to string
	serializeJson(doc, output);
	return output;
}

/// @brief Sets the configuration for this device
/// @param config A JSON string of the configuration settings
/// @param save If the configuration should be saved to a file
/// @return True on success
bool DFVOC::setConfig(String config, bool save) {
	// Allocate the JSON document
	JsonDocument doc;
	// Deserialize file contents
	DeserializationError error = deserializeJson(doc, config);
	// Test if parsing succeeds.
	if (error) {
		Logger.print(F("Deserialization failed: "));
		Logger.println(error.f_str());
		return false;
	}
	// Assign loaded values
	Description.name = doc["Name"].as<String>();
	current_config.pressureSeaLevel = doc["pressureSeaLevel"].as<float>();
	if (save) {
		return saveConfig(config_path, config);
	}
	return true;
}