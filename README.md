# Hue

## Installation
Find the IP address of your Hue bridge and follow [these](https://developers.meethue.com/develop/get-started-2/) instructions to create an authenticated API user.

```bash
git clone https://github.com/chriswmartin/Hue.git
cd Hue
echo "[hue bridge user] [hue bridge IP address]" > env
Make
```

## Usage
```bash
./hue [group] [action]

toggle a group's lights on/off
./hue --living-room --toggle

turn all lights in a group off
./hue --living-room --off

turn all lights in a group on
./hue --living-room --on

change the color of all lights in a group
./hue --living-room --red=0 --green=120 --blue=120 --color

set all lights in a group to their default color
./hue --living-room --color
```
