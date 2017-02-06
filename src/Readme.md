# Known Bottlenecks

If many enabled widgets (including panels) are overlapping at the same time, lag can ensue.




Program:

Panel p;
Button[] b;

p.create();
b[...].create(anchor.to(p).inside(p))

No clue how to fix it.

# Design Considerations

Should the basic unit of measurement be the window and not the pixel?
+ Better scaling
- Bad accuracy and stable scaling.
+ (Pixel:) good accuracy <--

Should the basic unit of the GUI be the window or the panel?
+ (Panel:) flexibility on type of window (movable, x, dialog)
+ (Window:) unified gui for window. <--

Should the Canvas output vertex buffers or primitives?
+ (Primitives:) More backends supported <--
+ (Vertex Buffers:) Less iteration per frame, faster

How should ids be worked?
- (In situ) this could work, except it doesn't take into account multiple panels
- (String id) store in hash map? with complex data types it's hard to work out.
- (pointers as id) unsafe.
- (each input type stores there own hashmaps) awkward imgui but works <--
- (RMGUI: make objects and store them) - hard to recurse.

ONLY STORE STATE BETWEEN FRAMES IN THE BACKEND.
- The gui button should not hold state - it is instead a representation OF the state.
- Subscribe to events in order to return them.

drawComponent(this.state, subscribe(hoverstacked("mycompnent"), cursoractive(bool)))
Within drawComponent:
events.override(cursoractive);
events.override(hoverstacked, mycomponent);

Then, at the end, after everything is done...

string x = gui.events.get(hoverstacked);

if cursoractive match string {
    if "mycomponent" set buttonon to !buttonon
}

Do not hold state - the client or component wrapper should hold the state.
All state should be released before the next frame - wrappers can release state after two frames instead.
Clients should always keep polling the state or the wrapped state in order to update their state to change the rendered component.



# Planned features

The gui context is a struct* with some basic api

It is updated like `gui.update(events...)`
Then, you call your own functions like `Button(gui, &data);`

Store a context-type for panel so this can be done: (finding hovered window) - exclude widgets on windows that are underneath.

Store bounds on the side of the client

vector<Bounds>


Events in an event context
Gui in a gui context
Each tree node/header node/panel has it's own gui context for it's widgets

```
static ImGuiWindow* FindHoveredWindow(ImVec2 pos, bool excluding_childs)
{
    ImGuiContext& g = *GImGui;
    for (int i = g.Windows.Size-1; i >= 0; i--)
    {
        ImGuiWindow* window = g.Windows[i]; // track widgets in context - which means one context per... child widget?
        if (!window->Active)
            continue;
        if (window->Flags & ImGuiWindowFlags_NoInputs)
            continue;
        if (excluding_childs && (window->Flags & ImGuiWindowFlags_ChildWindow) != 0)
            continue;

        // Using the clipped AABB so a child window will typically be clipped by its parent.
        ImRect bb(window->WindowRectClipped.Min - g.Style.TouchExtraPadding, window->WindowRectClipped.Max + g.Style.TouchExtraPadding);
        if (bb.Contains(pos))
            return window;
    }
    return NULL;
}

```
```
environmentcontext ctx;
ctx.setup();

guictx ptx;

panel(ctx, ptx, [&](){

});

Given vector t (target) with members m (mag) & d (dir) and ray with d (dir) we take the angle between t.d and r.d as a.

If a = 0; we set magnitude to 1.
if a= 180 we set magnitude to -1
if a = 90 we set magnitude to 0.

so divide by 90 and subtract 1 to get the magnitude of the motor vector.





```

/*

mscroll = 0;
glfwPollEvents();

glfwGetWindowSize(win, &width, &height);
glViewport(0, 0, width, height);

glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

// Draw UI
double mousex; double mousey;
glfwGetCursorPos(win, &mousex, &mousey);
mousey = height - mousey;
int leftButton = glfwGetMouseButton( win, GLFW_MOUSE_BUTTON_LEFT );
//int rightButton = glfwGetMouseButton( win, GLFW_MOUSE_BUTTON_RIGHT );
//int middleButton = glfwGetMouseButton( win, GLFW_MOUSE_BUTTON_MIDDLE );
int toggle = 0;

gui.update((int)mousex, (int)mousey, leftButton != GLFW_RELEASE, mscroll);

gui.panel("Scroll area", 80, 10, width / 5, height - 20, &scrollarea1, [&]() {
    gui.separatorLine();
    gui.separator();

    gui.button("Button");
    gui.button("Disabled button", false);
    gui.item("Item");
    gui.item("Disabled item", false);
    toggle = gui.check("Checkbox", checked1);
    if (toggle) checked1 = !checked1;
    toggle = gui.check("Disabled checkbox", checked2, false);
    if (toggle) checked2 = !checked2;
    toggle = gui.collapse("Collapse", "subtext", checked3);
    if (checked3)
    {
        gui.indent();
        gui.label("collapsed item");
        gui.unindent();
    }
    if (toggle) checked3 = !checked3;
    toggle = gui.collapse("Disabled collapse", "subtext", checked4, false);
    if (toggle) checked4 = !checked4;
    gui.label("Label");
    gui.value("Value");
    gui.slider("Slider", &axis_z, 0.f, 100.f, 1.f);
    gui.slider("Disabled slider", &axis_x, 0.f, 100.f, 1.f, false);
    gui.indent();
    gui.label("Indented");
    gui.unindent();
    gui.label("Unindented");
});


gui.panel("Scroll area", 20 + width / 5, 500, width / 5, height - 510, &scrollarea2, [&]() {
    gui.separatorLine();
    gui.separator();
    for (int i = 0; i < 100; ++i) gui.label("A wall of text");
});

canvas.addText(30 + width / 5 * 2, height - 20, IMGUI_ALIGN_LEFT, "Free text",  imguiRGBA(32,192, 32,192));
canvas.addText(30 + width / 5 * 2 + 100, height - 40, IMGUI_ALIGN_RIGHT, "Free text",  imguiRGBA(32, 32, 192, 192));
canvas.addText(30 + width / 5 * 2 + 50, height - 60, IMGUI_ALIGN_CENTER, "Free text",  imguiRGBA(192, 32, 32,192));

canvas.addLine(30 + width / 5 * 2, height - 80, 30 + width / 5 * 2 + 100, height - 60, 1.f, imguiRGBA(32,192, 32,192));
canvas.addLine(30 + width / 5 * 2, height - 100, 30 + width / 5 * 2 + 100, height - 80, 2.f, imguiRGBA(32, 32, 192, 192));
canvas.addLine(30 + width / 5 * 2, height - 120, 30 + width / 5 * 2 + 100, height - 100, 3.f, imguiRGBA(192, 32, 32,192));

canvas.addRoundedRect(30 + width / 5 * 2, height - 240, 100, 100, 5.f, imguiRGBA(32,192, 32,192));
canvas.addRoundedRect(30 + width / 5 * 2, height - 350, 100, 100, 10.f, imguiRGBA(32, 32, 192, 192));
canvas.addRoundedRect(30 + width / 5 * 2, height - 470, 100, 100, 20.f, imguiRGBA(192, 32, 32,192));

canvas.addRect(30 + width / 5 * 2, height - 590, 100, 100, imguiRGBA(32, 192, 32, 192));
canvas.addRect(30 + width / 5 * 2, height - 710, 100, 100, imguiRGBA(32, 32, 192, 192));
canvas.addRect(30 + width / 5 * 2, height - 830, 100, 100, imguiRGBA(192, 32, 32,192));

implgui.draw(width, height, &canvas);


// Check for errors
GLenum err = glGetError();
if(err != GL_NO_ERROR)
{
    //fprintf(stderr, "OpenGL Error : %s\n", gluErrorString(err));
}

// Swap buffers
glfwSwapBuffers(win);*/
