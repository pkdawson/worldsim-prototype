#define SDL_MAIN_HANDLED
#include <SDL.h>
#pragma comment(lib, "SDL2.lib")
#pragma comment(lib, "libwsim.lib")

#include "common.hpp"
#include "wsim.hpp"

static const float tileSize = 2;
static const size_t chunkSize = CHUNK_SIZE;
static const float winSize = chunkSize * tileSize;

int main()
{
    SDL_Init(SDL_INIT_EVERYTHING);

    SDL_Window* window;
    SDL_Renderer* renderer;

    window = SDL_CreateWindow(
        "worldsim viewer",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        static_cast<int>(winSize),
        static_cast<int>(winSize),
        SDL_WINDOW_SHOWN
        );

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_RenderSetScale(renderer, tileSize, tileSize);

    shared_ptr<World> w = make_shared<World>(10000, 10000);
    unique_ptr<Game> g = make_unique<Game>(w);
    w->populate();

    SDL_Event evt;

    auto t1 = high_resolution_clock::now();
    int frame;
    for (frame = 0; frame < chunkSize*2; ++frame)
    {
        g->tick();

        // TODO: multithread

        while (SDL_PollEvent(&evt) == 1)
        {
            if (evt.type == SDL_QUIT)
            {
                goto end;
            }

            else if (evt.type == SDL_KEYDOWN)
            {
                SDL_Keycode key = evt.key.keysym.sym;

                if (key == SDLK_ESCAPE)
                    goto end;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_Rect r;
        r.w = 1;
        r.h = 1;

        int offset = 5000;
        WorldChunk& chunk = w->chunkAt(offset, offset);

        for (int y = 0; y < CHUNK_SIZE; ++y)
        {
            for (int x = 0; x < CHUNK_SIZE; ++x)
            {
                if (chunk.terrain(x, y).type == TerrainType::Wall)
                {
                    r.x = x;
                    r.y = y;

                    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
                    SDL_RenderFillRect(renderer, &r);
                }
            }
        }

        for (EntityHandle eh : chunk.entities)
        {
            Entity* e = EM->getEntity(eh);
            Position& pos = e->getComponent<PositionData>()->pos;

            r.x = pos.x - offset;
            r.y = pos.y - offset;

            if (e->hasComponent<PlantData>()) {
                SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
            }
            else if (e->hasComponent<CreatureData>()) {
                SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
            }
            else {
                SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255);
            }

            SDL_RenderFillRect(renderer, &r);
        }

        SDL_RenderPresent(renderer);
    }

end:
    auto t2 = high_resolution_clock::now();
    duration<double> time_span = duration_cast<duration<double>>(t2 - t1);

    cout << frame / time_span.count() << " fps" << endl;

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    EM->clear();
    return 0;
}
