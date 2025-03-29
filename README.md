# RPi Platform

## Requirements

- Raspberry Pi 3 with OS installed
- Keyfile based SSH access configured
- CMake installed on Raspberry Pi

## How `upload.sh` works

1. The script will copy the contents of this directory to the Raspberry Pi
2. Then it will run `run.sh` script on the Raspberry Pi
3. The `run.sh` script will build the project and run it

## Recommended Configurations

#### SSH Configuration

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

#### System Configuration

Below is recommended system configuration for faster boot time

```
$ systemd-analyze
Startup finished in 10.991s (kernel) + 1min 56.018s (userspace) = 2min 7.009s
graphical.target reached after 1min 52.216s in userspace
```

```
sudo systemctl stop snapd
sudo systemctl disable snapd
sudo systemctl mask snapd
```

```
Startup finished in 9.813s (kernel) + 41.636s (userspace) = 51.450s
graphical.target reached after 38.503s in userspace
```

```
12.069s systemd-networkd-wait-online.service
 7.402s cloud-init-local.service
 6.709s snap.lxd.activate.service
 4.460s hciuart.service
 3.882s cloud-config.service
 3.118s cloud-final.service
 3.067s dev-mmcblk0p2.device
 2.853s cloud-init.service
 2.054s snapd.apparmor.service
 2.002s dev-loop3.device
 1.954s dev-loop2.device
 1.781s dev-loop4.device
 1.743s dev-loop5.device
 1.732s dev-loop0.device
 1.687s dev-loop1.device
 1.607s networkd-dispatcher.service
 1.393s systemd-udev-trigger.service
 1.307s systemd-fsck@dev-disk-by\x2dlabel-system\x2dboot.service
  889ms systemd-journal-flush.service
  873ms systemd-logind.service
  793ms ssh.service
  773ms apparmor.service
  717ms apport.service
  706ms systemd-resolved.service
  614ms keyboard-setup.service
  544ms systemd-timesyncd.service
  438ms e2scrub_reap.service
  430ms systemd-tmpfiles-setup-dev.service
  416ms dev-hugepages.mount
  415ms systemd-udevd.service
  403ms dev-mqueue.mount
  389ms sys-kernel-debug.mount
  376ms sys-kernel-tracing.mount
  352ms bluetooth.service
  325ms systemd-journald.service
  324ms lvm2-monitor.service
  322ms kmod-static-nodes.service
  321ms systemd-networkd.service
  298ms modprobe@configfs.service
  292ms modprobe@drm.service
  285ms modprobe@efi_pstore.service
  279ms modprobe@fuse.service
  272ms systemd-tmpfiles-setup.service
  228ms rpi-eeprom-update.service
  192ms systemd-binfmt.service
  192ms systemd-modules-load.service
  176ms systemd-sysusers.service
  169ms snap-core20-2437.mount
  166ms systemd-remount-fs.service
  156ms systemd-sysctl.service
  153ms snap-core20-2499.mount
  151ms wpa_supplicant.service
  147ms rsyslog.service
  136ms snap-lxd-29353.mount
  123ms snap-lxd-31335.mount
  116ms plymouth-read-write.service
  110ms console-setup.service
  106ms snap-snapd-23546.mount
  102ms systemd-random-seed.service
  102ms finalrd.service
   92ms proc-sys-fs-binfmt_misc.mount
   90ms sys-fs-fuse-connections.mount
   82ms snap-snapd-23772.mount
   81ms sys-kernel-config.mount
   79ms systemd-user-sessions.service
   70ms ufw.service
   70ms systemd-update-utmp.service
   67ms bthelper@hci0.service
   58ms plymouth-quit.service
   56ms systemd-rfkill.service
   48ms boot-firmware.mount
   43ms systemd-update-utmp-runlevel.service
   35ms plymouth-quit-wait.service
   29ms setvtrgb.service
  351us blk-availability.service
```

Disable `systemd-networkd-wait-online.service`

```
sudo systemctl disable systemd-networkd-wait-online.service
sudo systemctl mask systemd-networkd-wait-online.service
```

```
Startup finished in 9.822s (kernel) + 28.676s (userspace) = 38.498s
graphical.target reached after 25.515s in userspace
```

```
7.099s cloud-init-local.service
6.708s snap.lxd.activate.service
4.454s hciuart.service
4.019s cloud-config.service
3.145s cloud-final.service
2.993s dev-mmcblk0p2.device
2.955s cloud-init.service
2.134s snapd.apparmor.service
1.956s dev-loop3.device
1.920s dev-loop2.device
1.864s dev-loop5.device
1.766s dev-loop4.device
1.712s dev-loop1.device
1.698s dev-loop0.device
1.617s networkd-dispatcher.service
1.461s systemd-udev-trigger.service
1.366s systemd-fsck@dev-disk-by\x2dlabel-system\x2dboot.service
 950ms systemd-logind.service
 802ms systemd-journal-flush.service
 748ms systemd-resolved.service
 747ms apparmor.service
 712ms e2scrub_reap.service
 678ms ssh.service
 639ms keyboard-setup.service
 578ms apport.service
 551ms systemd-timesyncd.service
 393ms systemd-udevd.service
 364ms systemd-tmpfiles-setup-dev.service
 356ms bluetooth.service
 347ms dev-hugepages.mount
 345ms sys-kernel-tracing.mount
 341ms dev-mqueue.mount
 335ms sys-kernel-debug.mount
 334ms systemd-networkd.service
 307ms systemd-journald.service
 293ms rpi-eeprom-update.service
 292ms kmod-static-nodes.service
 284ms rsyslog.service
 279ms lvm2-monitor.service
 274ms modprobe@drm.service
 268ms systemd-tmpfiles-setup.service
 254ms modprobe@configfs.service
 252ms modprobe@fuse.service
 228ms modprobe@efi_pstore.service
 205ms systemd-modules-load.service
 194ms systemd-remount-fs.service
 185ms systemd-binfmt.service
 182ms systemd-sysusers.service
 182ms systemd-sysctl.service
 153ms snap-core20-2437.mount
 147ms snap-core20-2499.mount
 139ms snap-lxd-29353.mount
 123ms snap-lxd-31335.mount
 121ms snap-snapd-23772.mount
 121ms systemd-random-seed.service
 115ms console-setup.service
 109ms systemd-user-sessions.service
 109ms finalrd.service
 108ms snap-snapd-23546.mount
  99ms plymouth-read-write.service
  99ms sys-fs-fuse-connections.mount
  92ms setvtrgb.service
  90ms sys-kernel-config.mount
  77ms wpa_supplicant.service
  73ms systemd-rfkill.service
  66ms proc-sys-fs-binfmt_misc.mount
  66ms systemd-update-utmp.service
  62ms bthelper@hci0.service
  50ms boot-firmware.mount
  45ms ufw.service
  42ms systemd-update-utmp-runlevel.service
  29ms plymouth-quit-wait.service
  25ms plymouth-quit.service
 339us blk-availability.service
```

```
sudo touch /etc/cloud/cloud-init.disabled
sudo systemctl disable snap.lxd.activate.service
sudo systemctl disable hciuart.service
```

```
Startup finished in 9.801s (kernel) + 12.304s (userspace) = 22.105s
graphical.target reached after 12.237s in userspace
```

```
[all]
kernel=vmlinuz
cmdline=cmdline.txt
initramfs initrd.img followkernel
disable_splash=1 # Disable the splash screen
boot_delay=0 # Disable the boot delay

[pi4]
max_framebuffers=2
arm_boost=1

[all]
# Enable the audio output, I2C and SPI interfaces on the GPIO header. As these
# parameters related to the base device-tree they must appear *before* any
# other dtoverlay= specification
dtparam=audio=on
dtparam=i2c_arm=on
dtparam=spi=on

# Comment out the following line if the edges of the desktop appear outside
# the edges of your display
disable_overscan=1

# If you have issues with audio, you may try uncommenting the following line
# which forces the HDMI output into HDMI mode instead of DVI (which doesn't
# support audio output)
#hdmi_drive=2

# Enable the serial pins
enable_uart=1

# Autoload overlays for any recognized cameras or displays that are attached
# to the CSI/DSI ports. Please note this is for libcamera support, *not* for
# the legacy camera stack
camera_auto_detect=1
display_auto_detect=1

# Config settings specific to arm64
arm_64bit=1
dtoverlay=dwc2

[cm4]
# Enable the USB2 outputs on the IO board (assuming your CM4 is plugged into
# such a board)
dtoverlay=dwc2,dr_mode=host

[all]
```

```
console=serial0,115200 multipath=off dwc_otg.lpm_enable=0 console=tty1 root=LABEL=writable rootfstype=ext4 rootwait fixrtc isolcpus=3,2 quiet loglevel=5 logo.nologo
```
