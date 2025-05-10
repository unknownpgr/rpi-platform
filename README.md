# RPi Platform

A Clean Architecture-based, nearly pure C project for real-time robotics applications on Raspberry Pi.

The goal of this project is to demonstrate that real-time control with high precision (e.g., on the order of several microseconds), typically considered infeasible on a general-purpose operating system, is in fact achievable.

## System Requirements

- Raspberry Pi 3 B+ with Ubuntu Server 22.04 LTS installed.
  - Other versions of Raspberry Pi or OS may work but have not been tested.
- SSH access configured with a key file.
  - Required for automated uploading of code.
- CMake installed on the Raspberry Pi.

## Required Configurations

Edit the `/boot/firmware/cmdline.txt` file to include the following settings:

```
console=serial0,115200 multipath=off dwc_otg.lpm_enable=0 console=tty1 root=LABEL=writable rootfstype=ext4 rootwait fixrtc isolcpus=2,3 quiet loglevel=5 logo.nologo
```

The most critical setting is `isolcpus=2,3`, which isolates CPU cores for the application—this is essential for real-time performance. `quiet loglevel=5 logo.nologo` is optional, but recommended to reduce boot time.

## How to Upload the Code to the Raspberry Pi

Simply run the `upload` script. This script performs the following actions:

1. Prompts for target hostname and path if not previously configured
2. Uses `rsync` to copy files to the target Raspberry Pi, excluding files listed in `config/.rsyncignore`
3. Executes the installation process on the Raspberry Pi

The installation process:

1. Creates a build directory and compiles the project using CMake
2. Copies the systemd service file to `/etc/systemd/system/`
3. Reloads the systemd daemon
4. Enables the service to run on startup
5. Restarts the service
6. Syncs the filesystem to ensure changes are written

You can also:

- Use `./upload reset` to clean the target directory before uploading

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

**Warning: This configuration disables password authentication. If you are using password authentication, make sure to generate a key pair and add the public key to the Raspberry Pi's `~/.ssh/authorized_keys` file.**

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

Edit the `/boot/firmware/config.txt` file with the following content:

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

## Boot Time Analysis

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
