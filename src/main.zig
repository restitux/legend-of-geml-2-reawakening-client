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
    @cInclude("SDL2/SDL.h");
    @cInclude("emscripten.h");
});

const std = @import("std");

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
    _ = c.SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
    //std.debug.print("{}\n", .{ret});
    _ = c.SDL_RenderFillRect(renderer, &r);
    //std.debug.print("{}\n", .{ret});

    _= c.SDL_RenderPresent(renderer);

    //ctx.iteration += 1;
}

export fn main() void {
    _ = c.SDL_Init(c.SDL_INIT_VIDEO);
    //std.debug.print("{}\n", .{ret});

    var window: *c.SDL_Window = c.SDL_CreateWindow("test", c.SDL_WINDOWPOS_UNDEFINED, c.SDL_WINDOWPOS_UNDEFINED, 255, 255, 0).?;
    var renderer: *c.SDL_Renderer = c.SDL_CreateRenderer(window, -1, 0).?;

    //ret = c.SDL_CreateWindowAndRenderer(255, 255, 0, &window, &renderer);
    //std.debug.print("{}\n", .{ret});

    //var ctx = context{
    //    .renderer = renderer,
    //    .iteration = 0,
    //};

    const simulate_infinite_loop: c_int = 1; // call the function repeatedly
    const fps: c_int = -1; // call the functios as fast as the browser want to render (typically 60fps)
    c.emscripten_set_main_loop_arg(mainloop, @ptrCast(*anyopaque, renderer), fps, simulate_infinite_loop);

    c.SDL_DestroyRenderer(renderer);
    c.SDL_DestroyWindow(window);
    c.SDL_Quit();

    //return 0;
}
