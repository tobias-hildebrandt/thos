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

compiler = meson.get_compiler('c')

### scripts
script_list_sources = 'misc/list_sources.sh'
script_user_bin = 'misc/user-bin.sh'
script_user_asm = 'misc/user-asm.sh'
script_find_libgcc = 'misc/find_libgcc.sh'

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

### includes (meson has default implicit includes)
include_common = include_directories(dir_base / subdir_common)
include_libc = include_directories(dir_base / subdir_libc, is_system: true)
include_userlib = include_directories(dir_base / subdir_userlib)

# must be manually included in build targets, no project_includes() in meson
includes_proj = [include_common, include_libc]

### compiler and linker args
# machine-specific args, passed to both compiler and linker
args_machine = [
    '-mcmodel=medany',
]
# project-wide link arguments
link_args_proj = [
    '-nostdlib',
]
# project-wide c arguments
c_args_proj = [
    # embedded system args
    '-ffreestanding',
    '-nostdinc',
    '-fno-stack-protector',
    # warning args
    '-Wformat=2',
]

# clang c arguments
c_args_clang = [
    '-Wno-gnu-zero-variadic-macro-arguments',
]
# gcc c arguments
c_args_gcc = []

if target_machine.cpu() == 'riscv64'
    args_machine += ['-march=rv64g', '-mabi=lp64d']
    c_args_proj += '-DQEMU64=1'
elif target_machine.cpu() == 'riscv32'
    args_machine += ['-march=rv32g', '-mabi=ilp32d']
    c_args_proj += '-DQEMU32=1'
else
    error('unknown host_machine architecture')
endif

# command-line defines
option_defines_str = 'defines'
option_defines = get_option(option_defines_str)
if option_defines != ''
    foreach define : option_defines.split(' ')
        c_args_proj += '-D' + define
    endforeach
endif

# TODO: build libgcc / llvm compiler-rt as subprojects?

# add libgcc
archive_libgcc = run_command(
    script_find_libgcc,
    env: {'MACHINE_ARGS': ' '.join(args_machine)},
    check: true,
).stdout().strip()
dep_libgcc = declare_dependency(link_args: archive_libgcc)

if compiler.get_id() == 'clang'
    args_machine += '--target=' + target_machine.cpu() + '-unknown-elf'
    c_args_proj += c_args_clang
elif compiler.get_id() == 'gcc'
    c_args_proj += c_args_gcc
endif

c_args_proj = args_machine + c_args_proj
link_args_proj = args_machine + link_args_proj

absolute_source_dir = meson.project_source_root() / 'src'
kernel_link_args = ['-Wl,-T' + (absolute_source_dir / 'kernel' / 'kernel.lds')]
user_link_args = ['-Wl,-T' + (absolute_source_dir / 'user' / 'user.lds')]

add_project_arguments(c_args_proj, language: 'c')
add_project_link_arguments(link_args_proj, language: 'c')
add_project_dependencies(dep_libgcc, language: 'c')

### targets
# libc
target_libc = static_library(
    'libc',
    dict_c_sources[subdir_libc],
    include_directories: includes_proj,
)
dep_libc = declare_dependency(link_with: target_libc)

# userlib
target_userlib = static_library(
    'userlib',
    dict_c_sources[subdir_userlib],
    include_directories: includes_proj,
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
        include_directories: [includes_proj, include_userlib],
        link_args: [user_link_args],
    )

    # expanded user bin
    target_userexec_bin = custom_target(
        name_userexec_bin,
        command: ['misc/user-bin.sh', target_userexec_exec, '@OUTPUT@'],
        output: name_userexec_bin,
        depends: target_userexec_exec,
    )

    # assembly that .incbin's the user bin
    target_userexec_assembly = custom_target(
        name_userexec_assembly,
        command: ['misc/user-asm.sh', target_userexec_bin, '@OUTPUT@', basename],
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

# kernel itself
exec_kernel = executable(
    'kernel',
    dict_c_sources[subdir_kernel],
    dependencies: [dep_libc, deps_userexec_blobs],
    include_directories: includes_proj,
    link_args: [kernel_link_args],
)

message('final kernel path:', exec_kernel.full_path())

# TODO: add tests

# TODO: build opensbi here also?
