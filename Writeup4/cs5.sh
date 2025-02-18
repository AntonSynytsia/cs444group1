# After logging in, switch mode
sudo su
# optional: turn off lights
echo none > /sys/class/leds/led0/trigger
echo none > /sys/class/leds/led1/trigger
# launch
echo morse > /sys/class/leds/led1/trigger
# getting speed
cat /sys/class/leds/led1/speed
# setting speed (clamped between 1 - 1000)
echo 10 > /sys/class/leds/led1/speed
# getting message
cat /sys/class/leds/led1/message
# setting message
echo "Hello world" > /sys/class/leds/led1/message
# getting mode
cat /sys/class/leds/led1/mode
# setting "one-off" mode
echo 0 > /sys/class/leds/led1/mode
# setting "repeat" mode
echo 1 > /sys/class/leds/led1/mode
