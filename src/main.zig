//const testing = std.testing;
//
//export fn add(a: i32, b: i32) i32 {
//    return a + b;
//}
//
//test "basic add functionality" {
//    try testing.expect(add(3, 7) == 10);
//}

const c = @cImport({
    // zig c parser workaround functions
    @cInclude("utils.h");
    // emscripten
    @cInclude("emscripten.h");
    @cInclude("emscripten/fetch.h");
    // standard c headers (provided by emscripten)
    @cInclude("stdio.h");
    @cInclude("stdlib.h");
    // sdl2 headers (provided by emscripten)
    @cInclude("SDL2/SDL.h");
});

const std = @import("std");
const fmt = std.fmt;

const context = struct {
    renderer: *c.SDL_Renderer,
    iteration: i32,
};

fn mainloop(arg: ?*anyopaque) callconv(.C) void {
    //var ctx: *context = @ptrCast(*c.SDL_Renderer, arg);
    //var renderer: *c.SDL_Renderer = ctx.renderer;
    var renderer = @ptrCast(*c.SDL_Renderer, arg);

    // example: draw a moving rectangle

    // red background
    _ = c.SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    //std.debug.print("{}\n", .{ret});
    _ = c.SDL_RenderClear(renderer);
    //std.debug.print("{}\n", .{ret});

    // moving blue rectangle
    var r = c.SDL_Rect{
        //.x = ctx.iteration % 255,
        .x = 20,
        .y = 50,
        .w = 50,
        .h = 50,
    };
    _ = c.SDL_SetRenderDrawColor(renderer, 0, 255, 255, 255);
    //std.debug.print("{}\n", .{ret});
    _ = c.SDL_RenderFillRect(renderer, &r);
    //std.debug.print("{}\n", .{ret});

    _ = c.SDL_RenderPresent(renderer);

    //ctx.iteration += 1;
}

const Asset = struct {
    path: []const u8,
    status: bool,
    data: ?*u8 = null,
};

const GameState = struct {
    //assets: *std.array_list.ArrayListAligned(Asset, null),
    assets: *std.ArrayList(Asset),
    //window: *c.SDL_Window,
    //renderer: *c.SDL_Renderer,
};

fn downloadSucceeded(result: ?*c.emscripten_fetch_t) callconv(.C) void {
    var asset = @ptrCast(*Asset, @alignCast(@alignOf(*Asset), c.emscripten_fetch_t_get_userData(result)));
    _ = c.printf("Asset Path: %s\n", asset.path);
    _ = c.printf("Downloaded file: %s", c.emscripten_fetch_t_get_url(result));
    _ = c.printf(" (%d bytes)\n", c.emscripten_fetch_t_get_numBytes(result));
    _ = c.emscripten_fetch_close(result);
}

fn downloadAssets() *std.ArrayList(Asset) {
    var general_purpose_allocator = std.heap.GeneralPurposeAllocator(.{}){};
    var assets = std.ArrayList(Asset).init(general_purpose_allocator.allocator());
    assets.append(Asset{ .path = "/res/rpg16/default_grass.png", .status = false }) catch unreachable;
    assets.append(Asset{ .path = "/res/rpg16/default_sand.png", .status = false }) catch unreachable;
    //var textures = [_][]const u8{ "/res/rpg16/default_grass.png", "/res/rpg16/default_sand.png" };
    //u

    for (assets.items) |a, i| {
        var attr: c.emscripten_fetch_attr_t = undefined;

        c.emscripten_fetch_attr_init(&attr);

        //_ = fmt.bufPrint(attr.requestMethod[0..], "GET", .{}) catch return;
        _ = c.sprintf(&attr.requestMethod, "GET");

        //attr.attributes = c.EMSCRIPTEN_FETCH_LOAD_TO_MEMORY | c.EMSCRIPTEN_FETCH_SYNCHRONOUS;
        attr.attributes = c.EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;

        attr.onsuccess = downloadSucceeded;
        attr.userData = @ptrCast(*anyopaque, &assets.items[i]);

        _ = c.emscripten_fetch(&attr, @ptrCast([*c]const u8, a.path));
        //const status = c.emscripten_fetch_wait(result, std.math.inf(f64));
        //_ = c.printf("status: %d\n", status);
        //_ = c.printf("%d\n", c.emscripten_fetch_t_get_id(result));
        //_ = c.printf("%s\n", c.emscripten_fetch_t_get_url(result));
        //_ = c.printf("%d\n", c.emscripten_fetch_t_get_numBytes(result));
    }

    return &assets;
}

export fn main() void {
    var game_state = GameState{
        .assets = downloadAssets(),
    };

    _ = c.SDL_Init(c.SDL_INIT_VIDEO);
    //std.debug.print("{}\n", .{ret});

    var window: ?*c.SDL_Window = c.SDL_CreateWindow("test", c.SDL_WINDOWPOS_UNDEFINED, c.SDL_WINDOWPOS_UNDEFINED, 1000, 1000, 0).?;
    var renderer: *c.SDL_Renderer = c.SDL_CreateRenderer(window, -1, 0).?;

    //ret = c.SDL_CreateWindowAndRenderer(255, 255, 0, &window, &renderer);
    //std.debug.print("{}\n", .{ret});

    //var ctx = context{
    //    .renderer = renderer,
    //    .iteration = 0,
    //};

    const simulate_infinite_loop: c_int = 1; // call the function repeatedly
    const fps: c_int = -1; // call the functios as fast as the browser want to render (typically 60fps)
    //c.emscripten_set_main_loop_arg(mainloop, @ptrCast(*anyopaque, renderer), fps, simulate_infinite_loop);
    c.emscripten_set_main_loop_arg(mainloop, @ptrCast(*anyopaque, &game_state), fps, simulate_infinite_loop);

    c.SDL_DestroyRenderer(renderer);
    c.SDL_DestroyWindow(window);
    c.SDL_Quit();

    //return 0;
}
