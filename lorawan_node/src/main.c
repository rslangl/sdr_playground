#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/lorawan/lorawan.h>

#define LORAWAN_DEV_EUI                                                        \
  { 0xa0, 0xeb, 0xa3, 0xec, 0xc2, 0x96, 0x69, 0x85 }

#define LORAWAN_JOIN_EUI                                                       \
  { 0x69, 0xd5, 0x39, 0x97, 0x1e, 0x9b, 0x2d, 0x84 }

#define LORAWAN_APP_KEY                                                        \
  {                                                                            \
    0x42, 0x11, 0x6e, 0x2a, 0x03, 0xd0, 0xc3, 0x7e, 0x31, 0x40, 0xf3, 0x10,    \
        0xde, 0xb9, 0xe7, 0xc5                                                 \
  }

#define DELAY K_MSEC(10000)

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lorawan_node);

static void dl_callback(uint8_t port, bool data_pending, int16_t rssi,
                        int8_t snr, uint8_t len, const uint8_t *hex_data) {
  LOG_INF("Port %d, Pending %d, RSSI %ddB, SNR %ddBm", port, data_pending, rssi,
          snr);
  if (hex_data) {
    LOG_HEXDUMP_INF(hex_data, len, "Payload: ");
  }
}

static void lorawan_datarate_changed(enum lorawan_datarate dr) {
  uint8_t unused, max_size;
  lorawan_get_payload_sizes(&unused, &max_size);
  LOG_INF("New datarate: DR_%d, max payload %d", dr, max_size);
}

int main() {

  const struct device *lora_dev;
  struct lorawan_join_config join_cfg;
  uint8_t dev_eui[] = LORAWAN_DEV_EUI;
  uint8_t join_eui[] = LORAWAN_JOIN_EUI;
  uint8_t app_key[] = LORAWAN_APP_KEY;
  int ret;

  struct lorawan_downlink_cb downlink_cb = {.port = LW_RECV_PORT_ANY,
                                            .cb = dl_callback};

  lora_dev = DEVICE_DT_GET(DT_ALIAS(lora0));
  if (!device_is_ready(lora_dev)) {
    LOG_ERR("%s: device not ready", lora_dev->name);
    return 0;
  }

  ret = lorawan_set_region(LORAWAN_REGION_EU868);
  if (ret < 0) {
    LOG_ERR("lorawan_set_region failed: %d", ret);
    return 0;
  }

  ret = lorawan_start();
  if (ret < 0) {
    LOG_ERR("lorawan_start failed: %d", ret);
    return 0;
  }

  lorawan_register_downlink_callback(&downlink_cb);
  lorawan_register_dr_changed_callback(lorawan_datarate_changed);

  join_cfg.mode = LORAWAN_ACT_OTAA;
  join_cfg.dev_eui = dev_eui;
  join_cfg.otaa.join_eui = join_eui;
  join_cfg.otaa.app_key = app_key;
  join_cfg.otaa.nwk_key = app_key;
  join_cfg.otaa.dev_nonce = 0u;

  LOG_INF("Sending data...");

  char data[] = {'f', 'u', 'c', 'k', 'o', 'f', 'f'};

  while (1) {
    ret = lorawan_send(2, data, sizeof(data), LORAWAN_MSG_CONFIRMED);

    if (ret == -EAGAIN) {
      LOG_ERR("lorawan_send failed: %d. Continuing...", ret);
      k_sleep(DELAY);
      continue;
    }

    if (ret < 0) {
      LOG_ERR("lorawan_send failed: %d", ret);
      return 0;
    }

    LOG_INF("Data sent!");
    k_sleep(DELAY);
  }

  return 0;
}
