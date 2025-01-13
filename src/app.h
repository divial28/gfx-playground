#pragma once

class Canvas;

class App
{
public:
    App(int argc, char** argv);
    ~App();
    App(const App&) = delete;
    App(App&&) = delete;
    App& operator=(const App&) = delete;
    App& operator=(App&&) = delete;

    int Exec();

    static bool OpenWindow(Canvas* canvas);
    static bool IsOpened(Canvas* canvas);
    static bool CloseWindow(Canvas* canvas);
};