# **ImNAND**

A cross-platform NAND flash programming and configuration utility.

ImNAND (pronounced "I am NAND") is a project aimed at building a
comprehensive and flexible solution for programming and managing NAND
flash memory.

The project combines a graphical NAND configuration editor with a
dedicated OS connector, supporting both direct NAND programming and
Linux MTD passthrough.

The name combines the original project's name with the word NAND,
reflecting its focus on NAND flash memory.

> Project status: Early development / planning. Features and
> architecture described below represent the project's goals and may not
> yet be implemented.

## **Overview**

ImNAND is designed around two primary operating modes:

- Programming mode – Directly program NAND flash memory using
  configurable page layouts, partitions, and other NAND-specific
  parameters.
- MTD passthrough mode – Configure a Linux NAND controller through the
  Linux MTD subsystem, allowing other components of the Linux ecosystem,
  such as UBI and flash-oriented filesystems, to interact with the
  device independently of ImNAND.

Both modes share a common configuration model. This means a NAND
configuration created in the graphical editor can be reused for direct
programming or uploaded to a Linux machine for MTD passthrough
operation.

The project is not intended to be a fork of ImHex. Although ImHex serves
as the foundation and inspiration for the graphical editing experience,
ImNAND has its own specific goals and does not intend to preserve most
of the original project's plugins.

## **Features and goals**

The main objectives of ImNAND are:

- A flexible graphical interface for configuring NAND flash devices.
- Reusable NAND configuration profiles.
- Configurable NAND page layouts and partitions.
- Direct NAND image programming.
- Linux MTD passthrough configuration.
- Local and remote device management.
- A cross-platform OS connector architecture.
- Integration with existing Linux NAND infrastructure.
- A shared configuration model across supported operating systems.

## **Operating modes**

### **Programming mode**

Supported platforms: Windows and Linux

Programming mode provides direct access to NAND flash programming
functionality.

A NAND image can be programmed using a configuration assembled in the
graphical editor. Configuration nodes describe the NAND layout and
programming parameters, including:

- Page layouts.
- Partitions.
- NAND-specific configuration.
- Other programming-related parameters.

Communication with the actual NAND device is handled through an OS
connector service.

The intended backends are:

- Linux: `ufedmd`, which relies on the `ufedm` kernel module.
- Windows: A dedicated userspace implementation using WinUSB.

This architecture separates the graphical application from
platform-specific hardware access.

### **MTD passthrough mode**

Supported platform: Linux only

MTD passthrough mode allows ImNAND to configure a Linux NAND controller
without directly transferring NAND image data.

Instead, ImNAND uploads the relevant configuration to an OS connector
running on the target Linux machine. The connector loads the
configuration into `ufedmd`, allowing the NAND device to operate through
the Linux MTD subsystem.

This mode is particularly useful when integrating NAND storage with the
rest of the Linux ecosystem, including:

- UBI (Unsorted Block Images).
- UBIFS.
- Other MTD-aware tools and filesystems.

Once configured, NAND data handling takes place independently of ImNAND.

In other words, ImNAND acts as a configuration frontend rather than a
data programming utility in this mode.

## **Architecture**

ImNAND consists of two major components.

### **Graphical application**

The main application provides the user interface for creating, editing,
and managing NAND configurations.

It is responsible for:

- Managing NAND configuration profiles.
- Defining page layouts and partitions.
- Selecting the operating mode.
- Connecting to local or remote OS connector instances.
- Initiating programming operations.
- Uploading configurations for MTD passthrough.

The graphical application is intended to be cross-platform.

### **ImNAND OS connector**

The OS connector is a standalone service responsible for communicating
with the underlying NAND hardware.

It operates as a socket-listening daemon and exposes functionality
according to the operating system and available backend.

There are currently two planned backends:

| Backend | Platform | Description | 
| ----- | ----- | ----- | 
| `ufedmd` | Linux | Uses the `ufedm` kernel module and Linux MTD infrastructure. |
| WinUSB | Windows | Provides direct userspace NAND access through WinUSB. |

Each OS connector instance is associated with one backing NAND storage
device.

Multiple connector instances can be deployed to represent multiple NAND
devices, whether they are connected locally or to remote machines.

This makes it possible to manage several NAND devices through a single
graphical application.

## **Operating system support**

The availability of each operating mode depends on the underlying
operating system.

| Operating system | Programming mode | MTD passthrough | 
| ----- |----- | ----- | 
| Linux | Planned | Planned | 
| Windows | Planned | Not supported |
| macOS | Potential future support | Not supported |

### **Linux**

Linux provides the MTD subsystem, making it possible to integrate NAND
devices with existing kernel infrastructure.

Both programming and MTD passthrough modes are intended to be supported.

### **Windows**

Windows does not provide an equivalent to the Linux MTD subsystem.

Consequently, MTD passthrough mode is not applicable. Direct programming
is instead intended to be implemented through a WinUSB-based userspace
backend.

### **macOS**

There is currently no planned macOS implementation.

However, direct programming support may be possible in the future
through an appropriate userspace hardware-access backend.

MTD passthrough is not applicable to macOS.

## **Communication and networking**

The graphical application communicates with OS connector instances
through a socket-based protocol.

The preferred transport is a Unix domain socket (`AF_UNIX`), supported
natively by Linux and modern Windows versions.

For older Windows versions, such as Windows 7, and for remote
connections, regular TCP sockets can be used.

The intended architecture supports both local and remote NAND devices
without requiring the graphical application to implement
platform-specific hardware access.

### **OS connector protocol**

The communication protocol is designed around CBOR serialization and
length-prefixed messages.

The initial protocol design is as follows:

| Property | Specification | | ----- | ----- | | Serialization | CBOR |
| Message framing | 4-byte big-endian length prefix | | Maximum message
size | 16 MB (initially) | | Text encoding | UTF-8 | | Field identifiers
| Integer keys or documented text keys | | Byte order | Big-endian | |
Protocol versioning | Explicit protocol version | | Unknown fields |
Ignore when safe | | Unknown message types | Return an error | | Invalid
CBOR | Reject the message | | Remote transport security | TLS |

All numeric protocol fields are unsigned and encoded in big-endian byte
order. Raw data payloads are transmitted without byte-order conversion.

#### **Request format**

A client request consists of the following header followed by an
optional data payload:
```
+----------------------+------------------+----------------------+ 
| Data length (32-bit) | Command (32-bit) | Request ID (64-bit)  |
+----------------------+------------------+----------------------+ 
| Data (N bytes)                                                 |
+----------------------------------------------------------------+
```

#### **Response format**

A server response consists of the following header followed by an
optional data payload:
```
+----------------------+--------------------------+---------------------+
| Status (16-bit)      | Response length (32-bit) | Request ID (64-bit) |
+----------------------+--------------------------+---------------------+
| Data (N bytes)                                                        |
+-----------------------------------------------------------------------+
```

Request IDs allow clients to associate responses with their
corresponding requests.

#### **Connection handling**

Initially, an OS connector server will accept only one client connection
at a time.

If a client becomes unresponsive or stops communicating, the server will
eventually time out and terminate the connection. This allows another
client, or the original client, to reconnect.

## **Security**

ImNAND is primarily intended for use within trusted private networks,
such as:

- Local loopback connections.
- Private LANs.
- VPN connections.

The initial design does not include an authentication mechanism.

Running an OS connector on an Internet-exposed network without
additional security measures is strongly discouraged.

Remote connections should use TLS, and appropriate firewall rules and
network restrictions should be enforced.

## **Technology**

The project is intended to be written primarily in C.

Keeping the implementation in C provides several advantages:

- Portability across supported operating systems.
- Straightforward integration with existing Linux kernel interfaces.
- Easier integration with the existing `ufedmd` implementation.
- A relatively lightweight OS connector.

### **Planned external libraries**

| Library | Purpose | | ----- | ----- | |
[libuv](https://libuv.org/?utm_source=chatgpt.com) | Cross-platform
asynchronous networking and I/O. | |
[libcbor](https://libcbor.readthedocs.io/?utm_source=chatgpt.com) | CBOR
serialization and deserialization. |

The graphical application is based on the ImHex project, although ImNAND
is intended to evolve independently around its NAND-specific
functionality.

## **License**

The original project is licensed under GNU General Public License v2
(GPL-2.0).

ImNAND intends to retain GPL-2.0 licensing for its own code, subject to
the licensing requirements of the original project and any third-party
components.

Plugin-specific licensing arrangements may be considered where legally
compatible with the project's licensing obligations.

## **Project history**

The project originated from an idea to build a dedicated NAND flash
programming utility for Windows.

In July 2026, the initial concept was named NANDWin, short for:

NAND Without Impractical Nonsense.

Its original description was simply:

NAND flash programming utility for Windows.

During Fall 2026, the scope expanded after discovering the ImHex
project. Rather than maintaining a separate Windows-only application,
the direction shifted towards a cross-platform solution.

This led to the name ImNAND and the idea of building a unified
application that could provide both direct NAND programming and Linux
MTD configuration.

## **Project philosophy**

ImNAND aims to provide a practical and flexible NAND flash programming
environment without unnecessarily reinventing existing infrastructure.

The graphical application should make NAND configuration accessible and
reusable, while the OS connector should handle the platform-specific
details of communicating with the hardware.

Ultimately, the goal is to bring together direct NAND programming and
Linux MTD integration under one unified configuration and management
interface.
