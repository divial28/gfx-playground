#pragma once

class Canvas
{
public:
    Canvas() {};
    virtual ~Canvas() {};
    Canvas(const Canvas&) = delete;
    Canvas(Canvas&&) = delete;
    Canvas& operator=(const Canvas&) = delete;
    Canvas& operator=(Canvas&&) = delete;
    
    virtual void BuildUI() = 0;
    virtual void Render() = 0;
};
