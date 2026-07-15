project(
    'opensbi',
    ['c'],
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

verbose = get_option(option_verbose_str)
# TODO: build only specific fw type if this is passed in, otherwise build all
fw_type = get_option(option_fw_type_str)
fw_options = get_option(option_fw_options_str)

c_args = get_option('c_args')
link_args = get_option('c_link_args')

# TODO pass in?
fw_text_start = '0x0'
fw_platform = 'generic'

### includes
include_opensbi = include_directories('include')
include_firmware = include_directories(subdir_firmware)

include_all = [include_opensbi, include_firmware]

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

# TODO: FW_PAYLOAD_OFFSET FW_PAYLOAD_ALIGN
defines = [
    # TODO: move to option?
    '-DFW_TEXT_START='
    + fw_text_start,
    '-DFW_OPTIONS=' + fw_options,
]

args_preprocess = [
    '-P', defines,
    c_args,
]

name_linker_script_base = 'fw_base.ld'
# TODO: all 3 types of firmware as exported dependencies
name_linker_script_target = 'fw_dynamic.elf.ld'

# TODO: refactor to script? custom_target? something?
linker_script_base = compiler.preprocess(
    subdir_firmware / name_linker_script_base + 'S',
    output: '@BASENAME@.ld',
    compile_args: args_preprocess,
    include_directories: [include_all],
)
dep_linker_script_base = declare_dependency(sources: linker_script_base)
linker_script_target = compiler.preprocess(
    subdir_firmware / name_linker_script_target + 'S',
    output: '@BASENAME@.ld',
    compile_args: args_preprocess,
    include_directories: [include_all],
    dependencies: dep_linker_script_base,
)
linker_script_path = ''
foreach preprocessed : linker_script_target
    if preprocessed.full_path().split('/')[-1] == name_linker_script_target
        linker_script_path = preprocessed.full_path()
    endif
endforeach
if linker_script_path == ''
    error('failed to get preprocessed linker script:', name_linker_script_target)
endif
# TODO: real dependency
dep_firmware_linker_script = declare_dependency(
    link_args: '-Wl,-T' + linker_script_path,
)

### firmware elfs
link_args += [
    # taken from opensbi's Makefile ELFFLAGS
    '-Wl,--gc-sections',
    '-Wl,--build-id=none',
    '-Wl,--no-dynamic-linker',
    '-Wl,-pie',
]

name_firmware_dynamic = 'fw_dynamic'
firmware_dynamic_elf = executable(
    name_firmware_dynamic + '.elf',
    [
        subdir_firmware / 'fw_dynamic.S',
        # lib_libsbi,
        lib_libplatsbi,
    ],
    c_args: [defines],
    link_args: link_args,
    include_directories: [include_all],
    implicit_include_directories: false,
    build_by_default: true,
    dependencies: [dep_firmware_linker_script],
    link_depends: linker_script_target,
)
firmware_dynamic_bin = custom_target(
    name_firmware_dynamic + '.bin',
    command: [script_make_bin, firmware_dynamic_elf, '@OUTPUT@', '--strip-all'],
    output: name_firmware_dynamic + '.bin',
)
# exported, don't complain
_ = firmware_dynamic_bin
