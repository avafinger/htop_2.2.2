# htop_2.2.2

Ubuntu 20.04 LTS deb package

Fix a Crash when exiting htop, htop-2.2.2-11 deb package released with latest code.


Ubuntu 19.10 EOAN deb package

* htop for monitoring CPU Freq, CPU Temp, GPU Temp, VCore on arm64 boards
* Extended info: Kernel version, OS version, Armbian version, eth0 IP, eth1 IP, wlan0 IP, wlan1 IP, alias
* Configurable handlers

**Branches:**

* master
  RK3399 mainline kernel
* BSP_RK3399
  RK3399 BSP kernel
* generic
  x64 / Arm kernel
* armhf
  arm kernel (mainline)

![Htop screenshot](https://github.com/avafinger/htop_2.2.2/raw/master/htop_2.2.2.png)

![RK3399](https://github.com/avafinger/htop_2.2.2/raw/master/rk3399.png)

![F2 config](https://github.com/avafinger/htop_2.2.2/raw/master/armbian.png)

# deb packages

* arm64 (https://github.com/avafinger/htop_2.2.2/releases/tag/v1.0)

    htop_2.2.2-1_arm64.deb

* arm64 (https://github.com/avafinger/htop_2.2.2/releases/tag/v1.2)

    htop_2.2.2-2_arm64.deb
    
    
## Enhanced version

Htop 2.2.2-11 (arm64)

https://github.com/avafinger/htop_2.2.2/releases/tag/v1.9


Htop 2.2.2-8 (arm64 and big.LITTLE)

https://github.com/avafinger/htop_2.2.2/releases/tag/v1.8

* Bug fixes
* CPU Temp
* CPU Freq
* GPU Temp
* Eth0 / Eth1 stats
* Wlan0 stats
* fs stats
* Logged in counter
* Block dev stats

    
## Configurable handlers and Alias

The path to grab information depends on how kernel and DTB are built, they change from kernel version and different hardware, handlers have been added to the htop configuration file so you can adjust it to your need.

File configuration is in **/root/.config/htop** and there is a section you can edit and change the handlers with an editor or programatically.

		# Beware! This file is rewritten by htop when settings are changed in the interface.
		# The parser is also very primitive, and not human-friendly.
		fields=0 48 17 18 38 39 40 2 46 47 49 1
		sort_key=46
		sort_direction=1
		hide_threads=0
		hide_kernel_threads=1
		hide_userland_threads=0
		shadow_other_users=0
		show_thread_names=0
		show_program_path=1
		highlight_base_name=0
		highlight_megabytes=1
		highlight_threads=1
		tree_view=0
		header_margin=1
		detailed_cpu_time=0
		cpu_count_from_zero=0
		update_process_names=0
		account_guest_in_cpu_meter=0
		color_scheme=0
		delay=15
		left_meters=AllCPUs Memory Swap Kernelversion OSversion Armbianversion
		left_meter_modes=1 1 1 2 2 2
		right_meters=Tasks LoadAverage Uptime Eth0 CpuFreq CpuTemp GpuTemp
		right_meter_modes=2 2 2 2 2 2 2
		# SBC hardware and Kernel specific path.
		# Editable manually.
		BoardName=
		CpuFreq_handler=
		CpuTemp_handler=/sys/class/hwmon/hwmon1/temp2_input
		CpuVCore_l_handler=
		CpuVCore_b_handler=
		GpuVCore_handler=
		GpuTemp_handler=/sys/class/hwmon/hwmon2/temp1_input
		# Wlan / Eth alias
		eth0_alias=enp2s0
		eth1_alias=
		wlan0_alias=
		wlan1_alias=

**They are as follow:**

* CpuFreq_handler (CPUFREQ)
* CpuTemp_handler (CPUTEMP)
* CpuVCore_l_handler (CPUVCORE Little cores or normal)
* CpuVCore_b_handler (CPUVCORE Big cores)
* GpuVCore_handler (CPUVCORE)
* GpuTemp_handler (GPUTEMP)

## RK3399 CPUVCORE handlers (kernel 5.7.9)

	CpuVCore_l_handler=/sys/class/regulator/regulator.15/microvolts
	CpuVCore_b_handler=/sys/class/regulator/regulator.12/microvolts


**Alias**

Some kernel changes eth0 / eth1 /wlan0 / wlan1 names like so:

    enp2s0: renamed from eth0

in this case you add the alias here:

eth0_alias=**enp2s0**


# License

[![Build Status](https://travis-ci.org/hishamhm/htop.svg?branch=master)](https://travis-ci.org/hishamhm/htop)
[![PayPal donate](https://img.shields.io/badge/paypal-donate-green.svg)](http://hisham.hm/htop/index.php?page=donate)

[htop](http://hisham.hm/htop/)
====

by Hisham Muhammad <hisham@gobolinux.org> (2004 - 2016)

Introduction
------------

This is `htop`, an interactive process viewer.
It requires `ncurses`. It is developed primarily on Linux,
but we also have code for running under FreeBSD and Mac OS X
(help and testing are wanted for these platforms!)

This software has evolved considerably over the years,
and is reasonably complete, but there is always room for improvement.

Comparison between `htop` and classic `top`
-------------------------------------------

* In `htop` you can scroll the list vertically and horizontally
  to see all processes and full command lines.
* In `top` you are subject to a delay for each unassigned
  key you press (especially annoying when multi-key escape
  sequences are triggered by accident).
* `htop` starts faster (`top` seems to collect data for a while
  before displaying anything).
* In `htop` you don't need to type the process number to
  kill a process, in `top` you do.
* In `htop` you don't need to type the process number or
  the priority value to renice a process, in `top` you do.
* In `htop` you can kill multiple processes at once.
* `top` is older, hence, more tested.

Compilation instructions
------------------------

This program is distributed as a standard autotools-based package.
See the [INSTALL](/INSTALL) file for detailed instructions.

When compiling from a [release tarball](https://hisham.hm/htop/releases/), run:

    ./configure && make

For compiling sources downloaded from the Git repository, run:

    ./autogen.sh && ./configure && make

By default `make install` will install into `/usr/local`, for changing
the path use `./configure --prefix=/some/path`.

See the manual page (`man htop`) or the on-line help ('F1' or 'h'
inside `htop`) for a list of supported key commands.

If not all keys work check your curses configuration.

## License

GNU General Public License, version 2 (GPL-2.0)

