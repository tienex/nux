# IOKit Families - TODO List

## ✅ FULLY IMPLEMENTED (32 Families)

### Platform Firmware (1 family)
- [x] **Platform Family** - Universal device enumeration and matching across firmware interfaces
  - [x] ACPI device matcher (Hardware ID, Compatible ID, UID, path)
  - [x] ISA Plug and Play matcher (CSN/LDN, resource allocation)
  - [x] Device Tree matcher (DTB/DTS, compatible strings, phandle)
  - [x] OpenFirmware matcher (IEEE 1275, device-type, properties)
  - [x] ARC/ARCS matcher (component class/type, configuration data)
  - [x] PCI/PCIe matcher (vendor/device ID, class code, BDF)
  - [x] Legacy Bus matcher (NuBus, Zorro, SBus, TURBOchannel, MCA, VMEbus, UNIBUS, Q-bus, C-bus, S-100)
  - [x] 150+ ACPI HID database entries
  - [x] Platform firmware detection (BIOS, UEFI, DT, OF, ARC)

### Core Abstractions (2 families)
- [x] **Bus Family** - Unified bus interface for all system buses (30+ bus types)
- [x] **Storage Family** - Unified block storage abstraction (NVMe, SATA, SCSI, etc.)

### System Infrastructure (2 families)
- [x] **Timer/Clock Family** - PIT, RTC, HPET, TSC, APIC timers
- [x] **Power Management Family** - ACPI 1.0-6.5, battery, thermal zones

### Bus Families (9 families)
- [x] **PCIe Family** - Full PCI/PCI-X/PCIe support (Gen 1-6, 32/64-bit, AGP)
- [x] **Thunderbolt Family** - TB 1/2/3/4/5 + USB4 v1/v2 (120 Gbps)
- [x] **USB Family** - USB 1.0-4.0, USB-C PD (240W), 40+ controllers
- [x] **FireWire Family** - IEEE 1394a/b/c (100-3200 Mbps)
- [x] **ISA/EISA/VLB Family** - Complete legacy PC bus support
  - [x] 8-bit ISA bus support (XT bus)
  - [x] 16-bit ISA bus support (AT bus)
  - [x] I/O port address decoding (0x000-0x3FF)
  - [x] Memory address decoding (0xA0000-0xFFFFF)
  - [x] DMA controller support (8237A)
  - [x] IRQ handling (8259A PIC)
  - [x] ISA device enumeration
  - [x] 32-bit EISA bus support
  - [x] EISA configuration space access
  - [x] EISA ID and slot configuration
  - [x] EISA DMA (32-bit transfers)
  - [x] Extended IRQ support
  - [x] EISA device auto-configuration
  - [x] VL-Bus slot detection
  - [x] VL-Bus device enumeration
  - [x] 32-bit local bus transfers
  - [x] VL-Bus timing and clock configuration
  - [x] Video card support
  - [x] ISA PnP protocol implementation
  - [x] Read Port (0x279) and Address Port (0xA79)
  - [x] Resource data parsing
  - [x] Logical device configuration
  - [x] ISA PnP BIOS interface
  - [x] Device activation/deactivation
  - [x] Conflict resolution
  - [x] Common ISA PnP IDs (e.g., PNP0501 for serial ports)
  - [x] 100+ device database entries
- [x] **I2C Family** - SMBus, I2C 1.0-5.0 (up to 5 MHz)
- [x] **SPI Family** - SPI, Quad-SPI, Octal-SPI (up to 400 MHz)
- [x] **HID Family** - USB HID, PS/2, Bluetooth HID

### Storage Protocol Families (3 families)
- [x] **NVMe Family** - NVMe 1.0-2.0, NVMe-oF, ZNS, multipath
- [x] **SATA Family** - SATA 1.0-3.5 (up to 16 Gbps), AHCI
- [x] **SCSI Family** - SCSI-1 through Ultra640, SAS-4 (22.5 Gbps)

### High-Performance Networking (2 families)
- [x] **RDMA Family** - InfiniBand, RoCE, iWARP, 40 vendors
- [x] **Network Family** - Ethernet 10M-400G, 85+ NICs

### Multimedia (2 families)
- [x] **Display Family** - 123 GPUs (NVIDIA, AMD, Intel, ARM, Apple)
- [x] **Audio Family** - 103 codecs (HD Audio, AC'97, Sound Blaster)

### Security (1 family)
- [x] **Crypto Family** - TPM 1.2/2.0, 32 HW accelerators, 40+ algorithms

### Virtualization (1 family)
- [x] **Virtualization Family** - SR-IOV, virtio, VMware, Hyper-V, Xen

### Legacy/Vintage Computer Buses (10 families)
- [x] **NuBus Family** - Apple Macintosh II series (6 slots, 10 MB/s, Declaration ROM, 42+ cards)
- [x] **Zorro Family** - Commodore Amiga 2000/3000/4000 (Zorro II/III, AutoConfig, 42+ cards)
- [x] **SBus Family** - Sun Microsystems SPARCstation (FCode, DVMA, 32+ cards)
- [x] **TURBOchannel Family** - DEC DECstation/AlphaStation (100 MB/s, 35+ cards)
- [x] **MCA Family** - IBM PS/2/RS6000 (Micro Channel, POS registers, 59+ cards)
- [x] **VMEbus Family** - Industrial/military/aerospace (VME64/64x, 7-level interrupts, 32+ cards)
- [x] **UNIBUS Family** - DEC PDP-11 (18-bit, 256 KB, vectored interrupts, 57+ devices)
- [x] **Q-bus Family** - DEC PDP-11/MicroVAX/VAXstation (16/18/22-bit, CSR, 51+ modules)
- [x] **C-bus Family** - NEC PC-9801 series (8/16-bit, Japanese market, 48+ cards)
- [x] **S-100 Bus Family** - MITS Altair 8800 and compatibles (IEEE 696, 100-pin, 41+ cards)

**Total: 32 families, 70,000+ lines of code, 1,400+ device IDs, 110+ COM interfaces**

---

## 🟡 Medium Priority - Not Yet Implemented

### Future Bus Families

#### PC/104
- [ ] PC/104 detection
- [ ] ISA-compatible signals
- [ ] Stackthrough connectors
- [ ] Embedded/industrial support

#### CompactPCI
- [ ] cPCI 2.0/3.0/6.0 support
- [ ] Hot-swap capability
- [ ] System slot detection
- [ ] PICMG standards compliance

## Low Priority 🟢

### Historical/Legacy Buses

#### MultiBus (Intel)
- [ ] MultiBus I support
- [ ] MultiBus II support
- [ ] Message passing
- [ ] Multi-master arbitration

**Specifications:**
- 16/20-bit addressing (Multibus I)
- 32-bit with messaging (Multibus II)
- Used in: Industrial computers, Intel SBC

#### STD bus
- [ ] STD-32 bus support
- [ ] 8-bit backplane
- [ ] Z80/6502 support

#### CardBus
- [ ] 32-bit CardBus support (completed in PCIe driver)
- [ ] PCMCIA compatibility
- [ ] Hot-plug support
- [ ] Power management

## Future Enhancements 🔮

### Modern High-Speed Interconnects

#### CXL (Compute Express Link)
- [ ] CXL 1.0/1.1/2.0/3.0 support
- [ ] Memory pooling
- [ ] Cache coherency
- [ ] Type 1/2/3 devices

#### GenZ
- [ ] GenZ fabric support
- [ ] Memory-semantic operations
- [ ] Component/fabric managers

#### CCIX (Cache Coherent Interconnect)
- [ ] CCIX protocol support
- [ ] Multi-chip coherency
- [ ] Heterogeneous compute

### Automotive Buses

#### CAN (Controller Area Network)
- [ ] CAN 2.0A/2.0B support
- [ ] CAN FD (Flexible Data-rate)
- [ ] ISO 11898 compliance

#### FlexRay
- [ ] FlexRay protocol support
- [ ] Time-triggered communication
- [ ] Fault-tolerant design

#### MOST (Media Oriented Systems Transport)
- [ ] MOST25/50/150 support
- [ ] Isochronous/packet data
- [ ] Ring topology

### Industrial Buses

#### Profibus
- [ ] Profibus DP/PA support
- [ ] Master-slave communication
- [ ] Industrial protocols

#### CANopen
- [ ] CANopen protocol stack
- [ ] Object dictionary
- [ ] PDO/SDO support

## Implementation Guidelines

### General Architecture

All bus drivers should follow this pattern:

```c
// 1. Bus detection and initialization
IO_RETURN BusInitialize(void);

// 2. Device enumeration
IO_RETURN BusScanDevices(IIODevice **ppDevices, UINT32 *puCount);

// 3. Device interface
typedef struct IIOBusDevice {
    // Configuration access
    IO_RETURN (*ConfigRead)(UINT32 uOffset, UINT32 *puValue);
    IO_RETURN (*ConfigWrite)(UINT32 uOffset, UINT32 uValue);

    // Resource management
    IO_RETURN (*AllocateResources)(void);
    IO_RETURN (*FreeResources)(void);

    // Interrupt handling
    IO_RETURN (*EnableInterrupt)(UINT32 uVector);
    IO_RETURN (*DisableInterrupt)(UINT32 uVector);

    // DMA support (if applicable)
    IO_RETURN (*SetupDMA)(DMA_CONFIG *pConfig);
} IIOBusDevice;
```

### Testing Requirements

For each bus implementation:
- [ ] Unit tests for device detection
- [ ] Configuration space access tests
- [ ] Resource allocation tests
- [ ] Interrupt handling tests
- [ ] DMA transfer tests (if applicable)
- [ ] Hot-plug tests (if applicable)
- [ ] Multi-device enumeration tests
- [ ] Thread-safety tests

### Documentation Requirements

- [ ] Bus specification summary
- [ ] Supported devices list
- [ ] Configuration examples
- [ ] Known limitations
- [ ] Performance characteristics
- [ ] Compatibility notes

## Priority Ordering Rationale

**High Priority (ISA/EISA/VLB/ISA-PnP):**
- Still found in embedded systems
- Required for legacy PC compatibility
- ISA PnP provides auto-configuration
- Simple implementation

**Medium Priority (NuBus/Zorro/SBus/TURBOchannel):**
- Historical significance
- Retro computing community
- Well-documented specifications
- Limited modern use

**Low Priority (UNIBUS/Q-bus/VME/etc.):**
- Mainly historical interest
- Very specialized applications
- Complex implementations
- Limited hardware availability

## Contributing

When implementing a new bus driver:

1. Create header file in `include/public/iokit/families/`
2. Implement in `sources/families/`
3. Add examples in `examples/`
4. Update this TODO list
5. Add tests
6. Update README.md

## References

- PCI specifications: https://pcisig.com/specifications
- ISA PnP specification: Microsoft Plug and Play ISA Specification v1.0a
- EISA specification: EISA Specification Version 3.12
- NuBus specification: IEEE 1196-1987
- Zorro specification: Commodore Amiga Hardware Reference Manual
- TURBOchannel: DEC TURBOchannel Technical Overview
- VMEbus: VITA standards (ANSI/VITA 1-1994)
