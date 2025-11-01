# Zoo64 Archive Format Specification

**Version:** 1.0 Draft
**Date:** 2025-10-31
**Status:** Specification Draft


## Table of Contents

### PART I: CORE SPECIFICATION (REQUIRED)
1. Introduction
2. Archive Structure
3. File Format
   - Archive Header (REQUIRED)
   - File Entry (REQUIRED)
   - Central Directory (REQUIRED)
4. Compression (REQUIRED: at least "stored" mode)
5. Metadata Storage (REQUIRED: basic timestamps)

### PART II: OPTIONAL FEATURES
6. Advanced Compression (LZ77, LZMA, ZSTD...)
7. Encryption (AES-256-GCM, ChaCha20-Poly1305...)
8. Data Integrity (Hash verification, FEC)
9. Digital Signatures
10. Quick Directory (Fast listing)
11. Multi-Volume Archives

### PART III: PLATFORM EXTENSIONS (OPTIONAL)
12. Filesystem Metadata (ACLs, xattrs, ADS...)
13. VCS Metadata (Git, SVN, Mercurial...)
14. Legacy System Support (CP/M, DOS, WIM...)

### PART IV: IMPLEMENTATION
15. Compression Pipeline Details
16. COM Component Interface  
17. Error Handling
18. Classic Zoo Compatibility

---

## Conformance Levels

Zoo64 defines seven conformance levels to support implementations ranging from ultra-tiny parsers to full-featured enterprise archival solutions.

### Tiny Conformance
Absolute bare minimum for parsing:
- Archive Header (magic, version only)
- File Entry (path, size, offset)
- **NO Central Directory** (sequential scan only)
- **NO timestamps** (optional modification time only)
- **NO compression** (stored files only)
- **NO checksums** (CRC32 optional)
- **NO End of Archive marker** (optional)
- UTF-8 paths (basic support, no validation)

**Target**: Minimal parsers, toy implementations, learning/testing, absolute smallest code footprint (< 1KB)

### Compact Conformance
Ultra-minimal streaming implementation without random access:
- Archive Header structure (minimal flags)
- File Entry with path and minimal metadata (size, CRC32)
- End of Archive Marker
- Stored (uncompressed) files only
- UTF-8 paths (basic support, ASCII-safe subset recommended)
- **NO Central Directory** (sequential access only)
- Streamable mode required

**Target**: Network streaming, pipes, stdin/stdout processing, single-pass extraction

### Embedded Conformance
Minimal implementation for resource-constrained systems:
- Archive Header structure
- File Entry with path and basic metadata (size, CRC32, modification time only)
- Central Directory (simplified entries)
- End of Archive Marker
- Stored (uncompressed) files only
- UTF-8 paths (basic support, no full normalization required)
- Random access via Central Directory

**Target**: Microcontrollers, firmware updates, bootloaders, IoT devices, ROM filesystems

### Minimal Conformance (REQUIRED)
Basic Zoo64 implementation MUST support:
- Archive Header structure
- File Entry with path and basic metadata (timestamps, size, CRC32)
- Central Directory (REQUIRED at this level)
- Stored (uncompressed) files
- UTF-8 paths with full normalization (NFC, separator normalization)

**Target**: Simple archiving tools, minimal dependencies, basic file bundling

### Standard Conformance (RECOMMENDED)
Standard implementations SHOULD add:
- At least one modern compression algorithm (ZSTD or LZMA2 recommended)
- Quick Directory for fast listing
- Block deduplication
- SHA-256 file hashes
- Basic extended metadata (Unix permissions, Windows attributes)

**Target**: General-purpose archiving, backup utilities, software distribution

### Enhanced Conformance
Advanced implementations with professional features:
- Multiple compression algorithms (ZSTD, LZMA2, LZ4, ZLIB, Brotli)
- Encryption (AES-256-GCM, ChaCha20-Poly1305)
- Seekable compression
- Digital signatures (RSA-2048, Ed25519)
- Core metadata types (ACLs, xattrs, ADS, Security Descriptors)
- Streamable mode support
- SHA-256/SHA-512/BLAKE3 hashing

**Target**: Professional backup software, compliance archiving, secure distribution

### Full Conformance (OPTIONAL)
Complete feature set implementations MAY include:
- All compression algorithms (21+ algorithms including PAQ, legacy formats)
- All encryption methods and KDFs
- Data integrity (Reed-Solomon FEC, PAR2)
- All digital signature algorithms
- All platform-specific metadata (95+ metadata types)
- All filesystem metadata (APFS, ReFS, ZFS, etc.)
- VCS metadata (Git, SVN, Perforce, Mercurial, etc.)
- Multi-volume archives
- All operating modes (Streamable, Tape-Friendly, HSM)
- Solid compression with seekable blocks

**Target**: Enterprise backup, cross-platform migration, digital preservation, compliance archiving

## Conformance Summary Tables

### Table 1: Conformance Levels Overview

| Level | Designation | Random Access | Compression | Metadata | Checksums | Target Use Cases |
|-------|-------------|---------------|-------------|----------|-----------|------------------|
| **Tiny** | Optional | No | None | None (size only) | Optional | Minimal parsers, learning, testing |
| **Compact** | Optional | No (Stream only) | None (Stored) | Minimal | CRC32 | Network streaming, pipes, stdin/stdout |
| **Embedded** | Optional | Yes (Central Dir) | None (Stored) | Basic (mtime) | CRC32 | Microcontrollers, firmware, IoT, ROM FS |
| **Minimal** | REQUIRED | Yes (Central Dir) | None (Stored) | Standard (4×timestamps) | CRC32 | Simple archiving, minimal dependencies |
| **Standard** | RECOMMENDED | Yes + Quick Dir | ZSTD/LZMA2 | Extended | SHA-256 | General-purpose, backup, distribution |
| **Enhanced** | Optional | Yes + Seekable | Multiple algorithms | Encryption + core | SHA-512/BLAKE3 | Professional backup, compliance |
| **Full** | OPTIONAL | All modes | All algorithms | All metadata + FEC | All algorithms | Enterprise, preservation, migration |

### Table 2: Core Features by Conformance Level

#### Tiny Conformance (Absolute Minimum for Parsing)

| Category | Feature | Section | Description |
|----------|---------|---------|-------------|
| **Archive Structure** | Archive Header | 3 | Magic (0x5A4F4F3634415243) and version only |
| | File Entry | 5 | Path, size, offset to data |
| **Compression** | Stored Mode | 4 | Uncompressed only (Algorithm ID 0x0000) |
| **Path Support** | UTF-8 Paths (Basic) | 5.3 | UTF-8 support (no validation) |
| **Metadata** | File Size | 5.1 | Uncompressed size only |
| **OMITTED** | Central Directory | - | Not required (sequential scan) |
| | Timestamps | - | Optional (modification time if present) |
| | Checksums | - | Optional (CRC32 if present) |
| | End of Archive Marker | - | Optional |

**Implementation Guidance**: Tiny conformance is intended for:
- Educational implementations (learning the format)
- Minimal test parsers
- Proof-of-concept code
- Embedded systems with extreme constraints (< 1KB code)

**Note**: Tiny implementations should be able to *read* archives but not necessarily *create* them. Writing should target at least Compact level.

#### Compact Conformance (Streaming Only)

| Category | Feature | Section | Description |
|----------|---------|---------|-------------|
| **Archive Structure** | Archive Header | 3 | Minimal flags, streamable mode bit set |
| | File Entry | 5 | Path and minimal metadata (size, CRC32) |
| | End of Archive Marker | 11 | Archive terminator |
| **Compression** | Stored Mode | 4 | Uncompressed only (Algorithm ID 0x0000) |
| **Path Support** | UTF-8 Paths (Basic) | 5.3 | UTF-8 support, ASCII-safe subset recommended |
| **Metadata** | Minimal Integrity | 5.1 | CRC32 checksums only |
| | File Size | 5.1 | Uncompressed and compressed sizes |
| **Access Mode** | Sequential Only | 3.2a.1 | No seeking, stream-only access |
| **OMITTED** | Central Directory | - | Not present (sequential scan required) |

#### Embedded Conformance (Resource-Constrained)

| Category | Feature | Section | Description |
|----------|---------|---------|-------------|
| **Archive Structure** | Archive Header | 3 | Basic flags, version |
| | File Entry | 5 | Path and basic metadata (size, CRC32, mtime) |
| | Central Directory | 10 | Simplified entries for random access |
| | End of Archive Marker | 11 | Archive terminator |
| **Compression** | Stored Mode | 4 | Uncompressed only (Algorithm ID 0x0000) |
| **Path Support** | UTF-8 Paths | 5.3 | UTF-8 support (normalization optional) |
| **Metadata** | Basic Integrity | 5.1 | CRC32 checksums |
| | Modification Time | 5.1 | Single timestamp (modification time only) |
| | File Size | 5.1 | Uncompressed and compressed sizes |
| **Access Mode** | Random Access | 10 | Via simplified Central Directory |

#### Minimal Conformance (REQUIRED - Baseline)

| Category | Feature | Section | Description |
|----------|---------|---------|-------------|
| **Archive Structure** | Archive Header | 3 | Magic signature (0x5A4F4F3634415243), version, flags, timestamps |
| | File Entry | 5 | Local file header with path and metadata |
| | Central Directory | 10 | Fast access directory with all file entries |
| | End of Archive Marker | 11 | Archive terminator with integrity checks |
| **Compression** | Stored Mode | 4 | Uncompressed file storage (Algorithm ID 0x0000) |
| | Bit Squishing | 12.7 | Reduced alphabet compression (Algorithm ID 0x0010) |
| **Path Support** | UTF-8 Paths | 5.3 | Variable-length Unicode paths with full normalization |
| | NFC Normalization | 5.3 | Unicode NFC normalization required |
| | Path Separator Normalization | 5.3 | All separators to \0 |
| | Dot Resolution | 5.3 | Handle ./ and ../ components |
| **Metadata** | Full Timestamps | 5.1 | Birth, modification, access, change times (NTP format) |
| | Basic Integrity | 5.1 | CRC32 checksums for all files |
| | Basic Attributes | 5.1 | Unix mode, UID/GID, file size |

#### Standard Conformance (RECOMMENDED)

Includes all Minimal features, plus:

| Category | Feature | Section | Description |
|----------|---------|---------|-------------|
| **Compression** | LZMA2 or ZSTD | 4 | Modern compression (recommended: ZSTD for speed, LZMA2 for ratio) |
| **Directory** | Quick Directory | 4.6 | Fast file listing without full archive scan |
| **Deduplication** | Block Deduplication | 6.2 (0x0047) | Chunk-based storage deduplication |
| **Integrity** | SHA-256 Hashes | 5.1, 6b | Cryptographic file integrity verification |
| **Metadata** | Unix Permissions | 5.1 | Full Unix mode bits |
| | Windows Attributes | 6.2 (0x000E) | Basic Windows file attributes |

#### Enhanced Conformance (Professional)

Includes all Standard features, plus:

| Category | Feature | Section | Description |
|----------|---------|---------|-------------|
| **Compression** | Multiple Algorithms | 4 | ZSTD, LZMA2, LZ4, ZLIB, Brotli (5+ algorithms) |
| | Seekable Compression | 7 | Random access within compressed files |
| **Encryption** | AES-256-GCM | 6a | Authenticated encryption (recommended) |
| | ChaCha20-Poly1305 | 6a | Alternative authenticated encryption |
| | Argon2id KDF | 6a | Memory-hard key derivation |
| **Signatures** | RSA-2048 | 9 | Digital signatures (RSA) |
| | Ed25519 | 9 | Digital signatures (Edwards curve) |
| **Integrity** | SHA-512 / BLAKE3 | 6b | Advanced cryptographic hashing |
| **Metadata** | ACLs | 6.2 (0x0001) | POSIX, Windows NT ACLs |
| | Extended Attributes | 6.2 (0x0002) | Unix/Linux xattrs |
| | Alternate Data Streams | 6.2 (0x0003) | NTFS ADS support |
| | Security Descriptors | 6.2 (0x0004) | Windows security descriptors |
| **Modes** | Streamable Mode | 3.2a.1 | Sequential-only streaming support |

#### Full Conformance (OPTIONAL - Complete)

Includes all Enhanced features, plus:

| Category | Feature | Section | Description |
|----------|---------|---------|-------------|
| **Compression** | All Algorithms | 4.1 | Full support for 20+ compression algorithms |
| | Solid Compression | 8 | Multi-file compression streams |
| | Legacy Algorithms | 4.1 | PAQ, Crunch, ACE, ARJ, StuffIt, LTO, etc. |
| **Encryption** | All Encryption Methods | 6a | All 6 encryption algorithms |
| | All KDFs | 6a | All 5 key derivation functions |
| | Header Encryption | 6a | Encrypted archive headers |
| **Integrity** | Reed-Solomon FEC | 6b | Forward error correction for data recovery |
| | PAR2 Integration | 6b | Parity archive integration |
| | All Hash Algorithms | 6b | SHA1/256/384/512, SHA3, BLAKE2b, BLAKE3 |
| **Signatures** | All Signature Algorithms | 9 | RSA-2048/4096, ECDSA-P256/P384, Ed25519, Ed448 |
| | X.509 Certificates | 9 | Certificate chain support |
| **Metadata** | All 95+ Metadata Types | 6.2 | Comprehensive platform and filesystem metadata |
| | Filesystem Metadata | 6.2 | APFS, ReFS, ZFS, XFS, JFS, Btrfs, etc. (18 types) |
| | Legacy OS Metadata | 6.2 | VMS, z/OS, OS/400, Classic Mac, Amiga, etc. (12 types) |
| | VCS Metadata | 6.2 (0x0032-0x0038) | Git, SVN, Perforce, Mercurial, etc. (7 types) |
| | Network FS | 6.2 | NFS, SMB/CIFS, WebDAV, AFS, CODA, etc. (8 types) |
| **Archives** | Multi-Volume Support | 3.3-3.4 | Archive splitting across multiple volumes |
| **Modes** | Tape-Friendly Mode | 3.2a.2 | Optimized for tape storage |
| | HSM Mode | 3.2a.3 | Hierarchical storage management |
| **Links** | Hard Links | 6.4, 6.2 (0x000F) | Including directory hard links |
| | Symbolic Links | 6.5, 6.2 (0x0010) | With all variants and flags |
| | Magic Symlinks | 6.2 (0x0027) | Windows junctions, Mac aliases |

### Table 3: Compression Algorithms Matrix

| Algorithm | ID | Conformance Level | Best Use Case | Speed | Ratio |
|-----------|-----|-------------------|---------------|-------|-------|
| **Stored (None)** | 0x0000 | **Compact+** (ALL) | Pre-compressed files, streams | Fastest | None |
| **ZSTD** | 0x0004 | **Standard** (recommended) | General purpose, balanced | Fast | High |
| **LZMA2** | 0x0006 | **Standard** (alternative) | Maximum compression, multi-thread | Medium | Very High |
| **LZ4** | 0x0003 | Enhanced | Real-time compression | Fastest | Low-Medium |
| **ZLIB** | 0x0009 | Enhanced | Compatibility, networking | Medium | Medium-High |
| **Brotli** | 0x000C | Enhanced | Web content, text | Medium | Very High |
| **LZ77** | 0x0002 | Enhanced | General purpose | Fast | Medium |
| **LZMA** | 0x0005 | Full | Maximum compression | Slow | Very High |
| **LZX** | 0x0007 | Full | CAB/WIM compatibility | Medium | High |
| **LZFSE** | 0x0008 | Full | Apple platforms | Fast | High |
| **LZH** | 0x000A | Full | Legacy compatibility | Medium | Medium-High |
| **LZW** | 0x000B | Full | Legacy (GIF/TIFF) | Fast | Medium |
| **Bzip2** | 0x000D | Full | High compression | Slow | Very High |
| **PAQ** | 0x000E | Full | Maximum compression | Very Slow | Maximum |
| **Huffman** | 0x000F | Full | Simple compression | Fast | Low-Medium |
| **ZOZ** | 0x0001 | Full | Maximum compression, adaptive | Slow | Maximum |
| **Bit Squishing** | 0x0010 | **Minimal** | Reduced alphabet (text, logs) | Fast | High |

### Table 4: Encryption & Security Features Matrix

| Feature | Conformance Level | Algorithm/Type | Key Size | Use Case |
|---------|-------------------|----------------|----------|----------|
| **Hash Algorithms** | | | | |
| CRC32 | **Compact+** (ALL) | Checksum | 32-bit | Error detection |
| SHA-256 | **Standard** | Cryptographic | 256-bit | Standard integrity |
| SHA-512 | Enhanced | Cryptographic | 512-bit | Maximum security |
| BLAKE3 | Enhanced | Cryptographic | 256-bit | Maximum performance |
| SHA-384 | Full | Cryptographic | 384-bit | Enhanced security |
| SHA3-256 | Full | Cryptographic | 256-bit | Alternative standard |
| SHA3-512 | Full | Cryptographic | 512-bit | Alternative standard |
| BLAKE2b | Full | Cryptographic | 512-bit | High performance |
| SHA1 | Full | Cryptographic (deprecated) | 160-bit | Legacy compatibility |
| **Encryption Methods** | | | | |
| AES-256-GCM | **Enhanced** (recommended) | Authenticated | 256-bit | General purpose, best balance |
| ChaCha20-Poly1305 | Enhanced | Authenticated | 256-bit | Software-only systems |
| AES-256-CBC + HMAC | Full | Encrypt-then-MAC | 256-bit | Legacy compatibility |
| AES-128-GCM | Full | Authenticated | 128-bit | Performance-critical |
| Twofish-256-GCM | Full | Authenticated | 256-bit | Alternative cipher |
| Serpent-256-GCM | Full | Authenticated | 256-bit | High security |
| **Key Derivation** | | | | |
| Argon2id | **Enhanced** (recommended) | Memory-hard | Variable | Modern, resistance to GPU/ASIC |
| PBKDF2-HMAC-SHA256 | Enhanced | Standard | Variable | Compatibility |
| PBKDF2-HMAC-SHA512 | Full | Standard | Variable | Enhanced security |
| scrypt | Full | Memory-hard | Variable | Good balance |
| bcrypt | Full | CPU-hard | Variable | Password hashing |
| **Digital Signatures** | | | | |
| RSA-2048 | **Enhanced** | Public-key | 2048-bit | Wide compatibility |
| Ed25519 | **Enhanced** | Edwards curve | 256-bit | Modern, fast verification |
| RSA-4096 | Full | Public-key | 4096-bit | High security |
| ECDSA-P256 | Full | Elliptic curve | 256-bit | Smaller signatures |
| ECDSA-P384 | Full | Elliptic curve | 384-bit | Enhanced security |
| Ed448 | Full | Edwards curve | 448-bit | Maximum security |
| **Error Correction** | | | | |
| Reed-Solomon | Full | FEC | Variable | General purpose |
| LDPC | Full | FEC | Variable | Advanced correction |
| PAR2 | Full | Parity archive | Variable | Archive recovery |

### Table 5: Metadata Types Summary

| Category | Count | Conformance | Examples |
|----------|-------|-------------|----------|
| **Core Metadata** | 15 | Optional | ACLs, xattrs, ADS, Security Descriptors, Resource Forks |
| **Unix/POSIX** | 8 | Optional | Extended timestamps, SELinux, File capabilities, BSD flags, Linux flags |
| **Windows** | 5 | Optional | Security Descriptors, ADS, Extended attributes, Short filenames |
| **Legacy OS** | 12 | Optional | OpenVMS, z/OS, OS/400, Classic Mac OS, Amiga, Atari, Acorn, C64, Apple IIGS |
| **Filesystems** | 18 | Optional | APFS, ReFS, ZFS, XFS, JFS, Btrfs, HPFS, VxFS, AdvFS, ReiserFS, UDF, ISO 9660 |
| **Version Control** | 7 | Optional | Git, SVN, Perforce, CVS, RCS, Mercurial, Fossil |
| **Network FS** | 8 | Optional | NFS, SMB/CIFS, NetWare, AFP, AFS, CODA, DFS, WebDAV |
| **Special Files** | 6 | Optional | Device files, FIFOs, Sockets, Doors, Magic symlinks, Whiteouts |
| **Archive Formats** | 3 | Optional | WIM metadata, WIM resources, WIM boot/integrity |
| **File Features** | 6 | Optional | Sparse files, Delta revisions, Deduplication, Reserved names, DR-DOS passwords |
| **TOTAL** | **95+** | Optional | Comprehensive cross-platform metadata preservation |

### Table 6: Complete Feature Matrix by Category

#### Archive Structures & Formats

| Feature | Conformance | Magic/ID | Section | Notes |
|---------|-------------|----------|---------|-------|
| Archive Header | **REQUIRED** | 0x5A4F4F3634415243 | 3 | Must be present in all archives |
| Compression Descriptor | **REQUIRED** | 0x434F4D5044455343 | 4 | Defines compression pipeline |
| Archive YAML Metadata | Optional | 0x59414D4C4D455441 | 4.5 | Archive-level metadata |
| Quick Directory | Optional (**Recommended**) | 0x5155494344495220 | 4.6 | Fast file listing |
| File Entry | **REQUIRED** | 0x46494C45454E5452 | 5 | File metadata and data |
| Extended Metadata | Optional | 0x4D45544144415441 | 6 | Extended metadata chunks |
| Encryption Header | Optional | 0x454E4352595054 | 6a | Encryption parameters |
| Integrity Block | Optional | 0x494E544547524954 | 6b | Hash and FEC data |
| Seekable Block Table | Optional | 0x424C4B54424C | 7 | Random access offsets |
| Solid Block | Optional | 0x534F4C4944424C4B | 8 | Solid compression container |
| File Signature | Optional | 0x5349474E41545552 | 9 | Per-file digital signature |
| Central Directory | **REQUIRED** | 0x43454E5440495220 | 10 | Central file index |
| End of Archive | **REQUIRED** | 0x454E444F46415243 | 11 | Archive terminator |
| Volume Header | Optional | 0x564F4C554D4548 | 3.3 | Multi-volume marker |
| Volume Footer | Optional | 0x564F4C464F4F5452 | 3.4 | Volume boundary marker |

#### Operating Modes

| Mode | Conformance | Flag Bit | Section | Description |
|------|-------------|----------|---------|-------------|
| Normal Mode | **REQUIRED** | - | 2-11 | Standard random-access archive |
| Streamable Mode | Optional | Bit 16 | 3.2a.1 | Sequential-only, no seeking |
| Tape-Friendly Mode | Optional | Bit 17 | 3.2a.2 | Block-aligned for tape storage |
| HSM Mode | Optional | Bit 18 | 3.2a.3 | Hierarchical storage management |

#### Path & Filename Handling

| Feature | Conformance | Section | Description |
|---------|-------------|---------|-------------|
| UTF-8 Paths | **REQUIRED** | 5.3 | Unicode path support |
| NFC Normalization | **REQUIRED** | 5.3 | Unicode normalization |
| Path Separator Normalization | **REQUIRED** | 5.3 | Convert all to \0 |
| Dot Component Resolution | **REQUIRED** | 5.3 | Handle ./ and ../ |
| Reserved Name Handling | Optional | 6.2 (0x0048) | Windows/DOS device names |
| Short Filename (8.3) | Optional | 6.2 (0x003F) | DOS compatibility |
| HFS Filename Encoding | Optional | 6.2 (0x0040) | Classic Mac support |

#### Link Support

| Feature | Conformance | Section | Description |
|---------|-------------|---------|-------------|
| Hard Links | Optional | 6.4, 6.2 (0x000F) | Inode-based hard links |
| Symbolic Links | Optional | 6.5, 6.2 (0x0010) | Symlink targets and flags |
| Magic Symlinks | Optional | 6.2 (0x0027) | Windows junctions, Mac aliases |
| Directory Hard Links | Optional | 6.4 | HFS+/APFS directory links |

### Table 7: Implementation Recommendations by Conformance Level

| Conformance Level | Scenario | Required Features | Recommended Add-Ons | Target Environment |
|-------------------|----------|-------------------|---------------------|-------------------|
| **Compact** | Network Streaming | Archive Header, File Entry, End Marker, Stored mode, CRC32 | LZ4 for minimal compression | Network protocols, stdin/stdout, tar-like streaming |
| | Pipe Processing | Sequential access only, no Central Directory | None | Shell pipes, data transformation |
| | Single-Pass Tools | Basic UTF-8 support (ASCII-safe) | None | Stream processing, network delivery |
| **Embedded** | Firmware Updates | + Central Directory (simplified), modification time | LZ4 for space savings | Microcontrollers, bootloaders |
| | IoT Devices | Random access support | None | Resource-constrained devices |
| | ROM Filesystems | Minimal memory footprint | Read-only optimization | Embedded Linux, RTOS |
| **Minimal** | Simple Archiver | + Full timestamps, Full UTF-8 normalization, UID/GID | Quick Directory | Desktop utilities, file bundling |
| | Basic Backup | All required structures | SHA-256 for integrity | Personal backup tools |
| | File Distribution | Cross-platform paths | Compression (ZSTD) | Software distribution (uncompressed) |
| **Standard** | General Purpose | + ZSTD or LZMA2, Quick Directory, SHA-256, Deduplication | LZ4 for fast mode | Desktop archiving tools (7-Zip class) |
| | Backup Software | Block deduplication, Quick Directory | Multi-volume, FEC | Personal/small business backup |
| | Software Distribution | ZSTD compression, SHA-256 verification | Digital signatures | Package managers, installers |
| | Development Tools | Quick Directory, deduplication | Git metadata | Build systems, artifact storage |
| **Enhanced** | Professional Backup | + Encryption (AES-256-GCM), Signatures, ACLs/xattrs/ADS | Multi-volume, HSM mode | Enterprise backup software |
| | Compliance Archive | Encryption, digital signatures, SHA-512 | All metadata types | Legal/regulatory archiving |
| | Secure Distribution | AES-256-GCM, Ed25519 signatures, BLAKE3 | Certificate chains | Secure software delivery |
| | Cloud Backup | Multiple compression (ZSTD/LZMA2/LZ4), encryption | Deduplication, seekable | Cloud storage optimization |
| **Full** | Enterprise Backup | + All compression, all encryption, FEC, multi-volume | Tape mode, HSM mode | Large-scale enterprise systems |
| | Digital Preservation | All metadata types (95+), all filesystems | VCS metadata, legacy OS support | Archives, museums, libraries |
| | Cross-Platform Migration | All filesystem metadata, all OS support | All link types, special files | Data center migrations |
| | Maximum Compression | LZMA2, PAQ, solid compression, BWT pipeline | All legacy algorithms | Long-term archival storage |
| | Legacy System Archive | All legacy compression (Squoze, RAD50, SIXBIT) | All legacy metadata | Retrocomputing, data recovery |

### Table 8: Quick Reference - Choosing Your Conformance Level

| Question | Answer | Recommended Level |
|----------|--------|-------------------|
| Need random access to files? | No, sequential only | **Compact** |
| Severely resource-constrained (< 64KB RAM)? | Yes | **Embedded** |
| Just need basic archiving? | Yes | **Minimal** |
| Need compression for general use? | Yes | **Standard** |
| Need encryption or digital signatures? | Yes | **Enhanced** or **Full** |
| Need cross-platform metadata preservation? | Yes | **Enhanced** (basic) or **Full** (complete) |
| Archiving legacy systems (VMS, z/OS, Amiga)? | Yes | **Full** |
| Need FEC or PAR2 for data recovery? | Yes | **Full** |
| Enterprise compliance requirements? | Yes | **Enhanced** (most) or **Full** (complete) |
| Maximum possible compression? | Yes | **Full** (for PAQ, solid mode) |

---

## 1. Introduction

Zoo64 is a modern archive format designed for maximum compression, data integrity, and comprehensive metadata preservation. It supports multiple compression strategies, digital signatures, and full filesystem metadata including ACLs, extended attributes, and alternate data streams.

### 1.1 Design Goals

- **Maximum Compression**: Multi-stage pipeline with seekable solid compression
- **Modern Metadata**: Preserve all filesystem metadata (ACLs, xattrs, ADS, timestamps)
- **Flexibility**: Support per-file and solid compression with seekable variants
- **Integrity**: File-level and archive-level digital signatures
- **Unicode**: Full UTF-8 path support with no length restrictions
- **Cross-Platform**: Defined binary format for portability
- **Component Model**: Designed as COM component for language interoperability

### 1.2 Key Features

- Variable-length UTF-8 paths (no 16-character limit)
- ACL (Access Control List) storage
- Extended attributes (xattrs)
- Alternate Data Streams (ADS)
- YAML metadata at archive and file level
- Solid compression with seekable blocks
- Per-file compression with seekable blocks
- File-level and archive-level digital signatures
- Multiple compression algorithms
- Block-based seeking via LEB128 offset tables

## 2. Archive Structure [REQUIRED]

### 2.1 Overall Layout

Zoo64 uses a redundant directory structure similar to ZIP format. Each file has both a local header (embedded with the file data) and a central directory entry (at the end of the archive). This provides fast scanning and recovery from corruption.

```
[Archive Header]
[Compression Mode Descriptor]
[Archive YAML Metadata] (optional)
[Quick Directory] (optional)   ← NEW: Fast file listing without full scan
  [Quick Directory Header]
  [Quick File Entries...]       ← Minimal metadata: name, size, offset only
[File Entries...]
  [Local File Header]           ← Redundant: full file metadata
  [File Metadata]
    [Binary Metadata Chunks] (ACL, xattr, ADS, etc.)
    [YAML Metadata] (optional)
  [Encryption Header] (optional)
  [File Data / Compressed Data]
  [File Signature] (optional)
[Central Directory]             ← Redundant: complete file metadata
  [Central Directory Header]
  [Central Directory Entries...]
[Archive Signature] (optional)
[End of Archive Marker]
```

### 2.1a Multi-Volume Support

Zoo64 supports splitting archives across multiple volumes (files) for storage on limited media or for easier distribution.

```
Volume 1:
  [Archive Header (Volume 1)]
  [Compression Mode Descriptor]
  [Archive YAML Metadata] (optional)
  [File Entries... (partial)]
  [Volume Footer]

Volume 2-N:
  [Volume Header]
  [File Entries... (continued)]
  [Volume Footer]

Last Volume:
  [Volume Header]
  [File Entries... (final)]
  [Central Directory]
  [Archive Signature] (optional)
  [End of Archive Marker]
```

### 2.2 Magic Numbers

```
Zoo64 Archive:        0x5A4F4F3634415243  ("ZOO64ARC")
Classic Zoo:          0xFDC4A7DC          (Classic Zoo 2.1 format)
Solid Block:          0x534F4C4944424C4B  ("SOLIDBLK")
File Entry:           0x46494C45454E5452  ("FILEENTR")
Local File Header:    0x4C4F43414C4844    ("LOCALHD ")
Metadata Block:       0x4D45544144415441  ("METADATA")
YAML Metadata:        0x59414D4C4D455441  ("YAMLMETA")
Encryption Header:    0x454E4352595054    ("ENCRYPT ")
Signature Block:      0x5349474E41545552  ("SIGNATUR")
Central Directory:    0x43454E5444495220  ("CENTDIR ")
Quick Directory:      0x5155494344495220  ("QUIC DIR")
Volume Header:        0x564F4C554D4548    ("VOLUMEH ")
Volume Footer:        0x564F4C554D4546    ("VOLUMEF ")
End of Archive:       0x454E444F46415243  ("ENDOFARC")
```

## 3. Archive Header [REQUIRED]

The archive header appears at the beginning of every Zoo64 archive (or first volume in multi-volume archives).

```c
typedef struct _ZOO64_ARCHIVE_HEADER {
  UINT64  Magic;              /// 0x5A4F4F3634415243 ("ZOO64ARC")
  UINT16  MajorVersion;       /// Format major version (1)
  UINT16  MinorVersion;       /// Format minor version (0)
  UINT32  Flags;              /// Archive flags
  UINT64  CreationTime;       /// NTP extended format timestamp
  UINT64  ModificationTime;   /// NTP extended format timestamp
  UINT32  CompressionMode;    /// Compression mode identifier
  UINT32  FileCount;          /// Number of files in archive (all volumes)
  UINT64  CentralDirOffset;   /// Offset to central directory (in last volume)
  UINT64  ArchiveSize;        /// Total archive size in bytes (all volumes)
  UINT32  BlockSize;          /// Block size for seekable compression (power of 2)
  UINT64  YamlMetadataOffset; // Offset to archive YAML metadata (0 if none)
  UINT32  YamlMetadataSize;   /// Size of archive YAML metadata
  UINT16  VolumeNumber;       /// Volume number (0 for single archive, 1+ for multi-volume)
  UINT16  TotalVolumes;       /// Total number of volumes (0 for single archive)
  UINT32  VolumeSize;         /// Maximum size per volume (0 for single archive)
  UINT8   UUID[16];           /// Archive UUID (same across all volumes)
} ZOO64_ARCHIVE_HEADER;
#pragma pack(pop)
```

### 3.1a NTP Extended Format Timestamps

All timestamps in Zoo64 use **NTP extended format** (64-bit):

```
Bits 0-31:  Seconds since NTP epoch (1900-01-01 00:00:00 UTC)
Bits 32-63: Fraction of a second (1/2^32 second precision)
```

This provides:
- **Resolution**: ~232 picoseconds (2^-32 seconds)
- **Range**: 136 years per NTP era (wraps at 2^32 seconds)
- **Current Era**: Era 0 ends 2036-02-07, Era 1 begins 2036-02-08

**Conversion from Unix time to NTP extended format**:
```c
// Unix epoch (1970-01-01) is 2208988800 seconds after NTP epoch (1900-01-01)
#define NTP_OFFSET 2208988800ULL

UINT64 UnixToNTP(time_t unixSec, uint32_t nanoSec) {
  UINT64 ntpSec = (UINT64)unixSec + NTP_OFFSET;
  UINT64 ntpFrac = ((UINT64)nanoSec << 32) / 1000000000ULL;
  return (ntpSec << 32) | ntpFrac;
}
```

**Conversion from NTP extended format to Unix time**:
```c
void NTPToUnix(UINT64 ntpTime, time_t *unixSec, uint32_t *nanoSec) {
  UINT64 ntpSec = ntpTime >> 32;
  UINT64 ntpFrac = ntpTime & 0xFFFFFFFF;
  *unixSec = (time_t)(ntpSec - NTP_OFFSET);
  *nanoSec = (uint32_t)((ntpFrac * 1000000000ULL) >> 32);
}
```

### 3.1 Archive Flags

```
Bit 0:     Solid compression enabled
Bit 1:     Seekable blocks enabled
Bit 2:     Archive signature present
Bit 3:     Encrypted (archive-level encryption)
Bit 4:     UTF-8 strict mode
Bit 5:     Archive YAML metadata present
Bit 6:     Multi-volume archive
Bit 7:     Classic Zoo compatibility mode
Bit 8:     Extended metadata present
Bit 9:     ACLs preserved
Bit 10:    Extended attributes preserved
Bit 11:    Alternate data streams preserved
Bit 12:    YAML metadata in files
Bit 13:    Per-file encryption allowed
Bit 14:    Redundant directory (ZIP-like)
Bit 15:    Reserved
Bit 16:    Streamable mode (sequential access only)
Bit 17:    Tape-friendly mode (optimized for tape)
Bit 18:    HSM mode (Hierarchical Storage Management)
Bit 19-31: Reserved for future use
```

### 3.2 Compression Modes

```
0x0000: Uncompressed (stored)
0x0001: Per-file compression
0x0002: Per-file seekable compression
0x0100: Solid compression
0x0101: Solid seekable compression
0x0200: Adaptive (per-file for small, solid for large)
```

### 3.2a Archive Operating Modes

Zoo64 supports specialized operating modes optimized for different storage scenarios.

#### 3.2a.1 Streamable Mode (Bit 16)

Optimized for streaming operations without random access:

**Characteristics**:
- No central directory at end
- File entries written sequentially as encountered
- Each file entry contains complete metadata inline
- No seek operations required during extraction
- Forward-only reading
- Suitable for pipes, network streams, and stdin/stdout

**Structure**:
```
[Archive Header]
[File Entry 1 + Metadata + Data]
[File Entry 2 + Metadata + Data]
...
[File Entry N + Metadata + Data]
[End Marker]
```

**Constraints**:
- Cannot update existing archives
- Cannot seek to random files
- File count unknown until end reached
- Duplicate filenames possible (last wins)

**Use Cases**:
- Pipe operations: `tar | zoo64-stream | ssh remote`
- Network streaming
- Backup to stdout
- Docker image layers

#### 3.2a.2 Tape-Friendly Mode (Bit 17)

Optimized for sequential tape storage with proper block alignment:

**Characteristics**:
- Fixed block size (default 10KB, configurable to 32KB/64KB)
- Block-aligned file data
- Tape marks between logical sections
- Forward error correction (optional)
- Multiple copies of directory (beginning + end)
- Volume labels compatible with ANSI X3.27

**Block Structure**:
```c
typedef struct _ZOO64_TAPE_BLOCK {
  UINT64  Magic;              /// 0x5A4F4F54415045 ("ZOOTAPE ")
  UINT32  BlockNumber;        /// Sequential block number
  UINT32  BlockSize;          /// Block size (10KB, 32KB, 64KB)
  UINT32  Flags;              /// Block flags
  UINT32  CRC32;              /// Block CRC
  UINT8   Data[BlockSize-32]; // Block data (padded)
  UINT32  NextBlockNumber;    /// Next block number (for verification)
  UINT32  Reserved;           /// Reserved
} ZOO64_TAPE_BLOCK;
#pragma pack(pop)
```

**Block flags**:
```
0x00000001: START_OF_FILE     /// First block of file
0x00000002: END_OF_FILE       /// Last block of file
0x00000004: TAPE_MARK          /// Tape mark (filemark)
0x00000008: VOLUME_LABEL      /// Volume label block
0x00000010: DIRECTORY_BLOCK   /// Directory block
0x00000020: ECC_PRESENT       /// Forward error correction present
0x00000040: COMPRESSED_BLOCK  /// Block compressed
0x00000080: END_OF_ARCHIVE    /// End of archive marker
```

**Tape marks**:
- After archive header
- Between files (optional, controlled by flag)
- After directory
- At end of archive

**Error Recovery**:
- Block CRC32 for error detection
- Optional Reed-Solomon forward error correction
- Redundant directory (at start and end)
- Block numbers for sequence verification

**Use Cases**:
- LTO tape backup
- IBM 3592 enterprise tape
- DAT/DLT tape archival
- Compliance archival (WORM tapes)
- Long-term offline storage

#### 3.2a.3 HSM Mode (Hierarchical Storage Management) (Bit 18)

Optimized for tiered storage with stub files and migration metadata:

**Characteristics**:
- Supports stub files (metadata only, data offline)
- Migration state tracking
- Recall hints and policies
- Storage tier classification
- Access frequency tracking
- Automated tier transition metadata

**HSM Metadata**:
```c
typedef struct _ZOO64_HSM_METADATA {
  UINT32  Magic;              /// 0x48534D00 ("HSM\0")
  UINT8   MigrationState;     /// Migration state
  UINT8   StorageTier;        /// Current storage tier
  UINT8   TargetTier;         /// Target tier for migration
  UINT8   RecallPriority;     /// Recall priority (0-255)
  UINT64  MigrationTime;      /// Time of last migration
  UINT64  LastAccessTime;     /// Last access time
  UINT64  AccessCount;        /// Number of accesses
  UINT64  OfflineSize;        /// Size if migrated
  UINT32  RecallCost;         /// Estimated recall cost (ms)
  UINT32  PolicyID;           /// HSM policy ID
  UINT32  VolumeIDLength;     /// Length of offline volume ID
  UINT32  LocationLength;     /// Length of offline location
  /// Followed by:
  ///   [VolumeIDLength bytes: offline volume identifier]
  ///   [LocationLength bytes: offline storage location]
} ZOO64_HSM_METADATA;
#pragma pack(pop)
```

**Migration states**:
```
0: RESIDENT             /// Fully online (all data present)
1: PREMIGRATED          /// Dual-resident (online + offline copy)
2: MIGRATED             /// Stub only (data offline)
3: MIGRATING            /// Migration in progress
4: RECALLING            /// Recall in progress
5: RECALL_PENDING       /// Queued for recall
6: ARCHIVED             /// Archived to tape/cloud
```

**Storage tiers**:
```
0: TIER_NVME            /// NVMe SSD (fastest)
1: TIER_SSD             /// SATA SSD (fast)
2: TIER_SAS             /// SAS HDD (medium)
3: TIER_SATA            /// SATA HDD (slow)
4: TIER_NEARLINE        /// Nearline disk
5: TIER_TAPE            /// Tape library
6: TIER_CLOUD           /// Cloud storage
7: TIER_ARCHIVE         /// Deep archive
```

**HSM operations**:
- **Stub file**: Metadata preserved, data pointer to offline location
- **Recall**: Bring data back online from offline tier
- **Migration**: Move data to slower/cheaper tier
- **Pre-migration**: Dual-resident during transition

**Use Cases**:
- Enterprise HSM systems (IBM Spectrum Archive, HPE DMF)
- Cloud tiering (AWS S3 Glacier, Azure Archive)
- Tape archival with online catalog
- Research data management
- Media asset management (MAM)
- Compliance archival with fast recall

### 3.2b Path Normalization and Separator Handling

**CRITICAL: All paths are internally normalized** before storage in Zoo64 archives.

**Normalization Process**:
1. Unicode normalization to NFC (Canonical Composition)
2. All path separators (`/`, `\`, `:`, etc.) converted to `\0` (null byte)
3. Redundant separators collapsed (`//` → `/`)
4. Dot components resolved (`.` removed, `..` resolved)
5. Trailing separators removed (except root)
6. Case preserved (no case folding)
7. Whitespace preserved (no trimming)

**Internal Storage Format**:
```
[component1]\0[component2]\0...[componentN]\0\0
```

**Normalization Examples**:
```
Input:          /home//user/./docs/../file.txt
Normalized:     \0home\0user\0file.txt\0\0

Input:          C:\Users\John\..\Public\file.txt
Normalized:     C:\0Users\0Public\0file.txt\0\0

Input:          docs/./readme.md
Normalized:     docs\0readme.md\0\0

Input:          \\server\share\dir\file
Normalized:     \0\0server\0share\0dir\0file\0\0
```

**Path Types**:
- **Absolute POSIX**: First component empty → `/home/user` = `\0home\0user\0\0`
- **Absolute Windows**: Drive letter → `C:\Users` = `C:\0Users\0\0`
- **UNC Path**: Double empty start → `\\server\share` = `\0\0server\0share\0\0`
- **Relative Path**: First component non-empty → `docs/file` = `docs\0file\0\0`

**Benefits of Normalization**:
- Consistent representation across all platforms
- Duplicate path detection (binary comparison)
- Security (path traversal prevention via `..` resolution)
- Better compression (eliminates separator variance)
- Platform-agnostic archive format

**See Section 5.3** for complete normalization rules and examples.

## 3.3 Multi-Volume Support [OPTIONAL] - Volume Header

Appears at the start of volumes 2-N in multi-volume archives.

```c
typedef struct _ZOO64_VOLUME_HEADER {
  UINT64  Magic;              /// 0x564F4C554D4548 ("VOLUMEH ")
  UINT16  VolumeNumber;       /// Volume number (2, 3, 4...)
  UINT16  TotalVolumes;       /// Total number of volumes
  UINT64  VolumeSize;         /// Size of this volume
  UINT64  VolumeOffset;       /// Offset in complete archive
  UINT32  CRC32;              /// CRC32 of this volume
  UINT8   ArchiveUUID[16];    /// Archive UUID (matches main header)
} ZOO64_VOLUME_HEADER;
#pragma pack(pop)
```

## 3.4 Volume Footer

Appears at the end of volumes 1-(N-1) in multi-volume archives.

```c
typedef struct _ZOO64_VOLUME_FOOTER {
  UINT64  Magic;              /// 0x564F4C554D4546 ("VOLUMEF ")
  UINT16  VolumeNumber;       /// This volume number
  UINT16  NextVolumeNumber;   /// Next volume number
  UINT64  BytesInVolume;      /// Total bytes in this volume
  UINT32  CRC32;              /// CRC32 of this volume
} ZOO64_VOLUME_FOOTER;
#pragma pack(pop)
```

## 3.5 Overlay Archives [OPTIONAL]

Overlay archives reference a base archive and store only differences (deltas), making them ideal for incremental backups, tape archives, and HSM systems.

```c
//
// Overlay archive header (extends standard archive header)
// When archive flags bit 19 is set (Overlay Mode)
//
typedef struct _ZOO64_OVERLAY_HEADER {
  UINT64  Magic;              // 0x4F5645524C415920 ("OVERLAY ")
  UINT32  OverlayVersion;     // Overlay format version
  UINT32  Flags;              // Overlay flags

  // Base archive identification
  UINT8   BaseArchiveUUID[16];  // UUID of base archive
  UINT64  BaseArchiveTimestamp; // Timestamp of base archive
  UINT32  BaseArchiveCRC32;     // CRC32 of base archive header

  // Overlay metadata
  UINT64  OverlayTimestamp;     // When this overlay was created
  UINT32  OverlaySequence;      // Sequence number (for chaining)
  UINT32  FileCount;            // Number of files in overlay
  UINT32  NewFiles;             // Number of completely new files
  UINT32  ModifiedFiles;        // Number of modified files (delta stored)
  UINT32  DeletedFiles;         // Number of deleted files (tombstones)
  UINT32  UnchangedFiles;       // Number of unchanged files (references only)

  // Base archive location (for tape/HSM)
  UINT16  BaseVolumeNumber;     // Volume number where base resides
  UINT16  BaseTapeNumber;       // Tape number (0 if same tape)
  UINT64  BaseArchiveOffset;    // Offset to base archive

  UINT32  DescriptionLength;    // Length of description
  // [DescriptionLength bytes: UTF-8 description]
} ZOO64_OVERLAY_HEADER;
```

### 3.5.1 Overlay File Entry

Files in overlay archives use extended file headers to reference base archive:

```c
//
// Overlay file entry types
//
typedef enum _ZOO64_OVERLAY_TYPE {
  OverlayTypeNew       = 0,  // New file (not in base)
  OverlayTypeModified  = 1,  // Modified (delta stored)
  OverlayTypeDeleted   = 2,  // Deleted (tombstone)
  OverlayTypeUnchanged = 3,  // Unchanged (reference only)
  OverlayTypeMoved     = 4,  // Moved/renamed
} ZOO64_OVERLAY_TYPE;

//
// Overlay file metadata (follows file header)
//
typedef struct _ZOO64_OVERLAY_FILE_METADATA {
  UINT8   Type;                 // ZOO64_OVERLAY_TYPE
  UINT8   Reserved[3];

  // Reference to base archive file
  UINT64  BaseFileOffset;       // Offset in base archive (0 if new)
  UINT32  BaseFileCRC32;        // CRC32 of base file
  UINT64  BaseFileSize;         // Size of base file

  // Delta compression info (if Type == Modified)
  UINT16  DeltaAlgorithm;       // 0=xdelta3, 1=bsdiff, 2=zdelta, 3=vcdiff
  UINT64  DeltaSize;            // Size of delta

  // Move/rename info (if Type == Moved)
  UINT16  OldPathLength;        // Length of old path
  // [OldPathLength bytes: old UTF-8 path]
} ZOO64_OVERLAY_FILE_METADATA;
```

### 3.5.2 Overlay Archive Use Cases

**Incremental Tape Backup**:
```
Base Archive: Full backup to tape 1
Overlay 1:    Changes after 1 day  → tape 1
Overlay 2:    Changes after 2 days → tape 1
Overlay 3:    Changes after 3 days → tape 2 (tape 1 full)
```

**HSM Tiered Storage**:
```
Base Archive: Archived to slow tier (tape/cloud)
Overlay 1:    Recent changes on fast tier (disk)
Overlay 2:    Today's changes on fast tier
→ Restore by applying overlays to base
```

**Delta Storage for Large Files**:
```
Base:    database-2025-01-01.db (1 TB)
Overlay: database-2025-01-02.db (50 MB delta)
Overlay: database-2025-01-03.db (30 MB delta)
```

### 3.5.3 Overlay Flags

```
Bit 0:  Delta compression enabled
Bit 1:  Store tombstones for deleted files
Bit 2:  Store references for unchanged files
Bit 3:  Cross-volume base reference
Bit 4:  Cross-tape base reference
Bit 5:  Chained overlay (references another overlay)
Bit 6:  Compressed overlay header
Bit 7-31: Reserved
```

## 4. Compression (REQUIRED: Minimum "stored" mode)

Describes the compression algorithm and parameters.

```c
typedef struct _ZOO64_COMPRESSION_DESC {
  UINT32  DescriptorSize;     /// Size of this descriptor
  UINT32  Algorithm;          /// Compression algorithm ID
  UINT32  WindowSize;         /// Window size (4K - 1M)
  UINT32  BlockSize;          /// Block size for seekable (power of 2)
  UINT32  Level;              /// Compression level (0-9)
  UINT32  Flags;              /// Algorithm-specific flags
  UINT8   Parameters[64];     /// Algorithm-specific parameters
} ZOO64_COMPRESSION_DESC;
#pragma pack(pop)
```

### 4.1 Compression Algorithms

```
0x0000: None (stored)
0x0001: ZOZ (adaptive entropy-reduction pipeline)
0x0002: LZ77 (Lempel-Ziv 1977)
0x0003: LZ4 (extremely fast, moderate compression)
0x0004: ZSTD (Zstandard, Facebook)
0x0005: LZMA (7-Zip)
0x0006: LZMA2 (7-Zip, improved LZMA)
0x0007: LZX (Microsoft CAB)
0x0008: LZFSE (Apple)
0x0009: ZLIB (Deflate algorithm, RFC 1950)
0x000A: LZH (LHA/LZH archiver)
0x000B: LZW (Lempel-Ziv-Welch, GIF/TIFF)
0x000C: BROTLI (Google)
0x000D: BZIP2 (bzip2)
0x000E: PAQ (PAQ family, maximum compression)
0x000F: HUFFMAN (Huffman coding only)
0x0010: BIT_SQUISH (Reduced alphabet bit-packing)
0x0011: Crunch (Apple II Crunch)
0x0012: ACE (ACE archiver)
0x0013: ARJ (ARJ archiver)
0x0014: StuffIt (Macintosh StuffIt)
0x0015: MSCAB (Microsoft Cabinet format)
0x0016: LTO (LTO tape compression)
0x0100: Custom (parameters in descriptor)
```

**Algorithm Characteristics**:

| Algorithm | Speed  | Ratio | Memory | Notes                                    |
|-----------|--------|-------|--------|------------------------------------------|
| LZ4       | Fastest| Low   | Low    | Real-time compression, streaming         |
| ZSTD      | Fast   | High  | Med    | Best balance, Facebook, RFC 8478         |
| LZMA      | Slow   | Highest| High  | 7-Zip, excellent ratio                   |
| LZMA2     | Slow   | Highest| High  | Multi-threaded LZMA                      |
| BROTLI    | Medium | High  | Med    | Google, web compression, RFC 7932        |
| BZIP2     | Slow   | High  | Med    | Traditional, good ratio                  |
| ZLIB      | Fast   | Med   | Low    | Deflate, ubiquitous, RFC 1950            |
| LZX       | Medium | Med   | Med    | Microsoft CAB                            |
| LZFSE     | Fast   | Med   | Low    | Apple, optimized for Apple Silicon       |
| LZH       | Medium | Med   | Low    | LHA/LZH archiver, classic                |
| LZW       | Fast   | Low   | Low    | GIF/TIFF, patent-free since 2004         |
| PAQ       | Slowest| Max   | Huge   | Maximum compression, impractical         |
| HUFFMAN   | Fastest| Low   | Tiny   | Encoding only, no dictionary             |
| BIT_SQUISH| Fast   | High  | Low    | Reduced alphabet, excellent for text     |
| Crunch    | Medium | Med   | Low    | Apple II classic                         |
| ACE       | Slow   | High  | Med    | ACE 2.0 format                           |
| ARJ       | Medium | Med   | Low    | Classic DOS archiver                     |
| StuffIt   | Medium | Med   | Med    | Macintosh classic                        |
| MSCAB     | Medium | Med   | Med    | Microsoft Cabinet (LZX/MSZIP)            |
| LTO       | Fast   | Med   | Low    | LTO-5/6/7/8/9 tape hardware compression  |
| ZOZ       | Slow   | Max   | High   | Adaptive entropy-reduction pipeline      |

## 4.5 Archive YAML Metadata

The archive may contain YAML metadata that applies to the entire archive. This is optional and provides a flexible way to store arbitrary archive-level information.

### 4.5.1 YAML Metadata Block

```c
typedef struct _ZOO64_YAML_METADATA {
  UINT64  Magic;              /// 0x59414D4C4D455441 ("YAMLMETA")
  UINT32  YamlSize;           /// Size of YAML data in bytes
  UINT32  Flags;              /// YAML metadata flags
  /// Followed by YamlSize bytes of UTF-8 encoded YAML
} ZOO64_YAML_METADATA;
#pragma pack(pop)
```

### 4.5.2 YAML Metadata Flags

```
Bit 0:     Compressed (YAML data is compressed)
Bit 1:     Schema validated
Bit 2-31:  Reserved
```

### 4.5.3 Archive YAML Examples

Archive-level YAML metadata can contain:

```yaml
# Archive description and metadata
archive:
  title: "Project Source Code Archive"
  description: "Complete source code for Project XYZ v2.0"
  author: "Development Team"
  license: "MIT"
  version: "2.0.1"
  build: 12345

# Archive-level tags and categories
tags:
  - source-code
  - release
  - production

# Custom application-specific data
custom:
  project_id: "xyz-2024"
  repository: "https://github.com/org/project"
  ci_build_url: "https://ci.example.com/build/12345"

# Archive creation context
creation:
  hostname: "build-server-01"
  username: "ci-user"
  tool: "zoo64-cli v1.0"
  environment: "production"
```

## 4.6 Quick Directory [OPTIONAL]

The Quick Directory is an optional structure that appears at the beginning of the archive (after YAML metadata, before file entries). It provides fast file listing without requiring a full archive scan or seeking to the Central Directory at the end.

### 4.6.1 Quick Directory Header

```c
#pragma pack(push, 1)
typedef struct _ZOO64_QUICK_DIRECTORY {
  UINT64  Magic;              /// 0x5155494344495220 ("QUICKDIR")
  UINT32  DirectorySize;      /// Total size of quick directory
  UINT32  EntryCount;         /// Number of files in archive
  UINT32  Flags;              /// Quick directory flags
  UINT64  FirstFileOffset;    /// Offset to first file entry
  UINT64  CentralDirOffset;   /// Offset to central directory (for verification)
  /// Followed by EntryCount quick entries
} ZOO64_QUICK_DIRECTORY;
#pragma pack(pop)
```

**Quick Directory Flags**:
```
0x00000001: SORTED_BY_NAME     /// Entries sorted alphabetically by name
0x00000002: SORTED_BY_OFFSET   /// Entries sorted by file offset
0x00000004: SORTED_BY_SIZE     /// Entries sorted by size
0x00000008: HASH_TABLE         /// Includes hash table for O(1) lookup
0x00000010: COMPRESSED         /// Quick directory is compressed
0x00000020: ENCRYPTED          /// Quick directory is encrypted
0x00000040: INCREMENTAL        /// Support for incremental updates
```

### 4.6.2 Quick File Entry

Each entry in the Quick Directory contains minimal metadata for fast listing:

```c
#pragma pack(push, 1)
typedef struct _ZOO64_QUICK_ENTRY {
  UINT16  PathLength;         /// Length of path in bytes
  UINT64  FileOffset;         /// Offset to full file entry
  UINT64  UncompressedSize;   /// Uncompressed size
  UINT64  CompressedSize;     /// Compressed size
  UINT32  CRC32;              /// CRC32 of uncompressed data
  UINT32  Flags;              /// File flags (subset from full header)
  /// Followed by:
  ///   [PathLength bytes: UTF-8 path]
} ZOO64_QUICK_ENTRY;
#pragma pack(pop)
```

**Quick Entry Flags**:
```
0x00000001: IS_DIRECTORY       /// Entry is a directory
0x00000002: IS_COMPRESSED      /// File is compressed
0x00000004: IS_ENCRYPTED       /// File is encrypted
0x00000008: IS_SOLID           /// File is in solid block
0x00000010: HAS_METADATA       /// File has metadata chunks
0x00000020: HAS_SIGNATURE      /// File has digital signature
0x00000040: IS_SYMLINK         /// Entry is symbolic link
0x00000080: IS_DELETED         /// Entry marked for deletion (incremental)
```

### 4.6.3 Quick Directory Hash Table (Optional)

For O(1) filename lookup, an optional hash table can be included:

```c
#pragma pack(push, 1)
typedef struct _ZOO64_QUICK_HASH_TABLE {
  UINT32  BucketCount;        /// Number of hash buckets (power of 2)
  UINT32  EntryCount;         /// Total entries in table
  UINT32  HashFunction;       /// Hash function ID
  UINT32  Reserved;           /// Reserved
  /// Followed by:
  ///   [BucketCount * UINT32: bucket indices]
  ///   [EntryCount * HASH_ENTRY: hash entries]
} ZOO64_QUICK_HASH_TABLE;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _ZOO64_HASH_ENTRY {
  UINT32  Hash;               /// Hash of filename
  UINT32  EntryIndex;         /// Index into quick directory
  UINT32  NextIndex;          /// Next entry in bucket (chain)
} ZOO64_HASH_ENTRY;
#pragma pack(pop)
```

**Hash Functions**:
```
0x0000: None (linear search)
0x0001: FNV-1a 32-bit
0x0002: MurmurHash3 32-bit
0x0003: CityHash 32-bit
0x0004: xxHash 32-bit
```

### 4.6.4 Advantages of Quick Directory

**Performance Benefits**:
- **Fast listing**: O(n) scan of minimal entries vs. O(n) scan of full headers
- **No seeking**: Sequential read from archive start
- **Bandwidth efficient**: Only name + size + offset (typically 50-100 bytes/file)
- **Hash table**: O(1) lookup for specific files (vs. O(n) scan)

**Size Comparison** (1000 files):
- Quick Directory: ~80 KB (80 bytes/entry average)
- Full headers scan: ~500 KB - 5 MB (depends on metadata)
- Central Directory: ~500 KB - 5 MB (full metadata)

**Use Cases**:
- Listing archive contents without full scan
- Finding specific file offset without reading all headers
- Archive verification (compare quick dir vs. central dir)
- Streaming archives where end-of-archive is not yet available
- Network archives where seeking is expensive

### 4.6.5 Quick Directory Generation

**On Archive Creation**:
```
1. Write Archive Header, Compression Descriptor, YAML metadata
2. Reserve space for Quick Directory (estimate: 100 bytes/file)
3. For each file:
   a. Write file entry with full metadata
   b. Record offset, size, path for quick entry
4. After all files written:
   a. Generate Quick Directory with all entries
   b. Optionally build hash table
   c. Optionally compress/encrypt
   d. Write to reserved space (or append if too large)
5. Update Archive Header with quick dir offset/size
6. Write Central Directory and End marker
```

**On Archive Read**:
```
1. Read Archive Header
2. Check if Quick Directory present (QuickDirOffset != 0)
3. If present:
   a. Seek to quick directory
   b. Read and parse quick entries
   c. Build in-memory index
4. For file listing: use quick directory
5. For file extraction: use offset from quick entry to jump to file data
```

### 4.6.6 Incremental Updates

The Quick Directory supports incremental archive updates:

```c
#pragma pack(push, 1)
typedef struct _ZOO64_QUICK_ENTRY_V2 {
  UINT16  PathLength;         /// Length of path
  UINT64  FileOffset;         /// Offset to file entry
  UINT64  UncompressedSize;   /// Uncompressed size
  UINT64  CompressedSize;     /// Compressed size
  UINT32  CRC32;              /// CRC32
  UINT32  Flags;              /// Flags (including IS_DELETED)
  UINT32  Version;            /// File version number
  UINT64  Timestamp;          /// Last modification timestamp
  /// Followed by path
} ZOO64_QUICK_ENTRY_V2;
#pragma pack(pop)
```

**Incremental Operations**:
- **Add file**: Append new quick entry with highest version
- **Delete file**: Mark entry with IS_DELETED flag
- **Update file**: Add new entry with incremented version, mark old as deleted
- **Compaction**: Rebuild quick directory removing deleted entries

### 4.6.7 Example Code

**Reading Quick Directory**:
```c
bool read_quick_directory(
    FILE *archive,
    ZOO64_ARCHIVE_HEADER *header,
    ZOO64_QUICK_ENTRY **entries,
    uint32_t *count
) {
    if (header->QuickDirOffset == 0) {
        return false;  /// No quick directory
    }

    /// Seek to quick directory
    fseek(archive, header->QuickDirOffset, SEEK_SET);

    /// Read header
    ZOO64_QUICK_DIRECTORY qdir;
    fread(&qdir, sizeof(qdir), 1, archive);

    /// Allocate entries
    *entries = malloc(qdir.EntryCount * sizeof(ZOO64_QUICK_ENTRY));
    *count = qdir.EntryCount;

    /// Read each entry
    for (uint32_t i = 0; i < qdir.EntryCount; i++) {
        fread(&(*entries)[i], sizeof(ZOO64_QUICK_ENTRY), 1, archive);
        /// Read path
        char *path = malloc((*entries)[i].PathLength + 1);
        fread(path, 1, (*entries)[i].PathLength, archive);
        path[(*entries)[i].PathLength] = '\0';
        /// Store path...
    }

    return true;
}
```

**Fast File Lookup**:
```c
int64_t find_file_offset(
    ZOO64_QUICK_ENTRY *entries,
    uint32_t count,
    const char *filename
) {
    /// Linear search (or use hash table if available)
    for (uint32_t i = 0; i < count; i++) {
        if (strcmp(entries[i].path, filename) == 0) {
            return entries[i].FileOffset;
        }
    }
    return -1;  /// Not found
}
```

## 5. File Entry Format [REQUIRED]

Each file in the archive has the following structure:

```
[File Header]
[Variable-length UTF-8 Path]
[Metadata Block] (optional)
[Data Blocks]
[File Signature] (optional)
```

### 5.1 File Header

```c
typedef struct _ZOO64_FILE_HEADER {
  UINT64  Magic;              /// 0x46494C45454E5452 ("FILEENTR")
  UINT32  HeaderSize;         /// Total size of header + path
  UINT32  Flags;              /// File flags
  UINT64  UncompressedSize;   /// Original file size
  UINT64  CompressedSize;     /// Compressed size (0 if stored)
  UINT64  DataOffset;         /// Offset to file data
  UINT64  MetadataOffset;     /// Offset to metadata (0 if none)
  UINT32  MetadataSize;       /// Size of metadata block
  UINT16  PathLength;         /// Length of UTF-8 path in bytes
  UINT16  CompressionMethod;  /// Compression method for this file
  UINT32  CRC32;              /// CRC32 of uncompressed data
  UINT64  SHA256[4];          /// SHA-256 hash of uncompressed data
  UINT64  BirthTime;          /// File birth/creation time (NTP extended format)
  UINT64  ModificationTime;   /// File modification time (NTP extended format)
  UINT64  AccessTime;         /// File access time (NTP extended format)
  UINT64  ChangeTime;         /// File metadata change time (NTP extended format)
  UINT32  UID;                /// User ID (Unix)
  UINT32  GID;                /// Group ID (Unix)
  UINT32  Mode;               /// File mode/permissions (Unix)
  UINT32  Attributes;         /// Platform-specific attributes
} ZOO64_FILE_HEADER;
#pragma pack(pop)
```

**File Timestamps**:
- **BirthTime**: File creation/birth time (when the file was first created)
- **ModificationTime**: Last data modification time (mtime)
- **AccessTime**: Last access time (atime)
- **ChangeTime**: Last metadata change time (ctime - permissions, owner, etc.)

All four timestamps use NTP extended format for maximum precision and consistency.

### 5.1a Extended File Header (Zettabyte Support)

For archives containing files larger than 16 EiB (exbibytes), Zoo64 supports extended 128-bit file sizes. When bit 26 of file flags is set (Large File), the header is extended:

```c
//
// Extended file header for files >= 16 EiB
// Uses 128-bit integers for size fields
//
typedef struct _ZOO64_FILE_HEADER_EXTENDED {
  UINT64  Magic;              // 0x46494C45454E5452 ("FILEENTR")
  UINT32  HeaderSize;         // Total size of header + path
  UINT32  Flags;              // File flags (bit 26 set = Large File)

  // Extended 128-bit size fields for ZB support
  UINT128 UncompressedSize;   // Original file size (up to 16 ZiB)
  UINT128 CompressedSize;     // Compressed size

  UINT64  DataOffset;         // Offset to file data (or use extended if needed)
  UINT64  MetadataOffset;     // Offset to metadata (0 if none)
  UINT32  MetadataSize;       // Size of metadata block
  UINT16  PathLength;         // Length of UTF-8 path in bytes
  UINT16  CompressionMethod;  // Compression method for this file
  UINT32  CRC32;              // CRC32 of uncompressed data (or streaming CRC)
  UINT64  SHA256[4];          // SHA-256 hash of uncompressed data
  UINT64  BirthTime;          // File birth/creation time (NTP extended format)
  UINT64  ModificationTime;   // File modification time
  UINT64  AccessTime;         // File access time
  UINT64  ChangeTime;         // File metadata change time
  UINT32  UID;                // User ID
  UINT32  GID;                // Group ID
  UINT32  Mode;               // File mode/permissions
  UINT32  Attributes;         // Platform-specific attributes
} ZOO64_FILE_HEADER_EXTENDED;

//
// 128-bit integer type for ZB support
//
typedef struct _UINT128 {
  UINT64 Low;   // Lower 64 bits
  UINT64 High;  // Upper 64 bits
} UINT128;
```

**Size Ranges**:
- **Standard (64-bit)**: 0 to 16 EiB (2^64 bytes)
- **Extended (128-bit)**: 0 to 16 ZiB (zebibytes, 2^128 bytes)

**Usage**: Extended headers are required when archiving:
- Large databases (> 16 EiB)
- Data center tape archives
- Exascale computing datasets
- Large-scale scientific data (astronomy, genomics, climate)
- Aggregate HSM archives spanning petabytes

### 5.2 File Flags

```
Bit 0:     Compressed
Bit 1:     Seekable (has block table)
Bit 2:     Encrypted
Bit 3:     Signed
Bit 4:     Has metadata block
Bit 5:     Has ACLs
Bit 6:     Has extended attributes
Bit 7:     Has alternate data streams
Bit 8:     Symbolic link
Bit 9:     Hard link
Bit 10:    Directory
Bit 11:    FIFO (named pipe)
Bit 12:    Socket (Unix domain socket)
Bit 13:    Block device
Bit 14:    Character device
Bit 15:    Door (Solaris IPC)
Bit 16:    Event port
Bit 17:    Whiteout (BSD union mounts)
Bit 18:    Magic symlink (junction, alias, etc.)
Bit 19:    Has short filename (8.3, name~n, etc.)
Bit 20:    Has HFS-encoded filename (name#hex)
Bit 21:    Text file (detected or specified)
Bit 22:    Binary file (detected or specified)
Bit 23:    Sparse file (has sparse regions)
Bit 24:    Delta-compressed (stored as delta)
Bit 25:    Overlay reference (references base archive)
Bit 26:    Large file (uses 128-bit sizes for ZB support)
Bit 27:    Hybrid compression (auto-select algorithm per block)
Bit 28:    Variable block sizes
Bit 29-31: Reserved for future use
```

### 5.3 Variable-Length UTF-8 Path

Immediately follows file header. Length specified in `PathLength` field.

**IMPORTANT: Paths are internally normalized** before storage in the archive.

**Path Normalization Rules**:

1. **Unicode Normalization**: All paths converted to NFC (Canonical Composition)
   - Ensures consistent comparison across systems
   - Example: é (U+00E9) vs e+´ (U+0065 U+0301) → both become U+00E9

2. **Separator Normalization**: All path separators converted to `\0` (null byte)
   - `/` (POSIX) → `\0`
   - `\` (Windows) → `\0`
   - `:` (Classic Mac) → `\0`
   - Any platform separator → `\0`

3. **Redundant Separator Removal**: Multiple consecutive separators collapsed
   - `dir//file` → `dir\0file\0\0`
   - `\\server\\share` → `\0\0server\0share\0\0` (UNC preserved)

4. **Dot Component Resolution**:
   - `.` (current directory) removed from path
   - `..` (parent directory) resolved during archiving
   - Example: `dir/./subdir/../file` → `dir\0file\0\0`
   - **Exception**: Leading `..` in relative paths preserved

5. **Trailing Separator Removal**:
   - `dir/subdir/` → `dir\0subdir\0\0`
   - Except for root paths

6. **Case Preservation**:
   - Original case **always preserved** in archive
   - Case sensitivity flag stored in metadata
   - No case folding during normalization

7. **Whitespace Normalization**:
   - Leading/trailing whitespace in path components **preserved**
   - Internal whitespace **preserved**
   - No trimming applied

**Internal Format**:
```
[component1]\0[component2]\0...[componentN]\0\0
```

- Each component is UTF-8 encoded
- Components separated by `\0` (null byte)
- Path terminated by `\0\0` (double null)
- No separators within components
- Empty first component indicates absolute path

**Normalization Examples**:

```
Input:     /home//user/./docs/../file.txt
Normalized: \0home\0user\0file.txt\0\0

Input:     C:\Users\John\..\Public\file.txt
Normalized: C:\0Users\0Public\0file.txt\0\0

Input:     docs/./readme.md
Normalized: docs\0readme.md\0\0

Input:     ../../config/app.conf
Normalized: ..\0..\0config\0app.conf\0\0
```

**Path Component Structure**:
- **Absolute POSIX**: First component empty (starts with `\0`)
  - `/home/user` → `\0home\0user\0\0`

- **Absolute Windows**: Drive letter with colon in first component
  - `C:\Users` → `C:\0Users\0\0`

- **UNC Path**: First two components form `\\server\share`
  - `\\server\share\dir` → `\0\0server\0share\0dir\0\0`

- **Relative Path**: First component non-empty
  - `docs/file` → `docs\0file\0\0`

**Length Calculation**:
PathLength includes all bytes from first byte to final `\0` of the `\0\0` terminator.

Example:
```
Path: "docs\0readme.md\0\0"
Bytes: 'd' 'o' 'c' 's' '\0' 'r' 'e' 'a' 'd' 'm' 'e' '.' 'm' 'd' '\0' '\0'
Length: 16 bytes
```

**Denormalization on Extraction**:
When extracting, paths are denormalized to target platform format:
- Split on `\0` to get components
- Rejoin with platform separator (`/` on POSIX, `\` on Windows)
- Preserve absolute/relative nature
- Handle special cases (drive letters, UNC paths)

**Benefits of Normalization**:
- **Consistency**: Same logical path has same representation
- **Deduplication**: Duplicate paths detected reliably
- **Comparison**: Binary comparison of normalized paths
- **Platform-agnostic**: Archive format independent of source OS
- **Security**: Prevents path traversal attacks (.. resolved)
- **Compression**: Better compression ratio (no separator variance)

## 6. Extended Metadata [OPTIONAL]

Optional block containing extended filesystem metadata.

```c
typedef struct _ZOO64_METADATA_HEADER {
  UINT64  Magic;              /// 0x4D45544144415441 ("METADATA")
  UINT32  TotalSize;          /// Total size of metadata block
  UINT32  ChunkCount;         /// Number of metadata chunks
} ZOO64_METADATA_HEADER;
#pragma pack(pop)
```

### 6.1 Metadata Chunks

Each metadata chunk has the following format:

```c
typedef struct _ZOO64_METADATA_CHUNK {
  UINT32  ChunkType;          /// Chunk type identifier
  UINT32  ChunkSize;          /// Size of chunk data (excludes header)
  /// Followed by chunk data
} ZOO64_METADATA_CHUNK;
#pragma pack(pop)
```

### 6.2 Metadata Chunk Types

```
0x0001: ACL (Access Control List)
0x0002: Extended Attributes (xattrs)
0x0003: Alternate Data Streams
0x0004: Security Descriptor (Windows)
0x0005: Resource Fork (macOS)
0x0006: Extended timestamps
0x0007: File capabilities (Linux)
0x0008: SELinux context
0x0009: Custom metadata
0x000A: YAML metadata (file-level)
0x000B: macOS UUIDs (user/group)
0x000C: BSD flags
0x000D: Linux flags
0x000E: Windows attributes
0x000F: Hard link target
0x0010: Symbolic link target
0x0011: OpenVMS ODS-5 attributes
0x0012: z/OS dataset attributes
0x0013: OS/400 (IBM i) attributes
0x0014: Lisa Office System attributes
0x0015: UNIVAC 2200 attributes
0x0016: PDP-10 attributes (TENEX/ITS/TOPS-10/TOPS-20)
0x0017: Classic Mac OS attributes
0x0018: Amiga attributes
0x0019: Atari TOS/GEM attributes
0x001A: Acorn RISC OS attributes
0x001B: Commodore 64/128 attributes
0x001C: Apple IIGS ProDOS attributes
0x001D: Stratus VOS attributes
0x001E: Netware attributes
0x001F: Banyan VINES attributes
0x0020: AFS (Andrew File System) attributes
0x0021: CODA distributed filesystem attributes
0x0022: GFS (Global File System) attributes
0x0023: DFS (Distributed File System) attributes
0x0024: Device file attributes (block/character devices)
0x0025: FIFO attributes (named pipes)
0x0026: Socket attributes (Unix domain sockets)
0x0027: Door attributes (Solaris doors)
0x0028: Event port attributes
0x0029: Whiteout attributes (BSD union mounts)
0x002A: Magic symlink attributes (junctions, aliases, etc.)
0x002B: APFS attributes (clones, encryption, snapshots)
0x002C: ReFS attributes (integrity streams, block cloning)
0x002D: VxFS attributes (Veritas - checkpoints, DMAPI)
0x002E: HPFS attributes (OS/2 extended attributes)
0x002F: ZFS attributes (snapshots, clones, checksums)
0x0030: AdvFS attributes (Tru64 - filesets, clones)
0x0031: XFS attributes (real-time subvolumes, reflinks)
0x0032: JFS attributes (IBM JFS - compression)
0x0033: ReiserFS attributes (tail packing)
0x0034: Btrfs attributes (subvolumes, snapshots, reflinks)
0x0035: Git metadata (commits, refs, objects)
0x0036: Perforce (P4) metadata (changelists, revisions)
0x0037: Subversion (SVN) metadata (revisions, properties)
0x0038: CVS metadata (revisions, tags)
0x0039: RCS metadata (revisions, locks)
0x003A: Mercurial metadata (changesets, bookmarks)
0x003B: Fossil metadata (artifacts, manifests)
0x003C: WebDAV metadata (properties, locks)
0x003D: NFS metadata (file handles, attributes)
0x003E: SMB/CIFS metadata (streams, security)
0x003F: NetWare NCP metadata (trustees, attributes)
0x0040: AFP metadata (resource forks, Finder info)
0x0041: DCE DFS metadata (ACLs, file IDs)
0x0042: Short filename metadata (8.3, name~n formats)
0x0043: HFS filename metadata (HFS character encoding)
0x0044: File type detection metadata (text/binary/MIME)
0x0045: Sparse file metadata (hole map)
0x0046: Delta revision metadata (base + delta)
0x0047: UDF (Universal Disk Format) metadata
0x0048: ISO 9660 metadata (with Rock Ridge/Joliet/El Torito)
0x0049: High Sierra metadata
0x004A: Block deduplication metadata (chunk hashes)
0x004B: Reserved filename metadata (Windows/DOS/OS2 device names)
0x004C: DR-DOS file password metadata
0x004D: CP/M USER DIR metadata
0x004E: Olivetti pcos metadata
0x004F: WIM (Windows Imaging Format) metadata
0x0050: WIM resource metadata (single-instance storage)
0x0051: WIM boot metadata (bootable image)
0x0052: WIM integrity metadata
```

### 6.3 Universal ACL Format

Zoo64 uses a **normalized universal ACL format** that can represent ACLs from any system while preserving full semantic information for round-trip conversion.

```c
typedef struct _ZOO64_ACL_HEADER {
  UINT32  EntryCount;         /// Number of ACL entries
  UINT32  TotalSize;          /// Total size of ACL data
  UINT32  Flags;              /// ACL header flags
  UINT32  SourceSystem;       /// Original ACL system (for optimization)
} ZOO64_ACL_HEADER;
#pragma pack(pop)
```

#### 6.3.1 Universal ACL Entry

Each ACL entry uses a universal format that can represent any ACL system:

```c
typedef struct _ZOO64_ACL_ENTRY {
  UINT32  ACEType;            /// Universal ACE type
  UINT32  Flags;              /// Universal flags
  UINT64  Permissions;        /// Universal permission bitmap (64 bits)
  UINT16  PrincipalCount;     /// Number of principal identifiers (usually 1-3)
  UINT16  Reserved;           /// Reserved for alignment
  UINT32  SourceSystem;       /// Original ACL system
  UINT32  SourceSpecific[4];  /// System-specific metadata (16 bytes)
  /// Followed by PrincipalCount * sizeof(PRINCIPAL_ID) structures
} ZOO64_ACL_ENTRY;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _PRINCIPAL_ID {
  UINT16  PrincipalType;      /// Type of principal identifier
  UINT16  PrincipalLength;    /// Length of principal identifier data
  /// Followed by principal identifier data (variable length)
} PRINCIPAL_ID;
#pragma pack(pop)
```

#### 6.3.2 Source ACL Systems

```
0x0000: Unknown/Generic
0x0001: POSIX.1e (Linux, FreeBSD, etc.)
0x0002: NFSv4 (RFC 7530)
0x0003: Windows NT (DACL/SACL)
0x0004: macOS Extended ACLs
0x0005: OpenVMS ACLs
0x0006: OS/400 Object Authorities
0x0007: MVS/RACF
0x0008: Novell Netware
0x0009: Banyan VINES
0x000A: AFS (Andrew File System)
0x000B: CODA Distributed FS
0x000C: Solaris ZFS ACLs (NFSv4-based)
0x000D: AIX ACLs (POSIX.1e extended)
0x000E: HP-UX ACLs (HP-UX 11i ACLs)
0x000F: IRIX ACLs (XFS ACLs, NFSv4-like)
0x0010: UnixWare ACLs
0x0011: SCO OpenServer ACLs
0x0012: UNICOS ACLs (Cray supercomputers)
0x0013: UNICOS/mk ACLs (Cray T3E/SV1)
0x0014: MULTICS ACLs (Honeywell/MIT)
0x0015: GNU Hurd ACLs (translator-based)
0x0016: Plan 9 ACLs (capability-based)
0x0017: BeOS/Haiku ACLs (attribute-based)
0x0018: VMware VMFS ACLs (vSphere)
0x0019: DCE DFS ACLs (Distributed Computing Environment)
0x001A: GFS ACLs (Global File System, cluster-aware)
0x001B: DFS ACLs (Distributed File System, Microsoft)
```

**Design Rationale**: Storing the source system allows optimized round-trip conversion while the universal format enables cross-platform ACL translation.

**Note**: Many Unix variants (Solaris, AIX, IRIX) use NFSv4 or POSIX.1e as base, with extensions stored in SourceSpecific fields.

**DCE DFS Integration**: DCE DFS ACLs (type 0x0041 metadata) can be represented in the universal ACL format using source system 0x0019. The DCE-specific fields (file ID, replication info, epoch) are stored in SourceSpecific fields or as separate 0x0041 metadata when full DCE DFS context is required. For simple ACL-only representation, use the universal format; for complete DCE DFS metadata preservation, use 0x0041.

#### 6.3.3 Universal ACE Types

```
0x00000000: ACCESS_ALLOWED      /// Grant permissions
0x00000001: ACCESS_DENIED       /// Deny permissions
0x00000002: SYSTEM_AUDIT        /// Audit access
0x00000003: SYSTEM_ALARM        /// Alarm on access
0x00000004: ACCESS_ALLOWED_OBJECT    /// Object-specific allow
0x00000005: ACCESS_DENIED_OBJECT     /// Object-specific deny
0x00000006: SYSTEM_AUDIT_OBJECT      /// Object-specific audit
0x00000007: ACCESS_ALLOWED_CALLBACK  /// Callback allow
0x00000008: ACCESS_DENIED_CALLBACK   /// Callback deny
```

#### 6.3.4 Universal Flags

```
Inheritance Flags:
  0x00000001: FILE_INHERIT          /// Inherit to files
  0x00000002: DIRECTORY_INHERIT     /// Inherit to directories
  0x00000004: NO_PROPAGATE_INHERIT  /// Don't propagate beyond immediate children
  0x00000008: INHERIT_ONLY          /// ACE only for inheritance, not this object
  0x00000010: INHERITED_ACE         /// This ACE was inherited

Audit Flags:
  0x00000040: SUCCESSFUL_ACCESS     /// Audit successful access
  0x00000080: FAILED_ACCESS         /// Audit failed access

Special Flags:
  0x00000100: IDENTIFIER_GROUP      /// Principal is a group
  0x00000200: PROTECTED             /// Protected from modification
  0x00000400: CRITICAL              /// Critical ACE
  0x00000800: DEFAULT_ACL           /// Default ACL (for new objects)
```

#### 6.3.5 Universal Permissions (64-bit bitmap)

```
Basic Permissions (bits 0-7):
  0x0000000000000001: READ_DATA / LIST_DIRECTORY
  0x0000000000000002: WRITE_DATA / ADD_FILE
  0x0000000000000004: APPEND_DATA / ADD_SUBDIRECTORY
  0x0000000000000008: READ_EXTENDED_ATTR
  0x0000000000000010: WRITE_EXTENDED_ATTR
  0x0000000000000020: EXECUTE / TRAVERSE
  0x0000000000000040: DELETE_CHILD
  0x0000000000000080: READ_ATTRIBUTES

Object Permissions (bits 8-15):
  0x0000000000000100: WRITE_ATTRIBUTES
  0x0000000000000200: DELETE
  0x0000000000000400: READ_PERMISSIONS
  0x0000000000000800: WRITE_PERMISSIONS
  0x0000000000001000: CHANGE_OWNER
  0x0000000000002000: SYNCHRONIZE
  0x0000000000004000: ACCESS_SYSTEM_SECURITY
  0x0000000000008000: MAXIMUM_ALLOWED

Extended Permissions (bits 16-31):
  0x0000000000010000: LOCK / CREATE_LINK
  0x0000000000020000: EXTEND_FILE
  0x0000000000040000: READ_NAMED_ATTRS
  0x0000000000080000: WRITE_NAMED_ATTRS
  0x0000000000100000: MODIFY_METADATA
  0x0000000000200000: REPLICATE
  0x0000000000400000: OBJECT_MANAGEMENT
  0x0000000000800000: OBJECT_EXISTENCE

Control Permissions (bits 32-39):
  0x0000000001000000: OBJECT_ALTER
  0x0000000002000000: OBJECT_REFERENCE
  0x0000000004000000: OBJECT_OPERATIONAL
  0x0000000008000000: AUTHORIZATION_LIST
  0x0000000010000000: TRUSTEE_RIGHTS
  0x0000000020000000: INHERITED_RIGHTS_FILTER
  0x0000000040000000: SUPERVISOR
  0x0000000080000000: TAKE_OWNERSHIP

Audit/Security (bits 40-47):
  0x0000000100000000: READ_AUDIT
  0x0000000200000000: WRITE_AUDIT
  0x0000000400000000: SECURITY_LEVEL_0
  0x0000000800000000: SECURITY_LEVEL_1
  0x0000001000000000: SECURITY_LEVEL_2
  0x0000002000000000: SECURITY_LEVEL_3
  0x0000004000000000: CONTROL_ACCESS
  0x0000008000000000: ALTER_ACCESS

System-Specific (bits 48-63):
  Reserved for system-specific permissions
```

#### 6.3.6 Principal Types

```
0x0001: USER_ID              /// Numeric user ID (POSIX UID)
0x0002: GROUP_ID             /// Numeric group ID (POSIX GID)
0x0003: USER_NAME            /// UTF-8 username
0x0004: GROUP_NAME           /// UTF-8 group name
0x0005: SID                  /// Windows SID (binary)
0x0006: UUID                 /// macOS UUID (128-bit)
0x0007: UIC                  /// OpenVMS UIC (32-bit)
0x0008: IDENTIFIER_NAME      /// OpenVMS identifier name
0x0009: NETWARE_OBJECT_ID    /// Netware object ID (32-bit)
0x000A: STREETTALK_NAME      /// VINES StreetTalk name (item@group@org)
0x000B: AFS_PRINCIPAL        /// AFS principal name
0x000C: RACF_USER            /// RACF user ID (8 chars)
0x000D: OS400_PROFILE        /// OS/400 user profile (10 chars)
0x000E: AUTHORIZATION_LIST   /// OS/400 authorization list name
0x000F: SPECIAL_PRINCIPAL    /// Special (OWNER@, GROUP@, EVERYONE@, etc.)
```

#### 6.3.7 Special Principals

For SPECIAL_PRINCIPAL type, the principal data is a 4-byte identifier:

```
0x00000001: OWNER@           /// File owner
0x00000002: GROUP@           /// File group
0x00000003: EVERYONE@        /// All users
0x00000004: INTERACTIVE@     /// Interactive users
0x00000005: NETWORK@         /// Network users
0x00000006: DIALUP@          /// Dial-up users
0x00000007: BATCH@           /// Batch jobs
0x00000008: ANONYMOUS@       /// Anonymous users
0x00000009: AUTHENTICATED@   /// Authenticated users
0x0000000A: SERVICE@         /// Service accounts
0x0000000B: SYSTEM@          /// System processes
```

#### 6.3.8 System-Specific Metadata

The `SourceSpecific[4]` field (16 bytes) stores system-specific information:

**POSIX (0x0001)**:
- `[0]`: ACL tag (USER_OBJ, USER, GROUP_OBJ, GROUP, MASK, OTHER)
- `[1]`: Reserved
- `[2]`: Reserved
- `[3]`: Reserved

**NFSv4 (0x0002)**:
- `[0]`: Original NFSv4 flags
- `[1]`: Reserved
- `[2]`: Reserved
- `[3]`: Reserved

**Windows NT (0x0003)**:
- `[0]`: Original ACE type (DACL vs SACL)
- `[1]`: Object type GUID (if object ACE)
- `[2]`: Inherited object type GUID
- `[3]`: Reserved

**macOS (0x0004)**:
- `[0]`: Original KAUTH flags
- `[1-3]`: Reserved

**OpenVMS (0x0005)**:
- `[0]`: ACE type (FILE, KEYID, ADDACC, DEFAULT)
- `[1]`: OpenVMS-specific flags
- `[2-3]`: Reserved

**OS/400 (0x0006)**:
- `[0]`: Authorization type (user, group, *PUBLIC)
- `[1]`: Object authority flags
- `[2]`: Data authority flags
- `[3]`: Reserved

**MVS/RACF (0x0007)**:
- `[0]`: Access level (NONE, READ, UPDATE, CONTROL, ALTER)
- `[1]`: Access type (UNIVERSAL, CONDITIONAL, GROUP)
- `[2]`: Security level (0-255)
- `[3]`: Security categories bitmap (first 32 bits)

**Netware (0x0008)**:
- `[0]`: Trustee rights bitmap
- `[1]`: Inherited rights filter
- `[2]`: Object type
- `[3]`: Reserved

**VINES (0x0009)**:
- `[0]`: Entry type (user, group, list)
- `[1-3]`: Reserved

**AFS (0x000A)**:
- `[0]`: Entry type (POSITIVE or NEGATIVE)
- `[1-3]`: Reserved

**CODA (0x000B)**:
- `[0]`: Entry type (POSITIVE or NEGATIVE)
- `[1]`: Replication policy
- `[2-3]`: Reserved

#### 6.3.9 ACL Mapping Examples

**Example 1: POSIX ACL → Universal**
```
POSIX: user:alice:rwx
→ Universal:
  ACEType: ACCESS_ALLOWED
  Flags: 0
  Permissions: 0x0000000000000007 (READ_DATA | WRITE_DATA | EXECUTE)
  PrincipalType: USER_NAME
  Principal: "alice" (UTF-8)
  SourceSystem: POSIX
  SourceSpecific[0]: ACL_USER
```

**Example 2: Windows NT → Universal**
```
NT: Allow BUILTIN\Administrators Full Control
→ Universal:
  ACEType: ACCESS_ALLOWED
  Flags: 0
  Permissions: 0x00000000FFFFFFFF (all basic + object permissions)
  PrincipalType: SID
  Principal: S-1-5-32-544 (binary SID)
  SourceSystem: NT
  SourceSpecific[0]: ACCESS_ALLOWED_ACE_TYPE
```

**Example 3: NFS4 → Universal**
```
NFS4: A::OWNER@:rwatTnNcCoy
→ Universal:
  ACEType: ACCESS_ALLOWED
  Flags: 0
  Permissions: 0x0000000000001F87 (mapped from NFSv4 rights)
  PrincipalType: SPECIAL_PRINCIPAL
  Principal: 0x00000001 (OWNER@)
  SourceSystem: NFSv4
```

**Example 4: OS/400 → Universal**
```
OS/400: QSECOFR *ALL
→ Universal:
  ACEType: ACCESS_ALLOWED
  Flags: 0
  Permissions: 0x000000000000FFFF (all basic permissions)
  PrincipalType: OS400_PROFILE
  Principal: "QSECOFR   " (10 chars, space-padded)
  SourceSystem: OS/400
  SourceSpecific[0]: USER
  SourceSpecific[1]: *OBJMGT | *OBJEXIST | *OBJALTER | *OBJREF | *OBJOPER
  SourceSpecific[2]: *READ | *ADD | *UPD | *DLT | *EXECUTE
```

#### 6.3.10 ACL Translation Guidelines

When converting between ACL systems:

1. **Preserve semantics**: Map permissions to closest equivalent
2. **Record source**: Always set SourceSystem for round-trip accuracy
3. **Use SourceSpecific**: Store system-specific data for lossless conversion
4. **Flag incompatibility**: If exact mapping impossible, document in YAML metadata
5. **Special principals**: Map owner/group/everyone consistently across systems

#### 6.3.9 Multiple Principal IDs

Zoo64 supports **multiple identifiers for the same principal** in a single ACL entry, enabling seamless cross-platform ACL translation without requiring separate entries.

**Common Multi-ID Patterns**:

1. **POSIX + SID** (Linux <-> Windows):
   ```
   PrincipalCount: 2
   Principal[0]: USER_ID (UID 1000)
   Principal[1]: SID (S-1-5-21-...)
   ```

2. **POSIX + UUID** (Linux <-> macOS):
   ```
   PrincipalCount: 2
   Principal[0]: USER_ID (UID 1000)
   Principal[1]: UUID (XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX)
   ```

3. **UID + GID + SID + UUID** (Universal):
   ```
   PrincipalCount: 4
   Principal[0]: USER_ID (UID 1000)
   Principal[1]: GROUP_ID (GID 1000)
   Principal[2]: SID (S-1-5-21-...)
   Principal[3]: UUID (...)
   ```

4. **Name + ID** (Robustness):
   ```
   PrincipalCount: 2
   Principal[0]: USER_ID (UID 1000)
   Principal[1]: USER_NAME ("johndoe")
   ```

**Benefits**:
- **No duplication**: Single ACE with multiple IDs instead of multiple ACEs
- **Cross-platform**: Works on POSIX, Windows, and macOS without translation
- **Robustness**: Name-based fallback if numeric ID conflicts
- **Efficiency**: Reduced ACL size, faster lookups

**Matching Logic**:
When evaluating ACL, match if **any** principal ID matches:
- If UID matches → grant/deny
- OR if SID matches → grant/deny
- OR if UUID matches → grant/deny

**Creation**:
- On Unix: Store UID, derive SID/UUID using mapping algorithms
- On Windows: Store SID, derive UID using mapping algorithms
- On macOS: Store UUID, derive UID/SID using mapping algorithms

#### 6.3.10 POSIX to SID Standard Mapping

Zoo64 **always derives SID from POSIX UID/GID** using RFC 2307 standard mapping when archiving from POSIX systems. This ensures Windows compatibility without requiring manual mapping.

**UID to SID Mapping**:

Formula: `S-1-22-1-<UID>`

Examples:
```
UID 0    →  S-1-22-1-0     (root)
UID 1000 →  S-1-22-1-1000  (user)
UID 65534→  S-1-22-1-65534 (nobody)
```

**GID to SID Mapping**:

Formula: `S-1-22-2-<GID>`

Examples:
```
GID 0    →  S-1-22-2-0     (root group)
GID 1000 →  S-1-22-2-1000  (user group)
GID 65534→  S-1-22-2-65534 (nogroup)
```

**SID Components**:
- `S`: SID prefix
- `1`: Revision (always 1)
- `22`: Authority (POSIX/Unix mapping authority)
- `1`: RID type for users, `2` for groups
- `<UID/GID>`: Actual numeric ID

**Reverse Mapping** (SID to POSIX):
```
S-1-22-1-N  →  UID N
S-1-22-2-N  →  GID N
```

**Well-Known SID Mappings**:
```
S-1-0-0 (NULL SID)           →  UID 65534 (nobody)
S-1-1-0 (Everyone)           →  SPECIAL_PRINCIPAL EVERYONE@
S-1-5-18 (SYSTEM)            →  UID 0 (root)
S-1-5-32-544 (Administrators)→  GID 0 (root/wheel)
S-1-5-32-545 (Users)         →  GID 100 (users)
S-1-5-32-546 (Guests)        →  GID 65534 (nogroup)
```

**Implementation**:
When archiving from POSIX systems, Zoo64 **automatically adds SID** to all ACL entries:

1. Read ACL with POSIX UIDs/GIDs
2. For each UID, derive SID using S-1-22-1-<UID>
3. For each GID, derive SID using S-1-22-2-<GID>
4. Store both in PRINCIPAL_ID array
5. Set PrincipalCount to 2 (or more if UUID also added)

**Benefits**:
- **Windows compatibility**: Windows can understand ACLs from POSIX archives
- **Bidirectional**: Works POSIX→Windows and Windows→POSIX
- **Standard**: Based on RFC 2307 (LDAP for Unix)
- **No configuration**: Works out-of-the-box
- **Lossless**: Round-trip preserves both UID and SID

**Example ACL Entry**:
```c
// POSIX file owned by UID 1000, with read/write for UID 1001
ACL_ENTRY {
  ACEType: ACCESS_ALLOWED
  Permissions: READ_DATA | WRITE_DATA
  PrincipalCount: 2
  Principal[0]: USER_ID, Length=4, Data=[1001 (uint32)]
  Principal[1]: SID, Length=12, Data=[S-1-22-1-1001]
}
```

When extracted on Windows, the SID `S-1-22-1-1001` is used directly. When extracted on Linux, UID `1001` is used.

### 6.4 Hard Link Support

Zoo64 supports hard links for both files and directories (like HFS+).

```c
typedef struct _ZOO64_HARDLINK {
  UINT64  InodeNumber;        /// Inode number (for grouping hard links)
  UINT64  DeviceId;           /// Device ID (for uniqueness)
  UINT16  TargetPathLength;   /// Length of target path
  UINT32  Flags;              /// Hard link flags
  /// Followed by UTF-8 target path (first occurrence of this inode in archive)
} ZOO64_HARDLINK;
#pragma pack(pop)
```

#### Hard Link Flags

```
0x00000001: DIRECTORY_HARDLINK  /// Hard link to directory (HFS+, APFS)
0x00000002: CROSS_VOLUME        /// Cross-volume hard link (rare)
0x00000004: PRESERVED_ON_COPY   /// Preserve link on copy
```

**Note on Directory Hard Links**:
- Most Unix systems don't support directory hard links (to prevent cycles)
- HFS+ (Mac OS X) and APFS support directory hard links
- When archiving directory hard links:
  1. First occurrence stores full directory contents
  2. Subsequent hard links reference first occurrence via inode/device
  3. On restoration to non-supporting filesystems, create separate copies

**Deduplication Strategy**:
Files/directories with same inode+device are stored once, with multiple path entries pointing to the same data. This saves space while preserving link semantics.

### 6.5 Symbolic Link Support

```c
typedef struct _ZOO64_SYMLINK {
  UINT16  TargetPathLength;   /// Length of target path
  UINT32  Flags;              /// Symlink flags
  /// Followed by UTF-8 target path
} ZOO64_SYMLINK;
#pragma pack(pop)
```

#### Symbolic Link Flags

```
0x00000001: ABSOLUTE_PATH    /// Absolute path (vs relative)
0x00000002: DIRECTORY_TARGET // Target is a directory
0x00000003: BROKEN_LINK      /// Target doesn't exist
0x00000004: PRESERVED_ON_COPY // Preserve symlink on copy
```

### 6.6 Extended Attributes Format

```c
typedef struct _ZOO64_XATTR {
  UINT16  NameLength;         /// Length of attribute name
  UINT32  ValueLength;        /// Length of attribute value
  UINT16  Namespace;          /// Namespace (user, system, security, trusted)
  /// Followed by:
  ///   [NameLength bytes: UTF-8 name]
  ///   [ValueLength bytes: binary value]
} ZOO64_XATTR;
#pragma pack(pop)
0x0008: ACL_GROUP           /// Named group
0x0010: ACL_MASK            /// Maximum permissions
0x0020: ACL_OTHER           /// Other permissions
```

#### 6.3.3 NFS4 ACL Format

NFSv4 ACLs (RFC 7530) - used on Solaris, FreeBSD, NFSv4 exports

```c
typedef struct _ZOO64_NFS4_ACL_ENTRY {
  UINT32  Type;               /// ALLOW, DENY, AUDIT, ALARM
  UINT32  Flags;              /// Inheritance and other flags
  UINT32  AccessMask;         /// Permission bits
  UINT16  WhoType;            /// OWNER@, GROUP@, EVERYONE@, or named
  UINT16  WhoLength;          /// Length of who string (0 for special)
  /// Followed by:
  ///   [WhoLength bytes: UTF-8 username/group] (if WhoType is named)
} ZOO64_NFS4_ACL_ENTRY;
#pragma pack(pop)
```

NFS4 ACE types:
```
0x00000000: ACCESS_ALLOWED_ACE_TYPE
0x00000001: ACCESS_DENIED_ACE_TYPE
0x00000002: SYSTEM_AUDIT_ACE_TYPE
0x00000003: SYSTEM_ALARM_ACE_TYPE
```

NFS4 ACE flags:
```
0x00000001: FILE_INHERIT_ACE
0x00000002: DIRECTORY_INHERIT_ACE
0x00000004: NO_PROPAGATE_INHERIT_ACE
0x00000008: INHERIT_ONLY_ACE
0x00000010: SUCCESSFUL_ACCESS_ACE_FLAG
0x00000020: FAILED_ACCESS_ACE_FLAG
0x00000040: IDENTIFIER_GROUP
0x00000080: INHERITED_ACE
```

NFS4 access mask:
```
0x00000001: READ_DATA / LIST_DIRECTORY
0x00000002: WRITE_DATA / ADD_FILE
0x00000004: APPEND_DATA / ADD_SUBDIRECTORY
0x00000008: READ_NAMED_ATTRS
0x00000010: WRITE_NAMED_ATTRS
0x00000020: EXECUTE / TRAVERSE
0x00000040: DELETE_CHILD
0x00000080: READ_ATTRIBUTES
0x00000100: WRITE_ATTRIBUTES
0x00010000: DELETE
0x00020000: READ_ACL
0x00040000: WRITE_ACL
0x00080000: WRITE_OWNER
0x00100000: SYNCHRONIZE
```

NFS4 special identities (WhoType):
```
0x0001: OWNER@              /// File owner
0x0002: GROUP@              /// File group
0x0003: EVERYONE@           /// All users
0x0004: NAMED_USER          /// Specific user (WhoLength > 0)
0x0005: NAMED_GROUP         /// Specific group (WhoLength > 0)
```

#### 6.3.4 NT ACL Format

Windows NT ACLs with Security Identifiers (SIDs)

```c
typedef struct _ZOO64_NT_ACL_ENTRY {
  UINT32  Type;               /// ACCESS_ALLOWED, ACCESS_DENIED, AUDIT, etc.
  UINT32  Flags;              /// Inheritance flags
  UINT32  AccessMask;         /// Permission bits
  UINT16  SIDLength;          /// Length of SID
  /// Followed by:
  ///   [SIDLength bytes: binary SID structure]
} ZOO64_NT_ACL_ENTRY;
#pragma pack(pop)
```

NT ACE types:
```
0x00: ACCESS_ALLOWED_ACE_TYPE
0x01: ACCESS_DENIED_ACE_TYPE
0x02: SYSTEM_AUDIT_ACE_TYPE
0x03: SYSTEM_ALARM_ACE_TYPE (reserved)
0x04: ACCESS_ALLOWED_COMPOUND_ACE_TYPE (reserved)
0x05: ACCESS_ALLOWED_OBJECT_ACE_TYPE
0x06: ACCESS_DENIED_OBJECT_ACE_TYPE
0x07: SYSTEM_AUDIT_OBJECT_ACE_TYPE
0x08: SYSTEM_ALARM_OBJECT_ACE_TYPE (reserved)
```

NT ACE flags:
```
0x01: OBJECT_INHERIT_ACE
0x02: CONTAINER_INHERIT_ACE
0x04: NO_PROPAGATE_INHERIT_ACE
0x08: INHERIT_ONLY_ACE
0x10: INHERITED_ACE
0x40: SUCCESSFUL_ACCESS_ACE_FLAG
0x80: FAILED_ACCESS_ACE_FLAG
```

NT access mask (generic):
```
0x00000001: FILE_READ_DATA
0x00000002: FILE_WRITE_DATA
0x00000004: FILE_APPEND_DATA
0x00000008: FILE_READ_EA
0x00000010: FILE_WRITE_EA
0x00000020: FILE_EXECUTE
0x00000040: FILE_DELETE_CHILD
0x00000080: FILE_READ_ATTRIBUTES
0x00000100: FILE_WRITE_ATTRIBUTES
0x00010000: DELETE
0x00020000: READ_CONTROL
0x00040000: WRITE_DAC
0x00080000: WRITE_OWNER
0x00100000: SYNCHRONIZE
0x01000000: ACCESS_SYSTEM_SECURITY
0x10000000: GENERIC_ALL
0x20000000: GENERIC_EXECUTE
0x40000000: GENERIC_WRITE
0x80000000: GENERIC_READ
```

NT SID format (stored as-is):
```c
#pragma pack(push, 1)
typedef struct _NT_SID {
  UINT8   Revision;           /// Always 1
  UINT8   SubAuthorityCount;  /// Number of sub-authorities (1-15)
  UINT8   Authority[6];       /// 48-bit authority value
  UINT32  SubAuthority[];     /// Variable number of 32-bit values
} NT_SID;
#pragma pack(pop)
```

#### 6.3.5 macOS ACL Format

macOS extended ACLs (based on NFSv4 with macOS extensions)

```c
typedef struct _ZOO64_MACOS_ACL_ENTRY {
  UINT8   UUID[16];           /// User/group UUID (128-bit)
  UINT32  Type;               /// ALLOW, DENY
  UINT32  Flags;              /// Inheritance flags
  UINT32  Permissions;        /// Permission bits
  UINT32  Reserved;           /// Reserved for future use
} ZOO64_MACOS_ACL_ENTRY;
#pragma pack(pop)
```

macOS ACE types:
```
0x00000000: KAUTH_ACE_PERMIT
0x00000001: KAUTH_ACE_DENY
```

macOS ACE flags:
```
0x00000001: KAUTH_ACE_FILE_INHERIT
0x00000002: KAUTH_ACE_DIRECTORY_INHERIT
0x00000004: KAUTH_ACE_LIMIT_INHERIT
0x00000008: KAUTH_ACE_ONLY_INHERIT
0x00000010: KAUTH_ACE_SUCCESS
0x00000020: KAUTH_ACE_FAILURE
0x00000040: KAUTH_ACE_INHERITED
```

macOS permissions:
```
0x00000001: KAUTH_VNODE_READ_DATA
0x00000002: KAUTH_VNODE_LIST_DIRECTORY
0x00000004: KAUTH_VNODE_WRITE_DATA
0x00000008: KAUTH_VNODE_ADD_FILE
0x00000010: KAUTH_VNODE_EXECUTE
0x00000020: KAUTH_VNODE_SEARCH
0x00000040: KAUTH_VNODE_DELETE
0x00000080: KAUTH_VNODE_APPEND_DATA
0x00000100: KAUTH_VNODE_ADD_SUBDIRECTORY
0x00000200: KAUTH_VNODE_DELETE_CHILD
0x00000400: KAUTH_VNODE_READ_ATTRIBUTES
0x00000800: KAUTH_VNODE_WRITE_ATTRIBUTES
0x00001000: KAUTH_VNODE_READ_EXTATTRIBUTES
0x00002000: KAUTH_VNODE_WRITE_EXTATTRIBUTES
0x00004000: KAUTH_VNODE_READ_SECURITY
0x00008000: KAUTH_VNODE_WRITE_SECURITY
0x00010000: KAUTH_VNODE_TAKE_OWNERSHIP
0x00020000: KAUTH_VNODE_SYNCHRONIZE
0x00040000: KAUTH_VNODE_LINKTARGET
0x00080000: KAUTH_VNODE_CHECKIMMUTABLE
```

#### 6.3.6 OpenVMS ACL Format

OpenVMS uses both UIC-based (User Identification Code) and identifier-based ACLs.

```c
typedef struct _ZOO64_VMS_ACL_ENTRY {
  UINT32  ACEType;            /// ACE type (UIC, identifier, default)
  UINT32  AccessMask;         /// Permission bits
  UINT32  Flags;              /// ACE flags (protected, hidden, etc.)
  UINT16  IdentifierType;     /// UIC, general identifier, or facility
  UINT16  IdentifierLength;   /// Length of identifier name
  UINT32  UIC;                /// User Identification Code (if UIC type)
  /// Followed by identifier name (if named identifier)
} ZOO64_VMS_ACL_ENTRY;
#pragma pack(pop)
```

OpenVMS ACE types:
```
0x0001: ACL$C_FILE     /// File ACL
0x0002: ACL$C_KEYID    /// Identifier-based
0x0003: ACL$C_ADDACC   /// Access mode
0x0004: ACL$C_DEFAULT  /// Default ACL
```

OpenVMS access rights:
```
0x0001: ACL$M_READ     /// Read
0x0002: ACL$M_WRITE    /// Write
0x0004: ACL$M_EXECUTE  /// Execute
0x0008: ACL$M_DELETE   /// Delete
0x0010: ACL$M_CONTROL  /// Control (change ACL)
0x0020: ACL$M_EXTEND   /// Extend file
0x0040: ACL$M_READ_ATTRIBUTES
0x0080: ACL$M_WRITE_ATTRIBUTES
```

#### 6.3.7 OS/400 ACL Format

OS/400 uses object authorities and authorization lists.

```c
typedef struct _ZOO64_OS400_ACL_ENTRY {
  char    UserProfile[10];    /// User profile name
  UINT32  ObjectAuthority;    /// Object authority bits
  UINT32  DataAuthority;      /// Data authority bits
  UINT16  AuthorizationType;  /// User, group, or *PUBLIC
  UINT16  AuthListLength;     /// Authorization list name length
  /// Followed by authorization list name (if applicable)
} ZOO64_OS400_ACL_ENTRY;
#pragma pack(pop)
```

OS/400 object authorities:
```
0x00000001: *OBJMGT    /// Object management
0x00000002: *OBJEXIST  /// Object existence
0x00000004: *OBJALTER  /// Object alter
0x00000008: *OBJREF    /// Object reference
0x00000010: *OBJOPER   /// Object operational
```

OS/400 data authorities:
```
0x00000001: *READ      /// Read
0x00000002: *ADD       /// Add
0x00000004: *UPD       /// Update
0x00000008: *DLT       /// Delete
0x00000010: *EXECUTE   /// Execute
0x00000020: *AUTL      /// Authorization list management
0x00000040: *EXCLUDE   /// Exclude (deny all)
```

#### 6.3.8 MVS/RACF ACL Format

IBM MVS (z/OS) uses RACF (Resource Access Control Facility) for security.

```c
typedef struct _ZOO64_RACF_ACL_ENTRY {
  char    UserID[8];          /// RACF user ID
  char    GroupID[8];         /// RACF group ID
  UINT32  AccessLevel;        /// Access level (NONE, READ, UPDATE, CONTROL, ALTER)
  UINT32  AccessType;         /// Universal, conditional, or group
  UINT32  Flags;              /// RACF flags (WARN, ERASE, etc.)
  UINT8   SecurityLevel;      /// Security level (0-255)
  UINT8   SecurityCategories[16]; // Security categories bitmap
} ZOO64_RACF_ACL_ENTRY;
#pragma pack(pop)
```

RACF access levels:
```
0x00: NONE     /// No access
0x01: READ     /// Read access
0x02: UPDATE   /// Read and write
0x03: CONTROL  /// Read, write, and change permissions
0x04: ALTER    /// Full control including delete
```

RACF access types:
```
0x0001: UNIVERSAL   /// Applies to all
0x0002: CONDITIONAL // Based on conditions
0x0004: GROUP       /// Group-based
```

#### 6.3.9 Netware ACL Format

Novell Netware uses Trustee Rights and Inherited Rights Filters.

```c
typedef struct _ZOO64_NETWARE_ACL_ENTRY {
  UINT32  ObjectID;           /// Netware object ID
  UINT16  ObjectType;         /// User, group, or organizational role
  UINT16  TrusteeRights;      /// Trustee rights bitmap
  UINT16  InheritedRightsFilter; // IRF bitmap
  char    TrusteeName[48];    /// Trustee name (NDS format)
} ZOO64_NETWARE_ACL_ENTRY;
#pragma pack(pop)
```

Netware trustee rights:
```
0x0001: SUPERVISOR  /// [S] All rights
0x0002: READ        /// [R] Read files
0x0004: WRITE       /// [W] Write files
0x0008: CREATE      /// [C] Create files
0x0010: ERASE       /// [E] Delete files
0x0020: MODIFY      /// [M] Modify file attributes
0x0040: FILESCAN    /// [F] See files in directory
0x0080: ACCESSCTRL  /// [A] Change trustee rights
```

#### 6.3.10 Banyan VINES ACL Format

Banyan VINES uses StreetTalk directory services for permissions.

```c
typedef struct _ZOO64_VINES_ACL_ENTRY {
  char    StreetTalkName[256]; // Full StreetTalk name (item@group@organization)
  UINT32  Rights;             /// Access rights bitmap
  UINT16  EntryType;          /// User, group, or list
  UINT16  Flags;              /// Entry flags
} ZOO64_VINES_ACL_ENTRY;
#pragma pack(pop)
```

VINES access rights:
```
0x00000001: READ        /// Read data
0x00000002: WRITE       /// Write data
0x00000004: EXECUTE     /// Execute
0x00000008: DELETE      /// Delete
0x00000010: CREATE      /// Create
0x00000020: RENAME      /// Rename
0x00000040: ATTRIBUTES  /// Change attributes
0x00000080: SECURITY    /// Change security
0x00000100: OWNER       /// Ownership rights
```

#### 6.3.11 AFS ACL Format

Andrew File System (AFS) uses per-directory ACLs with specific rights.

```c
typedef struct _ZOO64_AFS_ACL_ENTRY {
  char    Principal[64];      /// User or group principal
  UINT32  Rights;             /// AFS rights bitmap
  UINT16  EntryType;          /// Positive or negative rights
  UINT16  Reserved;           /// Reserved
} ZOO64_AFS_ACL_ENTRY;
#pragma pack(pop)
```

AFS rights:
```
0x01: READ     /// [r] Read files
0x02: LIST     /// [l] List directory
0x04: INSERT   /// [i] Insert files
0x08: DELETE   /// [d] Delete files
0x10: WRITE    /// [w] Write files
0x20: LOCK     /// [k] Lock files
0x40: ADMIN    /// [a] Administer ACL
```

AFS entry types:
```
0x0001: POSITIVE   /// Grant rights
0x0002: NEGATIVE   /// Deny rights
```

#### 6.3.12 CODA ACL Format

CODA distributed filesystem extends AFS ACL model.

```c
typedef struct _ZOO64_CODA_ACL_ENTRY {
  char    Principal[64];      /// User or group principal
  UINT32  Rights;             /// CODA rights (extends AFS)
  UINT16  EntryType;          /// Positive or negative
  UINT16  ReplicationPolicy;  /// Replication-specific rights
} ZOO64_CODA_ACL_ENTRY;
#pragma pack(pop)
```

CODA rights (extends AFS):
```
0x01: READ          /// [r] Read files
0x02: LIST          /// [l] List directory
0x04: INSERT        /// [i] Insert files
0x08: DELETE        /// [d] Delete files
0x10: WRITE         /// [w] Write files
0x20: LOCK          /// [k] Lock files
0x40: ADMIN         /// [a] Administer ACL
0x80: REPLICATE     /// Control replication
```

#### 6.3.13 Multiple ACL Storage

When a file has ACLs from multiple systems (e.g., during cross-platform archival), store them as separate ACL chunks or as multiple ACL headers within a single ACL chunk:

```
[ZOO64_METADATA_CHUNK: type=0x0001 (ACL)]
  [ZOO64_ACL_HEADER: type=NFS4]
    [NFS4 ACL entries...]
  [ZOO64_ACL_HEADER: type=NT]
    [NT ACL entries...]
  [ZOO64_ACL_HEADER: type=OpenVMS]
    [OpenVMS ACL entries...]
```

This allows perfect preservation and restoration of ACLs regardless of source platform.

### 6.4 Extended Attributes Format

```c
typedef struct _ZOO64_XATTR {
  UINT16  NameLength;         /// Length of attribute name
  UINT32  ValueLength;        /// Length of attribute value
  UINT16  Namespace;          /// Namespace (user, system, security, trusted)
  /// Followed by:
  ///   [NameLength bytes: UTF-8 name]
  ///   [ValueLength bytes: binary value]
} ZOO64_XATTR;
#pragma pack(pop)
```

### 6.5 Alternate Data Streams Format

```c
typedef struct _ZOO64_ADS {
  UINT16  StreamNameLength;   /// Length of stream name
  UINT64  StreamSize;         /// Size of stream data
  UINT32  StreamFlags;        /// Stream flags
  /// Followed by:
  ///   [StreamNameLength bytes: UTF-8 stream name]
  ///   [StreamSize bytes: stream data]
} ZOO64_ADS;
#pragma pack(pop)
```

### 6.6 File-Level YAML Metadata Format

File-level YAML metadata provides flexible, extensible metadata for individual files.

```c
typedef struct _ZOO64_FILE_YAML {
  UINT32  YamlSize;           /// Size of YAML data
  UINT32  Flags;              /// YAML flags (compressed, validated, etc.)
  /// Followed by YamlSize bytes of UTF-8 YAML
} ZOO64_FILE_YAML;
#pragma pack(pop)
```

#### File YAML Examples

```yaml
# File-specific metadata
file:
  original_path: "/usr/local/bin/myapp"
  purpose: "Main application binary"
  version: "2.1.4"

# Build information
build:
  compiler: "gcc 11.2.0"
  flags: "-O2 -Wall"
  date: "2024-10-31"

# Custom application data
tags:
  - executable
  - production

checksums:
  md5: "abc123..."
  sha1: "def456..."
```

### 6.6a Extended Timestamps Format

For platforms that support additional timestamps beyond the standard four (birth, modification, access, change), or to store higher-precision timestamps than NTP extended format provides.

```c
typedef struct _ZOO64_EXTENDED_TIMESTAMPS {
  UINT64  BirthTime;          /// File birth/creation time (NTP extended format)
  UINT64  ModificationTime;   /// Data modification time (NTP extended format)
  UINT64  AccessTime;         /// Last access time (NTP extended format)
  UINT64  ChangeTime;         /// Metadata change time (NTP extended format)
  UINT64  BackupTime;         /// Last backup time (NTP extended format)
  UINT64  ArchivedTime;       /// Time archived (NTP extended format)
  UINT32  Flags;              /// Timestamp flags
  UINT32  Reserved;           /// Reserved for future use
} ZOO64_EXTENDED_TIMESTAMPS;
#pragma pack(pop)
```

**Note**: Extended timestamps chunk is optional and used only when:
1. Additional timestamps (backup, archived) are present
2. Platform has timestamps not covered by the file header
3. Need to store original timestamps alongside normalized timestamps

Standard file header already contains the four primary timestamps (birth, modification, access, change) in NTP extended format. This metadata chunk is for edge cases requiring additional timestamp metadata.

Timestamp flags:
```
Bit 0:     BirthTime valid
Bit 1:     ModificationTime valid
Bit 2:     AccessTime valid
Bit 3:     ChangeTime valid
Bit 4:     BackupTime valid
Bit 5:     ArchivedTime valid
Bit 6-31:  Reserved
```

### 6.7 macOS UUIDs Format

macOS stores UUIDs for users and groups alongside numeric IDs.

```c
typedef struct _ZOO64_MACOS_UUID {
  UINT8   UserUUID[16];       /// User UUID (128-bit)
  UINT8   GroupUUID[16];      /// Group UUID (128-bit)
  UINT32  Flags;              /// Reserved
} ZOO64_MACOS_UUID;
#pragma pack(pop)
```

### 6.8 BSD Flags Format

BSD systems use file flags for immutability, append-only, etc.

```c
typedef struct _ZOO64_BSD_FLAGS {
  UINT32  UserFlags;          /// User-settable flags
  UINT32  SystemFlags;        /// System/super-user flags
} ZOO64_BSD_FLAGS;
#pragma pack(pop)
```

#### BSD Flag Definitions

```
User Flags:
  UF_NODUMP      0x00000001  /// Do not dump file
  UF_IMMUTABLE   0x00000002  /// File may not be changed
  UF_APPEND      0x00000004  /// Writes to file may only append
  UF_OPAQUE      0x00000008  /// Directory is opaque (union)
  UF_HIDDEN      0x00008000  /// File is hidden (macOS)

System Flags:
  SF_ARCHIVED    0x00010000  /// File is archived
  SF_IMMUTABLE   0x00020000  /// File may not be changed
  SF_APPEND      0x00040000  /// Writes to file may only append
```

### 6.9 Linux Flags Format

Linux file attributes (chattr/lsattr).

```c
typedef struct _ZOO64_LINUX_FLAGS {
  UINT32  Flags;              /// Linux file attributes
  UINT32  Version;            /// File version (for ext2/3/4)
} ZOO64_LINUX_FLAGS;
#pragma pack(pop)
```

#### Linux Flag Definitions

```
FS_SECRM_FL        0x00000001  /// Secure deletion
FS_UNRM_FL         0x00000002  /// Undelete
FS_COMPR_FL        0x00000004  /// Compress file
FS_SYNC_FL         0x00000008  /// Synchronous updates
FS_IMMUTABLE_FL    0x00000010  /// Immutable file
FS_APPEND_FL       0x00000020  /// Append only
FS_NODUMP_FL       0x00000040  /// Do not dump file
FS_NOATIME_FL      0x00000080  /// Do not update atime
FS_NOCOW_FL        0x00800000  /// No copy-on-write (Btrfs)
```

### 6.10 Windows Attributes Format

Extended Windows file attributes.

```c
typedef struct _ZOO64_WINDOWS_ATTR {
  UINT32  FileAttributes;     /// Windows file attributes
  UINT32  ReparseTag;         /// Reparse point tag (if applicable)
  UINT32  EaSize;             /// Extended attributes size
  /// Followed by EA data if EaSize > 0
} ZOO64_WINDOWS_ATTR;
#pragma pack(pop)
```

#### Windows Attribute Definitions

```
FILE_ATTRIBUTE_READONLY             0x00000001
FILE_ATTRIBUTE_HIDDEN               0x00000002
FILE_ATTRIBUTE_SYSTEM               0x00000004
FILE_ATTRIBUTE_DIRECTORY            0x00000010
FILE_ATTRIBUTE_ARCHIVE              0x00000020
FILE_ATTRIBUTE_DEVICE               0x00000040
FILE_ATTRIBUTE_NORMAL               0x00000080
FILE_ATTRIBUTE_TEMPORARY            0x00000100
FILE_ATTRIBUTE_SPARSE_FILE          0x00000200
FILE_ATTRIBUTE_REPARSE_POINT        0x00000400
FILE_ATTRIBUTE_COMPRESSED           0x00000800
FILE_ATTRIBUTE_OFFLINE              0x00001000
FILE_ATTRIBUTE_NOT_CONTENT_INDEXED  0x00002000
FILE_ATTRIBUTE_ENCRYPTED            0x00004000
```

### 6.11 Hard Link Target Format

Stores information about hard links. Multiple files in the archive can point to the same inode.

```c
typedef struct _ZOO64_HARDLINK {
  UINT64  InodeNumber;        /// Inode number (for grouping hard links)
  UINT64  DeviceId;           /// Device ID (for uniqueness)
  UINT16  TargetPathLength;   /// Length of target path
  /// Followed by UTF-8 target path (first occurrence of this inode in archive)
} ZOO64_HARDLINK;
#pragma pack(pop)
```

**Note**: The first file with a given inode contains the actual data. Subsequent hard links reference the first file's path and contain no data themselves.

### 6.12 Symbolic Link Target Format

Stores symbolic link target path.

```c
typedef struct _ZOO64_SYMLINK {
  UINT16  TargetPathLength;   /// Length of target path
  UINT32  Flags;              /// Symlink flags
  /// Followed by UTF-8 target path
} ZOO64_SYMLINK;
#pragma pack(pop)
```

#### Symbolic Link Flags

```
Bit 0:     Absolute path (vs relative)
Bit 1:     Directory target
Bit 2:     Broken link (target doesn't exist)
Bit 3-31:  Reserved
```

### 6.13 OpenVMS ODS-5 Attributes

OpenVMS (formerly VMS) uses the Files-11 On-Disk Structure Level 5 (ODS-5) filesystem with rich metadata.

```c
typedef struct _ZOO64_ODS5_ATTR {
  UINT16  RecordType;         /// Record format (fixed, variable, stream, etc.)
  UINT16  RecordAttributes;   /// Record attributes (Fortran CC, print, etc.)
  UINT32  RecordSize;         /// Fixed record size (or max for variable)
  UINT32  FileOrganization;   /// Sequential, relative, indexed
  UINT16  FileCharacteristics;// File characteristics bits
  UINT16  RecordFormatFlags;  /// Additional record format flags
  UINT32  HighWaterMark;      /// Highest block written
  UINT32  EndOfFileBlock;     /// End-of-file block number
  UINT16  EndOfFileOffset;    /// Byte offset in EOF block
  UINT16  FileVersion;        /// File version number (;1, ;2, etc.)
  UINT32  UserPrivileges;     /// User privilege mask
  char    FileID[16];         /// Volume-unique file identifier (FID)
} ZOO64_ODS5_ATTR;
#pragma pack(pop)
```

#### ODS-5 Record Types

```
0x0001: RFM_UDF  /// Undefined (stream)
0x0002: RFM_FIX  /// Fixed-length records
0x0003: RFM_VAR  /// Variable-length records
0x0004: RFM_VFC  /// Variable with fixed control
0x0005: RFM_STM  /// Stream (LF-terminated)
0x0006: RFM_STMLF // Stream LF
0x0007: RFM_STMCR // Stream CR
```

#### ODS-5 File Organization

```
0x0001: FAB$C_SEQ  /// Sequential
0x0002: FAB$C_REL  /// Relative
0x0003: FAB$C_IDX  /// Indexed (ISAM)
0x0004: FAB$C_HSH  /// Hashed
```

#### ODS-5 File Characteristics

```
0x0001: FCH$V_NOBACKUP    /// Don't backup
0x0002: FCH$V_WRITECHECK  /// Verify all writes
0x0004: FCH$V_READCHECK   /// Verify all reads
0x0008: FCH$V_CONTIG      /// Contiguous allocation
0x0010: FCH$V_LOCKED      /// File locked
0x0020: FCH$V_CONTIGB     /// Contiguous best try
0x0040: FCH$V_SPOOL       /// Spool file (intermediate)
0x0080: FCH$V_DIRECTORY   /// Directory file
0x0100: FCH$V_BADBLOCK    /// Bad block processing
0x0200: FCH$V_MARKDEL     /// Mark for delete
0x0400: FCH$V_NOCHARGE    /// Don't charge quota
0x0800: FCH$V_ERASE       /// Erase on delete
```

### 6.14 z/OS Dataset Attributes

IBM z/OS (formerly OS/390, MVS) mainframe filesystem metadata for datasets.

```c
typedef struct _ZOO64_ZOS_ATTR {
  char    DatasetName[44];    /// Fully qualified dataset name (DSNAME)
  UINT8   DatasetOrganization;// DSORG (PS, PO, DA, IS, VS)
  UINT8   RecordFormat;       /// RECFM (F, FB, V, VB, U, etc.)
  UINT16  LogicalRecordLength;// LRECL
  UINT32  BlockSize;          /// BLKSIZE
  UINT16  PrimarySpace;       /// Primary space allocation (tracks/cylinders/blocks)
  UINT16  SecondarySpace;     /// Secondary space allocation
  UINT8   SpaceUnit;          /// Space unit (tracks, cylinders, blocks, etc.)
  UINT8   DirectoryBlocks;    /// Directory blocks (for PDS)
  char    DataClass[8];       /// SMS data class
  char    StorageClass[8];    /// SMS storage class
  char    ManagementClass[8]; // SMS management class
  char    VolSer[6];          /// Volume serial number
  UINT16  DatasetType;        /// PDS, PDSE, HFS, zFS
  UINT32  Flags;              /// Various dataset flags
} ZOO64_ZOS_ATTR;
#pragma pack(pop)
```

#### z/OS Dataset Organization (DSORG)

```
0x01: PS   /// Physical Sequential
0x02: PO   /// Partitioned (PDS)
0x04: DA   /// Direct Access
0x08: IS   /// Indexed Sequential (ISAM)
0x10: VS   /// VSAM
0x20: PSU  /// Unmovable PS
0x40: POU  /// Unmovable PO
```

#### z/OS Record Format (RECFM)

```
0x01: F    /// Fixed
0x02: FB   /// Fixed Blocked
0x03: V    /// Variable
0x04: VB   /// Variable Blocked
0x05: U    /// Undefined
0x06: FBA  /// Fixed Blocked ASCII
0x07: VBA  /// Variable Blocked ASCII
0x08: FBM  /// Fixed Blocked Machine code
0x09: VBM  /// Variable Blocked Machine code
```

#### z/OS Space Units

```
0x01: TRK  /// Tracks
0x02: CYL  /// Cylinders
0x03: BLK  /// Blocks
0x04: KB   /// Kilobytes
0x05: MB   /// Megabytes
```

#### z/OS Dataset Types

```
0x0001: PDS   /// Partitioned Dataset
0x0002: PDSE  /// Partitioned Dataset Extended
0x0004: HFS   /// Hierarchical File System
0x0008: ZFS   /// zSeries File System
0x0010: VSAM  /// Virtual Storage Access Method
0x0020: SEQ   /// Sequential
```

### 6.15 OS/400 (IBM i) Attributes

IBM i (formerly OS/400, AS/400) integrated filesystem metadata.

```c
typedef struct _ZOO64_OS400_ATTR {
  char    Library[10];        /// Library name
  char    Object[10];         /// Object name
  char    Member[10];         /// Member name (for source/data files)
  char    ObjectType[10];     /// Object type (*FILE, *PGM, *DTAARA, etc.)
  char    SourceType[10];     /// Source type (RPG, CLP, DSPF, etc.)
  UINT32  CCSID;              /// Coded Character Set ID
  UINT8   FileType;           /// Physical, logical, source, data
  UINT8   FileOrganization;   /// Sequential, keyed, stream
  UINT16  RecordLength;       /// Record length
  UINT32  MemberCount;        /// Number of members (for multi-member files)
  char    TextDescription[50];// Object text description
  UINT64  CreateTimestamp;    /// Create timestamp (NTP extended format)
  UINT64  ChangeTimestamp;    /// Last change timestamp (NTP extended format)
  char    OwnerProfile[10];   /// Owner user profile
  char    Authority[10];      /// Primary group authority
  UINT32  Flags;              /// Various OS/400 flags
} ZOO64_OS400_ATTR;
#pragma pack(pop)
```

#### OS/400 Object Types (common)

```
*FILE      /// Database file
*PGM       /// Program
*DTAARA    /// Data area
*DTAQ      /// Data queue
*USRPRF    /// User profile
*MSGQ      /// Message queue
*OUTQ      /// Output queue
*JOBQ      /// Job queue
*LIB       /// Library
*CMD       /// Command
*MENU      /// Menu
*PNLGRP    /// Panel group
*QRYDFN    /// Query definition
```

#### OS/400 File Types

```
0x01: PF   /// Physical file
0x02: LF   /// Logical file
0x03: DSPF // Display file
0x04: PRTF // Printer file
0x05: SAVF // Save file
0x06: TAPF // Tape file
0x07: SRC  /// Source physical file
0x08: DATA // Data physical file
```

#### OS/400 CCSID (common values)

```
Single-Byte EBCDIC:
  37:    EBCDIC US/Canada
  273:   EBCDIC Germany
  277:   EBCDIC Denmark/Norway
  278:   EBCDIC Sweden/Finland
  280:   EBCDIC Italy
  284:   EBCDIC Spain
  285:   EBCDIC UK
  297:   EBCDIC France
  500:   EBCDIC International

Mixed EBCDIC (DBCS):
  930:   Japanese EBCDIC/Katakana
  933:   Korean EBCDIC
  935:   Simplified Chinese EBCDIC
  937:   Traditional Chinese EBCDIC
  939:   Japanese EBCDIC/Latin

ASCII/PC:
  819:   ASCII ISO 8859-1
  850:   PC Latin-1
  858:   PC Latin-1 + Euro

Unicode (UTF-8):
  1208:  UTF-8
  1252:  Windows Latin-1 (often used with UTF-8)

Unicode (UTF-16):
  1200:  UTF-16 (big-endian)
  1201:  UTF-16 (little-endian)
  13488: UCS-2 (IBM i native)

Unicode (UTF-32):
  1232:  UTF-32 (big-endian)
  1233:  UTF-32 (little-endian)

UTF-EBCDIC (Unicode in EBCDIC encoding):
  4133:  UTF-EBCDIC (primary)
  5039:  Japanese UTF-EBCDIC
```

**Note on UTF-EBCDIC**: IBM i systems often use UTF-EBCDIC (CCSID 4133) for Unicode support while maintaining EBCDIC compatibility. When archiving from IBM i:
- **Archive format**: File paths and text stored as UTF-8 (Zoo64 standard)
- **CCSID preserved**: Original CCSID (including UTF-EBCDIC) stored in OS/400 attributes
- **Round-trip**: Converting UTF-EBCDIC → UTF-8 (archive) → UTF-EBCDIC (restore) preserves data

UTF-EBCDIC advantages on IBM i:
- Compatible with existing EBCDIC tooling
- Single-byte ASCII/EBCDIC characters unchanged (0x00-0xFF)
- Full Unicode support via multi-byte sequences
- Native support in IBM i V5R4+

### 6.16 Lisa Office System Attributes

Apple Lisa Office System (1983-1985) had a sophisticated document-based filesystem.

```c
typedef struct _ZOO64_LISA_ATTR {
  char    DocumentType[32];   /// Document type (LisaWrite, LisaCalc, etc.)
  UINT32  PrivateData[4];     /// Application private data
  char    DocumentName[32];   /// Original document name (Lisa limit: 31 chars)
  UINT32  PaperType;          /// Paper size (US Letter, A4, Legal, etc.)
  UINT16  Version;            /// Document version number
  UINT16  Edition;            /// Document edition number
  UINT32  IconResourceID;     /// Resource ID of document icon
  UINT32  Password;           /// Password hash (if password protected)
  UINT32  Flags;              /// Lisa-specific flags
  UINT64  CreationDate;       /// Lisa creation date (NTP extended format)
  UINT64  ModificationDate;   /// Lisa modification date (NTP extended format)
  char    Application[32];    /// Creating application name
  char    StationeryPad[32];  /// Stationery pad template (if any)
} ZOO64_LISA_ATTR;
#pragma pack(pop)
```

#### Lisa Document Types (common)

```
"LisaWrite/DOCUMENT"   /// LisaWrite word processor document
"LisaCalc/WORKSHEET"   /// LisaCalc spreadsheet
"LisaDraw/DRAWING"     /// LisaDraw graphics document
"LisaGraph/GRAPH"      /// LisaGraph business graphics
"LisaProject/PROJECT"  /// LisaProject project management
"LisaList/DATABASE"    /// LisaList database
"LisaTerminal/SETUP"   /// LisaTerminal configuration
```

#### Lisa Paper Types

```
0x0001: US Letter (8.5" x 11")
0x0002: US Legal (8.5" x 14")
0x0003: A4 (210mm x 297mm)
0x0004: B5 (176mm x 250mm)
0x0005: Fanfold (14.875" x 11")
```

#### Lisa Document Flags

```
0x00000001: STATIONERY        /// Document is stationery
0x00000002: PASSWORD_PROTECTED // Password required
0x00000004: COPY_PROTECTED     /// Copy protection enabled
0x00000008: PRINT_PROTECTED    /// Print protection enabled
0x00000010: SHARED_DOCUMENT    /// Multi-user shared document
0x00000020: AUTO_SAVE          /// Auto-save enabled
```

**Note**: Lisa Office System metadata is primarily of historical interest for archival purposes. Modern implementations should store Lisa documents with full metadata preservation for digital archaeology and computing history research.

### 6.17 UNIVAC 2200 Attributes

UNIVAC 2200 series (1100/2200) mainframe filesystem metadata.

```c
typedef struct _ZOO64_UNIVAC_ATTR {
  char    FileName[12];       /// File name (12 characters max)
  char    Qualifier[12];      /// File qualifier
  char    ProjectID[12];      /// Project identifier
  UINT16  FileType;           /// File type (program, data, etc.)
  UINT16  FileOrganization;   /// Sequential, random, indexed
  UINT32  GranuleSize;        /// Granule size (allocation unit)
  UINT32  MaxGranules;        /// Maximum granules allocated
  UINT32  HighGranule;        /// Highest granule used
  UINT16  RecordSize;         /// Logical record size (words)
  UINT8   WordSize;           /// Word size (36 or 9-bit bytes)
  UINT8   FileClass;          /// Removable, cataloged, etc.
  UINT32  CatalogInfo;        /// Catalog information
  UINT16  SecurityLevel;      /// Security classification
  UINT32  Flags;              /// File characteristic flags
} ZOO64_UNIVAC_ATTR;
#pragma pack(pop)
```

UNIVAC file types:
```
0x01: PROGRAM    /// Executable program
0x02: DATA       /// Data file
0x03: LIBRARY    /// Object library
0x04: SOURCE     /// Source code
0x05: PRINT      /// Print file
0x06: PUNCH      /// Card punch format
```

UNIVAC file organization:
```
0x01: SEQUENTIAL // Sequential access
0x02: RANDOM     /// Random access
0x03: INDEXED    /// Indexed sequential
```

### 6.18 PDP-10 Attributes

PDP-10 systems (TENEX, ITS, TOPS-10, TOPS-20) filesystem metadata.

```c
typedef struct _ZOO64_PDP10_ATTR {
  char    FileName[40];       /// File name (6-char name + extension)
  UINT32  ProtectionCode;     /// Protection code (octal format)
  UINT16  AccountNumber;      /// Account number
  char    Author[40];         /// Author/creator name
  UINT64  CreationDate;       /// Creation date (NTP extended format)
  UINT64  WriteDate;          /// Last write date (NTP extended format)
  UINT64  ReadDate;           /// Last read date (NTP extended format)
  UINT32  ByteSize;           /// Byte size (7, 8, 9, 18, 36 bits)
  UINT32  PageCount;          /// Number of pages (512-word pages)
  UINT16  FileMode;           /// File mode (ASCII, binary, etc.)
  UINT16  System;             /// System type (TENEX, ITS, TOPS-10, TOPS-20)
  UINT32  Generation;         /// Generation number (version)
  UINT32  Flags;              /// System-specific flags
} ZOO64_PDP10_ATTR;
#pragma pack(pop)
```

PDP-10 protection codes (octal, format: <owner><group><world>):
```
Each digit: 0-7 octal
Bit 0 (1): Read
Bit 1 (2): Write
Bit 2 (4): Execute
Example: 0644 = owner:rw, group:r, world:r
```

PDP-10 systems:
```
0x01: TENEX      /// TENEX operating system
0x02: ITS        /// Incompatible Timesharing System
0x03: TOPS10     /// TOPS-10
0x04: TOPS20     /// TOPS-20
```

### 6.19 Classic Mac OS Attributes

Classic Macintosh System (System 1-9, pre-OS X) filesystem metadata.

```c
typedef struct _ZOO64_CLASSIC_MAC_ATTR {
  char    TypeCode[4];        /// File type code (e.g., 'TEXT', 'APPL')
  char    CreatorCode[4];     /// Creator application code
  UINT16  FinderFlags;        /// Finder flags
  INT16   IconPositionV;      /// Vertical icon position
  INT16   IconPositionH;      /// Horizontal icon position
  UINT16  FolderID;           /// Folder ID
  UINT32  LabelColor;         /// Label color (0-7)
  UINT16  ScriptCode;         /// Script code for name
  UINT16  ExtendedFinderFlags;// Extended Finder flags
  INT32   CommentID;          /// Comment ID (-1 if none)
  UINT32  PutAwayFolderID;    /// "Put Away" folder ID
  UINT16  IconID;             /// Custom icon ID
  UINT8   VersionMajor;       /// File version (major)
  UINT8   VersionMinor;       /// File version (minor)
} ZOO64_CLASSIC_MAC_ATTR;
#pragma pack(pop)
```

Finder flags:
```
0x0001: IS_ON_DESK       /// File is on desktop
0x0002: COLOR            /// Color (not B&W) icon
0x0004: REQUIRES_SWITCH_LAUNCH // Requires mode switch
0x0008: IS_SHARED        /// Multiple users can run simultaneously
0x0010: HAS_NO_INITS     /// Has no INIT resources
0x0020: HAS_BEEN_INITED  /// Has been initialized
0x0040: HAS_CUSTOM_ICON  /// Has custom icon
0x0080: IS_STATIONERY    /// Is stationery pad
0x0100: NAME_LOCKED      /// Name is locked
0x0200: HAS_BUNDLE       /// Has BNDL resource
0x0400: IS_INVISIBLE     /// Invisible to Finder
0x0800: IS_ALIAS         /// Is an alias file
```

Common type codes:
```
'TEXT' // Text file
'APPL' // Application
'FFIL' // Finder file
'INIT' // System extension
'cdev' // Control panel
'PICT' // Picture
'snd ' // Sound
'ttro' // TrueType font
```

### 6.20 Amiga Attributes

Commodore Amiga AmigaDOS filesystem metadata.

```c
typedef struct _ZOO64_AMIGA_ATTR {
  char    Comment[80];        /// File comment
  UINT32  ProtectionBits;     /// Protection bits (DEWD ARWED)
  UINT32  DaysFromEpoch;      /// Days since 1978-01-01
  UINT32  Minutes;            /// Minutes since midnight
  UINT32  Ticks;              /// Ticks (1/50 second)
  UINT16  FileType;           /// File, directory, link
  UINT32  ScriptBits;         /// Script execution bits
  UINT32  Flags;              /// Additional flags
} ZOO64_AMIGA_ATTR;
#pragma pack(pop)
```

Amiga protection bits (inverted logic - bit SET means permission DENIED):
```
0x00000001: DELETE      /// [D] File deletable
0x00000002: EXECUTE     /// [E] File executable
0x00000004: WRITE       /// [W] File writable
0x00000008: READ        /// [R] File readable
0x00000010: ARCHIVE     /// [A] Archive bit
0x00000020: PURE        /// [P] Re-entrant/pure
0x00000040: SCRIPT      /// [S] Script file
0x00000080: HOLD        /// [H] Hold (for multi-user)
```

### 6.21 Atari TOS/GEM Attributes

Atari ST/TT/Falcon TOS and GEM filesystem metadata.

```c
typedef struct _ZOO64_ATARI_ATTR {
  UINT8   Attributes;         /// File attributes
  UINT16  Time;               /// MS-DOS format time
  UINT16  Date;               /// MS-DOS format date
  UINT32  StartCluster;       /// Starting cluster
  char    GEMType[4];         /// GEM file type
  UINT16  GEMIcon;            /// GEM icon number
  UINT32  Flags;              /// TOS flags
} ZOO64_ATARI_ATTR;
#pragma pack(pop)
```

Atari file attributes:
```
0x01: READ_ONLY   /// Read-only file
0x02: HIDDEN      /// Hidden file
0x04: SYSTEM      /// System file
0x08: VOLUME      /// Volume label
0x10: DIRECTORY   /// Directory
0x20: ARCHIVE     /// Archive bit
```

### 6.22 Acorn RISC OS Attributes

Acorn Archimedes RISC OS filesystem metadata.

```c
typedef struct _ZOO64_RISC_OS_ATTR {
  UINT32  LoadAddress;        /// Load address (or filetype if bit 12 set)
  UINT32  ExecAddress;        /// Execution address (or timestamp)
  UINT32  Attributes;         /// File attributes
  UINT16  FileType;           /// File type (12-bit, 0xFFF = untyped)
  UINT64  Timestamp;          /// RISC OS timestamp (centiseconds since 1900)
  char    SpriteName[12];     /// Associated sprite name
} ZOO64_RISC_OS_ATTR;
#pragma pack(pop)
```

RISC OS attributes:
```
0x00000001: OWNER_READ
0x00000002: OWNER_WRITE
0x00000008: LOCKED
0x00000010: PUBLIC_READ
0x00000020: PUBLIC_WRITE
```

Common RISC OS file types:
```
0xFFF: Untyped/Text
0xFAE: DrawFile
0xFF9: Sprite
0xFAF: HTML
0xC85: JPEG
0xB60: PNG
0xFFD: Data
0xFEB: Obey (script)
0xDEAD: Archive
```

### 6.23 Commodore 64/128 Attributes

Commodore 64 and 128 disk filesystem metadata.

```c
typedef struct _ZOO64_C64_ATTR {
  char    PETSCIIName[16];    /// PETSCII filename
  UINT8   FileType;           /// File type (PRG, SEQ, USR, REL)
  UINT8   RecordLength;       /// Record length (REL files)
  UINT16  StartAddress;       /// Load address (PRG files)
  UINT16  BlocksUsed;         /// Number of blocks used
  UINT8   DriveNumber;        /// Drive number (0-1)
  UINT8   TrackSector[2];     /// Track and sector of first block
  UINT8   Flags;              /// File flags (locked, etc.)
  UINT8   GEOSType;           /// GEOS file type (if GEOS)
  UINT8   GEOSStructure;      /// GEOS file structure
} ZOO64_C64_ATTR;
#pragma pack(pop)
```

C64 file types:
```
0x00: DEL  /// Deleted (scratched)
0x01: SEQ  /// Sequential
0x02: PRG  /// Program
0x03: USR  /// User
0x04: REL  /// Relative (random access)
```

C64 file flags:
```
0x40: LOCKED     /// File is write-protected
0x80: CLOSED     /// File properly closed
```

### 6.24 Apple IIGS ProDOS Attributes

Apple IIGS ProDOS 16 and GS/OS filesystem metadata.

```c
typedef struct _ZOO64_PRODOS_ATTR {
  UINT8   FileType;           /// ProDOS file type
  UINT16  AuxType;            /// Auxiliary type
  UINT8   Access;             /// Access flags
  UINT16  StorageType;        /// Storage type
  UINT64  CreateTime;         /// ProDOS timestamp (NTP format)
  UINT64  ModTime;            /// ProDOS timestamp (NTP format)
  UINT32  BlocksUsed;         /// Blocks used
  UINT16  VersionCreated;     /// ProDOS version that created file
  UINT16  MinVersion;         /// Minimum ProDOS version required
  UINT32  EndOfFile;          /// EOF marker
  UINT32  OptionList;         /// GS/OS option list
} ZOO64_PRODOS_ATTR;
#pragma pack(pop)
```

ProDOS file types (common):
```
0x00: UNK  /// Unknown
0x04: TXT  /// Text file
0x06: BIN  /// Binary
0x0F: DIR  /// Directory
0x19: ADB  /// AppleWorks database
0x1A: AWP  /// AppleWorks word processor
0x1B: ASP  /// AppleWorks spreadsheet
0x2E: PRG  /// ProDOS application
0xB0: SRC  /// Source code
0xEF: PAS  /// Pascal
0xF0: CMD  /// Command file
0xFA: INT  /// Integer BASIC
0xFC: BAS  /// Applesoft BASIC
0xFF: SYS  /// System file
```

ProDOS access flags:
```
0x01: READ      /// Read enable
0x02: WRITE     /// Write enable
0x04: INVISIBLE // Invisible file
0x20: BACKUP    /// Backup needed
0x40: RENAME    /// Rename enable
0x80: DESTROY   /// Delete enable
```

### 6.25 Stratus VOS Attributes

Stratus VOS (Virtual Operating System) fault-tolerant system metadata.

```c
typedef struct _ZOO64_STRATUS_ATTR {
  char    ModuleName[32];     /// Module name
  char    Organization[32];   /// Organization name
  UINT32  FileOrganization;   /// Sequential, relative, indexed
  UINT16  RecordSize;         /// Fixed record size
  UINT16  KeySize;            /// Key size (indexed files)
  UINT32  MaxRecords;         /// Maximum records
  UINT32  ReplicationLevel;   /// Replication level (0-3)
  UINT32  IntegrityLevel;     /// Integrity level
  char    UserID[32];         /// Owner user ID
  char    GroupID[32];        /// Owner group ID
  UINT32  AccessControl;      /// Access control flags
  UINT32  Flags;              /// VOS-specific flags
} ZOO64_STRATUS_ATTR;
#pragma pack(pop)
```

Stratus replication levels:
```
0: None         /// No replication
1: Disk         /// Disk mirroring
2: CPU          /// CPU pair replication
3: Full         /// Full fault tolerance
```

### 6.26 Netware Extended Attributes

Novell Netware extended filesystem metadata (beyond ACLs).

```c
typedef struct _ZOO64_NETWARE_ATTR {
  UINT32  OwnerID;            /// Owner object ID
  UINT64  ArchiveTime;        /// Last archive time (NTP format)
  UINT64  ArchiverID;         /// Archiver object ID
  UINT64  UpdateTime;         /// Last update time (NTP format)
  UINT64  UpdaterID;          /// Updater object ID
  UINT32  Attributes;         /// File attributes
  UINT16  InheritedRightsFilter; // IRF
  UINT32  MaxSpace;           /// Maximum space
  char    PrimaryNameSpace[16]; // Primary namespace (DOS, MAC, NFS, etc.)
  char    NDSPath[256];       /// NDS (Novell Directory Services) path
  UINT32  Flags;              /// Extended flags
} ZOO64_NETWARE_ATTR;
#pragma pack(pop)
```

Netware file attributes:
```
0x00000001: READ_ONLY
0x00000002: HIDDEN
0x00000004: SYSTEM
0x00000008: EXECUTE_ONLY
0x00000010: SUBDIRECTORY
0x00000020: ARCHIVE
0x00000040: EXECUTE_CONFIRM
0x00000080: SHAREABLE
0x00000100: DONT_COMPRESS
0x00000200: COMPRESSED
0x00000400: TRANSACTIONAL
0x00000800: INDEXED
0x00001000: READ_AUDIT
0x00002000: WRITE_AUDIT
0x00004000: IMMEDIATE_COMPRESS
0x00008000: PURGE
0x00010000: RENAME_INHIBIT
0x00020000: DELETE_INHIBIT
0x00040000: COPY_INHIBIT
```

### 6.27 Banyan VINES Extended Attributes

Banyan VINES StreetTalk filesystem metadata.

```c
typedef struct _ZOO64_VINES_ATTR {
  char    StreetTalkName[256]; // Full StreetTalk name
  char    ServiceType[32];    /// Service type
  UINT32  VINESVersion;       /// VINES OS version
  char    Organization[64];   /// Organization name
  char    Group[64];          /// Group name
  char    Item[64];           /// Item name
  UINT32  Attributes;         /// File attributes
  UINT64  ReplicationStatus;  /// Replication status
  char    PrimaryServer[64];  /// Primary file server
  UINT32  Flags;              /// VINES flags
} ZOO64_VINES_ATTR;
#pragma pack(pop)
```

### 6.28 AFS (Andrew File System) Attributes

AFS distributed filesystem metadata.

```c
typedef struct _ZOO64_AFS_ATTR {
  char    CellName[256];      /// AFS cell name
  UINT32  VolumeID;           /// Volume ID
  UINT32  Vnode;              /// Vnode number
  UINT32  Uniquifier;         /// Uniquifier
  UINT32  DataVersion;        /// Data version number
  char    VolumeName[64];     /// Volume name
  UINT32  QuotaUsed;          /// Quota used
  UINT32  QuotaLimit;         /// Quota limit
  UINT32  Author;             /// Author UID
  UINT32  Owner;              /// Owner UID
  UINT64  ServerModTime;      /// Server modification time (NTP format)
  UINT32  Callbacks;          /// Callback information
  UINT32  Flags;              /// AFS flags
} ZOO64_AFS_ATTR;
#pragma pack(pop)
```

### 6.29 CODA Distributed Filesystem Attributes

CODA filesystem metadata (extends AFS).

```c
typedef struct _ZOO64_CODA_ATTR {
  char    Realm[256];         /// CODA realm name
  UINT32  VolumeID;           /// Volume ID
  UINT32  Vnode;              /// Vnode number
  UINT32  Uniquifier;         /// Uniquifier
  UINT64  DataVersion;        /// Data version (64-bit)
  char    VolumeName[64];     /// Volume name
  UINT32  ReplicationFactor;  /// Replication factor
  UINT32  ReplicaCount;       /// Number of replicas
  char    ReplicaServers[256]; // Comma-separated replica servers
  UINT32  ConflictStatus;     /// Conflict resolution status
  UINT64  VectorVersion[8];   /// Version vector (for conflicts)
  UINT32  CachePriority;      /// Client cache priority
  UINT32  Flags;              /// CODA flags
} ZOO64_CODA_ATTR;
#pragma pack(pop)
```

### 6.30 GFS (Global File System) Attributes

Clustered filesystem metadata.

```c
typedef struct _ZOO64_GFS_ATTR {
  char    ClusterName[64];    /// Cluster name
  UINT32  NodeID;             /// Node ID
  UINT64  GlockNumber;        /// Global lock number
  UINT32  LockState;          /// Lock state
  char    JournalID[32];      /// Journal identifier
  UINT64  SequenceNumber;     /// Sequence number
  UINT32  ReplicationLevel;   /// Replication level
  UINT32  StripingFactor;     /// Striping factor
  UINT32  BlockAllocation;    /// Block allocation policy
  UINT64  ExtentSize;         /// Extent size
  UINT32  Flags;              /// GFS flags
} ZOO64_GFS_ATTR;
#pragma pack(pop)
```

### 6.31 DFS (Distributed File System) Attributes

General distributed filesystem metadata.

```c
typedef struct _ZOO64_DFS_ATTR {
  char    Namespace[256];     /// DFS namespace
  char    ServerPath[512];    /// Server UNC path
  char    LinkTarget[512];    /// DFS link target
  UINT32  Timeout;            /// Client cache timeout (seconds)
  UINT32  ReferralTTL;        /// Referral time-to-live
  UINT16  TargetPriority;     /// Target priority
  UINT16  TargetRank;         /// Target ranking
  char    SiteName[64];       /// AD site name
  UINT32  LinkState;          /// Link state (online, offline)
  UINT32  Flags;              /// DFS flags
} ZOO64_DFS_ATTR;
#pragma pack(pop)
```

### 6.32 Device File Attributes

Block and character device files.

```c
typedef struct _ZOO64_DEVICE_ATTR {
  UINT8   DeviceType;         /// Block or character
  UINT32  MajorNumber;        /// Device major number
  UINT32  MinorNumber;        /// Device minor number
  char    DeviceName[64];     /// Device name (e.g., "sda", "tty0")
  UINT32  Flags;              /// Device-specific flags
} ZOO64_DEVICE_ATTR;
#pragma pack(pop)
```

Device types:
```
0x01: BLOCK_DEVICE      /// Block device (disks, etc.)
0x02: CHARACTER_DEVICE  /// Character device (terminals, etc.)
```

### 6.33 FIFO Attributes

Named pipes (FIFOs).

```c
typedef struct _ZOO64_FIFO_ATTR {
  UINT32  BufferSize;         /// Pipe buffer size
  UINT32  Readers;            /// Number of readers (snapshot)
  UINT32  Writers;            /// Number of writers (snapshot)
  UINT32  Flags;              /// FIFO flags
} ZOO64_FIFO_ATTR;
#pragma pack(pop)
```

### 6.34 Socket Attributes

Unix domain sockets.

```c
typedef struct _ZOO64_SOCKET_ATTR {
  UINT16  SocketType;         /// SOCK_STREAM, SOCK_DGRAM, SOCK_SEQPACKET
  UINT16  Protocol;           /// Protocol (usually 0 for Unix domain)
  UINT32  BufferSize;         /// Socket buffer size
  char    BoundPath[256];     /// Bound path (if applicable)
  UINT32  Flags;              /// Socket flags
} ZOO64_SOCKET_ATTR;
#pragma pack(pop)
```

Socket types:
```
0x0001: SOCK_STREAM      /// Stream socket (TCP-like)
0x0002: SOCK_DGRAM       /// Datagram socket (UDP-like)
0x0003: SOCK_SEQPACKET   /// Sequenced packet socket
0x0004: SOCK_RAW         /// Raw socket
```

### 6.35 Door Attributes

Solaris doors (inter-process communication mechanism).

```c
typedef struct _ZOO64_DOOR_ATTR {
  UINT32  ServerPID;          /// Server process ID
  UINT32  ServerProcedure;    /// Server procedure address
  char    ServiceName[64];    /// Service name
  UINT32  Attributes;         /// Door attributes
  UINT32  Flags;              /// Door flags
} ZOO64_DOOR_ATTR;
#pragma pack(pop)
```

Door attributes:
```
0x00000001: DOOR_UNREF       /// Unref notification
0x00000002: DOOR_PRIVATE     /// Private door
0x00000004: DOOR_REFUSE_DESC // Refuse descriptors
0x00000008: DOOR_NO_CANCEL   /// Don't cancel
0x00000010: DOOR_LOCAL       /// Local door only
0x00000020: DOOR_REVOKED     /// Door has been revoked
```

### 6.36 Event Port Attributes

Solaris event ports.

```c
typedef struct _ZOO64_EVENTPORT_ATTR {
  UINT32  PortID;             /// Port identifier
  UINT32  MaxEvents;          /// Maximum events
  UINT32  Flags;              /// Event port flags
} ZOO64_EVENTPORT_ATTR;
#pragma pack(pop)
```

### 6.37 Whiteout Attributes

BSD union mount whiteouts.

```c
typedef struct _ZOO64_WHITEOUT_ATTR {
  char    HiddenPath[256];    /// Path being hidden
  UINT32  UnionLayer;         /// Union filesystem layer
  UINT32  Flags;              /// Whiteout flags
} ZOO64_WHITEOUT_ATTR;
#pragma pack(pop)
```

### 6.38 Magic Symlink Attributes

Special symbolic links with extended semantics.

```c
typedef struct _ZOO64_MAGIC_SYMLINK_ATTR {
  UINT16  MagicType;          /// Type of magic symlink
  UINT16  TargetPathLength;   /// Length of target path
  UINT32  Flags;              /// Magic symlink flags
  UINT8   TargetGUID[16];     /// Target GUID (for Windows shortcuts/aliases)
  /// Followed by:
  ///   [TargetPathLength bytes: UTF-8 target path]
  ///   [Variable: Type-specific data]
} ZOO64_MAGIC_SYMLINK_ATTR;
#pragma pack(pop)
```

Magic symlink types:
```
0x0001: WINDOWS_JUNCTION     /// Windows junction point
0x0002: WINDOWS_SHORTCUT     /// Windows .lnk shortcut
0x0003: MACOS_ALIAS          /// macOS alias file
0x0004: SHELL_LINK           /// Shell link (.desktop, etc.)
0x0005: SYMBOLIC_LINK        /// Standard symbolic link
0x0006: RELATIVE_SYMLINK     /// Relative symbolic link
0x0007: REPARSE_POINT        /// Windows reparse point
0x0008: VOLUME_MOUNT_POINT   /// Volume mount point
0x0009: APP_EXEC_LINK        /// Application execution link
```

Magic symlink flags:
```
0x00000001: TARGET_IS_DIRECTORY
0x00000002: TARGET_IS_VOLUME
0x00000003: TARGET_IS_NETWORK
0x00000004: TARGET_IS_REMOVABLE
0x00000008: RUN_AS_ADMINISTRATOR
0x00000010: RUN_IN_SEPARATE_VDM
0x00000020: HAS_ICON_LOCATION
0x00000040: HAS_ARGUMENTS
0x00000080: HAS_WORKING_DIRECTORY
0x00000100: HAS_HOTKEY
0x00000200: HAS_COMMENT
```

**Windows Junction Points**:
- Uses reparse points (IO_REPARSE_TAG_MOUNT_POINT)
- Target must be absolute path
- Only for directories

**macOS Aliases**:
- Store path + volume info + inode
- Survive file moves/renames
- Store in extended attributes

**Windows Shortcuts (.lnk)**:
- Shell link format
- Can include arguments, working directory, icon
- Store GUID for target resolution

### 6.39 APFS Attributes (0x002B)

Apple File System specific attributes for macOS 10.13+.

```c
typedef struct _ZOO64_APFS_ATTR {
  UINT64  FileID;             /// APFS file identifier
  UINT64  ParentID;           /// Parent directory ID
  UINT64  CloneID;            /// Clone source ID (0 if not cloned)
  UINT32  CloneGeneration;    /// Clone generation number
  UINT32  Flags;              /// APFS-specific flags
  UINT32  EncryptionClass;    /// Encryption class (0-7)
  UINT32  Reserved;           /// Reserved for future use
  UINT8   DocumentID[16];     /// Document identifier (UUID)
} ZOO64_APFS_ATTR;
#pragma pack(pop)
```

APFS flags:
```
0x00000001: CLONED_FILE          /// File is a clone (copy-on-write)
0x00000002: PURGEABLE            /// Can be purged by system
0x00000004: COMPRESSED           /// File is compressed
0x00000008: ENCRYPTION_ENABLED   /// File is encrypted
0x00000010: FAST_DIR_SIZING      /// Directory has fast size tracking
0x00000020: ATOMIC_SAFE_SAVE     /// File uses atomic safe-save
0x00000040: HAS_SECURITY_EA      /// Has security extended attributes
0x00000080: PINNED_TO_TIER       /// Pinned to specific storage tier
```

APFS encryption classes:
```
0: PROTECTION_NONE
1: PROTECTION_COMPLETE_UNLESS_OPEN
2: PROTECTION_COMPLETE_UNTIL_FIRST_USER_AUTHENTICATION
3: PROTECTION_COMPLETE
4: PROTECTION_PROTECTED_UNLESS_OPEN (default for new files)
5-7: Reserved
```

**Copy-on-Write Clones**:
- CloneID points to source file
- Blocks shared until modified
- On extraction: Create actual clone if supported, otherwise full copy

**Snapshot Integration**:
- Files may reference snapshot versions
- Document ID tracks file across snapshots

### 6.40 ReFS Attributes (0x002C)

Windows Resilient File System (ReFS) specific attributes.

```c
typedef struct _ZOO64_REFS_ATTR {
  UINT64  FileID;             /// ReFS file identifier
  UINT64  IntegrityStreamID;  /// Integrity stream ID
  UINT32  Flags;              /// ReFS-specific flags
  UINT32  IntegrityLevel;     /// Integrity level (0-2)
  UINT64  CloneSourceID;      /// Block clone source (0 if not cloned)
  UINT32  CloneRangeCount;    /// Number of cloned ranges
  UINT32  StorageTier;        /// Storage tier (SSD/HDD)
  /// Followed by CloneRangeCount * sizeof(REFS_CLONE_RANGE) if cloned
} ZOO64_REFS_ATTR;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _REFS_CLONE_RANGE {
  UINT64  SourceOffset;       /// Offset in source file
  UINT64  TargetOffset;       /// Offset in this file
  UINT64  Length;             /// Length of cloned region
} REFS_CLONE_RANGE;
#pragma pack(pop)
```

ReFS flags:
```
0x00000001: INTEGRITY_ENABLED     /// Integrity streams enabled
0x00000002: BLOCK_CLONED          /// File has block clones
0x00000004: TIERED_STORAGE        /// Uses storage tiering
0x00000008: DEDUPLICATION_ENABLED // File is deduplicated
0x00000010: SPARSE_VDL            /// Sparse valid data length
0x00000020: SCRUBBING_ENABLED     /// Integrity scrubbing enabled
```

ReFS integrity levels:
```
0: INTEGRITY_NONE        /// No integrity checking
1: INTEGRITY_METADATA    /// Metadata only
2: INTEGRITY_FULL        /// Metadata + data checksums
```

**Block Cloning**:
- ReFS supports efficient block-level clones
- Clone ranges track shared blocks
- On extraction: Use block cloning if available

### 6.41 VxFS Attributes (0x002D)

Veritas File System (VxFS) specific attributes.

```c
typedef struct _ZOO64_VXFS_ATTR {
  UINT64  Inode;              /// VxFS inode number
  UINT32  Flags;              /// VxFS-specific flags
  UINT32  ExtentMode;         /// Extent allocation mode
  UINT64  CheckpointID;       /// Storage checkpoint ID
  UINT32  ReorgPriority;      /// Reorganization priority
  UINT32  Reserved;           /// Reserved
  char    DMAPIAttributes[64]; // DMAPI attribute string
} ZOO64_VXFS_ATTR;
#pragma pack(pop)
```

VxFS flags:
```
0x00000001: QUICK_IO_ENABLED   /// Quick I/O enabled
0x00000002: CHECKPOINTED       /// File in storage checkpoint
0x00000004: DMAPI_MANAGED      /// Managed by DMAPI
0x00000008: CACHED_IO          /// Cached I/O mode
0x00000010: DIRECT_IO          /// Direct I/O mode
0x00000020: CONCURRENT_IO      /// Concurrent I/O enabled
```

VxFS extent modes:
```
0: EXTENT_NORMAL     /// Normal extent allocation
1: EXTENT_FIXED      /// Fixed extent size
2: EXTENT_RESERVE    /// Reserved extent
3: EXTENT_THIN       /// Thin provisioned
```

### 6.42 HPFS Attributes (0x002E)

OS/2 High Performance File System (HPFS) extended attributes.

```c
typedef struct _ZOO64_HPFS_ATTR {
  UINT32  EASize;             /// Total size of extended attributes
  UINT16  EACount;            /// Number of EA entries
  UINT16  Flags;              /// HPFS flags
  /// Followed by EACount * HPFS_EA_ENTRY structures
} ZOO64_HPFS_ATTR;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _HPFS_EA_ENTRY {
  UINT8   NameLength;         /// Length of EA name
  UINT16  ValueLength;        /// Length of EA value
  UINT8   Flags;              /// EA flags
  /// Followed by:
  ///   [NameLength bytes: EA name]
  ///   [ValueLength bytes: EA value]
} HPFS_EA_ENTRY;
#pragma pack(pop)
```

HPFS EA flags:
```
0x01: EA_CRITICAL    /// Critical EA (file unusable if not preserved)
0x02: EA_NEEDEA      /// File needs EAs to function
0x04: EA_BINARY      /// Binary data (not text)
```

**OS/2 Extended Attributes**:
- Store application-specific metadata
- Critical EAs must be preserved for file to work
- Common EAs: `.TYPE`, `.ICON`, `.LONGNAME`

### 6.43 ZFS Attributes (0x002F)

ZFS (Zettabyte File System) specific attributes.

```c
typedef struct _ZOO64_ZFS_ATTR {
  UINT64  ObjectID;           /// ZFS object ID (DMU object)
  UINT64  SnapshotID;         /// Snapshot ID (0 if not in snapshot)
  UINT64  CloneOriginID;      /// Clone origin ID (0 if not clone)
  UINT32  Flags;              /// ZFS-specific flags
  UINT16  CompressionAlg;     /// Compression algorithm
  UINT16  ChecksumAlg;        /// Checksum algorithm
  UINT64  LogicalSize;        /// Logical size (before compression)
  UINT64  PhysicalSize;       /// Physical size (after compression)
  UINT8   Checksum[32];       /// ZFS checksum (SHA256)
  UINT32  DedupRefCount;      /// Dedup reference count
  UINT32  Reserved;           /// Reserved
} ZOO64_ZFS_ATTR;
#pragma pack(pop)
```

ZFS flags:
```
0x00000001: COMPRESSED         /// File is compressed
0x00000002: DEDUPLICATED       /// File is deduplicated
0x00000004: IS_SNAPSHOT        /// File from snapshot
0x00000008: IS_CLONE           /// File is a clone
0x00000010: CHECKSUM_VERIFIED  /// Checksum verified on read
0x00000020: ARC_CACHED         /// In ARC cache
0x00000040: ENCRYPTED          /// ZFS native encryption
0x00000080: SPECIAL_VDEV       /// On special vdev (SSD)
```

ZFS compression algorithms:
```
0: NONE
1: LZJB
2: GZIP (levels 1-9)
3: ZLE (Zero-Length Encoding)
4: LZ4
5: ZSTD
```

ZFS checksum algorithms:
```
0: INHERIT
1: ON (default: Fletcher4)
2: OFF
3: FLETCHER2
4: FLETCHER4
5: SHA256
6: SHA512
7: SKEIN
8: EDONR
```

**ZFS Clones and Snapshots**:
- Clones share blocks with origin
- Snapshots are read-only point-in-time copies
- On extraction: Recreate as regular files

### 6.44 AdvFS Attributes (0x0030)

Tru64 UNIX Advanced File System (AdvFS) attributes.

```c
typedef struct _ZOO64_ADVFS_ATTR {
  UINT64  FilesetID;          /// Fileset identifier
  UINT64  FileID;             /// File identifier within fileset
  UINT64  CloneID;            /// Clone fileset ID (0 if not clone)
  UINT32  Flags;              /// AdvFS-specific flags
  UINT32  ServiceClass;       /// Storage service class
  UINT64  SnapshotID;         /// Snapshot ID
  UINT32  Reserved[4];        /// Reserved
} ZOO64_ADVFS_ATTR;
#pragma pack(pop)
```

AdvFS flags:
```
0x00000001: CLONED_FILESET    /// File in cloned fileset
0x00000002: SNAPSHOTED        /// File has snapshots
0x00000004: MIGRATED          /// File migrated between service classes
0x00000008: DIRECTIO          /// Direct I/O enabled
```

AdvFS service classes:
```
0: CLASS_UNSPECIFIED
1: CLASS_PERFORMANCE  /// High-performance storage
2: CLASS_CAPACITY     /// High-capacity storage
3: CLASS_ARCHIVE      /// Archive storage
```

### 6.45 XFS Attributes (0x0031)

SGI XFS filesystem attributes.

```c
typedef struct _ZOO64_XFS_ATTR {
  UINT64  Inode;              /// XFS inode number
  UINT64  Generation;         /// Inode generation number
  UINT32  Flags;              /// XFS-specific flags
  UINT32  ExtentSize;         /// Extent size hint
  UINT32  ProjectID;          /// Project ID (quota)
  UINT16  RealtimeFlags;      /// Real-time subvolume flags
  UINT16  RefLinkCount;       /// Reflink reference count
  UINT64  RefLinkSourceInode; // Source inode for reflink
  UINT32  Reserved[2];        /// Reserved
} ZOO64_XFS_ATTR;
#pragma pack(pop)
```

XFS flags:
```
0x00000001: REALTIME           /// File on real-time subvolume
0x00000002: PREALLOC           /// Space preallocated
0x00000004: IMMUTABLE          /// File cannot be modified
0x00000008: APPEND_ONLY        /// Append-only mode
0x00000010: SYNC               /// Synchronous I/O
0x00000020: NOATIME            /// Don't update access time
0x00000040: NODUMP             /// Don't dump this file
0x00000080: REFLINKED          /// File has reflinks (COW)
```

**XFS Reflinks**:
- Copy-on-write file clones
- RefLinkSourceInode identifies original file
- Share extents until modified

### 6.46 JFS Attributes (0x0032)

IBM Journaled File System (JFS) attributes.

```c
typedef struct _ZOO64_JFS_ATTR {
  UINT64  Inode;              /// JFS inode number
  UINT32  Flags;              /// JFS-specific flags
  UINT32  CompressionAlg;     /// Compression algorithm
  UINT64  CompressedSize;     /// Compressed size
  UINT64  UncompressedSize;   /// Original size
  UINT32  Reserved[4];        /// Reserved
} ZOO64_JFS_ATTR;
#pragma pack(pop)
```

JFS flags:
```
0x00000001: COMPRESSED         /// File is compressed
0x00000002: SPARSE             /// Sparse file
0x00000004: ENCRYPTED          /// File is encrypted
0x00000008: INLINE_EA          /// Extended attributes inline
```

JFS compression algorithms:
```
0: NONE
1: GZIP
2: BZIP2
3: LZO
```

### 6.47 ReiserFS Attributes (0x0033)

ReiserFS (version 3 and 4) attributes.

```c
typedef struct _ZOO64_REISERFS_ATTR {
  UINT64  ObjectID;           /// Object ID
  UINT64  DirectoryID;        /// Directory ID
  UINT32  Flags;              /// ReiserFS-specific flags
  UINT32  TailSize;           /// Tail size (for tail packing)
  UINT16  Version;            /// ReiserFS version (3 or 4)
  UINT16  PluginID;           /// Plugin ID (Reiser4)
  UINT32  Reserved[4];        /// Reserved
} ZOO64_REISERFS_ATTR;
#pragma pack(pop)
```

ReiserFS flags:
```
0x00000001: TAIL_PACKED        /// File tail is packed
0x00000002: COMPRESSED         /// Compressed (Reiser4)
0x00000004: ENCRYPTED          /// Encrypted (Reiser4)
0x00000008: IMMUTABLE          /// Immutable file
```

**Tail Packing**:
- Small files/tails stored in directory
- Saves space for small files
- TailSize indicates packed portion

### 6.48 Btrfs Attributes (0x0034)

B-tree File System (Btrfs) attributes.

```c
typedef struct _ZOO64_BTRFS_ATTR {
  UINT64  Inode;              /// Btrfs inode number
  UINT64  Generation;         /// Transaction ID
  UINT64  SubvolumeID;        /// Subvolume ID
  UINT64  SnapshotID;         /// Snapshot ID (0 if not snapshot)
  UINT64  RefLinkSource;      /// Reflink source inode (0 if not reflinked)
  UINT32  Flags;              /// Btrfs-specific flags
  UINT16  CompressionAlg;     /// Compression algorithm
  UINT16  ChecksumAlg;        /// Checksum algorithm
  UINT32  RefLinkCount;       /// Number of reflinks
  UINT8   Checksum[32];       /// File checksum (CRC32C or SHA256)
  UINT32  RAIDProfile;        /// RAID profile (data)
  UINT32  Reserved;           /// Reserved
} ZOO64_BTRFS_ATTR;
#pragma pack(pop)
```

Btrfs flags:
```
0x00000001: COMPRESSED         /// File is compressed
0x00000002: REFLINKED          /// File has reflinks (COW)
0x00000004: IS_SNAPSHOT        /// File from snapshot
0x00000008: IS_SUBVOLUME       /// Is subvolume root
0x00000010: CHECKSUMMED        /// Has checksums
0x00000020: NODATACOW          /// No copy-on-write for data
0x00000040: NODATASUM          /// No checksums for data
0x00000080: ENCRYPTED          /// File is encrypted
```

Btrfs compression algorithms:
```
0: NONE
1: ZLIB
2: LZO
3: ZSTD (default)
```

Btrfs checksum algorithms:
```
0: CRC32C (default)
1: XXHASH
2: SHA256
3: BLAKE2
```

Btrfs RAID profiles:
```
0: SINGLE
1: RAID0
2: RAID1
3: RAID5
4: RAID6
5: RAID10
6: RAID1C3 (3 copies)
7: RAID1C4 (4 copies)
```

**Btrfs Subvolumes and Snapshots**:
- Subvolumes are independent file trees
- Snapshots are read-only or writable point-in-time copies
- Reflinks share extents via copy-on-write
- On extraction: Extract as regular files, preserve metadata

**Btrfs Send/Receive**:
- Could use send stream for incremental archival
- SubvolumeID and Generation track version

### 6.49 Git Metadata (0x0035)

Git distributed version control system metadata.

```c
typedef struct _ZOO64_GIT_ATTR {
  UINT8   ObjectHash[32];     /// Git object hash (SHA-1 or SHA-256)
  UINT8   CommitHash[32];     /// Current commit hash
  UINT32  Flags;              /// Git-specific flags
  UINT16  HashType;           /// Hash algorithm (1=SHA-1, 2=SHA-256)
  UINT16  ObjectType;         /// Git object type
  UINT64  ObjectSize;         /// Size of git object
  UINT32  RefCount;           /// Number of refs
  UINT32  BranchNameLength;   /// Length of branch name
  UINT32  TagCount;           /// Number of tags
  UINT32  RemoteCount;        /// Number of remotes
  /// Followed by:
  ///   [BranchNameLength bytes: current branch name]
  ///   [RefCount * sizeof(GIT_REF): references]
  ///   [TagCount * sizeof(GIT_TAG): tags]
  ///   [RemoteCount * sizeof(GIT_REMOTE): remotes]
} ZOO64_GIT_ATTR;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _GIT_REF {
  UINT8   Hash[32];           /// Ref target hash
  UINT16  NameLength;         /// Length of ref name
  UINT8   RefType;            /// Type: 0=branch, 1=tag, 2=remote, 3=HEAD
  UINT8   Reserved;           /// Reserved
  /// Followed by [NameLength bytes: ref name]
} GIT_REF;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _GIT_TAG {
  UINT8   Hash[32];           /// Tag object hash
  UINT16  NameLength;         /// Length of tag name
  UINT16  MessageLength;      /// Length of tag message
  UINT32  TaggerTimestamp;    /// Tagger timestamp (Unix time)
  /// Followed by:
  ///   [NameLength bytes: tag name]
  ///   [MessageLength bytes: tag message]
} GIT_TAG;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _GIT_REMOTE {
  UINT16  NameLength;         /// Length of remote name
  UINT16  URLLength;          /// Length of remote URL
  UINT32  FetchRefspecCount;  /// Number of fetch refspecs
  /// Followed by:
  ///   [NameLength bytes: remote name]
  ///   [URLLength bytes: remote URL]
  ///   [FetchRefspecCount * variable: refspec strings]
} GIT_REMOTE;
#pragma pack(pop)
```

Git flags:
```
0x00000001: TRACKED            /// File tracked by git
0x00000002: MODIFIED           /// File has modifications
0x00000003: STAGED             /// File is staged
0x00000004: UNTRACKED          /// File is untracked
0x00000008: IGNORED            /// File is ignored (.gitignore)
0x00000010: SUBMODULE          /// File is in submodule
0x00000020: LFS_POINTER        /// File is Git LFS pointer
0x00000040: BARE_REPO          /// Bare repository
0x00000080: SHALLOW_CLONE      /// Shallow clone
0x00000100: WORKTREE           /// Git worktree
0x00000200: PARTIAL_CLONE      /// Partial clone (sparse)
```

Git object types:
```
0: NONE
1: COMMIT
2: TREE
3: BLOB
4: TAG
5: OFS_DELTA
6: REF_DELTA
```

**Git Repository Archival**:
- Store complete .git directory structure
- Preserve object database (objects/)
- Preserve refs (refs/heads/, refs/tags/, refs/remotes/)
- Preserve config, hooks, and info
- Support both SHA-1 and SHA-256 repositories
- Git LFS: Store pointer files and optionally actual LFS objects

### 6.50 Perforce (P4) Metadata (0x0036)

Perforce version control system metadata.

```c
typedef struct _ZOO64_P4_ATTR {
  UINT64  ChangelistNumber;   /// Changelist number
  UINT32  Revision;           /// File revision (head revision)
  UINT32  Flags;              /// Perforce-specific flags
  UINT16  ActionType;         /// Last action type
  UINT16  FileType;           /// Perforce file type
  UINT64  FileSize;           /// File size at head revision
  UINT32  ClientNameLength;   /// Length of client name
  UINT32  DepotPathLength;    /// Length of depot path
  UINT32  IntegrationCount;   /// Number of integration records
  /// Followed by:
  ///   [ClientNameLength bytes: client workspace name]
  ///   [DepotPathLength bytes: depot path]
  ///   [IntegrationCount * sizeof(P4_INTEGRATION): integration records]
} ZOO64_P4_ATTR;
#pragma pack(pop)

typedef struct _P4_INTEGRATION {
  UINT64  FromChangelist;     /// Source changelist
  UINT32  FromRevision;       /// Source revision
  UINT16  FromPathLength;     /// Length of source path
  UINT8   IntegrationType;    /// Type: 0=merge, 1=branch, 2=copy, 3=ignore
  UINT8   Reserved;           /// Reserved
  /// Followed by [FromPathLength bytes: source depot path]
} P4_INTEGRATION;
#pragma pack(pop)
```

Perforce flags:
```
0x00000001: SYNCED             /// File synced to workspace
0x00000002: OPENED_FOR_EDIT    /// Opened for edit
0x00000004: OPENED_FOR_ADD     /// Opened for add
0x00000008: OPENED_FOR_DELETE  /// Opened for delete
0x00000010: EXCLUSIVE_LOCK     /// Exclusive lock held
0x00000020: SHELVED            /// Changes shelved
0x00000040: RESOLVED           /// File resolved
0x00000080: UNRESOLVED         /// File has conflicts
```

Perforce action types:
```
0: ADD
1: EDIT
2: DELETE
3: BRANCH
4: INTEGRATE
5: IMPORT
6: MOVE_ADD
7: MOVE_DELETE
```

Perforce file types:
```
0x0001: TEXT               /// text
0x0002: BINARY             /// binary
0x0004: UNICODE            /// unicode
0x0008: UTF16              /// utf16
0x0010: SYMLINK            /// symlink
0x0020: EXECUTABLE         /// +x executable
0x0040: WRITABLE           /// +w writable
0x0080: EXCLUSIVE_OPEN     /// +l exclusive open
0x0100: COMPRESSED         /// +C compressed in depot
0x0200: RCS_KEYWORD_EXPAND // +k RCS keyword expansion
```

### 6.51 Subversion (SVN) Metadata (0x0037)

Apache Subversion (SVN) metadata.

```c
typedef struct _ZOO64_SVN_ATTR {
  UINT64  Revision;           /// Current revision number
  UINT64  LastChangedRev;     /// Last changed revision
  UINT64  CommittedRev;       /// Committed revision
  UINT32  Flags;              /// SVN-specific flags
  UINT64  CommittedDate;      /// Committed date (Unix timestamp)
  UINT32  RepositoryUUIDLength; // Length of repository UUID
  UINT32  URLLength;          /// Length of repository URL
  UINT32  RelativeURLLength;  /// Length of relative URL
  UINT32  AuthorLength;       /// Length of last author
  UINT32  PropertyCount;      /// Number of properties
  UINT8   NodeKind;           /// Node kind: 0=none, 1=file, 2=dir
  UINT8   Schedule;           /// Schedule: 0=normal, 1=add, 2=delete, 3=replace
  UINT16  Reserved;           /// Reserved
  /// Followed by:
  ///   [RepositoryUUIDLength bytes: repository UUID]
  ///   [URLLength bytes: repository URL]
  ///   [RelativeURLLength bytes: relative URL]
  ///   [AuthorLength bytes: last author]
  ///   [PropertyCount * sizeof(SVN_PROPERTY): properties]
} ZOO64_SVN_ATTR;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _SVN_PROPERTY {
  UINT16  NameLength;         /// Length of property name
  UINT16  ValueLength;        /// Length of property value
  UINT32  Flags;              /// Property flags
  /// Followed by:
  ///   [NameLength bytes: property name]
  ///   [ValueLength bytes: property value]
} SVN_PROPERTY;
#pragma pack(pop)
```

SVN flags:
```
0x00000001: VERSIONED          /// Under version control
0x00000002: MODIFIED           /// File modified
0x00000004: ADDED              /// File added
0x00000008: DELETED            /// File deleted
0x00000010: CONFLICTED         /// File has conflicts
0x00000020: LOCKED             /// File locked
0x00000040: SWITCHED           /// Switched to different URL
0x00000080: INCOMPLETE         /// Incomplete (interrupted operation)
0x00000100: EXTERNAL           /// SVN external
0x00000200: TREE_CONFLICTED    /// Tree conflict
```

SVN property flags:
```
0x00000001: VERSIONED_PROPERTY // Versioned property (svn:*)
0x00000002: INHERITED          /// Inherited property
0x00000004: CUSTOM_PROPERTY    /// Custom (non-svn:) property
```

**Common SVN Properties**:
- svn:executable
- svn:mime-type
- svn:eol-style
- svn:keywords
- svn:ignore
- svn:externals
- svn:needs-lock
- svn:special (for symlinks)

### 6.52 CVS Metadata (0x0038)

Concurrent Versions System (CVS) metadata.

```c
typedef struct _ZOO64_CVS_ATTR {
  UINT16  MajorRevision;      /// Major revision number
  UINT16  MinorRevision;      /// Minor revision number
  UINT32  Flags;              /// CVS-specific flags
  UINT64  CommitTimestamp;    /// Commit timestamp (Unix time)
  UINT32  RepositoryPathLength; // Length of repository path
  UINT32  ModuleLength;       /// Length of module name
  UINT32  BranchLength;       /// Length of branch name
  UINT32  TagCount;           /// Number of tags
  UINT32  AuthorLength;       /// Length of author name
  UINT32  StateLength;        /// Length of state string
  UINT32  LogMessageLength;   /// Length of log message
  UINT16  LockRevMajor;       /// Lock revision major
  UINT16  LockRevMinor;       /// Lock revision minor
  /// Followed by:
  ///   [RepositoryPathLength bytes: repository path]
  ///   [ModuleLength bytes: module name]
  ///   [BranchLength bytes: branch name]
  ///   [TagCount * sizeof(CVS_TAG): symbolic tags]
  ///   [AuthorLength bytes: author name]
  ///   [StateLength bytes: state (Exp, Stab, Rel, dead)]
  ///   [LogMessageLength bytes: log message]
} ZOO64_CVS_ATTR;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _CVS_TAG {
  UINT16  MajorRevision;      /// Tag revision major
  UINT16  MinorRevision;      /// Tag revision minor
  UINT16  NameLength;         /// Length of tag name
  UINT8   TagType;            /// 0=symbolic tag, 1=branch tag
  UINT8   Reserved;           /// Reserved
  /// Followed by [NameLength bytes: tag name]
} CVS_TAG;
#pragma pack(pop)
```

CVS flags:
```
0x00000001: UP_TO_DATE         /// File up to date
0x00000002: LOCALLY_MODIFIED   /// Locally modified
0x00000004: LOCALLY_ADDED      /// Locally added
0x00000008: LOCALLY_REMOVED    /// Locally removed
0x00000010: NEEDS_CHECKOUT     /// Needs checkout
0x00000020: NEEDS_MERGE        /// Needs merge
0x00000040: NEEDS_PATCH        /// Needs patch
0x00000080: CONFLICT           /// Unresolved conflict
0x00000100: LOCKED             /// File locked
0x00000200: STICKY_TAG         /// Has sticky tag
0x00000400: STICKY_DATE        /// Has sticky date
```

CVS states:
```
Exp:  Experimental
Stab: Stable
Rel:  Released
dead: File removed/deleted
```

### 6.53 RCS Metadata (0x0039)

Revision Control System (RCS) metadata.

```c
typedef struct _ZOO64_RCS_ATTR {
  UINT16  HeadMajor;          /// Head revision major
  UINT16  HeadMinor;          /// Head revision minor
  UINT16  WorkingMajor;       /// Working revision major
  UINT16  WorkingMinor;       /// Working revision minor
  UINT32  Flags;              /// RCS-specific flags
  UINT32  BranchLength;       /// Length of branch
  UINT32  AccessListLength;   /// Length of access list
  UINT32  SymbolCount;        /// Number of symbolic names
  UINT32  LocksCount;         /// Number of locks
  UINT32  CommentLength;      /// Length of comment leader
  UINT8   StrictLocking;      /// Strict locking enabled
  UINT8   Reserved[3];        /// Reserved
  /// Followed by:
  ///   [BranchLength bytes: default branch]
  ///   [AccessListLength bytes: access list (space-separated)]
  ///   [SymbolCount * sizeof(RCS_SYMBOL): symbolic names]
  ///   [LocksCount * sizeof(RCS_LOCK): locks]
  ///   [CommentLength bytes: comment leader string]
} ZOO64_RCS_ATTR;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _RCS_SYMBOL {
  UINT16  MajorRevision;      /// Symbol revision major
  UINT16  MinorRevision;      /// Symbol revision minor
  UINT16  NameLength;         /// Length of symbol name
  UINT16  Reserved;           /// Reserved
  /// Followed by [NameLength bytes: symbol name]
} RCS_SYMBOL;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _RCS_LOCK {
  UINT16  MajorRevision;      /// Locked revision major
  UINT16  MinorRevision;      /// Locked revision minor
  UINT16  LockerLength;       /// Length of locker name
  UINT16  Reserved;           /// Reserved
  /// Followed by [LockerLength bytes: locker username]
} RCS_LOCK;
#pragma pack(pop)
```

RCS flags:
```
0x00000001: CHECKED_OUT        /// File checked out
0x00000002: LOCKED             /// File locked
0x00000004: STRICT_LOCKING     /// Strict locking enabled
0x00000008: EXPAND_KEYWORDS    /// Keyword expansion enabled
0x00000010: BINARY_FILE        /// Binary file mode
```

**RCS Keywords**:
- $Id$
- $Header$
- $Author$
- $Date$
- $Revision$
- $Source$
- $State$
- $Log$
- $Locker$

### 6.54 Mercurial Metadata (0x003A)

Mercurial distributed version control system metadata.

```c
typedef struct _ZOO64_HG_ATTR {
  UINT8   NodeID[20];         /// Mercurial node ID (changeset hash)
  UINT8   ParentNode1[20];    /// First parent node ID
  UINT8   ParentNode2[20];    /// Second parent node ID (merge)
  UINT32  Flags;              /// Mercurial-specific flags
  UINT32  RevisionNumber;     /// Local revision number
  UINT64  CommitTimestamp;    /// Commit timestamp (Unix time)
  UINT32  BranchNameLength;   /// Length of branch name
  UINT32  BookmarkCount;      /// Number of bookmarks
  UINT32  TagCount;           /// Number of tags
  UINT32  PhaseCount;         /// Number of phase roots
  UINT8   Phase;              /// Current phase: 0=public, 1=draft, 2=secret
  UINT8   Reserved[3];        /// Reserved
  /// Followed by:
  ///   [BranchNameLength bytes: branch name]
  ///   [BookmarkCount * sizeof(HG_BOOKMARK): bookmarks]
  ///   [TagCount * sizeof(HG_TAG): tags]
  ///   [PhaseCount * sizeof(HG_PHASE_ROOT): phase roots]
} ZOO64_HG_ATTR;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _HG_BOOKMARK {
  UINT8   NodeID[20];         /// Bookmark target node ID
  UINT16  NameLength;         /// Length of bookmark name
  UINT16  Reserved;           /// Reserved
  /// Followed by [NameLength bytes: bookmark name]
} HG_BOOKMARK;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _HG_TAG {
  UINT8   NodeID[20];         /// Tag target node ID
  UINT16  NameLength;         /// Length of tag name
  UINT8   TagType;            /// 0=regular, 1=local
  UINT8   Reserved;           /// Reserved
  /// Followed by [NameLength bytes: tag name]
} HG_TAG;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _HG_PHASE_ROOT {
  UINT8   NodeID[20];         /// Phase root node ID
  UINT8   Phase;              /// Phase: 0=public, 1=draft, 2=secret
  UINT8   Reserved[3];        /// Reserved
} HG_PHASE_ROOT;
#pragma pack(pop)
```

Mercurial flags:
```
0x00000001: TRACKED            /// File tracked
0x00000002: MODIFIED           /// File modified
0x00000004: ADDED              /// File added
0x00000008: REMOVED            /// File removed
0x00000010: CLEAN              /// File clean
0x00000020: MISSING            /// File missing
0x00000040: IGNORED            /// File ignored (.hgignore)
0x00000080: MERGE_STATE        /// In merge state
0x00000100: LARGEFILE          /// Mercurial largefile
0x00000200: SUBREPO            /// Mercurial subrepo
```

**Mercurial Features**:
- Changesets identified by 20-byte node IDs
- Branches are lightweight (just names)
- Bookmarks are movable pointers (like Git branches)
- Tags can be regular (versioned) or local
- Phases: public (immutable), draft (mutable), secret (local only)
- Support for largefiles extension

### 6.55 Fossil Metadata (0x003B)

Fossil distributed version control system metadata.

```c
typedef struct _ZOO64_FOSSIL_ATTR {
  UINT8   ArtifactHash[32];   /// Artifact hash (SHA-1 or SHA-3)
  UINT8   CheckinHash[32];    /// Current checkin hash
  UINT32  Flags;              /// Fossil-specific flags
  UINT16  HashType;           /// Hash algorithm: 1=SHA-1, 2=SHA-3-256
  UINT16  ManifestVersion;    /// Manifest version
  UINT64  CheckinTimestamp;   /// Checkin timestamp (Unix time)
  UINT32  BranchNameLength;   /// Length of branch name
  UINT32  TagCount;           /// Number of tags
  UINT32  UserLength;         /// Length of user name
  UINT32  CommentLength;      /// Length of checkin comment
  UINT32  WikiPageCount;      /// Number of wiki pages
  UINT32  TicketCount;        /// Number of tickets
  /// Followed by:
  ///   [BranchNameLength bytes: branch name]
  ///   [TagCount * sizeof(FOSSIL_TAG): tags]
  ///   [UserLength bytes: user name]
  ///   [CommentLength bytes: checkin comment]
  ///   [WikiPageCount * sizeof(FOSSIL_WIKI): wiki page refs]
  ///   [TicketCount * sizeof(FOSSIL_TICKET): ticket refs]
} ZOO64_FOSSIL_ATTR;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _FOSSIL_TAG {
  UINT8   TagType;            /// 0=propagating, 1=singleton, 2=cancel
  UINT8   Reserved;           /// Reserved
  UINT16  NameLength;         /// Length of tag name
  UINT16  ValueLength;        /// Length of tag value
  UINT16  Reserved2;          /// Reserved
  /// Followed by:
  ///   [NameLength bytes: tag name]
  ///   [ValueLength bytes: tag value (optional)]
} FOSSIL_TAG;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _FOSSIL_WIKI {
  UINT8   ArtifactHash[32];   /// Wiki page artifact hash
  UINT16  PageNameLength;     /// Length of page name
  UINT16  Reserved;           /// Reserved
  /// Followed by [PageNameLength bytes: wiki page name]
} FOSSIL_WIKI;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _FOSSIL_TICKET {
  UINT8   TicketUUID[16];     /// Ticket UUID
  UINT16  TitleLength;        /// Length of ticket title
  UINT8   Status;             /// Status: 0=open, 1=closed, 2=review
  UINT8   Type;               /// Type: 0=bug, 1=feature, 2=task
  /// Followed by [TitleLength bytes: ticket title]
} FOSSIL_TICKET;
#pragma pack(pop)
```

Fossil flags:
```
0x00000001: TRACKED            /// File tracked
0x00000002: MODIFIED           /// File modified
0x00000004: ADDED              /// File added
0x00000008: DELETED            /// File deleted
0x00000010: RENAMED            /// File renamed
0x00000020: EXECUTABLE         /// File is executable
0x00000040: SYMLINK            /// File is symlink
0x00000080: HAS_WIKI           /// Repository has wiki pages
0x00000100: HAS_TICKETS        /// Repository has tickets
0x00000200: HAS_TECHNOTES      /// Repository has technotes
0x00000400: HAS_FORUM          /// Repository has forum
0x00000800: PRIVATE_BRANCH     /// Private branch
```

**Fossil Features**:
- Self-contained executable with built-in web UI
- Integrated bug tracking (tickets)
- Integrated wiki
- Integrated forum and technotes
- Uses SQLite database internally
- Supports both SHA-1 and SHA-3-256 hashing
- Autosync capability
- Blockchain-based design

### 6.56 WebDAV Metadata (0x003C)

Web Distributed Authoring and Versioning (WebDAV) metadata.

```c
typedef struct _ZOO64_WEBDAV_ATTR {
  UINT32  Flags;              /// WebDAV-specific flags
  UINT64  CreationDate;       /// Creation date (Unix timestamp)
  UINT64  LastModified;       /// Last modified date
  UINT64  GetContentLength;   /// Content length
  UINT32  GetContentTypeLength; // Length of content type
  UINT32  GetETagLength;      /// Length of ETag
  UINT32  DisplayNameLength;  /// Length of display name
  UINT32  LivePropCount;      /// Number of live properties
  UINT32  DeadPropCount;      /// Number of dead properties
  UINT32  LockCount;          /// Number of locks
  UINT8   ResourceType;       /// 0=regular, 1=collection
  UINT8   Reserved[3];        /// Reserved
  /// Followed by:
  ///   [GetContentTypeLength bytes: content type (MIME)]
  ///   [GetETagLength bytes: entity tag]
  ///   [DisplayNameLength bytes: display name]
  ///   [LivePropCount * sizeof(WEBDAV_LIVE_PROP): live properties]
  ///   [DeadPropCount * sizeof(WEBDAV_DEAD_PROP): dead properties]
  ///   [LockCount * sizeof(WEBDAV_LOCK): active locks]
} ZOO64_WEBDAV_ATTR;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _WEBDAV_LIVE_PROP {
  UINT16  PropertyID;         /// Standard property ID
  UINT16  ValueLength;        /// Length of value
  /// Followed by [ValueLength bytes: property value]
} WEBDAV_LIVE_PROP;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _WEBDAV_DEAD_PROP {
  UINT16  NamespaceLength;    /// Length of XML namespace
  UINT16  NameLength;         /// Length of property name
  UINT16  ValueLength;        /// Length of property value
  UINT16  Reserved;           /// Reserved
  /// Followed by:
  ///   [NamespaceLength bytes: XML namespace URI]
  ///   [NameLength bytes: property name]
  ///   [ValueLength bytes: property value (XML)]
} WEBDAV_DEAD_PROP;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _WEBDAV_LOCK {
  UINT8   LockToken[16];      /// Lock token (UUID)
  UINT64  Timeout;            /// Lock timeout (Unix timestamp)
  UINT32  OwnerLength;        /// Length of lock owner
  UINT8   LockScope;          /// 0=exclusive, 1=shared
  UINT8   LockType;           /// 0=write
  UINT16  DepthInfinity;      /// Depth: 0=depth-0, 1=depth-infinity
  /// Followed by [OwnerLength bytes: lock owner (XML)]
} WEBDAV_LOCK;
#pragma pack(pop)
```

WebDAV flags:
```
0x00000001: IS_COLLECTION      /// Resource is collection (directory)
0x00000002: HAS_LIVE_PROPS     /// Has live properties
0x00000004: HAS_DEAD_PROPS     /// Has dead (custom) properties
0x00000008: LOCKED             /// Resource locked
0x00000010: SUPPORTS_LOCKING   /// Server supports locking
0x00000020: VERSIONED          /// DeltaV versioned resource
0x00000040: CHECKED_OUT        /// DeltaV checked out
0x00000080: VERSION_CONTROLLED // DeltaV version-controlled
```

WebDAV live property IDs:
```
0x0001: creationdate
0x0002: displayname
0x0003: getcontentlanguage
0x0004: getcontentlength
0x0005: getcontenttype
0x0006: getetag
0x0007: getlastmodified
0x0008: lockdiscovery
0x0009: resourcetype
0x000A: supportedlock
0x000B: source (DeltaV)
0x000C: checked-in (DeltaV)
0x000D: checked-out (DeltaV)
0x000E: version-name (DeltaV)
0x000F: predecessor-set (DeltaV)
0x0010: successor-set (DeltaV)
```

**WebDAV Features**:
- Live properties: server-maintained (getlastmodified, etc.)
- Dead properties: custom XML properties set by clients
- Locking: exclusive or shared locks with depth
- Collections: WebDAV term for directories
- DeltaV: versioning extension (RFC 3253)
- ETags for cache validation
- MIME type tracking (getcontenttype)

**DeltaV (WebDAV Versioning)**:
- Version-controlled resources
- Check-out/check-in workflow
- Version history
- Workspaces and activities
- Baseline collections

### 6.57 NFS Metadata (0x003D)

Network File System (NFS) versions 2, 3, and 4 metadata.

```c
typedef struct _ZOO64_NFS_ATTR {
  UINT8   FileHandle[128];    /// NFS file handle (variable length, max 128)
  UINT16  FileHandleLength;   /// Actual length of file handle
  UINT16  NFSVersion;         /// NFS version: 2, 3, or 4
  UINT32  Flags;              /// NFS-specific flags
  UINT32  FileType;           /// NFS file type (ftype)
  UINT32  Mode;               /// Unix mode bits
  UINT32  NLink;              /// Number of hard links
  UINT32  UID;                /// Owner UID
  UINT32  GID;                /// Owner GID
  UINT64  Size;               /// File size in bytes
  UINT64  Used;               /// Disk space used
  UINT64  FileID;             /// File identifier
  UINT64  FSId;               /// Filesystem identifier
  UINT64  ATime;              /// Access time (NFS time)
  UINT64  MTime;              /// Modification time
  UINT64  CTime;              /// Change time
  UINT32  NFSv4ChangeAttr;    /// NFSv4 change attribute
  UINT32  NamedAttrCount;     /// NFSv4 named attributes count
  UINT32  ACLCount;           /// NFSv4 ACL entry count
  /// Followed by:
  ///   [NamedAttrCount * sizeof(NFS_NAMED_ATTR): named attributes]
  ///   [ACLCount * sizeof(NFSv4_ACE): ACL entries]
} ZOO64_NFS_ATTR;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _NFS_NAMED_ATTR {
  UINT16  NameLength;         /// Length of attribute name
  UINT32  ValueLength;        /// Length of attribute value
  UINT16  Reserved;           /// Reserved
  /// Followed by:
  ///   [NameLength bytes: attribute name]
  ///   [ValueLength bytes: attribute value]
} NFS_NAMED_ATTR;
#pragma pack(pop)

typedef struct _NFSv4_ACE {
  UINT32  Type;               /// ACE type (ALLOW, DENY, AUDIT, ALARM)
  UINT32  Flag;               /// ACE flags (inheritance, etc.)
  UINT32  AccessMask;         /// Access permissions
  UINT16  WhoLength;          /// Length of who (principal)
  UINT16  Reserved;           /// Reserved
  /// Followed by [WhoLength bytes: principal (user@domain)]
} NFSv4_ACE;
#pragma pack(pop)
```

NFS flags:
```
0x00000001: MOUNTED            /// File on NFS mount
0x00000002: HAS_NAMED_ATTRS    /// Has named attributes (NFSv4)
0x00000004: HAS_ACL            /// Has ACL (NFSv4)
0x00000008: DELEGATED          /// File has delegation (NFSv4)
0x00000010: CACHED             /// File data cached locally
0x00000020: LOCKED             /// File has byte-range lock
0x00000040: HOMOGENEOUS        /// Homogeneous file (NFSv4)
0x00000080: HIDDEN             /// Hidden file (Windows-style)
```

NFS file types (ftype):
```
1: NF4REG       /// Regular file
2: NF4DIR       /// Directory
3: NF4BLK       /// Block device
4: NF4CHR       /// Character device
5: NF4LNK       /// Symbolic link
6: NF4SOCK      /// Socket
7: NF4FIFO      /// FIFO
8: NF4ATTRDIR   /// Attribute directory (NFSv4)
9: NF4NAMEDATTR // Named attribute (NFSv4)
```

NFSv4 ACE types:
```
0: ACCESS_ALLOWED_ACE_TYPE
1: ACCESS_DENIED_ACE_TYPE
2: SYSTEM_AUDIT_ACE_TYPE
3: SYSTEM_ALARM_ACE_TYPE
```

NFSv4 access mask bits:
```
0x00000001: READ_DATA
0x00000002: WRITE_DATA
0x00000004: APPEND_DATA
0x00000008: READ_NAMED_ATTRS
0x00000010: WRITE_NAMED_ATTRS
0x00000020: EXECUTE
0x00000040: DELETE_CHILD
0x00000080: READ_ATTRIBUTES
0x00000100: WRITE_ATTRIBUTES
0x00000200: DELETE
0x00000400: READ_ACL
0x00000800: WRITE_ACL
0x00001000: WRITE_OWNER
0x00002000: SYNCHRONIZE
```

**NFS Version-Specific Features**:
- **NFSv2**: Basic file operations, 32-bit file sizes
- **NFSv3**: 64-bit file sizes, READDIRPLUS, async writes
- **NFSv4**: Stateful protocol, compound operations, delegations, named attributes, NFSv4 ACLs, internationalization (UTF-8)

### 6.58 SMB/CIFS Metadata (0x003E)

Server Message Block / Common Internet File System metadata.

```c
typedef struct _ZOO64_SMB_ATTR {
  UINT64  FileID;             /// SMB2+ persistent file ID
  UINT64  VolumeID;           /// SMB2+ volume ID
  UINT32  Flags;              /// SMB-specific flags
  UINT32  FileAttributes;     /// Windows file attributes
  UINT64  AllocationSize;     /// Allocation size
  UINT64  EndOfFile;          /// End of file
  UINT32  NumberOfLinks;      /// Number of hard links
  UINT8   DeletePending;      /// Delete pending flag
  UINT8   Directory;          /// Is directory
  UINT16  EASize;             /// Extended attributes size
  UINT32  StreamCount;        /// Number of alternate data streams
  UINT32  ReparseTag;         /// Reparse point tag (if reparse point)
  UINT8   FileId128[16];      /// SMB3+ 128-bit file ID
  UINT32  ShareAccess;        /// Share access flags
  UINT32  CreateOptions;      /// Create options
  /// Followed by:
  ///   [StreamCount * sizeof(SMB_STREAM): alternate data streams]
  ///   [Variable: EA data if EASize > 0]
  ///   [Variable: Reparse data if reparse point]
} ZOO64_SMB_ATTR;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _SMB_STREAM {
  UINT64  StreamSize;         /// Size of stream
  UINT64  StreamAllocationSize; // Allocation size of stream
  UINT16  StreamNameLength;   /// Length of stream name
  UINT16  Reserved;           /// Reserved
  /// Followed by [StreamNameLength bytes: stream name (UTF-16LE)]
} SMB_STREAM;
#pragma pack(pop)
```

SMB flags:
```
0x00000001: SMB1              /// SMB1/CIFS protocol
0x00000002: SMB2              /// SMB2 protocol
0x00000004: SMB3              /// SMB3 protocol
0x00000008: ENCRYPTED         /// SMB3 encryption enabled
0x00000010: COMPRESSED        /// SMB3 compression enabled
0x00000020: HAS_STREAMS       /// Has alternate data streams
0x00000040: HAS_EA            /// Has extended attributes
0x00000080: REPARSE_POINT     /// Is reparse point
0x00000100: SPARSE_FILE       /// Sparse file
0x00000200: OFFLINE           /// File is offline (HSM)
0x00000400: OPLOCK_HELD       /// Opportunistic lock held
0x00000800: LEASE_HELD        /// SMB2+ lease held
```

Windows file attributes (subset):
```
0x00000001: FILE_ATTRIBUTE_READONLY
0x00000002: FILE_ATTRIBUTE_HIDDEN
0x00000004: FILE_ATTRIBUTE_SYSTEM
0x00000008: FILE_ATTRIBUTE_DIRECTORY
0x00000020: FILE_ATTRIBUTE_ARCHIVE
0x00000040: FILE_ATTRIBUTE_DEVICE
0x00000080: FILE_ATTRIBUTE_NORMAL
0x00000100: FILE_ATTRIBUTE_TEMPORARY
0x00000200: FILE_ATTRIBUTE_SPARSE_FILE
0x00000400: FILE_ATTRIBUTE_REPARSE_POINT
0x00000800: FILE_ATTRIBUTE_COMPRESSED
0x00001000: FILE_ATTRIBUTE_OFFLINE
0x00002000: FILE_ATTRIBUTE_NOT_CONTENT_INDEXED
0x00004000: FILE_ATTRIBUTE_ENCRYPTED
0x00008000: FILE_ATTRIBUTE_INTEGRITY_STREAM
0x00020000: FILE_ATTRIBUTE_NO_SCRUB_DATA
```

Reparse point tags:
```
0x80000000: IO_REPARSE_TAG_SYMBOLIC_LINK
0x80000001: IO_REPARSE_TAG_MOUNT_POINT
0x80000002: IO_REPARSE_TAG_HSM
0x80000006: IO_REPARSE_TAG_DFS
0x8000000A: IO_REPARSE_TAG_DFSR
0x8000000C: IO_REPARSE_TAG_DEDUP
0x80000012: IO_REPARSE_TAG_NFS
0x80000014: IO_REPARSE_TAG_CLOUD
0x80000017: IO_REPARSE_TAG_APPEXECLINK
```

**SMB Protocol Features**:
- **SMB1/CIFS**: Basic file sharing, opportunistic locks
- **SMB2**: Compound requests, durable handles, larger reads/writes
- **SMB3**: Encryption, compression, multichannel, directory leases, witness protocol

### 6.59 NetWare NCP Metadata (0x003F)

NetWare Core Protocol (NCP) / NetWare filesystem metadata.

```c
typedef struct _ZOO64_NETWARE_ATTR {
  UINT32  FileNumber;         /// NetWare file number
  UINT32  DirectoryNumber;    /// Parent directory number
  UINT32  VolumeNumber;       /// Volume number
  UINT32  Flags;              /// NetWare-specific flags
  UINT32  Attributes;         /// NetWare file attributes
  UINT64  FileSize;           /// File size
  UINT64  CreationDate;       /// Creation date/time
  UINT64  LastAccessDate;     /// Last access date
  UINT64  LastModifiedDate;   /// Last modified date/time
  UINT64  LastArchivedDate;   /// Last archived date/time
  UINT32  OwnerID;            /// Owner object ID
  UINT32  ArchiverID;         /// Archiver object ID
  UINT32  TrusteeCount;       /// Number of trustees
  UINT32  NamespaceInfo;      /// Namespace information
  UINT16  InheritedRightsMask; // Inherited rights mask
  UINT16  Reserved;           /// Reserved
  /// Followed by:
  ///   [TrusteeCount * sizeof(NETWARE_TRUSTEE): trustees]
  ///   [Variable: Extended attributes]
} ZOO64_NETWARE_ATTR;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _NETWARE_TRUSTEE {
  UINT32  ObjectID;           /// Trustee object ID
  UINT16  Rights;             /// Trustee rights
  UINT16  ObjectType;         /// Object type (user, group, etc.)
  UINT32  NameLength;         /// Length of trustee name
  /// Followed by [NameLength bytes: trustee name]
} NETWARE_TRUSTEE;
#pragma pack(pop)
```

NetWare flags:
```
0x00000001: MIGRATED           /// File migrated to secondary storage
0x00000002: COMPRESSED         /// File compressed
0x00000004: SUBALLOCATED       /// File suballocated
0x00000008: IMMEDIATE_COMPRESS // Immediate compression
0x00000010: DATA_STREAM        /// Has data stream
0x00000020: NAME_SPACE         /// Supports namespaces
```

NetWare file attributes:
```
0x0001: READ_ONLY              /// Read-only
0x0002: HIDDEN                 /// Hidden
0x0004: SYSTEM                 /// System
0x0008: EXECUTE_ONLY           /// Execute only
0x0010: SUBDIRECTORY           /// Subdirectory
0x0020: ARCHIVE                /// Archive needed
0x0040: EXECUTE_CONFIRM        /// Execute confirm
0x0080: SHAREABLE              /// Shareable
0x0100: DONT_COMPRESS          /// Don't compress
0x0200: DONT_MIGRATE           /// Don't migrate
0x0400: IMMEDIATE_COMPRESS     /// Compress immediately
0x0800: RENAME_INHIBIT         /// Rename inhibited
0x1000: DELETE_INHIBIT         /// Delete inhibited
0x2000: COPY_INHIBIT           /// Copy inhibited
0x4000: PURGE                  /// Immediate purge
0x8000: TRANSACTIONAL          /// Transactional
```

NetWare trustee rights:
```
0x0001: READ                   /// Read
0x0002: WRITE                  /// Write
0x0004: CREATE                 /// Create
0x0008: ERASE                  /// Erase
0x0010: ACCESS_CONTROL         /// Modify
0x0020: FILE_SCAN              /// File scan
0x0040: MODIFY                 /// Modify attributes
0x0080: SUPERVISOR             /// Supervisor (all rights)
```

NetWare namespaces:
```
0: DOS_NAMESPACE               /// DOS 8.3 namespace
1: MAC_NAMESPACE               /// Macintosh namespace
2: NFS_NAMESPACE               /// NFS namespace
3: FTAM_NAMESPACE              /// FTAM namespace
4: OS2_NAMESPACE               /// OS/2 long namespace
5: UNIX_NAMESPACE              /// Unix namespace
```

### 6.60 AFP Metadata (0x0040)

Apple Filing Protocol (AFP) versions 1, 2, and 3 metadata.

```c
typedef struct _ZOO64_AFP_ATTR {
  UINT32  FileNumber;         /// AFP file number (CNID)
  UINT32  ParentDirNumber;    /// Parent directory CNID
  UINT16  AFPVersion;         /// AFP version (1, 2, or 3)
  UINT16  Flags;              /// AFP-specific flags
  UINT32  FinderInfo[8];      /// Finder info (32 bytes)
  UINT32  ExtendedFinderInfo[4]; // Extended Finder info (16 bytes)
  UINT64  DataForkSize;       /// Data fork logical size
  UINT64  ResourceForkSize;   /// Resource fork logical size
  UINT64  DataForkAllocSize;  /// Data fork allocation size
  UINT64  ResourceForkAllocSize; // Resource fork allocation size
  UINT16  AccessRights;       /// AFP access rights
  UINT16  UnixPrivileges;     /// Unix privileges mode
  UINT32  OwnerID;            /// Owner ID
  UINT32  GroupID;            /// Group ID
  UINT32  ACLCount;           /// Number of ACL entries
  /// Followed by:
  ///   [DataForkSize bytes: data fork content]
  ///   [ResourceForkSize bytes: resource fork content]
  ///   [ACLCount * sizeof(AFP_ACE): ACL entries]
} ZOO64_AFP_ATTR;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _AFP_ACE {
  UINT32  ACEType;            /// ACE type
  UINT32  ACEFlags;           /// ACE flags
  UINT32  AccessMask;         /// Access mask
  UINT32  UUIDLength;         /// Length of UUID (usually 16)
  /// Followed by [UUIDLength bytes: principal UUID]
} AFP_ACE;
#pragma pack(pop)
```

AFP flags:
```
0x0001: HAS_RESOURCE_FORK      /// Has resource fork
0x0002: HAS_CUSTOM_ICON        /// Has custom icon
0x0004: IS_ALIAS               /// Is alias file
0x0008: IS_INVISIBLE           /// Is invisible
0x0010: COPY_PROTECTED         /// Copy protected (AFP 2.0+)
0x0020: DELETE_INHIBIT         /// Delete inhibit (AFP 2.0+)
0x0040: RENAME_INHIBIT         /// Rename inhibit (AFP 2.0+)
0x0080: SET_CLEAR              /// Set/Clear bit (AFP 2.0+)
0x0100: BACKUP_NEEDED          /// Backup needed
0x0200: NO_COPY                /// Don't copy (AFP 3.0+)
```

Finder info structure (32 bytes):
```
Bytes 0-3:   File type (FourCC)
Bytes 4-7:   File creator (FourCC)
Bytes 8-9:   Finder flags
Bytes 10-13: Location (Point)
Bytes 14-15: Folder flags
Bytes 16-23: Reserved
Bytes 24-27: Icon location
Bytes 28-31: Reserved
```

Finder flags:
```
0x0001: kIsOnDesk
0x0002: kColor (3 bits)
0x0010: kIsShared
0x0020: kHasNoINITs
0x0040: kHasBeenInited
0x0100: kHasCustomIcon
0x0200: kIsStationery
0x0400: kNameLocked
0x0800: kHasBundle
0x1000: kIsInvisible
0x2000: kIsAlias
```

AFP access rights:
```
0x01: OWNER_SEARCH             /// Owner search
0x02: OWNER_READ               /// Owner read
0x04: OWNER_WRITE              /// Owner write
0x10: GROUP_SEARCH             /// Group search
0x20: GROUP_READ               /// Group read
0x40: GROUP_WRITE              /// Group write
0x100: EVERYONE_SEARCH         /// Everyone search
0x200: EVERYONE_READ           /// Everyone read
0x400: EVERYONE_WRITE          /// Everyone write
0x800: USER_HAS_NO_RIGHTS      /// User has no rights
0x1000: BLANK_ACCESS           /// Blank access
```

**AFP Protocol Features**:
- **AFP 1.x**: Basic file sharing, AppleTalk
- **AFP 2.x**: TCP/IP support, enhanced security, file server messages
- **AFP 3.x**: Kerberos authentication, ACLs (UUID-based), extended attributes, spotlight search

### 6.61 DCE DFS Metadata (0x0041)

Distributed Computing Environment Distributed File System metadata.

```c
typedef struct _ZOO64_DCE_DFS_ATTR {
  UINT8   FileUUID[16];       /// DCE file UUID
  UINT8   VolumeUUID[16];     /// DCE volume UUID
  UINT64  FileID;             /// File identifier
  UINT64  VolumeID;           /// Volume identifier
  UINT32  Flags;              /// DCE DFS-specific flags
  UINT32  FileType;           /// File type
  UINT64  DataVersion;        /// Data version number
  UINT64  ACLVersion;         /// ACL version number
  UINT32  ACLCount;           /// Number of ACL entries
  UINT32  ExtendedAttrCount;  /// Number of extended attributes
  UINT32  CellNameLength;     /// Length of cell name
  UINT32  VolumeNameLength;   /// Length of volume name
  /// Followed by:
  ///   [CellNameLength bytes: DCE cell name]
  ///   [VolumeNameLength bytes: volume name]
  ///   [ACLCount * sizeof(DCE_ACE): ACL entries]
  ///   [ExtendedAttrCount * sizeof(DCE_XATTR): extended attributes]
} ZOO64_DCE_DFS_ATTR;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _DCE_ACE {
  UINT8   PrincipalUUID[16];  /// Principal UUID
  UINT32  PermissionBits;     /// Permission bits
  UINT32  ACEType;            /// ACE type
  UINT16  PrincipalNameLength; // Length of principal name
  UINT16  Reserved;           /// Reserved
  /// Followed by [PrincipalNameLength bytes: principal name]
} DCE_ACE;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _DCE_XATTR {
  UINT16  NameLength;         /// Length of attribute name
  UINT32  ValueLength;        /// Length of attribute value
  UINT16  Flags;              /// Attribute flags
  /// Followed by:
  ///   [NameLength bytes: attribute name]
  ///   [ValueLength bytes: attribute value]
} DCE_XATTR;
#pragma pack(pop)
```

DCE DFS flags:
```
0x00000001: REPLICATED         /// File is replicated
0x00000002: FILESET_ROOT       /// Fileset root
0x00000004: MOUNT_POINT        /// Is mount point
0x00000008: VOLUME_ROOT        /// Volume root
0x00000010: CACHED             /// File cached locally
0x00000020: TOKEN_HELD         /// Access token held
0x00000040: CALLBACK_SET       /// Callback registered
```

DCE DFS file types:
```
0: INVALID
1: FILE
2: DIRECTORY
3: SYMBOLIC_LINK
4: MOUNT_POINT
```

DCE ACE permission bits:
```
0x00000001: READ               /// Read
0x00000002: WRITE              /// Write
0x00000004: EXECUTE            /// Execute
0x00000008: CONTROL            /// Control (admin)
0x00000010: INSERT             /// Insert
0x00000020: DELETE             /// Delete
0x00000040: LOCK               /// Lock
0x00000080: ADMINISTER         /// Administer
```

DCE ACE types:
```
0: USER_OBJ                    /// Owner
1: USER                        /// Named user
2: GROUP_OBJ                   /// Owning group
3: GROUP                       /// Named group
4: OTHER_OBJ                   /// Other
5: MASK_OBJ                    /// Mask
6: ANY_OTHER                   /// Any other
7: FOREIGN_OTHER               /// Foreign principal
```

**DCE DFS Features**:
- Global namespace across cells
- Transparent replication
- Location independence (mount points)
- Kerberos authentication
- Token-based caching with callbacks
- ACLs with UUID-based principals
- Fileset quotas
- Episode or UFS backend filesystems

### 6.62 Short Filename Metadata (0x0042)

Preserves short filenames (8.3 DOS format, Windows generated short names, etc.).

```c
typedef struct _ZOO64_SHORT_FILENAME_ATTR {
  UINT16  ShortNameLength;    /// Length of short filename
  UINT8   NameFormat;         /// Short name format type
  UINT8   GenerationMethod;   /// How name was generated
  UINT32  Flags;              /// Short filename flags
  /// Followed by [ShortNameLength bytes: short filename]
} ZOO64_SHORT_FILENAME_ATTR;
#pragma pack(pop)
```

**Name Format Types**:
```
0: DOS_8_3              /// Classic DOS 8.3 format (FILENAME.EXT)
1: WINDOWS_TILDE        /// Windows tildegenerated (LONGFI~1.TXT)
2: VFAT_NUMERIC         /// VFAT numeric suffix (LONGFI~2.TXT)
3: CUSTOM_SHORT         /// Custom/manually set short name
4: CASE_MANGLED         /// Case-mangled for case-insensitive FS
```

**Generation Methods**:
```
0: MANUAL               /// Manually specified by user
1: AUTO_WINDOWS         /// Windows automatic generation
2: AUTO_DOS             /// DOS automatic generation
3: AUTO_VFAT            /// VFAT automatic generation
4: PRESERVED_ORIGINAL   /// Preserved from source filesystem
```

**Short Filename Flags**:
```
0x00000001: LOSS_ON_RENAME      /// Loses short name if renamed
0x00000002: CASE_PRESERVED      /// Case preserved but ignored
0x00000004: UNIQUE_GUARANTEED   /// Guaranteed unique in directory
0x00000008: GENERATED_FROM_LONG // Generated from long filename
```

**Examples**:
```
Long: "My Important Document.docx"
Short (DOS 8.3): "MYIMPOR~1.DOC"

Long: "Annual_Financial_Report_2024_Q4_Final_v3.xlsx"
Short (VFAT): "ANNUAL~1.XLS"

Long: "ReadMe.txt"
Short (DOS 8.3): "README.TXT" (fits in 8.3)
```

**Use Cases**:
- Preserving DOS compatibility
- Windows VFAT/FAT32 filesystems
- OS/2 HPFS long names with short alias
- Dual-name systems (long + short)

### 6.63 HFS Filename Metadata (0x0043)

Preserves HFS-encoded filenames with character substitution for illegal characters.

```c
typedef struct _ZOO64_HFS_FILENAME_ATTR {
  UINT16  HFSNameLength;      /// Length of HFS-encoded name
  UINT16  SubstitutionCount;  /// Number of character substitutions
  UINT32  Flags;              /// HFS filename flags
  /// Followed by:
  ///   [HFSNameLength bytes: HFS-encoded filename]
  ///   [SubstitutionCount * sizeof(HFS_CHAR_SUBST): substitutions]
} ZOO64_HFS_FILENAME_ATTR;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _HFS_CHAR_SUBST {
  UINT16  Position;           /// Position in filename (0-indexed)
  UINT16  OriginalChar;       /// Original Unicode character
  UINT16  SubstitutedChar;    /// HFS-safe character (usually #HEX)
  UINT16  Reserved;           /// Reserved
} HFS_CHAR_SUBST;
#pragma pack(pop)
```

**HFS Filename Encoding**:
HFS (Classic Mac OS) and HFS+ have character restrictions:
- Cannot use `:` (colon) - used as path separator
- Limited to 255 bytes (UTF-16 on HFS+)
- Case-preserving but case-insensitive (HFS+)

**Character Substitution Format**:
```
Original: file:name.txt
HFS-safe: file#3Aname.txt
         (0x3A = colon in hex)

Original: path/to/file.txt
HFS-safe: path#2Fto#2Ffile.txt
         (0x2F = forward slash in hex)
```

**HFS Flags**:
```
0x00000001: HAS_SUBSTITUTIONS     /// Contains #XX substitutions
0x00000002: CASE_PRESERVED        /// Original case preserved
0x00000004: DECOMPOSED_UNICODE    /// Uses NFD normalization
0x00000008: RESOURCE_FORK_NAME    /// Name of resource fork (..namedfork/rsrc)
```

**Substitution Table** (Common illegal characters):
```
:  → #3A  (colon - path separator on HFS)
/  → #2F  (forward slash)
\  → #5C  (backslash)
*  → #2A  (asterisk)
?  → #3F  (question mark)
"  → #22  (double quote)
<  → #3C  (less than)
>  → #3E  (greater than)
|  → #7C  (pipe)
```

**Use Cases**:
- Archiving from non-HFS to HFS
- Preserving filenames with illegal HFS characters
- Cross-platform filename compatibility
- macOS Classic compatibility

**Round-Trip**:
When extracting to non-HFS system, reverse substitutions:
- `file#3Aname.txt` → `file:name.txt`
- Read substitution table and replace #XX with original chars

### 6.64 File Type Detection Metadata (0x0044)

Stores detected or specified file type (text vs binary, MIME type, etc.).

```c
typedef struct _ZOO64_FILETYPE_ATTR {
  UINT8   DetectionMethod;    /// How type was determined
  UINT8   FileCategory;       /// High-level category
  UINT16  MimeTypeLength;     /// Length of MIME type string
  UINT32  Confidence;         /// Detection confidence (0-100)
  UINT32  Flags;              /// File type flags
  UINT32  MagicNumber;        /// File magic number (if applicable)
  UINT32  CharsetLength;      /// Length of charset name (for text)
  UINT16  LineEnding;         /// Line ending type (for text)
  UINT16  BOM;                /// Byte Order Mark type (for text)
  /// Followed by:
  ///   [MimeTypeLength bytes: UTF-8 MIME type]
  ///   [CharsetLength bytes: charset name (e.g., "UTF-8", "ISO-8859-1")]
} ZOO64_FILETYPE_ATTR;
#pragma pack(pop)
```

**Detection Methods**:
```
0: UNKNOWN              /// Unknown/not detected
1: FILE_EXTENSION       /// Based on filename extension
2: MAGIC_NUMBER         /// Based on file magic number
3: CONTENT_ANALYSIS     /// Content heuristics
4: USER_SPECIFIED       /// Manually specified
5: MIME_TYPE_HEADER     /// From HTTP/email headers
6: LIBMAGIC             /// Using libmagic (file command)
7: COMBINED             /// Multiple methods combined
```

**File Categories**:
```
0: UNKNOWN
1: TEXT                 /// Plain text file
2: BINARY               /// Binary data
3: EXECUTABLE           /// Executable program
4: ARCHIVE              /// Archive/compressed file
5: IMAGE                /// Image file
6: VIDEO                /// Video file
7: AUDIO                /// Audio file
8: DOCUMENT             /// Document (PDF, Word, etc.)
9: SOURCE_CODE          /// Source code file
10: DATA                /// Structured data (JSON, XML, CSV)
11: DATABASE            /// Database file
12: FONT                /// Font file
13: MULTIMEDIA          /// Other multimedia
```

**File Type Flags**:
```
0x00000001: IS_TEXT             /// File is text
0x00000002: IS_BINARY           /// File is binary
0x00000004: HAS_BOM             /// Has byte order mark
0x00000008: MIXED_LINE_ENDINGS  /// Mixed CR/LF/CRLF
0x00000010: NULL_BYTES_PRESENT  /// Contains null bytes
0x00000020: HIGH_ENTROPY        /// High entropy (encrypted/compressed)
0x00000040: VALID_UTF8          /// Valid UTF-8 encoding
0x00000080: ASCII_ONLY          /// Only ASCII characters
0x00000100: COMPRESSED          /// Compressed format
0x00000200: ENCRYPTED           /// Encrypted format
```

**Line Ending Types** (for text files):
```
0: UNKNOWN
1: LF                   /// Unix/Linux/macOS (\n)
2: CRLF                 /// Windows (\r\n)
3: CR                   /// Classic Mac OS (\r)
4: MIXED                /// Mixed line endings
```

**BOM (Byte Order Mark) Types**:
```
0: NO_BOM
1: UTF8_BOM             /// EF BB BF
2: UTF16_LE_BOM         /// FF FE
3: UTF16_BE_BOM         /// FE FF
4: UTF32_LE_BOM         /// FF FE 00 00
5: UTF32_BE_BOM         /// 00 00 FE FF
```

**Common MIME Types**:
```
text/plain              /// Plain text
text/html               /// HTML
text/css                /// CSS
text/javascript         /// JavaScript
application/json        /// JSON
application/xml         /// XML
application/pdf         /// PDF
application/zip         /// ZIP archive
image/jpeg              /// JPEG image
image/png               /// PNG image
video/mp4               /// MP4 video
audio/mpeg              /// MP3 audio
```

**Text vs Binary Detection Heuristics**:
1. Check for BOM (→ text with encoding)
2. Sample first 8KB of file
3. Count null bytes (>1% → probably binary)
4. Check for valid UTF-8 sequences
5. Calculate entropy (high → binary/encrypted)
6. Check for ASCII printable characters (>95% → text)

**Examples**:
```
File: README.md
  Category: TEXT
  MIME: text/markdown
  Charset: UTF-8
  LineEnding: LF
  Confidence: 98%

File: program.exe
  Category: EXECUTABLE
  MIME: application/x-msdownload
  MagicNumber: 0x5A4D (MZ)
  Confidence: 100%

File: data.bin
  Category: BINARY
  MIME: application/octet-stream
  Confidence: 75%
```

**Use Cases**:
- Automatic line ending conversion
- Text encoding detection and conversion
- Binary safety checks
- Content-type preservation
- MIME type for HTTP serving
- Editor mode selection

### 6.65 Sparse File Metadata (0x0045)

Efficiently stores sparse files by tracking hole regions (unallocated/zero-filled blocks).

```c
typedef struct _ZOO64_SPARSE_ATTR {
  UINT64  LogicalSize;        /// Full logical file size
  UINT64  PhysicalSize;       /// Actual data size (excluding holes)
  UINT32  HoleCount;          /// Number of hole regions
  UINT32  Flags;              /// Sparse file flags
  UINT32  BlockSize;          /// Block size for hole alignment
  UINT32  Reserved;           /// Reserved
  /// Followed by HoleCount * sizeof(SPARSE_HOLE) structures
} ZOO64_SPARSE_ATTR;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _SPARSE_HOLE {
  UINT64  Offset;             /// Offset of hole start
  UINT64  Length;             /// Length of hole
} SPARSE_HOLE;
#pragma pack(pop)
```

**Sparse File Flags**:
```
0x00000001: SYSTEM_SPARSE      /// System-level sparse file
0x00000002: EXPLICIT_HOLES     /// Explicitly set holes (not just zeros)
0x00000004: TRIM_SUPPORTED     /// TRIM/UNMAP supported
0x00000008: THIN_PROVISIONED   /// Thin-provisioned storage
0x00000010: DEDUPLICATED       /// Deduplicated blocks
```

**Storage Format**:
- Data stored only for non-hole regions
- Holes represented in metadata, not stored
- On extraction: Create holes using SEEK_HOLE/SEEK_DATA or fallocate()

**Example**:
```
File: database.img (10 GB logical)
  Holes:
    [0x00000000-0x40000000]    (1 GB hole)
    [0x80000000-0xC0000000]    (1 GB hole)
  Data:
    [0x40000000-0x80000000]    (1 GB data)
    [0xC0000000-0x280000000]   (7 GB data)

  LogicalSize: 10 GB
  PhysicalSize: 8 GB
  HoleCount: 2
  Compression ratio: 20% (2 GB saved)
```

**Use Cases**:
- Virtual machine disk images (VMDK, VDI, QCOW2)
- Database files with sparse tables
- Large log files with gaps
- Disk dump images
- Sparse matrices

**Extraction Strategies**:
1. **POSIX systems**: Use `fallocate(FALLOC_FL_PUNCH_HOLE)` or `lseek(SEEK_HOLE)`
2. **Windows**: Use `DeviceIoControl(FSCTL_SET_SPARSE)` + `FSCTL_SET_ZERO_DATA`
3. **Fallback**: Write zeros to hole regions (inefficient)

**Compression Interaction**:
- Compress only data regions, not holes
- Holes remain holes after decompression
- Significant space savings for sparse files

### 6.66 Delta Revision Metadata (0x0046)

Stores file as delta (difference) from base revision for efficient version storage.

```c
typedef struct _ZOO64_DELTA_ATTR {
  UINT64  BaseRevisionID;     /// ID of base revision
  UINT32  DeltaFormat;        /// Delta encoding format
  UINT32  Flags;              /// Delta flags
  UINT64  BaseSize;           /// Size of base file
  UINT64  DeltaSize;          /// Size of delta data
  UINT64  TargetSize;         /// Size of reconstructed file
  UINT32  BaseHashLength;     /// Length of base file hash
  UINT32  TargetHashLength;   /// Length of target file hash
  /// Followed by:
  ///   [BaseHashLength bytes: base file hash]
  ///   [TargetHashLength bytes: target file hash]
  ///   [Delta data]
} ZOO64_DELTA_ATTR;
#pragma pack(pop)
```

**Delta Formats**:
```
0: NONE                 /// Not deltified
1: XDELTA3              /// xdelta3 algorithm
2: VCDIFF               /// RFC 3284 VCDIFF format
3: BSDIFF               /// bsdiff/bspatch
4: ZDELTA               /// zdelta format
5: RSYNC                /// rsync rolling checksum
6: GIT_DELTA            /// Git-style delta
7: FOSSIL_DELTA         /// Fossil delta compression
```

**Delta Flags**:
```
0x00000001: BASE_IN_ARCHIVE    /// Base revision in same archive
0x00000002: BASE_EXTERNAL      /// Base revision external
0x00000004: BIDIRECTIONAL      /// Can apply forward/reverse
0x00000008: COMPRESSED_DELTA   /// Delta is compressed
0x00000010: CHAIN_ALLOWED      /// Allow delta chains
0x00000020: VERIFY_HASH        /// Verify base/target hashes
```

**Delta Storage Strategies**:

1. **Full + Deltas** (Git-like):
   ```
   Version 1: Full file (100 KB)
   Version 2: Delta from v1 (5 KB)
   Version 3: Delta from v2 (3 KB)
   Version 4: Delta from v3 (4 KB)
   ```

2. **Snapshot + Deltas** (Periodic full):
   ```
   Version 1: Full (100 KB)     ← Snapshot
   Version 2: Delta from v1 (5 KB)
   Version 3: Delta from v1 (8 KB)
   Version 4: Delta from v1 (6 KB)
   Version 5: Full (120 KB)     ← Snapshot
   Version 6: Delta from v5 (4 KB)
   ```

3. **Delta Chains** (Most space-efficient):
   ```
   V1 (full) ← V2 (Δ from V1) ← V3 (Δ from V2) ← V4 (Δ from V3)

   To reconstruct V4:
   1. Extract V1 (full)
   2. Apply V2 delta → get V2
   3. Apply V3 delta → get V3
   4. Apply V4 delta → get V4
   ```

**Base Revision ID**:
- Hash-based: SHA-256 of base file
- Sequence-based: Monotonic revision number
- UUID-based: Universal unique identifier

**Compression Ratio Examples**:
```
Source code changes (C file):
  Full file: 45 KB
  Delta: 1.2 KB
  Ratio: 97.3% savings

Binary executable:
  Full file: 8 MB
  Delta: 250 KB
  Ratio: 96.9% savings

Document (small edit):
  Full file: 2 MB
  Delta: 15 KB
  Ratio: 99.2% savings
```

**Use Cases**:
- Version control systems (Git, SVN, Mercurial)
- Incremental backups
- Software updates/patches
- Document revision history
- VM snapshot chains

**Extraction Process**:
1. Locate base revision (by ID/hash)
2. Extract/decompress base file
3. Apply delta using specified format
4. Verify target hash (if VERIFY_HASH set)
5. Output reconstructed file

**Delta Chain Limits**:
- Recommended: Max 10-20 deltas in chain
- Reason: Each delta adds extraction overhead
- Solution: Periodic snapshots (full files)

**Optimization Tips**:
1. **Similar files**: Delta works best for similar content
2. **Block alignment**: Align data for better delta compression
3. **Chunk size**: Optimize for typical change patterns
4. **Hash verification**: Detect corruption early
5. **Snapshot frequency**: Balance space vs extraction speed

**Example Metadata**:
```c
// Version 2 stored as delta from Version 1
DELTA_ATTR {
  BaseRevisionID: 0xABCDEF1234567890  /// V1 hash
  DeltaFormat: XDELTA3
  Flags: BASE_IN_ARCHIVE | VERIFY_HASH
  BaseSize: 102400              /// 100 KB
  DeltaSize: 5120               /// 5 KB
  TargetSize: 104448            /// 102 KB
  BaseHash: [SHA256 of V1]
  TargetHash: [SHA256 of V2]
}
```

When extracting Version 2:
1. Find base (V1) using BaseRevisionID
2. Extract V1 (100 KB)
3. Verify V1 hash matches BaseHash
4. Apply xdelta3 patch (5 KB delta)
5. Verify result hash matches TargetHash
6. Output V2 (102 KB)

Total storage: 100 KB (V1 full) + 5 KB (V2 delta) = 105 KB
Without delta: 100 KB + 102 KB = 202 KB
Space savings: 48%

### 6.67 UDF (Universal Disk Format) Metadata (0x0047)

Preserves UDF filesystem metadata for optical media (CD/DVD/BD) and flash media.

```c
typedef struct _ZOO64_UDF_ATTR {
  UINT16  UDFRevision;        /// UDF revision (0x0102, 0x0150, 0x0200, 0x0201, 0x0250, 0x0260)
  UINT16  MinReadRevision;    /// Minimum UDF revision for reading
  UINT16  MinWriteRevision;   /// Minimum UDF revision for writing
  UINT16  MaxWriteRevision;   /// Maximum UDF revision for writing
  UINT64  UniqueID;           /// Unique ID (48-bit)
  UINT32  Flags;              /// UDF-specific flags
  UINT32  FileType;           /// UDF file type
  UINT32  PartitionNumber;    /// Partition number
  UINT64  LogicalBlockNumber; // Logical block number
  UINT32  ExtendedAttrLength; // Length of extended attributes
  UINT32  StreamDirCount;     /// Number of stream directories
  /// Followed by:
  ///   [ExtendedAttrLength bytes: UDF extended attributes]
  ///   [StreamDirCount * variable: stream directory entries]
} ZOO64_UDF_ATTR;
#pragma pack(pop)
```

**UDF Revisions**:
```
0x0102: UDF 1.02  /// DVD-Video
0x0150: UDF 1.50  /// DVD-RAM, DVD±R/RW
0x0200: UDF 2.00  /// DVD+RW, BDAV
0x0201: UDF 2.01  /// Blu-ray
0x0250: UDF 2.50  /// Blu-ray additions
0x0260: UDF 2.60  /// Latest (BD-R, flash media)
```

**UDF Flags**:
```
0x00000001: HARD_WRITE_PROTECT     /// Hardware write-protected
0x00000002: SOFT_WRITE_PROTECT     /// Software write-protected
0x00000004: REWRITABLE             /// Rewritable media
0x00000008: OVERWRITABLE           /// Overwritable once written
0x00000010: NAMED_STREAMS          /// Supports named streams
0x00000020: METADATA_PARTITION     /// Metadata partition present
0x00000040: SPARED_PARTITION       /// Sparing table present
0x00000080: VAT_PRESENT            /// Virtual allocation table
```

**UDF File Types**:
```
0: UNSPECIFIED
1: UNALLOCATED_SPACE_ENTRY
2: PARTITION_INTEGRITY_ENTRY
3: INDIRECT_ENTRY
4: DIRECTORY
5: REGULAR_FILE
6: BLOCK_SPECIAL
7: CHAR_SPECIAL
8: EXTENDED_ATTR_FILE
9: FIFO
10: SOCKET
11: TERMINAL_ENTRY
12: SYMBOLIC_LINK
13: STREAM_DIRECTORY
```

**Use Cases**: DVD-Video, Blu-ray discs, BD-R/BD-RE, flash drives with UDF

### 6.68 ISO 9660 Metadata (0x0048)

ISO 9660 with extensions (Rock Ridge, Joliet, El Torito) for CD-ROM/DVD.

```c
typedef struct _ZOO64_ISO9660_ATTR {
  UINT32  Flags;              /// ISO 9660 flags
  UINT32  ExtensionFlags;     /// Extension flags (RockRidge/Joliet)
  UINT64  LogicalBlockNumber; // Logical block number (LBA)
  UINT32  FileUnitSize;       /// File unit size
  UINT8   InterleaveGapSize;  /// Interleave gap size
  UINT16  VolumeSequenceNumber; // Volume sequence number
  UINT8   FileFlags;          /// ISO 9660 file flags
  UINT8   NameType;           /// Filename type
  UINT16  VersionNumber;      /// File version number ;n
  UINT16  RockRidgeLength;    /// Length of Rock Ridge data
  UINT16  JolietNameLength;   /// Length of Joliet name
  UINT32  ElToritoFlags;      /// El Torito boot flags
  /// Followed by:
  ///   [RockRidgeLength bytes: Rock Ridge extensions]
  ///   [JolietNameLength bytes: Unicode Joliet name]
} ZOO64_ISO9660_ATTR;
#pragma pack(pop)
```

**ISO 9660 Flags**:
```
0x00000001: LEVEL_1            /// ISO 9660 Level 1 (8.3 filenames)
0x00000002: LEVEL_2            /// ISO 9660 Level 2 (30 char names)
0x00000004: LEVEL_3            /// ISO 9660 Level 3 (non-sequential)
0x00000008: INTERCHANGE_LEVEL_4 // ISO 9660:1999
0x00000010: VERSION_2          /// ISO 9660:1999 version
```

**Extension Flags**:
```
0x00000001: ROCK_RIDGE         /// POSIX Rock Ridge extensions
0x00000002: JOLIET             /// Microsoft Joliet (Unicode)
0x00000004: EL_TORITO          /// El Torito bootable CD
0x00000008: APPLE_EXTENSIONS   /// Apple ISO 9660 extensions
0x00000010: AMIGA_EXTENSIONS   /// Amiga Rock Ridge extensions
```

**ISO 9660 File Flags**:
```
0x01: HIDDEN              /// Hidden file
0x02: DIRECTORY           /// Directory
0x04: ASSOCIATED_FILE     /// Associated file
0x08: RECORD_FORMAT       /// Record format in extended attribute
0x10: PERMISSIONS         /// Permissions in extended attribute
0x80: NOT_FINAL           /// Not final directory record
```

**Rock Ridge Extensions** (RRIP):
- **PX**: POSIX file attributes (mode, links, UID, GID)
- **PN**: POSIX device number (major/minor)
- **SL**: Symbolic link
- **NM**: Alternate name (long filename)
- **CL**: Child link (deep directory relocation)
- **PL**: Parent link
- **RE**: Relocated directory
- **TF**: Timestamps (access, modify, attributes, backup, creation, expiration)
- **SF**: Sparse file
- **SP**: System Use Sharing Protocol indicator
- **ER**: Extensions reference

**Joliet Extensions**:
- Unicode UCS-2 filenames (up to 64 chars)
- Three supplementary volume descriptors
- Path table for fast directory access

**El Torito Bootable CD**:
```
0x00000001: BOOTABLE           /// Bootable media
0x00000002: NO_EMULATION       /// No emulation mode
0x00000004: FLOPPY_1_2MB       /// 1.2 MB floppy emulation
0x00000008: FLOPPY_1_44MB      /// 1.44 MB floppy emulation
0x00000010: FLOPPY_2_88MB      /// 2.88 MB floppy emulation
0x00000020: HARD_DISK_EMULATION // Hard disk emulation
0x00000040: BOOT_INFO_TABLE    /// Boot information table present
```

**Use Cases**: CD-ROM, DVD-ROM, bootable CDs, Linux LiveCDs

### 6.69 High Sierra Metadata (0x0049)

High Sierra filesystem (predecessor to ISO 9660).

```c
typedef struct _ZOO64_HIGH_SIERRA_ATTR {
  UINT32  Flags;              /// High Sierra flags
  UINT64  LogicalBlockNumber; // Logical block number
  UINT32  ExtentLength;       /// Extent length
  UINT8   InterleaveSize;     /// Interleave file unit size
  UINT8   InterleaveSkip;     /// Interleave gap size
  UINT16  VolumeSequence;     /// Volume sequence number
  UINT8   FileFlags;          /// File flags
  UINT16  VersionNumber;      /// File version number
  UINT16  OwnerID;            /// Owner ID (if supported)
  UINT16  GroupID;            /// Group ID (if supported)
  UINT32  Reserved;           /// Reserved
} ZOO64_HIGH_SIERRA_ATTR;
#pragma pack(pop)
```

**High Sierra Flags**:
```
0x00000001: DIRECTORY_RECORD   /// Directory record
0x00000002: INTERLEAVED        /// File is interleaved
0x00000004: EXTENDED_ATTR      /// Has extended attributes
0x00000008: PERMISSIONS_PRESENT // POSIX permissions present
```

**High Sierra File Flags**:
```
0x01: HIDDEN              /// Hidden file
0x02: DIRECTORY           /// Directory entry
0x04: ASSOCIATED          /// Associated file
0x08: RECORD_FORMAT       /// Has record format
0x10: PERMISSIONS         /// Has permissions
0x80: MULTI_EXTENT        /// Multi-extent file
```

**Historical Note**:
High Sierra was the working group format (1985-1986) that became ISO 9660 (1988). Rare in modern use but may appear in archival CDs from the mid-1980s.

**Differences from ISO 9660**:
- Different magic numbers
- Different directory record format
- Limited to 8 levels of directories (ISO 9660 has 8, but enforced differently)
- No Rock Ridge or Joliet equivalents

**Use Cases**: Archival 1980s CD-ROMs, legacy systems

### 6.70 Block Deduplication Metadata (0x004A)

Content-defined chunking with hash-based block deduplication for storage optimization.

```c
typedef struct _ZOO64_DEDUP_ATTR {
  UINT32  ChunkingAlgorithm;  /// Chunking algorithm
  UINT32  HashAlgorithm;      /// Hash algorithm for dedup
  UINT32  MinChunkSize;       /// Minimum chunk size
  UINT32  AvgChunkSize;       /// Average chunk size target
  UINT32  MaxChunkSize;       /// Maximum chunk size
  UINT32  ChunkCount;         /// Number of chunks
  UINT64  LogicalSize;        /// Logical file size
  UINT64  PhysicalSize;       /// Physical size after dedup
  UINT32  Flags;              /// Deduplication flags
  UINT32  Reserved;           /// Reserved
  /// Followed by ChunkCount * sizeof(DEDUP_CHUNK) structures
} ZOO64_DEDUP_ATTR;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _DEDUP_CHUNK {
  UINT8   Hash[32];           /// Chunk hash (SHA-256 or BLAKE2b)
  UINT64  Offset;             /// Offset in logical file
  UINT32  Length;             /// Chunk length
  UINT32  RefCount;           /// Reference count (how many times chunk appears)
  /// If RefCount == 1: Unique chunk, data follows in archive
  /// If RefCount > 1: Deduplicated, points to first occurrence
} DEDUP_CHUNK;
#pragma pack(pop)
```

**Chunking Algorithms**:
```
0: FIXED_SIZE          /// Fixed-size chunks (simple but inefficient)
1: RABIN_FINGERPRINT   /// Rabin fingerprinting (CDC)
2: FASTCDC             /// FastCDC (optimized CDC)
3: GEAR_HASH           /// Gear hash-based chunking
4: BUZZHASH            /// BuzzHash rolling hash
5: SUPER_FEATURE       /// SuperFeature-based chunking
6: RSYNC_ROLLING       /// rsync rolling checksum
```

**Hash Algorithms**:
```
0: SHA256              /// SHA-256 (default, good balance)
1: SHA1                /// SHA-1 (legacy, faster but weaker)
2: BLAKE2B             /// BLAKE2b (fast, secure)
3: XXHASH64            /// xxHash (very fast, not cryptographic)
4: BLAKE3              /// BLAKE3 (fastest cryptographic)
```

**Deduplication Flags**:
```
0x00000001: GLOBAL_DEDUP       /// Global dedup across all files
0x00000002: FILE_LEVEL_DEDUP   /// Dedup within single file only
0x00000004: INLINE_DEDUP       /// Inline dedup (during archiving)
0x00000008: POST_PROCESS_DEDUP // Post-process dedup
0x00000010: COMPRESS_CHUNKS    /// Compress chunks after dedup
0x00000020: ENCRYPT_CHUNKS     /// Encrypt chunks after dedup
0x00000040: VERIFY_HASHES      /// Verify chunk hashes on read
```

**Content-Defined Chunking (CDC)**:
- Variable-size chunks based on content
- Shift-resistant: insertions don't affect other chunks
- Better dedup ratio than fixed-size

**Rabin Fingerprinting**:
- Rolling hash window (typically 48-64 bytes)
- Chunk boundary when hash % avg_chunk_size == 0
- Produces chunk sizes in range [min_size, max_size]

**Example**:
```
File: backup.tar (1 GB, highly repetitive)
  ChunkingAlgorithm: FASTCDC
  AvgChunkSize: 64 KB
  ChunkCount: 16384

  After dedup:
  Unique chunks: 2048 (12.5% of total)
  Logical size: 1 GB
  Physical size: 128 MB
  Dedup ratio: 87.5%
```

**Use Cases**:
- Backup systems (daily incrementals)
- Virtual machine images (similar VMs)
- Source code repositories (many similar files)
- Document archives (templates, boilerplate)
- Container images (shared layers)

**Storage Strategy**:
1. Chunk file using CDC algorithm
2. Hash each chunk (SHA-256/BLAKE2b)
3. Check if hash exists in dedup table
4. If exists: Store reference only
5. If new: Store chunk data + add to table

**Extraction Process**:
1. Read dedup metadata
2. For each chunk:
   - If RefCount == 1: Read chunk data directly
   - If RefCount > 1: Lookup chunk by hash in dedup pool
3. Reconstruct file by concatenating chunks

**Dedup Pool Structure**:
```
[Dedup Pool Header]
[Chunk Hash → Data Offset Map]
[Chunk Data Block 1]
[Chunk Data Block 2]
...
[Chunk Data Block N]
```

**Optimization**:
- **Compression**: Apply after chunking (better ratio)
- **Encryption**: Encrypt individual chunks (allows dedup of encrypted data if same key)
- **Reference counting**: Track chunk usage for garbage collection

**Dedup Ratios** (typical):
```
VM backups: 50-90% dedup
Source code: 30-60% dedup
Documents: 40-70% dedup
Databases: 10-30% dedup
Media files: 5-10% dedup (low, already compressed)
```

**Example Metadata**:
```c
// File with high duplication (e.g., VM snapshot)
DEDUP_ATTR {
  ChunkingAlgorithm: FASTCDC
  HashAlgorithm: SHA256
  AvgChunkSize: 65536        /// 64 KB
  ChunkCount: 1000
  LogicalSize: 67108864      /// 64 MB
  PhysicalSize: 16777216     /// 16 MB (75% dedup)
  Flags: GLOBAL_DEDUP | COMPRESS_CHUNKS

  Chunks:
  [0]: Hash=ABC..., Offset=0, Length=65432, RefCount=1    /// Unique
  [1]: Hash=DEF..., Offset=65432, Length=66102, RefCount=5 // Dedup 5x
  [2]: Hash=ABC..., Offset=131534, Length=65432, RefCount=1 // Ref to [0]
  ...
}
```

Benefits:
- **Space savings**: 50-90% reduction for repetitive data
- **Bandwidth**: Reduced network transfer for backups
- **Incremental**: Only new chunks stored
- **Scalable**: Works at file, archive, or global level

### 6.71 Reserved Filename Metadata (0x004B)

Handles Windows/DOS/OS2 reserved device names that cannot be used as regular filenames.

```c
typedef struct _ZOO64_RESERVED_NAME_ATTR {
  UINT8   ReservedType;       /// Type of reserved name
  UINT8   Platform;           /// Platform: 0=DOS, 1=Windows, 2=OS2, 3=Multi
  UINT16  OriginalNameLength; // Length of original (unsafe) name
  UINT16  SafeNameLength;     /// Length of safe alternative name
  UINT32  Flags;              /// Reserved name flags
  /// Followed by:
  ///   [OriginalNameLength bytes: original name (e.g., "CON")]
  ///   [SafeNameLength bytes: safe alternative (e.g., "CON_")]
} ZOO64_RESERVED_NAME_ATTR;
#pragma pack(pop)
```

**Reserved Device Names** (case-insensitive):

**Class 1: Character Devices** (no extension allowed)
```
CON     /// Console (input/output)
PRN     /// Printer (parallel port, same as LPT1)
AUX     /// Auxiliary (serial port, same as COM1)
NUL     /// Null device (discard output)
CLOCK$  /// System clock (DOS/OS2)
KBD$    /// Keyboard (OS/2)
SCREEN$ // Screen (OS/2)
POINTER$// Mouse pointer (OS/2)
```

**Class 2: Serial Ports** (COM1-COM9, COM0 on some systems)
```
COM1, COM2, COM3, COM4, COM5, COM6, COM7, COM8, COM9
COM¹, COM², COM³ (Unicode superscripts also reserved on NT)
```

**Class 3: Parallel Ports** (LPT1-LPT9)
```
LPT1, LPT2, LPT3, LPT4, LPT5, LPT6, LPT7, LPT8, LPT9
LPT¹, LPT², LPT³ (Unicode superscripts also reserved on NT)
```

**Class 4: Special OS/2 Devices**
```
\DEV\CON
\DEV\NUL
\DEV\PRN
\DEV\AUX
\DEV\COMn
\DEV\LPTn
\PIPE\*   /// Named pipes
\QUEUES\* // Named queues
```

**Important Rules**:
1. **Case insensitive**: `CON`, `Con`, `con` all reserved
2. **Extension ignored**: `CON.txt`, `CON.doc` still reserved
3. **Whitespace trimmed**: `CON ` and ` CON` both reserved (Windows)
4. **Path irrelevant**: `C:\dir\CON` reserved regardless of path
5. **Wildcard matching**: `COM[1-9]`, `LPT[1-9]` patterns

**Reserved Type Values**:
```
0: UNKNOWN
1: CHARACTER_DEVICE    /// CON, PRN, AUX, NUL
2: SERIAL_PORT         /// COM1-COM9
3: PARALLEL_PORT       /// LPT1-LPT9
4: CLOCK_DEVICE        /// CLOCK$
5: KEYBOARD_DEVICE     /// KBD$
6: SCREEN_DEVICE       /// SCREEN$
7: POINTER_DEVICE      /// POINTER$
8: PIPE_NAMESPACE      /// \PIPE\*
9: QUEUE_NAMESPACE     /// \QUEUES\*
10: DEV_NAMESPACE      /// \DEV\*
```

**Platform Values**:
```
0: DOS              /// MS-DOS, PC DOS, DR-DOS
1: WINDOWS          /// Windows 3.x/9x/NT/2000/XP/Vista/7/8/10/11
2: OS2              /// OS/2 1.x/2.x/Warp
3: MULTI_PLATFORM   /// Reserved on multiple platforms
```

**Reserved Name Flags**:
```
0x00000001: EXTENSION_IRRELEVANT   /// Extension doesn't matter (CON.txt = CON)
0x00000002: CASE_INSENSITIVE       /// Case doesn't matter
0x00000004: WHITESPACE_TRIMMED     /// Trailing whitespace ignored
0x00000008: PATH_IRRELEVANT        /// Path doesn't matter
0x00000010: WILDCARD_MATCH         /// Matches pattern (COM[1-9])
0x00000020: NAMESPACE_PREFIX       /// Uses \DEV\ or \PIPE\ prefix
0x00000040: LEGACY_DOS             /// DOS-era device
0x00000080: WIN32_NAMESPACE        /// Win32 namespace (\\?\, \\.\)
0x00000100: UNC_PATH_SAFE          /// Safe in UNC path (\\server\share\CON may work)
```

**Detection Algorithm**:
```c
bool is_reserved_name(const char* filename) {
    /// Extract basename (strip path and extension)
    char base[256];
    extract_basename(filename, base);

    /// Trim trailing whitespace (Windows behavior)
    trim_whitespace(base);

    /// Convert to uppercase for comparison
    to_uppercase(base);

    /// Check Class 1: Character devices
    if (strcmp(base, "CON") == 0 ||
        strcmp(base, "PRN") == 0 ||
        strcmp(base, "AUX") == 0 ||
        strcmp(base, "NUL") == 0 ||
        strcmp(base, "CLOCK$") == 0 ||
        strcmp(base, "KBD$") == 0 ||
        strcmp(base, "SCREEN$") == 0 ||
        strcmp(base, "POINTER$") == 0) {
        return true;
    }

    /// Check Class 2: Serial ports (COM1-COM9)
    if (strlen(base) == 4 &&
        base[0] == 'C' && base[1] == 'O' && base[2] == 'M' &&
        base[3] >= '1' && base[3] <= '9') {
        return true;
    }

    /// Check Class 3: Parallel ports (LPT1-LPT9)
    if (strlen(base) == 4 &&
        base[0] == 'L' && base[1] == 'P' && base[2] == 'T' &&
        base[3] >= '1' && base[3] <= '9') {
        return true;
    }

    /// Check Class 4: OS/2 namespace prefixes
    if (strncmp(filename, "\\DEV\\", 5) == 0 ||
        strncmp(filename, "\\PIPE\\", 6) == 0 ||
        strncmp(filename, "\\QUEUES\\", 8) == 0) {
        return true;
    }

    return false;
}
```

**Safe Name Generation Strategies**:

1. **Suffix underscore** (simple, preserves readability):
   - `CON` → `CON_`
   - `LPT1` → `LPT1_`
   - `NUL.txt` → `NUL_.txt`

2. **Prefix underscore** (alternate):
   - `CON` → `_CON`
   - `LPT1` → `_LPT1`

3. **Unicode replacement** (preserves length):
   - `CON` → `CОN` (Cyrillic O: U+041E)
   - `NUL` → `ΝUL` (Greek N: U+039D)

4. **Percent encoding** (URL-style):
   - `CON` → `%43ON` or `CO%4E`
   - Reversible and unambiguous

5. **Win32 namespace** (Windows-specific):
   - `CON` → `\\?\C:\path\CON`
   - Works on NTFS but requires special handling

6. **Extension prefix** (if extension present):
   - `CON.txt` → `CON_.txt`
   - `NUL.log` → `NUL_.log`

**Recommended Strategy**: Suffix underscore (Strategy 1)
- Simple and reversible
- Human-readable
- Works on all platforms
- Easy to detect and reverse

**Archiving Behavior**:

When encountering a reserved name during archiving:

1. **Detection**: Check if filename is reserved
2. **Recording**: Store original name in `OriginalNameLength` field
3. **Safe alternative**: Generate safe name (e.g., `CON_`)
4. **Metadata**: Set `ReservedType`, `Platform`, and `Flags`
5. **Path normalization**: Use safe name in normalized path
6. **Preservation**: Keep original for round-trip to compatible platform

**Extraction Behavior**:

When extracting to different platforms:

**Extracting to Windows/DOS/OS2**:
- Use safe alternative name (e.g., `CON_`)
- Or use Win32 namespace (`\\?\C:\path\CON`)
- Warn user about name change

**Extracting to POSIX (Linux/macOS/BSD)**:
- Use original name (e.g., `CON`) - safe on POSIX
- No warning needed

**Extracting to other systems**:
- Check if name conflicts with that system's reserved names
- Apply appropriate strategy

**Round-Trip Preservation**:

Windows → Archive → Windows:
```
Original:    C:\Users\test\CON.txt (error: cannot create)
Archived as: CON_ (with metadata preserving "CON")
Extracted:   C:\Users\test\CON_.txt (safe)
```

POSIX → Archive → POSIX:
```
Original:    /home/user/CON (valid on POSIX)
Archived as: CON (no metadata needed)
Extracted:   /home/user/CON (identical)
```

POSIX → Archive → Windows:
```
Original:    /home/user/CON
Archived as: CON (with metadata warning)
Extracted:   C:\Users\CON_.txt (safe on Windows)
```

**Example Metadata**:
```c
// File originally named "CON.txt" on POSIX system
RESERVED_NAME_ATTR {
  ReservedType: CHARACTER_DEVICE
  Platform: WINDOWS
  OriginalNameLength: 3
  SafeNameLength: 4
  Flags: EXTENSION_IRRELEVANT | CASE_INSENSITIVE | WHITESPACE_TRIMMED
  OriginalName: "CON"
  SafeName: "CON_"
}
```

**Testing Reserved Names**:

On Windows, these commands will fail:
```batch
echo test > CON        REM Writes to console, not file
echo test > NUL        REM Discards output
echo test > LPT1.txt   REM Attempts to write to parallel port
type CON               REM Reads from console
```

Attempting to create these files via API will return `ERROR_INVALID_NAME` or `ERROR_FILE_EXISTS`.

**Historical Note**:
These reserved names originate from CP/M (1974) and were carried forward to MS-DOS (1981) for compatibility. Windows inherited them from DOS, and they persist even in modern Windows 11 for backward compatibility.

**Security Consideration**:
Attackers may use reserved names to cause denial-of-service:
- Archive with `CON` extracts and hangs extraction tool (reads from console)
- Archive with `NUL` silently discards data
- Archive with `LPT1` attempts to access parallel port (privilege escalation)

Zoo64 implementations **must** detect and handle reserved names safely.

### 6.72 DR-DOS File Password Metadata (0x004C)

DR-DOS (Digital Research DOS) featured built-in file-level password protection at the filesystem level, a unique capability not present in MS-DOS. This metadata preserves DR-DOS password information for archives containing password-protected files.

```c
typedef struct _ZOO64_DRDOS_PASSWORD_ATTR {
  UINT8   PasswordType;       /// Password type: 0=None, 1=Read, 2=Write, 3=Read+Write
  UINT8   HashAlgorithm;      /// Hash algorithm: 0=Original DR-DOS, 1=MD5, 2=SHA-256
  UINT16  HashLength;         /// Length of password hash
  UINT32  Flags;              /// DR-DOS password flags
  UINT32  Reserved;           /// Reserved for future use
  /// Followed by:
  ///   [HashLength bytes: password hash]
  ///   [Optional: salt for modern hash algorithms]
} ZOO64_DRDOS_PASSWORD_ATTR;
#pragma pack(pop)
```

**Password Type Values**:
```
0: NONE              /// No password protection
1: READ_PROTECTED    /// Password required to read file
2: WRITE_PROTECTED   /// Password required to write/modify file
3: READ_WRITE        /// Password required for both read and write
```

**Hash Algorithm Values**:
```
0: DRDOS_ORIGINAL    /// Original DR-DOS password algorithm (weak, for compatibility)
1: MD5               /// MD5 hash (better, still weak by modern standards)
2: SHA256            /// SHA-256 hash (recommended for modern archives)
```

**DR-DOS Password Flags**:
```
0x00000001: PASSWORD_ENCRYPTED      /// File data is encrypted with password
0x00000002: PASSWORD_HASH_ONLY      /// Only hash stored (data not encrypted)
0x00000004: CASE_SENSITIVE          /// Password is case-sensitive
0x00000008: CASE_INSENSITIVE        /// Password is case-insensitive (DR-DOS default)
0x00000010: LEGACY_COMPATIBILITY    /// Use DR-DOS 3.x/5.x/6.x compatibility mode
0x00000020: SALT_PRESENT            /// Hash includes salt
0x00000040: ENHANCED_SECURITY       /// Use enhanced security (multiple rounds)
```

**DR-DOS Password System**:

DR-DOS versions 3.40 and later supported password protection at the filesystem level:

1. **Read passwords**: Required to open file for reading
2. **Write passwords**: Required to open file for writing
3. **Delete passwords**: Required to delete file
4. **Both passwords**: Separate read/write passwords

The passwords were stored in the directory entry's reserved bytes, making them filesystem-level protection rather than application-level.

**Original Algorithm** (DR-DOS 3.x-6.x):
```c
// Simplified DR-DOS password hash (original algorithm)
uint16_t drdos_password_hash(const char* password) {
    uint16_t hash = 0;
    for (const char* p = password; *p; p++) {
        hash = ((hash << 1) | (hash >> 15)) ^ toupper(*p);
    }
    return hash;
}
```

**Security Note**: The original DR-DOS password algorithm was simple and not cryptographically secure. For archival purposes, Zoo64 can store the original hash for compatibility, but should also store a modern hash (SHA-256) and optionally encrypt the file data using proper encryption (see Section 6a).

**Archiving Behavior**:

When archiving DR-DOS password-protected files:

1. **Detect password**: Check DR-DOS directory entry for password bytes
2. **Extract hash**: Read password hash from directory entry
3. **Store metadata**: Create DRDOS_PASSWORD_ATTR metadata chunk
4. **Optional encryption**: If file data was encrypted, include encryption metadata (0x004E)
5. **Compatibility**: Store both original hash and modern hash for round-trip

**Extraction Behavior**:

When extracting to different platforms:

**Extracting to DR-DOS**:
- Restore password hash to directory entry
- Preserve read/write protection flags
- Maintain compatibility with DR-DOS filesystem

**Extracting to other DOS variants (MS-DOS, PC DOS)**:
- Cannot restore passwords (not supported)
- Warn user that password protection will be lost
- Extract file normally without protection

**Extracting to modern systems (Windows/Linux/macOS)**:
- Cannot restore DR-DOS passwords
- If file data is encrypted, require password for decryption
- Extract as normal file

**Round-Trip Preservation**:

DR-DOS → Archive → DR-DOS:
```
Original:    PASSWORD.DAT (read password: "secret", write password: "admin")
Archived as: PASSWORD.DAT with DRDOS_PASSWORD_ATTR
             PasswordType: READ_WRITE
             HashAlgorithm: DRDOS_ORIGINAL
             Hash: 0xA5C3 (read), 0x7B21 (write)
Extracted:   PASSWORD.DAT (passwords restored to directory entry)
```

**Example Metadata**:
```c
// File with DR-DOS read password
DRDOS_PASSWORD_ATTR {
  PasswordType: READ_PROTECTED
  HashAlgorithm: SHA256
  HashLength: 32
  Flags: CASE_INSENSITIVE | SALT_PRESENT
  Reserved: 0
  Hash: [32 bytes SHA-256 hash]
  Salt: [16 bytes random salt]
}
```

**Historical Context**:

DR-DOS was developed by Digital Research as a competitor to MS-DOS. Its password protection feature was one of several enhancements over MS-DOS, along with:
- Task switching (DR-DOS 5.0, 1990)
- Memory management improvements
- Disk compression (SuperStor/Stacker integration)
- Network support

Novell acquired Digital Research in 1991 and continued DR-DOS development through version 7.x. Later versions (OpenDOS, Caldera DR-DOS, DRDOS 8.x) maintained password compatibility.

**Compatibility Matrix**:

| DR-DOS Version | Password Support | Algorithm         | Notes                     |
|----------------|------------------|-------------------|---------------------------|
| 3.40           | Read/Write       | 16-bit hash       | First version with passwords |
| 3.41           | Read/Write       | 16-bit hash       | Improved algorithm        |
| 5.0            | Read/Write/Del   | 16-bit hash       | Added delete passwords    |
| 6.0            | Read/Write/Del   | 16-bit hash       | Enhanced security mode    |
| 7.x (Novell)   | Read/Write/Del   | 16-bit hash       | Maintained compatibility  |
| 8.x (Caldera)  | Read/Write/Del   | 16-bit hash       | Legacy support            |

### 6.73 CP/M USER DIR Metadata (0x004D)

CP/M (Control Program for Microcomputers) used a USER number system (0-15) to provide basic multi-user directory separation. Each file had an associated USER number, and only files matching the current USER number were visible. This metadata preserves CP/M USER directory information.

```c
typedef struct _ZOO64_CPM_USER_ATTR {
  UINT8   UserNumber;         /// CP/M USER number (0-15)
  UINT8   SystemAttribute;    /// System file attribute (visible to all users if set)
  UINT8   ReadOnly;           /// Read-only attribute
  UINT8   ArchiveFlag;        /// Archive flag (for backup software)
  UINT32  ExtentNumber;       /// Extent number (for large files split across extents)
  UINT32  RecordCount;        /// Number of 128-byte records in extent
  UINT16  BlockAllocation[8]; // CP/M block allocation map
  UINT32  Flags;              /// CP/M flags
} ZOO64_CPM_USER_ATTR;
#pragma pack(pop)
```

**User Number Values**:
```
0-15: Valid USER numbers
  0:  Default user (visible when USER 0 is active)
  1-15: Additional users
255: Special marker for deleted files (internal CP/M use)
```

**CP/M Attributes**:
```
0x01: READ_ONLY      /// File cannot be modified or deleted
0x02: SYSTEM         /// System file (visible regardless of USER number)
0x04: ARCHIVE        /// Archive flag (set when file modified)
0x80: REQUIRES_PASSWORD // File requires password (some CP/M variants)
```

**CP/M USER System**:

The CP/M USER number system provided primitive multi-user file organization:

1. **USER command**: Switch between USER numbers (USER 0 through USER 15)
2. **File visibility**: Only files with matching USER number are visible
3. **System files**: Files with System attribute (F2) visible to all users
4. **Cross-user access**: Special utilities could access files across USER numbers

**Example CP/M Session**:
```
A>USER 0              ; Switch to USER 0
A>DIR                 ; Shows only USER 0 files + system files
A>USER 5              ; Switch to USER 5
A>DIR                 ; Shows only USER 5 files + system files
A>USER 0              ; Back to USER 0
A>DIR B: [USER 5]     ; Special syntax to view USER 5 files on drive B:
```

**CP/M Extent System**:

CP/M files larger than the maximum extent size were split across multiple directory entries (extents). Each extent contained up to 16K bytes (128 records of 128 bytes each).

**Extent Structure**:
- **Extent Number**: Sequential number (0, 1, 2, ...) for file parts
- **Record Count**: Number of 128-byte records in this extent
- **Block Allocation**: Map of disk blocks used by this extent

**Archiving Behavior**:

When archiving CP/M files:

1. **Read USER number**: Extract from directory entry byte 0
2. **Read attributes**: Extract System/ReadOnly/Archive bits from filename bytes
3. **Read extent info**: Extract extent number and record count
4. **Combine extents**: Merge multiple extents into single file
5. **Store metadata**: Create CPM_USER_ATTR metadata chunk

**Extraction Behavior**:

**Extracting to CP/M**:
- Restore USER number to directory entry
- Restore System/ReadOnly/Archive attributes
- Split large files into extents if necessary
- Recreate block allocation map

**Extracting to other systems**:
- Map USER number to directory: `/user0/`, `/user5/`, etc.
- Preserve attributes as file permissions (ReadOnly → chmod 444)
- Extract as normal file (no extent splitting needed)

**USER Number Directory Mapping**:
```
CP/M USER 0  → /user0/  or /default/
CP/M USER 1  → /user1/
CP/M USER 5  → /user5/
...
CP/M USER 15 → /user15/
```

**Round-Trip Preservation**:

CP/M → Archive → CP/M:
```
Original:    USER 5: REPORT.TXT (read-only, system file)
             Directory entry: User=5, Attrs=0x03 (RO|SYS)
Archived as: REPORT.TXT with CPM_USER_ATTR
             UserNumber: 5
             SystemAttribute: 1
             ReadOnly: 1
Extracted:   USER 5: REPORT.TXT (attributes restored)
```

CP/M → Archive → Modern OS:
```
Original:    USER 5: REPORT.TXT
Archived as: REPORT.TXT with CPM_USER_ATTR (UserNumber: 5)
Extracted:   /user5/REPORT.TXT (or REPORT.TXT with xattr: user.cpm.user=5)
```

**Example Metadata**:
```c
// CP/M file in USER 5, system file, read-only
CPM_USER_ATTR {
  UserNumber: 5
  SystemAttribute: 1     /// Visible to all users
  ReadOnly: 1            /// Cannot be modified
  ArchiveFlag: 0         /// Not modified since last backup
  ExtentNumber: 0        /// First extent
  RecordCount: 64        /// 64 * 128 = 8192 bytes
  BlockAllocation: [0x0010, 0x0011, 0x0012, 0x0013, 0x0000, ...]
  Flags: 0
}
```

**CP/M Flags**:
```
0x00000001: MULTI_EXTENT        /// File spans multiple extents
0x00000002: PASSWORD_PROTECTED  /// File requires password (CP/M Plus)
0x00000004: TIMESTAMP_PRESENT   /// File has timestamps (CP/M Plus, DateStamper)
0x00000008: FILE_LABEL          /// Special file label entry
```

**Historical Context**:

CP/M was created by Gary Kildall at Digital Research in 1974 and became the dominant microcomputer operating system in the late 1970s. The USER number system was a simple but effective way to provide file organization on single-user systems.

**CP/M Versions and USER Support**:

| Version  | Year | USER Support | Max Users | Notes                        |
|----------|------|--------------|-----------|------------------------------|
| CP/M 1.x | 1974 | No           | 1         | No USER number support       |
| CP/M 2.x | 1979 | Yes          | 16        | USER 0-15 supported          |
| CP/M 3.x | 1983 | Yes          | 16        | CP/M Plus, enhanced features |
| CP/M-86  | 1981 | Yes          | 16        | 8086/8088 version            |

**Compatibility Notes**:
- MP/M (Multi-user CP/M) used true user accounts instead of USER numbers
- CP/NET added network file access across USER numbers
- Some CP/M clones (TurboDOS, CDOS) extended USER numbers beyond 15

### 6.74 Olivetti pcos Metadata (0x004E)

Olivetti pcos (Personal Computer Operating System) was the operating system used on Olivetti M20 and similar personal computers in the early 1980s. pcos had unique features for file protection, versioning, and structured files.

```c
typedef struct _ZOO64_PCOS_ATTR {
  UINT8   FileClass;          /// File class: 0=Temporary, 1=Permanent, 2=System
  UINT8   ProtectionLevel;    /// Protection level (0-7)
  UINT8   VersionNumber;      /// File version number (1-255)
  UINT8   GenerationNumber;   /// File generation number
  UINT16  FileOrganization;   /// File organization type
  UINT16  RecordLength;       /// Logical record length (for structured files)
  UINT32  RecordCount;        /// Number of logical records
  UINT32  AllocationSize;     /// Allocated size (may be larger than used)
  UINT32  Flags;              /// pcos-specific flags
  UINT32  OwnerID;            /// Owner user ID
  UINT32  GroupID;            /// Group ID
  UINT64  CreationTime;       /// pcos creation timestamp
  UINT64  ExpirationTime;     /// File expiration time (0 = no expiration)
  /// Followed by:
  ///   [Optional: structured file header]
  ///   [Optional: index structure for indexed files]
} ZOO64_PCOS_ATTR;
#pragma pack(pop)
```

**File Class Values**:
```
0: TEMPORARY    /// Temporary file (deleted on system restart)
1: PERMANENT    /// Permanent file (normal file)
2: SYSTEM       /// System file (protected, elevated access)
3: SHARED       /// Shared file (multi-user access)
```

**Protection Level** (0-7, octal-style like Unix):
```
0: No access
1: Execute only
2: Write only
3: Write + Execute
4: Read only
5: Read + Execute
6: Read + Write
7: Read + Write + Execute (full access)
```

**File Organization Types**:
```
0: SEQUENTIAL           /// Sequential access (like tape)
1: RELATIVE             /// Direct access by record number
2: INDEXED_SEQUENTIAL   /// ISAM (Indexed Sequential Access Method)
3: KEYED                /// Keyed access (B-tree index)
4: STREAM               /// Byte stream (like Unix)
```

**pcos Flags**:
```
0x00000001: VERSIONING_ENABLED    /// File uses version numbers
0x00000002: EXPIRATION_SET        /// File has expiration date
0x00000004: BACKUP_REQUIRED       /// File should be backed up
0x00000008: ARCHIVE_BIT           /// File modified since last backup
0x00000010: LOCKED                /// File is locked (in use)
0x00000020: STRUCTURED_FILE       /// File has structured records
0x00000040: INDEXED_FILE          /// File has index structure
0x00000080: COMPRESSED            /// File is compressed by pcos
0x00000100: ENCRYPTED             /// File is encrypted by pcos
0x00000200: REMOTE_FILE           /// File is on remote system (pcos networking)
```

**pcos Versioning System**:

Olivetti pcos supported automatic file versioning similar to VMS:

1. **Version numbers**: Files had explicit version numbers (FILE.TXT;1, FILE.TXT;2, etc.)
2. **Automatic versioning**: Opening file for write created new version
3. **Version retention**: System could retain multiple versions
4. **Version purge**: Old versions could be purged to save space

**Example pcos Versioning**:
```
$ CREATE FILE.TXT           ; Creates FILE.TXT;1
$ EDIT FILE.TXT             ; Creates FILE.TXT;2 (new version)
$ EDIT FILE.TXT             ; Creates FILE.TXT;3
$ DIR FILE.TXT
  FILE.TXT;3  1024 bytes  2025-01-15 10:30
  FILE.TXT;2   512 bytes  2025-01-14 14:20
  FILE.TXT;1   256 bytes  2025-01-13 09:00
$ PURGE FILE.TXT /KEEP=1    ; Delete all but latest version
```

**Structured File Support**:

pcos supported structured files with fixed-length or variable-length records:

1. **Sequential files**: Records accessed sequentially
2. **Relative files**: Records accessed by record number (like array)
3. **Indexed files**: Records accessed by key (like database)
4. **Keyed files**: Multiple indexes per file (B-tree)

**Archiving Behavior**:

When archiving pcos files:

1. **Read file class**: Extract from pcos directory entry
2. **Read protection**: Extract protection level
3. **Read version info**: Extract version and generation numbers
4. **Read organization**: Determine file structure type
5. **Store metadata**: Create PCOS_ATTR metadata chunk
6. **Preserve structure**: For structured files, preserve record boundaries

**Extraction Behavior**:

**Extracting to pcos**:
- Restore file class and protection level
- Restore version and generation numbers
- Restore file organization type
- Recreate index structures for indexed files
- Set owner/group IDs

**Extracting to modern systems**:
- Map file class to file attributes
- Map protection level to Unix permissions:
  ```
  pcos 7 (RWX) → Unix 0700
  pcos 6 (RW)  → Unix 0600
  pcos 4 (R)   → Unix 0400
  ```
- Store version number in filename: `FILE.TXT` → `FILE.TXT.v3`
- Convert structured files to plain files (lose record structure)

**Round-Trip Preservation**:

pcos → Archive → pcos:
```
Original:    REPORT.TXT;3 (Permanent, Protection=6, Indexed file)
Archived as: REPORT.TXT with PCOS_ATTR
             FileClass: PERMANENT
             ProtectionLevel: 6 (RW)
             VersionNumber: 3
             FileOrganization: INDEXED_SEQUENTIAL
Extracted:   REPORT.TXT;3 (all attributes restored)
```

pcos → Archive → Unix/Linux:
```
Original:    REPORT.TXT;3
Archived as: REPORT.TXT with PCOS_ATTR (VersionNumber: 3)
Extracted:   REPORT.TXT.v3 or REPORT.TXT (with xattr: user.pcos.version=3)
             Permissions: 0600 (from ProtectionLevel: 6)
```

**Example Metadata**:
```c
// pcos indexed file with versioning
PCOS_ATTR {
  FileClass: PERMANENT
  ProtectionLevel: 6              /// Read + Write
  VersionNumber: 5                /// Version 5 of file
  GenerationNumber: 1             /// First generation
  FileOrganization: INDEXED_SEQUENTIAL
  RecordLength: 256               /// 256-byte records
  RecordCount: 1024               /// 1024 records = 256KB
  AllocationSize: 262144          /// Allocated space
  Flags: VERSIONING_ENABLED | INDEXED_FILE | BACKUP_REQUIRED
  OwnerID: 42                     /// User 42
  GroupID: 10                     /// Group 10
  CreationTime: [timestamp]
  ExpirationTime: 0               /// No expiration
  /// Followed by index structure...
}
```

**Historical Context**:

Olivetti M20 was released in 1982 and used a Zilog Z8000 processor. Unlike most personal computers of the era (which used 8080, Z80, or 8086), the M20 required a unique operating system. pcos was developed by Olivetti and featured:

- Multi-user support (via terminal connections)
- File versioning (similar to DEC VMS)
- Structured file support (like COBOL/RPG record files)
- Protection levels (more sophisticated than DOS)
- File expiration (automatic cleanup)
- Built-in networking (pcos-NET)

**Olivetti pcos Features**:

| Feature              | Description                                | Similar to          |
|----------------------|--------------------------------------------|--------------------|
| File versioning      | Automatic version numbers                  | VMS, TOPS-20       |
| Structured files     | ISAM, keyed access                         | COBOL, RPG         |
| Protection levels    | 0-7 access control                         | Unix permissions   |
| File expiration      | Automatic deletion after date              | VMS expiration     |
| File classes         | Temporary/Permanent/System                 | VMS file types     |
| Owner/Group IDs      | Multi-user ownership                       | Unix UID/GID       |

**Compatibility Notes**:
- Olivetti M20 also ran MS-DOS via Z8000-to-8086 emulation (slower)
- pcos files could be exported to MS-DOS format (losing structure)
- Later Olivetti systems (M24, M28) used standard MS-DOS

**Security Considerations**:
- Protection levels should be mapped to modern ACLs on extraction
- Expired files should be flagged or automatically removed
- Encrypted pcos files should trigger Zoo64 encryption metadata

### 6.75 WIM (Windows Imaging Format) Metadata (0x004F)

WIM is Microsoft's file-based disk image format used for Windows deployment. Zoo64 can store WIM-specific metadata for archives that need to preserve WIM semantics or create WIM-compatible images.

```c
#pragma pack(push, 1)
typedef struct _ZOO64_WIM_ATTR {
  UINT32  WIMVersion;         /// WIM format version (typically 0x00010D00)
  UINT32  ImageIndex;         /// Image index within WIM (1-based)
  UINT32  ImageCount;         /// Total number of images
  UINT32  BootIndex;          /// Boot image index (0 if not bootable)
  UINT64  ImageFlags;         /// WIM image flags
  UINT32  CompressionType;    /// WIM compression type
  UINT32  ChunkSize;          /// Compression chunk size (32KB default)
  UINT16  ImageNameLength;    /// Length of image name
  UINT16  ImageDescLength;    /// Length of image description
  UINT32  Reserved;           /// Reserved for future use
  /// Followed by:
  ///   [ImageNameLength bytes: image name]
  ///   [ImageDescLength bytes: image description]
} ZOO64_WIM_ATTR;
#pragma pack(pop)
```

**WIM Version Values**:
```
0x00010D00: WIM 1.13 (Windows Vista/7/8/10/11)
0x000E0000: WIM 14.0  (Windows 8.1)
```

**WIM Image Flags**:
```
0x00000001: COMPRESSION         /// Image uses compression
0x00000002: READONLY            /// Image is read-only
0x00000004: SPANNED             /// Image spans multiple files
0x00000008: RESOURCE_ONLY       /// Image contains only resources
0x00000010: METADATA_ONLY       /// Image contains only metadata
0x00000020: SINGLE_INSTANCE     /// Use single-instance storage
0x00000040: BOOTABLE            /// Image is bootable
0x00000080: INTEGRITY_CHECK     /// Image has integrity table
0x00000100: DEDUPLICATED        /// Image uses chunk deduplication
0x00000200: PIPEABLE            /// Image supports pipeable WIM
```

**WIM Compression Types**:
```
0: NONE           /// No compression (stored)
1: XPRESS         /// LZ77-based (fast)
2: LZX            /// LZ77+Huffman (better ratio)
3: LZMS           /// LZMA-like (best ratio, Windows 8+)
```

**WIM Features**:

1. **Single-Instance Storage**:
   - Files with identical content stored once
   - All instances reference the same resource
   - Implemented via file hash lookup

2. **Resource-Based Storage**:
   - Files stored as resources (chunks)
   - Resources can be shared across images
   - Efficient for multiple OS images

3. **Bootable Images**:
   - Can contain Windows Boot Manager
   - Supports BIOS and UEFI boot
   - Boot configuration in boot.wim

4. **Integrity Tables**:
   - SHA-1 checksums for verification
   - Optional integrity checking

### 6.76 WIM Resource Metadata (0x0050)

WIM uses resource-based storage where files reference shared resource chunks. This metadata tracks resource information.

```c
#pragma pack(push, 1)
typedef struct _ZOO64_WIM_RESOURCE_ATTR {
  UINT8   ResourceHash[20];   /// SHA-1 hash of resource (WIM standard)
  UINT64  ResourceOffset;     /// Offset to resource data
  UINT64  ResourceSize;       /// Size of resource (compressed)
  UINT64  OriginalSize;       /// Original size (uncompressed)
  UINT32  RefCount;           /// Number of files referencing this resource
  UINT32  PartNumber;         /// Part number (for split WIM)
  UINT32  Flags;              /// Resource flags
  UINT32  Reserved;           /// Reserved
} ZOO64_WIM_RESOURCE_ATTR;
#pragma pack(pop)
```

**Resource Flags**:
```
0x00000001: COMPRESSED         /// Resource is compressed
0x00000002: FREE               /// Resource is free/deleted
0x00000004: METADATA           /// Resource contains metadata
0x00000008: SPANNED            /// Resource spans multiple parts
0x00000010: DEDUPLICATED       /// Resource uses chunk deduplication
```

**Single-Instance Storage Algorithm**:
```c
// When adding file to WIM-style archive:
1. Calculate SHA-1 hash of file content
2. Look up hash in resource table
3. If found:
   a. Increment RefCount
   b. Store only reference to existing resource
4. If not found:
   a. Compress file content
   b. Store as new resource
   c. Add hash to resource table
   d. Set RefCount = 1
```

**Benefits**:
- **Space savings**: Eliminates duplicate files
- **Performance**: Single read for multiple instances
- **Deduplication**: Automatic at file level

**Example** (Windows installation with multiple editions):
```
Windows 10 Home:      3.5 GB
Windows 10 Pro:       3.6 GB
Windows 10 Enterprise: 3.7 GB
---
Total without SIS:   10.8 GB
Total with SIS:       4.2 GB (system files shared)
Savings:              61%
```

### 6.77 WIM Boot Metadata (0x0051)

For bootable WIM images (like boot.wim or install.wim), this metadata stores boot configuration.

```c
#pragma pack(push, 1)
typedef struct _ZOO64_WIM_BOOT_ATTR {
  UINT32  BootType;           /// Boot type: 0=BIOS, 1=UEFI, 2=Both
  UINT32  BootFlags;          /// Boot flags
  UINT64  BootloaderOffset;   /// Offset to bootloader
  UINT64  BootloaderSize;     /// Size of bootloader
  UINT64  WIMBootOffset;      /// Offset to WIMBoot metadata
  UINT16  BootFileCount;      /// Number of boot files
  UINT16  BcdStoreLength;     /// Length of BCD store
  UINT32  Reserved[4];        /// Reserved
  /// Followed by:
  ///   [BcdStoreLength bytes: BCD store (Boot Configuration Data)]
  ///   [BootFileCount * BOOT_FILE_ENTRY]
} ZOO64_WIM_BOOT_ATTR;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _ZOO64_BOOT_FILE_ENTRY {
  UINT16  PathLength;         /// Length of boot file path
  UINT64  FileOffset;         /// Offset to file in archive
  UINT64  FileSize;           /// Size of boot file
  UINT32  LoadAddress;        /// Load address for bootloader
  UINT32  Flags;              /// Boot file flags
  /// Followed by:
  ///   [PathLength bytes: file path]
} ZOO64_BOOT_FILE_ENTRY;
#pragma pack(pop)
```

**Boot Types**:
```
0: BIOS_LEGACY    /// Legacy BIOS boot
1: UEFI           /// UEFI boot
2: DUAL_BOOT      /// Both BIOS and UEFI
3: IPXE           /// Network boot (iPXE)
4: GRUB           /// GRUB bootloader
```

**Boot Flags**:
```
0x00000001: SECURE_BOOT_ENABLED    /// UEFI Secure Boot enabled
0x00000002: TEST_SIGNING           /// Allow test-signed drivers
0x00000004: DISABLE_INTEGRITY      /// Disable integrity checks
0x00000008: RECOVERY_MODE          /// Boot to recovery
0x00000010: SAFE_MODE              /// Boot to safe mode
0x00000020: WIMBOOT_ENABLED        /// Use WIMBoot (pointer files)
```

**WIMBoot**:
WIMBoot allows Windows to boot directly from WIM files:
- System files remain in WIM
- Pointer files (reparse points) link to WIM resources
- Saves disk space on tablets/embedded devices
- Requires NTFS with reparse point support

**BCD (Boot Configuration Data)**:
- Binary hive format (like Windows Registry)
- Contains boot menu, boot options, boot sequence
- Required for Windows Boot Manager

### 6.78 Removed - Merged into Section 6b (Unified Integrity)

## 6a. Encryption Support [OPTIONAL]

Zoo64 supports both archive-level and per-file encryption using modern authenticated encryption algorithms.

### 6a.1 Encryption Header

Appears before encrypted file data (per-file encryption) or after compression descriptor (archive-level encryption).

```c
typedef struct _ZOO64_ENCRYPTION_HEADER {
  UINT64  Magic;              /// 0x454E4352595054 ("ENCRYPT ")
  UINT32  HeaderSize;         /// Size of this header including all fields
  UINT16  EncryptionMethod;   /// Encryption algorithm
  UINT16  KeyDerivation;      /// Key derivation function
  UINT32  Iterations;         /// KDF iteration count
  UINT16  SaltLength;         /// Length of salt
  UINT16  IVLength;           /// Length of IV/nonce
  UINT32  TagLength;          /// Length of authentication tag
  UINT32  EncryptedSize;      /// Size of encrypted data
  UINT32  Flags;              /// Encryption flags
  /// Followed by:
  ///   [SaltLength bytes: random salt for KDF]
  ///   [IVLength bytes: initialization vector/nonce]
  ///   [Encrypted data]
  ///   [TagLength bytes: authentication tag]
} ZOO64_ENCRYPTION_HEADER;
#pragma pack(pop)
```

### 6a.2 Encryption Methods

```
0x0000: None (not encrypted)
0x0001: AES-256-GCM (recommended)
0x0002: AES-256-CBC + HMAC-SHA256
0x0003: ChaCha20-Poly1305
0x0004: AES-128-GCM
0x0005: Twofish-256-GCM
0x0006: Serpent-256-GCM
```

### 6a.3 Key Derivation Functions

```
0x0000: None (use key directly - not recommended)
0x0001: PBKDF2-HMAC-SHA256 (compatible, moderate security)
0x0002: PBKDF2-HMAC-SHA512
0x0003: Argon2id (recommended - memory-hard)
0x0004: scrypt
0x0005: bcrypt
```

### 6a.4 Encryption Flags

```
Bit 0:     Compress before encrypt (default)
Bit 1:     Encrypt then compress (not recommended)
Bit 2:     Store password hint (followed by hint string)
Bit 3:     Use key file (in addition to password)
Bit 4:     Header encryption (encrypt file names/metadata)
Bit 5-31:  Reserved
```

### 6a.5 Key Derivation Parameters

For **PBKDF2**:
- Salt: 16-32 bytes random
- Iterations: 600,000+ (OWASP 2023 recommendation)
- Output: 256 bits for AES-256, 128 bits for AES-128

For **Argon2id** (recommended):
- Salt: 16 bytes random
- Memory: 64 MB (65536 KB)
- Iterations: 3-4
- Parallelism: 4 threads
- Output: 256 bits

For **scrypt**:
- Salt: 32 bytes random
- N: 2^17 (131072) - CPU/memory cost
- r: 8 - block size
- p: 1 - parallelization
- Output: 256 bits

### 6a.6 Archive-Level vs File-Level Encryption

**Archive-Level Encryption:**
- Entire archive (except headers) encrypted as single stream
- Central directory encrypted
- File names encrypted (if header encryption enabled)
- Most secure against metadata leakage
- Cannot extract individual files without full decryption

**File-Level Encryption:**
- Each file encrypted independently
- Central directory not encrypted (file names visible)
- Allows selective file extraction
- Better for large archives where partial access is needed

**Hybrid Mode:**
- Files encrypted individually
- Central directory encrypted separately
- Balances security and selective access

### 6a.7 Password Verification

To verify password without full decryption:

```c
typedef struct _ZOO64_PASSWORD_VERIFY {
  UINT32  VerifyMethod;       /// Verification method
  UINT16  VerifyDataLength;   /// Length of verification data
  /// Followed by verification data
} ZOO64_PASSWORD_VERIFY;
#pragma pack(pop)
```

Verification methods:
```
0x0001: Encrypted known plaintext (16 bytes zeros encrypted)
0x0002: HMAC of password + salt
0x0003: Argon2 hash of password
```

## 6b. Data Integrity and Error Correction [OPTIONAL] and Error Correction

Unified integrity system combining hash verification (WIM-style) and error correction (FEC).

### 6b.1 Integrity Header

```c
#pragma pack(push, 1)
typedef struct _ZOO64_INTEGRITY_HEADER {
  UINT64  Magic;              /// 0x494E544547524954 ("INTEGRIT")
  UINT32  HeaderSize;         /// Size of header
  UINT16  Mode;               /// 0=Hash, 1=FEC, 2=Both
  UINT16  HashAlgorithm;      /// 0=SHA1, 1=SHA256, 2=BLAKE3
  UINT16  FECAlgorithm;       /// 0=None, 1=RS, 2=LDPC, 3=PAR2
  UINT16  RecoveryPercent;    /// 0-100% (0=hash only)
  UINT32  ChunkSize;          /// Chunk size
  UINT32  ChunkCount;         /// Data chunks
  UINT32  ParityCount;        /// Parity chunks (0 if hash-only)
  UINT64  DataOffset;         /// Offset to protected data
  UINT64  ParityOffset;       /// Offset to parity (0 if hash-only)
  UINT32  Flags;              /// Flags
  UINT32  Reserved;           /// Reserved
} ZOO64_INTEGRITY_HEADER;
#pragma pack(pop)
```

### 6b.2 Integrity Modes

```
0: HASH_ONLY       /// Verification only (like WIM integrity)
1: FEC_ONLY        /// Error correction without separate hash table
2: HASH_AND_FEC    /// Both verification and correction
```

### 6b.3 Hash Algorithms

```
0: SHA1    /// Fast, 20 bytes (WIM compatible)
1: SHA256  /// Secure, 32 bytes  
2: BLAKE3  /// Fastest, 32 bytes
```

### 6b.4 FEC Algorithms

```
0: NONE            /// No error correction
1: REED_SOLOMON    /// Standard, good balance
2: LDPC            /// Modern, efficient
3: PAR2            /// External .par2 files
```

### 6b.5 Usage

**Hash-only** (like WIM):
- Mode=0, RecoveryPercent=0
- Detects corruption, cannot repair
- Minimal overhead (hash table only)

**FEC-only**:
- Mode=1, RecoveryPercent=5-100
- Repairs corruption automatically
- No separate verification step

**Both**:
- Mode=2
- Hash for quick verification
- FEC for repair if corruption detected

## 7. Seekable Compression [OPTIONAL]

### 7.1 Block Table Format

For seekable compression, a block table precedes the compressed data.

```c
typedef struct _ZOO64_BLOCK_TABLE {
  UINT32  Magic;              /// 0x424C4B54424C ("BLKTBL")
  UINT32  BlockCount;         /// Number of blocks
  UINT32  BlockSize;          /// Uncompressed block size (power of 2)
  UINT32  Flags;              /// Block table flags
  /// Followed by BlockCount entries of block offsets in LEB128 format
} ZOO64_BLOCK_TABLE;
#pragma pack(pop)
```

### 7.2 LEB128 Offset Encoding

Block offsets are stored in LEB128 (Little Endian Base 128) format for space efficiency.

Each offset is the **delta** from the previous offset, stored as LEB128.

Example:
```
Block 0: offset 1000  → LEB128(1000)
Block 1: offset 1500  → LEB128(500)
Block 2: offset 2100  → LEB128(600)
```

### 7.3 Block Format

Each compressed block:
```
[Block Header]
[Compressed Data]
```

```c
typedef struct _ZOO64_BLOCK_HEADER {
  UINT32  UncompressedSize;   /// Size before compression
  UINT32  CompressedSize;     /// Size after compression
  UINT32  CRC32;              /// CRC32 of uncompressed block
} ZOO64_BLOCK_HEADER;
#pragma pack(pop)
```

### 7.4 Variable Block Sizes [OPTIONAL]

When file flag bit 28 is set (Variable Block Sizes), blocks can have different sizes within the same file. This allows optimal compression by adapting block size to content characteristics.

```c
//
// Variable block size table
// Replaces standard fixed-size block table
//
typedef struct _ZOO64_VARIABLE_BLOCK_TABLE {
  UINT32  Magic;              // 0x56424C4B544C ("VBLKTBL")
  UINT32  BlockCount;         // Number of blocks
  UINT32  MinBlockSize;       // Minimum block size (power of 2)
  UINT32  MaxBlockSize;       // Maximum block size (power of 2)
  UINT32  Flags;              // Flags

  // Followed by BlockCount entries:
  // For each block:
  //   [LEB128: uncompressed size]
  //   [LEB128: compressed size delta]
  //   [LEB128: offset delta]
} ZOO64_VARIABLE_BLOCK_TABLE;
```

**Variable Block Size Selection Algorithm**:
1. Analyze content characteristics (entropy, redundancy)
2. High entropy (random/compressed) → smaller blocks (4KB-64KB)
3. Low entropy (text, repetitive) → larger blocks (256KB-4MB)
4. Streaming data → adaptive sizing based on compression ratio

**Benefits**:
- Better compression for heterogeneous files
- Faster seeks for high-entropy regions (smaller blocks)
- Higher compression for low-entropy regions (larger dictionary)

**Use Cases**:
- Database files with mixed data and indexes
- Multimedia files with metadata and streams
- Virtual machine images with sparse and dense regions
- Scientific datasets with varying compression characteristics

### 7.5 Hybrid Compression Mode [OPTIONAL]

When file flag bit 27 is set (Hybrid Compression), different compression algorithms are automatically selected per block for optimal size. This is very slow but produces the best compression.

```c
//
// Hybrid compression block header
//
typedef struct _ZOO64_HYBRID_BLOCK_HEADER {
  UINT16  Algorithm;          // Compression algorithm for THIS block
  UINT32  UncompressedSize;   // Size before compression
  UINT32  CompressedSize;     // Size after compression
  UINT32  CRC32;              // CRC32 of uncompressed block
} ZOO64_HYBRID_BLOCK_HEADER;
```

**Algorithm Selection Process**:
1. For each block, try multiple compression algorithms:
   - LZ4 (fast baseline)
   - ZSTD (balanced)
   - LZMA2 (high compression)
   - BWT+MTF+LZ78+Range (text-optimized)
   - Custom/PAQ (maximum compression)

2. Select algorithm producing smallest compressed size

3. Store algorithm ID in block header

4. If no algorithm beats stored, use stored (0x0000)

**Performance Characteristics**:
- Compression speed: **Very slow** (5-20x slower than single algorithm)
- Decompression speed: **Moderate** (depends on selected algorithms)
- Compression ratio: **Maximum possible** for heterogeneous data

**Best Use Cases**:
- Long-term archival where compression time doesn't matter
- Highly mixed content (text + binary + multimedia)
- Maximum space savings required
- Datasets with unknown or varying characteristics

**Example Compression Ratios**:
```
Mixed dataset (10 GB):
  ZSTD alone:       4.2 GB (58% reduction)
  LZMA2 alone:      3.8 GB (62% reduction)
  Hybrid mode:      3.1 GB (69% reduction) ← 18% better than LZMA2
  Time:             12x slower than LZMA2
```

**Hybrid Compression Flags**:
```
Bit 0:  Try LZ4
Bit 1:  Try ZSTD
Bit 2:  Try LZMA2
Bit 3:  Try BWT pipeline
Bit 4:  Try PAQ
Bit 5:  Try all algorithms (exhaustive search)
Bit 6:  Cache compression results (for parallel compression)
Bit 7:  Use machine learning predictor (experimental)
Bit 8-31: Reserved
```

## 8. Solid Compression [OPTIONAL]

### 8.1 Solid Block Format

```c
typedef struct _ZOO64_SOLID_BLOCK {
  UINT64  Magic;              /// 0x534F4C4944424C4B ("SOLIDBLK")
  UINT32  FileCount;          /// Number of files in solid block
  UINT64  UncompressedSize;   /// Total uncompressed size
  UINT64  CompressedSize;     /// Total compressed size
  UINT32  WindowSize;         /// Compression window size
  UINT32  Flags;              /// Solid block flags
  /// Followed by:
  ///   [File offsets table] - LEB128 encoded offsets
  ///   [Compressed data]
} ZOO64_SOLID_BLOCK;
#pragma pack(pop)
```

### 8.2 Solid Seekable Format

Combines solid compression with block table for random access:

```
[Solid Block Header]
[File Offsets Table] (LEB128)
[Block Table] (LEB128 block offsets)
[Compressed Blocks...]
```

## 9. Digital Signatures [OPTIONAL]

### 9.1 Signature Block

```c
typedef struct _ZOO64_SIGNATURE {
  UINT64  Magic;              /// 0x5349474E41545552 ("SIGNATUR")
  UINT32  SignatureSize;      /// Total size of signature block
  UINT16  SignatureType;      /// Signature algorithm
  UINT16  HashAlgorithm;      /// Hash algorithm used
  UINT64  SigningTime;        /// Signing timestamp (NTP extended format)
  UINT32  CertificateSize;    /// Size of certificate (0 if none)
  UINT32  SignatureDataSize;  /// Size of signature data
  /// Followed by:
  ///   [CertificateSize bytes: X.509 certificate]
  ///   [SignatureDataSize bytes: signature data]
} ZOO64_SIGNATURE;
#pragma pack(pop)
```

### 9.2 Signature Types

```
0x0001: RSA-2048
0x0002: RSA-4096
0x0003: ECDSA-P256
0x0004: ECDSA-P384
0x0005: Ed25519
0x0006: Ed448
```

### 9.3 Hash Algorithms

```
0x0001: SHA-256
0x0002: SHA-384
0x0003: SHA-512
0x0004: SHA3-256
0x0005: SHA3-512
0x0006: BLAKE2b
0x0007: BLAKE3
```

### 9.4 File-Level Signature

Appears after file data, hash covers uncompressed file data.

### 9.5 Archive-Level Signature

Appears at end of archive, hash covers entire archive except signature block itself.

## 10. Central Directory [REQUIRED]

The central directory provides fast access to all files in the archive.

```c
typedef struct _ZOO64_CENTRAL_DIR {
  UINT64  Magic;              /// 0x43454E5444495220 ("CENTDIR ")
  UINT32  EntryCount;         /// Number of entries
  UINT64  DirectorySize;      /// Size of central directory
  UINT32  Flags;              /// Directory flags
  /// Followed by EntryCount directory entries
} ZOO64_CENTRAL_DIR;
#pragma pack(pop)
```

### 10.1 Central Directory Entry

```c
typedef struct _ZOO64_CENTRAL_ENTRY {
  UINT64  FileHeaderOffset;   /// Offset to file header
  UINT64  UncompressedSize;   /// File size
  UINT64  CompressedSize;     /// Compressed size (0 if stored)
  UINT32  CRC32;              /// CRC32 of file
  UINT16  PathLength;         /// Length of path
  UINT16  Flags;              /// Entry flags
  /// Followed by UTF-8 path
} ZOO64_CENTRAL_ENTRY;
#pragma pack(pop)
```

## 11. End of Archive Marker

```c
typedef struct _ZOO64_END_OF_ARCHIVE {
  UINT64  Magic;              /// 0x454E444F46415243 ("ENDOFARC")
  UINT64  ArchiveSize;        /// Total archive size
  UINT64  CentralDirOffset;   /// Offset to central directory
  UINT32  FileCount;          /// Total files in archive
  UINT32  CRC32;              /// CRC32 of central directory
  UINT8   SHA256[32];         /// SHA-256 of entire archive (excl. this block)
} ZOO64_END_OF_ARCHIVE;
#pragma pack(pop)
```

## 12. Compression Pipeline

### 12.1 Default Pipeline (Algorithm 0x0001)

For text/structured data:
```
Input → BWT → MTF → RAD50RLE → Windowed LZ78 → Range Encoding → Output
```

### 12.2 Pipeline Stages

1. **BWT (Burrows-Wheeler Transform)**: Groups similar characters
2. **MTF (Move-To-Front)**: Converts repeated chars to small values
3. **RAD50RLE**: RAD-50 encoding with integrated RLE, LEB128, and bit transposition
4. **Windowed LZ78**: Dictionary compression with configurable window
5. **Range Encoding**: Adaptive arithmetic encoding

### 12.2a RAD50RLE Stage Details

The RAD50RLE stage combines multiple encoding techniques:

**Processing Flow**:
```
MTF Output → RAD-50 Encoding → Run-Length Encoding → LEB128 → Bit Transposition → Output
```

**RAD-50 Encoding**:
- Encodes alphanumeric sequences (A-Z, 0-9, space, $, .) as base-40 triplets
- 3 characters → 16-bit value (40³ = 64000 < 65536)
- Highly efficient for text with uppercase letters and digits
- Falls back to literal encoding for non-RAD50 characters

**Run-Length Encoding (RLE)**:
- Applied AFTER RAD-50 encoding to encoded values
- Encodes sequences of repeated RAD-50 values or literals
- Format: (value, count) pairs where count is LEB128-encoded
- Particularly effective after MTF which produces many zeros

**RLE Examples**:
```
Input (after RAD50):  [15, 15, 15, 15, 42, 42, 3, 3, 3]
After RLE:            [(15, 4), (42, 2), (3, 3)]
Encoded:              [15, LEB128(4), 42, LEB128(2), 3, LEB128(3)]
```

**LEB128 Encoding**:
- Variable-length integer encoding for run counts
- Small counts (< 128) use 1 byte
- Larger counts use more bytes as needed
- Saves space compared to fixed-width counts

**Bit Transposition**:
- Transposes bits across byte boundaries
- Groups high bits together, low bits together
- Improves subsequent compression (LZ78, Range)
- Reversible transformation

**Complete RAD50RLE Example**:
```
Input:     "AAA" (after MTF)
RAD50:     Encode "AAA" → 0x0021 (RAD-50 triplet)
RLE:       Single occurrence → (0x0021, 1)
LEB128:    Count 1 → 0x01
Output:    [0x00, 0x21, 0x01]
Bit Trans: [rearranged bits for better compression]
```

**When RLE is Applied**:
- RLE is ALWAYS applied when RAD50RLE stage is active
- Even non-RAD50 characters benefit from RLE
- RLE operates on both RAD-50 encoded values AND literal bytes
- Threshold: minimum run length of 3 for compression benefit

**RLE Encoding Format**:
```c
// RLE token (variable length)
struct RLE_TOKEN {
  UINT8 type;           /// 0=literal, 1=RAD50, 2=run
  union {
    UINT8 literal;      /// If type=0: raw byte
    UINT16 rad50;       /// If type=1: RAD-50 value
    struct {            /// If type=2: run
      UINT16 value;     ///   Value to repeat
      UINT8 count_leb;  ///   Count in LEB128 format
    } run;
  };
};
```

### 12.3 Window Sizes

Supported window sizes (power of 2):
- 4K, 8K, 16K, 32K, 64K (default), 128K, 256K, 512K, 1M

### 12.4 Block Sizes for Seekable

Supported block sizes (power of 2):
- 4K, 8K, 16K, 32K, 64K, 128K (default), 256K, 512K, 1M, 2M, 4M

### 12.5 ZOZ Compression Algorithm (0x0001)

ZOZ is an adaptive entropy-reduction compression pipeline designed for maximum compression ratio. It combines multiple techniques in a carefully orchestrated sequence, with each stage applying only if it reduces entropy.

**Algorithm Name**: ZOZ (Zo-oz, adaptive entropy reduction)
**Algorithm ID**: 0x0001
**Conformance Level**: Full
**Best Use Case**: Maximum compression for text, source code, structured data

#### 12.5.1 ZOZ Pipeline Overview

The ZOZ algorithm consists of the following adaptive stages:

0. **Block Sorting** (256 blocks, optional)
   - Divide data into 256 equal-sized blocks at seek boundaries
   - Sort blocks lexicographically
   - Emit compact remapping table (256 entries)
   - Significantly improves compression for repetitive data

1. **Adaptive Transform Stage** (BWT/MTF selection)
   - Test: BWT, MTF, BWT+MTF, MTF+BWT
   - Apply only the variant that reduces entropy the most
   - If none reduces entropy, skip to next stage

2. **RLE Pattern Matching** (bit-aligned ULEB128)
   - Detects any block-length repetitions
   - Adaptive maximum length for optimal compression
   - Count encoded as ULEB128 at bit-aligned positions

3. **Symbol Encoding** (RAD50/Bit-Squishing/Raw)
   - If beneficial, encode using RAD50 or bit squishing
   - Otherwise emit raw bytes

4. **Bit Transposition** (optional entropy reduction)
   - If bit transposition reduces entropy, apply it

5. **Multi-Window LZ78** (1/2/3 windows, variable sizes)
   - Block sizes: 8K to 16M
   - Window sizes: 4K to 12M (may be variable)
   - 1 window: Classic LZ78
   - 2 windows: Prediction from previous uncompressed block
   - 3 windows: Bidirectional prediction (like MPEG B-frames), requires special block ordering

#### 12.5.1a Stage 0: Block Sorting Transform (Optional)

Block sorting is an optional preprocessing stage that divides data into exactly **256 equal-sized blocks** and sorts them lexicographically. This transform is applied at **seek boundaries** (e.g., every 8K-16M depending on seekable block size) and can dramatically improve compression for files with repetitive patterns.

**Key Features**:
- **Fixed 256 blocks**: Always divides data into exactly 256 blocks
- **Variable block size**: Each block size = SeekableBlockSize / 256
- **Examples**:
  - 16 MB seekable block → 256 blocks × 64 KB each
  - 1 MB seekable block → 256 blocks × 4 KB each
  - 256 KB seekable block → 256 blocks × 1 KB each

**When to Apply**:
- Repetitive data structures (database records, log files)
- Files with many identical or similar chunks
- Virtual machine disk images with repeated sectors
- Backup files with duplicate regions

**When to Skip**:
- Random or encrypted data
- Pre-compressed data (JPEG, PNG, MP4)
- When entropy testing shows no benefit

**Block Sorting Algorithm**:

```c
//
// Divide data into exactly 256 blocks and sort them
//
typedef struct _SORTED_BLOCK {
  UINT8   *Data;              // Block data (variable size)
  UINTN   Size;               // Block size
  UINT8   OriginalIndex;      // Original position (0-255)
} SORTED_BLOCK;

BOOLEAN
BlockSort256 (
  IN     CONST UINT8    *Input,
  IN     UINTN          InputSize,
  OUT    UINT8          **SortedData,
  OUT    UINT8          *RemapTable,    // Always 256 entries
  OUT    UINTN          *BlockSize
  )
{
  SORTED_BLOCK   Blocks[256];
  UINTN          BaseBlockSize;
  UINTN          Remainder;
  UINTN          Index;

  //
  // Step 1: Calculate block size (256 blocks total)
  //
  BaseBlockSize = InputSize / 256;
  Remainder = InputSize % 256;
  *BlockSize = BaseBlockSize;

  //
  // Step 2: Create array of 256 blocks with original indices
  //
  for (Index = 0; Index < 256; Index++) {
    UINTN Offset = Index * BaseBlockSize;
    UINTN ThisBlockSize = BaseBlockSize;

    //
    // Distribute remainder bytes to first blocks
    //
    if (Index < Remainder) {
      Offset += Index;
      ThisBlockSize++;
    } else {
      Offset += Remainder;
    }

    Blocks[Index].Data = (UINT8*)&Input[Offset];
    Blocks[Index].Size = ThisBlockSize;
    Blocks[Index].OriginalIndex = (UINT8)Index;
  }

  //
  // Step 3: Sort blocks lexicographically
  //
  QuickSortBlocks (Blocks, 0, 255);

  //
  // Step 4: Extract sorted data and create remap table
  //
  *SortedData = AllocatePool (InputSize);
  UINTN WriteOffset = 0;

  for (Index = 0; Index < 256; Index++) {
    CopyMem (&(*SortedData)[WriteOffset], Blocks[Index].Data, Blocks[Index].Size);
    RemapTable[Index] = Blocks[Index].OriginalIndex;
    WriteOffset += Blocks[Index].Size;
  }

  return TRUE;
}

//
// Lexicographic comparison for variable-size blocks
//
INTN
CompareBlocks (
  IN     CONST SORTED_BLOCK   *Block1,
  IN     CONST SORTED_BLOCK   *Block2
  )
{
  UINTN   CompareSize = MIN(Block1->Size, Block2->Size);
  INTN    Result;

  //
  // Compare common bytes lexicographically
  //
  Result = CompareMem (Block1->Data, Block2->Data, CompareSize);

  if (Result != 0) {
    return Result;
  }

  //
  // If common bytes equal, shorter block comes first
  //
  if (Block1->Size < Block2->Size) {
    return -1;
  }
  if (Block1->Size > Block2->Size) {
    return 1;
  }

  //
  // If completely equal, maintain stable sort by original index
  //
  return (INTN)Block1->OriginalIndex - (INTN)Block2->OriginalIndex;
}
```

**Compact Remapping Table Encoding**:

The remapping table stores the original position (0-255) of each sorted block. Since we have exactly **256 entries** with values 0-255, the table is compact:

```c
//
// Encode remap table compactly (256 entries, values 0-255)
//
typedef enum _REMAP_ENCODING_MODE {
  RemapEncodingDirect      = 0,  // Direct 256 bytes (1 byte per entry)
  RemapEncodingDelta       = 1,  // Delta encoding with signed bytes
  RemapEncodingRLE         = 2,  // RLE + delta encoding
  RemapEncodingBitmap      = 3   // Bitmap for mostly-sorted data
} REMAP_ENCODING_MODE;

UINTN
EncodeRemapTableCompact (
  IN     CONST UINT8    *RemapTable,    // 256 entries (0-255)
  OUT    UINT8          **CompactTable,
  OUT    UINT8          *EncodingMode
  )
{
  UINTN   DirectSize, DeltaSize, RleSize, BitmapSize;
  UINTN   BestSize;

  //
  // Test all encoding modes and select the smallest
  //
  DirectSize = 256;  // Always 256 bytes for direct
  DeltaSize = EncodeRemapDelta (RemapTable, NULL);
  RleSize = EncodeRemapRLE (RemapTable, NULL);
  BitmapSize = EncodeRemapBitmap (RemapTable, NULL);

  //
  // Select best encoding
  //
  BestSize = DirectSize;
  *EncodingMode = RemapEncodingDirect;

  if (DeltaSize < BestSize) {
    BestSize = DeltaSize;
    *EncodingMode = RemapEncodingDelta;
  }
  if (RleSize < BestSize) {
    BestSize = RleSize;
    *EncodingMode = RemapEncodingRLE;
  }
  if (BitmapSize < BestSize) {
    BestSize = BitmapSize;
    *EncodingMode = RemapEncodingBitmap;
  }

  //
  // Encode using best mode
  //
  *CompactTable = AllocatePool (BestSize);

  switch (*EncodingMode) {
    case RemapEncodingDirect:
      CopyMem (*CompactTable, RemapTable, 256);
      break;
    case RemapEncodingDelta:
      EncodeRemapDelta (RemapTable, *CompactTable);
      break;
    case RemapEncodingRLE:
      EncodeRemapRLE (RemapTable, *CompactTable);
      break;
    case RemapEncodingBitmap:
      EncodeRemapBitmap (RemapTable, *CompactTable);
      break;
  }

  return BestSize;
}
```

**Encoding Mode 1: Delta Encoding (Best for Most Cases)**:

```c
//
// Delta encoding: Store first index, then signed deltas
//
UINTN
EncodeRemapDelta (
  IN     CONST UINT8    *RemapTable,    // 256 entries
  OUT    UINT8          *Output
  )
{
  UINTN    OutputSize = 0;
  UINT8    PrevIndex;
  INT16    Delta;

  if (Output == NULL) {
    //
    // Size calculation: first index + 255 deltas
    // Each delta fits in 1-2 bytes (signed, -255 to +255)
    //
    UINTN EstimatedSize = 1;  // First index

    PrevIndex = RemapTable[0];
    for (UINTN i = 1; i < 256; i++) {
      Delta = (INT16)RemapTable[i] - (INT16)PrevIndex;
      // Small deltas (-127 to +127): 1 byte
      // Large deltas: 2 bytes with marker
      EstimatedSize += (Delta >= -127 && Delta <= 127) ? 1 : 2;
      PrevIndex = RemapTable[i];
    }

    return EstimatedSize;
  }

  //
  // Emit first index (1 byte)
  //
  Output[OutputSize++] = RemapTable[0];

  //
  // Emit deltas
  //
  PrevIndex = RemapTable[0];
  for (UINTN i = 1; i < 256; i++) {
    Delta = (INT16)RemapTable[i] - (INT16)PrevIndex;

    if (Delta >= -127 && Delta <= 127) {
      //
      // Small delta: 1 byte signed
      //
      Output[OutputSize++] = (UINT8)(INT8)Delta;
    } else {
      //
      // Large delta: marker (0x80) + 1 byte unsigned index
      //
      Output[OutputSize++] = 0x80;  // Escape marker
      Output[OutputSize++] = RemapTable[i];
    }

    PrevIndex = RemapTable[i];
  }

  return OutputSize;
}
```

**Encoding Mode 2: RLE + Delta (Best for Partially Sorted Data)**:

```c
//
// RLE + Delta: Compress runs of sequential indices
//
UINTN
EncodeRemapRLE (
  IN     CONST UINT8    *RemapTable,    // 256 entries
  OUT    UINT8          *Output
  )
{
  UINTN    OutputSize = 0;
  UINTN    Index = 0;

  while (Index < 256) {
    UINT8    StartValue = RemapTable[Index];
    UINTN    RunLength = 1;

    //
    // Detect runs of sequential indices (N, N+1, N+2, ...)
    //
    while (Index + RunLength < 256 &&
           RemapTable[Index + RunLength] == (UINT8)(StartValue + RunLength)) {
      RunLength++;
    }

    if (RunLength >= 4) {
      //
      // Emit RLE: [0xFF] [start_index] [run_length]
      //
      if (Output != NULL) {
        Output[OutputSize + 0] = 0xFF;          // RLE marker
        Output[OutputSize + 1] = StartValue;
        Output[OutputSize + 2] = (UINT8)RunLength;
      }
      OutputSize += 3;
      Index += RunLength;
    } else {
      //
      // Emit individual indices (short run or single)
      //
      for (UINTN i = 0; i < RunLength; i++) {
        if (Output != NULL) {
          Output[OutputSize] = RemapTable[Index];
        }
        OutputSize++;
        Index++;
      }
    }
  }

  return OutputSize;
}
```

**Encoding Mode 3: Bitmap (Best for Nearly-Sorted Data)**:

```c
//
// Bitmap encoding: For data that's mostly sorted
//
UINTN
EncodeRemapBitmap (
  IN     CONST UINT8    *RemapTable,    // 256 entries
  OUT    UINT8          *Output
  )
{
  UINTN    MovedCount = 0;
  UINT8    Bitmap[32];  // 256 bits = 32 bytes
  UINTN    OutputSize = 0;

  //
  // Count how many blocks are out of order
  //
  for (UINTN i = 0; i < 256; i++) {
    if (RemapTable[i] != i) {
      MovedCount++;
    }
  }

  //
  // Only use bitmap if < 25% of blocks moved
  //
  if (MovedCount > 64) {
    return UINTN_MAX;  // Not efficient (> 25% moved)
  }

  //
  // Size: 32-byte bitmap + moved indices
  //
  if (Output == NULL) {
    return 32 + MovedCount;
  }

  //
  // Build bitmap: 1 = moved, 0 = in place
  //
  SetMem (Bitmap, 32, 0);

  for (UINTN i = 0; i < 256; i++) {
    if (RemapTable[i] != i) {
      Bitmap[i / 8] |= (1 << (i % 8));
    }
  }

  //
  // Emit bitmap
  //
  CopyMem (Output, Bitmap, 32);
  OutputSize = 32;

  //
  // Emit only the moved indices
  //
  for (UINTN i = 0; i < 256; i++) {
    if (RemapTable[i] != i) {
      Output[OutputSize++] = RemapTable[i];
    }
  }

  return OutputSize;
}
```

**Remapping Table Format**:

The header is embedded in the ZOZ block header (no separate structure needed):

```c
// Block sorting info is part of ZOZ_BLOCK_HEADER:
// - BlockSorted: 1 byte (0=no, 1=yes)
// - RemapEncoding: 1 byte (encoding mode 0-3)
// - RemapTableSize: 4 bytes (compact table size)
```

**Complete Transform Structure**:

```
[At Seek Boundary - e.g., every 16 MB]

[ZOZ_BLOCK_HEADER] (with BlockSorted=1)
[Compact Remap Table]         // 64-256 bytes (256 entries, optimized)
[Sorted Block Data]           // Same size as input
[Further ZOZ compression stages...]
```

**Remap Table Size (Always 256 Entries)**:

For **ANY** seekable block size:
- **Block count**: Always 256 blocks
- **Direct encoding**: 256 bytes (1 byte per entry)
- **Delta encoding**: 128-200 bytes (typical, small deltas)
- **RLE encoding**: 64-180 bytes (if partially sorted)
- **Bitmap encoding**: 32-96 bytes (if > 75% already sorted)

**Overhead Analysis**:

| Input Size | Block Size | Direct | Delta (avg) | RLE (best) | Bitmap (best) | Overhead % |
|------------|------------|--------|-------------|------------|---------------|------------|
| 256 KB | 1 KB | 256 B | 160 B | 100 B | 64 B | 0.025-0.1% |
| 1 MB | 4 KB | 256 B | 160 B | 100 B | 64 B | 0.006-0.025% |
| 16 MB | 64 KB | 256 B | 160 B | 100 B | 64 B | 0.0004-0.0015% |

**Key Advantage**: Overhead is **constant** (256 bytes max) regardless of input size!

**Compression Benefit Examples**:

```
Example 1: Log files with repetitive 64 KB blocks
  Original: 16 MB (256 blocks × 64 KB)
  Unique blocks: 32 (87.5% duplication)
  After block sort: Identical blocks grouped together
  ZOZ compression:
    - Without block sort: 8 MB (50% ratio)
    - With block sort: 1.5 MB (90.6% ratio)
  Improvement: 5.3x better compression
  Remap table: 148 bytes (delta encoding)

Example 2: Database dump with repeated 4 KB pages
  Original: 1 MB (256 blocks × 4 KB)
  After block sort: Many identical pages grouped
  RLE compression: 75% of blocks are identical
  Final size: 80 KB (92% compression)
  Remap table: 96 bytes (RLE encoding)

Example 3: VM disk image (16 MB, mostly zeros)
  Original: 16 MB (256 blocks × 64 KB)
  After block sort: All zero blocks at start
  ZOZ + RLE: 99% compression
  Final size: 160 KB
  Remap table: 72 bytes (bitmap encoding)
```

**Decompression Algorithm**:

```c
//
// Restore original block order from sorted data (256 blocks)
//
BOOLEAN
BlockUnsort256 (
  IN     CONST UINT8             *SortedData,
  IN     UINTN                   DataSize,
  IN     CONST UINT8             *CompactRemap,
  IN     UINTN                   RemapTableSize,
  IN     UINT8                   EncodingMode,
  OUT    UINT8                   **OriginalData
  )
{
  UINT8    RemapTable[256];
  UINTN    BaseBlockSize;
  UINTN    Remainder;
  UINTN    Index;

  //
  // Step 1: Decode compact remap table (always 256 entries)
  //
  DecodeRemapTable (
    CompactRemap,
    RemapTableSize,
    EncodingMode,
    RemapTable  // Output: 256 bytes
  );

  //
  // Step 2: Calculate block size
  //
  BaseBlockSize = DataSize / 256;
  Remainder = DataSize % 256;

  //
  // Step 3: Allocate output buffer
  //
  *OriginalData = AllocatePool (DataSize);

  //
  // Step 4: Restore blocks to original positions
  //
  UINTN ReadOffset = 0;

  for (Index = 0; Index < 256; Index++) {
    UINT8    OriginalPos = RemapTable[Index];  // Where this block originally was
    UINTN    ThisBlockSize = BaseBlockSize;

    //
    // First 'Remainder' blocks are 1 byte larger
    //
    if (OriginalPos < Remainder) {
      ThisBlockSize++;
    }

    //
    // Calculate original offset
    //
    UINTN OriginalOffset = OriginalPos * BaseBlockSize;
    if (OriginalPos < Remainder) {
      OriginalOffset += OriginalPos;
    } else {
      OriginalOffset += Remainder;
    }

    //
    // Copy block to its original position
    //
    CopyMem (
      &(*OriginalData)[OriginalOffset],
      &SortedData[ReadOffset],
      ThisBlockSize
    );

    ReadOffset += ThisBlockSize;
  }

  return TRUE;
}
```

**Integration with ZOZ Pipeline**:

```
Seekable Block (16 MB)
    ↓
[Stage 0: Block Sort 256]
    ↓
  Divide into 256 blocks of 64 KB each
    ↓
  Sort lexicographically
    ↓
  Encode remap table (delta mode → 160 bytes)
    ↓
  Emit remap table + sorted blocks
    ↓
[Stage 1: BWT/MTF] (operates on sorted data)
    ↓
[Stage 2-5: RLE, Symbol, Transpose, LZ78]
    ↓
Final compressed block

Overhead: 160 bytes / 16 MB = 0.00095%
```

**When Block Sort Helps Most**:
1. **Structured data**: Repetitive chunks (logs, databases)
2. **Virtual machines**: Disk images with repeated sectors
3. **Backup files**: Incremental backups with duplicate regions
4. **Scientific data**: Repeated measurement patterns
5. **Binary files**: Repeated data structures

**When to Skip Block Sort**:
1. Already compressed data
2. Random/encrypted data
3. High entropy data (measured before transformation)
4. Data with no repetition patterns

#### 12.5.2 Stage 1: Adaptive Transform Selection

ZOZ tests four transform variants and selects the one with lowest entropy:

**Transform Variants**:
1. **BWT Only**: Burrows-Wheeler Transform
2. **MTF Only**: Move-To-Front Transform
3. **BWT → MTF**: BWT followed by MTF
4. **MTF → BWT**: MTF followed by BWT

**Selection Process**:
```c
//
// Entropy calculation for transform selection
//
FLOAT64
CalculateEntropy (
  IN     CONST UINT8   *Data,
  IN     UINTN         Size
  )
{
  UINT64   Histogram[256];
  FLOAT64  Entropy;
  UINTN    Index;

  SetMem (Histogram, sizeof(Histogram), 0);

  //
  // Build histogram
  //
  for (Index = 0; Index < Size; Index++) {
    Histogram[Data[Index]]++;
  }

  //
  // Calculate Shannon entropy
  //
  Entropy = 0.0;
  for (Index = 0; Index < 256; Index++) {
    if (Histogram[Index] > 0) {
      FLOAT64 Probability = (FLOAT64)Histogram[Index] / (FLOAT64)Size;
      Entropy -= Probability * log2(Probability);
    }
  }

  return Entropy;
}

//
// Select best transform
//
UINT8
SelectBestTransform (
  IN     CONST UINT8   *Input,
  IN     UINTN         InputSize,
  OUT    UINT8         *Output,
  OUT    UINTN         *TransformData
  )
{
  UINT8    *BwtBuffer;
  UINT8    *MtfBuffer;
  UINT8    *BwtMtfBuffer;
  UINT8    *MtfBwtBuffer;
  FLOAT64  OriginalEntropy;
  FLOAT64  BwtEntropy;
  FLOAT64  MtfEntropy;
  FLOAT64  BwtMtfEntropy;
  FLOAT64  MtfBwtEntropy;
  UINT8    BestTransform;
  FLOAT64  BestEntropy;

  //
  // Calculate original entropy
  //
  OriginalEntropy = CalculateEntropy (Input, InputSize);
  BestEntropy = OriginalEntropy;
  BestTransform = 0;  // No transform

  //
  // Allocate buffers
  //
  BwtBuffer = AllocatePool (InputSize);
  MtfBuffer = AllocatePool (InputSize);
  BwtMtfBuffer = AllocatePool (InputSize);
  MtfBwtBuffer = AllocatePool (InputSize);

  if (BwtBuffer == NULL || MtfBuffer == NULL ||
      BwtMtfBuffer == NULL || MtfBwtBuffer == NULL) {
    goto Cleanup;
  }

  //
  // Test BWT
  //
  if (BwtTransform (Input, InputSize, BwtBuffer, &TransformData[0])) {
    BwtEntropy = CalculateEntropy (BwtBuffer, InputSize);
    if (BwtEntropy < BestEntropy) {
      BestEntropy = BwtEntropy;
      BestTransform = 1;  // BWT only
    }
  }

  //
  // Test MTF
  //
  if (MtfTransform (Input, InputSize, MtfBuffer)) {
    MtfEntropy = CalculateEntropy (MtfBuffer, InputSize);
    if (MtfEntropy < BestEntropy) {
      BestEntropy = MtfEntropy;
      BestTransform = 2;  // MTF only
    }
  }

  //
  // Test BWT → MTF
  //
  if (BwtTransform (Input, InputSize, BwtBuffer, &TransformData[0])) {
    if (MtfTransform (BwtBuffer, InputSize, BwtMtfBuffer)) {
      BwtMtfEntropy = CalculateEntropy (BwtMtfBuffer, InputSize);
      if (BwtMtfEntropy < BestEntropy) {
        BestEntropy = BwtMtfEntropy;
        BestTransform = 3;  // BWT → MTF
      }
    }
  }

  //
  // Test MTF → BWT
  //
  if (MtfTransform (Input, InputSize, MtfBuffer)) {
    if (BwtTransform (MtfBuffer, InputSize, MtfBwtBuffer, &TransformData[0])) {
      MtfBwtEntropy = CalculateEntropy (MtfBwtBuffer, InputSize);
      if (MtfBwtEntropy < BestEntropy) {
        BestEntropy = MtfBwtEntropy;
        BestTransform = 4;  // MTF → BWT
      }
    }
  }

  //
  // Copy best result to output
  //
  switch (BestTransform) {
    case 1:
      CopyMem (Output, BwtBuffer, InputSize);
      break;
    case 2:
      CopyMem (Output, MtfBuffer, InputSize);
      break;
    case 3:
      CopyMem (Output, BwtMtfBuffer, InputSize);
      break;
    case 4:
      CopyMem (Output, MtfBwtBuffer, InputSize);
      break;
    default:
      CopyMem (Output, Input, InputSize);
      break;
  }

Cleanup:
  if (BwtBuffer != NULL) FreePool (BwtBuffer);
  if (MtfBuffer != NULL) FreePool (MtfBuffer);
  if (BwtMtfBuffer != NULL) FreePool (BwtMtfBuffer);
  if (MtfBwtBuffer != NULL) FreePool (MtfBwtBuffer);

  return BestTransform;
}
```

**Transform Encoding**:
- Bits 0-2: Transform type (0=none, 1=BWT, 2=MTF, 3=BWT→MTF, 4=MTF→BWT)
- Additional data: BWT primary index if applicable

#### 12.5.3 Stage 2: RLE Pattern Matching with Bit-Aligned ULEB128

RLE (Run-Length Encoding) in ZOZ is sophisticated and bit-aligned for maximum compression.

**Features**:
- Detects repetitions of any block length (not just single bytes)
- Adaptive maximum run length (chooses optimal cutoff)
- ULEB128 encoding for counts (variable-length)
- **Critical**: Emitted at bit-aligned positions, not byte-aligned

**RLE Encoding Format**:
```
[Literal flag: 1 bit]  0 = run, 1 = literal
If run (0):
  [Run length: ULEB128, bit-aligned]
  [Run data: N bytes]
If literal (1):
  [Literal length: ULEB128, bit-aligned]
  [Literal data: N bytes]
```

**ULEB128 Bit-Aligned Encoding**:
```c
//
// ULEB128 encoding (LEB128 unsigned) at bit-aligned positions
//
UINTN
WriteBitAlignedUleb128 (
  IN OUT UINT8    *BitStream,
  IN OUT UINTN    *BitOffset,
  IN     UINT64   Value
  )
{
  UINTN  BitsWritten;

  BitsWritten = 0;

  //
  // Write ULEB128: 7 data bits + 1 continuation bit per byte
  //
  do {
    UINT8 Byte = (UINT8)(Value & 0x7F);
    Value >>= 7;

    //
    // Set continuation bit if more bytes follow
    //
    if (Value != 0) {
      Byte |= 0x80;
    }

    //
    // Write byte at bit-aligned position
    //
    WriteBitsToStream (BitStream, BitOffset, Byte, 8);
    BitsWritten += 8;
  } while (Value != 0);

  return BitsWritten;
}

//
// RLE pattern matching with adaptive max length
//
BOOLEAN
RleEncode (
  IN     CONST UINT8   *Input,
  IN     UINTN         InputSize,
  OUT    UINT8         *Output,
  OUT    UINTN         *OutputBits
  )
{
  UINTN   InputIndex;
  UINTN   BitOffset;
  UINTN   RunLength;
  UINTN   MaxRunLength;

  InputIndex = 0;
  BitOffset = 0;

  //
  // Determine adaptive max run length
  //
  MaxRunLength = DetermineOptimalMaxRunLength (Input, InputSize);

  while (InputIndex < InputSize) {
    //
    // Count run length
    //
    RunLength = 1;
    while (InputIndex + RunLength < InputSize &&
           RunLength < MaxRunLength &&
           Input[InputIndex] == Input[InputIndex + RunLength]) {
      RunLength++;
    }

    if (RunLength >= 3) {
      //
      // Encode as run: [0 bit][ULEB128 length][byte]
      //
      WriteBitsToStream (Output, &BitOffset, 0, 1);  // Run flag
      WriteBitAlignedUleb128 (Output, &BitOffset, RunLength);
      WriteBitsToStream (Output, &BitOffset, Input[InputIndex], 8);
      InputIndex += RunLength;
    } else {
      //
      // Encode as literal: [1 bit][ULEB128 length][literal bytes]
      //
      UINTN LiteralStart = InputIndex;
      UINTN LiteralLength = 0;

      //
      // Find literal run (no repeating patterns)
      //
      while (InputIndex < InputSize &&
             !HasRepeatingPattern (Input, InputIndex, InputSize, 3)) {
        InputIndex++;
        LiteralLength++;
      }

      WriteBitsToStream (Output, &BitOffset, 1, 1);  // Literal flag
      WriteBitAlignedUleb128 (Output, &BitOffset, LiteralLength);

      //
      // Write literal bytes
      //
      for (UINTN i = 0; i < LiteralLength; i++) {
        WriteBitsToStream (Output, &BitOffset, Input[LiteralStart + i], 8);
      }
    }
  }

  *OutputBits = BitOffset;
  return TRUE;
}
```

**Adaptive Max Run Length**: The algorithm analyzes the input data to determine the optimal maximum run length that provides the best compression ratio. Typical values: 3-255.

#### 12.5.4 Stage 3: Symbol Encoding (RAD50/Bit-Squishing/Raw)

After RLE, ZOZ analyzes whether specialized symbol encoding provides benefits:

**Encoding Options**:
1. **RAD50**: If data fits RAD50 character set (A-Z, 0-9, space, $, .)
2. **Bit Squishing**: If alphabet size ≤ 128 unique symbols (see section 12.7)
3. **Raw Bytes**: If neither provides compression benefit

**Selection Process**:
```c
//
// Select symbol encoding method
//
UINT8
SelectSymbolEncoding (
  IN     CONST UINT8   *Input,
  IN     UINTN         InputSize,
  OUT    UINT8         *Output,
  OUT    UINTN         *OutputSize
  )
{
  UINTN   Rad50Size;
  UINTN   BitSquishSize;
  UINT8   Encoding;

  //
  // Try RAD50 encoding
  //
  if (IsRad50Compatible (Input, InputSize)) {
    Rad50Size = Rad50Encode (Input, InputSize, Output);
  } else {
    Rad50Size = SIZE_MAX;
  }

  //
  // Try bit squishing
  //
  if (GetAlphabetSize (Input, InputSize) <= 128) {
    BitSquishSize = BitSquishCompress (Input, InputSize, Output, OutputSize);
  } else {
    BitSquishSize = SIZE_MAX;
  }

  //
  // Select best encoding
  //
  if (Rad50Size < InputSize && Rad50Size <= BitSquishSize) {
    Encoding = 1;  // RAD50
    *OutputSize = Rad50Size;
  } else if (BitSquishSize < InputSize && BitSquishSize < Rad50Size) {
    Encoding = 2;  // Bit squishing
    *OutputSize = BitSquishSize;
  } else {
    //
    // Use raw bytes
    //
    Encoding = 0;
    CopyMem (Output, Input, InputSize);
    *OutputSize = InputSize;
  }

  return Encoding;
}
```

**Encoding Header**:
- Bits 0-1: Encoding type (0=raw, 1=RAD50, 2=bit-squish)

#### 12.5.5 Stage 4: Bit Transposition (Optional Entropy Reduction)

Bit transposition can significantly reduce entropy for certain data patterns (e.g., 16-bit audio, structured binary).

**Transposition**: Reorganize bits across bytes to group similar bit patterns

**Example** (4 bytes):
```
Original:   10110011 11001010 10110111 11001110
            Byte 0   Byte 1   Byte 2   Byte 3

Transposed: 11110000 00111110 11001011 11010111
            Bit 7    Bit 6    Bit 5-4  Bit 3-0
```

**Implementation**:
```c
//
// Bit transposition for entropy reduction
//
BOOLEAN
BitTranspose (
  IN     CONST UINT8   *Input,
  IN     UINTN         InputSize,
  OUT    UINT8         *Output
  )
{
  UINTN  ByteIndex;
  UINTN  BitIndex;

  SetMem (Output, InputSize, 0);

  //
  // Transpose: group same bit positions across all bytes
  //
  for (ByteIndex = 0; ByteIndex < InputSize; ByteIndex++) {
    for (BitIndex = 0; BitIndex < 8; BitIndex++) {
      if (Input[ByteIndex] & (1 << BitIndex)) {
        UINTN TransposedByte = (BitIndex * InputSize + ByteIndex) / 8;
        UINTN TransposedBit = (BitIndex * InputSize + ByteIndex) % 8;
        Output[TransposedByte] |= (1 << TransposedBit);
      }
    }
  }

  return TRUE;
}

//
// Apply bit transposition if it reduces entropy
//
BOOLEAN
ApplyBitTransposeIfBeneficial (
  IN     CONST UINT8   *Input,
  IN     UINTN         InputSize,
  OUT    UINT8         *Output,
  OUT    BOOLEAN       *Applied
  )
{
  UINT8    *TransposedBuffer;
  FLOAT64  OriginalEntropy;
  FLOAT64  TransposedEntropy;

  TransposedBuffer = AllocatePool (InputSize);
  if (TransposedBuffer == NULL) {
    return FALSE;
  }

  //
  // Calculate original entropy
  //
  OriginalEntropy = CalculateEntropy (Input, InputSize);

  //
  // Transpose and calculate new entropy
  //
  BitTranspose (Input, InputSize, TransposedBuffer);
  TransposedEntropy = CalculateEntropy (TransposedBuffer, InputSize);

  //
  // Use transposition if it reduces entropy
  //
  if (TransposedEntropy < OriginalEntropy * 0.95) {
    CopyMem (Output, TransposedBuffer, InputSize);
    *Applied = TRUE;
  } else {
    CopyMem (Output, Input, InputSize);
    *Applied = FALSE;
  }

  FreePool (TransposedBuffer);
  return TRUE;
}
```

**Transposition Flag**: 1 bit indicating whether transposition was applied

#### 12.5.6 Stage 5: Multi-Window LZ78 Compression

ZOZ uses an advanced multi-window LZ78 variant that supports 1, 2, or 3 prediction windows.

**Configuration**:
- **Block sizes**: 8K to 16M (variable)
- **Window sizes**: 4K to 12M (variable)
- **Window modes**:
  - 1 window: Classic LZ78
  - 2 windows: Prediction from previous uncompressed block
  - 3 windows: Bidirectional prediction (MPEG B-frame style)

**1-Window Mode (Classic LZ78)**:
```
[Current Block] → [Dictionary] → [Compressed Output]
```

**2-Window Mode (Forward Prediction)**:
```
[Previous Uncompressed Block] ─┐
                               ├→ [Dictionary] → [Compressed Output]
[Current Block] ───────────────┘
```

**3-Window Mode (Bidirectional Prediction)**:
```
[Previous Uncompressed Block] ─┐
[Current Block] ───────────────├→ [Dictionary] → [Compressed Output]
[Next Uncompressed Block] ─────┘

Special block ordering required: I P B B P B B P
(I = Intra, P = Predicted, B = Bidirectional)
```

**Data Structures**:
```c
//
// LZ78 multi-window configuration
//
typedef struct _ZOZ_LZ78_CONFIG {
  UINT8   WindowCount;      // 1, 2, or 3
  UINT32  BlockSize;        // 8K to 16M
  UINT32  WindowSize;       // 4K to 12M
  UINT32  Window1Size;      // For variable windows
  UINT32  Window2Size;
  UINT32  Window3Size;
  BOOLEAN VariableWindows;  // Windows may vary in size
} ZOZ_LZ78_CONFIG;

//
// Multi-window LZ78 compression
//
BOOLEAN
Lz78MultiWindow (
  IN     CONST UINT8       *Input,
  IN     UINTN             InputSize,
  IN     CONST UINT8       *PrevBlock,    // NULL for 1-window mode
  IN     CONST UINT8       *NextBlock,    // NULL for 1/2-window modes
  IN     ZOZ_LZ78_CONFIG   *Config,
  OUT    UINT8             *Output,
  OUT    UINTN             *OutputSize
  )
{
  LZ78_DICT  *Dictionary;
  UINTN      InputIndex;
  UINTN      OutputIndex;
  UINT32     Match;

  //
  // Initialize dictionary
  //
  Dictionary = Lz78CreateDict (Config->WindowSize);
  if (Dictionary == NULL) {
    return FALSE;
  }

  //
  // Populate dictionary from previous block if available
  //
  if (PrevBlock != NULL && Config->WindowCount >= 2) {
    Lz78PopulateDict (Dictionary, PrevBlock, Config->BlockSize);
  }

  //
  // Populate dictionary from next block if available (3-window mode)
  //
  if (NextBlock != NULL && Config->WindowCount == 3) {
    Lz78PopulateDict (Dictionary, NextBlock, Config->BlockSize);
  }

  //
  // Compress input using multi-window dictionary
  //
  InputIndex = 0;
  OutputIndex = 0;

  while (InputIndex < InputSize) {
    //
    // Find longest match in dictionary
    //
    Match = Lz78FindMatch (Dictionary, &Input[InputIndex], InputSize - InputIndex);

    if (Match != 0) {
      //
      // Emit match: [dictionary index][new character]
      //
      WriteLz78Match (Output, &OutputIndex, Match);
      InputIndex += Lz78GetMatchLength (Dictionary, Match) + 1;
    } else {
      //
      // Emit literal
      //
      WriteLz78Literal (Output, &OutputIndex, Input[InputIndex]);
      InputIndex++;
    }

    //
    // Add new entry to dictionary
    //
    Lz78AddEntry (Dictionary, &Input[InputIndex - 1]);
  }

  *OutputSize = OutputIndex;
  Lz78DestroyDict (Dictionary);

  return TRUE;
}
```

**3-Window Block Ordering**:

For 3-window mode, blocks must be stored in this order:
```
Storage Order: I0 P1 P4 B2 B3 P7 B5 B6 ...
Decode Order:  I0 P1 B2 B3 P4 B5 B6 P7 ...
```

Where:
- **I** = Intra block (no prediction)
- **P** = Predicted block (uses previous block)
- **B** = Bidirectional block (uses previous AND next blocks)

**Detailed Block Encoding Sequence**:

The ZOZ LZ78 3-window mode uses a sophisticated block encoding pattern inspired by MPEG video compression:

```
Block Sequence Pattern: I P B B P B B P B B ...

Frame 0 (I):  Intra block - no prediction, self-contained
Frame 1 (P):  Forward prediction from I0
Frame 2 (B):  Bidirectional prediction from P1 and P4
Frame 3 (B):  Bidirectional prediction from P1 and P4
Frame 4 (P):  Forward prediction from P1
Frame 5 (B):  Bidirectional prediction from P4 and P7
Frame 6 (B):  Bidirectional prediction from P4 and P7
Frame 7 (P):  Forward prediction from P4
...
```

**Storage Reordering for 3-Window Mode**:

Blocks are reordered for storage to enable bidirectional prediction:

```
Logical Order (file sequence):  0  1  2  3  4  5  6  7  8  9 ...
Block Type:                     I  P  B  B  P  B  B  P  B  B ...
Storage Order:                  0  1  4  2  3  7  5  6 10  8  9 ...
                                ↑  ↑  ↑  ↑  ↑  ↑  ↑  ↑  ↑  ↑  ↑
                                I  P  P  B  B  P  B  B  P  B  B
```

**Compression Pass 1 (Intra and P-frames)**:
```
Step 1: Compress I0 (no prediction)
Step 2: Compress P1 using I0 as reference window
Step 3: Compress P4 using P1 as reference window
Step 4: Compress P7 using P4 as reference window
...
```

**Compression Pass 2 (B-frames)**:
```
Step 5: Compress B2 using P1 (previous) and P4 (next) as reference windows
Step 6: Compress B3 using P1 (previous) and P4 (next) as reference windows
Step 7: Compress B5 using P4 (previous) and P7 (next) as reference windows
Step 8: Compress B6 using P4 (previous) and P7 (next) as reference windows
...
```

**Window Population Strategy**:

For 2-window mode (forward prediction only):
```c
//
// 2-window dictionary population
//
VOID
Lz78Populate2Windows (
  IN OUT LZ78_DICT    *Dictionary,
  IN     CONST UINT8  *PrevBlock,
  IN     UINTN        PrevBlockSize,
  IN     CONST UINT8  *CurrentBlock,
  IN     UINTN        CurrentBlockSize
  )
{
  //
  // Stage 1: Pre-populate dictionary with patterns from previous block
  //
  for (UINTN Offset = 0; Offset < PrevBlockSize; Offset++) {
    Lz78AddDictEntry (Dictionary, &PrevBlock[Offset],
                      MIN(32, PrevBlockSize - Offset),
                      TRUE);  // Mark as reference-only
  }

  //
  // Stage 2: Compress current block using pre-populated dictionary
  // New entries from current block are added during compression
  //
}
```

For 3-window mode (bidirectional prediction):
```c
//
// 3-window dictionary population for B-frames
//
VOID
Lz78Populate3Windows (
  IN OUT LZ78_DICT    *Dictionary,
  IN     CONST UINT8  *PrevBlock,    // P-frame before B-frame
  IN     UINTN        PrevBlockSize,
  IN     CONST UINT8  *CurrentBlock, // B-frame being compressed
  IN     UINTN        CurrentBlockSize,
  IN     CONST UINT8  *NextBlock,    // P-frame after B-frame
  IN     UINTN        NextBlockSize
  )
{
  UINTN   Offset;
  UINT32  Weight;

  //
  // Stage 1: Pre-populate from previous P-frame (higher weight)
  //
  for (Offset = 0; Offset < PrevBlockSize; Offset += 4) {
    Lz78AddWeightedEntry (
      Dictionary,
      &PrevBlock[Offset],
      MIN(64, PrevBlockSize - Offset),
      2  // Weight: previous block preferred
    );
  }

  //
  // Stage 2: Pre-populate from next P-frame (lower weight)
  //
  for (Offset = 0; Offset < NextBlockSize; Offset += 8) {
    Lz78AddWeightedEntry (
      Dictionary,
      &NextBlock[Offset],
      MIN(64, NextBlockSize - Offset),
      1  // Weight: next block less preferred
    );
  }

  //
  // Stage 3: Compress current B-frame
  // When finding matches, prefer higher-weight entries
  //
}
```

**Block Encoding Efficiency**:

The 2/3 window prediction provides significant compression improvements:

```
1-window mode (classic LZ78):
  Block size: 256 KB
  Compressed: 128 KB (50% compression ratio)
  Dictionary built from current block only

2-window mode (forward prediction):
  Block size: 256 KB
  Compressed: 96 KB (62.5% compression ratio)
  Dictionary pre-populated from previous block
  Improvement: +25% over 1-window

3-window mode (bidirectional prediction):
  Block size: 256 KB
  Compressed: 80 KB (68.75% compression ratio)
  Dictionary pre-populated from prev + next blocks
  Improvement: +37.5% over 1-window, +16.7% over 2-window
```

**Typical Use Cases by Window Mode**:

| Mode | Best For | Compression | Speed | Memory | Complexity |
|------|----------|-------------|-------|--------|------------|
| 1-window | Streaming, real-time | Baseline | Fast | Low | Simple |
| 2-window | Archive files, backups | +25% | Medium | Medium | Medium |
| 3-window | Maximum compression | +37% | Slow | High | Complex |

**Decoding Dependencies**:

For extraction, blocks must be decoded in the correct order:

```
Decode sequence for 3-window mode:
  1. Decode I0 (no dependencies)
  2. Decode P1 (depends on I0)
  3. Store P1 in buffer (needed for B2, B3)
  4. Decode P4 (depends on P1)
  5. Store P4 in buffer (needed for B2, B3)
  6. Decode B2 (depends on P1 + P4)
  7. Decode B3 (depends on P1 + P4)
  8. Release P1 from buffer (no longer needed)
  9. Decode P7 (depends on P4)
  10. Decode B5 (depends on P4 + P7)
  11. Decode B6 (depends on P4 + P7)
  12. Release P4 from buffer
  ...

Memory requirement: 2-3 uncompressed blocks in buffer
```

**Block Type Selection Algorithm**:

```c
//
// Determine block type for optimal compression
//
LZ78_BLOCK_TYPE
DetermineBlockType (
  IN     UINTN   BlockIndex,
  IN     UINT8   WindowMode
  )
{
  if (WindowMode == 1) {
    //
    // 1-window mode: all blocks are intra
    //
    return Lz78BlockIntra;
  }

  if (WindowMode == 2) {
    //
    // 2-window mode: first block intra, rest predicted
    //
    return (BlockIndex == 0) ? Lz78BlockIntra : Lz78BlockPredicted;
  }

  if (WindowMode == 3) {
    //
    // 3-window mode: I P B B pattern
    //
    if (BlockIndex == 0) {
      return Lz78BlockIntra;
    }

    //
    // Pattern: positions 1, 4, 7, 10, ... are P-frames
    // Pattern: positions 2, 3, 5, 6, 8, 9, ... are B-frames
    //
    UINTN Offset = BlockIndex - 1;  // Adjust for I-frame at position 0
    UINTN GroupPos = Offset % 3;

    if (GroupPos == 0) {
      return Lz78BlockPredicted;  // P-frame
    } else {
      return Lz78BlockBidirectional;  // B-frame
    }
  }

  return Lz78BlockIntra;
}
```

#### 12.5.7 ZOZ Block Format

```c
//
// ZOZ compressed block header
//
typedef struct _ZOZ_BLOCK_HEADER {
  UINT32  Magic;              // 0x5A4F5A00 ("ZOZ\0")
  UINT32  UncompressedSize;   // Original size
  UINT32  CompressedSize;     // After all stages
  UINT8   BlockSorted;        // 0=no, 1=yes (256-byte block sorting applied)
  UINT8   TransformType;      // 0-4: none/BWT/MTF/BWT→MTF/MTF→BWT
  UINT8   SymbolEncoding;     // 0-2: raw/RAD50/bit-squish
  UINT8   BitTranspose;       // 0=no, 1=yes
  UINT8   Lz78Windows;        // 1-3: window count
  UINT8   RemapEncoding;      // Remap table encoding mode (if BlockSorted=1)
  UINT16  Reserved1;          // Reserved for alignment
  UINT32  Lz78BlockSize;      // 8K-16M
  UINT32  Lz78WindowSize;     // 4K-12M
  UINT32  RemapTableSize;     // Size of remap table (if BlockSorted=1)
} ZOZ_BLOCK_HEADER;
```

**Block Structure (without block sorting)**:
```
[ZOZ_BLOCK_HEADER]
[Transform metadata] (if applicable: BWT primary index, etc.)
[RLE bit-stream] (bit-aligned ULEB128 format)
[LZ78 compressed data]
[Padding to byte boundary]
```

**Block Structure (with block sorting)**:
```
[ZOZ_BLOCK_HEADER] (BlockSorted=1)
[Compact remap table] (RemapTableSize bytes)
[Transform metadata] (if applicable: BWT primary index, etc.)
[RLE bit-stream] (bit-aligned ULEB128 format)
[LZ78 compressed data]
[Padding to byte boundary]
```
```

#### 12.5.8 ZOZ Conformance and Performance

**Conformance**: Full

**Performance Characteristics**:
- **Compression Speed**: Very slow (5-20x slower than LZMA2)
- **Compression Ratio**: Maximum (typically 15-25% better than LZMA2)
  - **With block sorting**: Up to 85% additional improvement for repetitive 256-byte patterns
  - **Examples**: Log files (6.7x improvement), database dumps (95% ratio), VM images (99% ratio)
- **Decompression Speed**: Medium (2-3x slower than LZMA2)
- **Memory Usage**: High (10-100 MB depending on block/window sizes)

**Best Use Cases**:
- Archival compression (maximum ratio priority)
- Source code repositories
- Text and log file archives
- Structured data (JSON, XML, CSV)
- Database dumps and log files (especially with block sorting)
- Virtual machine disk images (with block sorting)
- Long-term storage where compression time is acceptable

**Not Recommended For**:
- Real-time compression
- Streaming applications
- Low-memory environments
- Pre-compressed data (JPEG, PNG, MP4)

## 12.6 B# Tree Filesystem Layout [OPTIONAL]

For archives that need to be mounted as filesystems, Zoo64 supports an optimized B# tree hierarchy instead of storing full paths in each file entry.

### 12.6.1 B# Tree Overview

B# trees (B-sharp trees) are a variant of B+ trees optimized for:
- Fast lookups (O(log n))
- Efficient iteration (sequential access)
- Cache-friendly node layout
- Prefix compression for paths
- Copy-on-write friendly (for FUSE mounts)

```c
//
// B# Tree filesystem layout mode
// When archive flag bit 20 is set (B# Tree Mode)
//
typedef struct _ZOO64_BSHARP_HEADER {
  UINT64  Magic;              // 0x4253484152502020 ("BSHARP  ")
  UINT32  Version;            // B# tree format version
  UINT32  NodeSize;           // Node size in bytes (power of 2, typically 4KB)
  UINT32  MaxDegree;          // Maximum node degree
  UINT32  TreeHeight;         // Height of tree
  UINT64  RootNodeOffset;     // Offset to root node
  OUT    UINT8         *Output,
  OUT    UINTN         *OutputSize
  )
{
  UINTN    ReadPos;
  UINTN    WritePos;
  UINT8    RunValue;
  UINTN    RunLength;

  ReadPos = 0;
  WritePos = 0;

  while (ReadPos < InputSize) {
    //
    // Try to encode RAD-50 triplet (uppercase letters, digits, space, $, .)
    //
    if (ReadPos + 2 < InputSize &&
        IsRad50Char (Input[ReadPos]) &&
        IsRad50Char (Input[ReadPos + 1]) &&
        IsRad50Char (Input[ReadPos + 2])) {

      UINT16 Rad50Value;

      //
      // Encode three characters as 16-bit RAD-50 value
      // Value = C1*1600 + C2*40 + C3 (where Cn is character code)
      //
      Rad50Value = (CharToRad50 (Input[ReadPos]) * 1600) +
                   (CharToRad50 (Input[ReadPos + 1]) * 40) +
                   CharToRad50 (Input[ReadPos + 2]);

      //
      // Write RAD-50 token
      //
      Output[WritePos++] = 0x01;  // RAD-50 marker
      Output[WritePos++] = (UINT8)(Rad50Value >> 8);
      Output[WritePos++] = (UINT8)(Rad50Value & 0xFF);

      ReadPos += 3;
    } else {
      //
      // Detect runs for RLE
      //
      RunValue = Input[ReadPos];
      RunLength = 1;

      while (ReadPos + RunLength < InputSize &&
             Input[ReadPos + RunLength] == RunValue &&
             RunLength < 127) {
        RunLength++;
      }

      if (RunLength >= 3) {
        //
        // Encode as run
        //
        Output[WritePos++] = 0x02;  // RLE marker
        Output[WritePos++] = RunValue;
        WritePos += EncodeLeb128 (RunLength, &Output[WritePos]);
      } else {
        //
        // Encode as literal
        //
        Output[WritePos++] = 0x00;  // Literal marker
        Output[WritePos++] = RunValue;
      }

      ReadPos += RunLength;
    }
  }

  //
  // Apply bit transposition for better compression
  //
  BitTranspose (Output, WritePos);

  *OutputSize = WritePos;
  return TRUE;
}

//
// LEB128 (Little Endian Base 128) Encoding
//
UINTN
EncodeLeb128 (
  IN     UINTN    Value,
  OUT    UINT8    *Output
  )
{
  UINTN  ByteCount;

  ByteCount = 0;

  do {
    UINT8 Byte;

    Byte = (UINT8)(Value & 0x7F);
    Value >>= 7;

    if (Value != 0) {
      Byte |= 0x80;  // More bytes follow
    }

    Output[ByteCount++] = Byte;
  } while (Value != 0);

  return ByteCount;
}

//
// Windowed LZ78 Dictionary Compression
//
BOOLEAN
Lz78Compress (
  IN     CONST UINT8   *Input,
  IN     UINTN         InputSize,
  OUT    UINT8         *Output,
  OUT    UINTN         *OutputSize,
  IN     UINT32        WindowSize
  )
{
  LZ78_DICT    *Dictionary;
  UINTN        ReadPos;
  UINTN        WritePos;
  UINT32       PhraseIndex;
  UINT32       MatchLength;

  //
  // Allocate dictionary
  //
  Dictionary = Lz78CreateDictionary (WindowSize);
  if (Dictionary == NULL) {
    return FALSE;
  }

  ReadPos = 0;
  WritePos = 0;

  while (ReadPos < InputSize) {
    //
    // Find longest match in dictionary
    //
    PhraseIndex = Lz78FindMatch (
                    Dictionary,
                    &Input[ReadPos],
                    InputSize - ReadPos,
                    &MatchLength
                    );

    //
    // Output: (phrase_index, next_char)
    //
    WritePos += EncodeLeb128 (PhraseIndex, &Output[WritePos]);

    if (ReadPos + MatchLength < InputSize) {
      Output[WritePos++] = Input[ReadPos + MatchLength];

      //
      // Add new phrase to dictionary
      //
      Lz78AddPhrase (Dictionary, PhraseIndex, Input[ReadPos + MatchLength]);
    }

    ReadPos += MatchLength + 1;
  }

  Lz78FreeDictionary (Dictionary);
  *OutputSize = WritePos;
  return TRUE;
}

//
// Range Encoding (Adaptive Arithmetic Coding)
//
BOOLEAN
RangeEncode (
  IN     CONST UINT8   *Input,
  IN     UINTN         InputSize,
  OUT    UINT8         *Output,
  OUT    UINTN         *OutputSize
  )
{
  UINT64   Low;
  UINT64   High;
  UINT32   Frequency[256];
  UINT32   CumulativeFreq[257];
  UINTN    Index;
  UINTN    WritePos;

  //
  // Initialize frequency table
  //
  ZeroMem (Frequency, sizeof(Frequency));

  //
  // Count frequencies
  //
  for (Index = 0; Index < InputSize; Index++) {
    Frequency[Input[Index]]++;
  }

  //
  // Build cumulative frequency table
  //
  CumulativeFreq[0] = 0;
  for (Index = 0; Index < 256; Index++) {
    CumulativeFreq[Index + 1] = CumulativeFreq[Index] + Frequency[Index];
  }

  //
  // Initialize range
  //
  Low = 0;
  High = 0xFFFFFFFFFFFFFFFFULL;
  WritePos = 0;

  //
  // Encode each symbol
  //
  for (Index = 0; Index < InputSize; Index++) {
    UINT8  Symbol;
    UINT64 Range;
    UINT64 Total;

    Symbol = Input[Index];
    Range = High - Low + 1;
    Total = CumulativeFreq[256];

    //
    // Narrow range based on symbol frequency
    //
    High = Low + (Range * CumulativeFreq[Symbol + 1]) / Total - 1;
    Low = Low + (Range * CumulativeFreq[Symbol]) / Total;

    //
    // Output bytes when range narrows
    //
    while ((High ^ Low) < 0x100000000ULL) {
      Output[WritePos++] = (UINT8)(Low >> 56);
      Low <<= 8;
      High = (High << 8) | 0xFF;
    }
  }

  //
  // Output final range
  //
  while (WritePos < 8) {
    Output[WritePos++] = (UINT8)(Low >> 56);
    Low <<= 8;
  }

  *OutputSize = WritePos;
  return TRUE;
}

//
// Complete Pipeline
//
BOOLEAN
Zoo64Compress (
  IN     CONST UINT8   *Input,
  IN     UINTN         InputSize,
  OUT    UINT8         *Output,
  OUT    UINTN         *OutputSize,
  IN     UINT32        WindowSize,
  IN     UINT32        CompressionLevel
  )
{
  UINT8   *TempBuffer1;
  UINT8   *TempBuffer2;
  UINTN   TempSize;
  UINTN   PrimaryIndex;
  BOOLEAN Status;

  //
  // Allocate temporary buffers
  //
  TempBuffer1 = AllocatePool (InputSize * 2);
  TempBuffer2 = AllocatePool (InputSize * 2);

  if (TempBuffer1 == NULL || TempBuffer2 == NULL) {
    if (TempBuffer1 != NULL) FreePool (TempBuffer1);
    if (TempBuffer2 != NULL) FreePool (TempBuffer2);
    return FALSE;
  }

  //
  // Stage 1: Burrows-Wheeler Transform
  //
  Status = BwtTransform (Input, InputSize, TempBuffer1, &PrimaryIndex);
  if (!Status) goto Cleanup;

  //
  // Stage 2: Move-To-Front
  //
  Status = MtfTransform (TempBuffer1, InputSize, TempBuffer2);
  if (!Status) goto Cleanup;

  //
  // Stage 3: RAD50RLE (RAD-50 + RLE + LEB128 + Bit Transpose)
  //
  Status = Rad50RleEncode (TempBuffer2, InputSize, TempBuffer1, &TempSize);
  if (!Status) goto Cleanup;

  //
  // Stage 4: Windowed LZ78
  //
  Status = Lz78Compress (TempBuffer1, TempSize, TempBuffer2, &TempSize, WindowSize);
  if (!Status) goto Cleanup;

  //
  // Stage 5: Range Encoding
  //
  Status = RangeEncode (TempBuffer2, TempSize, Output, OutputSize);
  if (!Status) goto Cleanup;

  //
  // Store primary index for BWT inverse
  //
  *(UINTN*)Output = PrimaryIndex;
  (*OutputSize) += sizeof(UINTN);

Cleanup:
  FreePool (TempBuffer1);
  FreePool (TempBuffer2);

  return Status;
}
```

## 12.6 B# Tree Filesystem Layout [OPTIONAL]

For archives that need to be mounted as filesystems, Zoo64 supports an optimized B# tree hierarchy instead of storing full paths in each file entry.

### 12.6.1 B# Tree Overview

B# trees (B-sharp trees) are a variant of B+ trees optimized for:
- Fast lookups (O(log n))
- Efficient iteration (sequential access)
- Cache-friendly node layout
- Prefix compression for paths
- Copy-on-write friendly (for FUSE mounts)

```c
//
// B# Tree filesystem layout mode
// When archive flag bit 20 is set (B# Tree Mode)
//
typedef struct _ZOO64_BSHARP_HEADER {
  UINT64  Magic;              // 0x4253484152502020 ("BSHARP  ")
  UINT32  Version;            // B# tree format version
  UINT32  NodeSize;           // Node size in bytes (power of 2, typically 4KB)
  UINT32  MaxDegree;          // Maximum node degree
  UINT32  TreeHeight;         // Height of tree
  UINT64  RootNodeOffset;     // Offset to root node
  UINT64  NodeCount;          // Total number of nodes
  UINT32  Flags;              // B# tree flags
  UINT32  Reserved;
} ZOO64_BSHARP_HEADER;

//
// B# Tree Node (Internal or Leaf)
//
typedef struct _ZOO64_BSHARP_NODE {
  UINT32  Magic;              // 0x42534E4F ("BSNO")
  UINT16  NodeType;           // 0=Internal, 1=Leaf
  UINT16  EntryCount;         // Number of entries in node
  UINT64  ParentOffset;       // Offset to parent node (0 for root)
  UINT64  PrevLeafOffset;     // Previous leaf (0 if not leaf or first)
  UINT64  NextLeafOffset;     // Next leaf (0 if not leaf or last)

  //
  // Followed by entries based on node type
  //
} ZOO64_BSHARP_NODE;

//
// Internal Node Entry (points to child nodes)
//
typedef struct _ZOO64_BSHARP_INTERNAL_ENTRY {
  UINT16  KeyLength;          // Length of separator key
  UINT16  PrefixLength;       // Common prefix length (for compression)
  UINT64  ChildOffset;        // Offset to child node

  // Followed by:
  //   [PrefixLength bytes: common prefix (shared with siblings)]
  //   [KeyLength bytes: unique suffix]
} ZOO64_BSHARP_INTERNAL_ENTRY;

//
// Leaf Node Entry (points to file data)
//
typedef struct _ZOO64_BSHARP_LEAF_ENTRY {
  UINT16  NameLength;         // Length of filename component
  UINT16  PrefixLength;       // Common prefix with siblings
  UINT32  FileIndex;          // Index into file table
  UINT64  FileOffset;         // Offset to file data
  UINT64  FileSize;           // File size
  UINT32  Attributes;         // File attributes
  UINT32  Reserved;

  // Followed by:
  //   [PrefixLength bytes: common prefix]
  //   [NameLength bytes: unique suffix]
} ZOO64_BSHARP_LEAF_ENTRY;
```

### 12.6.2 Path-Free File Entries

When B# tree mode is enabled, file entries no longer store paths:

```c
//
// Simplified file entry for B# tree mode
// Path is stored in B# tree, referenced by FileIndex
//
typedef struct _ZOO64_FILE_ENTRY_BSHARP {
  UINT64  Magic;              // 0x46494C45454E5452 ("FILEENTR")
  UINT32  FileIndex;          // Index in B# tree
  UINT32  Flags;              // File flags

  // Size fields
  UINT64  UncompressedSize;
  UINT64  CompressedSize;

  // Timestamps
  UINT64  BirthTime;
  UINT64  ModificationTime;
  UINT64  AccessTime;
  UINT64  ChangeTime;

  // Checksums
  UINT32  CRC32;
  UINT64  SHA256[4];

  // Metadata
  UINT64  MetadataOffset;
  UINT32  MetadataSize;

  // Attributes
  UINT32  UID;
  UINT32  GID;
  UINT32  Mode;
  UINT32  Attributes;

  // NO path field - path is in B# tree
} ZOO64_FILE_ENTRY_BSHARP;
```

### 12.6.3 Benefits of B# Tree Mode

**Space Efficiency**:
- Path prefix compression reduces redundancy
- Common directory components stored once
- 30-50% space savings for deep directory trees

**Performance**:
- O(log n) lookups vs O(n) linear scan
- Cache-friendly node layout (4KB nodes match page size)
- Sequential iteration for directory listings
- Fast prefix searches (all files in directory)

**Filesystem Mounting**:
- Direct mapping to FUSE operations
- No path parsing required
- Efficient readdir() implementation
- Natural tree traversal

**Example Space Savings**:
```
Traditional (with full paths):
  /usr/local/share/doc/project/file1.txt  (37 bytes)
  /usr/local/share/doc/project/file2.txt  (37 bytes)
  /usr/local/share/doc/project/file3.txt  (37 bytes)
  Total: 111 bytes for paths

B# Tree (with prefix compression):
  Root → usr → local → share → doc → project
    ├─ file1.txt (9 bytes)
    ├─ file2.txt (9 bytes)
    └─ file3.txt (9 bytes)
  Total: ~45 bytes (including tree structure)
  Savings: 60%
```

### 12.6.4 Compatibility

**Quick Directory**: Still present, references FileIndex
**Central Directory**: Still present, uses FileIndex as key
**End of Archive**: Unchanged

B# tree is an **alternative representation**, not a replacement. Archives can be converted between path-based and B# tree modes without data loss.

### 12.7 Bit Squishing Compression (Algorithm 0x0010)

Bit squishing is a specialized compression algorithm optimized for data with reduced symbol alphabets. It analyzes blocks to determine unique symbol count and uses bit-packing to achieve excellent compression ratios for text, source code, and structured data.

#### 12.7.1 Algorithm Overview

The bit squishing algorithm operates in the following stages:

1. **Alphabet Analysis**: Count unique symbols in the block
2. **Symbol Reduction**: If unique symbols ≤ 128, proceed with compression
3. **Minimum Subtraction**: Subtract minimum symbol value from all symbols
4. **Encoding Selection**: Choose encoding based on alphabet size:
   - ≤ 8 symbols: Emit symbol table (RAD50 or raw), pack indexes using 1-3 bits
   - ≤ 16 symbols: Emit symbol table, pack indexes using 4 bits
   - ≤ 32 symbols: Emit symbol table, pack indexes using 5 bits
   - ≤ 64 symbols: Emit symbol table, pack indexes using 6 bits
   - ≤ 128 symbols: Emit symbol table, pack indexes using 7 bits
   - > 128 symbols: Use alternative compression (no benefit)

5. **Bitmap vs Table**: For larger alphabets, emit bitmap (0 to max value) where bit 1 = symbol used

#### 12.7.2 Data Structures

```c
//
// Bit squish block header
//
typedef struct _ZOO64_BITSQUISH_HEADER {
  UINT32  Magic;              // 0x42535148 ("BSQH")
  UINT32  UncompressedSize;   // Original block size
  UINT32  CompressedSize;     // Compressed size (including header)
  UINT16  AlphabetSize;       // Number of unique symbols (1-128)
  UINT8   MinSymbol;          // Minimum symbol value (for subtraction)
  UINT8   BitsPerSymbol;      // Bits per symbol index (1-7)
  UINT32  Flags;              // Encoding flags
  UINT32  Reserved;
} ZOO64_BITSQUISH_HEADER;

//
// Bit squish encoding flags
//
#define ZOO64_BITSQUISH_USE_BITMAP    0x00000001  // Use bitmap encoding
#define ZOO64_BITSQUISH_USE_RAD50     0x00000002  // Use RAD50 for symbol table
#define ZOO64_BITSQUISH_USE_RLE       0x00000004  // Apply RLE to bit stream
```

**Encoding Format**:

For alphabet ≤ 8 symbols (small table):
```
[ZOO64_BITSQUISH_HEADER]
[Symbol count: UINT8]
[Symbol table: UINT8[AlphabetSize]] (subtracted values, or RAD50 encoded)
[Packed bit stream] (indexes using BitsPerSymbol bits each)
```

For alphabet > 8 symbols (bitmap):
```
[ZOO64_BITSQUISH_HEADER]
[Bitmap size: UINT16] (in bytes)
[Bitmap: UINT8[]] (bit 1 = symbol present, covers range MinSymbol to MaxSymbol)
[Symbol count: UINT8]
[Packed bit stream] (indexes into bitmap using BitsPerSymbol bits each)
```

#### 12.7.3 Compression Examples

**Example 1: ASCII Text (uppercase only)**
- Input: "HELLO WORLD" (11 bytes)
- Unique symbols: 8 (' ', 'D', 'E', 'H', 'L', 'O', 'R', 'W')
- MinSymbol: 0x20 (space)
- Alphabet after subtraction: [0, 36, 37, 40, 44, 47, 50, 55]
- BitsPerSymbol: 3 (can represent 0-7)
- Symbol mapping: {' ':0, 'D':1, 'E':2, 'H':3, 'L':4, 'O':5, 'R':6, 'W':7}
- Encoded: 3-5-4-4-5-0-7-5-6-4-1 (33 bits = 5 bytes vs 11 bytes original)
- Compression ratio: ~54%

**Example 2: Binary Data (4 values)**
- Input: byte stream with only values {0x00, 0x01, 0x10, 0x11}
- AlphabetSize: 4
- MinSymbol: 0x00
- BitsPerSymbol: 2
- Symbol mapping: {0x00:0, 0x01:1, 0x10:2, 0x11:3}
- Compression ratio: ~25% (2 bits per byte vs 8 bits)

**Example 3: Source Code**
- Input: C source code (alphanumeric + symbols)
- Typical unique symbols: 40-60 characters
- BitsPerSymbol: 6 (can represent 0-63)
- Compression ratio: ~75% (6 bits per byte vs 8 bits)

#### 12.7.4 Implementation (NT/UEFI Style)

Complete implementation of bit squishing compression:

```c
//
// Zoo64 Bit Squishing Implementation
// Copyright (c) 2025. All rights reserved.
//
// SPDX-License-Identifier: BSD-3-Clause
//

#include "zoo64_compress.h"

//
// Analyze block to determine unique symbols
//
BOOLEAN
AnalyzeAlphabet (
  IN     CONST UINT8   *Input,
  IN     UINTN         InputSize,
  OUT    UINT8         *SymbolTable,
  OUT    UINT16        *AlphabetSize,
  OUT    UINT8         *MinSymbol,
  OUT    UINT8         *MaxSymbol
  )
{
  BOOLEAN  SymbolPresent[256];
  UINTN    Index;
  UINT16   Count;
  UINT8    Min;
  UINT8    Max;

  //
  // Initialize symbol presence array
  //
  SetMem (SymbolPresent, sizeof(SymbolPresent), 0);

  Min = 0xFF;
  Max = 0x00;

  //
  // Scan input to find unique symbols
  //
  for (Index = 0; Index < InputSize; Index++) {
    UINT8 Symbol = Input[Index];
    SymbolPresent[Symbol] = TRUE;

    if (Symbol < Min) {
      Min = Symbol;
    }
    if (Symbol > Max) {
      Max = Symbol;
    }
  }

  //
  // Build symbol table
  //
  Count = 0;
  for (Index = 0; Index < 256; Index++) {
    if (SymbolPresent[Index]) {
      SymbolTable[Count++] = (UINT8)Index;
    }
  }

  *AlphabetSize = Count;
  *MinSymbol = Min;
  *MaxSymbol = Max;

  return TRUE;
}

//
// Build symbol-to-index mapping
//
BOOLEAN
BuildSymbolMap (
  IN     CONST UINT8   *SymbolTable,
  IN     UINT16        AlphabetSize,
  OUT    UINT8         *SymbolToIndex
  )
{
  UINT16  Index;

  //
  // Initialize mapping to 0xFF (invalid)
  //
  SetMem (SymbolToIndex, 256, 0xFF);

  //
  // Map each symbol to its index
  //
  for (Index = 0; Index < AlphabetSize; Index++) {
    SymbolToIndex[SymbolTable[Index]] = (UINT8)Index;
  }

  return TRUE;
}

//
// Pack symbols using specified bits per symbol
//
BOOLEAN
PackBitStream (
  IN     CONST UINT8   *Input,
  IN     UINTN         InputSize,
  IN     CONST UINT8   *SymbolToIndex,
  IN     UINT8         BitsPerSymbol,
  OUT    UINT8         *Output,
  OUT    UINTN         *OutputSize
  )
{
  UINT64  BitBuffer;
  UINT32  BitsInBuffer;
  UINTN   InputIndex;
  UINTN   OutputIndex;
  UINT8   SymbolIndex;

  BitBuffer = 0;
  BitsInBuffer = 0;
  OutputIndex = 0;

  //
  // Process each input symbol
  //
  for (InputIndex = 0; InputIndex < InputSize; InputIndex++) {
    SymbolIndex = SymbolToIndex[Input[InputIndex]];
    if (SymbolIndex == 0xFF) {
      return FALSE;  // Invalid symbol
    }

    //
    // Add symbol index to bit buffer
    //
    BitBuffer |= ((UINT64)SymbolIndex << BitsInBuffer);
    BitsInBuffer += BitsPerSymbol;

    //
    // Flush complete bytes
    //
    while (BitsInBuffer >= 8) {
      Output[OutputIndex++] = (UINT8)(BitBuffer & 0xFF);
      BitBuffer >>= 8;
      BitsInBuffer -= 8;
    }
  }

  //
  // Flush remaining bits
  //
  if (BitsInBuffer > 0) {
    Output[OutputIndex++] = (UINT8)(BitBuffer & 0xFF);
  }

  *OutputSize = OutputIndex;
  return TRUE;
}

//
// Unpack bit stream back to symbols
//
BOOLEAN
UnpackBitStream (
  IN     CONST UINT8   *Input,
  IN     UINTN         InputSize,
  IN     CONST UINT8   *SymbolTable,
  IN     UINT8         BitsPerSymbol,
  IN     UINTN         OutputSize,
  OUT    UINT8         *Output
  )
{
  UINT64  BitBuffer;
  UINT32  BitsInBuffer;
  UINTN   InputIndex;
  UINTN   OutputIndex;
  UINT8   SymbolIndex;
  UINT8   IndexMask;

  //
  // Calculate mask for extracting symbol indexes
  //
  IndexMask = (UINT8)((1 << BitsPerSymbol) - 1);

  BitBuffer = 0;
  BitsInBuffer = 0;
  InputIndex = 0;
  OutputIndex = 0;

  //
  // Process until output complete
  //
  while (OutputIndex < OutputSize) {
    //
    // Refill bit buffer if needed
    //
    while (BitsInBuffer < BitsPerSymbol && InputIndex < InputSize) {
      BitBuffer |= ((UINT64)Input[InputIndex++] << BitsInBuffer);
      BitsInBuffer += 8;
    }

    if (BitsInBuffer < BitsPerSymbol) {
      return FALSE;  // Truncated input
    }

    //
    // Extract symbol index
    //
    SymbolIndex = (UINT8)(BitBuffer & IndexMask);
    BitBuffer >>= BitsPerSymbol;
    BitsInBuffer -= BitsPerSymbol;

    //
    // Emit symbol
    //
    Output[OutputIndex++] = SymbolTable[SymbolIndex];
  }

  return TRUE;
}

//
// Compress block using bit squishing
//
BOOLEAN
BitSquishCompress (
  IN     CONST UINT8   *Input,
  IN     UINTN         InputSize,
  OUT    UINT8         *Output,
  OUT    UINTN         *OutputSize
  )
{
  ZOO64_BITSQUISH_HEADER  Header;
  UINT8                   SymbolTable[256];
  UINT8                   SymbolToIndex[256];
  UINT16                  AlphabetSize;
  UINT8                   MinSymbol;
  UINT8                   MaxSymbol;
  UINT8                   BitsPerSymbol;
  UINT8                   *PackedData;
  UINTN                   PackedSize;
  UINTN                   TotalSize;
  BOOLEAN                 Status;

  //
  // Analyze alphabet
  //
  Status = AnalyzeAlphabet (
             Input,
             InputSize,
             SymbolTable,
             &AlphabetSize,
             &MinSymbol,
             &MaxSymbol
             );
  if (!Status || AlphabetSize > 128) {
    return FALSE;  // Not suitable for bit squishing
  }

  //
  // Determine bits per symbol
  //
  if (AlphabetSize <= 2) {
    BitsPerSymbol = 1;
  } else if (AlphabetSize <= 4) {
    BitsPerSymbol = 2;
  } else if (AlphabetSize <= 8) {
    BitsPerSymbol = 3;
  } else if (AlphabetSize <= 16) {
    BitsPerSymbol = 4;
  } else if (AlphabetSize <= 32) {
    BitsPerSymbol = 5;
  } else if (AlphabetSize <= 64) {
    BitsPerSymbol = 6;
  } else {
    BitsPerSymbol = 7;
  }

  //
  // Build symbol mapping
  //
  Status = BuildSymbolMap (SymbolTable, AlphabetSize, SymbolToIndex);
  if (!Status) {
    return FALSE;
  }

  //
  // Allocate temporary buffer for packed data
  //
  PackedData = AllocatePool (InputSize);
  if (PackedData == NULL) {
    return FALSE;
  }

  //
  // Pack bit stream
  //
  Status = PackBitStream (
             Input,
             InputSize,
             SymbolToIndex,
             BitsPerSymbol,
             PackedData,
             &PackedSize
             );
  if (!Status) {
    FreePool (PackedData);
    return FALSE;
  }

  //
  // Build header
  //
  Header.Magic = 0x42535148;  // "BSQH"
  Header.UncompressedSize = (UINT32)InputSize;
  Header.AlphabetSize = AlphabetSize;
  Header.MinSymbol = MinSymbol;
  Header.BitsPerSymbol = BitsPerSymbol;
  Header.Flags = 0;
  Header.Reserved = 0;

  //
  // Determine encoding method
  //
  if (AlphabetSize <= 8) {
    //
    // Small alphabet: use symbol table
    //
    Header.Flags &= ~ZOO64_BITSQUISH_USE_BITMAP;
    TotalSize = sizeof(Header) + 1 + AlphabetSize + PackedSize;
  } else {
    //
    // Larger alphabet: use bitmap
    //
    UINT16 BitmapSize = (MaxSymbol - MinSymbol + 8) / 8;
    Header.Flags |= ZOO64_BITSQUISH_USE_BITMAP;
    TotalSize = sizeof(Header) + 2 + BitmapSize + 1 + PackedSize;
  }

  Header.CompressedSize = (UINT32)TotalSize;

  //
  // Check if compression is beneficial
  //
  if (TotalSize >= InputSize) {
    FreePool (PackedData);
    return FALSE;  // Not worth compressing
  }

  //
  // Write output
  //
  CopyMem (Output, &Header, sizeof(Header));
  UINTN Offset = sizeof(Header);

  if (AlphabetSize <= 8) {
    //
    // Write symbol table
    //
    Output[Offset++] = (UINT8)AlphabetSize;
    CopyMem (&Output[Offset], SymbolTable, AlphabetSize);
    Offset += AlphabetSize;
  } else {
    //
    // Write bitmap
    //
    UINT16 BitmapSize = (MaxSymbol - MinSymbol + 8) / 8;
    UINT8  *Bitmap = AllocateZeroPool (BitmapSize);

    if (Bitmap == NULL) {
      FreePool (PackedData);
      return FALSE;
    }

    //
    // Build bitmap
    //
    for (UINT16 i = 0; i < AlphabetSize; i++) {
      UINT8 Symbol = SymbolTable[i];
      UINT8 BitIndex = Symbol - MinSymbol;
      Bitmap[BitIndex / 8] |= (1 << (BitIndex % 8));
    }

    *((UINT16*)&Output[Offset]) = BitmapSize;
    Offset += 2;
    CopyMem (&Output[Offset], Bitmap, BitmapSize);
    Offset += BitmapSize;
    Output[Offset++] = (UINT8)AlphabetSize;

    FreePool (Bitmap);
  }

  //
  // Write packed data
  //
  CopyMem (&Output[Offset], PackedData, PackedSize);
  Offset += PackedSize;

  *OutputSize = Offset;
  FreePool (PackedData);

  return TRUE;
}

//
// Decompress bit squished block
//
BOOLEAN
BitSquishDecompress (
  IN     CONST UINT8   *Input,
  IN     UINTN         InputSize,
  OUT    UINT8         *Output,
  OUT    UINTN         *OutputSize
  )
{
  ZOO64_BITSQUISH_HEADER  *Header;
  UINT8                   SymbolTable[256];
  UINTN                   Offset;
  UINT16                  AlphabetSize;
  UINT8                   *PackedData;
  UINTN                   PackedSize;
  BOOLEAN                 Status;

  if (InputSize < sizeof(ZOO64_BITSQUISH_HEADER)) {
    return FALSE;
  }

  Header = (ZOO64_BITSQUISH_HEADER*)Input;

  if (Header->Magic != 0x42535148) {
    return FALSE;
  }

  Offset = sizeof(ZOO64_BITSQUISH_HEADER);

  if ((Header->Flags & ZOO64_BITSQUISH_USE_BITMAP) == 0) {
    //
    // Read symbol table
    //
    AlphabetSize = Input[Offset++];
    CopyMem (SymbolTable, &Input[Offset], AlphabetSize);
    Offset += AlphabetSize;
  } else {
    //
    // Read bitmap and reconstruct symbol table
    //
    UINT16 BitmapSize = *((UINT16*)&Input[Offset]);
    Offset += 2;

    CONST UINT8 *Bitmap = &Input[Offset];
    Offset += BitmapSize;

    AlphabetSize = Input[Offset++];

    UINT16 TableIndex = 0;
    for (UINT16 BitIndex = 0; BitIndex < BitmapSize * 8; BitIndex++) {
      if (Bitmap[BitIndex / 8] & (1 << (BitIndex % 8))) {
        SymbolTable[TableIndex++] = Header->MinSymbol + BitIndex;
      }
    }
  }

  //
  // Unpack bit stream
  //
  PackedData = (UINT8*)&Input[Offset];
  PackedSize = InputSize - Offset;

  Status = UnpackBitStream (
             PackedData,
             PackedSize,
             SymbolTable,
             Header->BitsPerSymbol,
             Header->UncompressedSize,
             Output
             );

  if (Status) {
    *OutputSize = Header->UncompressedSize;
  }

  return Status;
}
```

#### 12.7.5 Use Cases

**Optimal for**:
- ASCII text files (typical alphabet: 70-90 characters)
- Source code (40-80 unique characters)
- Log files with limited symbol sets
- Structured data (CSV, JSON with consistent formatting)
- Binary data with limited value ranges

**Not recommended for**:
- Pre-compressed data (JPEG, PNG, MP4)
- Encrypted data (full 0-255 alphabet)
- Binary executables (high entropy)
- Random data

**Typical Compression Ratios**:
- English text: 60-75% (6 bits/char for ~64 unique chars)
- Source code: 50-70% (depends on language)
- Log files: 40-60% (limited vocabulary)
- Binary with 16 values: 50% (4 bits/byte)
- Binary with 4 values: 25% (2 bits/byte)

#### 12.7.6 Conformance Levels

- **Tiny**: Decompression only
- **Compact**: Decompression only
- **Embedded**: Decompression only
- **Minimal**: Full support (compress + decompress)
- **Standard**: Full support with auto-detection
- **Enhanced**: Full support with hybrid mode
- **Full**: Full support with all optimizations

## 12.8 WASM32 Embeddable Modules for Custom Algorithms

Zoo64 supports embedding WASM32 (WebAssembly 32-bit) modules directly into archives. This allows archives to be self-contained with custom implementations of compression, encryption, hashing, and signing algorithms that may not be available on the target system.

**Use Cases**:
- Proprietary compression algorithms
- Custom encryption schemes
- Domain-specific hashing functions
- Organization-specific digital signatures
- Experimental or research algorithms
- Legacy algorithm support
- Cross-platform algorithm portability

### 12.8.1 WASM32 Module Overview

WASM32 modules embedded in Zoo64 archives provide custom algorithm implementations using the WebAssembly standard. Modules are stored as metadata chunks and can be loaded dynamically when the archive is opened.

**Benefits**:
- **Self-Contained**: Archive includes all code needed to process it
- **Platform-Independent**: WASM runs on any system with a WASM runtime
- **Secure**: WASM sandboxing prevents malicious code execution
- **Efficient**: Near-native performance
- **Standardized**: Uses W3C WebAssembly specification

**Security Model**:
- WASM modules run in isolated sandbox
- No file system access (except through API)
- No network access
- Limited memory (configurable, default 16MB max)
- Deterministic execution
- Resource limits (CPU time, memory allocation)

### 12.8.2 Data Structures

```c
//
// WASM32 module metadata chunk
// Metadata type: 0x0100 (Zoo64MetaWasm32Module)
//
typedef struct _ZOO64_WASM32_MODULE {
  UINT64  Magic;              // 0x5741534D33320000 ("WASM32\0\0")
  UINT32  ModuleSize;         // Size of WASM binary
  UINT32  Flags;              // Module flags
  UINT8   ModuleUuid[16];     // Unique identifier for this module
  UINT32  Version;            // Module version (major.minor.patch.build)
  UINT32  ApiVersion;         // Zoo64 WASM API version required
  UINT32  ModuleType;         // Type of algorithm(s) provided
  UINT32  AlgorithmId;        // Custom algorithm ID
  CHAR8   ModuleName[64];     // UTF-8 module name
  CHAR8   Author[64];         // UTF-8 author/organization
  UINT32  MaxMemoryMB;        // Maximum memory in MB (default 16, max 256)
  UINT32  MaxExecutionMs;     // Maximum execution time per call (ms)
  UINT8   Sha256Hash[32];     // SHA-256 hash of WASM binary
  UINT32  Reserved[8];
  // Followed by WASM binary (ModuleSize bytes)
} ZOO64_WASM32_MODULE;

//
// WASM32 module flags
//
#define ZOO64_WASM32_REQUIRED         0x00000001  // Required for archive access
#define ZOO64_WASM32_OPTIONAL         0x00000002  // Optional enhancement
#define ZOO64_WASM32_COMPRESSION      0x00000010  // Provides compression
#define ZOO64_WASM32_ENCRYPTION       0x00000020  // Provides encryption
#define ZOO64_WASM32_HASHING          0x00000040  // Provides hashing
#define ZOO64_WASM32_SIGNING          0x00000080  // Provides signing
#define ZOO64_WASM32_VERIFIED         0x00000100  // Digitally signed module
#define ZOO64_WASM32_TRUSTED          0x00000200  // From trusted source

//
// WASM32 module types
//
typedef enum _ZOO64_WASM32_MODULE_TYPE {
  Zoo64Wasm32TypeCompression    = 0x0001,  // Compression algorithm
  Zoo64Wasm32TypeEncryption     = 0x0002,  // Encryption algorithm
  Zoo64Wasm32TypeHashing        = 0x0003,  // Hashing algorithm
  Zoo64Wasm32TypeSigning        = 0x0004,  // Digital signature
  Zoo64Wasm32TypeKdf            = 0x0005,  // Key derivation function
  Zoo64Wasm32TypeMulti          = 0x00FF   // Multiple algorithms
} ZOO64_WASM32_MODULE_TYPE;
```

### 12.8.3 WASM32 API Interface

Zoo64 defines a standard API for WASM modules to implement. The API uses the WASM import/export mechanism.

**Memory Model**:
- Linear memory allocated by host
- Modules use `malloc`/`free` exported by host
- Data passed via memory pointers
- No shared memory between modules

**Imported Functions** (provided by host to WASM module):

```c
//
// Memory allocation (imported by module from host)
//
void* zoo64_malloc(uint32_t size);
void  zoo64_free(void* ptr);

//
// Logging and debugging (imported by module)
//
void zoo64_log(const char* message, uint32_t length);
void zoo64_error(const char* message, uint32_t length);

//
// Get module configuration
//
uint32_t zoo64_get_config(const char* key, char* value, uint32_t max_len);
```

**Exported Functions** (implemented by WASM module, called by host):

#### Compression Module API

```c
//
// Initialize compression module
// Returns: 0 on success, error code otherwise
//
int32_t zoo64_compress_init(void);

//
// Get algorithm information
//
void zoo64_compress_get_info(
  char* name,           // Output: algorithm name
  uint32_t name_len,    // Input: max name length
  uint32_t* flags       // Output: algorithm flags
);

//
// Compress data
// Returns: compressed size, or negative error code
//
int32_t zoo64_compress(
  const uint8_t* input,     // Input data
  uint32_t input_size,      // Input size
  uint8_t* output,          // Output buffer
  uint32_t output_max,      // Output buffer size
  uint32_t level            // Compression level (0-9)
);

//
// Decompress data
// Returns: decompressed size, or negative error code
//
int32_t zoo64_decompress(
  const uint8_t* input,     // Compressed data
  uint32_t input_size,      // Compressed size
  uint8_t* output,          // Output buffer
  uint32_t output_max       // Output buffer size
);

//
// Cleanup
//
void zoo64_compress_cleanup(void);
```

#### Encryption Module API

```c
//
// Initialize encryption module
//
int32_t zoo64_encrypt_init(void);

//
// Get algorithm information
//
void zoo64_encrypt_get_info(
  char* name,
  uint32_t name_len,
  uint32_t* key_size,      // Key size in bits
  uint32_t* block_size,    // Block size in bytes
  uint32_t* flags
);

//
// Encrypt data
// Returns: encrypted size (may include IV, tag), or negative error code
//
int32_t zoo64_encrypt(
  const uint8_t* input,     // Plaintext
  uint32_t input_size,
  const uint8_t* key,       // Encryption key
  uint32_t key_size,
  const uint8_t* iv,        // Initialization vector (may be NULL)
  uint32_t iv_size,
  uint8_t* output,          // Output buffer (ciphertext)
  uint32_t output_max
);

//
// Decrypt data
// Returns: decrypted size, or negative error code
//
int32_t zoo64_decrypt(
  const uint8_t* input,     // Ciphertext
  uint32_t input_size,
  const uint8_t* key,       // Decryption key
  uint32_t key_size,
  const uint8_t* iv,        // Initialization vector (may be NULL)
  uint32_t iv_size,
  uint8_t* output,          // Output buffer (plaintext)
  uint32_t output_max
);

//
// Cleanup
//
void zoo64_encrypt_cleanup(void);
```

#### Hashing Module API

```c
//
// Initialize hashing module
//
int32_t zoo64_hash_init(void);

//
// Get algorithm information
//
void zoo64_hash_get_info(
  char* name,
  uint32_t name_len,
  uint32_t* hash_size,     // Hash output size in bytes
  uint32_t* flags
);

//
// Hash data (one-shot)
// Returns: hash size, or negative error code
//
int32_t zoo64_hash(
  const uint8_t* input,     // Data to hash
  uint32_t input_size,
  uint8_t* output,          // Hash output buffer
  uint32_t output_max
);

//
// Incremental hashing (for large files)
//
void* zoo64_hash_create_context(void);
int32_t zoo64_hash_update(void* ctx, const uint8_t* data, uint32_t size);
int32_t zoo64_hash_finalize(void* ctx, uint8_t* output, uint32_t output_max);
void zoo64_hash_destroy_context(void* ctx);

//
// Cleanup
//
void zoo64_hash_cleanup(void);
```

#### Signing Module API

```c
//
// Initialize signing module
//
int32_t zoo64_sign_init(void);

//
// Get algorithm information
//
void zoo64_sign_get_info(
  char* name,
  uint32_t name_len,
  uint32_t* signature_size, // Signature size in bytes
  uint32_t* flags
);

//
// Sign data
// Returns: signature size, or negative error code
//
int32_t zoo64_sign(
  const uint8_t* data,      // Data to sign
  uint32_t data_size,
  const uint8_t* private_key, // Private key
  uint32_t key_size,
  uint8_t* signature,       // Signature output
  uint32_t signature_max
);

//
// Verify signature
// Returns: 1 if valid, 0 if invalid, negative on error
//
int32_t zoo64_verify(
  const uint8_t* data,      // Signed data
  uint32_t data_size,
  const uint8_t* signature, // Signature to verify
  uint32_t signature_size,
  const uint8_t* public_key, // Public key
  uint32_t key_size
);

//
// Cleanup
//
void zoo64_sign_cleanup(void);
```

### 12.8.4 Module Loading and Execution

**Loading Process**:

1. **Discovery**: Scan archive metadata for WASM32 modules
2. **Validation**: Verify SHA-256 hash of WASM binary
3. **Parse**: Parse WASM binary header
4. **Instantiate**: Load WASM module into runtime
5. **Initialize**: Call module's init function
6. **Register**: Register custom algorithm with Zoo64

**Example Implementation (NT/UEFI Style)**:

```c
//
// Load WASM32 module from archive metadata
//
BOOLEAN
LoadWasm32Module (
  IN     ZOO64_WASM32_MODULE  *ModuleMetadata,
  OUT    WASM_MODULE_HANDLE   *Handle
  )
{
  UINT8    *WasmBinary;
  UINT8    ComputedHash[32];
  BOOLEAN  Status;

  //
  // Allocate buffer for WASM binary
  //
  WasmBinary = AllocatePool (ModuleMetadata->ModuleSize);
  if (WasmBinary == NULL) {
    return FALSE;
  }

  //
  // Copy WASM binary from metadata
  //
  CopyMem (
    WasmBinary,
    (UINT8*)ModuleMetadata + sizeof(ZOO64_WASM32_MODULE),
    ModuleMetadata->ModuleSize
    );

  //
  // Verify SHA-256 hash
  //
  Sha256Hash (WasmBinary, ModuleMetadata->ModuleSize, ComputedHash);
  if (CompareMem (ComputedHash, ModuleMetadata->Sha256Hash, 32) != 0) {
    FreePool (WasmBinary);
    return FALSE;  // Hash mismatch - security violation
  }

  //
  // Check API version compatibility
  //
  if (ModuleMetadata->ApiVersion > ZOO64_WASM_API_VERSION) {
    FreePool (WasmBinary);
    return FALSE;  // Unsupported API version
  }

  //
  // Load WASM module into runtime
  //
  Status = WasmRuntimeLoad (
             WasmBinary,
             ModuleMetadata->ModuleSize,
             ModuleMetadata->MaxMemoryMB * 1024 * 1024,
             Handle
             );

  if (!Status) {
    FreePool (WasmBinary);
    return FALSE;
  }

  //
  // Call module initialization function
  //
  switch (ModuleMetadata->ModuleType) {
    case Zoo64Wasm32TypeCompression:
      Status = WasmCall (*Handle, "zoo64_compress_init", NULL, 0);
      break;

    case Zoo64Wasm32TypeEncryption:
      Status = WasmCall (*Handle, "zoo64_encrypt_init", NULL, 0);
      break;

    case Zoo64Wasm32TypeHashing:
      Status = WasmCall (*Handle, "zoo64_hash_init", NULL, 0);
      break;

    case Zoo64Wasm32TypeSigning:
      Status = WasmCall (*Handle, "zoo64_sign_init", NULL, 0);
      break;

    default:
      FreePool (WasmBinary);
      WasmRuntimeUnload (*Handle);
      return FALSE;
  }

  FreePool (WasmBinary);

  if (!Status) {
    WasmRuntimeUnload (*Handle);
    return FALSE;
  }

  return TRUE;
}

//
// Call WASM compression function
//
BOOLEAN
CallWasmCompress (
  IN     WASM_MODULE_HANDLE  Handle,
  IN     CONST UINT8         *Input,
  IN     UINTN               InputSize,
  OUT    UINT8               *Output,
  OUT    UINTN               *OutputSize,
  IN     UINT32              Level
  )
{
  WASM_VALUE  Args[5];
  WASM_VALUE  Result;
  INT32       CompressedSize;
  UINT8       *WasmInput;
  UINT8       *WasmOutput;

  //
  // Allocate memory in WASM linear memory
  //
  WasmInput = WasmMalloc (Handle, (UINT32)InputSize);
  WasmOutput = WasmMalloc (Handle, (UINT32)*OutputSize);

  if (WasmInput == NULL || WasmOutput == NULL) {
    if (WasmInput != NULL) WasmFree (Handle, WasmInput);
    if (WasmOutput != NULL) WasmFree (Handle, WasmOutput);
    return FALSE;
  }

  //
  // Copy input data to WASM memory
  //
  WasmMemWrite (Handle, WasmInput, Input, InputSize);

  //
  // Prepare arguments: zoo64_compress(input, input_size, output, output_max, level)
  //
  Args[0].Type = WASM_TYPE_I32;
  Args[0].Value.I32 = (UINT32)(UINTN)WasmInput;
  Args[1].Type = WASM_TYPE_I32;
  Args[1].Value.I32 = (UINT32)InputSize;
  Args[2].Type = WASM_TYPE_I32;
  Args[2].Value.I32 = (UINT32)(UINTN)WasmOutput;
  Args[3].Type = WASM_TYPE_I32;
  Args[3].Value.I32 = (UINT32)*OutputSize;
  Args[4].Type = WASM_TYPE_I32;
  Args[4].Value.I32 = Level;

  //
  // Call WASM function
  //
  if (!WasmCall (Handle, "zoo64_compress", Args, 5, &Result, 1)) {
    WasmFree (Handle, WasmInput);
    WasmFree (Handle, WasmOutput);
    return FALSE;
  }

  CompressedSize = Result.Value.I32;

  if (CompressedSize < 0) {
    //
    // Compression failed (error code returned)
    //
    WasmFree (Handle, WasmInput);
    WasmFree (Handle, WasmOutput);
    return FALSE;
  }

  //
  // Copy compressed data from WASM memory
  //
  WasmMemRead (Handle, Output, WasmOutput, CompressedSize);
  *OutputSize = CompressedSize;

  //
  // Free WASM memory
  //
  WasmFree (Handle, WasmInput);
  WasmFree (Handle, WasmOutput);

  return TRUE;
}
```

### 12.8.5 Custom Algorithm IDs

When using WASM32 modules, custom algorithm IDs are used:

**Compression Algorithms**:
- `0x0100` - `0x01FF`: Custom compression (via WASM32)
- Specific ID stored in `ZOO64_WASM32_MODULE.AlgorithmId`

**Encryption Methods**:
- `0x0100` - `0x01FF`: Custom encryption (via WASM32)

**Hash Algorithms**:
- `0x0100` - `0x01FF`: Custom hashing (via WASM32)

**Signature Types**:
- `0x0100` - `0x01FF`: Custom signing (via WASM32)

**Module Reference**:
```c
//
// Reference to WASM32 module in algorithm descriptor
//
typedef struct _ZOO64_WASM_ALGORITHM_REF {
  UINT8   ModuleUuid[16];     // UUID of WASM32 module
  UINT32  AlgorithmId;        // Custom algorithm ID (0x0100-0x01FF)
  UINT32  Reserved;
} ZOO64_WASM_ALGORITHM_REF;
```

### 12.8.6 Security Considerations

**Sandboxing**:
- All WASM modules execute in isolated sandbox
- No access to file system outside provided buffers
- No network access
- Memory limits enforced (default 16MB, max 256MB)
- CPU time limits enforced (default 30s per operation)

**Validation**:
- SHA-256 hash verification before loading
- WASM binary validation (magic number, structure)
- API version compatibility check
- Resource limit verification

**Trust Model**:
- **Required modules** (flag `ZOO64_WASM32_REQUIRED`): User must explicitly trust
- **Optional modules**: Can be disabled via policy
- **Verified modules** (flag `ZOO64_WASM32_VERIFIED`): Digitally signed by author
- **Trusted modules** (flag `ZOO64_WASM32_TRUSTED`): From organization-trusted sources

**User Consent**:
```
Warning: This archive contains a custom compression algorithm
implemented as a WASM32 module.

Module: "CorpCompress v2.1"
Author: "ACME Corporation"
SHA-256: 3d4f2bf07dc1be38b20cd6e46949a1071f9fc9c03e...
Type: Compression (required)

The module requests:
- Maximum memory: 64 MB
- Maximum execution time: 60 seconds per operation

Allow this module to execute? [Yes/No/Always/Never for this author]
```

**Denial of Service Protection**:
- Watchdog timer terminates long-running modules
- Memory allocation limits prevent OOM
- Infinite loop detection via instruction counting

### 12.8.7 Example WASM32 Module (C Source)

Complete example of a simple compression module:

```c
//
// example_compress.c - Simple RLE compression WASM32 module for Zoo64
//
// Compile: clang --target=wasm32 -O2 -nostdlib -Wl,--no-entry \
//          -Wl,--export-all -o example_compress.wasm example_compress.c
//

#include <stdint.h>
#include <stddef.h>

//
// Import functions from host
//
__attribute__((import_module("zoo64"), import_name("malloc")))
extern void* zoo64_malloc(uint32_t size);

__attribute__((import_module("zoo64"), import_name("free")))
extern void zoo64_free(void* ptr);

__attribute__((import_module("zoo64"), import_name("log")))
extern void zoo64_log(const char* msg, uint32_t len);

//
// Simple string length (no libc)
//
static uint32_t strlen_simple(const char* s) {
  uint32_t len = 0;
  while (s[len]) len++;
  return len;
}

//
// Simple memory copy (no libc)
//
static void memcpy_simple(void* dst, const void* src, uint32_t n) {
  uint8_t* d = (uint8_t*)dst;
  const uint8_t* s = (const uint8_t*)src;
  for (uint32_t i = 0; i < n; i++) {
    d[i] = s[i];
  }
}

//
// Initialize module
//
__attribute__((export_name("zoo64_compress_init")))
int32_t zoo64_compress_init(void) {
  const char* msg = "Simple RLE compressor initialized";
  zoo64_log(msg, strlen_simple(msg));
  return 0;  // Success
}

//
// Get algorithm information
//
__attribute__((export_name("zoo64_compress_get_info")))
void zoo64_compress_get_info(
  char* name,
  uint32_t name_len,
  uint32_t* flags
) {
  const char* algo_name = "SimpleRLE";
  uint32_t len = strlen_simple(algo_name);
  if (len >= name_len) len = name_len - 1;
  memcpy_simple(name, algo_name, len);
  name[len] = '\0';
  *flags = 0x00000001;  // Fast compression
}

//
// Compress data (simple RLE)
//
__attribute__((export_name("zoo64_compress")))
int32_t zoo64_compress(
  const uint8_t* input,
  uint32_t input_size,
  uint8_t* output,
  uint32_t output_max,
  uint32_t level
) {
  uint32_t in_pos = 0;
  uint32_t out_pos = 0;

  while (in_pos < input_size) {
    uint8_t value = input[in_pos];
    uint32_t run_length = 1;

    //
    // Count run
    //
    while (in_pos + run_length < input_size &&
           input[in_pos + run_length] == value &&
           run_length < 255) {
      run_length++;
    }

    if (run_length >= 3) {
      //
      // Encode run: [255][length][value]
      //
      if (out_pos + 3 > output_max) return -1;  // Buffer too small
      output[out_pos++] = 255;  // RLE marker
      output[out_pos++] = (uint8_t)run_length;
      output[out_pos++] = value;
      in_pos += run_length;
    } else {
      //
      // Literal byte
      //
      if (out_pos + 1 > output_max) return -1;
      output[out_pos++] = value;
      in_pos++;
    }
  }

  return out_pos;  // Return compressed size
}

//
// Decompress data
//
__attribute__((export_name("zoo64_decompress")))
int32_t zoo64_decompress(
  const uint8_t* input,
  uint32_t input_size,
  uint8_t* output,
  uint32_t output_max
) {
  uint32_t in_pos = 0;
  uint32_t out_pos = 0;

  while (in_pos < input_size) {
    if (input[in_pos] == 255) {
      //
      // RLE run
      //
      if (in_pos + 2 >= input_size) return -2;  // Truncated data
      uint32_t run_length = input[in_pos + 1];
      uint8_t value = input[in_pos + 2];

      if (out_pos + run_length > output_max) return -1;  // Buffer too small

      for (uint32_t i = 0; i < run_length; i++) {
        output[out_pos++] = value;
      }

      in_pos += 3;
    } else {
      //
      // Literal byte
      //
      if (out_pos >= output_max) return -1;
      output[out_pos++] = input[in_pos++];
    }
  }

  return out_pos;  // Return decompressed size
}

//
// Cleanup
//
__attribute__((export_name("zoo64_compress_cleanup")))
void zoo64_compress_cleanup(void) {
  const char* msg = "Simple RLE compressor cleanup";
  zoo64_log(msg, strlen_simple(msg));
}
```

### 12.8.8 Conformance Levels

- **Tiny**: No WASM support
- **Compact**: No WASM support
- **Embedded**: No WASM support
- **Minimal**: No WASM support
- **Standard**: No WASM support
- **Enhanced**: WASM32 decompression/decryption only (read-only)
- **Full**: Complete WASM32 support (read and write)

**Implementation Requirements (Full)**:
- WASM32 runtime (e.g., wasm3, wasmer, wasmtime)
- Sandboxing support
- Resource limit enforcement
- Module validation
- Hash verification

### 12.8.9 Best Practices

**For Archive Creators**:
1. Only embed WASM modules when necessary (proprietary/custom algorithms)
2. Keep modules small (< 1MB recommended)
3. Set appropriate resource limits
4. Include fallback to standard algorithms when possible
5. Document algorithm in module metadata
6. Sign modules for verification
7. Test modules thoroughly before embedding

**For Archive Consumers**:
1. Verify module hashes before loading
2. Enforce resource limits strictly
3. Provide user consent UI for required modules
4. Allow disabling optional modules via policy
5. Maintain whitelist/blacklist of module authors
6. Log all WASM module executions
7. Consider using read-only mode for untrusted archives

**Security Checklist**:
- [ ] SHA-256 hash verified
- [ ] API version compatible
- [ ] Resource limits set appropriately
- [ ] User consent obtained (for required modules)
- [ ] Module runs in sandbox
- [ ] Watchdog timer enabled
- [ ] Memory limits enforced
- [ ] Execution logged

## 12.9 Delta Computation and Deduplication Pipeline

This section documents the delta computation and block deduplication algorithms used in Zoo64, which operate **above** (before) the standard compression/encryption layers. Hashing for integrity verification always occurs on the **full reconstructed data** after delta application and deduplication.

### 12.9.1 Processing Pipeline Order

Zoo64 applies data transformations in the following order:

```
Original File
    ↓
[1] Delta Computation (if storing revision)
    ↓
[2] Block Deduplication (if enabled)
    ↓
[3] Compression (ZSTD, LZMA2, ZOZ, etc.)
    ↓
[4] Encryption (AES-256-GCM, ChaCha20, etc.)
    ↓
[5] Archive Storage
```

**Extraction reverses the order**:

```
Archive Storage
    ↓
[1] Decryption
    ↓
[2] Decompression
    ↓
[3] Deduplication Reconstruction (chunk reassembly)
    ↓
[4] Delta Application (if deltified)
    ↓
[5] Hash Verification (on FULL reconstructed data)
    ↓
Final File
```

### 12.9.2 Delta Computation Algorithm

Delta encoding stores files as differences from a base revision, dramatically reducing storage for versioned content.

#### 12.9.2.1 Delta Algorithm Selection

Zoo64 supports multiple delta algorithms, each optimized for different scenarios:

```c
typedef enum _ZOO64_DELTA_ALGORITHM {
  Zoo64DeltaNone           = 0,  // Not deltified
  Zoo64DeltaXdelta3        = 1,  // xdelta3 (general purpose, fast)
  Zoo64DeltaVcdiff         = 2,  // RFC 3284 VCDIFF (standard)
  Zoo64DeltaBsdiff         = 3,  // bsdiff (executables, binary)
  Zoo64DeltaZdelta         = 4,  // zdelta (large files)
  Zoo64DeltaRsync          = 5,  // rsync rolling checksum
  Zoo64DeltaGit            = 6,  // Git-style delta
  Zoo64DeltaFossil         = 7   // Fossil delta compression
} ZOO64_DELTA_ALGORITHM;
```

**Algorithm Characteristics**:

| Algorithm | Best For | Speed | Ratio | Memory |
|-----------|----------|-------|-------|--------|
| XDELTA3 | General purpose, text, source code | Fast | Good | Low |
| VCDIFF | Standard compliance, interop | Medium | Good | Low |
| BSDIFF | Executables, binaries | Slow | Excellent | High |
| ZDELTA | Large files (>100 MB) | Fast | Good | Medium |
| RSYNC | Network sync, incremental backup | Fast | Medium | Low |
| GIT_DELTA | Source repositories, text | Fast | Good | Low |
| FOSSIL_DELTA | Simple, embeddable | Very Fast | Medium | Very Low |

#### 12.9.2.2 Delta Computation Process

```c
//
// Delta computation function
//
BOOLEAN
Zoo64ComputeDelta (
  IN     CONST UINT8              *BaseData,
  IN     UINTN                    BaseSize,
  IN     CONST UINT8              *TargetData,
  IN     UINTN                    TargetSize,
  IN     ZOO64_DELTA_ALGORITHM    Algorithm,
  OUT    UINT8                    **DeltaData,
  OUT    UINTN                    *DeltaSize,
  OUT    UINT8                    *BaseHash,      // SHA-256 of base
  OUT    UINT8                    *TargetHash     // SHA-256 of target
  )
{
  UINTN   Index;
  VOID    *Context;

  //
  // Step 1: Hash the base data (full data)
  //
  Sha256Hash (BaseData, BaseSize, BaseHash);

  //
  // Step 2: Hash the target data (full data)
  //
  Sha256Hash (TargetData, TargetSize, TargetHash);

  //
  // Step 3: Compute delta based on algorithm
  //
  switch (Algorithm) {
    case Zoo64DeltaXdelta3:
      return Xdelta3Encode (BaseData, BaseSize, TargetData, TargetSize,
                            DeltaData, DeltaSize);

    case Zoo64DeltaBsdiff:
      return BsdiffEncode (BaseData, BaseSize, TargetData, TargetSize,
                           DeltaData, DeltaSize);

    case Zoo64DeltaGit:
      return GitDeltaEncode (BaseData, BaseSize, TargetData, TargetSize,
                             DeltaData, DeltaSize);

    // ... other algorithms
  }

  return FALSE;
}
```

**Key Points**:
1. **Hashing occurs on FULL data** (not delta)
2. Both base and target are hashed for verification
3. Delta is computed from complete files
4. Compression is applied AFTER delta computation

#### 12.9.2.3 Delta Storage Strategies

Zoo64 supports multiple delta storage strategies:

**1. Forward Delta Chain (Git-style)**:
```
V1 (full) ← V2 (delta) ← V3 (delta) ← V4 (delta)
         ↖             ↖             ↖
          5 KB          3 KB          4 KB

Storage: V1=100KB + V2=5KB + V3=3KB + V4=4KB = 112 KB
Extract V4: Read V1 → Apply V2 → Apply V3 → Apply V4
```

**2. Reverse Delta (Fossil-style)**:
```
V1 (delta) → V2 (delta) → V3 (delta) → V4 (full)
         ↘             ↘             ↘
          8 KB          6 KB          7 KB

Storage: V1=8KB + V2=6KB + V3=7KB + V4=102KB = 123 KB
Extract V4: Read V4 directly (fastest for latest version)
Extract V1: Read V4 → Reverse-apply V3 → V2 → V1
```

**3. Snapshot + Deltas (Hybrid)**:
```
V1 (full) ← V2 (delta) ← V3 (delta) → V4 (full) ← V5 (delta)
         ↖             ↖                        ↖
          5 KB          3 KB                     4 KB

Periodic snapshots every N versions to limit chain depth
Max extraction overhead: N-1 delta applications
```

**4. Tree-Based Delta**:
```
        V4 (full)
       ↗  ↑  ↖
    V1    V2    V3
   (Δ)   (Δ)   (Δ)

Deltas computed from nearest full version
Limits chain depth to 1
Higher storage but faster extraction
```

#### 12.9.2.4 Delta Application Algorithm

```c
//
// Apply delta to reconstruct target file
//
BOOLEAN
Zoo64ApplyDelta (
  IN     CONST UINT8              *BaseData,
  IN     UINTN                    BaseSize,
  IN     CONST UINT8              *DeltaData,
  IN     UINTN                    DeltaSize,
  IN     ZOO64_DELTA_ALGORITHM    Algorithm,
  IN     CONST UINT8              *BaseHash,      // Expected base hash
  IN     CONST UINT8              *TargetHash,    // Expected target hash
  OUT    UINT8                    **TargetData,
  OUT    UINTN                    *TargetSize
  )
{
  UINT8   ComputedHash[32];

  //
  // Step 1: Verify base data hash
  //
  Sha256Hash (BaseData, BaseSize, ComputedHash);
  if (CompareMem (ComputedHash, BaseHash, 32) != 0) {
    return FALSE;  // Base data corrupted
  }

  //
  // Step 2: Apply delta algorithm
  //
  switch (Algorithm) {
    case Zoo64DeltaXdelta3:
      if (!Xdelta3Decode (BaseData, BaseSize, DeltaData, DeltaSize,
                          TargetData, TargetSize)) {
        return FALSE;
      }
      break;

    case Zoo64DeltaBsdiff:
      if (!BspatchDecode (BaseData, BaseSize, DeltaData, DeltaSize,
                          TargetData, TargetSize)) {
        return FALSE;
      }
      break;

    // ... other algorithms
  }

  //
  // Step 3: Verify reconstructed target hash (FULL DATA)
  //
  Sha256Hash (*TargetData, *TargetSize, ComputedHash);
  if (CompareMem (ComputedHash, TargetHash, 32) != 0) {
    FreePool (*TargetData);
    return FALSE;  // Reconstructed data corrupted
  }

  return TRUE;
}
```

**Verification Flow**:
1. Hash base data and verify against stored hash
2. Apply delta transformation
3. Hash **complete reconstructed data** and verify
4. If any verification fails, abort extraction

### 12.9.3 Block Deduplication Algorithm

Block deduplication eliminates duplicate chunks across files using content-defined chunking (CDC) and cryptographic hashing.

#### 12.9.3.1 Content-Defined Chunking (CDC)

CDC creates variable-size chunks based on content, making deduplication resilient to insertions/deletions.

**Rabin Fingerprinting Algorithm**:

```c
//
// Rabin fingerprint chunker
//
BOOLEAN
RabinChunkData (
  IN     CONST UINT8   *Data,
  IN     UINTN         DataSize,
  IN     UINT32        MinChunkSize,    // e.g., 4 KB
  IN     UINT32        AvgChunkSize,    // e.g., 64 KB
  IN     UINT32        MaxChunkSize,    // e.g., 256 KB
  OUT    CHUNK_LIST    **Chunks
  )
{
  UINT64   RabinHash;
  UINT32   WindowSize;
  UINTN    Offset;
  UINTN    ChunkStart;
  UINT32   Mask;

  WindowSize = 48;  // Rolling window size
  Mask = AvgChunkSize - 1;  // For chunk boundary detection
  RabinHash = 0;
  ChunkStart = 0;

  //
  // Initialize Rabin polynomial rolling hash
  //
  for (Offset = 0; Offset < WindowSize && Offset < DataSize; Offset++) {
    RabinHash = RabinUpdate (RabinHash, Data[Offset]);
  }

  //
  // Slide window and detect chunk boundaries
  //
  for (Offset = WindowSize; Offset < DataSize; Offset++) {
    //
    // Update rolling hash (remove old byte, add new byte)
    //
    RabinHash = RabinRoll (RabinHash, Data[Offset - WindowSize], Data[Offset]);

    //
    // Check for chunk boundary
    // Boundary when: (hash & mask) == 0 AND size >= MinChunkSize
    //
    if ((Offset - ChunkStart) >= MinChunkSize &&
        ((RabinHash & Mask) == 0 || (Offset - ChunkStart) >= MaxChunkSize)) {
      //
      // Found chunk boundary
      //
      AddChunk (Chunks, &Data[ChunkStart], Offset - ChunkStart, ChunkStart);
      ChunkStart = Offset;
    }
  }

  //
  // Add final chunk
  //
  if (ChunkStart < DataSize) {
    AddChunk (Chunks, &Data[ChunkStart], DataSize - ChunkStart, ChunkStart);
  }

  return TRUE;
}
```

**FastCDC Optimization**:

FastCDC improves CDC performance using gear hash and normalized chunking:

```c
//
// FastCDC: Fast Content-Defined Chunking
//
BOOLEAN
FastCdcChunkData (
  IN     CONST UINT8   *Data,
  IN     UINTN         DataSize,
  IN     UINT32        AvgChunkSize,
  OUT    CHUNK_LIST    **Chunks
  )
{
  UINT64   GearHash;
  UINT32   MinSize, MaxSize;
  UINT32   MaskS, MaskL;
  UINTN    Offset;
  UINTN    ChunkStart;

  //
  // FastCDC normalization: avoid small/large chunks
  //
  MinSize = AvgChunkSize / 4;
  MaxSize = AvgChunkSize * 4;
  MaskS = (AvgChunkSize / 2) - 1;  // Small chunk mask
  MaskL = AvgChunkSize * 2 - 1;    // Large chunk mask

  GearHash = 0;
  ChunkStart = 0;

  for (Offset = 0; Offset < DataSize; Offset++) {
    //
    // Update gear hash (simple but effective)
    //
    GearHash = (GearHash << 1) + GearTable[Data[Offset]];

    UINT32 ChunkSize = Offset - ChunkStart;

    //
    // Three-tier boundary detection for normalized chunking
    //
    if (ChunkSize >= MinSize) {
      if (ChunkSize < AvgChunkSize && (GearHash & MaskS) == 0) {
        // Small chunk boundary
        goto CreateChunk;
      } else if (ChunkSize < MaxSize && (GearHash & MaskL) == 0) {
        // Normal chunk boundary
        goto CreateChunk;
      } else if (ChunkSize >= MaxSize) {
        // Force chunk at max size
        goto CreateChunk;
      }
    }
    continue;

CreateChunk:
    AddChunk (Chunks, &Data[ChunkStart], Offset - ChunkStart, ChunkStart);
    ChunkStart = Offset;
    GearHash = 0;
  }

  //
  // Final chunk
  //
  if (ChunkStart < DataSize) {
    AddChunk (Chunks, &Data[ChunkStart], DataSize - ChunkStart, ChunkStart);
  }

  return TRUE;
}
```

#### 12.9.3.2 Deduplication Process

```c
//
// Complete deduplication pipeline
//
BOOLEAN
Zoo64DeduplicateFile (
  IN     CONST UINT8       *FileData,
  IN     UINTN             FileSize,
  IN     DEDUP_ALGORITHM   ChunkAlgo,
  IN     HASH_ALGORITHM    HashAlgo,
  IN OUT DEDUP_STORE       *GlobalStore,
  OUT    DEDUP_METADATA    **Metadata
  )
{
  CHUNK_LIST    *Chunks;
  UINTN         ChunkIndex;
  UINT8         ChunkHash[32];
  DEDUP_CHUNK   *Chunk;
  UINT64        PhysicalSize;

  //
  // Step 1: Chunk the file using CDC
  //
  switch (ChunkAlgo) {
    case Zoo64DedupFastCdc:
      FastCdcChunkData (FileData, FileSize, 65536, &Chunks);
      break;
    case Zoo64DedupRabinFingerprint:
      RabinChunkData (FileData, FileSize, 4096, 65536, 262144, &Chunks);
      break;
    // ... other algorithms
  }

  //
  // Step 2: Hash and deduplicate each chunk
  //
  PhysicalSize = 0;
  *Metadata = AllocateZeroPool (sizeof(DEDUP_METADATA));
  (*Metadata)->ChunkCount = Chunks->Count;
  (*Metadata)->Chunks = AllocateZeroPool (Chunks->Count * sizeof(DEDUP_CHUNK));

  for (ChunkIndex = 0; ChunkIndex < Chunks->Count; ChunkIndex++) {
    Chunk = &Chunks->Items[ChunkIndex];

    //
    // Step 2a: Hash the chunk (SHA-256 or BLAKE2b)
    //
    switch (HashAlgo) {
      case Zoo64HashSha256:
        Sha256Hash (Chunk->Data, Chunk->Length, ChunkHash);
        break;
      case Zoo64HashBlake2b:
        Blake2bHash (Chunk->Data, Chunk->Length, ChunkHash);
        break;
      case Zoo64HashBlake3:
        Blake3Hash (Chunk->Data, Chunk->Length, ChunkHash);
        break;
    }

    //
    // Step 2b: Check if chunk exists in global store
    //
    DEDUP_ENTRY *Existing = DedupStoreLookup (GlobalStore, ChunkHash);

    if (Existing != NULL) {
      //
      // Chunk already exists - reference it
      //
      (*Metadata)->Chunks[ChunkIndex].Hash = ChunkHash;
      (*Metadata)->Chunks[ChunkIndex].Offset = Chunk->Offset;
      (*Metadata)->Chunks[ChunkIndex].Length = Chunk->Length;
      (*Metadata)->Chunks[ChunkIndex].RefCount = 0;  // Reference to existing
      (*Metadata)->Chunks[ChunkIndex].StorageOffset = Existing->StorageOffset;

      Existing->RefCount++;  // Increment reference count
    } else {
      //
      // New unique chunk - store it
      //
      UINT64 StorageOffset = DedupStoreAdd (GlobalStore, ChunkHash,
                                             Chunk->Data, Chunk->Length);

      (*Metadata)->Chunks[ChunkIndex].Hash = ChunkHash;
      (*Metadata)->Chunks[ChunkIndex].Offset = Chunk->Offset;
      (*Metadata)->Chunks[ChunkIndex].Length = Chunk->Length;
      (*Metadata)->Chunks[ChunkIndex].RefCount = 1;  // Unique chunk
      (*Metadata)->Chunks[ChunkIndex].StorageOffset = StorageOffset;

      PhysicalSize += Chunk->Length;
    }
  }

  (*Metadata)->LogicalSize = FileSize;
  (*Metadata)->PhysicalSize = PhysicalSize;

  return TRUE;
}
```

#### 12.9.3.3 Deduplication with Compression

**Important**: Compression is applied **AFTER** chunking but **BEFORE** storage:

```
File (1 GB)
    ↓
[1] Chunk with FastCDC → 16,384 chunks @ 64 KB avg
    ↓
[2] Deduplicate chunks → 2,048 unique chunks (87.5% dedup)
    ↓
[3] Compress each unique chunk with ZSTD
    ↓
    2,048 chunks × ~50% compression = 64 MB compressed
    ↓
[4] Store compressed chunks
```

**Per-Chunk Compression**:

```c
//
// Compress deduplicated chunks
//
BOOLEAN
CompressDedupChunks (
  IN     DEDUP_METADATA    *Metadata,
  IN     COMPRESS_ALGO     Algorithm
  )
{
  UINTN   ChunkIndex;

  for (ChunkIndex = 0; ChunkIndex < Metadata->ChunkCount; ChunkIndex++) {
    DEDUP_CHUNK *Chunk = &Metadata->Chunks[ChunkIndex];

    //
    // Only compress unique chunks (RefCount > 0)
    //
    if (Chunk->RefCount > 0) {
      UINT8  *Compressed;
      UINTN  CompressedSize;

      //
      // Compress chunk with ZSTD, LZMA2, or other algorithm
      //
      ZstdCompress (Chunk->Data, Chunk->Length, &Compressed, &CompressedSize);

      //
      // Replace chunk data with compressed version
      //
      FreePool (Chunk->Data);
      Chunk->Data = Compressed;
      Chunk->CompressedLength = CompressedSize;
    }
  }

  return TRUE;
}
```

#### 12.9.3.4 Hash Verification with Deduplication

**Critical**: Hashing for integrity verification occurs on **FULL reconstructed data**:

```c
//
// Extract and verify deduplicated file
//
BOOLEAN
Zoo64ExtractDedupFile (
  IN     DEDUP_METADATA    *Metadata,
  IN     DEDUP_STORE       *GlobalStore,
  IN     CONST UINT8       *ExpectedFileHash,  // SHA-256 of FULL file
  OUT    UINT8             **FileData,
  OUT    UINTN             *FileSize
  )
{
  UINT8    *ReconstructedFile;
  UINTN    ChunkIndex;
  UINTN    FileOffset;
  UINT8    ComputedHash[32];

  //
  // Step 1: Allocate buffer for full file
  //
  ReconstructedFile = AllocatePool (Metadata->LogicalSize);
  FileOffset = 0;

  //
  // Step 2: Reconstruct file from chunks
  //
  for (ChunkIndex = 0; ChunkIndex < Metadata->ChunkCount; ChunkIndex++) {
    DEDUP_CHUNK *Chunk = &Metadata->Chunks[ChunkIndex];
    UINT8       *ChunkData;
    UINTN       ChunkSize;

    //
    // Step 2a: Retrieve chunk from global store
    //
    DedupStoreRetrieve (GlobalStore, Chunk->Hash, &ChunkData, &ChunkSize);

    //
    // Step 2b: Decompress chunk if compressed
    //
    if (Chunk->CompressedLength > 0) {
      UINT8  *Decompressed;
      UINTN  DecompressedSize;

      ZstdDecompress (ChunkData, Chunk->CompressedLength,
                      &Decompressed, &DecompressedSize);

      FreePool (ChunkData);
      ChunkData = Decompressed;
      ChunkSize = DecompressedSize;
    }

    //
    // Step 2c: Copy chunk to file buffer
    //
    CopyMem (&ReconstructedFile[FileOffset], ChunkData, ChunkSize);
    FileOffset += ChunkSize;

    FreePool (ChunkData);
  }

  //
  // Step 3: Hash the COMPLETE reconstructed file
  //
  Sha256Hash (ReconstructedFile, Metadata->LogicalSize, ComputedHash);

  //
  // Step 4: Verify hash against expected value
  //
  if (CompareMem (ComputedHash, ExpectedFileHash, 32) != 0) {
    FreePool (ReconstructedFile);
    return FALSE;  // File corrupted or dedup error
  }

  *FileData = ReconstructedFile;
  *FileSize = Metadata->LogicalSize;

  return TRUE;
}
```

### 12.9.4 Combined Delta + Deduplication

Files can use both delta encoding AND deduplication:

```
Version 2 of large file:
    ↓
[1] Compute delta from Version 1 → 5 MB delta
    ↓
[2] Chunk the delta (5 MB) with FastCDC → 80 chunks
    ↓
[3] Deduplicate chunks → 60 unique chunks (25% dedup)
    ↓
[4] Compress unique chunks with ZSTD → 2 MB compressed
    ↓
[5] Store compressed deduplicated delta
```

**Storage efficiency**:
- Original V2 size: 100 MB
- Delta size: 5 MB (95% reduction)
- After dedup: 3.75 MB (25% additional reduction)
- After compression: 2 MB (50% additional reduction)
- **Total**: 98% reduction from original

### 12.9.5 Hashing Policy

**Zoo64 Hashing Rules**:

1. **File-level hashes** (CRC32, SHA-256, BLAKE3):
   - Always computed on **FULL reconstructed data**
   - Computed AFTER delta application
   - Computed AFTER deduplication reconstruction
   - Computed BEFORE compression
   - Stored in file entry header

2. **Chunk-level hashes** (for deduplication):
   - Computed on individual chunks
   - Used for dedup matching only
   - NOT used for file integrity verification

3. **Base/Target hashes** (for delta):
   - Computed on complete base file
   - Computed on complete target file
   - Stored in delta metadata
   - Used to verify delta application correctness

**Example Hash Flow**:

```
Archive Creation:
  Original File (100 MB)
      ↓
  Compute file hash: SHA-256(full 100 MB) → Store in file entry
      ↓
  Compute delta from base → 5 MB delta
      ↓
  Hash base: SHA-256(base 100 MB) → Store in delta metadata
  Hash target: SHA-256(target 100 MB) → Store in delta metadata
      ↓
  Chunk delta (5 MB) → 80 chunks
      ↓
  Hash each chunk: SHA-256(chunk) → For dedup lookup
      ↓
  Deduplicate → 60 unique chunks
      ↓
  Compress chunks with ZSTD
      ↓
  Store in archive

Extraction:
  Retrieve compressed chunks
      ↓
  Decompress chunks
      ↓
  Reconstruct delta (5 MB) from chunks
      ↓
  Retrieve base file (100 MB)
      ↓
  Verify base hash: SHA-256(base) vs stored
      ↓
  Apply delta to base → Target (100 MB)
      ↓
  Verify target hash: SHA-256(target) vs stored
      ↓
  Verify file hash: SHA-256(full file) vs file entry
      ↓
  Output file if all hashes match
```

### 12.9.6 Performance Characteristics

**Deduplication Performance**:

| Chunking Algorithm | Speed | Dedup Ratio | Memory |
|-------------------|-------|-------------|---------|
| Fixed Size | Very Fast | Poor | Very Low |
| Rabin Fingerprint | Medium | Good | Low |
| FastCDC | Fast | Excellent | Low |
| Gear Hash | Fast | Good | Low |
| Super Feature | Slow | Excellent | Medium |

**Delta Performance**:

| Delta Algorithm | Compression | Decompression | Memory | Best For |
|----------------|-------------|---------------|---------|----------|
| XDELTA3 | Fast | Very Fast | 16 MB | General |
| BSDIFF | Very Slow | Fast | 256 MB | Executables |
| GIT_DELTA | Very Fast | Very Fast | 8 MB | Source code |
| FOSSIL_DELTA | Very Fast | Very Fast | 1 MB | Embedded |

**Typical Dedup Ratios** (with FastCDC @ 64 KB avg chunk):
```
VM backups (daily):     70-90% dedup
Source repositories:    40-60% dedup
Document archives:      50-70% dedup
Database backups:       20-40% dedup
Media files (JPEG/MP4): 5-10% dedup
```

**Delta Compression Ratios**:
```
Source code (daily):    95-99% delta reduction
Documents (versions):   85-95% delta reduction
Executables (patches):  80-90% delta reduction
VM snapshots (daily):   90-95% delta reduction
```

### 12.9.7 Conformance Levels

**Delta Support**:
- **Minimal**: Optional (not required)
- **Standard**: Optional (recommended for backups)
- **Enhanced**: Required (at least XDELTA3 or VCDIFF)
- **Full**: Required (all delta algorithms)

**Deduplication Support**:
- **Minimal**: Not required
- **Standard**: Optional (recommended)
- **Enhanced**: Required (FastCDC or Rabin)
- **Full**: Required (all chunking algorithms)

## 13. COM Component Interface

Zoo64 is designed as a COM component for cross-language interoperability.

### 13.1 IZoo64Archive Interface

```cpp
interface IZoo64Archive : IUnknown {
  /// Archive operations
  HRESULT Open([in] BSTR path, [in] DWORD mode);
  HRESULT Create([in] BSTR path, [in] ZOO64_ARCHIVE_OPTIONS* options);
  HRESULT Close();

  /// File operations
  HRESULT AddFile([in] BSTR sourcePath, [in] BSTR archivePath,
                  [in] ZOO64_FILE_OPTIONS* options);
  HRESULT ExtractFile([in] BSTR archivePath, [in] BSTR destPath);
  HRESULT RemoveFile([in] BSTR archivePath);

  /// Enumeration
  HRESULT GetFileCount([out] DWORD* count);
  HRESULT GetFileInfo([in] DWORD index, [out] ZOO64_FILE_INFO* info);

  /// Compression
  HRESULT SetCompressionMode([in] DWORD mode);
  HRESULT SetWindowSize([in] DWORD size);
  HRESULT SetBlockSize([in] DWORD size);

  /// Signatures
  HRESULT SignArchive([in] IZoo64SigningKey* key);
  HRESULT VerifyArchive([out] BOOL* valid);
  HRESULT SignFile([in] BSTR archivePath, [in] IZoo64SigningKey* key);
  HRESULT VerifyFile([in] BSTR archivePath, [out] BOOL* valid);

  /// Metadata
  HRESULT GetFileMetadata([in] BSTR archivePath,
                          [out] IZoo64Metadata** metadata);
  HRESULT SetFileMetadata([in] BSTR archivePath,
                          [in] IZoo64Metadata* metadata);

  /// YAML Metadata
  HRESULT GetArchiveYaml([out] BSTR* yaml);
  HRESULT SetArchiveYaml([in] BSTR yaml);
  HRESULT GetFileYaml([in] BSTR archivePath, [out] BSTR* yaml);
  HRESULT SetFileYaml([in] BSTR archivePath, [in] BSTR yaml);

  /// Encryption
  HRESULT SetPassword([in] BSTR password);
  HRESULT SetKeyFile([in] BSTR keyFilePath);
  HRESULT SetEncryptionMethod([in] WORD method, [in] WORD kdf);
  HRESULT EncryptArchive([in] BOOL headerEncryption);
  HRESULT EncryptFile([in] BSTR archivePath);
  HRESULT DecryptFile([in] BSTR archivePath, [in] BSTR password);
  HRESULT VerifyPassword([out] BOOL* valid);

  /// Multi-Volume
  HRESULT SetVolumeSize([in] UINT64 sizeBytes);
  HRESULT GetVolumeCount([out] DWORD* count);
  HRESULT GetVolumeInfo([in] DWORD volumeNumber, [out] ZOO64_VOLUME_INFO* info);
  HRESULT MergeVolumes([in] BSTR outputPath);

  /// Classic Zoo Compatibility
  HRESULT ConvertFromClassicZoo([in] BSTR classicZooPath, [in] BSTR outputPath);
  HRESULT ConvertToClassicZoo([in] BSTR outputPath, [in] BOOL dualFormat);
  HRESULT IsClassicZoo([in] BSTR path, [out] BOOL* isClassic);
};
```

### 13.2 IZoo64Metadata Interface

```cpp
interface IZoo64Metadata : IUnknown {
  /// ACLs
  HRESULT GetACL([out] IZoo64ACL** acl);
  HRESULT SetACL([in] IZoo64ACL* acl);

  /// Extended Attributes
  HRESULT GetXattr([in] BSTR name, [out] VARIANT* value);
  HRESULT SetXattr([in] BSTR name, [in] VARIANT value);
  HRESULT EnumXattrs([out] IEnumVARIANT** enumerator);

  /// Alternate Data Streams
  HRESULT GetADS([in] BSTR streamName, [out] IStream** stream);
  HRESULT SetADS([in] BSTR streamName, [in] IStream* stream);
  HRESULT EnumADS([out] IEnumVARIANT** enumerator);

  /// Timestamps
  HRESULT GetTimestamps([out] ZOO64_TIMESTAMPS* timestamps);
  HRESULT SetTimestamps([in] ZOO64_TIMESTAMPS* timestamps);

  /// Permissions
  HRESULT GetPermissions([out] DWORD* mode);
  HRESULT SetPermissions([in] DWORD mode);

  /// YAML Metadata
  HRESULT GetYaml([out] BSTR* yaml);
  HRESULT SetYaml([in] BSTR yaml);

  /// Platform-specific
  HRESULT GetMacOSUUIDs([out] ZOO64_MACOS_UUID* uuids);
  HRESULT SetMacOSUUIDs([in] ZOO64_MACOS_UUID* uuids);
  HRESULT GetBSDFlags([out] ZOO64_BSD_FLAGS* flags);
  HRESULT SetBSDFlags([in] ZOO64_BSD_FLAGS* flags);
  HRESULT GetLinuxFlags([out] ZOO64_LINUX_FLAGS* flags);
  HRESULT SetLinuxFlags([in] ZOO64_LINUX_FLAGS* flags);
  HRESULT GetWindowsAttributes([out] ZOO64_WINDOWS_ATTR* attr);
  HRESULT SetWindowsAttributes([in] ZOO64_WINDOWS_ATTR* attr);

  /// Links
  HRESULT GetHardLinkTarget([out] BSTR* targetPath);
  HRESULT SetHardLinkTarget([in] BSTR targetPath, [in] UINT64 inodeNumber);
  HRESULT GetSymbolicLinkTarget([out] BSTR* targetPath, [out] DWORD* flags);
  HRESULT SetSymbolicLinkTarget([in] BSTR targetPath, [in] DWORD flags);
};
```

## 14. Platform-Specific Considerations

### 14.1 Unix/Linux

- Store full POSIX permissions in `Mode` field
- Store UID/GID
- Preserve xattrs (user, system, security, trusted namespaces)
- SELinux contexts in metadata
- File capabilities

### 14.2 Windows

- Convert NTFS permissions to ACL metadata
- Store security descriptors
- Preserve alternate data streams
- Store file attributes (hidden, system, archive, etc.)
- Store reparse point information for symlinks

### 14.3 macOS

- Store resource forks in ADS metadata
- Preserve xattrs (com.apple.* namespace)
- Store Finder info
- HFS+ compression flags

### 14.4 When to Use VCS Metadata

Version Control System (VCS) metadata should be stored selectively based on the archiving scenario. This section provides guidance on when to include Git, SVN, Perforce, and other VCS metadata.

#### 14.4.1 Scenarios Requiring VCS Metadata

**1. Source Code Archiving**
```
Use Case: Archiving source code repositories for long-term preservation
Metadata Types: ZOO64_META_GIT (0x0032), ZOO64_META_SVN (0x0034), ZOO64_META_MERCURIAL (0x0037)

When to Include:
✓ Creating compliance archives (GPL/LGPL requirement to provide source)
✓ Historical preservation of software projects
✓ Legal discovery/eDiscovery requirements
✓ Backup of development repositories
✓ Migration between VCS systems

What to Store:
- Git: Commit hashes, ref names, object hashes, branch/tag info, worktree state
- SVN: Revision numbers, property lists, externals definitions
- Perforce: Changelist numbers, file revisions, labels
- Mercurial: Node IDs, parent hashes, bookmarks, tags
```

**2. Build Artifact Tracing**
```
Use Case: Tracking which commit built which artifact
Metadata Types: ZOO64_META_GIT (0x0032)

When to Include:
✓ Binary distribution packages (linking binaries to source)
✓ Release archives (tracking exact source code used)
✓ Docker/container images (reproducible builds)
✓ Firmware updates (traceability for safety/security)

What to Store:
- Commit hash that produced the build
- Branch name and tag (if any)
- Dirty state indicator (uncommitted changes)
- Submodule commit hashes
```

**3. Continuous Integration/Deployment**
```
Use Case: CI/CD pipeline artifacts
Metadata Types: ZOO64_META_GIT (0x0032), ZOO64_META_SVN (0x0034)

When to Include:
✓ Build artifacts from CI systems
✓ Test result archives
✓ Deployment packages
✓ Release candidates

What to Store:
- Exact commit that triggered the build
- Build number and timestamp
- Branch and tag information
- CI job ID for traceability
```

**4. Migration and Conversion**
```
Use Case: Moving between different VCS or preserving history during migration
Metadata Types: All VCS types (0x0032-0x0037, 0x003B)

When to Include:
✓ SVN → Git migration
✓ Perforce → Git migration
✓ CVS/RCS → Modern VCS migration
✓ Mercurial ↔ Git conversion
✓ Fossil → Git migration

What to Store:
- Original revision identifiers
- Author mapping information
- Branch/tag correspondence
- Merge parent relationships
- Original timestamps
```

#### 14.4.2 Scenarios NOT Requiring VCS Metadata

**1. User Data Backups**
```
❌ Home directory backups
❌ Document archives
❌ Photo/media libraries
❌ General file backups

Reason: User files are not under version control; VCS metadata wastes space
```

**2. System Backups**
```
❌ Operating system images
❌ Configuration backups (unless in /etc/.git)
❌ Database dumps
❌ Log archives

Reason: System files don't have VCS context; use standard metadata instead
```

**3. Binary-Only Distribution**
```
❌ End-user software installers
❌ Proprietary binary libraries (no source available)
❌ Closed-source applications
❌ Precompiled packages without source

Reason: VCS metadata only useful with corresponding source code
Exception: May include commit hash for traceability if binary was built from VCS
```

**4. Temporary/Working Archives**
```
❌ Compressed transfers between systems
❌ Temporary build directories
❌ Cache archives
❌ Incremental backups of non-VCS content

Reason: Short-lived archives don't benefit from VCS context
```

#### 14.4.3 Conditional VCS Metadata Usage

**Git Working Tree Archives**
```
Archive Type: Development snapshot
Include VCS Metadata IF:
  - Archive is for disaster recovery
  - Need to resume work elsewhere
  - Preserving work-in-progress state
  - Debugging build issues requiring exact state

Store:
  - Current branch and commit
  - Stash entries (if any)
  - Untracked file list
  - Submodule states
  - Worktree configuration

Omit VCS Metadata IF:
  - Just distributing source code
  - Archive is temporary
  - Workspace is clean (no local changes)
```

**Git LFS Handling**
```
Archive Type: Repository with Large File Storage
Include Git Metadata: YES
Include LFS Metadata: CONDITIONAL

Store LFS Metadata IF:
  - Archiving for offline access (include actual LFS objects)
  - Migration to different Git hosting (preserve LFS pointers)
  - Compliance requires all data (include full LFS content)

Store Only Git Metadata IF:
  - LFS objects available on server
  - Archiving just working tree
  - Bandwidth/storage constrained
```

#### 14.4.4 Metadata Storage Recommendations

**Minimal (Source Distribution)**
```c
//
// Store only commit hash for traceability
//
typedef struct _ZOO64_GIT_MINIMAL {
  UINT8   CommitHash[20];  // SHA-1 of commit
  UINT8   Dirty;           // Non-zero if workspace had changes
} ZOO64_GIT_MINIMAL;

Size: 21 bytes per file
Use: Binary releases, build artifacts
```

**Standard (Repository Backup)**
```c
//
// Store commit, branch, and refs
//
typedef struct _ZOO64_GIT_STANDARD {
  UINT8   CommitHash[20];    // SHA-1 of commit
  UINT8   TreeHash[20];      // Tree object hash
  UINT8   BranchName[256];   // Current branch
  UINT8   Tag[256];          // Tag name (if any)
  UINT32  Flags;             // Dirty, detached, etc.
} ZOO64_GIT_STANDARD;

Size: ~552 bytes per file
Use: Source archives, CI/CD artifacts
```

**Complete (Full Repository Preservation)**
```c
//
// Store all Git metadata (see section 6.34)
//
typedef struct _ZOO64_GIT_COMPLETE {
  UINT8   CommitHash[20];
  UINT8   TreeHash[20];
  UINT8   ParentCount;
  // ... (see full structure in section 6.34)
  UINT32  SubmoduleCount;
  UINT32  RemoteCount;
  UINT32  RefCount;
} ZOO64_GIT_COMPLETE;

Size: Variable (1-10 KB per file typical)
Use: Historical preservation, legal archives, full backups
```

#### 14.4.5 Performance Considerations

**Storage Overhead**
```
VCS metadata adds overhead - use judiciously:

Without VCS metadata: 150 bytes per file (basic Zoo64 metadata)
With minimal Git:     +21 bytes (14% overhead)
With standard Git:    +552 bytes (268% overhead)
With complete Git:    +1-10 KB (567-6666% overhead)

Recommendation: Use minimal metadata unless specific requirement exists
```

**Extraction Performance**
```
VCS metadata affects extraction:
- No impact on extraction speed (metadata is separate chunk)
- Enables selective extraction by commit/branch/tag
- Allows verification of source-to-binary mapping
```

#### 14.4.6 Example Decision Tree

```
Should I include VCS metadata?

1. Is this source code?
   NO  → Skip VCS metadata
   YES → Continue to 2

2. Is long-term preservation required?
   YES → Include STANDARD Git metadata
   NO  → Continue to 3

3. Is this a binary artifact?
   YES → Include MINIMAL Git metadata (commit hash only)
   NO  → Continue to 4

4. Is this for migration/conversion?
   YES → Include COMPLETE VCS metadata
   NO  → Continue to 5

5. Is this temporary or working copy?
   YES → Skip VCS metadata
   NO  → Include STANDARD metadata (default for source archives)
```

#### 14.4.7 Best Practices

1. **Default to NO**: Don't include VCS metadata unless there's a clear requirement
2. **Match Scope to Need**: Use minimal metadata when commit hash suffices
3. **Document Decisions**: Use archive YAML metadata to explain why VCS metadata was included/excluded
4. **Consider Compliance**: Legal requirements may mandate VCS metadata inclusion
5. **Think Long-Term**: Historical archives should include more metadata than temporary ones
6. **Verify Completeness**: If including VCS metadata, ensure all referenced objects are available

## 15. Coding Conventions

This specification follows NT/UEFI coding style conventions for all code examples and structure definitions.

### 15.1 Data Types

All structure definitions use UEFI standard data types:

```c
//
// UEFI Standard Data Types
//
typedef unsigned char      UINT8;
typedef unsigned short     UINT16;
typedef unsigned int       UINT32;
typedef unsigned long long UINT64;
typedef char               CHAR8;
typedef short              CHAR16;
typedef unsigned long      UINTN;    // Native pointer size
typedef long               INTN;     // Signed native pointer size
typedef unsigned char      BOOLEAN;

//
// Extended Types for Zoo64
//
typedef struct {
  UINT64  Low;   // Lower 64 bits
  UINT64  High;  // Upper 64 bits
} UINT128;

//
// UUID Type (RFC 4122)
//
typedef struct {
  UINT32  Data1;    // Time low
  UINT16  Data2;    // Time mid
  UINT16  Data3;    // Time high and version
  UINT8   Data4[8]; // Clock seq and node
} EFI_GUID;

//
// Zoo64 uses EFI_GUID for all UUID fields
//
typedef EFI_GUID ZOO64_UUID;

//
// Boolean Constants
//
#define TRUE   1
#define FALSE  0
```

**Important**: Magics and signatures MUST use `UINT8` arrays, NOT `CHAR8`:

```c
//
// CORRECT - Use UINT8 for binary magic values
//
typedef struct {
  UINT8   Magic[8];  // 0x5A 0x4F 0x4F 0x36 0x34 0x41 0x52 0x43
  UINT32  Version;
  // ...
} ZOO64_ARCHIVE_HEADER;

//
// INCORRECT - Do not use CHAR8 for binary data
//
typedef struct {
  CHAR8   Magic[8];  // WRONG - CHAR8 is for text
  // ...
} WRONG_HEADER;
```

### 15.2 Enumeration Style

All enumerations follow NT/UEFI PascalCase naming conventions (not underscore style):

**NT Enumeration Naming Rules**:
- Type name: PascalCase, prefixed with `ZOO64_` (e.g., `ZOO64_COMPRESSION_ALGORITHM`)
- Enum values: PascalCase, prefixed with type category (e.g., `Zoo64CompressZstd`)
- NO underscores in enum values (except in type name)
- Clear, readable names describing the value

```c
//
// Compression Algorithm Enumeration
// NT-style: Zoo64Compress* (PascalCase, no underscores)
//
//
// Compression Algorithm Enumeration (32-bit IDs)
// Comprehensive list of all known compression methods
//
typedef enum _ZOO64_COMPRESSION_ALGORITHM {
  // 0x0000-0x000F: Basic & Statistical
  Zoo64CompressStored               = 0x00000000,  // No compression
  Zoo64CompressZoz                  = 0x00000001,  // ZOZ adaptive pipeline
  Zoo64CompressRle                  = 0x00000002,  // Run-Length Encoding
  Zoo64CompressHuffman              = 0x00000003,  // Huffman coding only
  Zoo64CompressBitSquish            = 0x00000004,  // Bit squishing (reduced alphabet)
  Zoo64CompressArithmetic           = 0x00000005,  // Arithmetic coding
  Zoo64CompressRangeCoding          = 0x00000006,  // Range coding
  Zoo64CompressAns                  = 0x00000007,  // Asymmetric Numeral Systems

  // 0x0010-0x002F: LZ77 Family
  Zoo64CompressLz77                 = 0x00000010,  // Lempel-Ziv 1977
  Zoo64CompressLzss                 = 0x00000011,  // LZSS (LZ77 variant)
  Zoo64CompressDeflate              = 0x00000012,  // DEFLATE (RFC 1951)
  Zoo64CompressZlib                 = 0x00000013,  // ZLIB (RFC 1950, DEFLATE + wrapper)
  Zoo64CompressGzip                 = 0x00000014,  // GZIP (RFC 1952)
  Zoo64CompressLzo                  = 0x00000015,  // LZO (fast)
  Zoo64CompressLz4                  = 0x00000016,  // LZ4 (extremely fast)
  Zoo64CompressLz4Hc                = 0x00000017,  // LZ4 High Compression
  Zoo64CompressSnappy               = 0x00000018,  // Google Snappy
  Zoo64CompressLzf                  = 0x00000019,  // LibLZF
  Zoo64CompressLzrw                 = 0x0000001A,  // LZRW1/3/4
  Zoo64CompressLzjb                 = 0x0000001B,  // LZJB (ZFS)
  Zoo64CompressLzp                  = 0x0000001C,  // LZP (Lempel-Ziv-Predict)
  Zoo64CompressQuickLz              = 0x0000001D,  // QuickLZ
  Zoo64CompressLzham                = 0x0000001E,  // LZHAM
  Zoo64CompressLzx                  = 0x0000001F,  // Microsoft LZX (CAB)

  // 0x0030-0x003F: LZ78 Family
  Zoo64CompressLz78                 = 0x00000030,  // Lempel-Ziv 1978
  Zoo64CompressLzw                  = 0x00000031,  // Lempel-Ziv-Welch (GIF/TIFF)
  Zoo64CompressLzc                  = 0x00000032,  // UNIX compress (.Z)
  Zoo64CompressGif                  = 0x00000033,  // GIF LZW variant

  // 0x0040-0x005F: LZMA Family
  Zoo64CompressLzma                 = 0x00000040,  // LZMA (7-Zip)
  Zoo64CompressLzma2                = 0x00000041,  // LZMA2 (multi-threaded)
  Zoo64CompressXz                   = 0x00000042,  // XZ Utils (LZMA2-based)

  // 0x0060-0x007F: Modern General-Purpose
  Zoo64CompressZstd                 = 0x00000060,  // Zstandard (Facebook, recommended)
  Zoo64CompressBrotli               = 0x00000061,  // Brotli (Google, RFC 7932)
  Zoo64CompressLzfse                = 0x00000062,  // LZFSE (Apple)
  Zoo64CompressLizard               = 0x00000063,  // Lizard (LZ4 + Huffman)
  Zoo64CompressDensity              = 0x00000064,  // Density (Chameleon/Lion)

  // 0x0080-0x009F: BWT-Based
  Zoo64CompressBzip2                = 0x00000080,  // bzip2 (BWT + RLE + Huffman)
  Zoo64CompressBzip3                = 0x00000081,  // bzip3
  Zoo64CompressBwt                  = 0x00000082,  // Burrows-Wheeler Transform alone
  Zoo64CompressSbwt                 = 0x00000083,  // Sort-based BWT

  // 0x00A0-0x00BF: Context Modeling (PPM)
  Zoo64CompressPpmd                 = 0x000000A0,  // PPMd (Prediction by Partial Matching)
  Zoo64CompressPpmdH                = 0x000000A1,  // PPMD variant H
  Zoo64CompressPpmdJ                = 0x000000A2,  // PPMD variant J
  Zoo64CompressPpm                  = 0x000000A3,  // PPM (general)
  Zoo64CompressPpmii                = 0x000000A4,  // PPMII

  // 0x00C0-0x00CF: PAQ Family (Maximum Compression)
  Zoo64CompressPaq                  = 0x000000C0,  // PAQ general
  Zoo64CompressPaq6                 = 0x000000C1,  // PAQ6
  Zoo64CompressPaq7                 = 0x000000C2,  // PAQ7
  Zoo64CompressPaq8                 = 0x000000C3,  // PAQ8
  Zoo64CompressLpaq                 = 0x000000C4,  // LPAQ
  Zoo64CompressFpaq                 = 0x000000C5,  // FPAQ
  Zoo64CompressZpaq                 = 0x000000C6,  // ZPAQ

  // 0x00D0-0x00EF: Dictionary/LZH Family
  Zoo64CompressLzh                  = 0x000000D0,  // LHA/LZH
  Zoo64CompressLhArc                = 0x000000D1,  // LHARC (LHA archiver)
  Zoo64CompressLh1                  = 0x000000D2,  // LH1 method
  Zoo64CompressLh4                  = 0x000000D3,  // LH4 method
  Zoo64CompressLh5                  = 0x000000D4,  // LH5 method
  Zoo64CompressLh6                  = 0x000000D5,  // LH6 method
  Zoo64CompressLh7                  = 0x000000D6,  // LH7 method

  // 0x0100-0x011F: Legacy Archive Formats
  Zoo64CompressArc                  = 0x00000100,  // ARC (Squashing, Crunching, etc.)
  Zoo64CompressZoo                  = 0x00000101,  // ZOO archiver
  Zoo64CompressArj                  = 0x00000102,  // ARJ
  Zoo64CompressAce                  = 0x00000103,  // ACE
  Zoo64CompressRar                  = 0x00000104,  // RAR
  Zoo64CompressRar5                 = 0x00000105,  // RAR 5
  Zoo64CompressCab                  = 0x00000106,  // Microsoft Cabinet
  Zoo64CompressChm                  = 0x00000107,  // Microsoft CHM (LZX)
  Zoo64CompressSit                  = 0x00000108,  // StuffIt (Macintosh)
  Zoo64CompressSit5                 = 0x00000109,  // StuffIt 5
  Zoo64CompressCompact              = 0x0000010A,  // Compact
  Zoo64CompressPack                 = 0x0000010B,  // Pack
  Zoo64CompressCrunch               = 0x0000010C,  // Apple II Crunch
  Zoo64CompressSqueeze              = 0x0000010D,  // Squeeze (CP/M)
  Zoo64CompressSquash               = 0x0000010E,  // ARC Squash method
  Zoo64CompressImplode              = 0x0000010F,  // ZIP Implode
  Zoo64CompressShrink               = 0x00000110,  // ZIP Shrink
  Zoo64CompressReduce1              = 0x00000111,  // ZIP Reduce method 1
  Zoo64CompressReduce2              = 0x00000112,  // ZIP Reduce method 2
  Zoo64CompressReduce3              = 0x00000113,  // ZIP Reduce method 3
  Zoo64CompressReduce4              = 0x00000114,  // ZIP Reduce method 4

  // 0x0120-0x013F: Quantum/IBM
  Zoo64CompressQuantum              = 0x00000120,  // Quantum compression
  Zoo64CompressAlz                  = 0x00000121,  // ALZip
  Zoo64CompressUharc                = 0x00000122,  // UHARC
  Zoo64CompressFreeze               = 0x00000123,  // Freeze

  // 0x0140-0x015F: Platform-Specific
  Zoo64CompressNsis                 = 0x00000140,  // NSIS (LZMA variant)
  Zoo64CompressMszip                = 0x00000141,  // Microsoft MSZIP
  Zoo64CompressXpress               = 0x00000142,  // Microsoft Xpress (LZ77 + Huffman)
  Zoo64CompressXpressHuffman        = 0x00000143,  // Microsoft Xpress Huffman
  Zoo64CompressAplib                = 0x00000144,  // aPLib
  Zoo64CompressExepack              = 0x00000145,  // EXEPACK (DOS)
  Zoo64CompressLhark                = 0x00000146,  // LHARK
  Zoo64CompressPklite               = 0x00000147,  // PKLITE
  Zoo64CompressUpx                  = 0x00000148,  // UPX (executable packer)

  // 0x0160-0x017F: Game/Proprietary
  Zoo64CompressRefpack              = 0x00000160,  // RefPack (EA Games)
  Zoo64CompressOodle                = 0x00000161,  // Oodle (RAD Game Tools)
  Zoo64CompressKraken               = 0x00000162,  // Oodle Kraken
  Zoo64CompressMermaid              = 0x00000163,  // Oodle Mermaid
  Zoo64CompressSelkie               = 0x00000164,  // Oodle Selkie
  Zoo64CompressLeviathan            = 0x00000165,  // Oodle Leviathan

  // 0x0180-0x019F: Specialized
  Zoo64CompressShrinker             = 0x00000180,  // Text-optimized
  Zoo64CompressBcm                  = 0x00000181,  // BCM (BWT-based)
  Zoo64CompressCm                   = 0x00000182,  // Context Mixing
  Zoo64CompressDmC                  = 0x00000183,  // Dynamic Markov Compression
  Zoo64CompressFse                  = 0x00000184,  // Finite State Entropy
  Zoo64CompressHuff0                = 0x00000185,  // Huff0 (FSE variant)
  Zoo64CompressTans                 = 0x00000186,  // tANS

  // 0x01A0-0x01BF: Experimental/Research
  Zoo64CompressZling                = 0x000001A0,  // ZLING
  Zoo64CompressTornado              = 0x000001A1,  // Tornado
  Zoo64CompressBriggs               = 0x000001A2,  // Briggs
  Zoo64CompressCsc                  = 0x000001A3,  // CSC (Context Sorting Compression)
  Zoo64CompressNanozip              = 0x000001A4,  // NanoZip
  Zoo64CompressSrep                 = 0x000001A5,  // SREP
  Zoo64CompressBcj                  = 0x000001A6,  // BCJ (x86 filter)
  Zoo64CompressBcj2                 = 0x000001A7,  // BCJ2

  // 0x01C0-0x01CF: Video/Image-Specific
  Zoo64CompressJpeg                 = 0x000001C0,  // JPEG compression
  Zoo64CompressPng                  = 0x000001C1,  // PNG (DEFLATE-based)
  Zoo64CompressFlac                 = 0x000001C2,  // FLAC (audio)
  Zoo64CompressAlac                 = 0x000001C3,  // Apple Lossless
  Zoo64CompressWebp                 = 0x000001C4,  // WebP
  Zoo64CompressAvif                 = 0x000001C5,  // AVIF
  Zoo64CompressJxl                  = 0x000001C6,  // JPEG XL
  Zoo64CompressHeif                 = 0x000001C7,  // HEIF

  // 0xFFFF0000-0xFFFFFFFF: Custom & Reserved
  Zoo64CompressWasm32               = 0xFFFF0000,  // WASM32 embeddable module
  Zoo64CompressCustom1              = 0xFFFF0001,  // Custom algorithm 1
  Zoo64CompressCustom2              = 0xFFFF0002,  // Custom algorithm 2
  Zoo64CompressCustom3              = 0xFFFF0003,  // Custom algorithm 3
  Zoo64CompressReserved             = 0xFFFFFFFF   // Reserved
} ZOO64_COMPRESSION_ALGORITHM;

//
// Encryption Method Enumeration (32-bit IDs)
// NT-style: Zoo64Encrypt* (PascalCase)
//
typedef enum _ZOO64_ENCRYPTION_METHOD {
  Zoo64EncryptNone                  = 0x00000000,  // Not encrypted
  Zoo64EncryptAes256Gcm             = 0x00000001,  // AES-256-GCM (recommended)
  Zoo64EncryptAes256CbcHmac         = 0x00000002,  // AES-256-CBC + HMAC-SHA256
  Zoo64EncryptChaCha20Poly1305      = 0x00000003,  // ChaCha20-Poly1305
  Zoo64EncryptAes128Gcm             = 0x00000004,  // AES-128-GCM
  Zoo64EncryptTwofish256Gcm         = 0x00000005,  // Twofish-256-GCM
  Zoo64EncryptSerpent256Gcm         = 0x00000006,  // Serpent-256-GCM
  Zoo64EncryptAes192Gcm             = 0x00000007,  // AES-192-GCM
  Zoo64EncryptCamellia256Gcm        = 0x00000008,  // Camellia-256-GCM
  Zoo64EncryptAria256Gcm            = 0x00000009,  // ARIA-256-GCM
  Zoo64EncryptSm4Gcm                = 0x0000000A,  // SM4-GCM (Chinese standard)
  Zoo64EncryptXChaCha20Poly1305     = 0x0000000B   // XChaCha20-Poly1305
} ZOO64_ENCRYPTION_METHOD;

//
// Key Derivation Function Enumeration (32-bit IDs)
// NT-style: Zoo64Kdf* (PascalCase)
//
typedef enum _ZOO64_KDF_ALGORITHM {
  Zoo64KdfNone                      = 0x00000000,  // Direct key (not recommended)
  Zoo64KdfPbkdf2HmacSha256          = 0x00000001,  // PBKDF2 with SHA-256
  Zoo64KdfPbkdf2HmacSha512          = 0x00000002,  // PBKDF2 with SHA-512
  Zoo64KdfArgon2id                  = 0x00000003,  // Argon2id (recommended)
  Zoo64KdfArgon2i                   = 0x00000004,  // Argon2i
  Zoo64KdfArgon2d                   = 0x00000005,  // Argon2d
  Zoo64KdfScrypt                    = 0x00000006,  // scrypt
  Zoo64KdfBcrypt                    = 0x00000007,  // bcrypt
  Zoo64KdfBalloon                   = 0x00000008,  // Balloon hashing
  Zoo64KdfCatena                    = 0x00000009,  // Catena
  Zoo64KdfMakwa                     = 0x0000000A   // Makwa
} ZOO64_KDF_ALGORITHM;

//
// Hash Algorithm Enumeration (32-bit IDs)
// NT-style: Zoo64Hash* (PascalCase)
//
typedef enum _ZOO64_HASH_ALGORITHM {
  Zoo64HashNone                     = 0x00000000,  // No hash
  Zoo64HashCrc32                    = 0x00000001,  // CRC-32 (IEEE)
  Zoo64HashCrc32C                   = 0x00000002,  // CRC-32C (Castagnoli)
  Zoo64HashCrc64                    = 0x00000003,  // CRC-64 (ECMA)
  Zoo64HashAdler32                  = 0x00000004,  // Adler-32
  Zoo64HashMd5                      = 0x00000005,  // MD5 (deprecated)
  Zoo64HashSha1                     = 0x00000006,  // SHA-1 (deprecated)
  Zoo64HashSha224                   = 0x00000007,  // SHA-224
  Zoo64HashSha256                   = 0x00000008,  // SHA-256 (recommended)
  Zoo64HashSha384                   = 0x00000009,  // SHA-384
  Zoo64HashSha512                   = 0x0000000A,  // SHA-512
  Zoo64HashSha512_224               = 0x0000000B,  // SHA-512/224
  Zoo64HashSha512_256               = 0x0000000C,  // SHA-512/256
  Zoo64HashSha3_224                 = 0x0000000D,  // SHA3-224
  Zoo64HashSha3_256                 = 0x0000000E,  // SHA3-256
  Zoo64HashSha3_384                 = 0x0000000F,  // SHA3-384
  Zoo64HashSha3_512                 = 0x00000010,  // SHA3-512
  Zoo64HashBlake2b                  = 0x00000011,  // BLAKE2b
  Zoo64HashBlake2s                  = 0x00000012,  // BLAKE2s
  Zoo64HashBlake3                   = 0x00000013,  // BLAKE3 (recommended)
  Zoo64HashXxhash32                 = 0x00000014,  // xxHash-32
  Zoo64HashXxhash64                 = 0x00000015,  // xxHash-64
  Zoo64HashXxh3                     = 0x00000016,  // xxHash3
  Zoo64HashMurmur3_32               = 0x00000017,  // MurmurHash3-32
  Zoo64HashMurmur3_128              = 0x00000018,  // MurmurHash3-128
  Zoo64HashCityHash64               = 0x00000019,  // CityHash64
  Zoo64HashCityHash128              = 0x0000001A,  // CityHash128
  Zoo64HashFarmHash64               = 0x0000001B,  // FarmHash64
  Zoo64HashMetroHash64              = 0x0000001C,  // MetroHash64
  Zoo64HashSipHash24                = 0x0000001D,  // SipHash-2-4
  Zoo64HashHighwayHash256           = 0x0000001E,  // HighwayHash256
  Zoo64HashSm3                      = 0x0000001F,  // SM3 (Chinese standard)
  Zoo64HashStreebog256              = 0x00000020,  // Streebog-256 (Russian standard)
  Zoo64HashStreebog512              = 0x00000021,  // Streebog-512
  Zoo64HashRipemd160                = 0x00000022,  // RIPEMD-160
  Zoo64HashWhirlpool                = 0x00000023,  // Whirlpool
  Zoo64HashTiger                    = 0x00000024,  // Tiger
  Zoo64HashGost                     = 0x00000025   // GOST R 34.11-94
} ZOO64_HASH_ALGORITHM;

//
// Digital Signature Type Enumeration (32-bit IDs)
// NT-style: Zoo64Sig* (PascalCase)
//
typedef enum _ZOO64_SIGNATURE_TYPE {
  Zoo64SigNone                      = 0x00000000,  // No signature
  Zoo64SigRsa2048                   = 0x00000001,  // RSA-2048
  Zoo64SigRsa3072                   = 0x00000002,  // RSA-3072
  Zoo64SigRsa4096                   = 0x00000003,  // RSA-4096
  Zoo64SigEcdsaP256                 = 0x00000004,  // ECDSA P-256 (secp256r1)
  Zoo64SigEcdsaP384                 = 0x00000005,  // ECDSA P-384 (secp384r1)
  Zoo64SigEcdsaP521                 = 0x00000006,  // ECDSA P-521 (secp521r1)
  Zoo64SigEd25519                   = 0x00000007,  // Ed25519 (recommended)
  Zoo64SigEd448                     = 0x00000008,  // Ed448
  Zoo64SigDsa2048                   = 0x00000009,  // DSA-2048
  Zoo64SigDsa3072                   = 0x0000000A,  // DSA-3072
  Zoo64SigEcdsaSecp256k1            = 0x0000000B,  // ECDSA secp256k1 (Bitcoin)
  Zoo64SigSm2                       = 0x0000000C,  // SM2 (Chinese standard)
  Zoo64SigFalcon512                 = 0x0000000D,  // Falcon-512 (post-quantum)
  Zoo64SigFalcon1024                = 0x0000000E,  // Falcon-1024 (post-quantum)
  Zoo64SigDilithium2                = 0x0000000F,  // Dilithium2 (post-quantum)
  Zoo64SigDilithium3                = 0x00000010,  // Dilithium3 (post-quantum)
  Zoo64SigDilithium5                = 0x00000011,  // Dilithium5 (post-quantum)
  Zoo64SigSphincsPlus128            = 0x00000012,  // SPHINCS+ 128 (post-quantum)
  Zoo64SigSphincsPlus256            = 0x00000013   // SPHINCS+ 256 (post-quantum)
} ZOO64_SIGNATURE_TYPE;

//
// Overlay File Type Enumeration (32-bit IDs)
// NT-style: Zoo64Overlay* (PascalCase)
//
typedef enum _ZOO64_OVERLAY_TYPE {
  Zoo64OverlayNew                   = 0x00000000,  // New file
  Zoo64OverlayModified              = 0x00000001,  // Modified (delta)
  Zoo64OverlayDeleted               = 0x00000002,  // Deleted (tombstone)
  Zoo64OverlayUnchanged             = 0x00000003,  // Unchanged (reference)
  Zoo64OverlayMoved                 = 0x00000004   // Moved/renamed
} ZOO64_OVERLAY_TYPE;

//
// Metadata Chunk Type Enumeration
// NT-style: Zoo64Meta* (PascalCase)
//
typedef enum _ZOO64_METADATA_TYPE {
  Zoo64MetaAcl                      = 0x0001,  // Access Control List
  Zoo64MetaXattr                    = 0x0002,  // Extended attributes
  Zoo64MetaAds                      = 0x0003,  // Alternate Data Streams
  Zoo64MetaSecurityDescriptor       = 0x0004,  // Windows security
  Zoo64MetaResourceFork             = 0x0005,  // macOS resource fork
  Zoo64MetaExtendedTimestamps       = 0x0006,  // Additional timestamps
  Zoo64MetaFileCapabilities         = 0x0007,  // Linux capabilities
  Zoo64MetaSelinuxContext           = 0x0008,  // SELinux context
  Zoo64MetaYaml                     = 0x000A,  // YAML metadata
  Zoo64MetaMacosUuid                = 0x000B,  // macOS UUIDs
  Zoo64MetaBsdFlags                 = 0x000C,  // BSD file flags
  Zoo64MetaLinuxFlags               = 0x000D,  // Linux attributes
  Zoo64MetaWindowsAttr              = 0x000E,  // Windows attributes
  Zoo64MetaHardLink                 = 0x000F,  // Hard link target
  Zoo64MetaSymlink                  = 0x0010,  // Symbolic link
  Zoo64MetaGit                      = 0x0032,  // Git metadata
  Zoo64MetaPerforce                 = 0x0033,  // Perforce metadata
  Zoo64MetaSvn                      = 0x0034,  // Subversion metadata
  Zoo64MetaMercurial                = 0x0037,  // Mercurial metadata
  Zoo64MetaSparseFile               = 0x0045,  // Sparse file holes
  Zoo64MetaDeltaRevision            = 0x0046,  // Delta revision
  Zoo64MetaBlockDedup               = 0x004A,  // Block deduplication
  Zoo64MetaWasm32Module             = 0x0100   // WASM32 module (custom algorithms)
  // See section 6.2 for complete list
} ZOO64_METADATA_TYPE;

//
// ACL Source System Enumeration
// NT-style: Zoo64AclSource* (PascalCase)
// Example from user: Zoo64AclSourceWindowsNt
//
typedef enum _ZOO64_ACL_SOURCE_SYSTEM {
  Zoo64AclSourceUnknown             = 0x0000,  // Unknown/Generic
  Zoo64AclSourcePosix               = 0x0001,  // POSIX.1e
  Zoo64AclSourceNfsv4               = 0x0002,  // NFSv4 (RFC 7530)
  Zoo64AclSourceWindowsNt           = 0x0003,  // Windows NT DACL/SACL
  Zoo64AclSourceMacos               = 0x0004,  // macOS Extended ACLs
  Zoo64AclSourceOpenvms             = 0x0005,  // OpenVMS ACLs
  Zoo64AclSourceOs400               = 0x0006,  // OS/400 Authorities
  Zoo64AclSourceMvsRacf             = 0x0007,  // MVS/RACF
  Zoo64AclSourceNetware             = 0x0008,  // Novell NetWare
  Zoo64AclSourceVines               = 0x0009,  // Banyan VINES
  Zoo64AclSourceAfs                 = 0x000A,  // Andrew File System
  Zoo64AclSourceCoda                = 0x000B,  // CODA Distributed FS
  Zoo64AclSourceZfs                 = 0x000C,  // Solaris ZFS
  Zoo64AclSourceDceDfs              = 0x0019,  // DCE DFS
  Zoo64AclSourceGfs                 = 0x001A,  // Global File System
  Zoo64AclSourceMsDfs               = 0x001B   // Microsoft DFS
} ZOO64_ACL_SOURCE_SYSTEM;
```

### 15.3 Commenting Style

All code follows UEFI commenting conventions:

```c
//
// Multi-line comment describing structure, function, or block
// Each line starts with // and is properly indented
//
typedef struct {
  UINT64  Field1;  // Inline comment for field
  UINT32  Field2;  // Brief field description
} EXAMPLE_STRUCTURE;

//
// Function description explaining purpose, parameters, and return value
//
BOOLEAN
ExampleFunction (
  IN     CONST UINT8   *Input,      // Input buffer
  IN     UINTN         InputSize,   // Size of input
  OUT    UINT8         *Output,     // Output buffer
  OUT    UINTN         *OutputSize  // Size of output
  )
{
  //
  // Implementation with clear comments
  //
  return TRUE;
}
```

### 15.4 Naming Conventions

**No Hungarian Notation**: Variable names describe purpose, not type:

```c
//
// CORRECT - Clear descriptive names
//
UINT32  FileCount;
UINT64  TotalSize;
UINT8   *Buffer;
UINTN   BytesProcessed;

//
// INCORRECT - Hungarian notation (do not use)
//
UINT32  dwFileCount;    // Wrong - no 'dw' prefix
UINT64  ullTotalSize;   // Wrong - no 'ull' prefix
UINT8   *pBuffer;       // Wrong - no 'p' prefix
UINTN   cbBytes;        // Wrong - no 'cb' prefix
```

**Structure Member Names**: Use clear, descriptive names:
- Start with capital letter
- Use camel case for multiple words
- No underscores except in structure tags

```c
typedef struct _ZOO64_FILE_HEADER {
  UINT64  UncompressedSize;  // Clear name
  UINT64  CompressedSize;    // No abbreviations unless standard
  UINT32  CRC32;             // Standard abbreviation OK
  UINT64  SHA256[4];         // Standard algorithm name
} ZOO64_FILE_HEADER;
```

### 15.5 Magic Values

All magic values are defined as byte arrays with hexadecimal constants:

```c
//
// Archive magic signature
//
#define ZOO64_ARCHIVE_MAGIC { 0x5A, 0x4F, 0x4F, 0x36, 0x34, 0x41, 0x52, 0x43 }
// "ZOO64ARC" in ASCII: Z=0x5A O=0x4F O=0x4F 6=0x36 4=0x34 A=0x41 R=0x52 C=0x43

//
// File entry magic
//
#define ZOO64_FILE_ENTRY_MAGIC { 0x46, 0x49, 0x4C, 0x45, 0x45, 0x4E, 0x54, 0x52 }
// "FILEENTR"

//
// Quick directory magic
//
#define ZOO64_QUICK_DIR_MAGIC { 0x51, 0x55, 0x49, 0x43, 0x4B, 0x44, 0x49, 0x52 }
// "QUICKDIR"
```

## 16. Error Handling

### 16.1 Error Codes

Following NT HRESULT style, all error codes use UINT32 type with standard facility codes:

```c
//
// Success codes
//
#define ZOO64_OK                     ((UINT32)0x00000000)

//
// Error codes (0x8004xxxx range - standard COM facility)
//
#define ZOO64_E_INVALID_MAGIC        ((UINT32)0x80040001)
#define ZOO64_E_UNSUPPORTED_VERSION  ((UINT32)0x80040002)
#define ZOO64_E_CORRUPT_HEADER       ((UINT32)0x80040003)
#define ZOO64_E_CORRUPT_DATA         ((UINT32)0x80040004)
#define ZOO64_E_DECOMPRESSION_ERROR  ((UINT32)0x80040005)
#define ZOO64_E_SIGNATURE_INVALID    ((UINT32)0x80040006)
#define ZOO64_E_ENCRYPTION_ERROR     ((UINT32)0x80040007)
#define ZOO64_E_FILE_NOT_FOUND       ((UINT32)0x80040008)
#define ZOO64_E_INVALID_PATH         ((UINT32)0x80040009)
#define ZOO64_E_ACCESS_DENIED        ((UINT32)0x8004000A)
#define ZOO64_E_OUT_OF_MEMORY        ((UINT32)0x8004000B)
#define ZOO64_E_INVALID_PARAMETER    ((UINT32)0x8004000C)
#define ZOO64_E_NOT_IMPLEMENTED      ((UINT32)0x8004000D)

//
// Macro to test success
//
#define ZOO64_SUCCESS(Status)  ((Status) == ZOO64_OK)
#define ZOO64_ERROR(Status)    ((Status) != ZOO64_OK)
```

## 17. Implementation Notes

### 17.1 Byte Order (Endianness)

**CRITICAL**: All Zoo64 archives use **LITTLE-ENDIAN** byte order for all multi-byte integers.

This applies to:
- All UINT16, UINT32, UINT64, UINT128 fields
- All structure members
- All magic signatures
- All numeric metadata

**Endianness Examples**:

```c
//
// UINT32 value 0x12345678 stored as:
// Offset: 00 01 02 03
// Bytes:  78 56 34 12  (little-endian)
//

//
// Archive magic 0x5A4F4F3634415243 ("ZOO64ARC") stored as:
// Offset: 00 01 02 03 04 05 06 07
// Bytes:  43 52 41 34 36 4F 4F 5A  (little-endian UINT64)
//
// OR as UINT8 array (platform-independent):
// Offset: 00 01 02 03 04 05 06 07
// Bytes:  5A 4F 4F 36 34 41 52 43  (ASCII "ZOO64ARC")
//

//
// UINT128 value (low=0x1122334455667788, high=0x99AABBCCDDEEFF00):
// Offset: 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F
// Bytes:  88 77 66 55 44 33 22 11 00 FF EE DD CC BB AA 99
//         └─────── Low (LE) ──────┘ └────── High (LE) ─────┘
//
```

**Why Little-Endian**:
- x86/x64 native byte order (most common platforms)
- ARM can operate in little-endian mode (common configuration)
- Simpler implementation on dominant architectures
- Consistent with ZIP, ELF, PE, and most modern formats

**Cross-Platform Compatibility**:

On big-endian systems (PowerPC, SPARC, some ARM configurations), implementations MUST perform byte swapping:

```c
//
// Byte swap macros for big-endian systems
//
#if defined(__BIG_ENDIAN__) || defined(_BIG_ENDIAN)

UINT16 SwapUint16(UINT16 Value) {
  return (Value >> 8) | (Value << 8);
}

UINT32 SwapUint32(UINT32 Value) {
  return ((Value >> 24) & 0x000000FF) |
         ((Value >> 8)  & 0x0000FF00) |
         ((Value << 8)  & 0x00FF0000) |
         ((Value << 24) & 0xFF000000);
}

UINT64 SwapUint64(UINT64 Value) {
  return ((Value >> 56) & 0x00000000000000FFULL) |
         ((Value >> 40) & 0x000000000000FF00ULL) |
         ((Value >> 24) & 0x0000000000FF0000ULL) |
         ((Value >> 8)  & 0x00000000FF000000ULL) |
         ((Value << 8)  & 0x000000FF00000000ULL) |
         ((Value << 24) & 0x0000FF0000000000ULL) |
         ((Value << 40) & 0x00FF000000000000ULL) |
         ((Value << 56) & 0xFF00000000000000ULL);
}

//
// Use after reading from archive
//
UINT32 FileSize = SwapUint32(Header->UncompressedSize);

#else
//
// Little-endian systems: no swapping needed
//
#define SwapUint16(x) (x)
#define SwapUint32(x) (x)
#define SwapUint64(x) (x)
#endif
```

**Verification**:

All implementations MUST verify endianness by checking magic signatures:

```c
//
// Verify archive magic (handles endianness automatically with UINT8 array)
//
UINT8 ExpectedMagic[8] = { 0x5A, 0x4F, 0x4F, 0x36, 0x34, 0x41, 0x52, 0x43 };

if (CompareMem(Header->Magic, ExpectedMagic, 8) != 0) {
  //
  // Try byte-swapped magic (in case of endianness mismatch)
  //
  UINT64 SwappedMagic = SwapUint64(*(UINT64*)Header->Magic);
  if (CompareMem(&SwappedMagic, ExpectedMagic, 8) == 0) {
    return ZOO64_E_WRONG_ENDIAN;  // Archive created on different-endian system
  }
  return ZOO64_E_INVALID_MAGIC;
}
```

**Summary**:
- **All multi-byte values**: Little-endian
- **All magic signatures**: Use UINT8 arrays (platform-independent)
- **Big-endian systems**: MUST byte-swap when reading/writing
- **Never store big-endian**: Archives are always little-endian on disk

### 17.2 Alignment

Structures are packed (no padding). Use `__attribute__((packed))` or `#pragma pack(1)`.

```c
//
// All structures must be packed to ensure correct layout
//
#pragma pack(push, 1)
typedef struct _ZOO64_ARCHIVE_HEADER {
  UINT8   Magic[8];           // No padding here
  UINT32  Version;            // Immediately follows Magic
  UINT32  Flags;              // Immediately follows Version
  // ...
} ZOO64_ARCHIVE_HEADER;
#pragma pack(pop)
```

**Note**: Packed structures may cause unaligned access issues on some architectures (ARM, MIPS). Implementations should either:
1. Read into packed structure (may be slow on ARM)
2. Read bytes and assemble fields manually (portable)
3. Use compiler-specific unaligned access support

### 17.3 String Encoding

All paths and text metadata use **UTF-8** encoding.

- **Paths**: UTF-8 with NFC normalization (Unicode Normalization Form C)
- **YAML metadata**: UTF-8
- **Comments**: UTF-8
- **Text fields**: UTF-8

**Never use**:
- ASCII (except for legacy compatibility)
- UTF-16 (except in Windows-specific metadata)
- ISO-8859-1 or other code pages
- Platform-specific encodings

### 17.4 Checksums

- **CRC32**: Uses IEEE polynomial (0xEDB88320), little-endian representation
- **SHA-256**: Standard byte order (big-endian in specification, stored as byte array)
- **SHA-512, BLAKE3, etc.**: Stored as byte arrays (no endianness issues)

**Note**: Hash digests are stored as UINT8 arrays, so endianness does not apply.

### 17.5 Compression Reset Points

For seekable compression, compressor state is reset at each block boundary to enable independent decompression.

### 17.6 Magic Signature Format

All magic signatures are stored as UINT8 byte arrays in ASCII character order:

```c
//
// Magic signatures - always use UINT8 arrays
//
UINT8 Magic[8];  // NOT UINT64, NOT CHAR8

//
// Correct initialization
//
UINT8 ArchiveMagic[8] = { 0x5A, 0x4F, 0x4F, 0x36, 0x34, 0x41, 0x52, 0x43 };
// Reads as "ZOO64ARC" in ASCII

//
// When comparing magics, use byte-by-byte comparison
//
if (CompareMem(Header->Magic, ArchiveMagic, 8) == 0) {
  // Valid magic
}
```

This approach is endian-neutral because each byte is compared individually.

## 17. Classic Zoo Format Compatibility

Zoo64 can read and convert classic Zoo 2.1 archives while providing bidirectional compatibility options.

### 17.1 Classic Zoo Format Overview

The original Zoo format (created by Rahul Dhesi, 1986-1991) used:
- Magic: 0xFDC4A7DC (little-endian: 0xDCA7C4FD)
- 13-character filenames (MS-DOS 8.3 + path)
- LZH and LZD compression
- Generation numbers for versioning
- Directory entries with deleted file tracking

### 17.2 Reading Classic Zoo Archives

When Zoo64 encounters classic Zoo magic (0xFDC4A7DC):

```c
#pragma pack(push, 1)
typedef struct _CLASSIC_ZOO_HEADER {
  UINT32  Magic;              /// 0xFDC4A7DC
  UINT32  FirstEntryOffset;   /// Offset to first file entry
  UINT32  MinusOffset;        /// Negative offset to last entry (-)
  UINT8   MajorVersion;       /// Major version (2)
  UINT8   MinorVersion;       /// Minor version (1)
  /// Additional header fields...
} CLASSIC_ZOO_HEADER;
#pragma pack(pop)
```

### 17.3 Classic Zoo Directory Entry

```c
#pragma pack(push, 1)
typedef struct _CLASSIC_ZOO_ENTRY {
  UINT32  Magic;              /// 0xFDC4A7DC
  UINT8   CompressionMethod;  /// 0=stored, 1=LZD, 2=LZH
  UINT8   NextEntryOffset;    /// Offset to next entry (variable)
  UINT32  OriginalSize;       /// Uncompressed size
  UINT32  CompressedSize;     /// Compressed size
  UINT16  Date;               /// MS-DOS date format
  UINT16  Time;               /// MS-DOS time format
  UINT16  CRC;                /// CRC-16
  UINT32  OriginalPosition;   /// Original file position
  UINT16  Attributes;         /// File attributes
  UINT8   Generation;         /// Generation number
  UINT8   Deleted;            /// Deletion marker
  char    FileName[13];       /// Null-terminated filename
  char    Comment[?];         /// Variable-length comment
  /// Followed by compressed data
} CLASSIC_ZOO_ENTRY;
#pragma pack(pop)
```

### 17.4 Conversion: Classic Zoo → Zoo64

When converting classic Zoo to Zoo64:

1. **Archive Header**: Create Zoo64 header with compatibility flag set
2. **Filenames**: Convert 13-char filenames to UTF-8 paths
3. **Compression**:
   - **Option 1**: Decompress LZH/LZD, recompress with Zoo64 pipeline
   - **Option 2**: Store compressed data as-is with compression method marker
4. **Timestamps**: Convert MS-DOS date/time to NTP extended format
5. **Metadata**: Store generation numbers in YAML metadata
6. **CRC**: Upgrade CRC-16 to CRC-32 + SHA-256
7. **Attributes**: Convert to appropriate platform metadata

#### 17.4.1 MS-DOS to NTP Timestamp Conversion

Classic Zoo stores timestamps in MS-DOS date/time format (16-bit date + 16-bit time):

```c
// MS-DOS date format (16 bits)
// Bits 0-4:   Day (1-31)
// Bits 5-8:   Month (1-12)
// Bits 9-15:  Year (0 = 1980, 127 = 2107)

// MS-DOS time format (16 bits)
// Bits 0-4:   Seconds/2 (0-29)
// Bits 5-10:  Minutes (0-59)
// Bits 11-15: Hours (0-23)

UINT64 DOSTimeToNTP(UINT16 dosDate, UINT16 dosTime) {
  /// Extract fields
  int day   = dosDate & 0x1F;
  int month = (dosDate >> 5) & 0x0F;
  int year  = ((dosDate >> 9) & 0x7F) + 1980;
  int sec   = (dosTime & 0x1F) * 2;
  int min   = (dosTime >> 5) & 0x3F;
  int hour  = (dosTime >> 11) & 0x1F;

  /// Convert to Unix timestamp (use mktime or equivalent)
  struct tm t = {0};
  t.tm_year = year - 1900;
  t.tm_mon = month - 1;
  t.tm_mday = day;
  t.tm_hour = hour;
  t.tm_min = min;
  t.tm_sec = sec;
  time_t unixTime = mktime(&t);

  /// Convert to NTP extended format
  return UnixToNTP(unixTime, 0);
}
```

**Note**: MS-DOS timestamps have 2-second precision and no timezone information (assumed local time). When converting to Zoo64, record both ModificationTime and BirthTime as the same value since classic Zoo doesn't distinguish creation vs modification time.

### 17.5 Conversion: Zoo64 → Classic Zoo (Limited)

For compatibility with legacy tools:

**Restrictions:**
- Filenames truncated to 13 characters (8.3 DOS format)
- Paths flattened (directory separators converted to underscores)
- Compression converted to LZH or stored
- Metadata discarded (except what fits in comment field)
- File size limit: 4 GB (32-bit size fields)
- Timestamps truncated to 2-second precision (MS-DOS format)
- Timestamps after 2107 cannot be represented

**Process:**
```
1. Check constraints (filename length, file size, compression type)
2. Warn user about metadata loss
3. Decompress Zoo64 data
4. Recompress with LZH (if beneficial) or store
5. Convert NTP timestamps to MS-DOS date/time
6. Generate classic Zoo directory entries
7. Write classic Zoo format
```

#### 17.5.1 NTP to MS-DOS Timestamp Conversion

```c
void NTPToDOSTime(UINT64 ntpTime, UINT16 *dosDate, UINT16 *dosTime) {
  /// Convert NTP to Unix time
  time_t unixSec;
  uint32_t nanoSec;
  NTPToUnix(ntpTime, &unixSec, &nanoSec);

  /// Convert to local time
  struct tm *t = localtime(&unixSec);

  /// Validate range (1980-2107)
  if (t->tm_year + 1900 < 1980) {
    /// Clamp to minimum (1980-01-01 00:00:00)
    *dosDate = 0x0021;  /// 1980-01-01
    *dosTime = 0x0000;  /// 00:00:00
    return;
  }
  if (t->tm_year + 1900 > 2107) {
    /// Clamp to maximum (2107-12-31 23:59:58)
    *dosDate = 0xFF9F;  /// 2107-12-31
    *dosTime = 0xBF7D;  /// 23:59:58
    return;
  }

  /// Encode MS-DOS date
  *dosDate = ((t->tm_year + 1900 - 1980) << 9) |
             ((t->tm_mon + 1) << 5) |
             t->tm_mday;

  /// Encode MS-DOS time (2-second precision)
  *dosTime = (t->tm_hour << 11) |
             (t->tm_min << 5) |
             (t->tm_sec / 2);
}
```

### 17.6 Dual-Format Archives

Zoo64 supports creating "dual-format" archives readable by both Zoo64 and classic Zoo:

```
[Classic Zoo Header]
[Classic Zoo Entries...] (limited 13-char filenames)
[Classic Zoo Data]
[Zoo64 Extended Header] (magic: 0x5A4F4F3634415243)
[Zoo64 Entries...] (full metadata, long filenames)
[Zoo64 Central Directory]
[End of Archive]
```

**Behavior:**
- Classic Zoo tools: Read first header, see classic format archive
- Zoo64 tools: Detect extended header, use Zoo64 format with full metadata
- File data stored once, referenced by both formats

### 17.7 Classic Zoo Compression Methods

For compatibility, Zoo64 can optionally support classic compression:

```
Method 0: Stored (no compression)
Method 1: LZD (Lempel-Ziv with dynamic Huffman)
Method 2: LZH (Lempel-Ziv + static Huffman, similar to LHA)
```

Implemented via compatibility shims:
```c
BOOLEAN ClassicZooDecompress(UINT8 method, const UINT8 *input, ...);
BOOLEAN ClassicZooCompress(UINT8 method, const UINT8 *input, ...);
```

### 17.8 Migration Strategy

Recommended migration path from classic Zoo to Zoo64:

1. **Phase 1**: Read-only compatibility
   - Zoo64 tools can extract classic Zoo archives
   - No modification of classic archives

2. **Phase 2**: Conversion tool
   - Provide `zoo64conv` utility
   - Convert classic → Zoo64 with full metadata preservation
   - Optionally create dual-format archives

3. **Phase 3**: Native Zoo64
   - Create new archives in Zoo64 format
   - Maintain classic Zoo reading capability for legacy data

### 17.9 Compatibility Mode Flag

When archive header flag bit 7 (Classic Zoo compatibility mode) is set:
- Enforce 13-character filename limit
- Disable UTF-8 strict mode
- Store filenames in uppercase (DOS convention)
- Generate both classic and Zoo64 directory entries
- Use MS-DOS timestamp formats

## 18. Version History

- **1.0** (2025-10-31): Initial specification draft
  - Basic archive structure with redundant directories
  - ZOZ adaptive entropy-reduction compression pipeline
  - Variable-length UTF-8 paths
  - ACL, xattr, ADS metadata support
  - YAML metadata (archive and file level)
  - Digital signatures (file and archive level)
  - Seekable compression with block tables
  - Solid compression support
  - COM component interfaces

- **1.1** (2025-11-01): Enhanced features
  - Platform-specific metadata (macOS UUIDs, BSD/Linux flags, Windows attributes)
  - Hard link and symbolic link support
  - Encryption support (AES-256-GCM, ChaCha20-Poly1305, Argon2id KDF)
  - Multi-volume archive support
  - Classic Zoo format compatibility
  - Redundant directory structure (ZIP-like)
  - Archive-level and file-level encryption
  - Password verification mechanisms
  - Dual-format archives (Zoo64 + Classic Zoo)

## 19. Future Extensions

Reserved areas for future features:
- Delta compression (binary diff between versions)
- Content-based deduplication (hash-based)
- Error correction codes (Reed-Solomon, LDPC)
- Compression algorithm plugins (dynamic loading)
- Streaming compression (no seekable table)
- Archive repair and recovery tools
- Incremental backups with change tracking
- Network-transparent archives (HTTP range requests)
- Archive virtualization (mount as filesystem)
- Smart compression (ML-based algorithm selection)

---

**End of Specification**
