#include <zephyr/device.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/logging/log.h>
#include <zephyr/lora/lora.h>
#include <zephyr/lorawan/lorawan.h>
#include <zephyr/zephyr.h>

#define DEFAULT_RADIO_NODE DT_ALIAS(lora0)
BUILD_ASSERT(DT_NODE_HAS_STATUS(DEFAULT_RADIO_NODE, okay),
             "No default LoRa radio specified in DT");

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
LOG_MODULE_REGISTER(lorawan_node);

#define MAX_DATA_LEN 10

int main() {

  int ret;
  const struct device *const lora_dev = DEVICE_DT_GET(DEFAULT_RADIO_NODE);
  struct lora_modem_config config;

  struct lorawan_join_config otaa_config = {
      .dev_eui = {0xa0, 0xeb, 0xa3, 0xec, 0xc2, 0x96, 0x69, 0x85},
      .join_eui = {0x69, 0xd5, 0x39, 0x97, 0x1e, 0x9b, 0x2d, 0x84},
      .app_key = {0x42, 0x11, 0x6e, 0x2a, 0x03, 0xd0, 0xc3, 0x7e, 0x31, 0x40,
                  0xf3, 0x10, 0xde, 0xb9, 0xe7, 0xc5}};

  if (!device_is_ready(lora_dev)) {
    LOG_ERR("%s Device not ready", lora_dev->name);
    return -1;
  }

  if (lorawan_join(LORAWAN_ACT_OTAA, &otaa_config) == 0) {
    LOG_INF("Joined successfully");
  } else {
    LOG_ERR("Failed to join");
    return -1;
  }

  config.frequency = 865100000;
  config.bandwidth = BW_125_KHZ;
  config.datarate = SF_10;
  config.preamble_len = 8;
  config.coding_rate = CR_4_5;
  config.iq_inverted = false;
  config.public_network = false;
  config.tx_power = 4;
  config.tx = true;

  ret = lora_config(lora_dev, &config);

  if (ret < 0) {
    LOG_ERR("LoRa config failed");
    return -1;
  }

  char data[MAX_DATA_LEN] = {'f', 'u', 'c', 'k', 'o', 'f', 'f'};

  while (1) {
    ret = lora_send(lora_dev, data, MAX_DATA_LEN);
    LOG_INF("Data sent!");
  }

  return 0;
}
