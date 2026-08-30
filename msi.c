#include <acpi/battery.h>
#include <linux/acpi.h>
#include <linux/dmi.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/hwmon.h>
#include <linux/platform_device.h>
#include "msi.h"


static const char *supported_firmwares[SUPPORTED_FIRMWARES_NUM] = {
    "E17F4IMS.108"
};


static bool firmware_supported(void)
{
    const char *bios = dmi_get_system_info(DMI_BIOS_VERSION);
    int i;

    if (!bios)
        return false;

    for (i = 0; i < SUPPORTED_FIRMWARES_NUM; i++)
        if (!strcmp(bios, supported_firmwares[i]))
            return true;

    return false;
}


static ssize_t ec_write_check(u8 addr, u8 value, size_t count)
{
    int ret = ec_write(addr, value);

    return ret ? ret : count;
}


static ssize_t cpu_fan_speed_config_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    u8 output[FAN_SPEED_CONFIG_NUM];
    int i;

    for (i = 0; i < FAN_SPEED_CONFIG_NUM; i++) {
        int ret = ec_read(CPU_FAN_SPEED_CONFIG_ADDR + i, &output[i]);
        if (ret)
            return ret;
    }

    return sysfs_emit(buf, "%d %d %d %d %d %d\n",
                      output[0], output[1], output[2],
                      output[3], output[4], output[5]);
}


static ssize_t cpu_fan_speed_config_store(struct device *dev, struct device_attribute *attr,
                                          const char *buf, size_t count)
{
    int input[FAN_SPEED_CONFIG_NUM];
    int i;

    if (sscanf(buf, "%d %d %d %d %d %d",
               &input[0], &input[1], &input[2],
               &input[3], &input[4], &input[5]) != FAN_SPEED_CONFIG_NUM)
        return -EINVAL;

    for (i = 0; i < FAN_SPEED_CONFIG_NUM; i++)
        if (input[i] < 0 || input[i] > 100)
            return -EINVAL;

    for (i = 0; i < FAN_SPEED_CONFIG_NUM; i++) {
        int ret = ec_write(CPU_FAN_SPEED_CONFIG_ADDR + i, input[i]);
        if (ret)
            return ret;
    }

    return count;
}


static ssize_t gpu_fan_speed_config_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    u8 output[FAN_SPEED_CONFIG_NUM];
    int i;

    for (i = 0; i < FAN_SPEED_CONFIG_NUM; i++) {
        int ret = ec_read(GPU_FAN_SPEED_CONFIG_ADDR + i, &output[i]);
        if (ret)
            return ret;
    }

    return sysfs_emit(buf, "%d %d %d %d %d %d\n",
                      output[0], output[1], output[2],
                      output[3], output[4], output[5]);
}


static ssize_t gpu_fan_speed_config_store(struct device *dev, struct device_attribute *attr,
                                          const char *buf, size_t count)
{
    int input[FAN_SPEED_CONFIG_NUM];
    int i;

    if (sscanf(buf, "%d %d %d %d %d %d",
               &input[0], &input[1], &input[2],
               &input[3], &input[4], &input[5]) != FAN_SPEED_CONFIG_NUM)
        return -EINVAL;

    for (i = 0; i < FAN_SPEED_CONFIG_NUM; i++)
        if (input[i] < 0 || input[i] > 100)
            return -EINVAL;

    for (i = 0; i < FAN_SPEED_CONFIG_NUM; i++) {
        int ret = ec_write(GPU_FAN_SPEED_CONFIG_ADDR + i, input[i]);
        if (ret)
            return ret;
    }

    return count;
}


static ssize_t cpu_fan_speed_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    u8 output;
    int ret = ec_read(CPU_FAN_SPEED_ADDR, &output);

    if (ret)
        return ret;

    return sysfs_emit(buf, "%d\n", (int)output);
}


static ssize_t gpu_fan_speed_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    u8 output;
    int ret = ec_read(GPU_FAN_SPEED_ADDR, &output);

    if (ret)
        return ret;

    return sysfs_emit(buf, "%d\n", (int)output);
}


static ssize_t performance_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    u8 output;
    int ret = ec_read(PERFORMANCE_MODE_ADDR, &output);

    if (ret)
        return ret;

    switch (output) {
    case LOW_PERFORMANCE:
        return sysfs_emit(buf, "low\n");
    case MEDIUM_PERFORMANCE:
        return sysfs_emit(buf, "medium\n");
    case HIGH_PERFORMANCE:
        return sysfs_emit(buf, "high\n");
    case TURBO_PERFORMANCE:
        return sysfs_emit(buf, "turbo\n");
    case AUTO_PERFORMANCE:
        return sysfs_emit(buf, "auto\n");
    default:
        return sysfs_emit(buf, "error\n");
    }
}


static ssize_t performance_mode_store(struct device *dev, struct device_attribute *attr,
                                      const char *buf, size_t count)
{
    u8 value;

    if (sysfs_streq(buf, "low"))
        value = LOW_PERFORMANCE;
    else if (sysfs_streq(buf, "medium"))
        value = MEDIUM_PERFORMANCE;
    else if (sysfs_streq(buf, "high"))
        value = HIGH_PERFORMANCE;
    else if (sysfs_streq(buf, "turbo"))
        value = TURBO_PERFORMANCE;
    else if (sysfs_streq(buf, "auto"))
        value = AUTO_PERFORMANCE;
    else
        return -EINVAL;

    return ec_write_check(PERFORMANCE_MODE_ADDR, value, count);
}


static ssize_t charge_control_end_threshold_show(struct device *dev,
                                                 struct device_attribute *attr, char *buf)
{
    u8 output;
    int ret = ec_read(BATTERY_THRESHOLD_ADDR, &output);

    if (ret)
        return ret;

    return sysfs_emit(buf, "%d\n", (int)output - BATTERY_THRESHOLD_OFFSET);
}


static ssize_t charge_control_end_threshold_store(struct device *dev,
                                                  struct device_attribute *attr,
                                                  const char *buf, size_t count)
{
    int input;

    if (sscanf(buf, "%d", &input) != 1)
        return -EINVAL;

    if (input < 50 || input > 100 || (input % 10) != 0)
        return -EINVAL;

    return ec_write_check(BATTERY_THRESHOLD_ADDR, input + BATTERY_THRESHOLD_OFFSET, count);
}


static DEVICE_ATTR_RW(charge_control_end_threshold);


static struct attribute *msi_battery_attrs[] = {
    &dev_attr_charge_control_end_threshold.attr,
    NULL
};


ATTRIBUTE_GROUPS(msi_battery);


static int msi_battery_add(struct power_supply *battery, struct acpi_battery_hook *hook)
{
    return device_add_groups(&battery->dev, msi_battery_groups);
}


static int msi_battery_remove(struct power_supply *battery, struct acpi_battery_hook *hook)
{
    device_remove_groups(&battery->dev, msi_battery_groups);
    return 0;
}


static struct acpi_battery_hook msi_battery_hook = {
    .add_battery = msi_battery_add,
    .remove_battery = msi_battery_remove,
    .name = "dragon_ec",
};


static ssize_t cooler_boost_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    u8 output;
    int ret = ec_read(COOLER_BOOST_ADDR, &output);

    if (ret)
        return ret;

    switch (output) {
    case CB_OFF:
        return sysfs_emit(buf, "off\n");
    case CB_ON:
        return sysfs_emit(buf, "on\n");
    default:
        return sysfs_emit(buf, "error\n");
    }
}


static ssize_t cooler_boost_store(struct device *dev, struct device_attribute *attr,
                                  const char *buf, size_t count)
{
    u8 value;

    if (sysfs_streq(buf, "off"))
        value = CB_OFF;
    else if (sysfs_streq(buf, "on"))
        value = CB_ON;
    else
        return -EINVAL;

    return ec_write_check(COOLER_BOOST_ADDR, value, count);
}


static ssize_t backlight_led_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    u8 output;
    int ret = ec_read(BACKLIGHT_LED_LEVEL_ADDR, &output);

    if (ret)
        return ret;

    switch (output) {
    case BACKLIGHT_OFF:
        return sysfs_emit(buf, "off\n");
    case BACKLIGHT_LOW:
        return sysfs_emit(buf, "low\n");
    case BACKLIGHT_MEDIUM:
        return sysfs_emit(buf, "medium\n");
    case BACKLIGHT_HIGH:
        return sysfs_emit(buf, "high\n");
    default:
        return sysfs_emit(buf, "error\n");
    }
}


static DEVICE_ATTR_RW(cpu_fan_speed_config);
static DEVICE_ATTR_RW(gpu_fan_speed_config);
static DEVICE_ATTR_RO(cpu_fan_speed);
static DEVICE_ATTR_RO(gpu_fan_speed);
static DEVICE_ATTR_RW(performance_mode);
static DEVICE_ATTR_RW(cooler_boost);
static DEVICE_ATTR_RO(backlight_led);


static struct attribute *msi_ec_attrs[] = {
    &dev_attr_cpu_fan_speed_config.attr,
    &dev_attr_gpu_fan_speed_config.attr,
    &dev_attr_cpu_fan_speed.attr,
    &dev_attr_gpu_fan_speed.attr,
    &dev_attr_performance_mode.attr,
    &dev_attr_cooler_boost.attr,
    &dev_attr_backlight_led.attr,
    NULL
};


static const struct attribute_group msi_ec_group = {
    .attrs = msi_ec_attrs,
};


static const struct attribute_group *msi_ec_groups[] = {
    &msi_ec_group,
    NULL
};


static int msi_ec_hwmon_read(struct device *dev, enum hwmon_sensor_types type,
                             u32 attr, int channel, long *val)
{
    u8 output;
    int ret;

    switch (type) {
    case hwmon_temp:
        if (channel != 0 && channel != 1)
            return -EINVAL;
        ret = ec_read(channel == 0 ? CPU_TEMP_ADDR : GPU_TEMP_ADDR, &output);
        if (ret)
            return ret;
        *val = output * 1000;
        return 0;

    case hwmon_fan:
        if (channel != 0 && channel != 1)
            return -EINVAL;
        if (attr == hwmon_fan_input) {
            u8 block[FAN_RPM_BLOCK_NUM];
            u8 base = channel == 0 ? CPU_FAN_RPM_ADDR : GPU_FAN_RPM_ADDR;
            int value;

            ret = ec_read(base, &block[0]);
            if (ret)
                return ret;
            ret = ec_read(base + 1, &block[1]);
            if (ret)
                return ret;

            value = block[0] * 256 + block[1];
            *val = value ? FAN_RPM_BASE / value : 0;
            return 0;
        }
        return -EOPNOTSUPP;

    case hwmon_pwm:
        if (attr == hwmon_pwm_enable) {
            ret = ec_read(FAN_MODE_ADDR, &output);
            if (ret)
                return ret;
            if (output == AUTO_FAN)
                *val = 2;
            else if (output == ADVANCED_FAN)
                *val = 1;
            else
                *val = 0;
            return 0;
        }
        return -EOPNOTSUPP;

    default:
        return -EOPNOTSUPP;
    }
}


static int msi_ec_hwmon_read_string(struct device *dev, enum hwmon_sensor_types type,
                                    u32 attr, int channel, const char **str)
{
    if (type == hwmon_temp && attr == hwmon_temp_label) {
        *str = channel == 0 ? "CPU" : "GPU";
        return 0;
    }

    return -EOPNOTSUPP;
}


static int msi_ec_hwmon_write(struct device *dev, enum hwmon_sensor_types type,
                              u32 attr, int channel, long val)
{
    u8 value;

    if (type == hwmon_pwm && attr == hwmon_pwm_enable) {
        if (val == 2)
            value = AUTO_FAN;
        else if (val == 1)
            value = ADVANCED_FAN;
        else
            return -EINVAL;

        return ec_write(FAN_MODE_ADDR, value);
    }

    return -EOPNOTSUPP;
}


static umode_t msi_ec_hwmon_is_visible(const void *drvdata,
                                       enum hwmon_sensor_types type,
                                       u32 attr, int channel)
{
    if (type == hwmon_pwm && attr == hwmon_pwm_enable)
        return 0644;

    return 0444;
}


static const struct hwmon_ops msi_ec_hwmon_ops = {
    .is_visible = msi_ec_hwmon_is_visible,
    .read = msi_ec_hwmon_read,
    .read_string = msi_ec_hwmon_read_string,
    .write = msi_ec_hwmon_write,
};


static const struct hwmon_channel_info * const msi_ec_hwmon_info[] = {
    HWMON_CHANNEL_INFO(temp,
                       HWMON_T_INPUT | HWMON_T_LABEL,
                       HWMON_T_INPUT | HWMON_T_LABEL),
    HWMON_CHANNEL_INFO(fan,
                       HWMON_F_INPUT,
                       HWMON_F_INPUT),
    HWMON_CHANNEL_INFO(pwm,
                       HWMON_PWM_ENABLE,
                       HWMON_PWM_ENABLE),
    NULL
};


static const struct hwmon_chip_info msi_ec_chip_info = {
    .ops = &msi_ec_hwmon_ops,
    .info = msi_ec_hwmon_info,
};


static int msi_ec_probe(struct platform_device *pdev)
{
    struct device *hwmon;
    int ret;

    if (!firmware_supported()) {
        pr_err("ec: this device is not supported\n");
        return -ENODEV;
    }

    hwmon = devm_hwmon_device_register_with_info(&pdev->dev, "dragon-ec", NULL,
                                                 &msi_ec_chip_info, NULL);
    if (IS_ERR(hwmon)) {
        pr_err("ec: hwmon registration failed: %ld\n", PTR_ERR(hwmon));
        return PTR_ERR(hwmon);
    }

    ret = devm_battery_hook_register(&pdev->dev, &msi_battery_hook);
    if (ret)
        return ret;

    pr_info("ec: hwmon device registered\n");
    return 0;
}


static struct platform_driver msi_ec_driver = {
    .driver = {
        .name = "dragon_ec",
        .dev_groups = msi_ec_groups,
    },
    .probe = msi_ec_probe,
};


static struct platform_device *msi_ec_pdev;


static int __init msi_ec_init(void)
{
    int ret;

    if (!firmware_supported()) {
        pr_err("ec: module not loaded, BIOS %s not supported\n",
               dmi_get_system_info(DMI_BIOS_VERSION) ?: "unknown");
        return -ENODEV;
    }

    ret = platform_driver_register(&msi_ec_driver);
    if (ret)
        return ret;

    msi_ec_pdev = platform_device_register_simple("dragon_ec", PLATFORM_DEVID_NONE, NULL, 0);
    if (IS_ERR(msi_ec_pdev)) {
        platform_driver_unregister(&msi_ec_driver);
        return PTR_ERR(msi_ec_pdev);
    }

    pr_info("ec: module registered\n");
    return 0;
}


static void __exit msi_ec_exit(void)
{
    platform_device_unregister(msi_ec_pdev);
    platform_driver_unregister(&msi_ec_driver);

    pr_info("ec: module unregistered\n");
}


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ilia Alizadeh <iliaalizadee@gmail.com>");
MODULE_DESCRIPTION("MSI embedded controller module");
MODULE_VERSION("1.00");

module_init(msi_ec_init);
module_exit(msi_ec_exit);
