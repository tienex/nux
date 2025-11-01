#!/usr/bin/env python3
"""
Script to update all makefiles to use uppercase ECRT and CCRT
"""

import os
import re
from pathlib import Path

def update_file_uppercase(filepath):
    """Update a single file to use uppercase variable names"""
    try:
        with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()

        original = content

        # Update eCRT to ECRT in variable names
        replacements = [
            (r'\$\(eCRT_', r'$(ECRT_'),
            (r'\$\{eCRT_', r'${ECRT_'),
            (r'eCRT_DIR', 'ECRT_DIR'),
            (r'eCRT_ARCH_DIR', 'ECRT_ARCH_DIR'),
            (r'eCRT_ARCH_SRCS', 'ECRT_ARCH_SRCS'),
            (r'eCRT_SRCS', 'ECRT_SRCS'),
            (r'eCRT_CFLAGS', 'ECRT_CFLAGS'),
            (r'eCRT_LDFLAGS', 'ECRT_LDFLAGS'),
            (r'eCRT_CPPFLAGS', 'ECRT_CPPFLAGS'),
            (r'eCRT_CCASFLAGS', 'ECRT_CCASFLAGS'),
            (r'eCRT_INTRINSICS_SRCS', 'ECRT_INTRINSICS_SRCS'),
            (r'eCRT_MACHINE', 'ECRT_MACHINE'),
            (r'LIBeCRT_SRCDIR', 'LIBECRT_SRCDIR'),
            (r'eCRTSRCDIR', 'ECRTSRCDIR'),

            # Update cCRT to CCRT in variable names
            (r'\$\(cCRT_', r'$(CCRT_'),
            (r'\$\{cCRT_', r'${CCRT_'),
            (r'cCRT_DIR', 'CCRT_DIR'),
            (r'cCRT_SRCS', 'CCRT_SRCS'),
            (r'CCRTSRCDIR', 'CCRTSRCDIR'),  # Already uppercase
        ]

        for pattern, replacement in replacements:
            content = re.sub(pattern, replacement, content)

        if content != original:
            with open(filepath, 'w', encoding='utf-8') as f:
                f.write(content)
            print(f"Updated: {filepath}")
            return True
        return False

    except Exception as e:
        print(f"Error processing {filepath}: {e}")
        return False

def main():
    """Main function to process all makefiles"""
    base_dir = Path('/home/user/nux')

    # Find all makefiles and build config files
    patterns = ['**/*.mk', '**/*.mk.in', '**/*.am', '**/*.am.in', '**/Makefile.in', '**/Makefile.am']
    files_to_update = set()

    for pattern in patterns:
        files_to_update.update(base_dir.glob(pattern))

    # Also update configure.ac
    files_to_update.update(base_dir.glob('**/configure.ac'))

    print(f"Found {len(files_to_update)} files to process")

    updated = 0
    for filepath in sorted(files_to_update):
        if 'ananke/libs/ecrt' in str(filepath) or 'ananke/libs/ccrt' in str(filepath):
            if update_file_uppercase(filepath):
                updated += 1

    print(f"\nProcessed {len(files_to_update)} files, updated {updated}")

if __name__ == '__main__':
    main()
