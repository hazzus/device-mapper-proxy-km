# device-mapper-proxy-km
Simple kernel module to track some stats while performing I/O operations with virtual block devices  
Shortly: `dmp`  
## Tested on: 
* Distros: Ubuntu 20.04 LTS, Ubuntu 18.04 LTS  
* Linux kernel versions: 5.3.0-28-generic, 5.3.0-51-generic  
# Preparations
For building perform:  
Ubuntu: `sudo apt-get install build-essential linux-headers-$(uname -r) libdevmapper-dev`  
Or use your package-manager to install those
# Usage
1. Run `make` to compile module
2. Run `sudo insmod dmp.ko` for installing module
3. Now device-mapper has new target type: `dmp`.  
It requires 2 arguments in table when creating: original destination device and rather reload stats or not (maybe usefull 
if creating many proxies). Can be tested via `dmsetup`

# Example: 
```shell
# creating a dumb-zero target original vbd:
sudo dmsetup create dumb --table "0 512 zero"
# creating our proxy:
sudo dmsetup create proxy --table "0 512 dmp /dev/mapper/dumb 0"
# now all I/O requests to "/dev/mapper/proxy" will originaly go to "/dev/mapper/dumb"
# moreover some stats: amount of requests and their average size
# will be written to "/sys/module/dmp/stat/op_stat"

# look at this:
sudo dd if=/dev/mapper/proxy of=smth_here bs=8k count=2
# this command should create a file "smth_here" with size 16k and write zeroes to it
# check with:
hexdump smth_here # should be zeroes
ls -al | grep smth_here # should be 16k

# stats:
sudo cat /sys/module/dmp/stat/op_stat

# new proxy with reloading stats (cleaning stats in /sys/module/dmp/stat/op_stat) :
sudo dmsetup create proxy2 --table "0 512 dmp /dev/mapper/dumb 1"
# without:
sudo dmsetup create proxy3 --table "0 512 dmp /dev/mapper/dumb 0"
```

# Automatical testing
1. Use `make setup` to automate setup steps.
2. Use `make load` to generate some load on setuped device and show stats.
