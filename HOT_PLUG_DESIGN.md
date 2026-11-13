# Hardware hot-plug support

The main feature of ImNAND with its OS connector is to connect to a NAND
flash device, even if that device is connected after the start of the
start of either component, or swapped in runtime.

## Linux

### Introduction

It's quite tricky, because the MTD subsystem doesn't naturally have hot
plug semantics.

Most NAND flash controller drivers assume you never disconnect and
re-connect a NAND flash device during kernel runtime, because raw 
NAND devices are typically soldered onto the board and are not designed
around a hot-pluggable interface.

Special hardware can be developed, so in the actual implementation there
is a mention of such idea.

For the USB NAND flash controller kernel module, we could simply add
a mechanism that either uses uevent or something similar.

The main work flow is that the OS connector has a "mapping" of its NAND
flash memories at all time, either via USB or a platform interface.

A certain platform can have a mix of these - multiple USB adapters, and
multiple platform busses to NAND flash memories.

An appropriate command should be allocated, to retrieve the fixed layout
and another command for finding out what is being connected at the
moment.

For kernel drivers that do not have a hotplug support, we would simply
have a stale MTD device node. There would be no automatic detection,
and another mechanism is needed.

Worse, we could quickly swap NAND devices, and the kernel metadata is
simply not relevant anymore.

### USB NAND flash adapters

The kernel module for a USB NAND flash controller is not written yet,
and it's wise to give it a thought on such case.

We need to make ufedm and the OS-connector (which integrates ufedmd 
within) to be aware of the possibility of USB hotplug - 

For USB adapters, we need to ask the ufedm kernel module to attach a 
new device, based on a mechanism to map all NAND flash devices at the
moment, and remove stale devices, where necessary.

### Detachable sockets

Special hardware can be developed for boards that have a non 
hot-pluggable interface with the help of detachable sockets -
For example, if we use a DIP48-to-TSOP48 socket the main idea is to have
a closing circuit on two pins, so software can easily detect a faulty
socket or a disconnection of such.

That can be used for platform interfaces that normally lack hotplug
support.

It can also be used by USB adapters, to signal that a NAND device has
been removed as well. A proper support in host software (i.e. kernel
module) should be provided for such capability too.

### Actual implementation

The original `ufedmd` component doesn't need to change - we keep it as
the foundation of the OS connector.

However, I should add hot plug capabilities directly to the OS connector
code.

The `ufedm` kernel module needs to adapt - it will have 2 modes,
the first is auto-spawn, which is the original mode, that is already
implemented.

That mode is useful for platforms that don't have any hot-plug capable
interfaces like USB, or simply don't utilize those in a custom design.

It is expected that a swapped device would be then detected after a
system reboot, or another manual mechanism (like a GPIO-based button
that triggers a whole reboot, or just a reload of the ufedm kernel
module).

The second mode will spawn a control device and `/proc/nand` node that
prints all NAND flash memories (i.e. MTD NAND device nodes).

For example, an output from such procfs node can look like this:
```sh
# cat /proc/nand
name blocksize totalsize state driver

mtd1 20000 1000000 stale usbnfc
mtd3 10000 1000000 alive gpmc-nand
mtd4 20000 1000000 alive usbnfc
```

It relies on a hot-pluggable hardware interface - USB is a notable
example, but even a normally non-hotpluggable platform-based interface
can be adapted to have such capability (see #Detachable sockets).

Thus, a proxy MTD can be spawned with a specific ioctl to the control
device. Another ioctl can detach the MTD from the list.

The hot-plug support is deliberately managed by userspace - it's up to
the kernel to expose an event (via USB hot-plug event, for example) in
a sane manner, but userspace is responsible for invoking of the ioctls
and do its own re-map in its runtime's memory structures as well.

## Windows

On Windows it's actually quite easy to handle this, becasue there's no
MTD subsystem at all.

We have one "driver" for the USB NAND flash controller, and that's it.
For remote machines, it's their responsibility to maintain what is
connected at the moment.

