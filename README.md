# DragonEC

A Linux kernel module providing userspace access to the embedded controller (EC) on MSI Dragon series laptops.

## Features

- **Fan control** -- Configure 6-point fan speed curves for CPU and GPU fans
- **Fan monitoring** -- Read current fan speeds (percentage and RPM) via sysfs and hwmon
- **Temperature monitoring** -- CPU and GPU temperature readout through the hwmon subsystem
- **Performance modes** -- Switch between low, medium, high, turbo, and auto modes
- **Cooler Boost** -- Toggle maximum fan speed on/off
- **Battery charge threshold** -- Set charge limit between 50% and 100% (in 10% steps)
- **Keyboard backlight** -- Read current backlight level (off / low / medium / high)

## Supported Hardware

| Device | Firmware |
|---|---|
| MSI Dragon series | `E17F4IMS.108` |

The module refuses to load if the BIOS version does not match a supported firmware, preventing incorrect EC writes on unsupported hardware.

## Building

### Prerequisites

- Linux kernel headers for your running kernel

### Compile

```bash
make
```

### Clean

```bash
make clean
```

## Installation

### Manual

```bash
sudo insmod msi.ko
```

### DKMS (persistent across kernel updates)

```bash
sudo cp msi.c msi.h /usr/src/msi-1.00/
sudo cp dkms.conf /usr/src/msi-1.00/
sudo dkms add msi/1.00
sudo dkms build msi/1.00
sudo dkms install msi/1.00
```

### Load / Unload

```bash
sudo modprobe msi        # load
sudo modprobe -r msi     # unload
```

## Usage

### Sysfs Interface

The module creates a platform device `dragon_ec` with the following attributes:

| Attribute | R/W | Description |
|---|---|---|
| `cpu_fan_speed_config` | RW | 6-value CPU fan curve (space-separated, 0-100 each) |
| `gpu_fan_speed_config` | RW | 6-value GPU fan curve (space-separated, 0-100 each) |
| `cpu_fan_speed` | RO | Current CPU fan speed (%) |
| `gpu_fan_speed` | RO | Current GPU fan speed (%) |
| `performance_mode` | RW | `low`, `medium`, `high`, `turbo`, or `auto` |
| `cooler_boost` | RW | `on` or `off` |
| `backlight_led` | RO | `off`, `low`, `medium`, or `high` |
| `charge_control_end_threshold` | RW | Battery charge limit (50-100, multiples of 10) |

Example -- set CPU fan curve:

```bash
echo "30 40 50 60 80 100" | sudo tee /sys/devices/platform/dragon_ec/cpu_fan_speed_config
```

Example -- set performance mode to turbo:

```bash
echo turbo | sudo tee /sys/devices/platform/dragon_ec/performance_mode
```

### hwmon Interface

The module registers with the hwmon subsystem as `dragon-ec`, compatible with standard tools like `lm-sensors`:

| Channel | Type | Description |
|---|---|---|
| `temp1_input` / `temp2_input` | temp | CPU / GPU temperature (millidegrees C) |
| `temp1_label` / `temp2_label` | temp | `CPU` / `GPU` |
| `fan1_input` / `fan2_input` | fan | CPU / GPU fan RPM |
| `pwm1_enable` / `pwm2_enable` | pwm | Fan mode: 0=off, 1=advanced, 2=auto |

## EC Register Map

| Address | Name |
|---|---|
| `0x72` | CPU fan speed config (6 bytes) |
| `0x8A` | GPU fan speed config (6 bytes) |
| `0x71` | CPU fan speed |
| `0x89` | GPU fan speed |
| `0xCC` | CPU fan RPM (2 bytes, big-endian) |
| `0xCA` | GPU fan RPM (2 bytes, big-endian) |
| `0x68` | CPU temperature |
| `0x80` | GPU temperature |
| `0xF2` | Performance mode |
| `0xF4` | Fan control mode |
| `0xEF` | Battery charge threshold |
| `0x98` | Cooler Boost |
| `0xF3` | Backlight LED level |

## License

GPL

## Author

Ilia Alizadeh -- iliaalizadee@gmail.com
