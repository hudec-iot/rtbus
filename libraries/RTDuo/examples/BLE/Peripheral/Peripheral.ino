/* SPDX-License-Identifier: MIT */

#include <Arduino.h>
#include <RTDuo.h>

// Requires a board running RTBus with BLE peripheral support enabled.
// Scan for "RTBus Peripheral".
// Connect, read the characteristic (initially 00), and enable notifications.
// Characteristic UUID: 78573611-95a9-4e8d-ba92-4feb1648dfa0.
// Write one byte in hex (for example, 2A), then read it back or watch the
// notification. The serial console logs connection and write events.

#define SERVICE_UUID RUNTIME_BLE_UUID128_ENCODE( \
    0x78573610, 0x95a9, 0x4e8d, 0xba92, 0x4feb1648dfa0ULL)

static const char deviceName[] = "RTBus Peripheral";
static const runtime_ble_gatt_chrc_handle_t valueCharacteristic = 0;
static runtime_ble_gatt_handle_t serviceHandle;

static bool checkResult(const char *operation, int result) {
  if (result < 0) {
    Serial.printf("%s failed (%d)\r\n", operation, result);
    return false;
  }
  return true;
}

static void startAdvertising() {
  if (checkResult("Advertising request", runtime_ble_adv_start())) {
    Serial.println("Advertising requested");
  }
}

static void onRuntimeEvent(const struct runtime_event_tlv *event) {
  runtime_ble_conn_t connection;
  uint8_t reason;
  runtime_ble_gatt_handle_t handle;
  runtime_ble_gatt_chrc_handle_t characteristic;
  const uint8_t *data;
  uint8_t length;

  if (runtime_event_is_ble_connected(event, &connection)) {
    Serial.println("BLE connected");
  } else if (runtime_event_is_ble_disconnected(event, &reason)) {
    Serial.printf("BLE disconnected (0x%02X)\r\n", (unsigned int)reason);
  } else if (runtime_event_is_ble_connect_failed(event, &reason)) {
    Serial.printf("BLE connection failed (0x%02X)\r\n", (unsigned int)reason);
  } else if (runtime_event_is_ble_recycled(event)) {
    // The runtime can accept a new connection after recycling the old one.
    startAdvertising();
  } else if (runtime_event_is_ble_gatt_write(event, &handle, &characteristic,
                                             &data, &length) &&
             handle == serviceHandle && characteristic == valueCharacteristic) {
    if (length != 1) {
      Serial.println("Write ignored. Send exactly one byte.");
      return;
    }

    uint8_t value = data[0];
    if (checkResult("GATT value update",
                    runtime_ble_gatt_value_set(handle, characteristic,
                                               &value, sizeof(value)))) {
      Serial.printf("BLE value = 0x%02X\r\n", (unsigned int)value);
    }
  }
}

void setup() {
  Serial.begin(115200);

  // The runtime copies the GAP and GATT definitions during these calls.
  const runtime_ble_gap_data_def advertising[] = {
    {RUNTIME_BLE_GAP_AD_FLAGS, 1,
     {RUNTIME_BLE_GAP_FLAG_GENERAL | RUNTIME_BLE_GAP_FLAG_NO_BREDR}},
    {RUNTIME_BLE_GAP_AD_UUID128_ALL, 16, {SERVICE_UUID}},
  };
  runtime_ble_gap_data_def scanResponse = {
    RUNTIME_BLE_GAP_AD_NAME_COMPLETE, sizeof(deviceName) - 1, {},
  };
  for (uint8_t i = 0; i < scanResponse.data_len; ++i) {
    scanResponse.data[i] = deviceName[i];
  }

  const runtime_ble_gap_def gap = {
    RUNTIME_BLE_SECURITY_NONE,
    0,
    sizeof(advertising) / sizeof(advertising[0]),
    1,
    (uint32_t)(uintptr_t)advertising,
    (uint32_t)(uintptr_t)&scanResponse,
  };
  if (!checkResult("GAP initialization", runtime_ble_gap_init(&gap))) {
    return;
  }

  const uint8_t initialValue = 0;
  const runtime_ble_gatt_chrc_def characteristic = {
    {RUNTIME_BLE_UUID128_ENCODE(
        0x78573611, 0x95a9, 0x4e8d, 0xba92, 0x4feb1648dfa0ULL)},
    sizeof(initialValue),
    RUNTIME_BLE_GATT_CHRC_READ | RUNTIME_BLE_GATT_CHRC_WRITE |
        RUNTIME_BLE_GATT_CHRC_NOTIFY,
    RUNTIME_BLE_GATT_PERM_READ | RUNTIME_BLE_GATT_PERM_WRITE,
    RUNTIME_BLE_GATT_PERM_READ | RUNTIME_BLE_GATT_PERM_WRITE,
    (uint32_t)(uintptr_t)&initialValue,
  };
  const runtime_ble_gatt_service_def service = {
    {SERVICE_UUID},
    1,
    {},
    (uint32_t)(uintptr_t)&characteristic,
  };
  int handle = runtime_ble_gatt_service_register(&service);
  if (!checkResult("GATT service registration", handle)) {
    return;
  }
  serviceHandle = (runtime_ble_gatt_handle_t)handle;

  if (checkResult("BLE event registration", rtbus_on_event(onRuntimeEvent))) {
    startAdvertising();
  }
}

void loop() {
  // RTBus dispatches queued BLE events in the native application thread.
  delay(20);
}
