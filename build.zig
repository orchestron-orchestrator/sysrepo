const std = @import("std");
const print = @import("std").debug.print;

pub fn build(b: *std.Build) void {
    const optimize = b.standardOptimizeOption(.{});
    const target = b.standardTargetOptions(.{});

    const lib = b.addStaticLibrary(.{
        .name = "sysrepo",
        .target = target,
        .optimize = optimize,
    });

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

    const config_header = b.addConfigHeader(
        .{
            .style = .blank,
        },
        .{
	    .SR_HAVE_EACCESS = true,
            .SR_HAVE_DLOPEN = true,
            .SR_PLG_PATH = "/usr/local/lib/sysrepo/plugins",
            .SR_PLG_SUFFIX = ".so",
            .SR_PLG_SUFFIX_LEN = 3,
            .SR_SU_SUID = 0,
            .SR_NACM_RECOVERY_USER = "root",
            //.SR_YANGLIB_REVISION = 2019-01-04,
            .SR_REPO_PATH = "./build/repository",
            .SR_REPO_PATH_ENv = "SYSREPO_REPOSITORY_PATH",
            .SR_STARTUP_PATH = "",
            .SR_FACTORY_DEFAULT_PATH = "",
            .SR_NOTIFICATION_PATH = "",
            .SR_YANG_PATH = "",
            .SR_INIT_MOD_DATA = "",
            //.SR_INIT_MOD_DATA_FORMAT = LYD_XML ,
            .SR_INT_MOD_DISABLED_RUNNING = "",
            .SR_SHM_DIR = "/dev/shm",
            .SR_SHM_PREFIX_DEFAULT = "sr",
            .SR_SHM_PREFIX_ENV = "SYSREPO_SHM_PREFIX",
            .SR_GROUP = "",
            //.SR_UMASK = 00000,
            //.SR_DIR_PERM = 00777,
            //.SR_PLG_DIR_PERM = 00770,
            //.SR_YANG_PERM = 00644,
            //.SR_FILE_PERM = 00600,
            //.SR_INTMOD_MAIN_FILE_PERM = 00666,
            //.SR_INTMOD_NACM_SRMON_FILE_PERM = 00600,
            //.SR_INTMOD_NODATA_FILE_PERM = 00444,
            //.SR_INTMOD_WITHDATA_FILE_PERM = 00644,
            .SR_DEFAULT_STARTUP_DS = "JSON DS file",
            .SR_DEFAULT_RUNNING_DS = "JSON DS file",
            .SR_DEFAULT_CANDIDATE_DS = "JSON DS file",
            .SR_DEFAULT_OPERATIONAL_DS = "JSON DS file",
            .SR_DEFAULT_FACTORY_DEFAULT_DS = "JSON DS file",
            .SR_DEFAULT_NOTIFICATION_DS = "JSON DS file",
        },
    );

    lib.addConfigHeader(config_header);
    lib.addIncludePath(b.path("src"));
    lib.addIncludePath(b.path("src/plugins"));
    lib.linkSystemLibrary("libyang");
    lib.linkLibC();
    //b.installFile(".h", "include/utf8proc.h");
    b.installArtifact(lib);
}
