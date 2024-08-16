const std = @import("std");
const print = @import("std").debug.print;

pub fn build(b: *std.Build) void {
    const optimize = b.standardOptimizeOption(.{});
    const target = b.standardTargetOptions(.{});

    const build_shared_libs = b.option(bool, "BUILD_SHARED_LIBS",
                "Build shared libraries (otherwise static ones)") orelse true;

    const dep_libyang = b.dependency("libyang", .{
        .target = target,
        .optimize = optimize,
        .BUILD_SHARED_LIBS = build_shared_libs,
    });

    const dep_pcre2 = b.dependency("pcre2", .{
        .target = target,
        .optimize = optimize,
        .linkage = .static,
    });


    var lib = b.addStaticLibrary(.{
        .name = "sysrepo",
        .target = target,
        .optimize = optimize,
    });
    if (build_shared_libs) {
        lib = b.addSharedLibrary(.{
            .name = "sysrepo",
            .target = target,
            .optimize = optimize,
        });
    }

    const source_files = [_][]const u8{
        "src/sysrepo.c",
        "src/common.c",
        "src/ly_wrap.c",
        "src/subscr.c",
        "src/log.c",
        "src/replay.c",
        "src/modinfo.c",
        "src/edit_diff.c",
        "src/lyd_mods.c",
        "src/context_change.c",
        "src/shm_main.c",
        "src/shm_ext.c",
        "src/shm_mod.c",
        "src/shm_sub.c",
        "src/sr_cond/sr_cond_futex.c",
        "src/plugins/ds_json.c",
        "src/plugins/ntf_json.c",
        "src/plugins/common_json.c",
        "src/utils/values.c",
        "src/utils/xpath.c",
        "src/utils/error_format.c",
        "src/utils/nacm.c",
        "src/utils/subscribed_notifications.c",
        "src/utils/sn_common.c",
        "src/utils/sn_yang_push.c",
    };

    lib.addCSourceFiles(.{
        .files = &source_files,
        .flags = &[_][]const u8{
            //"-D",
        },
    });

    const config_header = b.addConfigHeader(.{
        .style = .{ .cmake = b.path("src/config.h.in") },
        .include_path = "config.h"
        }, .{
        .INTERNAL_MODULE_DATA_FORMAT = "LYD_XML", // TODO: option? JSON?
        .NACM_RECOVERY_USER="root", // TODO: option
        .REPO_PATH="./repository", // TODO: improve?
        .SHM_DIR = "/dev/shm", // TODO: auto-detect freebsd = "/tmp/shm"
        .SYSREPO_SUPERUSER_UID=0, // TODO: option
        .SYSREPO_UMASK="000", // TODO: option?

        .DEFAULT_CANDIDATE_DS_PLG = "JSON DS file",
        .DEFAULT_FACTORY_DEFAULT_DS_PLG = "JSON DS file",
        .DEFAULT_NOTIFICAITON_DEFAULT_DS_PLG = "JSON DS file",
        .DEFAULT_OPERATIONAL_DS_PLG = "JSON DS file",
        .DEFAULT_RUNNING_DS_PLG = "JSON DS file",
        .DEFAULT_STARTUP_DS_PLG = "JSON DS file",
        .NACM_SRMON_DATA_PERM = "600",
        .SR_COND_IMPL = "sr_cond_futex", // TODO: auto-detect maybe?
        .SR_HAVE_DLOPEN = true, // TODO: auto-detect
        .SR_HAVE_EACCESS = true, // TODO: auto-detect
        .SR_PLUGINS_PATH = "/usr/local/lib/sysrepo/plugins",
        .YANGLIB_REVISION = "2019-01-04",
        },
                                            );

    const compat_header = b.addConfigHeader(.{
        .style = .{ .cmake = b.path("compat/compat.h.in") },
        .include_path = "compat.h"
        }, .{
        .HAVE_STRDUPA=1,
        .HAVE_CLOCK_MONOTONIC=1, // TODO: auto-detect, remove this because we directly set below?
        .COMPAT_CLOCK_ID="CLOCK_MONOTONIC", // TODO: auto-detect
    });

    const version_header = b.addConfigHeader(.{
        .style = .{ .cmake = b.path("src/version.h.in") },
        .include_path = "version.h"
        }, .{
        .SYSREPO_MAJOR_SOVERSION=7,
        .SYSREPO_MINOR_SOVERSION=27,
        .SYSREPO_MICRO_SOVERSION=1,
        .SYSREPO_SOVERSION_FULL="7.27.1",
        .SYSREPO_VERSION="7",
    });

    const bin_common_header = b.addConfigHeader(.{
        .style = .{ .cmake = b.path("src/executables/bin_common.h.in") },
        .include_path = "bin_common.h"
        }, .{
    });


    lib.addConfigHeader(config_header);
    lib.addConfigHeader(compat_header);
    lib.addConfigHeader(version_header);
    lib.addIncludePath(b.path("src"));
    lib.addIncludePath(b.path("src/plugins"));
    lib.linkLibrary(dep_libyang.artifact("yang"));
    lib.linkLibrary(dep_pcre2.artifact("pcre2-8"));
    //lib.linkSystemLibrary("libyang");
    lib.linkLibC();

    lib.installLibraryHeaders(dep_libyang.artifact("yang"));
    lib.linkLibrary(dep_libyang.artifact("yang"));

    lib.installHeader(b.path("src/sysrepo.h"), "sysrepo.h");
    lib.installHeader(compat_header.getOutput(), "sysrepo/compat.h");
    b.installArtifact(lib);

    const sysrepoctl = b.addExecutable(.{
        .name = "sysrepoctl",
        .target = target,
        .optimize = optimize,
    });
    sysrepoctl.addCSourceFiles(.{
        .files = &[_][]const u8{
            "src/executables/sysrepoctl.c",
        },
        .flags = &[_][]const u8{
        },
    });
    sysrepoctl.addConfigHeader(compat_header);
    sysrepoctl.addConfigHeader(bin_common_header);
    sysrepoctl.addIncludePath(b.path("src"));
    sysrepoctl.installLibraryHeaders(lib);
    sysrepoctl.installLibraryHeaders(dep_libyang.artifact("yang"));
    sysrepoctl.installLibraryHeaders(dep_pcre2.artifact("pcre2-8"));
    sysrepoctl.linkLibrary(lib);
    sysrepoctl.linkLibrary(dep_libyang.artifact("yang"));
    sysrepoctl.linkLibrary(dep_pcre2.artifact("pcre2-8"));
    sysrepoctl.linkLibC();
    b.installArtifact(sysrepoctl);
}
