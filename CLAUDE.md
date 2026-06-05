# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

**HyphenOS** — a "never fail" Arduino/ESP32 firmware runtime for field-deployed environmental IoT sensing, part of Similie's Hyphen Ecosystem. It dynamically configures a roster of sensor drivers at runtime, reads them on a timer, and publishes JSON telemetry over TLS MQTT to Hyphen Command Center. Designed to self-recover, reconnect, and keep sensing in remote/low-power deployments.

Target hardware: ESP32 (`esp32dev`, 16MB flash, PSRAM required), with a SIM7600 cellular modem and/or WiFi.

## Build / flash / monitor

PlatformIO is the only build system (no separate lint or unit-test harness — `test/` contains only a stub README, so "testing" means build + flash + watch the serial console).

```bash
pio run                 # compile
pio run -t upload       # compile + flash over USB
pio device monitor      # serial monitor (115200 baud, esp32_exception_decoder filter)
pio run -t uploadfs     # flash the SPIFFS data/ partition
pio run -t erase        # full chip erase (wipes EEPROM/NVS device config)
pio run -t clean        # clean build artifacts
```

`esp32_exception_decoder` is enabled in the monitor filters, so crash backtraces are symbolized automatically when watching serial.

> **Build gotcha — keep `.pio` out of cloud sync.** This project lives in a cloud-synced folder (iCloud/Dropbox). The sync intermittently corrupts the PlatformIO package cache under `.pio/libdeps/` — dropping files (e.g. a missing `TinyGsmClient.h` or `ArduinoJson/src/ArduinoJson.h`) and creating `" 2"`/`" 3"` duplicate dirs — which surfaces as bogus `fatal error: ... No such file or directory` build failures. Build with the workspace redirected outside the synced tree to avoid it:
> ```bash
> PLATFORMIO_WORKSPACE_DIR=/tmp/hyphenos-pio pio run -e esp32dev
> ```
> If a build suddenly can't find a dependency header that's clearly installed, this is why — re-run with the redirected workspace (or `rm -rf .pio/libdeps && pio run`).

## Testing

There is a **host (native) unit-test harness** — fast logic tests that run on your machine and in CI with no ESP32, no network, and no certs. It mirrors the HyphenConnect harness (Unity + Arduino/FreeRTOS shims + a controllable fake clock).

```bash
cd tests-native && pio test -e native      # run all host unit tests
```

- **Why `tests-native/`:** the firmware `platformio.ini` is gitignored/generated, and `pio test -c <file>` doesn't propagate a custom config across multiple suites. So the native env lives in its own committed project, `tests-native/platformio.ini` (force-un-ignored in `.gitignore`), with `test_dir` pointing back at the shared `test/native/` tree.
- **Layout:** suites in `test/native/test_*/test_main.cpp`; shims in `test/native/shims/` (Arduino `String`, `millis()` via `test_clock.h`, no-op Log/Ticker/FreeRTOS). CI runs them via `.github/workflows/native-tests.yml`.
- **What's testable on host:** dependency-free pure logic only. Add such logic as small headers under `src/resources/utils/` and call them from production code. Current units: `timing.h` (rollover-safe `timedOut`/`elapsed`, used by `waitForTrue` et al.) and `aggregation.h` (`median`/`maximum` for sensor/ADC sample reduction). Hardware/network paths (SDI-12, MQTT, modem, persistence) stay on-device.
- **On-device / integration** verification (boot with no network, stuck-sensor timeouts, reconnect behavior) is done by flashing a board and watching serial — see `/run` and `/verify`.

## Configuration is gitignored — start from the example

`platformio.ini` and `src/certs/` are **gitignored and not in the repo**. Hyphen Command Center normally generates them; to build manually, copy `platformio.example.ini` → `platformio.ini` and fill it in.

**Nearly all device behavior is set through `-D` build flags in `platformio.ini`, not in code.** This includes the MQTT endpoint, device ID, WiFi/APN credentials, cellular pins, topic base, keep-alive intervals, camera/WSS endpoints, and feature toggles. Key ones:

- `CONNECTION_TYPE` — `0`=WiFi-preferred, `1`=Cellular-preferred, `2`=WiFi-only, `3`=Cellular-only
- `HYPHEN_THREADED` — run the publisher on its own FreeRTOS task (see Threading below)
- `DEVICE_PUBLIC_ID` — the device's identity on the MQTT network
- `BOARD_HAS_PSRAM` / `CONFIG_SPIRAM_USE_CAPS_ALLOC` — required; mbedtls is routed to PSRAM
- `COMPRESSED_PUBLISH` — switches the publish topic and enables payload compression

TLS certs must be named **exactly** `src/certs/root-ca.pem`, `device-cert.pem`, `private-key.pem` (optionally `isrgrootx1.pem`); they're embedded into the binary via `board_build.embed_txtfiles`.

The custom 16MB partition table is `part_16mb_app.csv` (dual OTA app slots + SPIFFS).

## Architecture

`main.cpp` is deliberately tiny: it installs a PSRAM-backed mbedtls allocator, starts the `Watchdog`, restores time, then hands everything to two globals — `LocalProcessor processor` and `DeviceManager manager`. `setup()` calls `manager.init()`; `loop()` only calls `Watchdog.loop()` + `manager.loop()`. All real work lives below.

### The runtime pipeline

```
DeviceManager  ── owns the device roster, drives read/publish/OTA/low-power
   ├── Configurator   maps device-name strings → concrete Device subclasses (add/remove at runtime)
   ├── Bootstrap      EEPROM-persisted device registration + read/publish/heartbeat timers
   ├── Device[]       up to MAX_DEVICES (7) live sensor instances
   └── LocalProcessor networking/publish topics → HyphenConnect
```

- **`DeviceManager`** (`src/resources/devices/device-manager.cpp`) is the heart of the system. It bootstraps persisted devices, reads them on `Bootstrap`'s read timer, aggregates readings, writes JSON payloads, and publishes on the publish timer. It also handles offline buffering (store-and-forward when disconnected), OTA, low-power/radio toggling, reboot requests, and registering cloud functions for runtime device add/remove. `read()` and `publish()` are mutually exclusive via `readBusy`/`publishBusy` guarded by `waitForTrue(...)`.

- **`Device`** (`src/resources/devices/device.h`) is the abstract sensor interface every driver implements: `init()`, `read()`, `loop()`, `publish(JsonObject&, ...)`, `clear()`, `name()`, `paramCount()`, `maintenanceCount()`, `restoreDefaults()`. Concrete drivers live in `src/resources/devices/` (soil-moisture, all-weather, rain-gauge, gps, battery, flow-meter, turbidity, accelerometer, relay, ip-camera/video-capture, wl-device, etc.).

- **`Configurator`** (`src/resources/utils/configurator.h`) holds the registry of supported device types — the `devicesAvailable[]` string array and matching enum. **To add a new sensor type you must register it here** (string name + enum + a case in `pullDeviceType`) in addition to writing the `Device` subclass. Devices are added/removed at runtime by name via cloud functions, persisted by `Bootstrap`.

- **`Bootstrap`** (`src/resources/bootstrap/bootstrap.h`) manages EEPROM persistence (device list, timezone, publish interval, low-power mode, maintenance flag) and owns the read/publish/heartbeat timers that `DeviceManager` polls each loop. EEPROM addresses are computed as a chain of `sizeof`-based offsets — be careful changing the persisted structs, as it shifts the whole layout.

- **`LocalProcessor`** (`src/resources/processors/LocalProcessor.h`) implements the abstract `Processor` interface, owning connection lifecycle and the MQTT publish topics (`Hy/Post/Black` normal, `Hy/Post/Gold` compressed, plus maintenance/heartbeat topics). It delegates the actual transport to HyphenConnect.

### Hyphen cloud API (Particle-style)

`Hyphen` (`include/Hyphen.h`, global `HyphenClass`) wraps the external **HyphenConnect** library and exposes a Particle-cloud-like API used throughout the drivers:
- `Hyphen.variable(name, &var)` — expose a variable for remote read
- `Hyphen.function(name, &Class::method, instance)` — register a remotely callable function (returns `int`, takes a `String`/`std::string` arg). Drivers call this in their `init()`/`setFunctions()` to add per-device cloud commands (e.g. calibration, address set).
- `Hyphen.publish(...)`, `connect()`, `getLocation()`, `syncTime()`.

### System singletons

`include/system/modules.h` declares global hardware/service singletons (defined in `src/system/modules.cpp`), available everywhere: `Watchdog`, `FuelGauge`, `Time`, `Persist`, `Storage` (SD card), `Blue` (Bluetooth config). Other system code under `src/system/`: `ota/` (cloud + local OTA, dual-slot), `security/` (payload IDs, device security), `tcp/`, `time/`, `persistence/` (+ `sd-card`), `wws/` + `resources/utils/wss-rtsp-tunnel.cpp` (WebSocket tunnel for streaming IP-camera/RTSP frames out through the firewall).

### SDI-12 sensors (important shared-bus pattern)

Many sensors (soil moisture, water level, all-weather, etc.) talk over **one shared SDI-12 bus on GPIO 15**. The bus is a strict singleton: `SDI12DeviceManager::getInstance()` (`src/resources/utils/sdi-12.h`) — never construct a second `SDI12` object. Such drivers subclass **`SDI12Device`** and pair with an **`SDIParamElements`** subclass that defines the value-name map and the `extractValue()` post-processing (e.g. median, calibration equations). A `sendIdentity` int lets multiple sensors of the same type coexist by suffixing their parameter names.

## Concurrency, watchdog, and memory model

- **Watchdog** (`src/system/watchdog/`): the ESP32 core-0 task WDT is disabled (`disableCore0WDT()`); instead a custom `Watchdog` pulses an *external hardware* watchdog (GPIO 32). An **always-on dedicated FreeRTOS feeder task** (independent of the main loop) drives the pin, but only while a **liveness heartbeat** is fresh: the main loop calls `Watchdog.loop()` (non-blocking — it just records `millis()`) each iteration. `automatic()` starts the feeder in unconditional "grace" mode for boot; `start()` arms liveness gating. If the main loop wedges past `HYPHEN_WD_LIVENESS_TIMEOUT_MS` (default 30 s) the feeder stops pulsing and the hardware watchdog resets the board — the "never hang" mechanism. Legitimately long main-loop operations (e.g. an OTA download) call `Watchdog.heartbeat()` so honest progress keeps the board alive while a true wedge still resets it. The feed decision is the pure, host-tested `hyphen::watchdog::shouldFeed()` (`src/resources/utils/watchdog_policy.h`).
- **Threading**: with `HYPHEN_THREADED`, the publisher runs on a dedicated FreeRTOS task (`taskHandle`, ~4KB stack) so network publishes don't block sensor reads.
- **PSRAM**: PSRAM is required. mbedtls allocations are redirected to PSRAM in `main.cpp` (`mbedtls_platform_set_calloc_free`) before the TLS stacks initialize — needed because cert chains + MQTT buffers exceed internal RAM.

## Conventions

- Commits follow Conventional Commits (`feat:`, `chore:`, `fix:` …); current firmware version is in `include/version.h` (`BUILD_VERSION`).
- The base `main.cpp` `DEBUGGER` flag should be set to `false` for field devices.
- Adding a sensor = write `Device` subclass in `src/resources/devices/` + register it in `Configurator` (`CURRENT_DEVICES_COUNT`, `devicesAvailable[]`, enum, `pullDeviceType`).

## Related repos (external)

`HyphenConnect` (the networking/MQTT/cloud library this firmware is built on) and Hyphen Command Center are separate Similie repos — see README.md for the full list.
