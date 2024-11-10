#pragma once
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <iostream>
#include <mutex>
#define SETUP_SDL_OPENGL
#include "atlas_ui3.0.h"
#include "atlas_ui_utilities.h"
#pragma comment(linker,"/ENTRY:mainCRTStartup")

class UI {
public:
    UI() : bottomText("") {}
    std::string UIText;
    void renderUI();
    void endUI();
    void updateText(const std::string& text);
    void clearScreen();
private:
    std::string bottomText;
    std::mutex uiMutex; // Mutex to protect UIText
};

// UI.h
void UI::renderUI() {
    Atlas::Setup("Endless-Online Auto Updater", 800, 600);
    Atlas::initOpenGL();
    Atlas::setProjectionMatrix(800, 600);
    Atlas::TextRenderer::SetGlobalFont("C:/Windows/Fonts/arial.ttf");

    bool quit = false;
    SDL_Event event;

    while (!quit) {
        while (SDL_PollEvent(&event) != 0) {
            if (event.type == SDL_QUIT) {
                quit = true;
            }

            Atlas::handleEvents(&event);
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED) {
                int newWidth = event.window.data1;
                int newHeight = event.window.data2;
                Atlas::setProjectionMatrix(newWidth, newHeight);
                glViewport(0, 0, newWidth, newHeight);
            }
        }

        glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // Clear color
        glClear(GL_COLOR_BUFFER_BIT); // Clear the screen

        // Calculate the centered position for the upper text
        float textSize = 30.0f; // Increase text size
        float textWidth = textSize * 20; // Assuming each character is approximately textSize wide
        float textHeight = textSize; // Assuming text height is approximately textSize

        float upperTextX = (800 - textWidth) / 2.0f;
        float upperTextY = 100.0f; // Bring the text down a bit

        Atlas::createWidget(1, 0, 0, 800, 600, Atlas::WidgetOptions::WIDGET_NONE, "UPDATER_GFX/test.png");
        Atlas::Text("Endless-Online Updater", textSize, upperTextX, upperTextY);

        // Calculate the centered position for the bottom text
        float centerTextX = (800 - textWidth) / 2.0f;
        float centerTextY = (600 - textHeight) / 2.0f;

        {
            std::lock_guard<std::mutex> lock(uiMutex);
            Atlas::Text(UIText.c_str(), textSize, centerTextX, centerTextY);
        }

        Atlas::endWidget();

        // Finalize UI rendering
        Atlas::renderUI();

        // Swap buffers to display the frame
        SDL_GL_SwapWindow(Atlas::g_window);
    }

    SDL_StopTextInput();
    Atlas::Shutdown();
    return;
}



void UI::clearScreen()
{
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // Clear color
    glClear(GL_COLOR_BUFFER_BIT); // Clear the screen
    //force ui to reset for updating text
	SDL_GL_SwapWindow(Atlas::g_window);
}


void UI::endUI() {
    // Cleanup code if needed
}

void UI::updateText(const std::string& text) {
    std::lock_guard<std::mutex> lock(uiMutex);
    UIText = text;
    clearScreen();
}
