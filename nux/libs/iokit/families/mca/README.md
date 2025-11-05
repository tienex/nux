# MCA (Micro Channel Architecture) Family for IOKit

Comprehensive implementation of IBM's Micro Channel Architecture bus support for IOKit.

## Overview

MCA (Micro Channel Architecture) was IBM's proprietary 32-bit bus introduced with the PS/2 in 1987 to replace the ISA bus. This implementation provides full driver support for MCA systems.

## Features

### Bus Support
- **16/32-bit bus operations**: Full support for both 16-bit and 32-bit MCA variants
- **10 MHz bus clock**: Standard MCA timing
- **Up to 320 MB/s bandwidth**: Streaming data mode support
- **8-16 expansion slots**: Typical configuration support

### POS (Programmable Option Select) Registers
- Complete POS register access (0x100-0x107 per slot)
- Adapter ID reading (16-bit unique identifier)
- Card enable/disable control
- Software configuration (no DIP switches)

### Advanced Features
- **Bus Arbitration**: Multi-master bus arbitration support with fairness and priority modes
- **Streaming Data Mode**: High-bandwidth streaming transfers up to 320 MB/s
- **Shared Slots**: Support for multi-slot adapters
- **Bus Mastering**: Full bus master arbitration and control
- **Level-Triggered Interrupts**: Standard MCA interrupt handling

### Device Database
- **59 known MCA cards** from major manufacturers:
  - IBM (VGA, XGA, 8514/A, Token Ring, Ethernet, SCSI, Memory)
  - Adaptec (AHA-1640, AHA-1641, AHA-1650 SCSI)
  - 3Com (3C523, 3C529, 3C527 Ethernet)
  - Western Digital / SMC (WD8003E/A, WD8013E/A, Elite16)
  - Intel (EtherExpress, TokenExpress)
  - ATI (Mach32), Matrox (MGA)
  - BusLogic, Future Domain, DPT, NCR (SCSI controllers)
  - Sound cards (Pro AudioSpectrum, Roland MPU-401)

### Supported Systems
- IBM PS/2 Models: 50, 55, 60, 65, 70, 80, 90, 95
- IBM RS/6000 workstations
- NCR System 3000 series
- Tandy 5000 MC
- Reply Model 32
- IBM servers (3Server, AS/400)

## Files

### include/mca.h (1,079 lines)
Complete header with:
- IIOMCABus and IIOMCADevice COM interfaces
- POS register definitions and constants
- MCA device ID structures
- Bus arbitration and streaming configurations
- 41 predefined adapter IDs for common cards
- Comprehensive enumerations and structures

### sources/mca.c (1,074 lines)
Full implementation including:
- MCA bus detection logic
- POS register access functions
- Device enumeration and identification
- 59-entry card database with real MCA adapters
- Bus arbitration implementation
- Streaming data mode support
- Complete COM interface implementations

### Makefile.in (22 lines)
Standard IOKit family build configuration

## Technical Specifications

### Bus Characteristics
- **Data Bus Width**: 16-bit or 32-bit
- **Address Bus**: 24-bit (16 MB) or 32-bit (4 GB)
- **Clock Frequency**: 10 MHz
- **Transfer Modes**:
  - Standard: 20-40 MB/s
  - Burst: 40-80 MB/s  
  - Streaming: up to 320 MB/s

### POS Register Layout
- **0x100-0x101**: Adapter ID (16-bit)
- **0x102**: Option Select 1 (card enable, I/O decode)
- **0x103**: Option Select 2 (memory/IRQ decode)
- **0x104**: Option Select 3 (DMA/arbitration)
- **0x105**: Option Select 4 (extended options)
- **0x106-0x107**: Subaddress extension

### Resource Allocation
- I/O Ports: Multiple ranges per card
- Memory: Shared memory regions (cacheable/non-cacheable)
- Interrupts: IRQ 3, 4, 5, 7, 9, 10, 11, 12, 14, 15 (level-triggered)
- DMA: 8/16/32-bit DMA channels

## API Usage

### Bus Detection
```c
BOOLEAN bPresent;
MCADetect(&bPresent);
```

### Bus Initialization
```c
IIOMCABus *pBus;
IOMCABusCreate(&pBus);
```

### Device Enumeration
```c
IIOMCADevice *devices[16];
UINT32 count = 16;
IIOMCABus_EnumerateSlots(pBus, devices, &count);
```

### Reading POS Registers
```c
UINT8 posValue;
IIOMCABus_ReadPOS(pBus, slotNum, MCA_POS_OPTION_1, &posValue);
```

### Card Database Lookup
```c
CONST MCA_CARD_DB_ENTRY *pEntry;
MCALookupCard(adapterID, &pEntry);
if (pEntry != NULL) {
    printf("Found: %s %s\n", pEntry->pszVendor, pEntry->pszName);
}
```

## Card Categories

The implementation supports the following card categories:
- **Disk Controllers**: ST-506, ESDI controllers
- **Display Adapters**: VGA, 8514/A, XGA, XGA-2
- **Network Adapters**: Ethernet, Token Ring
- **SCSI Controllers**: Fast SCSI, Fast-Wide SCSI
- **Communications**: Serial, Parallel, Multi-protocol
- **Memory Expansion**: 2-64 MB modules
- **Audio**: Sound cards, MIDI interfaces
- **System**: Motherboard components, CPU complexes

## Implementation Notes

### COM Interface Pattern
Follows standard IOKit COM patterns with:
- IUnknown methods (QueryInterface, AddRef, Release)
- IIOService inheritance
- Family-specific methods

### Bus Arbitration
Implements simplified arbitration with:
- Fairness mode (round-robin)
- Priority mode (level-based)
- Timeout handling

### Streaming Data Mode
Supports high-bandwidth streaming with:
- Configurable burst lengths
- Preemption control
- Bandwidth allocation

## Historical Context

MCA was introduced in 1987 as IBM's replacement for ISA, featuring:
- Software configuration (vs. DIP switches)
- Bus mastering
- Higher bandwidth
- Better noise immunity

However, it faced challenges:
- Proprietary licensing
- Incompatible with ISA cards
- Higher cost

MCA was used primarily in IBM PS/2, RS/6000, and some high-end servers until the mid-1990s when PCI became dominant.

## License

Copyright (c) 2025 NUX Project
