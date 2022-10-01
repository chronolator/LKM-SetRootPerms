# LKM-SetRootPerms
Kernel Version: 2.6.x / 3.x / 4.x / 5.x  
Linux Kernel Module (LKM) example of commiting new creds using a hijacked kill syscall.

## Dependencies
Ensure kernel headers, kernel dev packagesm and gcc is installed before building.  

Deb-based Systems:  
```
apt install linux-headers-$(uname -r) build-essential -y
```

RHEL-based Systems:  
```
yum install kernel kernel-devel kernel-headers gcc -y
```

## Install
Clone repository:  
```
git clone https://github.com/chronolator/LKM-SetRootPerms.git
```

Enter folder:  
```
cd LKM-SetRootPerms
```

Build the module:  
```
make
```

Clear dmesg:
```
dmesg --clear
```

Load the module as root:  
```
insmod ./setrootperms.ko
```

To view the SetRootPerms output:  
```
dmesg -T --color
```

## Uninstall
Unload the module as root:  
```
rmmod setrootperms
```

## Usage
Escalate privileges to root
```
kill -64 0
```

Verify
```
id
```

## References
https://github.com/m0nad/Diamorphine
https://github.com/xcellerator/linux_kernel_hacking/blob/master/3_RootkitTechniques/3.3_set_root/rootkit.c
https://xcellerator.github.io/posts/linux_rootkits_02/
https://xcellerator.github.io/posts/linux_rootkits_03/
https://jm33.me/we-can-no-longer-easily-disable-cr0-wp-write-protection.html
https://github.com/maK-/Syscall-table-hijack-LKM
https://github.com/torvalds/linux/blob/b07175dc41babfec057f494d22a750af755297d8/include/linux/syscalls.h
