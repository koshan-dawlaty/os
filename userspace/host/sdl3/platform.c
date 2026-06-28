#include <SDL3/SDL.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_surface.h>
#include <SDL3/SDL_video.h>
#include <stdint.h>

struct wm_client
{
    SDL_Surface *surface;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    SDL_Window *win;
};

struct wm_client wm_client_connect(size_t height, size_t width)
{

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        /* TODO: error */
    }

    SDL_Window *window = SDL_CreateWindow("os", width, height, 0);
    if (!window)
    {
        /* TODO: error */

        SDL_Quit();
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer)
    {
        /* TODO: error */
        SDL_DestroyWindow(window);
        SDL_Quit();
    }
    SDL_Surface *surface =
        SDL_CreateSurface(width, height, SDL_PIXELFORMAT_RGBA8888);
    uint32_t *pixels = (uint32_t *)surface->pixels;
    memset(pixels, 127, width * height * 4);

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);

    return (struct wm_client){
        .renderer = renderer, .surface = surface, .texture = texture};
}

void wm_render(struct wm_client client)
{
    SDL_RenderClear(client.renderer);
    SDL_UpdateTexture(client.texture, NULL, client.surface->pixels,
                      client.surface->pitch);
    SDL_RenderTexture(client.renderer, client.texture, NULL, NULL);
    SDL_RenderPresent(client.renderer);
}
