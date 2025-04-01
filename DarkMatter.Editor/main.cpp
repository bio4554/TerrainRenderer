// DarkMatter.Editor.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <SDL3/SDL.h>

#include "DMEditor.h"
#include "DMEngine.h"

int main()
{
    int width = 2560;
    int height = 1440;

    if (SDL_Init(SDL_INIT_VIDEO) == false)
    {
        auto err = SDL_GetError();
        std::cerr << "SDL_Init error: " << err << '\n';
        return 1;
    }

    auto sdlWindow = SDL_CreateWindow("SDL3 Window", width, height, SDL_WINDOW_RESIZABLE);

    if (sdlWindow == nullptr)
    {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << '\n';
        SDL_Quit();
        return 1;
    }

    try
    {
        dm::editor::Editor editor(sdlWindow, width, height);

        editor.Run();
    }
    catch (std::runtime_error& e)
    {
        std::cout << "Caught exception: " << e.what() << "\n";
    }
    catch (...)
    {
        std::cout << "Caught unhandled exception\n";
    }

    if (sdlWindow != nullptr)
    {
        SDL_DestroyWindow(sdlWindow);
        sdlWindow = nullptr;
    }

    SDL_Quit();

    return 0;
}
