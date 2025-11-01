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
- Solid compression with seekable blocks
- Per-file compression with seekable blocks
- File-level and archive-level digital signatures
- Multiple compression algorithms
- Block-based seeking via LEB128 offset tables

## 2. Archive Structure

### 2.1 Overall Layout

```
[Archive Header]
[Compression Mode Descriptor]
[File Entries...]
  [File Header]
  [File Metadata]
  [File Data / Compressed Data]
  [File Signature] (optional)
[Central Directory]
[Archive Signature] (optional)
[End of Archive Marker]
```

### 2.2 Magic Numbers

```
Zoo64 Archive:        0x5A4F4F36 0x34415243  ("ZOO64ARC")
Solid Block:          0x534F4C49 0x44424C4B  ("SOLIDBLK")
File Entry:           0x46494C45 0x454E5452  ("FILEENTR")
Metadata Block:       0x4D455441 0x44415441  ("METADATA")
Signature Block:      0x5349474E 0x41545552  ("SIGNATUR")
Central Directory:    0x43454E54 0x44495220  ("CENTDIR ")
End of Archive:       0x454E444F 0x46415243  ("ENDOFARC")
```

## 3. Archive Header

The archive header appears at the beginning of every Zoo64 archive.

```c
typedef struct _ZOO64_ARCHIVE_HEADER {
  UINT64  Magic;              // 0x5A4F4F3634415243 ("ZOO64ARC")
  UINT16  MajorVersion;       // Format major version (1)
  UINT16  MinorVersion;       // Format minor version (0)
  UINT32  Flags;              // Archive flags
  UINT64  CreationTime;       // Unix timestamp (microseconds since epoch)
  UINT64  ModificationTime;   // Unix timestamp (microseconds since epoch)
  UINT32  CompressionMode;    // Compression mode identifier
  UINT32  FileCount;          // Number of files in archive
  UINT64  CentralDirOffset;   // Offset to central directory
  UINT64  ArchiveSize;        // Total archive size in bytes
  UINT32  BlockSize;          // Block size for seekable compression (power of 2)
  UINT32  Reserved;           // Reserved for future use
  UINT8   UUID[16];           // Archive UUID
} __attribute__((packed)) ZOO64_ARCHIVE_HEADER;
```

### 3.1 Archive Flags

```
Bit 0:     Solid compression enabled
Bit 1:     Seekable blocks enabled
Bit 2:     Archive signature present
Bit 3:     Encrypted
Bit 4:     UTF-8 strict mode
Bit 5-7:   Reserved
Bit 8:     Extended metadata present
Bit 9:     ACLs preserved
Bit 10:    Extended attributes preserved
Bit 11:    Alternate data streams preserved
Bit 12-15: Reserved
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
  UINT64  CreationTime;       // File creation time (microseconds)
  UINT64  ModificationTime;   // File modification time (microseconds)
  UINT64  AccessTime;         // File access time (microseconds)
  UINT32  UID;                // User ID (Unix)
  UINT32  GID;                // Group ID (Unix)
  UINT32  Mode;               // File mode/permissions (Unix)
  UINT32  Attributes;         // Platform-specific attributes
} __attribute__((packed)) ZOO64_FILE_HEADER;
```

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
```

### 6.3 ACL Format

```c
typedef struct _ZOO64_ACL_ENTRY {
  UINT32  EntryType;          // ACE type (allow/deny/audit)
  UINT32  Permissions;        // Permission mask
  UINT32  Flags;              // ACE flags
  UINT16  IdentifierType;     // User/Group/SID type
  UINT16  IdentifierLength;   // Length of identifier
  // Followed by identifier (UTF-8 username, SID, etc.)
} __attribute__((packed)) ZOO64_ACL_ENTRY;
```

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
  UINT64  SigningTime;        // Unix timestamp
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

## 17. Version History

- **1.0** (2025-10-31): Initial specification draft

## 18. Future Extensions

Reserved areas for future features:
- Encryption (AES-256, ChaCha20-Poly1305)
- Delta compression
- Deduplication
- Volume spanning
- Error correction codes
- Compression algorithm plugins

---

**End of Specification**
