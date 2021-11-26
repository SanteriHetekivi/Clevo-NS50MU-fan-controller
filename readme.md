# This version

This is fork from [Vega D's implementation](https://gitlab.com/vega-d/clevo-ns50mu-fan-controller) witch is fork from [François Kneib's one](https://gitlab.com/francois.kneib/clevo-N151ZU-fan-controller).

## Prerequisites

Since this is C++, for this to compile you will need `gcc` and `build-essential`.

On debian-based distros (Ubuntu, ...) you can install them with this command:

```
sudo apt install -y git gcc build-essential
```

## Installing

Clone the repo:

```
git clone https://gitlab.com/vega-d/clevo-ns50mu-fan-controller.git
```

Go into the project folder:

```
cd clevo-ns50mu-fan-controller
```

Build & install:

```
sudo make all
```

This will:

1. build the c++ file into ns50mu-fan-controller and make it executable,
2. copy ns50mu-fan-controller bin file into /usr/local/bin,
3. copy the service file ns50mu-fan-controller.service into /etc/systemd/system/,
4. enable the service at startup,
5. launch the service.

You can now check that the service is running:

```
systemctl status ns50mu-fan-controller.service
```
## Note

To watch for the temperature of your cpu (for example with a desktop widget), you can use the following command (in millidegrees):

```
cat /sys/class/thermal/thermal_zone0/temp
```

If you want the daemon to write logs in syslog, use `sudo make -VERBOSE=ON all`.

## Authors

* **François Kneib** - *Initial work for N151ZU*
* **Vega D** - *Adaptation for NS50MU*
* **Santeri Hetekivi** - *This custom implementation for NS50MU on Arch*

## License

This project is licensed under the GNU General Public License v3 or any later version (GPL-3.0-or-later). See [LICENSE](LICENSE).

## Acknowledgments

This project uses some code from TuxedoFanControl program: https://github.com/tuxedocomputers/tuxedo-fan-control

 
