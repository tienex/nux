# Zoo64 Archive Format Specification

**Version:** 1.0 Draft
**Date:** 2025-10-31
**Status:** Specification Draft

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

## 2. Archive Structure

### 2.1 Overall Layout

Zoo64 uses a redundant directory structure similar to ZIP format. Each file has both a local header (embedded with the file data) and a central directory entry (at the end of the archive). This provides fast scanning and recovery from corruption.

```
[Archive Header]
[Compression Mode Descriptor]
[Archive YAML Metadata] (optional)
[File Entries...]
  [Local File Header]         ← Redundant: full file metadata
  [File Metadata]
    [Binary Metadata Chunks] (ACL, xattr, ADS, etc.)
    [YAML Metadata] (optional)
  [Encryption Header] (optional)
  [File Data / Compressed Data]
  [File Signature] (optional)
[Central Directory]            ← Redundant: duplicates file metadata
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
Volume Header:        0x564F4C554D4548    ("VOLUMEH ")
Volume Footer:        0x564F4C554D4546    ("VOLUMEF ")
End of Archive:       0x454E444F46415243  ("ENDOFARC")
```

## 3. Archive Header

The archive header appears at the beginning of every Zoo64 archive (or first volume in multi-volume archives).

```c
typedef struct _ZOO64_ARCHIVE_HEADER {
  UINT64  Magic;              // 0x5A4F4F3634415243 ("ZOO64ARC")
  UINT16  MajorVersion;       // Format major version (1)
  UINT16  MinorVersion;       // Format minor version (0)
  UINT32  Flags;              // Archive flags
  UINT64  CreationTime;       // NTP extended format timestamp
  UINT64  ModificationTime;   // NTP extended format timestamp
  UINT32  CompressionMode;    // Compression mode identifier
  UINT32  FileCount;          // Number of files in archive (all volumes)
  UINT64  CentralDirOffset;   // Offset to central directory (in last volume)
  UINT64  ArchiveSize;        // Total archive size in bytes (all volumes)
  UINT32  BlockSize;          // Block size for seekable compression (power of 2)
  UINT64  YamlMetadataOffset; // Offset to archive YAML metadata (0 if none)
  UINT32  YamlMetadataSize;   // Size of archive YAML metadata
  UINT16  VolumeNumber;       // Volume number (0 for single archive, 1+ for multi-volume)
  UINT16  TotalVolumes;       // Total number of volumes (0 for single archive)
  UINT32  VolumeSize;         // Maximum size per volume (0 for single archive)
  UINT8   UUID[16];           // Archive UUID (same across all volumes)
} __attribute__((packed)) ZOO64_ARCHIVE_HEADER;
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
Bit 16-31: Reserved for future use
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

## 3.3 Volume Header

Appears at the start of volumes 2-N in multi-volume archives.

```c
typedef struct _ZOO64_VOLUME_HEADER {
  UINT64  Magic;              // 0x564F4C554D4548 ("VOLUMEH ")
  UINT16  VolumeNumber;       // Volume number (2, 3, 4...)
  UINT16  TotalVolumes;       // Total number of volumes
  UINT64  VolumeSize;         // Size of this volume
  UINT64  VolumeOffset;       // Offset in complete archive
  UINT32  CRC32;              // CRC32 of this volume
  UINT8   ArchiveUUID[16];    // Archive UUID (matches main header)
} __attribute__((packed)) ZOO64_VOLUME_HEADER;
```

## 3.4 Volume Footer

Appears at the end of volumes 1-(N-1) in multi-volume archives.

```c
typedef struct _ZOO64_VOLUME_FOOTER {
  UINT64  Magic;              // 0x564F4C554D4546 ("VOLUMEF ")
  UINT16  VolumeNumber;       // This volume number
  UINT16  NextVolumeNumber;   // Next volume number
  UINT64  BytesInVolume;      // Total bytes in this volume
  UINT32  CRC32;              // CRC32 of this volume
} __attribute__((packed)) ZOO64_VOLUME_FOOTER;
```

## 4. Compression Mode Descriptor

Describes the compression algorithm and parameters.

```c
typedef struct _ZOO64_COMPRESSION_DESC {
  UINT32  DescriptorSize;     // Size of this descriptor
  UINT32  Algorithm;          // Compression algorithm ID
  UINT32  WindowSize;         // Window size (4K - 1M)
  UINT32  BlockSize;          // Block size for seekable (power of 2)
  UINT32  Level;              // Compression level (0-9)
  UINT32  Flags;              // Algorithm-specific flags
  UINT8   Parameters[64];     // Algorithm-specific parameters
} __attribute__((packed)) ZOO64_COMPRESSION_DESC;
```

### 4.1 Compression Algorithms

```
0x0000: None (stored)
0x0001: BWT+MTF+RAD50RLE+LZ78+Range (default)
0x0002: LZ77
0x0003: LZ4
0x0004: ZSTD
0x0005: LZMA
0x0100: Custom (parameters in descriptor)
```

## 4.5 Archive YAML Metadata

The archive may contain YAML metadata that applies to the entire archive. This is optional and provides a flexible way to store arbitrary archive-level information.

### 4.5.1 YAML Metadata Block

```c
typedef struct _ZOO64_YAML_METADATA {
  UINT64  Magic;              // 0x59414D4C4D455441 ("YAMLMETA")
  UINT32  YamlSize;           // Size of YAML data in bytes
  UINT32  Flags;              // YAML metadata flags
  // Followed by YamlSize bytes of UTF-8 encoded YAML
} __attribute__((packed)) ZOO64_YAML_METADATA;
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

## 5. File Entry Format

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
  UINT64  Magic;              // 0x46494C45454E5452 ("FILEENTR")
  UINT32  HeaderSize;         // Total size of header + path
  UINT32  Flags;              // File flags
  UINT64  UncompressedSize;   // Original file size
  UINT64  CompressedSize;     // Compressed size (0 if stored)
  UINT64  DataOffset;         // Offset to file data
  UINT64  MetadataOffset;     // Offset to metadata (0 if none)
  UINT32  MetadataSize;       // Size of metadata block
  UINT16  PathLength;         // Length of UTF-8 path in bytes
  UINT16  CompressionMethod;  // Compression method for this file
  UINT32  CRC32;              // CRC32 of uncompressed data
  UINT64  SHA256[4];          // SHA-256 hash of uncompressed data
  UINT64  BirthTime;          // File birth/creation time (NTP extended format)
  UINT64  ModificationTime;   // File modification time (NTP extended format)
  UINT64  AccessTime;         // File access time (NTP extended format)
  UINT64  ChangeTime;         // File metadata change time (NTP extended format)
  UINT32  UID;                // User ID (Unix)
  UINT32  GID;                // Group ID (Unix)
  UINT32  Mode;               // File mode/permissions (Unix)
  UINT32  Attributes;         // Platform-specific attributes
} __attribute__((packed)) ZOO64_FILE_HEADER;
```

**File Timestamps**:
- **BirthTime**: File creation/birth time (when the file was first created)
- **ModificationTime**: Last data modification time (mtime)
- **AccessTime**: Last access time (atime)
- **ChangeTime**: Last metadata change time (ctime - permissions, owner, etc.)

All four timestamps use NTP extended format for maximum precision and consistency.

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
Bit 11:    Special file (device, socket, etc.)
Bit 12-15: Reserved
Bit 16-31: Platform-specific
```

### 5.3 Variable-Length UTF-8 Path

Immediately follows file header. Length specified in `PathLength` field.

```
[PathLength bytes of UTF-8 data]
```

Paths use forward slash (/) as separator, regardless of platform.

## 6. Metadata Block Format

Optional block containing extended filesystem metadata.

```c
typedef struct _ZOO64_METADATA_HEADER {
  UINT64  Magic;              // 0x4D45544144415441 ("METADATA")
  UINT32  TotalSize;          // Total size of metadata block
  UINT32  ChunkCount;         // Number of metadata chunks
} __attribute__((packed)) ZOO64_METADATA_HEADER;
```

### 6.1 Metadata Chunks

Each metadata chunk has the following format:

```c
typedef struct _ZOO64_METADATA_CHUNK {
  UINT32  ChunkType;          // Chunk type identifier
  UINT32  ChunkSize;          // Size of chunk data (excludes header)
  // Followed by chunk data
} __attribute__((packed)) ZOO64_METADATA_CHUNK;
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
```

### 6.3 Universal ACL Format

Zoo64 uses a **normalized universal ACL format** that can represent ACLs from any system while preserving full semantic information for round-trip conversion.

```c
typedef struct _ZOO64_ACL_HEADER {
  UINT32  EntryCount;         // Number of ACL entries
  UINT32  TotalSize;          // Total size of ACL data
  UINT32  Flags;              // ACL header flags
  UINT32  SourceSystem;       // Original ACL system (for optimization)
} __attribute__((packed)) ZOO64_ACL_HEADER;
```

#### 6.3.1 Universal ACL Entry

Each ACL entry uses a universal format that can represent any ACL system:

```c
typedef struct _ZOO64_ACL_ENTRY {
  UINT32  ACEType;            // Universal ACE type
  UINT32  Flags;              // Universal flags
  UINT64  Permissions;        // Universal permission bitmap (64 bits)
  UINT16  PrincipalType;      // Type of principal identifier
  UINT16  PrincipalLength;    // Length of principal identifier
  UINT32  SourceSystem;       // Original ACL system
  UINT32  SourceSpecific[4];  // System-specific metadata (16 bytes)
  // Followed by principal identifier (variable length)
} __attribute__((packed)) ZOO64_ACL_ENTRY;
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
```

**Design Rationale**: Storing the source system allows optimized round-trip conversion while the universal format enables cross-platform ACL translation.

#### 6.3.3 Universal ACE Types

```
0x00000000: ACCESS_ALLOWED      // Grant permissions
0x00000001: ACCESS_DENIED       // Deny permissions
0x00000002: SYSTEM_AUDIT        // Audit access
0x00000003: SYSTEM_ALARM        // Alarm on access
0x00000004: ACCESS_ALLOWED_OBJECT    // Object-specific allow
0x00000005: ACCESS_DENIED_OBJECT     // Object-specific deny
0x00000006: SYSTEM_AUDIT_OBJECT      // Object-specific audit
0x00000007: ACCESS_ALLOWED_CALLBACK  // Callback allow
0x00000008: ACCESS_DENIED_CALLBACK   // Callback deny
```

#### 6.3.4 Universal Flags

```
Inheritance Flags:
  0x00000001: FILE_INHERIT          // Inherit to files
  0x00000002: DIRECTORY_INHERIT     // Inherit to directories
  0x00000004: NO_PROPAGATE_INHERIT  // Don't propagate beyond immediate children
  0x00000008: INHERIT_ONLY          // ACE only for inheritance, not this object
  0x00000010: INHERITED_ACE         // This ACE was inherited

Audit Flags:
  0x00000040: SUCCESSFUL_ACCESS     // Audit successful access
  0x00000080: FAILED_ACCESS         // Audit failed access

Special Flags:
  0x00000100: IDENTIFIER_GROUP      // Principal is a group
  0x00000200: PROTECTED             // Protected from modification
  0x00000400: CRITICAL              // Critical ACE
  0x00000800: DEFAULT_ACL           // Default ACL (for new objects)
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
0x0001: USER_ID              // Numeric user ID (POSIX UID)
0x0002: GROUP_ID             // Numeric group ID (POSIX GID)
0x0003: USER_NAME            // UTF-8 username
0x0004: GROUP_NAME           // UTF-8 group name
0x0005: SID                  // Windows SID (binary)
0x0006: UUID                 // macOS UUID (128-bit)
0x0007: UIC                  // OpenVMS UIC (32-bit)
0x0008: IDENTIFIER_NAME      // OpenVMS identifier name
0x0009: NETWARE_OBJECT_ID    // Netware object ID (32-bit)
0x000A: STREETTALK_NAME      // VINES StreetTalk name (item@group@org)
0x000B: AFS_PRINCIPAL        // AFS principal name
0x000C: RACF_USER            // RACF user ID (8 chars)
0x000D: OS400_PROFILE        // OS/400 user profile (10 chars)
0x000E: AUTHORIZATION_LIST   // OS/400 authorization list name
0x000F: SPECIAL_PRINCIPAL    // Special (OWNER@, GROUP@, EVERYONE@, etc.)
```

#### 6.3.7 Special Principals

For SPECIAL_PRINCIPAL type, the principal data is a 4-byte identifier:

```
0x00000001: OWNER@           // File owner
0x00000002: GROUP@           // File group
0x00000003: EVERYONE@        // All users
0x00000004: INTERACTIVE@     // Interactive users
0x00000005: NETWORK@         // Network users
0x00000006: DIALUP@          // Dial-up users
0x00000007: BATCH@           // Batch jobs
0x00000008: ANONYMOUS@       // Anonymous users
0x00000009: AUTHENTICATED@   // Authenticated users
0x0000000A: SERVICE@         // Service accounts
0x0000000B: SYSTEM@          // System processes
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

### 6.4 Hard Link Support

Zoo64 supports hard links for both files and directories (like HFS+).

```c
typedef struct _ZOO64_HARDLINK {
  UINT64  InodeNumber;        // Inode number (for grouping hard links)
  UINT64  DeviceId;           // Device ID (for uniqueness)
  UINT16  TargetPathLength;   // Length of target path
  UINT32  Flags;              // Hard link flags
  // Followed by UTF-8 target path (first occurrence of this inode in archive)
} __attribute__((packed)) ZOO64_HARDLINK;
```

#### Hard Link Flags

```
0x00000001: DIRECTORY_HARDLINK  // Hard link to directory (HFS+, APFS)
0x00000002: CROSS_VOLUME        // Cross-volume hard link (rare)
0x00000004: PRESERVED_ON_COPY   // Preserve link on copy
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
  UINT16  TargetPathLength;   // Length of target path
  UINT32  Flags;              // Symlink flags
  // Followed by UTF-8 target path
} __attribute__((packed)) ZOO64_SYMLINK;
```

#### Symbolic Link Flags

```
0x00000001: ABSOLUTE_PATH    // Absolute path (vs relative)
0x00000002: DIRECTORY_TARGET // Target is a directory
0x00000003: BROKEN_LINK      // Target doesn't exist
0x00000004: PRESERVED_ON_COPY // Preserve symlink on copy
```

### 6.6 Extended Attributes Format

```c
typedef struct _ZOO64_XATTR {
  UINT16  NameLength;         // Length of attribute name
  UINT32  ValueLength;        // Length of attribute value
  UINT16  Namespace;          // Namespace (user, system, security, trusted)
  // Followed by:
  //   [NameLength bytes: UTF-8 name]
  //   [ValueLength bytes: binary value]
} __attribute__((packed)) ZOO64_XATTR;
0x0008: ACL_GROUP           // Named group
0x0010: ACL_MASK            // Maximum permissions
0x0020: ACL_OTHER           // Other permissions
```

#### 6.3.3 NFS4 ACL Format

NFSv4 ACLs (RFC 7530) - used on Solaris, FreeBSD, NFSv4 exports

```c
typedef struct _ZOO64_NFS4_ACL_ENTRY {
  UINT32  Type;               // ALLOW, DENY, AUDIT, ALARM
  UINT32  Flags;              // Inheritance and other flags
  UINT32  AccessMask;         // Permission bits
  UINT16  WhoType;            // OWNER@, GROUP@, EVERYONE@, or named
  UINT16  WhoLength;          // Length of who string (0 for special)
  // Followed by:
  //   [WhoLength bytes: UTF-8 username/group] (if WhoType is named)
} __attribute__((packed)) ZOO64_NFS4_ACL_ENTRY;
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
0x0001: OWNER@              // File owner
0x0002: GROUP@              // File group
0x0003: EVERYONE@           // All users
0x0004: NAMED_USER          // Specific user (WhoLength > 0)
0x0005: NAMED_GROUP         // Specific group (WhoLength > 0)
```

#### 6.3.4 NT ACL Format

Windows NT ACLs with Security Identifiers (SIDs)

```c
typedef struct _ZOO64_NT_ACL_ENTRY {
  UINT32  Type;               // ACCESS_ALLOWED, ACCESS_DENIED, AUDIT, etc.
  UINT32  Flags;              // Inheritance flags
  UINT32  AccessMask;         // Permission bits
  UINT16  SIDLength;          // Length of SID
  // Followed by:
  //   [SIDLength bytes: binary SID structure]
} __attribute__((packed)) ZOO64_NT_ACL_ENTRY;
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
typedef struct _NT_SID {
  UINT8   Revision;           // Always 1
  UINT8   SubAuthorityCount;  // Number of sub-authorities (1-15)
  UINT8   Authority[6];       // 48-bit authority value
  UINT32  SubAuthority[];     // Variable number of 32-bit values
} __attribute__((packed)) NT_SID;
```

#### 6.3.5 macOS ACL Format

macOS extended ACLs (based on NFSv4 with macOS extensions)

```c
typedef struct _ZOO64_MACOS_ACL_ENTRY {
  UINT8   UUID[16];           // User/group UUID (128-bit)
  UINT32  Type;               // ALLOW, DENY
  UINT32  Flags;              // Inheritance flags
  UINT32  Permissions;        // Permission bits
  UINT32  Reserved;           // Reserved for future use
} __attribute__((packed)) ZOO64_MACOS_ACL_ENTRY;
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
  UINT32  ACEType;            // ACE type (UIC, identifier, default)
  UINT32  AccessMask;         // Permission bits
  UINT32  Flags;              // ACE flags (protected, hidden, etc.)
  UINT16  IdentifierType;     // UIC, general identifier, or facility
  UINT16  IdentifierLength;   // Length of identifier name
  UINT32  UIC;                // User Identification Code (if UIC type)
  // Followed by identifier name (if named identifier)
} __attribute__((packed)) ZOO64_VMS_ACL_ENTRY;
```

OpenVMS ACE types:
```
0x0001: ACL$C_FILE     // File ACL
0x0002: ACL$C_KEYID    // Identifier-based
0x0003: ACL$C_ADDACC   // Access mode
0x0004: ACL$C_DEFAULT  // Default ACL
```

OpenVMS access rights:
```
0x0001: ACL$M_READ     // Read
0x0002: ACL$M_WRITE    // Write
0x0004: ACL$M_EXECUTE  // Execute
0x0008: ACL$M_DELETE   // Delete
0x0010: ACL$M_CONTROL  // Control (change ACL)
0x0020: ACL$M_EXTEND   // Extend file
0x0040: ACL$M_READ_ATTRIBUTES
0x0080: ACL$M_WRITE_ATTRIBUTES
```

#### 6.3.7 OS/400 ACL Format

OS/400 uses object authorities and authorization lists.

```c
typedef struct _ZOO64_OS400_ACL_ENTRY {
  char    UserProfile[10];    // User profile name
  UINT32  ObjectAuthority;    // Object authority bits
  UINT32  DataAuthority;      // Data authority bits
  UINT16  AuthorizationType;  // User, group, or *PUBLIC
  UINT16  AuthListLength;     // Authorization list name length
  // Followed by authorization list name (if applicable)
} __attribute__((packed)) ZOO64_OS400_ACL_ENTRY;
```

OS/400 object authorities:
```
0x00000001: *OBJMGT    // Object management
0x00000002: *OBJEXIST  // Object existence
0x00000004: *OBJALTER  // Object alter
0x00000008: *OBJREF    // Object reference
0x00000010: *OBJOPER   // Object operational
```

OS/400 data authorities:
```
0x00000001: *READ      // Read
0x00000002: *ADD       // Add
0x00000004: *UPD       // Update
0x00000008: *DLT       // Delete
0x00000010: *EXECUTE   // Execute
0x00000020: *AUTL      // Authorization list management
0x00000040: *EXCLUDE   // Exclude (deny all)
```

#### 6.3.8 MVS/RACF ACL Format

IBM MVS (z/OS) uses RACF (Resource Access Control Facility) for security.

```c
typedef struct _ZOO64_RACF_ACL_ENTRY {
  char    UserID[8];          // RACF user ID
  char    GroupID[8];         // RACF group ID
  UINT32  AccessLevel;        // Access level (NONE, READ, UPDATE, CONTROL, ALTER)
  UINT32  AccessType;         // Universal, conditional, or group
  UINT32  Flags;              // RACF flags (WARN, ERASE, etc.)
  UINT8   SecurityLevel;      // Security level (0-255)
  UINT8   SecurityCategories[16]; // Security categories bitmap
} __attribute__((packed)) ZOO64_RACF_ACL_ENTRY;
```

RACF access levels:
```
0x00: NONE     // No access
0x01: READ     // Read access
0x02: UPDATE   // Read and write
0x03: CONTROL  // Read, write, and change permissions
0x04: ALTER    // Full control including delete
```

RACF access types:
```
0x0001: UNIVERSAL   // Applies to all
0x0002: CONDITIONAL // Based on conditions
0x0004: GROUP       // Group-based
```

#### 6.3.9 Netware ACL Format

Novell Netware uses Trustee Rights and Inherited Rights Filters.

```c
typedef struct _ZOO64_NETWARE_ACL_ENTRY {
  UINT32  ObjectID;           // Netware object ID
  UINT16  ObjectType;         // User, group, or organizational role
  UINT16  TrusteeRights;      // Trustee rights bitmap
  UINT16  InheritedRightsFilter; // IRF bitmap
  char    TrusteeName[48];    // Trustee name (NDS format)
} __attribute__((packed)) ZOO64_NETWARE_ACL_ENTRY;
```

Netware trustee rights:
```
0x0001: SUPERVISOR  // [S] All rights
0x0002: READ        // [R] Read files
0x0004: WRITE       // [W] Write files
0x0008: CREATE      // [C] Create files
0x0010: ERASE       // [E] Delete files
0x0020: MODIFY      // [M] Modify file attributes
0x0040: FILESCAN    // [F] See files in directory
0x0080: ACCESSCTRL  // [A] Change trustee rights
```

#### 6.3.10 Banyan VINES ACL Format

Banyan VINES uses StreetTalk directory services for permissions.

```c
typedef struct _ZOO64_VINES_ACL_ENTRY {
  char    StreetTalkName[256]; // Full StreetTalk name (item@group@organization)
  UINT32  Rights;             // Access rights bitmap
  UINT16  EntryType;          // User, group, or list
  UINT16  Flags;              // Entry flags
} __attribute__((packed)) ZOO64_VINES_ACL_ENTRY;
```

VINES access rights:
```
0x00000001: READ        // Read data
0x00000002: WRITE       // Write data
0x00000004: EXECUTE     // Execute
0x00000008: DELETE      // Delete
0x00000010: CREATE      // Create
0x00000020: RENAME      // Rename
0x00000040: ATTRIBUTES  // Change attributes
0x00000080: SECURITY    // Change security
0x00000100: OWNER       // Ownership rights
```

#### 6.3.11 AFS ACL Format

Andrew File System (AFS) uses per-directory ACLs with specific rights.

```c
typedef struct _ZOO64_AFS_ACL_ENTRY {
  char    Principal[64];      // User or group principal
  UINT32  Rights;             // AFS rights bitmap
  UINT16  EntryType;          // Positive or negative rights
  UINT16  Reserved;           // Reserved
} __attribute__((packed)) ZOO64_AFS_ACL_ENTRY;
```

AFS rights:
```
0x01: READ     // [r] Read files
0x02: LIST     // [l] List directory
0x04: INSERT   // [i] Insert files
0x08: DELETE   // [d] Delete files
0x10: WRITE    // [w] Write files
0x20: LOCK     // [k] Lock files
0x40: ADMIN    // [a] Administer ACL
```

AFS entry types:
```
0x0001: POSITIVE   // Grant rights
0x0002: NEGATIVE   // Deny rights
```

#### 6.3.12 CODA ACL Format

CODA distributed filesystem extends AFS ACL model.

```c
typedef struct _ZOO64_CODA_ACL_ENTRY {
  char    Principal[64];      // User or group principal
  UINT32  Rights;             // CODA rights (extends AFS)
  UINT16  EntryType;          // Positive or negative
  UINT16  ReplicationPolicy;  // Replication-specific rights
} __attribute__((packed)) ZOO64_CODA_ACL_ENTRY;
```

CODA rights (extends AFS):
```
0x01: READ          // [r] Read files
0x02: LIST          // [l] List directory
0x04: INSERT        // [i] Insert files
0x08: DELETE        // [d] Delete files
0x10: WRITE         // [w] Write files
0x20: LOCK          // [k] Lock files
0x40: ADMIN         // [a] Administer ACL
0x80: REPLICATE     // Control replication
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
  UINT16  NameLength;         // Length of attribute name
  UINT32  ValueLength;        // Length of attribute value
  UINT16  Namespace;          // Namespace (user, system, security, trusted)
  // Followed by:
  //   [NameLength bytes: UTF-8 name]
  //   [ValueLength bytes: binary value]
} __attribute__((packed)) ZOO64_XATTR;
```

### 6.5 Alternate Data Streams Format

```c
typedef struct _ZOO64_ADS {
  UINT16  StreamNameLength;   // Length of stream name
  UINT64  StreamSize;         // Size of stream data
  UINT32  StreamFlags;        // Stream flags
  // Followed by:
  //   [StreamNameLength bytes: UTF-8 stream name]
  //   [StreamSize bytes: stream data]
} __attribute__((packed)) ZOO64_ADS;
```

### 6.6 File-Level YAML Metadata Format

File-level YAML metadata provides flexible, extensible metadata for individual files.

```c
typedef struct _ZOO64_FILE_YAML {
  UINT32  YamlSize;           // Size of YAML data
  UINT32  Flags;              // YAML flags (compressed, validated, etc.)
  // Followed by YamlSize bytes of UTF-8 YAML
} __attribute__((packed)) ZOO64_FILE_YAML;
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
  UINT64  BirthTime;          // File birth/creation time (NTP extended format)
  UINT64  ModificationTime;   // Data modification time (NTP extended format)
  UINT64  AccessTime;         // Last access time (NTP extended format)
  UINT64  ChangeTime;         // Metadata change time (NTP extended format)
  UINT64  BackupTime;         // Last backup time (NTP extended format)
  UINT64  ArchivedTime;       // Time archived (NTP extended format)
  UINT32  Flags;              // Timestamp flags
  UINT32  Reserved;           // Reserved for future use
} __attribute__((packed)) ZOO64_EXTENDED_TIMESTAMPS;
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
  UINT8   UserUUID[16];       // User UUID (128-bit)
  UINT8   GroupUUID[16];      // Group UUID (128-bit)
  UINT32  Flags;              // Reserved
} __attribute__((packed)) ZOO64_MACOS_UUID;
```

### 6.8 BSD Flags Format

BSD systems use file flags for immutability, append-only, etc.

```c
typedef struct _ZOO64_BSD_FLAGS {
  UINT32  UserFlags;          // User-settable flags
  UINT32  SystemFlags;        // System/super-user flags
} __attribute__((packed)) ZOO64_BSD_FLAGS;
```

#### BSD Flag Definitions

```
User Flags:
  UF_NODUMP      0x00000001  // Do not dump file
  UF_IMMUTABLE   0x00000002  // File may not be changed
  UF_APPEND      0x00000004  // Writes to file may only append
  UF_OPAQUE      0x00000008  // Directory is opaque (union)
  UF_HIDDEN      0x00008000  // File is hidden (macOS)

System Flags:
  SF_ARCHIVED    0x00010000  // File is archived
  SF_IMMUTABLE   0x00020000  // File may not be changed
  SF_APPEND      0x00040000  // Writes to file may only append
```

### 6.9 Linux Flags Format

Linux file attributes (chattr/lsattr).

```c
typedef struct _ZOO64_LINUX_FLAGS {
  UINT32  Flags;              // Linux file attributes
  UINT32  Version;            // File version (for ext2/3/4)
} __attribute__((packed)) ZOO64_LINUX_FLAGS;
```

#### Linux Flag Definitions

```
FS_SECRM_FL        0x00000001  // Secure deletion
FS_UNRM_FL         0x00000002  // Undelete
FS_COMPR_FL        0x00000004  // Compress file
FS_SYNC_FL         0x00000008  // Synchronous updates
FS_IMMUTABLE_FL    0x00000010  // Immutable file
FS_APPEND_FL       0x00000020  // Append only
FS_NODUMP_FL       0x00000040  // Do not dump file
FS_NOATIME_FL      0x00000080  // Do not update atime
FS_NOCOW_FL        0x00800000  // No copy-on-write (Btrfs)
```

### 6.10 Windows Attributes Format

Extended Windows file attributes.

```c
typedef struct _ZOO64_WINDOWS_ATTR {
  UINT32  FileAttributes;     // Windows file attributes
  UINT32  ReparseTag;         // Reparse point tag (if applicable)
  UINT32  EaSize;             // Extended attributes size
  // Followed by EA data if EaSize > 0
} __attribute__((packed)) ZOO64_WINDOWS_ATTR;
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
  UINT64  InodeNumber;        // Inode number (for grouping hard links)
  UINT64  DeviceId;           // Device ID (for uniqueness)
  UINT16  TargetPathLength;   // Length of target path
  // Followed by UTF-8 target path (first occurrence of this inode in archive)
} __attribute__((packed)) ZOO64_HARDLINK;
```

**Note**: The first file with a given inode contains the actual data. Subsequent hard links reference the first file's path and contain no data themselves.

### 6.12 Symbolic Link Target Format

Stores symbolic link target path.

```c
typedef struct _ZOO64_SYMLINK {
  UINT16  TargetPathLength;   // Length of target path
  UINT32  Flags;              // Symlink flags
  // Followed by UTF-8 target path
} __attribute__((packed)) ZOO64_SYMLINK;
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
  UINT16  RecordType;         // Record format (fixed, variable, stream, etc.)
  UINT16  RecordAttributes;   // Record attributes (Fortran CC, print, etc.)
  UINT32  RecordSize;         // Fixed record size (or max for variable)
  UINT32  FileOrganization;   // Sequential, relative, indexed
  UINT16  FileCharacteristics;// File characteristics bits
  UINT16  RecordFormatFlags;  // Additional record format flags
  UINT32  HighWaterMark;      // Highest block written
  UINT32  EndOfFileBlock;     // End-of-file block number
  UINT16  EndOfFileOffset;    // Byte offset in EOF block
  UINT16  FileVersion;        // File version number (;1, ;2, etc.)
  UINT32  UserPrivileges;     // User privilege mask
  char    FileID[16];         // Volume-unique file identifier (FID)
} __attribute__((packed)) ZOO64_ODS5_ATTR;
```

#### ODS-5 Record Types

```
0x0001: RFM_UDF  // Undefined (stream)
0x0002: RFM_FIX  // Fixed-length records
0x0003: RFM_VAR  // Variable-length records
0x0004: RFM_VFC  // Variable with fixed control
0x0005: RFM_STM  // Stream (LF-terminated)
0x0006: RFM_STMLF // Stream LF
0x0007: RFM_STMCR // Stream CR
```

#### ODS-5 File Organization

```
0x0001: FAB$C_SEQ  // Sequential
0x0002: FAB$C_REL  // Relative
0x0003: FAB$C_IDX  // Indexed (ISAM)
0x0004: FAB$C_HSH  // Hashed
```

#### ODS-5 File Characteristics

```
0x0001: FCH$V_NOBACKUP    // Don't backup
0x0002: FCH$V_WRITECHECK  // Verify all writes
0x0004: FCH$V_READCHECK   // Verify all reads
0x0008: FCH$V_CONTIG      // Contiguous allocation
0x0010: FCH$V_LOCKED      // File locked
0x0020: FCH$V_CONTIGB     // Contiguous best try
0x0040: FCH$V_SPOOL       // Spool file (intermediate)
0x0080: FCH$V_DIRECTORY   // Directory file
0x0100: FCH$V_BADBLOCK    // Bad block processing
0x0200: FCH$V_MARKDEL     // Mark for delete
0x0400: FCH$V_NOCHARGE    // Don't charge quota
0x0800: FCH$V_ERASE       // Erase on delete
```

### 6.14 z/OS Dataset Attributes

IBM z/OS (formerly OS/390, MVS) mainframe filesystem metadata for datasets.

```c
typedef struct _ZOO64_ZOS_ATTR {
  char    DatasetName[44];    // Fully qualified dataset name (DSNAME)
  UINT8   DatasetOrganization;// DSORG (PS, PO, DA, IS, VS)
  UINT8   RecordFormat;       // RECFM (F, FB, V, VB, U, etc.)
  UINT16  LogicalRecordLength;// LRECL
  UINT32  BlockSize;          // BLKSIZE
  UINT16  PrimarySpace;       // Primary space allocation (tracks/cylinders/blocks)
  UINT16  SecondarySpace;     // Secondary space allocation
  UINT8   SpaceUnit;          // Space unit (tracks, cylinders, blocks, etc.)
  UINT8   DirectoryBlocks;    // Directory blocks (for PDS)
  char    DataClass[8];       // SMS data class
  char    StorageClass[8];    // SMS storage class
  char    ManagementClass[8]; // SMS management class
  char    VolSer[6];          // Volume serial number
  UINT16  DatasetType;        // PDS, PDSE, HFS, zFS
  UINT32  Flags;              // Various dataset flags
} __attribute__((packed)) ZOO64_ZOS_ATTR;
```

#### z/OS Dataset Organization (DSORG)

```
0x01: PS   // Physical Sequential
0x02: PO   // Partitioned (PDS)
0x04: DA   // Direct Access
0x08: IS   // Indexed Sequential (ISAM)
0x10: VS   // VSAM
0x20: PSU  // Unmovable PS
0x40: POU  // Unmovable PO
```

#### z/OS Record Format (RECFM)

```
0x01: F    // Fixed
0x02: FB   // Fixed Blocked
0x03: V    // Variable
0x04: VB   // Variable Blocked
0x05: U    // Undefined
0x06: FBA  // Fixed Blocked ASCII
0x07: VBA  // Variable Blocked ASCII
0x08: FBM  // Fixed Blocked Machine code
0x09: VBM  // Variable Blocked Machine code
```

#### z/OS Space Units

```
0x01: TRK  // Tracks
0x02: CYL  // Cylinders
0x03: BLK  // Blocks
0x04: KB   // Kilobytes
0x05: MB   // Megabytes
```

#### z/OS Dataset Types

```
0x0001: PDS   // Partitioned Dataset
0x0002: PDSE  // Partitioned Dataset Extended
0x0004: HFS   // Hierarchical File System
0x0008: ZFS   // zSeries File System
0x0010: VSAM  // Virtual Storage Access Method
0x0020: SEQ   // Sequential
```

### 6.15 OS/400 (IBM i) Attributes

IBM i (formerly OS/400, AS/400) integrated filesystem metadata.

```c
typedef struct _ZOO64_OS400_ATTR {
  char    Library[10];        // Library name
  char    Object[10];         // Object name
  char    Member[10];         // Member name (for source/data files)
  char    ObjectType[10];     // Object type (*FILE, *PGM, *DTAARA, etc.)
  char    SourceType[10];     // Source type (RPG, CLP, DSPF, etc.)
  UINT32  CCSID;              // Coded Character Set ID
  UINT8   FileType;           // Physical, logical, source, data
  UINT8   FileOrganization;   // Sequential, keyed, stream
  UINT16  RecordLength;       // Record length
  UINT32  MemberCount;        // Number of members (for multi-member files)
  char    TextDescription[50];// Object text description
  UINT64  CreateTimestamp;    // Create timestamp (NTP extended format)
  UINT64  ChangeTimestamp;    // Last change timestamp (NTP extended format)
  char    OwnerProfile[10];   // Owner user profile
  char    Authority[10];      // Primary group authority
  UINT32  Flags;              // Various OS/400 flags
} __attribute__((packed)) ZOO64_OS400_ATTR;
```

#### OS/400 Object Types (common)

```
*FILE      // Database file
*PGM       // Program
*DTAARA    // Data area
*DTAQ      // Data queue
*USRPRF    // User profile
*MSGQ      // Message queue
*OUTQ      // Output queue
*JOBQ      // Job queue
*LIB       // Library
*CMD       // Command
*MENU      // Menu
*PNLGRP    // Panel group
*QRYDFN    // Query definition
```

#### OS/400 File Types

```
0x01: PF   // Physical file
0x02: LF   // Logical file
0x03: DSPF // Display file
0x04: PRTF // Printer file
0x05: SAVF // Save file
0x06: TAPF // Tape file
0x07: SRC  // Source physical file
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
  char    DocumentType[32];   // Document type (LisaWrite, LisaCalc, etc.)
  UINT32  PrivateData[4];     // Application private data
  char    DocumentName[32];   // Original document name (Lisa limit: 31 chars)
  UINT32  PaperType;          // Paper size (US Letter, A4, Legal, etc.)
  UINT16  Version;            // Document version number
  UINT16  Edition;            // Document edition number
  UINT32  IconResourceID;     // Resource ID of document icon
  UINT32  Password;           // Password hash (if password protected)
  UINT32  Flags;              // Lisa-specific flags
  UINT64  CreationDate;       // Lisa creation date (NTP extended format)
  UINT64  ModificationDate;   // Lisa modification date (NTP extended format)
  char    Application[32];    // Creating application name
  char    StationeryPad[32];  // Stationery pad template (if any)
} __attribute__((packed)) ZOO64_LISA_ATTR;
```

#### Lisa Document Types (common)

```
"LisaWrite/DOCUMENT"   // LisaWrite word processor document
"LisaCalc/WORKSHEET"   // LisaCalc spreadsheet
"LisaDraw/DRAWING"     // LisaDraw graphics document
"LisaGraph/GRAPH"      // LisaGraph business graphics
"LisaProject/PROJECT"  // LisaProject project management
"LisaList/DATABASE"    // LisaList database
"LisaTerminal/SETUP"   // LisaTerminal configuration
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
0x00000001: STATIONERY        // Document is stationery
0x00000002: PASSWORD_PROTECTED // Password required
0x00000004: COPY_PROTECTED     // Copy protection enabled
0x00000008: PRINT_PROTECTED    // Print protection enabled
0x00000010: SHARED_DOCUMENT    // Multi-user shared document
0x00000020: AUTO_SAVE          // Auto-save enabled
```

**Note**: Lisa Office System metadata is primarily of historical interest for archival purposes. Modern implementations should store Lisa documents with full metadata preservation for digital archaeology and computing history research.

### 6.17 UNIVAC 2200 Attributes

UNIVAC 2200 series (1100/2200) mainframe filesystem metadata.

```c
typedef struct _ZOO64_UNIVAC_ATTR {
  char    FileName[12];       // File name (12 characters max)
  char    Qualifier[12];      // File qualifier
  char    ProjectID[12];      // Project identifier
  UINT16  FileType;           // File type (program, data, etc.)
  UINT16  FileOrganization;   // Sequential, random, indexed
  UINT32  GranuleSize;        // Granule size (allocation unit)
  UINT32  MaxGranules;        // Maximum granules allocated
  UINT32  HighGranule;        // Highest granule used
  UINT16  RecordSize;         // Logical record size (words)
  UINT8   WordSize;           // Word size (36 or 9-bit bytes)
  UINT8   FileClass;          // Removable, cataloged, etc.
  UINT32  CatalogInfo;        // Catalog information
  UINT16  SecurityLevel;      // Security classification
  UINT32  Flags;              // File characteristic flags
} __attribute__((packed)) ZOO64_UNIVAC_ATTR;
```

UNIVAC file types:
```
0x01: PROGRAM    // Executable program
0x02: DATA       // Data file
0x03: LIBRARY    // Object library
0x04: SOURCE     // Source code
0x05: PRINT      // Print file
0x06: PUNCH      // Card punch format
```

UNIVAC file organization:
```
0x01: SEQUENTIAL // Sequential access
0x02: RANDOM     // Random access
0x03: INDEXED    // Indexed sequential
```

### 6.18 PDP-10 Attributes

PDP-10 systems (TENEX, ITS, TOPS-10, TOPS-20) filesystem metadata.

```c
typedef struct _ZOO64_PDP10_ATTR {
  char    FileName[40];       // File name (6-char name + extension)
  UINT32  ProtectionCode;     // Protection code (octal format)
  UINT16  AccountNumber;      // Account number
  char    Author[40];         // Author/creator name
  UINT64  CreationDate;       // Creation date (NTP extended format)
  UINT64  WriteDate;          // Last write date (NTP extended format)
  UINT64  ReadDate;           // Last read date (NTP extended format)
  UINT32  ByteSize;           // Byte size (7, 8, 9, 18, 36 bits)
  UINT32  PageCount;          // Number of pages (512-word pages)
  UINT16  FileMode;           // File mode (ASCII, binary, etc.)
  UINT16  System;             // System type (TENEX, ITS, TOPS-10, TOPS-20)
  UINT32  Generation;         // Generation number (version)
  UINT32  Flags;              // System-specific flags
} __attribute__((packed)) ZOO64_PDP10_ATTR;
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
0x01: TENEX      // TENEX operating system
0x02: ITS        // Incompatible Timesharing System
0x03: TOPS10     // TOPS-10
0x04: TOPS20     // TOPS-20
```

### 6.19 Classic Mac OS Attributes

Classic Macintosh System (System 1-9, pre-OS X) filesystem metadata.

```c
typedef struct _ZOO64_CLASSIC_MAC_ATTR {
  char    TypeCode[4];        // File type code (e.g., 'TEXT', 'APPL')
  char    CreatorCode[4];     // Creator application code
  UINT16  FinderFlags;        // Finder flags
  INT16   IconPositionV;      // Vertical icon position
  INT16   IconPositionH;      // Horizontal icon position
  UINT16  FolderID;           // Folder ID
  UINT32  LabelColor;         // Label color (0-7)
  UINT16  ScriptCode;         // Script code for name
  UINT16  ExtendedFinderFlags;// Extended Finder flags
  INT32   CommentID;          // Comment ID (-1 if none)
  UINT32  PutAwayFolderID;    // "Put Away" folder ID
  UINT16  IconID;             // Custom icon ID
  UINT8   VersionMajor;       // File version (major)
  UINT8   VersionMinor;       // File version (minor)
} __attribute__((packed)) ZOO64_CLASSIC_MAC_ATTR;
```

Finder flags:
```
0x0001: IS_ON_DESK       // File is on desktop
0x0002: COLOR            // Color (not B&W) icon
0x0004: REQUIRES_SWITCH_LAUNCH // Requires mode switch
0x0008: IS_SHARED        // Multiple users can run simultaneously
0x0010: HAS_NO_INITS     // Has no INIT resources
0x0020: HAS_BEEN_INITED  // Has been initialized
0x0040: HAS_CUSTOM_ICON  // Has custom icon
0x0080: IS_STATIONERY    // Is stationery pad
0x0100: NAME_LOCKED      // Name is locked
0x0200: HAS_BUNDLE       // Has BNDL resource
0x0400: IS_INVISIBLE     // Invisible to Finder
0x0800: IS_ALIAS         // Is an alias file
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
  char    Comment[80];        // File comment
  UINT32  ProtectionBits;     // Protection bits (DEWD ARWED)
  UINT32  DaysFromEpoch;      // Days since 1978-01-01
  UINT32  Minutes;            // Minutes since midnight
  UINT32  Ticks;              // Ticks (1/50 second)
  UINT16  FileType;           // File, directory, link
  UINT32  ScriptBits;         // Script execution bits
  UINT32  Flags;              // Additional flags
} __attribute__((packed)) ZOO64_AMIGA_ATTR;
```

Amiga protection bits (inverted logic - bit SET means permission DENIED):
```
0x00000001: DELETE      // [D] File deletable
0x00000002: EXECUTE     // [E] File executable
0x00000004: WRITE       // [W] File writable
0x00000008: READ        // [R] File readable
0x00000010: ARCHIVE     // [A] Archive bit
0x00000020: PURE        // [P] Re-entrant/pure
0x00000040: SCRIPT      // [S] Script file
0x00000080: HOLD        // [H] Hold (for multi-user)
```

### 6.21 Atari TOS/GEM Attributes

Atari ST/TT/Falcon TOS and GEM filesystem metadata.

```c
typedef struct _ZOO64_ATARI_ATTR {
  UINT8   Attributes;         // File attributes
  UINT16  Time;               // MS-DOS format time
  UINT16  Date;               // MS-DOS format date
  UINT32  StartCluster;       // Starting cluster
  char    GEMType[4];         // GEM file type
  UINT16  GEMIcon;            // GEM icon number
  UINT32  Flags;              // TOS flags
} __attribute__((packed)) ZOO64_ATARI_ATTR;
```

Atari file attributes:
```
0x01: READ_ONLY   // Read-only file
0x02: HIDDEN      // Hidden file
0x04: SYSTEM      // System file
0x08: VOLUME      // Volume label
0x10: DIRECTORY   // Directory
0x20: ARCHIVE     // Archive bit
```

### 6.22 Acorn RISC OS Attributes

Acorn Archimedes RISC OS filesystem metadata.

```c
typedef struct _ZOO64_RISC_OS_ATTR {
  UINT32  LoadAddress;        // Load address (or filetype if bit 12 set)
  UINT32  ExecAddress;        // Execution address (or timestamp)
  UINT32  Attributes;         // File attributes
  UINT16  FileType;           // File type (12-bit, 0xFFF = untyped)
  UINT64  Timestamp;          // RISC OS timestamp (centiseconds since 1900)
  char    SpriteName[12];     // Associated sprite name
} __attribute__((packed)) ZOO64_RISC_OS_ATTR;
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
  char    PETSCIIName[16];    // PETSCII filename
  UINT8   FileType;           // File type (PRG, SEQ, USR, REL)
  UINT8   RecordLength;       // Record length (REL files)
  UINT16  StartAddress;       // Load address (PRG files)
  UINT16  BlocksUsed;         // Number of blocks used
  UINT8   DriveNumber;        // Drive number (0-1)
  UINT8   TrackSector[2];     // Track and sector of first block
  UINT8   Flags;              // File flags (locked, etc.)
  UINT8   GEOSType;           // GEOS file type (if GEOS)
  UINT8   GEOSStructure;      // GEOS file structure
} __attribute__((packed)) ZOO64_C64_ATTR;
```

C64 file types:
```
0x00: DEL  // Deleted (scratched)
0x01: SEQ  // Sequential
0x02: PRG  // Program
0x03: USR  // User
0x04: REL  // Relative (random access)
```

C64 file flags:
```
0x40: LOCKED     // File is write-protected
0x80: CLOSED     // File properly closed
```

### 6.24 Apple IIGS ProDOS Attributes

Apple IIGS ProDOS 16 and GS/OS filesystem metadata.

```c
typedef struct _ZOO64_PRODOS_ATTR {
  UINT8   FileType;           // ProDOS file type
  UINT16  AuxType;            // Auxiliary type
  UINT8   Access;             // Access flags
  UINT16  StorageType;        // Storage type
  UINT64  CreateTime;         // ProDOS timestamp (NTP format)
  UINT64  ModTime;            // ProDOS timestamp (NTP format)
  UINT32  BlocksUsed;         // Blocks used
  UINT16  VersionCreated;     // ProDOS version that created file
  UINT16  MinVersion;         // Minimum ProDOS version required
  UINT32  EndOfFile;          // EOF marker
  UINT32  OptionList;         // GS/OS option list
} __attribute__((packed)) ZOO64_PRODOS_ATTR;
```

ProDOS file types (common):
```
0x00: UNK  // Unknown
0x04: TXT  // Text file
0x06: BIN  // Binary
0x0F: DIR  // Directory
0x19: ADB  // AppleWorks database
0x1A: AWP  // AppleWorks word processor
0x1B: ASP  // AppleWorks spreadsheet
0x2E: PRG  // ProDOS application
0xB0: SRC  // Source code
0xEF: PAS  // Pascal
0xF0: CMD  // Command file
0xFA: INT  // Integer BASIC
0xFC: BAS  // Applesoft BASIC
0xFF: SYS  // System file
```

ProDOS access flags:
```
0x01: READ      // Read enable
0x02: WRITE     // Write enable
0x04: INVISIBLE // Invisible file
0x20: BACKUP    // Backup needed
0x40: RENAME    // Rename enable
0x80: DESTROY   // Delete enable
```

### 6.25 Stratus VOS Attributes

Stratus VOS (Virtual Operating System) fault-tolerant system metadata.

```c
typedef struct _ZOO64_STRATUS_ATTR {
  char    ModuleName[32];     // Module name
  char    Organization[32];   // Organization name
  UINT32  FileOrganization;   // Sequential, relative, indexed
  UINT16  RecordSize;         // Fixed record size
  UINT16  KeySize;            // Key size (indexed files)
  UINT32  MaxRecords;         // Maximum records
  UINT32  ReplicationLevel;   // Replication level (0-3)
  UINT32  IntegrityLevel;     // Integrity level
  char    UserID[32];         // Owner user ID
  char    GroupID[32];        // Owner group ID
  UINT32  AccessControl;      // Access control flags
  UINT32  Flags;              // VOS-specific flags
} __attribute__((packed)) ZOO64_STRATUS_ATTR;
```

Stratus replication levels:
```
0: None         // No replication
1: Disk         // Disk mirroring
2: CPU          // CPU pair replication
3: Full         // Full fault tolerance
```

### 6.26 Netware Extended Attributes

Novell Netware extended filesystem metadata (beyond ACLs).

```c
typedef struct _ZOO64_NETWARE_ATTR {
  UINT32  OwnerID;            // Owner object ID
  UINT64  ArchiveTime;        // Last archive time (NTP format)
  UINT64  ArchiverID;         // Archiver object ID
  UINT64  UpdateTime;         // Last update time (NTP format)
  UINT64  UpdaterID;          // Updater object ID
  UINT32  Attributes;         // File attributes
  UINT16  InheritedRightsFilter; // IRF
  UINT32  MaxSpace;           // Maximum space
  char    PrimaryNameSpace[16]; // Primary namespace (DOS, MAC, NFS, etc.)
  char    NDSPath[256];       // NDS (Novell Directory Services) path
  UINT32  Flags;              // Extended flags
} __attribute__((packed)) ZOO64_NETWARE_ATTR;
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
  char    ServiceType[32];    // Service type
  UINT32  VINESVersion;       // VINES OS version
  char    Organization[64];   // Organization name
  char    Group[64];          // Group name
  char    Item[64];           // Item name
  UINT32  Attributes;         // File attributes
  UINT64  ReplicationStatus;  // Replication status
  char    PrimaryServer[64];  // Primary file server
  UINT32  Flags;              // VINES flags
} __attribute__((packed)) ZOO64_VINES_ATTR;
```

### 6.28 AFS (Andrew File System) Attributes

AFS distributed filesystem metadata.

```c
typedef struct _ZOO64_AFS_ATTR {
  char    CellName[256];      // AFS cell name
  UINT32  VolumeID;           // Volume ID
  UINT32  Vnode;              // Vnode number
  UINT32  Uniquifier;         // Uniquifier
  UINT32  DataVersion;        // Data version number
  char    VolumeName[64];     // Volume name
  UINT32  QuotaUsed;          // Quota used
  UINT32  QuotaLimit;         // Quota limit
  UINT32  Author;             // Author UID
  UINT32  Owner;              // Owner UID
  UINT64  ServerModTime;      // Server modification time (NTP format)
  UINT32  Callbacks;          // Callback information
  UINT32  Flags;              // AFS flags
} __attribute__((packed)) ZOO64_AFS_ATTR;
```

### 6.29 CODA Distributed Filesystem Attributes

CODA filesystem metadata (extends AFS).

```c
typedef struct _ZOO64_CODA_ATTR {
  char    Realm[256];         // CODA realm name
  UINT32  VolumeID;           // Volume ID
  UINT32  Vnode;              // Vnode number
  UINT32  Uniquifier;         // Uniquifier
  UINT64  DataVersion;        // Data version (64-bit)
  char    VolumeName[64];     // Volume name
  UINT32  ReplicationFactor;  // Replication factor
  UINT32  ReplicaCount;       // Number of replicas
  char    ReplicaServers[256]; // Comma-separated replica servers
  UINT32  ConflictStatus;     // Conflict resolution status
  UINT64  VectorVersion[8];   // Version vector (for conflicts)
  UINT32  CachePriority;      // Client cache priority
  UINT32  Flags;              // CODA flags
} __attribute__((packed)) ZOO64_CODA_ATTR;
```

### 6.30 GFS (Global File System) Attributes

Clustered filesystem metadata.

```c
typedef struct _ZOO64_GFS_ATTR {
  char    ClusterName[64];    // Cluster name
  UINT32  NodeID;             // Node ID
  UINT64  GlockNumber;        // Global lock number
  UINT32  LockState;          // Lock state
  char    JournalID[32];      // Journal identifier
  UINT64  SequenceNumber;     // Sequence number
  UINT32  ReplicationLevel;   // Replication level
  UINT32  StripingFactor;     // Striping factor
  UINT32  BlockAllocation;    // Block allocation policy
  UINT64  ExtentSize;         // Extent size
  UINT32  Flags;              // GFS flags
} __attribute__((packed)) ZOO64_GFS_ATTR;
```

### 6.31 DFS (Distributed File System) Attributes

General distributed filesystem metadata.

```c
typedef struct _ZOO64_DFS_ATTR {
  char    Namespace[256];     // DFS namespace
  char    ServerPath[512];    // Server UNC path
  char    LinkTarget[512];    // DFS link target
  UINT32  Timeout;            // Client cache timeout (seconds)
  UINT32  ReferralTTL;        // Referral time-to-live
  UINT16  TargetPriority;     // Target priority
  UINT16  TargetRank;         // Target ranking
  char    SiteName[64];       // AD site name
  UINT32  LinkState;          // Link state (online, offline)
  UINT32  Flags;              // DFS flags
} __attribute__((packed)) ZOO64_DFS_ATTR;
```

## 6a. Encryption Support

Zoo64 supports both archive-level and per-file encryption using modern authenticated encryption algorithms.

### 6a.1 Encryption Header

Appears before encrypted file data (per-file encryption) or after compression descriptor (archive-level encryption).

```c
typedef struct _ZOO64_ENCRYPTION_HEADER {
  UINT64  Magic;              // 0x454E4352595054 ("ENCRYPT ")
  UINT32  HeaderSize;         // Size of this header including all fields
  UINT16  EncryptionMethod;   // Encryption algorithm
  UINT16  KeyDerivation;      // Key derivation function
  UINT32  Iterations;         // KDF iteration count
  UINT16  SaltLength;         // Length of salt
  UINT16  IVLength;           // Length of IV/nonce
  UINT32  TagLength;          // Length of authentication tag
  UINT32  EncryptedSize;      // Size of encrypted data
  UINT32  Flags;              // Encryption flags
  // Followed by:
  //   [SaltLength bytes: random salt for KDF]
  //   [IVLength bytes: initialization vector/nonce]
  //   [Encrypted data]
  //   [TagLength bytes: authentication tag]
} __attribute__((packed)) ZOO64_ENCRYPTION_HEADER;
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
  UINT32  VerifyMethod;       // Verification method
  UINT16  VerifyDataLength;   // Length of verification data
  // Followed by verification data
} __attribute__((packed)) ZOO64_PASSWORD_VERIFY;
```

Verification methods:
```
0x0001: Encrypted known plaintext (16 bytes zeros encrypted)
0x0002: HMAC of password + salt
0x0003: Argon2 hash of password
```

## 7. Seekable Compression

### 7.1 Block Table Format

For seekable compression, a block table precedes the compressed data.

```c
typedef struct _ZOO64_BLOCK_TABLE {
  UINT32  Magic;              // 0x424C4B54424C ("BLKTBL")
  UINT32  BlockCount;         // Number of blocks
  UINT32  BlockSize;          // Uncompressed block size (power of 2)
  UINT32  Flags;              // Block table flags
  // Followed by BlockCount entries of block offsets in LEB128 format
} __attribute__((packed)) ZOO64_BLOCK_TABLE;
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
  UINT32  UncompressedSize;   // Size before compression
  UINT32  CompressedSize;     // Size after compression
  UINT32  CRC32;              // CRC32 of uncompressed block
} __attribute__((packed)) ZOO64_BLOCK_HEADER;
```

## 8. Solid Compression

### 8.1 Solid Block Format

```c
typedef struct _ZOO64_SOLID_BLOCK {
  UINT64  Magic;              // 0x534F4C4944424C4B ("SOLIDBLK")
  UINT32  FileCount;          // Number of files in solid block
  UINT64  UncompressedSize;   // Total uncompressed size
  UINT64  CompressedSize;     // Total compressed size
  UINT32  WindowSize;         // Compression window size
  UINT32  Flags;              // Solid block flags
  // Followed by:
  //   [File offsets table] - LEB128 encoded offsets
  //   [Compressed data]
} __attribute__((packed)) ZOO64_SOLID_BLOCK;
```

### 8.2 Solid Seekable Format

Combines solid compression with block table for random access:

```
[Solid Block Header]
[File Offsets Table] (LEB128)
[Block Table] (LEB128 block offsets)
[Compressed Blocks...]
```

## 9. Digital Signatures

### 9.1 Signature Block

```c
typedef struct _ZOO64_SIGNATURE {
  UINT64  Magic;              // 0x5349474E41545552 ("SIGNATUR")
  UINT32  SignatureSize;      // Total size of signature block
  UINT16  SignatureType;      // Signature algorithm
  UINT16  HashAlgorithm;      // Hash algorithm used
  UINT64  SigningTime;        // Signing timestamp (NTP extended format)
  UINT32  CertificateSize;    // Size of certificate (0 if none)
  UINT32  SignatureDataSize;  // Size of signature data
  // Followed by:
  //   [CertificateSize bytes: X.509 certificate]
  //   [SignatureDataSize bytes: signature data]
} __attribute__((packed)) ZOO64_SIGNATURE;
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

## 10. Central Directory

The central directory provides fast access to all files in the archive.

```c
typedef struct _ZOO64_CENTRAL_DIR {
  UINT64  Magic;              // 0x43454E5444495220 ("CENTDIR ")
  UINT32  EntryCount;         // Number of entries
  UINT64  DirectorySize;      // Size of central directory
  UINT32  Flags;              // Directory flags
  // Followed by EntryCount directory entries
} __attribute__((packed)) ZOO64_CENTRAL_DIR;
```

### 10.1 Central Directory Entry

```c
typedef struct _ZOO64_CENTRAL_ENTRY {
  UINT64  FileHeaderOffset;   // Offset to file header
  UINT64  UncompressedSize;   // File size
  UINT64  CompressedSize;     // Compressed size (0 if stored)
  UINT32  CRC32;              // CRC32 of file
  UINT16  PathLength;         // Length of path
  UINT16  Flags;              // Entry flags
  // Followed by UTF-8 path
} __attribute__((packed)) ZOO64_CENTRAL_ENTRY;
```

## 11. End of Archive Marker

```c
typedef struct _ZOO64_END_OF_ARCHIVE {
  UINT64  Magic;              // 0x454E444F46415243 ("ENDOFARC")
  UINT64  ArchiveSize;        // Total archive size
  UINT64  CentralDirOffset;   // Offset to central directory
  UINT32  FileCount;          // Total files in archive
  UINT32  CRC32;              // CRC32 of central directory
  UINT8   SHA256[32];         // SHA-256 of entire archive (excl. this block)
} __attribute__((packed)) ZOO64_END_OF_ARCHIVE;
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
3. **RAD50RLE**: RAD-50 encoding + RLE + LEB128 + bit transposition
4. **Windowed LZ78**: Dictionary compression with configurable window
5. **Range Encoding**: Adaptive arithmetic encoding

### 12.3 Window Sizes

Supported window sizes (power of 2):
- 4K, 8K, 16K, 32K, 64K (default), 128K, 256K, 512K, 1M

### 12.4 Block Sizes for Seekable

Supported block sizes (power of 2):
- 4K, 8K, 16K, 32K, 64K, 128K (default), 256K, 512K, 1M, 2M, 4M

## 13. COM Component Interface

Zoo64 is designed as a COM component for cross-language interoperability.

### 13.1 IZoo64Archive Interface

```cpp
interface IZoo64Archive : IUnknown {
  // Archive operations
  HRESULT Open([in] BSTR path, [in] DWORD mode);
  HRESULT Create([in] BSTR path, [in] ZOO64_ARCHIVE_OPTIONS* options);
  HRESULT Close();

  // File operations
  HRESULT AddFile([in] BSTR sourcePath, [in] BSTR archivePath,
                  [in] ZOO64_FILE_OPTIONS* options);
  HRESULT ExtractFile([in] BSTR archivePath, [in] BSTR destPath);
  HRESULT RemoveFile([in] BSTR archivePath);

  // Enumeration
  HRESULT GetFileCount([out] DWORD* count);
  HRESULT GetFileInfo([in] DWORD index, [out] ZOO64_FILE_INFO* info);

  // Compression
  HRESULT SetCompressionMode([in] DWORD mode);
  HRESULT SetWindowSize([in] DWORD size);
  HRESULT SetBlockSize([in] DWORD size);

  // Signatures
  HRESULT SignArchive([in] IZoo64SigningKey* key);
  HRESULT VerifyArchive([out] BOOL* valid);
  HRESULT SignFile([in] BSTR archivePath, [in] IZoo64SigningKey* key);
  HRESULT VerifyFile([in] BSTR archivePath, [out] BOOL* valid);

  // Metadata
  HRESULT GetFileMetadata([in] BSTR archivePath,
                          [out] IZoo64Metadata** metadata);
  HRESULT SetFileMetadata([in] BSTR archivePath,
                          [in] IZoo64Metadata* metadata);

  // YAML Metadata
  HRESULT GetArchiveYaml([out] BSTR* yaml);
  HRESULT SetArchiveYaml([in] BSTR yaml);
  HRESULT GetFileYaml([in] BSTR archivePath, [out] BSTR* yaml);
  HRESULT SetFileYaml([in] BSTR archivePath, [in] BSTR yaml);

  // Encryption
  HRESULT SetPassword([in] BSTR password);
  HRESULT SetKeyFile([in] BSTR keyFilePath);
  HRESULT SetEncryptionMethod([in] WORD method, [in] WORD kdf);
  HRESULT EncryptArchive([in] BOOL headerEncryption);
  HRESULT EncryptFile([in] BSTR archivePath);
  HRESULT DecryptFile([in] BSTR archivePath, [in] BSTR password);
  HRESULT VerifyPassword([out] BOOL* valid);

  // Multi-Volume
  HRESULT SetVolumeSize([in] UINT64 sizeBytes);
  HRESULT GetVolumeCount([out] DWORD* count);
  HRESULT GetVolumeInfo([in] DWORD volumeNumber, [out] ZOO64_VOLUME_INFO* info);
  HRESULT MergeVolumes([in] BSTR outputPath);

  // Classic Zoo Compatibility
  HRESULT ConvertFromClassicZoo([in] BSTR classicZooPath, [in] BSTR outputPath);
  HRESULT ConvertToClassicZoo([in] BSTR outputPath, [in] BOOL dualFormat);
  HRESULT IsClassicZoo([in] BSTR path, [out] BOOL* isClassic);
};
```

### 13.2 IZoo64Metadata Interface

```cpp
interface IZoo64Metadata : IUnknown {
  // ACLs
  HRESULT GetACL([out] IZoo64ACL** acl);
  HRESULT SetACL([in] IZoo64ACL* acl);

  // Extended Attributes
  HRESULT GetXattr([in] BSTR name, [out] VARIANT* value);
  HRESULT SetXattr([in] BSTR name, [in] VARIANT value);
  HRESULT EnumXattrs([out] IEnumVARIANT** enumerator);

  // Alternate Data Streams
  HRESULT GetADS([in] BSTR streamName, [out] IStream** stream);
  HRESULT SetADS([in] BSTR streamName, [in] IStream* stream);
  HRESULT EnumADS([out] IEnumVARIANT** enumerator);

  // Timestamps
  HRESULT GetTimestamps([out] ZOO64_TIMESTAMPS* timestamps);
  HRESULT SetTimestamps([in] ZOO64_TIMESTAMPS* timestamps);

  // Permissions
  HRESULT GetPermissions([out] DWORD* mode);
  HRESULT SetPermissions([in] DWORD mode);

  // YAML Metadata
  HRESULT GetYaml([out] BSTR* yaml);
  HRESULT SetYaml([in] BSTR yaml);

  // Platform-specific
  HRESULT GetMacOSUUIDs([out] ZOO64_MACOS_UUID* uuids);
  HRESULT SetMacOSUUIDs([in] ZOO64_MACOS_UUID* uuids);
  HRESULT GetBSDFlags([out] ZOO64_BSD_FLAGS* flags);
  HRESULT SetBSDFlags([in] ZOO64_BSD_FLAGS* flags);
  HRESULT GetLinuxFlags([out] ZOO64_LINUX_FLAGS* flags);
  HRESULT SetLinuxFlags([in] ZOO64_LINUX_FLAGS* flags);
  HRESULT GetWindowsAttributes([out] ZOO64_WINDOWS_ATTR* attr);
  HRESULT SetWindowsAttributes([in] ZOO64_WINDOWS_ATTR* attr);

  // Links
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

## 15. Error Handling

### 15.1 Error Codes

```
ZOO64_OK                    = 0x00000000
ZOO64_E_INVALID_MAGIC       = 0x80040001
ZOO64_E_UNSUPPORTED_VERSION = 0x80040002
ZOO64_E_CORRUPT_HEADER      = 0x80040003
ZOO64_E_CORRUPT_DATA        = 0x80040004
ZOO64_E_DECOMPRESSION_ERROR = 0x80040005
ZOO64_E_SIGNATURE_INVALID   = 0x80040006
ZOO64_E_ENCRYPTION_ERROR    = 0x80040007
ZOO64_E_FILE_NOT_FOUND      = 0x80040008
ZOO64_E_INVALID_PATH        = 0x80040009
ZOO64_E_ACCESS_DENIED       = 0x8004000A
```

## 16. Implementation Notes

### 16.1 Byte Order

All multi-byte integers are stored in **little-endian** format.

### 16.2 Alignment

Structures are packed (no padding). Use `__attribute__((packed))` or `#pragma pack(1)`.

### 16.3 String Encoding

All paths and text metadata use **UTF-8** encoding.

### 16.4 Checksums

- CRC32 uses IEEE polynomial (0xEDB88320)
- SHA-256 for file integrity
- Additional hash in signature blocks

### 16.5 Compression Reset Points

For seekable compression, compressor state is reset at each block boundary to enable independent decompression.

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
typedef struct _CLASSIC_ZOO_HEADER {
  UINT32  Magic;              // 0xFDC4A7DC
  UINT32  FirstEntryOffset;   // Offset to first file entry
  UINT32  MinusOffset;        // Negative offset to last entry (-)
  UINT8   MajorVersion;       // Major version (2)
  UINT8   MinorVersion;       // Minor version (1)
  // Additional header fields...
} __attribute__((packed)) CLASSIC_ZOO_HEADER;
```

### 17.3 Classic Zoo Directory Entry

```c
typedef struct _CLASSIC_ZOO_ENTRY {
  UINT32  Magic;              // 0xFDC4A7DC
  UINT8   CompressionMethod;  // 0=stored, 1=LZD, 2=LZH
  UINT8   NextEntryOffset;    // Offset to next entry (variable)
  UINT32  OriginalSize;       // Uncompressed size
  UINT32  CompressedSize;     // Compressed size
  UINT16  Date;               // MS-DOS date format
  UINT16  Time;               // MS-DOS time format
  UINT16  CRC;                // CRC-16
  UINT32  OriginalPosition;   // Original file position
  UINT16  Attributes;         // File attributes
  UINT8   Generation;         // Generation number
  UINT8   Deleted;            // Deletion marker
  char    FileName[13];       // Null-terminated filename
  char    Comment[?];         // Variable-length comment
  // Followed by compressed data
} __attribute__((packed)) CLASSIC_ZOO_ENTRY;
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
  // Extract fields
  int day   = dosDate & 0x1F;
  int month = (dosDate >> 5) & 0x0F;
  int year  = ((dosDate >> 9) & 0x7F) + 1980;
  int sec   = (dosTime & 0x1F) * 2;
  int min   = (dosTime >> 5) & 0x3F;
  int hour  = (dosTime >> 11) & 0x1F;

  // Convert to Unix timestamp (use mktime or equivalent)
  struct tm t = {0};
  t.tm_year = year - 1900;
  t.tm_mon = month - 1;
  t.tm_mday = day;
  t.tm_hour = hour;
  t.tm_min = min;
  t.tm_sec = sec;
  time_t unixTime = mktime(&t);

  // Convert to NTP extended format
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
  // Convert NTP to Unix time
  time_t unixSec;
  uint32_t nanoSec;
  NTPToUnix(ntpTime, &unixSec, &nanoSec);

  // Convert to local time
  struct tm *t = localtime(&unixSec);

  // Validate range (1980-2107)
  if (t->tm_year + 1900 < 1980) {
    // Clamp to minimum (1980-01-01 00:00:00)
    *dosDate = 0x0021;  // 1980-01-01
    *dosTime = 0x0000;  // 00:00:00
    return;
  }
  if (t->tm_year + 1900 > 2107) {
    // Clamp to maximum (2107-12-31 23:59:58)
    *dosDate = 0xFF9F;  // 2107-12-31
    *dosTime = 0xBF7D;  // 23:59:58
    return;
  }

  // Encode MS-DOS date
  *dosDate = ((t->tm_year + 1900 - 1980) << 9) |
             ((t->tm_mon + 1) << 5) |
             t->tm_mday;

  // Encode MS-DOS time (2-second precision)
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
  - BWT+MTF+RAD50RLE+LZ78+Range compression pipeline
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
