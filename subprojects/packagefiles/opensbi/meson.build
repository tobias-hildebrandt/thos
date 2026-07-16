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
fw_text_start = '0x0'
fw_jump_address = '0x80200000'
fw_platform = 'generic'

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
name_firmwares = ['fw_dynamic', 'fw_jump']

defines = [
    # TODO: move to options
    ['-DFW_OPTIONS=' + fw_options],
    ['-DFW_TEXT_START=' + fw_text_start],
    ['-DFW_JUMP_ADDR=' + fw_jump_address],
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

# TODO: refactor to script? custom_target? something?

### firmware elfs

firmwares = {}
link_args += [
    # taken from opensbi's Makefile ELFFLAGS
    '-Wl,--gc-sections',
    '-Wl,--build-id=none',
    '-Wl,--no-dynamic-linker',
    '-Wl,-pie',
]

foreach name_firmware : name_firmwares
    firmware_elf = executable(
        name_firmware + '.elf',
        # sources
        [subdir_firmware / name_firmware + '.S', lib_libplatsbi],
        c_args: [defines],
        link_args: link_args,
        include_directories: [include_all],
        implicit_include_directories: false,
        dependencies: linker_script_targets[name_firmware],
    )
    firmware_bin = custom_target(
        name_firmware + '.bin',
        command: [
            script_make_bin,
            firmware_elf,
            '@OUTPUT@',
            '--strip-all',
        ],
        output: name_firmware + '.bin',
    )

    firmwares += {
        name_firmware: {
            'elf': firmware_elf,
            'bin': firmware_bin,
        },
    }
endforeach

# TODO: export stuff to easily make fw_payload?
#  FW_PAYLOAD_OFFSET FW_PAYLOAD_ALIGN
