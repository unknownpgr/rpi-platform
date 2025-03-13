# RPi Platform

#### Requirements

- Raspberry Pi 3 with OS installed
- Keyfile based SSH access configured
- CMake installed on Raspberry Pi

#### How `upload.sh` works

1. The script will copy the contents of this directory to the Raspberry Pi
2. Then it will run `run.sh` script on the Raspberry Pi
3. The `run.sh` script will build the project and run it

#### Recommended

- Below is recommended `/etc/ssh/sshd_config` configuration for faster SSH access

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
