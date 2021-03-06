const std = @import("std");

pub fn build(b: *std.build.Builder) void {
    // Standard release options allow the person running `zig build` to select
    // between Debug, ReleaseSafe, ReleaseFast, and ReleaseSmall.
    const mode = b.standardReleaseOptions();

    const target = b.standardTargetOptions(.{});

    const lib = b.addStaticLibrary("gamejam", "src/main.c");
    lib.addIncludeDir("/home/robby/.emscripten_cache/sysroot/include");
    lib.addIncludeDir(".");
    lib.setBuildMode(mode);
    lib.setTarget(target);
    //lib.linkLibC();
    lib.install();

    //const main_tests = b.addTest("src/main.zig");
    //main_tests.setBuildMode(mode);

    //const test_step = b.step("test", "Run library tests");
    //test_step.dependOn(&main_tests.step);
}
