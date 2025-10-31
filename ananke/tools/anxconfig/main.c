/*
 * ANXCONFIG - Ananke Configuration Tool
 * Portable menuconfig-like configuration system
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ananke/anxconfig.h>
#include <ananke/tui.h>

static void print_usage(const char *progname)
{
    printf("ANXCONFIG - Ananke Configuration Tool\n");
    printf("\n");
    printf("Usage: %s [options] <config.yaml>\n", progname);
    printf("\n");
    printf("Options:\n");
    printf("  -m, --menuconfig       Run interactive menu configuration (default)\n");
    printf("  -l, --load <file>      Load configuration values from file\n");
    printf("  -s, --save <file>      Save configuration values to file\n");
    printf("  -c, --cmake <file>     Generate CMake cache file\n");
    printf("  -H, --header <file>    Generate C header file\n");
    printf("  -M, --makefile <file>  Generate Makefile fragment\n");
    printf("  -a, --autoconf <file>  Generate autoconf fragment\n");
    printf("  -d, --defconfig        Load default configuration\n");
    printf("  -h, --help             Show this help\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s anxconfig.yaml                    # Interactive menu\n", progname);
    printf("  %s -l .config anxconfig.yaml         # Load saved config\n", progname);
    printf("  %s -s .config anxconfig.yaml         # Save config\n", progname);
    printf("  %s -c CMakeCache.txt anxconfig.yaml  # Generate CMake cache\n", progname);
    printf("\n");
}

int main(int argc, char **argv)
{
    IConfigDatabase *database = NULL;
    IConfigGenerator *generator = NULL;
    HRESULT hr;
    const char *yaml_file = NULL;
    const char *load_file = NULL;
    const char *save_file = NULL;
    const char *cmake_file = NULL;
    const char *header_file = NULL;
    const char *makefile_file = NULL;
    const char *autoconf_file = NULL;
    int run_menu = 1;
    int load_defaults = 0;
    int i;

    /* Parse command line arguments */
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-m") == 0 || strcmp(argv[i], "--menuconfig") == 0) {
            run_menu = 1;
        } else if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--load") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "Error: --load requires an argument\n");
                return 1;
            }
            load_file = argv[i];
        } else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--save") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "Error: --save requires an argument\n");
                return 1;
            }
            save_file = argv[i];
        } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--cmake") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "Error: --cmake requires an argument\n");
                return 1;
            }
            cmake_file = argv[i];
        } else if (strcmp(argv[i], "-H") == 0 || strcmp(argv[i], "--header") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "Error: --header requires an argument\n");
                return 1;
            }
            header_file = argv[i];
        } else if (strcmp(argv[i], "-M") == 0 || strcmp(argv[i], "--makefile") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "Error: --makefile requires an argument\n");
                return 1;
            }
            makefile_file = argv[i];
        } else if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--autoconf") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "Error: --autoconf requires an argument\n");
                return 1;
            }
            autoconf_file = argv[i];
        } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--defconfig") == 0) {
            load_defaults = 1;
        } else if (argv[i][0] != '-') {
            yaml_file = argv[i];
        } else {
            fprintf(stderr, "Error: Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    if (!yaml_file) {
        fprintf(stderr, "Error: No configuration file specified\n");
        print_usage(argv[0]);
        return 1;
    }

    /* Create configuration database */
    hr = AnxConfigCreateDatabase(&database);
    if (FAILED(hr)) {
        fprintf(stderr, "Error: Failed to create configuration database: 0x%08X\n", hr);
        return 1;
    }

    /* Load YAML configuration */
    printf("Loading configuration from: %s\n", yaml_file);
    hr = database->Vtbl->LoadFromFile(database, yaml_file);
    if (FAILED(hr)) {
        fprintf(stderr, "Error: Failed to load configuration file: 0x%08X\n", hr);
        database->Vtbl->Release(database);
        return 1;
    }

    /* Load existing values if specified */
    if (load_file) {
        printf("Loading values from: %s\n", load_file);
        hr = database->Vtbl->LoadValues(database, load_file);
        if (FAILED(hr)) {
            fprintf(stderr, "Warning: Failed to load values: 0x%08X\n", hr);
        }
    }

    /* Set defaults if requested */
    if (load_defaults) {
        printf("Loading default configuration...\n");
        /* Defaults are already loaded from YAML */
    }

    /* Evaluate dependencies */
    hr = database->Vtbl->EvaluateDependencies(database);
    if (FAILED(hr)) {
        fprintf(stderr, "Error: Failed to evaluate dependencies: 0x%08X\n", hr);
    }

    /* Run interactive menu if requested */
    if (run_menu) {
        printf("Starting interactive configuration menu...\n");
        printf("(Note: Full TUI not yet implemented, using placeholder)\n");

        /* This would call AnxConfigRunMenu(database, "Configuration Menu") */
        /* For now, just display info */
        UINTN count = 0;
        database->Vtbl->GetItemCount(database, &count);
        printf("Configuration has %zu items\n", (size_t)count);
    }

    /* Save configuration if requested */
    if (save_file) {
        printf("Saving configuration to: %s\n", save_file);
        hr = database->Vtbl->SaveValues(database, save_file);
        if (FAILED(hr)) {
            fprintf(stderr, "Error: Failed to save configuration: 0x%08X\n", hr);
        } else {
            printf("Configuration saved successfully\n");
        }
    }

    /* Generate output files */
    if (cmake_file || header_file || makefile_file || autoconf_file) {
        hr = AnxConfigCreateGenerator(&generator);
        if (FAILED(hr)) {
            fprintf(stderr, "Error: Failed to create generator: 0x%08X\n", hr);
            database->Vtbl->Release(database);
            return 1;
        }

        if (cmake_file) {
            printf("Generating CMake cache: %s\n", cmake_file);
            hr = generator->Vtbl->GenerateCMakeCache(generator, database, cmake_file);
            if (FAILED(hr)) {
                fprintf(stderr, "Error: Failed to generate CMake cache: 0x%08X\n", hr);
            } else {
                printf("CMake cache generated successfully\n");
            }
        }

        if (header_file) {
            printf("Generating C header: %s\n", header_file);
            hr = generator->Vtbl->GenerateCHeader(generator, database, header_file);
            if (FAILED(hr)) {
                fprintf(stderr, "Error: Failed to generate header: 0x%08X\n", hr);
            } else {
                printf("C header generated successfully\n");
            }
        }

        if (makefile_file) {
            printf("Generating Makefile: %s\n", makefile_file);
            hr = generator->Vtbl->GenerateMakefile(generator, database, makefile_file);
            if (FAILED(hr)) {
                fprintf(stderr, "Error: Failed to generate Makefile: 0x%08X\n", hr);
            } else {
                printf("Makefile generated successfully\n");
            }
        }

        if (autoconf_file) {
            printf("Generating autoconf: %s\n", autoconf_file);
            hr = generator->Vtbl->GenerateAutoconf(generator, database, autoconf_file);
            if (FAILED(hr)) {
                fprintf(stderr, "Error: Failed to generate autoconf: 0x%08X\n", hr);
            } else {
                printf("Autoconf generated successfully\n");
            }
        }

        generator->Vtbl->Release(generator);
    }

    /* Cleanup */
    database->Vtbl->Release(database);

    printf("ANXCONFIG completed successfully\n");
    return 0;
}
