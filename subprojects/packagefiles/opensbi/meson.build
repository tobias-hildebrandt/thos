project(
    'opensbi',
    ['c'],
    version: '1.9.0',
    meson_version: '1.7.0',
)

compiler = meson.get_compiler('c')

### sources
path_source = meson.current_source_dir()
subdir_firmware = 'firmware'

### scripts
script_make_opensbi = find_program('../../misc/make_opensbi.sh')
script_make_bin = find_program('../../misc/make_bin.sh')

### options
option_fw_type_str = 'fw-type'
option_fw_options_str = 'fw-options'
option_verbose_str = 'verbose'

# TODO: build only specific fw type if this is passed in, otherwise build all
fw_type = get_option(option_fw_type_str)
fw_options = get_option(option_fw_options_str)
verbose = get_option(option_verbose_str)

c_args = get_option('c_args')
link_args = get_option('c_link_args')

# TODO pass in
fw_platform = 'generic'
# address to put opensbi
fw_text_start = '0x80000000'
# address of kernel
fw_jump_address = '0x80200000'
# fw_payload_align = target_machine.cpu().contains('32') ? '0x400000' : '0x200000'
fw_payload_align = '0x200000'
fw_payload_offset = '0x200000' # TODO: jump_address - text_start??
# TODO devicetree args? FW_FDT_*

### includes
include_opensbi = include_directories('include')
include_firmware = include_directories(subdir_firmware)

include_all = [include_opensbi, include_firmware]
include_args = ['-I', 'include', '-I', subdir_firmware]

### make-wrapped static libraries

# extract args to pass to make
c_arch = ''
c_abi = ''
c_code_model = ''
foreach arg : c_args
    if arg.startswith('-march=')
        c_arch = arg.replace('-march=', '')
    endif
    if arg.startswith('-mabi=')
        c_abi = arg.replace('-mabi=', '')
    endif
    if arg.startswith('-mcmodel=')
        c_code_model = arg.replace('-mcmodel=', '')
    endif
endforeach

make_env = {
    # no CROSS_COMPILE, since gnu cross bfd doesn't work
    # (affects gcc only, since LLVM='') force use of CC and LD
    'CC': ' '.join(compiler.cmd_array()),
    'LD': ' '.join(compiler.get_linker_id()),
    'PLATFORM_RISCV_XLEN': target_machine.cpu().strip('riscv'),
    'PLATFORM': fw_platform,
    'FW_OPTIONS': fw_options,
    'FW_TEXT_START': fw_text_start,
    'LLVM': compiler.get_id() == 'clang' ? '1' : '',
    'V': verbose.to_int().to_string(), # verbose,
    'PLATFORM_RISCV_ABI': c_abi, # -mabi
    'PLATFORM_RISCV_ISA': c_arch, # -march
    'PLATFORM_RISCV_CODE_MODEL': c_code_model, # -mcmodel
}

# lib platform sbi
# TODO: move completely into meson
# make target $(platform_build_dir)/lib/libplatsbi.a
name_libplatsbi = 'libplatsbi.a'
path_libplatsbi = 'platform' / fw_platform / 'lib' / name_libplatsbi
lib_libplatsbi = custom_target(
    name_libplatsbi,
    command: [
        script_make_opensbi,
        # source dir
        path_source,
        # private build dir
        '@PRIVATE_DIR@',
        # target name, relative to private build dir
        path_libplatsbi,
        # copy out destination
        '@OUTPUT@',
        # verbosity
        verbose.to_int().to_string(),
    ],
    env: make_env,
    output: name_libplatsbi,
)

### pre-processed linker scripts
name_firmwares = ['fw_dynamic', 'fw_jump', 'fw_payload']

defines = [
    # TODO: move to options
    ['-DFW_OPTIONS=' + fw_options],
    ['-DFW_TEXT_START=' + fw_text_start],
    ['-DFW_JUMP_ADDR=' + fw_jump_address],
    ['-DFW_PAYLOAD_OFFSET=' + fw_payload_offset],
    # ['-DFW_PAYLOAD_ALIGN=' + fw_payload_align],
]

# preprocessor only
# no #line commands
args_preprocess = ['-E', '-x', 'c', '-P', defines, c_args]

linker_script_targets = {}

foreach name_firmware : name_firmwares
    name = name_firmware + '.ld'
    input = subdir_firmware / name_firmware + '.elf.ldS'
    # TODO: move to script?
    target = custom_target(
        name,
        command: [
            compiler.cmd_array(),
            args_preprocess,
            include_args,
            '@INPUT@',
            '-o', '@OUTPUT@',
        ],
        input: input,
        output: name,
    )

    # TODO: figure out real dependency?
    dep_linker_script = declare_dependency(
        sources: target,
        link_args: '-Wl,-T' + target.full_path(),
    )

    linker_script_targets += {
        name_firmware: dep_linker_script,
    }
endforeach

### firmware elfs

# map of fw_* -> { args: { elf: ..., bin: ... } } | { elf: ..., bin: ..., args: ... }
firmwares = {}

link_args += [
    # taken from opensbi's Makefile ELFFLAGS
    '-Wl,--gc-sections',
    '-Wl,--build-id=none',
    '-Wl,--no-dynamic-linker',
    '-Wl,-pie',
]

foreach name_firmware : name_firmwares
    # copy assembly source so that superprojects can use it
    assembly_source = custom_target(
        name_firmware + '.S',
        command: [
            'cp',
            '@INPUT@',
            '@OUTPUT@',
        ],
        input: meson.current_source_dir() / subdir_firmware / name_firmware + '.S',
        output: name_firmware + '.S',
    )

    # arguments for targets
    target_args = {
        'elf': {
            'extension': '.elf',
            'sources': [assembly_source, lib_libplatsbi],
            'c_args': defines,
            'link_args': link_args,
            'include_directories': [include_all],
            'implicit_include_directories': false,
            'dependencies': linker_script_targets[name_firmware],
        },
        'bin': {
            'extension': '.bin',
            'output': name_firmware + '.bin',
            'depends': name_firmware + '.bin',
        },
    }

    # fw_payload only exports args, not elf or bin
    # user of this dependency must make target themselves
    # and add -DFW_PAYLOAD_PATH=your_target.bin
    if name_firmware == 'fw_payload'
        firmwares += {
            name_firmware: {
                'args': target_args,
            },
        }
        continue
    endif

    # fw_dynamic and fw_jump can be built here and used simply
    elf = target_args['elf']
    bin = target_args['bin']

    firmware_elf = executable(
        name_firmware + elf['extension'],
        # sources
        elf['sources'],
        c_args: elf['c_args'],
        link_args: elf['link_args'],
        include_directories: elf['include_directories'],
        implicit_include_directories: elf['implicit_include_directories'],
        dependencies: elf['dependencies'],
    )
    firmware_bin = custom_target(
        name_firmware + bin['extension'],
        # command depends on elf
        # TODO: call above executable() inline??
        command: [
            script_make_bin,
            firmware_elf,
            '@OUTPUT@',
            '--strip-all',
        ],
        output: bin['output'],
    )

    firmwares += {
        name_firmware: {
            'elf': firmware_elf,
            'bin': firmware_bin,
            # if the user wants to build it themselves for some reason
            'args': target_args,
        },
    }
endforeach
