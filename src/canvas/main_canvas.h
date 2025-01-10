#pragma once

#include "canvas.h"
#include <string>
#include <vector>

class MainCanvas : public Canvas
{
public:
    MainCanvas();
    ~MainCanvas();

    void BuildUI() override;
    void Render() override;

private:
    void FillExamplesVector();
    void TableRow(size_t i);

private:
    using Initializer = Canvas*(*)();
    struct Example
    {
        Example (std::string_view title, Initializer&& initializer)
            : title(title), initializer(std::move(initializer)) {}

        std::string title;
        Initializer initializer;
        Canvas* canvas = nullptr;
    };
    std::vector<Example> examples_;

    
};