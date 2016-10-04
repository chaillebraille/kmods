This is the most current release of the ixgbe driver for Linux, which 
supports kernel versions 2.6.18 up through 4.6.1.  It also has been tested on 
the following distributions:

	- RHEL 6.8
	- RHEL 7.2
	- SLES 11SP4
	- SLES 12PS1

Changes in this release:

	- Fix for link crosstalk on some platforms
	- Refactor configuration of RSS
	- Initial SWFW semaphore on startup
	- Module parameter for MDD disablement
	- Assorted other bug fixes

Note that while we attempt to keep the driver version number (4.4.6) in sync 
with its counterpart in the Linux kernel that has similar functionality this 
is far from authoritative.  If you are using a newer kernel.org kernel or 
distro it is likely that its ixgbe driver is at least as up to date as the out 
of tree (OOT) driver found here.
