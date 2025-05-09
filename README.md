# RPi Platform

A Clean Architecture-based, nearly pure C project for real-time robotics applications on Raspberry Pi.

## System Requirements

- Raspberry Pi 3 B+ with Ubuntu Server 22.04 LTS installed.
  - Other versions of Raspberry Pi or Ubuntu may work but have not been tested.
- SSH access configured with a key file.
- CMake installed on the Raspberry Pi.

## How to Upload the Code to the Raspberry Pi

Simply run the `upload.sh` script. This script performs the following actions:

1. Copies the contents of this directory to the Raspberry Pi.
2. Executes the `run.sh` script on the Raspberry Pi.

The `run.sh` script builds the project and sets up a systemd service to run the application. Specifically, it will:

1. Create a build directory and compile the project using CMake.
2. Install the application as a systemd service.
3. Enable and start the service so that the application runs automatically.

## Required Configurations

Edit the `/boot/firmware/cmdline.txt` file to include the following settings:

```
console=serial0,115200 multipath=off dwc_otg.lpm_enable=0 console=tty1 root=LABEL=writable rootfstype=ext4 rootwait fixrtc isolcpus=2,3 quiet loglevel=5 logo.nologo
```

The most critical setting is `isolcpus=2,3`, which isolates CPU cores for the application—this is essential for real-time performance.

## Recommended Configurations

### SSH Configuration

The following is a recommended `/etc/ssh/sshd_config` configuration for faster SSH access:

```
Include /etc/ssh/sshd_config.d/*.conf
PubkeyAuthentication yes
PasswordAuthentication no
KbdInteractiveAuthentication no
GSSAPIAuthentication no
UsePAM no
X11Forwarding no
PrintMotd no
UseDNS no
AcceptEnv LANG LC_*
```

**Warning: This configuration disables password authentication.**

### System Configuration

The following system configurations are recommended to reduce boot time.

#### Services

Disable the following services:

```bash
sudo systemctl disable snapd                                # Disable snapd
sudo systemctl mask snapd
sudo systemctl disable systemd-networkd-wait-online.service # Skip network wait
sudo systemctl mask systemd-networkd-wait-online.service
sudo touch /etc/cloud/cloud-init.disabled                   # Disable cloud-init
sudo systemctl disable snap.lxd.activate.service            # Disable LXD
sudo systemctl disable hciuart.service                      # Disable Bluetooth
```

#### Kernel Configuration

Edit the /boot/firmware/config.txt file with the following content:

```
[all]
kernel=vmlinuz
cmdline=cmdline.txt
initramfs initrd.img followkernel
disable_splash=1  # Disable the splash screen
boot_delay=0      # Eliminate boot delay

[pi4]
max_framebuffers=2
arm_boost=1

[all]
dtparam=audio=on
dtparam=i2c_arm=on
dtparam=spi=on
disable_overscan=1
hdmi_drive=2
enable_uart=1
camera_auto_detect=1
display_auto_detect=1
arm_64bit=1
dtoverlay=dwc2

[cm4]
dtoverlay=dwc2,dr_mode=host

[all]
```

#### Boot Time Analysis

Before optimization, the boot time was approximately 2 minutes and 7 seconds:

```
$ systemd-analyze
Startup finished in 10.991s (kernel) + 1min 56.018s (userspace) = 2min 7.009s
graphical.target reached after 1min 52.216s in userspace
```

After optimization, the boot time was reduced to approximately 22 seconds:

```
Startup finished in 9.801s (kernel) + 12.304s (userspace) = 22.105s
graphical.target reached after 12.237s in userspace
```

## References

- [BCM2835 ARM Peripherals](https://www.raspberrypi.org/documentation/hardware/raspberrypi/bcm2835/README.md)
- [AXI Monitor](https://forums.raspberrypi.com/viewtopic.php?p=1664415)
- [SPI Register Control](https://forums.raspberrypi.com/viewtopic.php?t=365275)
