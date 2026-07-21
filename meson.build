project(
    'thos',
    ['c'],
    version: '0.0.1',
    meson_version: '1.7.0',
    default_options: {
        'c_std': 'gnu17',
        'warning_level': '3', # -Wall -Wextra -Wpedantic
        'optimization': '2',
    },
)

proj_opensbi = subproject('opensbi')

compiler = meson.get_compiler('c')

### scripts
misc_dir = meson.current_source_dir() / 'misc'

script_list_sources = find_program('list_sources.sh', dirs: [misc_dir])
script_make_bin = find_program('make_bin.sh', dirs: [misc_dir])
script_user_asm = find_program('make_userexec_blob_asm.sh', dirs: [misc_dir])
script_find_libgcc = find_program('find_libgcc.sh', dirs: [misc_dir])
script_find_tests = find_program('find_tests.sh', dirs: [misc_dir])
script_make_testinfo = find_program('make_test_info.sh', dirs: [misc_dir])
script_run_single_test = find_program('run_single_test.sh', dirs: [misc_dir])
script_make_qemu_launch = find_program('make_qemu_launch.sh', dirs: [misc_dir])

### source directories
dir_base = 'src'

subdir_kernel = 'kernel'
subdir_libc = 'libc'
subdir_common = 'common'
subdir_user = 'user'
subdir_userlib = 'userlib'

subdir_all = [
    subdir_kernel,
    subdir_libc,
    subdir_common,
    subdir_user,
    subdir_userlib,
]

# map of subdir -> list of c source files in that subdir
dict_c_sources = {}
foreach subdir : subdir_all
    c_sources_subdir = run_command(
        script_list_sources,
        env: {'TARGET': dir_base / subdir},
        check: true,
    ).stdout().splitlines()
    dict_c_sources += {subdir: c_sources_subdir}
endforeach

### includes
include_common = include_directories(dir_base / subdir_common)
include_kernel = include_directories(dir_base / subdir_kernel)
include_libc = include_directories(dir_base / subdir_libc, is_system: true)
include_userlib = include_directories(dir_base / subdir_userlib)
include_user = include_directories(dir_base / subdir_user)
include_build_dir = include_directories('.')

# must be manually included in build targets, no project_includes() in meson
includes_proj = [include_common, include_libc]

### compiler and linker args
# args from cross files
default_c_args = get_option('c_args')
default_link_args = get_option('c_link_args')

# project-wide link arguments
link_args_proj = default_link_args
# project-wide c arguments
c_args_proj = default_c_args + [
    # warning args
    '-Wformat=2',
    # fix path in __FILE__ macro, debug source info, etc
    '-ffile-prefix-map=../../../=',
    '-fmacro-prefix-map=../../=',
]

# clang c arguments
c_args_clang = [
    '-Wno-gnu-zero-variadic-macro-arguments',
    '-Wno-empty-translation-unit',
]
# gcc c arguments
c_args_gcc = []

if compiler.get_id() == 'clang'
    c_args_proj += c_args_clang
elif compiler.get_id() == 'gcc'
    c_args_proj += c_args_gcc
endif

# defines
option_defines_str = 'defines'
option_defines = get_option(option_defines_str)

file_defines_debug = misc_dir / 'defines' / 'debug.txt'
file_defines_other = misc_dir / 'defines' / 'other.txt'

# parse defines files, generate a flags.h
# TODO: add and parse comments in files?
defines_debug = run_command(
    ['grep', '-v', '#', file_defines_debug],
    check: true,
).stdout().splitlines()
defines_other = run_command(
    ['grep', '-v', '#', file_defines_other],
    check: true,
).stdout().splitlines()

# TODO: move to shell script?
defines_map = {}
define_debug_all = false
foreach define : option_defines.strip().split(' ')
    if define == ''
        continue
    endif
    define_splits = define.split('=')
    if define_splits[0] == 'DEBUG' and define_splits.get(1, '1').to_int() > 0
        define_debug_all = true
        continue
    endif
    if not defines_debug.contains(define_splits[0]) and defines_other.contains(define_splits[0])
        error('unknown define: ', define_splits[0])
    endif
    defines_map += {
        define_splits[0]: define_splits.get(1, '1'),
    }
endforeach
foreach define : defines_other
    if define == ''
        continue
    endif
    define_splits = define.split('=')
    if defines_map.has_key(define_splits[0])
        continue
    endif
    defines_map += {
        define_splits[0]: define_splits[1],
    }
endforeach
foreach define : defines_debug
    if define == ''
        continue
    endif
    if define_debug_all
        defines_map += {
            define: '1',
        }
        continue
    endif
    if defines_map.has_key(define)
        continue
    endif
    defines_map += {
        define: '0',
    }
endforeach

# TODO: make private subdir, copy there, -I that instead of entire builddir
message(defines_map)
header_flags = configure_file(
    output: 'flags.h',
    configuration: defines_map,
)

# TESTS_ENABLED and MACHINE still get passed to compiler on commandline

# tests option
option_tests_str = 'enable-tests'
c_define_tests = 'TESTS_ENABLED'
option_tests = get_option(option_tests_str)

# normal kernel gets 0, test kernel gets 1
c_arg_kernel_nontest = '-D' + c_define_tests + '=0'
c_arg_kernel_test = '-D' + c_define_tests + '=1'

# machine option
option_machine_str = 'machine'
option_machine = get_option(option_machine_str)

c_args_proj += '-DMACHINE=' + option_machine

# TODO: build libgcc / llvm compiler-rt as subprojects?

# add libgcc
args_machine = []
foreach arg : default_c_args
    if arg.startswith('-m')
        args_machine += arg
    endif
endforeach

archive_libgcc = run_command(
    script_find_libgcc,
    env: {'MACHINE_ARGS': ' '.join(args_machine)},
    check: true,
).stdout().strip()
dep_libgcc = declare_dependency(link_args: archive_libgcc)

source_dir_absolute = meson.project_source_root() / 'src'
kernel_link_args = ['-Wl,-T' + (source_dir_absolute / 'kernel' / 'kernel.lds')]
user_link_args = ['-Wl,-T' + (source_dir_absolute / 'user' / 'user.lds')]

add_project_arguments(c_args_proj, language: 'c')
add_project_link_arguments(link_args_proj, language: 'c')
add_project_dependencies(dep_libgcc, language: 'c')

### targets
# libc
target_libc = static_library(
    'libc',
    dict_c_sources[subdir_libc],
    include_directories: [includes_proj], # already has libc
    implicit_include_directories: false,
)
dep_libc = declare_dependency(link_with: target_libc)

# userlib
target_userlib = static_library(
    'userlib',
    dict_c_sources[subdir_userlib],
    include_directories: [includes_proj, include_userlib],
    implicit_include_directories: false,

)
dep_userlib = declare_dependency(link_with: target_userlib)

# generate user exec blob sources
deps_userexec_blobs = []
foreach source_user : dict_c_sources[subdir_user]
    # strip dir path and file extension
    basename = source_user.split('/')[-1].substring(0, -2)
    name = 'userexec_' + basename

    name_userexec_exec = name + '.step0.exec'
    name_userexec_bin = name + '.step1.bin'
    name_userexec_assembly = name + '.step2.S'
    name_userexec_blob = name + '.step3.blob'

    # user executable
    target_userexec_exec = executable(
        name_userexec_exec,
        source_user,
        name_suffix: 'o',
        dependencies: [dep_libc, dep_userlib],
        include_directories: [includes_proj, include_userlib, include_user],
        implicit_include_directories: false,
        link_args: [user_link_args],
    )

    # expanded user bin
    target_userexec_bin = custom_target(
        name_userexec_bin,
        command: [
            script_make_bin,
            target_userexec_exec,
            '@OUTPUT@',
            '--set-section-flags', '*=alloc,contents',
        ],
        output: name_userexec_bin,
        depends: target_userexec_exec,
    )

    # assembly that .incbin's the user bin
    target_userexec_assembly = custom_target(
        name_userexec_assembly,
        command: [script_user_asm, target_userexec_bin, '@OUTPUT@', basename],
        output: name_userexec_assembly,
        depends: target_userexec_bin,
    )
    dep_userexec_assembly = declare_dependency(sources: target_userexec_assembly)

    # blob of compiled assembly
    target_userexec_blob = static_library(
        name_userexec_blob,
        name_prefix: '',
        dependencies: dep_userexec_assembly,
        implicit_include_directories: false, # no includes needed
    )
    # use the objects instead of the archive
    obj_userexec_blob = target_userexec_blob.extract_all_objects(recursive: false)
    dep_userexec_blob = declare_dependency(objects: obj_userexec_blob)

    deps_userexec_blobs += dep_userexec_blob
endforeach

# kernel
name_kernel = 'kernel'
tag_test = '-test'
tag_nontest = ''

name_kernel_tests = 'tests.txt'
name_kernel_testinfo = 'testinfo.c'

# nontest kernel
exec_kernel_nontest = executable(
    name_kernel + tag_nontest,
    dict_c_sources[subdir_kernel] + header_flags,
    dependencies: [dep_libc, deps_userexec_blobs],
    include_directories: [includes_proj, include_kernel, include_build_dir],
    implicit_include_directories: false,
    link_args: kernel_link_args,
    c_args: c_arg_kernel_nontest,
)
message('kernel path:', exec_kernel_nontest.full_path())

kernel_elfs = {
    tag_nontest: exec_kernel_nontest,
}

# kernel test target
if option_tests
    # finds tests in kernel source code
    target_tests = run_command(
        [
            script_find_tests,
            '--full', source_dir_absolute / subdir_kernel,
        ],
        check: true,
    ).stdout().splitlines()

    # testinfo file
    target_testinfo = custom_target(
        name_kernel_testinfo,
        command: [script_make_testinfo, '@OUTPUT@', target_tests],
        output: name_kernel_testinfo,
    )

    # test kernel
    exec_kernel_test = executable(
        name_kernel + tag_test,
        [dict_c_sources[subdir_kernel], target_testinfo, header_flags],
        dependencies: [dep_libc, deps_userexec_blobs],
        include_directories: [includes_proj, include_kernel, include_build_dir],
        implicit_include_directories: false,
        c_args: c_arg_kernel_test,
        link_args: kernel_link_args,
    )
    message('test kernel path:', exec_kernel_test.full_path())

    kernel_elfs += {tag_test: exec_kernel_test}
endif

# TODO: add support for '-bios default'
opensbi_fw_type = proj_opensbi.get_variable('fw_type')
opensbi_firmware_dict = proj_opensbi.get_variable('firmwares')['fw_' + opensbi_fw_type]

kernel_firmwares = {}
if opensbi_fw_type == 'payload'
    opensbi_elf_args = opensbi_firmware_dict['args']['elf']
    opensbi_bin_args = opensbi_firmware_dict['args']['bin']

    foreach suffix : kernel_elfs.keys()
        kernel_bin = custom_target(
            name_kernel + suffix + '.bin',
            command: [
                script_make_bin,
                kernel_elfs[suffix],
                '@OUTPUT@',
                '--strip-all',
            ],
            output: name_kernel + suffix + '.bin',
        )

        opensbi_elf = executable(
            'fw_payload' + suffix + opensbi_elf_args['extension'],
            # sources
            opensbi_elf_args['sources'],
            c_args: opensbi_elf_args['c_args']
            + ['-DFW_PAYLOAD_PATH="' + kernel_bin.full_path().strip() + '"'],
            link_args: opensbi_elf_args['link_args'],
            include_directories: opensbi_elf_args['include_directories'],
            implicit_include_directories: opensbi_elf_args['implicit_include_directories'],
            dependencies: [
                opensbi_elf_args['dependencies'],
                declare_dependency(sources: kernel_bin),
            ],
        )
        opensbi_bin = custom_target(
            'fw_payload' + suffix + opensbi_bin_args['extension'],
            command: [
                script_make_bin,
                opensbi_elf,
                '@OUTPUT@',
                '--strip-all',
            ],
            output: 'fw_payload' + suffix + opensbi_bin_args['extension'],
        )

        kernel_firmwares += {suffix: opensbi_bin}
    endforeach
else
    kernel_firmwares = {
        tag_nontest: opensbi_firmware_dict['bin'],
        tag_test: opensbi_firmware_dict['bin'],
    }
endif
# custom target to write QEMU launch command

if option_machine != ''
    name_qemu_launch = 'qemu-launch-' + opensbi_fw_type + '.sh'

    # TODO: centralize config in machine files readable in C and meson?
    qemu_no_reboot = false

    qemu_extra_devices = ''
    if option_machine == 'sifive_u'
        qemu_no_reboot = true
        # TODO: actually use this in qemu launch script and tests
        qemu_extra_devices = [
            '-chardev pipe,path=build/serial1.fifo,id=char1',
            ' -serial chardev:char1',
        ]
    endif

    # only partial, still needs the bios bin or 'none' and ' -kernel <kernel>' str
    qemu_partial_launch = ' '.join(
        [
            'qemu-system-' + target_machine.cpu(),
            '-machine', option_machine,
            '-nographic -serial mon:stdio', qemu_no_reboot ? '-no-reboot' : '',
            '-bios ',
        ],
    )
    # TODO: use format strings? or move logic into shell script
    if opensbi_fw_type == 'payload'
        # launch via empty bios with -kernel fw_payload
        qemu_full_launch = [
            qemu_partial_launch
            + 'none -kernel '
            + kernel_firmwares[tag_nontest].full_path(),
        ][0]
    else
        # launch via normal bios with -kernel
        qemu_full_launch = [
            qemu_partial_launch
            + kernel_firmwares[tag_nontest].full_path()
            + ' -kernel '
            + exec_kernel_nontest.full_path(),
        ][0]
    endif

    target_qemu_launch = custom_target(
        name_qemu_launch,
        command: [
            script_make_qemu_launch,
            '@OUTPUT@',
            qemu_full_launch,
        ],
        output: name_qemu_launch,
        build_by_default: true,
        depends: [exec_kernel_nontest, kernel_firmwares[tag_nontest]],
    )
    message('qemu launch command written to', target_qemu_launch.full_path())
else
    warning('no machine arg passed, not generating launch script!')
endif

### tests
# TODO: use something similar to target_qemu_launch?
if option_tests
    if option_machine == ''
        error('must pass machine arg to enable tests')
    endif
    # parse exitcode from stdout
    test_parse_exitcode_str = '--parse-exitcode'

    if opensbi_fw_type == 'payload'
        # launch via empty bios with -kernel fw_payload
        qemu_full_launch = [
            qemu_partial_launch
            + 'none -kernel '
            + kernel_firmwares[tag_test].full_path(),
        ][0]
    else
        # launch via normal bios and kernel as -kernel
        qemu_full_launch = [
            qemu_partial_launch
            + kernel_firmwares[tag_test].full_path()
            + ' -kernel '
            + exec_kernel_test.full_path(),
        ][0]
    endif

    # create individual tests
    foreach test : target_tests
        test(
            'kernel test '
            + test.replace('_test__', '').replace('_should_fail', ''),
            script_run_single_test,
            args: [
                qemu_no_reboot ? test_parse_exitcode_str : '',
                qemu_full_launch + ' -append --test' + test,
            ],
            should_fail: test.contains('_should_fail'),
            verbose: true,
            depends: [exec_kernel_test, kernel_firmwares[tag_test]],
        )
    endforeach
else
    # always-failing test in case wrong options are set
    test(
        'tests not enabled',
        find_program('false'),
    )
endif

# TODO: use custom_target() for compdb?

# TODO: move gdbinit make here too
