#include "app.h"
#include "canvas/main_canvas.h"

int main(int argc, char** argv)
{
    App app(argc, argv);
    App::OpenWindow(new MainCanvas());
    return app.Exec();
}
