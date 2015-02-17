#include "gradient.h"
#include "rdsys.h"
#include <array>
#include <cassert>
#include <random>
#include <algorithm>
#include <iterator>
#include <iostream>

#include <SDL2/SDL.h>

using namespace std;

const int scale = 1;
const int size = 800;
const int WIDTH = size/scale, HEIGHT = size/scale;
SDL_Texture                   *tex      = nullptr; 
SDL_Renderer                  *renderer = nullptr;
SDL_Window                    *win      = nullptr;

RDSys<size> rdSys(.2, .5, .1, .8, .01, .1);
array<uint32_t, WIDTH*HEIGHT>  img;

static array<uint32_t, 16> colors = {{
        0x008000, 0x800080, 0x008080, 0x000080,
        0xC0C0C0, 0x808080, 0x800000, 0x808000,  
        0x000000, 0xFFFFFF, 0xFF0000, 0x00FF00,
        0x0000FF, 0xFFFF00, 0x00FFFF, 0xFF00FF,}};

enum ERenderState {A, B};
ERenderState render_state = A;
unsigned long generation  = 0;


void shuffle_colors() {
    static random_device rd;
    static mt19937 g(rd());
    shuffle(colors.begin(), colors.end(), g);
}

template<int N>
std::array<double, N> calc_lut(std::array<double, size*size> active) {
    auto cdf(active);
    std::sort(cdf.begin(), cdf.end());

    double lut_min = cdf.front();
    assert(lut_min >= 0.0);
    std::array<double, N> lut;
    for (int i = 0; i < lut.size(); i++) {
	auto idx = (i * active.size()) / lut.size();
	lut[i] = cdf[idx];
    }
    return lut;
}
std::array<double, 100> lut;

void recalc_lut() {
    const auto& active = (A == render_state) ? rdSys.a : rdSys.b;
    lut = calc_lut<lut.size()>(active);
}

void reset_board() {
    for(auto& c : img) c = colors[0];
    rdSys = RDSys<size>(
			0.005, .02, .1980,
			.82, .084, .0061);
    generation = 0;
    for (unsigned int i = 0; i < lut.size(); i++) {
	lut[i] = 10 * i / double(lut.size());
    }
}

double lut_up(double v) {
    const static double lut_scale = 1.0 / double(lut.size());
    auto it = std::lower_bound(lut.begin(), lut.end(), v);
    if (lut.end() == it )
	return 1.0;
    if (lut.begin() == it)
	return 0.0;
    assert( v <= *it );

    auto d = std::distance(lut.begin(), it);
    double frac = 0.0;
    if ( (it+1) != lut.end()) {
	double w = *it - v ;
	assert( w >= 0);
	double len = *(it +1) - *it;
	assert(len >= 0);
	w = std::min(w, len);
	assert(len >= w);
	frac = 1.0 - w/len;
	assert(frac >= 0);
	assert(frac <= 1.0);
    }
    return (double(d) + frac ) * lut_scale;
}

void render() {
    const auto& active = (A == render_state) ? rdSys.a : rdSys.b;

    for(int i = 0; i < WIDTH * HEIGHT; i++) {
    	img[i] = GRADIENT(lut_up(active[i]));
    }

    int prev_line = (generation-1)%HEIGHT;
    for(int i = 0; i < WIDTH; i++)
	img[prev_line * WIDTH + i] = colors[15];
    
    // draw
    SDL_UpdateTexture(tex, NULL, img.data(), WIDTH * sizeof(uint32_t));
    SDL_Rect rect = {0,0, scale * WIDTH, scale * HEIGHT};
    SDL_RenderCopy(renderer, tex, NULL, &rect);
}

int main(int argc, char **argv) {

    if (0 != SDL_Init(SDL_INIT_EVERYTHING) ) exit(42);
    SDL_CreateWindowAndRenderer(scale * WIDTH, scale * HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL, &win, &renderer);
    if(nullptr == win || nullptr == renderer) exit(42);
    
    tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
    SDL_UpdateTexture(tex, NULL, img.data(), WIDTH * sizeof(uint32_t));

    int generations_pr_frame = 1;
    
    reset_board();
    
    SDL_Event e;
    float fps = 10.0;
    auto start_time = SDL_GetTicks();

    while(1) {
        for(int i = 0; i < generations_pr_frame; i++) {
	    rdSys.tick(static_cast<int>((generation + i )%size));
        }
        generation += generations_pr_frame;

        // calc FPS
        auto end_time = SDL_GetTicks();
        auto dt = end_time - start_time;
        // limit FPS
        while (dt < 10) {
            SDL_Delay(10);
            end_time = SDL_GetTicks();
            dt = end_time - start_time;
        }
        fps = fps * .9 + .1 * (1000.0 / dt);
        start_time = end_time;

        // update texture
        render();

        // check input
        while (SDL_PollEvent(&e)){
            if (e.type == SDL_QUIT || SDLK_q == e.key.keysym.sym) exit(0);
            if (SDLK_UP   == e.key.keysym.sym) generations_pr_frame  *= 2;
            if (SDLK_DOWN == e.key.keysym.sym) generations_pr_frame  /= 2;
            if (SDLK_c    == e.key.keysym.sym) shuffle_colors();
	    if (SDLK_a    == e.key.keysym.sym) render_state = A;
	    if (SDLK_b    == e.key.keysym.sym) render_state = B;
            if (SDLK_k    == e.key.keysym.sym) {reset_board();}
	    if (SDLK_r    == e.key.keysym.sym) {recalc_lut();}

            generations_pr_frame = max(1, generations_pr_frame);
        }

        // display
        char buf[512];
        sprintf(buf, "%s %lu generation (%3d ms, %.2f fps) %d gpf",
                argv[0], generation, dt, fps, generations_pr_frame);
        SDL_SetWindowTitle(win, buf);
        SDL_RenderPresent(renderer);

    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}

