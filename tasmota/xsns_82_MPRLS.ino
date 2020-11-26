/*
  xsns_72_MPRLS - MPRLS I2C pressure sensor support for Tasmota

  Copyright (C) 2020  Martin Wagner and Theo Arends

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifdef USE_I2C
#ifdef USE_MPRLS
/*********************************************************************************************\
 * MPRLS - TDS Sensor
 *
 * I2C Address: 0x18
 *
\*********************************************************************************************/

#define XSNS_82              82
#define XI2C_82              82  // See I2CDEVICES.md

#include "Adafruit_MPRLS.h"
Adafruit_MPRLS MPRLS = Adafruit_MPRLS(); // create object copy

#define MPRLS_MAX_SENSORS    2
#define MPRLS_START_ADDRESS  0x18

struct {
char types[9] = "MPRLS";
uint8_t count  = 0;
} MPRLS_cfg;

struct {
  float   pressure = NAN;
  uint8_t address;
} MPRLS_sensors[MPRLS_MAX_SENSORS];

/********************************************************************************************/

float MPRLSRead(uint8_t addr) {
  float t = MPRLS.readPressure(addr);
  return t;
}
//todo
void MPRLSDetect(void) {
  for (uint8_t i = 0; i < MPRLS_MAX_SENSORS; i++) {
    if (!I2cSetDevice(MPRLS_START_ADDRESS + i)) { continue; }

    if (MPRLS.begin(MPRLS_START_ADDRESS + i)) {
      MPRLS_sensors[MPRLS_cfg.count].address = MPRLS_START_ADDRESS + i;
      I2cSetActiveFound(MPRLS_sensors[MPRLS_cfg.count].address, MPRLS_cfg.types);
      MPRLS.setResolution (MPRLS_sensors[MPRLS_cfg.count].address, 2); // Set Resolution to 0.125°C
      MPRLS_cfg.count++;
    }
  }
}

void MPRLSEverySecond(void) {
  for (uint32_t i = 0; i < MPRLS_cfg.count; i++) {
    float t = MPRLSRead(MPRLS_sensors[i].address);
    MPRLS_sensors[i].pressure = ConvertTemp(t);
  }
}

void MPRLSShow(bool json) {
  for (uint32_t i = 0; i < MPRLS_cfg.count; i++) {
    char pressure[33];
    dtostrfd(MPRLS_sensors[i].pressure, Settings.flag2.pressure_resolution, pressure);

    char sensor_name[11];
    strlcpy(sensor_name, MPRLS_cfg.types, sizeof(sensor_name));
    if (MPRLS_cfg.count > 1) {
      snprintf_P(sensor_name, sizeof(sensor_name), PSTR("%s%c%02X"), sensor_name, IndexSeparator(), MPRLS_sensors[i].address); // MPRLS-18, MPRLS-1A  etc.
    }

    if (json) {
      ResponseAppend_P(JSON_SNS_TEMP, sensor_name, pressure);
      if ((0 == tele_period) && (0 == i)) {
#ifdef USE_DOMOTICZ
        DomoticzSensor(DZ_TEMP, pressure);
#endif  // USE_DOMOTICZ
#ifdef USE_KNX
        KnxSensor(KNX_pressure, MPRLS_sensors[i].pressure);
#endif // USE_KNX
      }
#ifdef USE_WEBSERVER
    } else {
      WSContentSend_PD(HTTP_SNS_TEMP, sensor_name, pressure, TempUnit());
#endif  // USE_WEBSERVER
    }
  }
}

/*********************************************************************************************\
 * Interface
\*********************************************************************************************/

bool Xsns82(uint8_t function)
{
  if (!I2cEnabled(XI2C_82)) { return false; }
  bool result = false;

  if (FUNC_INIT == function) {
    MPRLSDetect();
  }
  else if (MPRLS_cfg.count){
    switch (function) {
      case FUNC_EVERY_SECOND:
        MPRLSEverySecond();
        break;
      case FUNC_JSON_APPEND:
        MPRLSShow(1);
        break;
  #ifdef USE_WEBSERVER
      case FUNC_WEB_SENSOR:
        MPRLSShow(0);
        break;
  #endif  // USE_WEBSERVER
    }
  }
  return result;
}

#endif  // USE_MPRLS
#endif  // USE_I2C
