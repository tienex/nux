#!/usr/bin/env python3
"""
Script to update eCRT files with A•NUX Project banner and rename EC to eCRT
"""

import os
import re
from pathlib import Path

# Banner template for C/C++ files
C_BANNER = """/** @file
  eCRT - An embedded C runtime library

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

"""

# Banner template for assembly files
ASM_BANNER = """/* @file
 * eCRT - An embedded C runtime library
 *
 * Copyright (C) 2025 A•NUX Project
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

"""

def has_anux_banner(content):
    """Check if file already has A•NUX Project banner"""
    return "A•NUX Project" in content or "A\u2022NUX Project" in content

def update_file(filepath):
    """Update a single file with banner and EC->eCRT replacements"""
    try:
        with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()

        if not content.strip():
            return False

        # Skip if already has banner
        if has_anux_banner(content):
            print(f"Skipping {filepath} - already has A•NUX banner")
            return False

        # Choose banner based on file extension
        ext = filepath.suffix.lower()
        if ext == '.s':
            banner = ASM_BANNER
        else:
            banner = C_BANNER

        # Prepend banner
        new_content = banner + content

        # Replace EC references with eCRT
        # Be careful to preserve _EC_SOURCE and similar defines
        replacements = [
            (r'\bEC\s+C\s+Runtime', 'eCRT C Runtime'),
            (r'LibEC\b', 'libeCRT'),
            (r'libec\b', 'libecrt'),
            (r'\bEC\s+-\s+An embedded', 'eCRT - An embedded'),
            (r'__ecrt_', '__ecrt_'),  # Already correct
            (r'<ecrt/', '<ecrt/'),  # Already correct
            (r'"ecrt/', '"ecrt/'),  # Already correct
            (r'#ifndef\s+__EC_', '#ifndef __eCRT_'),
            (r'#define\s+__EC_', '#define __eCRT_'),
            (r'#endif\s+/\*\s*EC_', '#endif /* eCRT_'),
        ]

        for pattern, replacement in replacements:
            new_content = re.sub(pattern, replacement, new_content)

        # Write updated content
        with open(filepath, 'w', encoding='utf-8') as f:
            f.write(new_content)

        print(f"Updated: {filepath}")
        return True

    except Exception as e:
        print(f"Error processing {filepath}: {e}")
        return False

def main():
    """Main function to process all files"""
    ecrt_dir = Path('/home/user/nux/ananke/libs/ecrt')

    # Find all source files
    patterns = ['**/*.c', '**/*.h', '**/*.S']
    files_to_update = []

    for pattern in patterns:
        files_to_update.extend(ecrt_dir.glob(pattern))

    print(f"Found {len(files_to_update)} files to process")

    updated = 0
    for filepath in sorted(files_to_update):
        if update_file(filepath):
            updated += 1

    print(f"\nProcessed {len(files_to_update)} files, updated {updated}")

if __name__ == '__main__':
    main()
