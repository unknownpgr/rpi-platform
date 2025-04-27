# RPi Platform

Clean-Architecture based, almost pure C project for Raspberry Pi based real time robotics applications.

## Requirements

- Raspberry Pi 3 with OS installed.
- Keyfile based SSH access configured.
- CMake installed on Raspberry Pi.

## How `upload.sh` works

1. The script will copy the contents of this directory to the Raspberry Pi.
2. Then it will run `run.sh` script on the Raspberry Pi.
3. The `run.sh` script will build the project and run it.

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

#### Boot Configuration

Edit the `/boot/firmware/cmdline.txt` file to include the following settings:

```
console=serial0,115200 multipath=off dwc_otg.lpm_enable=0 console=tty1 root=LABEL=writable rootfstype=ext4 rootwait fixrtc isolcpus=3,2 quiet loglevel=5 logo.nologo
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

## Useful References

- [BCM2835 ARM Peripherals](https://www.raspberrypi.org/documentation/hardware/raspberrypi/bcm2835/README.md)
- [AXI Monitor](https://forums.raspberrypi.com/viewtopic.php?p=1664415)
- [SPI Register Control](https://forums.raspberrypi.com/viewtopic.php?t=365275)
