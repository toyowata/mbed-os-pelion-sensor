# Pelion Device Management Client example for Mbed OS

This is an basic Device Management client example for Mbed OS with the following features:
- Support for latest Mbed OS and Device Management Client releases.
- Support for Developer mode provisioning.
- Support for FW Update.

There is a more advanced example of the client with support for multiple operating systems in [mbed-cloud-client-example](https://github.com/ARMmbed/mbed-cloud-client-example) repository. The underlying client library is the same for both. This Mbed OS only example is simpler as it only supports one OS with a limited set of demonstrated features. If you want to do development in Linux and Mbed OS at the same time - you should use the [mbed-cloud-client-example](https://github.com/ARMmbed/mbed-cloud-client-example).

<span class="notes">**Note:** If you want to use production provisioning modes, or use more advanced client features, those are demonstrated via [mbed-cloud-client-example](https://github.com/ARMmbed/mbed-cloud-client-example).</span>

## Supported boards

This table shows a list of boards that are supported.

Board                               |  Connectivity     | Storage for credentials and FW candidate | Notes
------------------------------------| ------------------| ------------------------| --------------
NXP `K64F`                          | Ethernet          | Internal Flash          |
Renesas `GR_LYCHEE`                 | Wi-Fi ESP32       | External Flash ([See security limitation of this board](https://os.mbed.com/platforms/Renesas-GR-LYCHEE/#security-limitation-of-this-platform)) |
Renesas `RZ_A1H`                    | Ethernet          | External Flash ([See security limitation of this board](https://os.mbed.com/platforms/Renesas-GR-PEACH/#security-limitation-of-this-platform)) |
Seeed `WIO_3G`                      | Cellular          | Internal Flash          |
Seeed `WIO_BG96`                    | Cellular          | Internal Flash          |
ST `NUCLEO_F429ZI`                  | Ethernet          | Internal Flash          |

# Developer guide

This section is intended for developers to get started, import the example application, compile and get it running on their device.

## Requirements

- Mbed CLI >= 1.10.0

  For instructions on installing and using Mbed CLI, please see our [documentation](https://os.mbed.com/docs/mbed-os/latest/tools/developing-mbed-cli.html).

- Install the `CLOUD_SDK_API_KEY`

   `mbed config -G CLOUD_SDK_API_KEY ak_1MDE1...<snip>`

   You should generate your own API key. Pelion Device Management is available for any Mbed developer. Create a [free trial](https://os.mbed.com/pelion-free-tier).

   For instructions on how to generate your API key, please see our [documentation](https://cloud.mbed.com/docs/current/integrate-web-app/api-keys.html#generating-an-api-key).

## Deploying

This repository is in the process of being updated and depends on few enhancements being deployed in mbed-cloud-client. In the meantime, follow these steps to import and apply the patches before compiling.

    ```
    mbed import mbed-os-example-pelion
    cd mbed-os-example-pelion
    ```

## Compiling

    mbed target K64F
    mbed toolchain GCC_ARM
    mbed device-management init -d arm.com --model-name example-app --force -q
    mbed compile

## Program Flow

1. Initialize, connect and register to Pelion DM
1. Interact with the user through the serial port (115200 bauds)
   - Press enter through putty/minicom to simulate button
   - Press `i` to print endpoint name
   - Press Ctrl-C to to unregister
   - Press `r` to reset storage and reboot (warning: it generates a new device ID!)

## Further information and requirements

Check the public tutorial for further information:

  [https://www.pelion.com/docs/device-management/current/connecting/mbed-os.html](https://www.pelion.com/docs/device-management/current/connecting/mbed-os.html)

## Enabling logs

Logging (or tracing) can be enabled by modifying the [`mbed_app.json`](https://github.com/ARMmbed/mbed-os-example-pelion/blob/master/mbed_app.json#L19) file.

    ```
            "mbed-trace.enable"                         : null,
    ```
By modifying that `null` to `1` and recompiling the application.

Log level can be modified compile-time by defining `MBED_TRACE_MAX_LEVEL` -macro to `mbed_app.json`:

   ```
    "target.macros_add": [
         "MBED_TRACE_MAX_LEVEL=TRACE_LEVEL_INFO",
   ```

Default level is `TRACE_LEVEL_DEBUG`, possible values are:
* `TRACE_LEVEL_DEBUG` (largest amounts of logs)
* `TRACE_LEVEL_INFO`
* `TRACE_LEVEL_WARN` and
* `TRACE_LEVEL_ERROR` (smallest amount of logs).

Component level run-time control is also possible by setting log levels (by calling `mbed_trace_config_set()`) and inclusions/exclusions (by calling `mbed_trace_include_filters_set()` or mbed_trace_exclude_filters_set()`).

For more details, see the [`mbed-trace`](https://github.com/ARMmbed/mbed-trace) library.

## Troubleshooting

- Device initializes but can't register to Pelion

  Error: `client_error(3) -> Bootstrap server URL is not correctly formed`

  Solution: Format the the storage by pressing 'r' in the serial terminal.

