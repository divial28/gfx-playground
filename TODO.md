# general
- [X] orginize files
- [X] multiple contexts render
- [X] multiple contexts event handling
- [X] extract backend code into separated framework
- [X] fix issue with black rects after imgui context deletion
  it was a problewm with texture, made it global and init just one time
- [X] dpi (just use different bigger font)
- [ ] imgui config save for each context
- [X] spdlog logging
- [ ] ? resource manager

# graphics
- [X] hello triangle
- [ ] things from red book
  - [X] different commands
  - [ ] instancing
  - [ ] transform feedback
- [X] gl framework
  - [X] shader load/compile/link/destroy
  - [ ] texture load/destroy
  - [ ] buffers routine
    - [ ] vao
    - [ ] vbo
    - [ ] ebo
    - [ ] ...
  - [ ] render pass 
    - [ ] draw commands
    - [ ] render pipeline
- [ ] basic stuff
  - [ ] camera with different modes
  - [ ] basic ray casting (needs camera)
