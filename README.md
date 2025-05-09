# RPi Platform

Clean-Architecture based, almost pure C project for Raspberry Pi based real time robotics applications.

## System Requirements

- Raspberry Pi 3 B+ with Ubuntu Server 22.04 LTS installed.
  - Other versions of Raspberry Pi or Ubuntu may work, but are not tested.
- Keyfile based SSH access configured.
- CMake installed on Raspberry Pi.

## How to Upload the Code to the Raspberry Pi

What you need to do is just run the `upload.sh` script. This script will do the following things:

1. Copy the contents of this directory to the Raspberry Pi.
2. Run `run.sh` script on the Raspberry Pi.

The `run.sh` script will build the project and set up a systemd service to run the application. Specifically, it will:

1. Create a build directory and compile the project using CMake
2. Install the application as a systemd service
3. Enable and start the service to run the application automatically

## Required Configurations

Edit the `/boot/firmware/cmdline.txt` file to include the following settings:

```
console=serial0,115200 multipath=off dwc_otg.lpm_enable=0 console=tty1 root=LABEL=writable rootfstype=ext4 rootwait fixrtc isolcpus=2,3 quiet loglevel=5 logo.nologo
```

The most important part is the `isolcpus=2,3` setting. This isolates the CPU cores to the application, which is necessary for real-time performance.

## Recommended Configurations

### SSH Configuration

Below is recommended `/etc/ssh/sshd_config` configuration for faster SSH access

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

**Warning: This configuration will disable password authentication.**

### System Configuration

Below is recommended system configuration for faster boot time.

#### Services

Disable the following services to reduce boot time:

```bash
sudo systemctl disable snapd                                # Disable snapd
sudo systemctl mask snapd
sudo systemctl disable systemd-networkd-wait-online.service # Do not wait for network
sudo systemctl mask systemd-networkd-wait-online.service
sudo touch /etc/cloud/cloud-init.disabled                   # Disable cloud-init
sudo systemctl disable snap.lxd.activate.service            # Disable LXD
sudo systemctl disable hciuart.service                      # Disable Bluetooth
```

#### Kernel Configuration

Edit the `/boot/firmware/config.txt` file to include the following settings:

```
[all]
kernel=vmlinuz
cmdline=cmdline.txt
initramfs initrd.img followkernel
disable_splash=1                    # Disable the splash screen
boot_delay=0                        # Disable the boot delay

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

Before the optimization, the boot time was around 2 minutes and 7 seconds.

```
$ systemd-analyze
Startup finished in 10.991s (kernel) + 1min 56.018s (userspace) = 2min 7.009s
graphical.target reached after 1min 52.216s in userspace
```

After the optimization, the boot time was reduced to around 22 seconds.

```
Startup finished in 9.801s (kernel) + 12.304s (userspace) = 22.105s
graphical.target reached after 12.237s in userspace
```

## References

- [BCM2835 ARM Peripherals](https://www.raspberrypi.org/documentation/hardware/raspberrypi/bcm2835/README.md)
- [AXI Monitor](https://forums.raspberrypi.com/viewtopic.php?p=1664415)
- [SPI Register Control](https://forums.raspberrypi.com/viewtopic.php?t=365275)
