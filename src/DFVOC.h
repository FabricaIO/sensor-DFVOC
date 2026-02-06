/*
* This file and associated .cpp file are licensed under the GPLv3 License Copyright (c) 2026 Sam Groveman
* 
* External libraries needed:
* DFRobot_ENS160: https://github.com/DFRobot/DFRobot_ENS160/
* DFRobot_BME280: https://github.com/DFRobot/DFRobot_BME280/
* 
* https://www.dfrobot.com/product-2064.html
* 
* Contributors: Sam Groveman
*/

#pragma once
#include <Sensor.h>
#include <Wire.h>
#include <DFRobot_ENS160.h>
#include <DFRobot_BME280.h>

/// @brief Device for interfacing with the DF Robot ENS160 + BME280 environmental sensor
class DFVOC : public Sensor {
	public:
		DFVOC(String Name, TwoWire* I2C_bus = &Wire, uint8_t ens160_address = 0x53, uint8_t bme280_address = 0x76, String ConfigFile = "DFVOC.json");
		DFVOC(String Name, int sda, int scl, TwoWire* I2C_bus = &Wire, uint8_t ens160_address = 0x53, uint8_t bme280_address = 0x76, String ConfigFile = "DFVOC.json");
		bool begin();
		bool takeMeasurement();
		String getConfig();
		bool setConfig(String config, bool save);
		
	protected:
		/// @brief DFVOC sensor configuration
		struct {
			/// @brief The value air pressure at sea level
			float pressureSeaLevel = 1013.25;
		} current_config;

		/// @brief I2C bus in use
		TwoWire* i2c_bus;

		/// @brief SCL pin in use
		int scl_pin = -1;

		/// @brief SDA pin in use
		int sda_pin = -1;

		/// @brief ENS160 sensor object
		DFRobot_ENS160_I2C ens160_sensor;

		/// @brief BME280 sensor object
		DFRobot_BME280_IIC bme280_sensor;

		/// @brief Full path to config file
		String config_path;
};