#include <iostream>
#include <Hale.h>
#include <glm/glm.hpp>
#include <GL/glu.h>

#include <nanogui/screen.h>
#include <nanogui/window.h>
#include <nanogui/layout.h>
#include <nanogui/label.h>
#include <nanogui/checkbox.h>
#include <nanogui/button.h>
#include <nanogui/toolbutton.h>
#include <nanogui/popupbutton.h>
#include <nanogui/combobox.h>
#include <nanogui/progressbar.h>
#include <nanogui/entypo.h>
#include <nanogui/messagedialog.h>
#include <nanogui/textbox.h>
#include <nanogui/slider.h>
#include <nanogui/imagepanel.h>
#include <nanogui/imageview.h>
#include <nanogui/vscrollpanel.h>
#include <nanogui/colorwheel.h>
#include <nanogui/graph.h>
#include <nanogui/tabwidget.h>

#include <nanogui/window.h>
#include <nanogui/theme.h>
#include <nanogui/opengl.h>
#include <nanogui/screen.h>
#include <nanogui/layout.h>
#include <nanogui/serializer/core.h>
#include <nanogui/formhelper.h>
#if defined(_WIN32)
#include <windows.h>
#endif
#include <nanogui/glutil.h>
#include <iostream>
#include <string>

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::to_string;


class VisApp : public nanogui::Screen {
public:
    /*
     * let the default size be 400x300. This works well on all resolutions.
    */
    VisApp(int argc, const char** argv) : nanogui::Screen(Eigen::Vector2i(400, 300), "Hale with NanoGUI") {
        setOptions(argc, argv);
        using namespace nanogui;
        Window *window = new Window(this, "Button demo");
        window->setPosition(Vector2i(15, 15));
        window->setLayout(new GroupLayout());

        /* No need to store a pointer, the data structure will be automatically
           freed when the parent window is deleted */
        new Label(window, "Push buttons", "sans-bold");

        Button *b = new Button(window, "Plain button");
        b->setCallback([] { cout << "pushed!" << endl; });

        /* Alternative construction notation using variadic template */
        b = window->add<Button>("Styled", ENTYPO_ICON_ROCKET);
        b->setBackgroundColor(Color(0, 0, 255, 25));
        b->setCallback([] { cout << "pushed!" << endl; });

        new Label(window, "Toggle buttons", "sans-bold");
        b = new Button(window, "Toggle me");
        b->setFlags(Button::ToggleButton);
        b->setChangeCallback([](bool state) { cout << "Toggle button state: " << state << endl; });

        new Label(window, "Radio buttons", "sans-bold");
        b = new Button(window, "Radio button 1");
        b->setFlags(Button::RadioButton);
        b = new Button(window, "Radio button 2");
        b->setFlags(Button::RadioButton);

        new Label(window, "A tool palette", "sans-bold");
        Widget *tools = new Widget(window);
        tools->setLayout(new BoxLayout(Orientation::Horizontal,
                                       Alignment::Middle, 0, 6));

        b = new ToolButton(tools, ENTYPO_ICON_CLOUD);
        b = new ToolButton(tools, ENTYPO_ICON_FF);
        b = new ToolButton(tools, ENTYPO_ICON_COMPASS);
        b = new ToolButton(tools, ENTYPO_ICON_INSTALL);

        new Label(window, "Popup buttons", "sans-bold");
        PopupButton *popupBtn = new PopupButton(window, "Popup", ENTYPO_ICON_EXPORT);
        Popup *popup = popupBtn->popup();
        popup->setLayout(new GroupLayout());
        new Label(popup, "Arbitrary widgets can be placed here");
        new CheckBox(popup, "A check box");
        popupBtn = new PopupButton(popup, "Recursive popup", ENTYPO_ICON_FLASH);
        popup = popupBtn->popup();
        popup->setLayout(new GroupLayout());
        new CheckBox(popup, "Another check box");

        window = new Window(this, "Basic widgets");
        window->setPosition(Vector2i(200, 15));
        window->setLayout(new GroupLayout());

        new Label(window, "Message dialog", "sans-bold");
        tools = new Widget(window);
        tools->setLayout(new BoxLayout(Orientation::Horizontal,
                                       Alignment::Middle, 0, 6));
        b = new Button(tools, "Info");
        b->setCallback([&] {
            auto dlg = new MessageDialog(this, MessageDialog::Type::Information, "Title", "This is an information message");
            dlg->setCallback([](int result) { cout << "Dialog result: " << result << endl; });
        });
        b = new Button(tools, "Warn");
        b->setCallback([&] {
            auto dlg = new MessageDialog(this, MessageDialog::Type::Warning, "Title", "This is a warning message");
            dlg->setCallback([](int result) { cout << "Dialog result: " << result << endl; });
        });
        b = new Button(tools, "Ask");
        b->setCallback([&] {
            auto dlg = new MessageDialog(this, MessageDialog::Type::Warning, "Title", "This is a question message", "Yes", "No", true);
            dlg->setCallback([](int result) { cout << "Dialog result: " << result << endl; });
        });

        std::vector<std::pair<int, std::string>>
            icons = loadImageDirectory(mNVGContext, "icons");

        new Label(window, "Image panel & scroll panel", "sans-bold");
        PopupButton *imagePanelBtn = new PopupButton(window, "Image Panel");
        imagePanelBtn->setIcon(ENTYPO_ICON_FOLDER);
        popup = imagePanelBtn->popup();
        VScrollPanel *vscroll = new VScrollPanel(popup);
        ImagePanel *imgPanel = new ImagePanel(vscroll);
        imgPanel->setImages(icons);

        popup->setFixedSize(Vector2i(245, 150));

        /// dvar, bar, strvar, etc. are double/bool/string/.. variables

        FormHelper *gui = new FormHelper(this);
        ref<Window> win = gui->addWindow(Eigen::Vector2i(10, 10), "Form helper example");
        gui->addGroup("Basic types");
        // bool* boolptr = new bool(true);
        bool boolptr = true;
        gui->addVariable<double>("boolean",
            [&](double v) { fprintf(stderr,"setting %f\n",v); },
            [&]() -> double { return 4.2; });
        // gui->addVariable("string", "strvar");

        // gui->addGroup("Validating fields");
        // gui->addVariable("int", 3);
        // gui->addVariable("float", 3.5);
        // gui->addVariable("double", 3.14);

        // gui->addGroup("Complex types");
        // gui->addVariable("Enumeration", enumval, enabled)
        //    ->setItems({"Item 1", "Item 2", "Item 3"});
        // gui->addVariable("Color", colval);

        // gui->addGroup("Other widgets");
        // gui->addButton("A button", [](){ std::cout << "Button pressed." << std::endl; });

        // other stuff

        auto img_window = new Window(this, "Selected image");
        img_window->setPosition(Vector2i(710, 15));
        img_window->setLayout(new GroupLayout());

        auto img = new ImageView(img_window);
        img->setPolicy(ImageView::SizePolicy::Expand);
        img->setFixedSize(Vector2i(275, 275));
        img->setImage(icons[0].first);
        imgPanel->setCallback([&, img, imgPanel, imagePanelBtn](int i) {
            img->setImage(imgPanel->images()[i].first); cout << "Selected item " << i << endl;
        });
        auto img_cb = new CheckBox(img_window, "Expand",
            [img](bool state) { if (state) img->setPolicy(ImageView::SizePolicy::Expand);
                                else       img->setPolicy(ImageView::SizePolicy::Fixed); });
        img_cb->setChecked(true);

        new Label(window, "File dialog", "sans-bold");
        tools = new Widget(window);
        tools->setLayout(new BoxLayout(Orientation::Horizontal,
                                       Alignment::Middle, 0, 6));
        b = new Button(tools, "Open");
        b->setCallback([&] {
            cout << "File dialog result: " << file_dialog(
                    { {"png", "Portable Network Graphics"}, {"txt", "Text file"} }, false) << endl;
        });
        b = new Button(tools, "Save");
        b->setCallback([&] {
            cout << "File dialog result: " << file_dialog(
                    { {"png", "Portable Network Graphics"}, {"txt", "Text file"} }, true) << endl;
        });

        new Label(window, "Combo box", "sans-bold");
        new ComboBox(window, { "Combo box item 1", "Combo box item 2", "Combo box item 3"});
        new Label(window, "Check box", "sans-bold");
        CheckBox *cb = new CheckBox(window, "Flag 1",
            [](bool state) { cout << "Check box 1 state: " << state << endl; }
        );
        cb->setChecked(true);
        cb = new CheckBox(window, "Flag 2",
            [](bool state) { cout << "Check box 2 state: " << state << endl; }
        );
        new Label(window, "Progress bar", "sans-bold");
        mProgress = new ProgressBar(window);

        new Label(window, "Slider and text box", "sans-bold");

        Widget *panel = new Widget(window);
        panel->setLayout(new BoxLayout(Orientation::Horizontal,
                                       Alignment::Middle, 0, 20));

        Slider *slider = new Slider(panel);
        slider->setValue(0.5f);
        slider->setFixedWidth(80);

        TextBox *textBox = new TextBox(panel);
        textBox->setFixedSize(Vector2i(60, 25));
        textBox->setValue("50");
        textBox->setUnits("%");
        slider->setCallback([textBox](float value) {
            textBox->setValue(std::to_string((int) (value * 100)));
        });
        slider->setFinalCallback([&](float value) {
            cout << "Final slider value: " << (int) (value * 100) << endl;
        });
        textBox->setFixedSize(Vector2i(60,25));
        textBox->setFontSize(20);
        textBox->setAlignment(TextBox::Alignment::Right);

        window = new Window(this, "Misc. widgets");
        window->setPosition(Vector2i(425,15));
        window->setLayout(new GroupLayout());

        TabWidget* tabWidget = window->add<TabWidget>();

        Widget* layer = tabWidget->createTab("Color Wheel");
        layer->setLayout(new GroupLayout());

        // Use overloaded variadic add to fill the tab widget with Different tabs.
        layer->add<Label>("Color wheel widget", "sans-bold");
        layer->add<ColorWheel>();

        layer = tabWidget->createTab("Function Graph");
        layer->setLayout(new GroupLayout());

        layer->add<Label>("Function graph widget", "sans-bold");

        Graph *graph = layer->add<Graph>("Some Function");

        graph->setHeader("E = 2.35e-3");
        graph->setFooter("Iteration 89");
        VectorXf &func = graph->values();
        func.resize(100);
        for (int i = 0; i < 100; ++i)
            func[i] = 0.5f * (0.5f * std::sin(i / 10.f) +
                              0.5f * std::cos(i / 23.f) + 1);

        // Dummy tab used to represent the last tab button.
        tabWidget->createTab("+");

        // A simple counter.
        int counter = 1;
        tabWidget->setCallback([tabWidget, this, counter] (int index) mutable {
            if (index == (tabWidget->tabCount()-1)) {
                // When the "+" tab has been clicked, simply add a new tab.
                string tabName = "Dynamic " + to_string(counter);
                Widget* layerDyn = tabWidget->createTab(index, tabName);
                layerDyn->setLayout(new GroupLayout());
                layerDyn->add<Label>("Function graph widget", "sans-bold");
                Graph *graphDyn = layerDyn->add<Graph>("Dynamic function");

                graphDyn->setHeader("E = 2.35e-3");
                graphDyn->setFooter("Iteration " + to_string(index*counter));
                VectorXf &funcDyn = graphDyn->values();
                funcDyn.resize(100);
                for (int i = 0; i < 100; ++i)
                    funcDyn[i] = 0.5f *
                        std::abs((0.5f * std::sin(i / 10.f + counter) +
                                  0.5f * std::cos(i / 23.f + 1 + counter)));
                ++counter;
                // We must invoke perform layout from the screen instance to keep everything in order.
                // This is essential when creating tabs dynamically.
                performLayout();
                // Ensure that the newly added header is visible on screen
                tabWidget->ensureTabVisible(index);

            }
        });
        tabWidget->setActiveTab(0);

        // A button to go back to the first tab and scroll the window.
        panel = window->add<Widget>();
        panel->add<Label>("Jump to tab: ");
        panel->setLayout(new BoxLayout(Orientation::Horizontal,
                                       Alignment::Middle, 0, 6));

        auto ib = panel->add<IntBox<int>>();
        ib->setEditable(true);

        b = panel->add<Button>("", ENTYPO_ICON_FORWARD);
        b->setFixedSize(Vector2i(22, 22));
        ib->setFixedHeight(22);
        b->setCallback([tabWidget, ib] {
            int value = ib->value();
            if (value >= 0 && value < tabWidget->tabCount()) {
                tabWidget->setActiveTab(value);
                tabWidget->ensureTabVisible(value);
            }
        });

        window = new Window(this, "Grid of small widgets");
        window->setPosition(Vector2i(425, 300));
        GridLayout *layout =
            new GridLayout(Orientation::Horizontal, 2,
                           Alignment::Middle, 15, 5);
        layout->setColAlignment(
            { Alignment::Maximum, Alignment::Fill });
        layout->setSpacing(0, 10);
        window->setLayout(layout);

        {
            new Label(window, "Floating point :", "sans-bold");
            textBox = new TextBox(window);
            textBox->setEditable(true);
            textBox->setFixedSize(Vector2i(100, 20));
            textBox->setValue("50");
            textBox->setUnits("GiB");
            textBox->setDefaultValue("0.0");
            textBox->setFontSize(16);
            textBox->setFormat("[-]?[0-9]*\\.?[0-9]+");
        }

        {
            new Label(window, "Positive integer :", "sans-bold");
            textBox = new TextBox(window);
            textBox->setEditable(true);
            textBox->setFixedSize(Vector2i(100, 20));
            textBox->setValue("50");
            textBox->setUnits("Mhz");
            textBox->setDefaultValue("0.0");
            textBox->setFontSize(16);
            textBox->setFormat("[1-9][0-9]*");
        }

        {
            new Label(window, "Checkbox :", "sans-bold");

            cb = new CheckBox(window, "Check me");
            cb->setFontSize(16);
            cb->setChecked(true);
        }

        new Label(window, "Combo box :", "sans-bold");
        ComboBox *cobo =
            new ComboBox(window, { "Item 1", "Item 2", "Item 3" });
        cobo->setFontSize(16);
        cobo->setFixedSize(Vector2i(100,20));

        new Label(window, "Color button :", "sans-bold");
        popupBtn = new PopupButton(window, "", 0);
        popupBtn->setBackgroundColor(Color(255, 120, 0, 255));
        popupBtn->setFontSize(16);
        popupBtn->setFixedSize(Vector2i(100, 20));
        popup = popupBtn->popup();
        popup->setLayout(new GroupLayout());

        ColorWheel *colorwheel = new ColorWheel(popup);
        colorwheel->setColor(popupBtn->backgroundColor());

        Button *colorBtn = new Button(popup, "Pick");
        colorBtn->setFixedSize(Vector2i(100, 25));
        Color c = colorwheel->color();
        colorBtn->setBackgroundColor(c);

        colorwheel->setCallback([colorBtn](const Color &value) {
            colorBtn->setBackgroundColor(value);
        });

        colorBtn->setChangeCallback([colorBtn, popupBtn](bool pushed) {
            if (pushed) {
                popupBtn->setBackgroundColor(colorBtn->backgroundColor());
                popupBtn->setPushed(false);
            }
        });




        performLayout();

        /* All NanoGUI widgets are initialized at this point. Now
           create an OpenGL shader to draw the main window contents.

           NanoGUI comes with a simple Eigen-based wrapper around OpenGL 3,
           which eliminates most of the tedious and error-prone shader and
           buffer object management.
        */

        // mShader.init(
        //     /* An identifying name */
        //     "a_simple_shader",

        //     /* Vertex shader */
        //     "#version 330\n"
        //     "uniform mat4 modelViewProj;\n"
        //     "in vec3 position;\n"
        //     "void main() {\n"
        //     "    gl_Position = modelViewProj * vec4(position, 1.0);\n"
        //     "}",

        //     /* Fragment shader */
        //     "#version 330\n"
        //     "out vec4 color;\n"
        //     "uniform float intensity;\n"
        //     "void main() {\n"
        //     "    color = vec4(vec3(intensity), 1.0);\n"
        //     "}"
        // );

        // MatrixXu indices(3, 2); /* Draw 2 triangles */
        // indices.col(0) << 0, 1, 2;
        // indices.col(1) << 2, 3, 0;

        // MatrixXf positions(3, 4);
        // positions.col(0) << -1, -1, 0;
        // positions.col(1) <<  1, -1, 0;
        // positions.col(2) <<  1,  1, 0;
        // positions.col(3) << -1,  1, 0;

        // mShader.bind();
        // mShader.uploadIndices(indices);
        // mShader.uploadAttrib("position", positions);
        // mShader.setUniform("intensity", 0.5f);
    }

    ~VisApp() {
        // mShader.free();
    }
    virtual bool mouseMotionEvent(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers){
        int fpathsize = mFocusPath.size();
        if(!Screen::mouseMotionEvent(p,rel,button,modifiers) && fpathsize <= 1){
            // pass focus to hale application.
            // pViewer->setFocused(pViewer->getWindow(), true);
            pViewer->cursorPosCB(pViewer->getWindow(), p[0], p[1]);
        }
        else{
            // pViewer->setFocused(pViewer->getWindow(), false);
        }
        return true;
    }
    virtual bool resizeEvent(const nanogui::Vector2i &s) {
        fprintf(stderr,"resized");
        pViewer->framebufferSizeCB(pViewer->getWindow(), s[0],s[1]);
        return Screen::resizeEvent(s);
    }
    virtual bool mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down, int modifiers){
        // within the nanogui framework, only mouse clicks are able to
        // change the focus of a window.
        fprintf(stderr,"mouse button");
        int fpathsize = mFocusPath.size();
        fprintf(stderr,"path size: %d\n",fpathsize);
        if(!Screen::mouseButtonEvent(p,button,down,modifiers) && fpathsize <= 1){
            // the underlying (hale) application has focus.
            pViewer->setFocused(pViewer->getWindow(), true);
            pViewer->mouseButtonCB(pViewer->getWindow(), button, down?GLFW_PRESS:GLFW_RELEASE, modifiers);
        }
        else{
            // lost focus.
            pViewer->setFocused(pViewer->getWindow(), false);
        }
        return true;
    }
    virtual bool keyboardEvent(int key, int scancode, int action, int modifiers) {
        fprintf(stderr,"keyboard");
        int fpathsize = mFocusPath.size();
        if (!Screen::keyboardEvent(key, scancode, action, modifiers) && fpathsize <= 1){
            // pass events to hale application.
            pViewer->setFocused(pViewer->getWindow(), true);
            pViewer->keyCB(pViewer->getWindow(), key, scancode, action, modifiers);
        }
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            setVisible(false);
            return true;
        }
        return true;
    }

    virtual void draw(NVGcontext *ctx) {
        /* Animate the scrollbar */
        mProgress->setValue(std::fmod((float) glfwGetTime() / 10, 1.0f));

        /* Draw the user interface */
        Screen::draw(ctx);

        drawContents();
    }


    virtual void drawContents() {
        static int prog_last = 0;
        using namespace nanogui;

        // int prog;glGetIntegerv(GL_CURRENT_PROGRAM,&prog);
        // fprintf(stderr, "\nusing program %d\n", prog);
        // if(prog !=18){
            // fprintf(stderr,"setting program to 18\n");
            // glUseProgram(18);
        // }
        sleep(0.1);
        glEnable(GL_DEPTH_TEST);
        // if (pViewer.sliding() && sliso != isovalue) {
        //   isovalue = sliso;
        //   printf("%s: isosurfacing at %g\n", me, isovalue);
        //   seekIsovalueSet(sctx, isovalue);
        //   seekUpdate(sctx);
        //   seekExtract(sctx, lpld);
        //   hply.rebuffer();
        // }
        if(prog_last){
          glUseProgram(prog_last);
        }

        pViewer->draw();

        glGetIntegerv(GL_CURRENT_PROGRAM,&prog_last);
        // fprintf(stderr, "\nused program %d\n", prog_last);


        /* Example code using 2D NanoVectorGraphics (NVG) framework */

/*
        mShader.bind();

        Matrix4f mvp;
        mvp.setIdentity();
        mvp.topLeftCorner<3,3>() = Matrix3f(Eigen::AngleAxisf((float) glfwGetTime(),  Vector3f::UnitZ())) * 0.25f;

        mvp.row(0) *= (float) mSize.y() / (float) mSize.x();
        mShader.setUniform("modelViewProj", mvp);

        // Draw 2 triangles starting at index 0 
        mShader.drawIndexed(GL_TRIANGLES, 0, 2);
*/

    }
    void setOptions(int argc, const char** argv){

    }
    Hale::Viewer *pViewer;
private:
    nanogui::ProgressBar *mProgress;
    // nanogui::GLShader mShader;
};

int
main(int argc, const char **argv) {

  const char *me;
  char *err;
  hestOpt *hopt=NULL;
  hestParm *hparm;
  airArray *mop;

  /* variables learned via hest */
  Nrrd *nin;
  float camfr[3], camat[3], camup[3], camnc, camfc, camFOV;
  int camortho, hitandquit;
  unsigned int camsize[2];
  double isovalue, sliso, isomin, isomax;

  /* boilerplate hest code */
  me = argv[0];
  mop = airMopNew();
  hparm = hestParmNew();
  hparm->respFileEnable = AIR_TRUE;
  airMopAdd(mop, hparm, (airMopper)hestParmFree, airMopAlways);
  

  /* setting up the command-line options */
  hparm->respFileEnable = AIR_TRUE;
  hestOptAdd(&hopt, "i", "volume", airTypeOther, 1, 1, &nin, NULL,
             "input volume to isosurface", NULL, NULL, nrrdHestNrrd);
  hestOptAdd(&hopt, "v", "isovalue", airTypeDouble, 1, 1, &isovalue, "nan",
             "isovalue at which to run Marching Cubes");
  hestOptAdd(&hopt, "fr", "x y z", airTypeFloat, 3, 3, camfr, "3 4 5",
             "look-from point");
  hestOptAdd(&hopt, "at", "x y z", airTypeFloat, 3, 3, camat, "0 0 0",
             "look-at point");
  hestOptAdd(&hopt, "up", "x y z", airTypeFloat, 3, 3, camup, "0 0 1",
             "up direction");
  hestOptAdd(&hopt, "nc", "dist", airTypeFloat, 1, 1, &(camnc), "-2",
             "at-relative near clipping distance");
  hestOptAdd(&hopt, "fc", "dist", airTypeFloat, 1, 1, &(camfc), "2",
             "at-relative far clipping distance");
  hestOptAdd(&hopt, "fov", "angle", airTypeFloat, 1, 1, &(camFOV), "20",
             "vertical field-of-view, in degrees. Full vertical "
             "extent of image plane subtends this angle.");
  hestOptAdd(&hopt, "sz", "s0 s1", airTypeUInt, 2, 2, &(camsize), "640 480",
             "# samples (horz vert) of image plane. ");
  hestOptAdd(&hopt, "ortho", NULL, airTypeInt, 0, 0, &(camortho), NULL,
             "use orthographic instead of (the default) "
             "perspective projection ");
  hestOptAdd(&hopt, "haq", NULL, airTypeBool, 0, 0, &(hitandquit), NULL,
             "save a screenshot rather than display the viewer");

  hestParseOrDie(hopt, argc-1, argv+1, hparm,
                 me, "demo program", AIR_TRUE, AIR_TRUE, AIR_TRUE);
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

  /* learn value range, and set initial isovalue if needed */
  NrrdRange *range = nrrdRangeNewSet(nin, AIR_FALSE);
  airMopAdd(mop, range, (airMopper)nrrdRangeNix, airMopAlways);
  isomin = range->min;
  isomax = range->max;
  if (!AIR_EXISTS(isovalue)) {
    isovalue = (isomin + isomax)/2;
  }

  /* first, make sure we can isosurface ok */
  limnPolyData *lpld = limnPolyDataNew();
  seekContext *sctx = seekContextNew();
  airMopAdd(mop, sctx, (airMopper)seekContextNix, airMopAlways);
  sctx->pldArrIncr = nrrdElementNumber(nin);
  seekVerboseSet(sctx, 0);
  seekNormalsFindSet(sctx, AIR_TRUE);
  if (seekDataSet(sctx, nin, NULL, 0)
      || seekTypeSet(sctx, seekTypeIsocontour)
      || seekIsovalueSet(sctx, isovalue)
      || seekUpdate(sctx)
      || seekExtract(sctx, lpld)) {
    airMopAdd(mop, err=biffGetDone(SEEK), airFree, airMopAlways);
    fprintf(stderr, "trouble with isosurfacing:\n%s", err);
    airMopError(mop);
    return 1;
  }
  if (!lpld->xyzwNum) {
    fprintf(stderr, "%s: warning: No isocontour generated at isovalue %g\n",
            me, isovalue);
  }


  /* initialize gui and viewer */

  nanogui::init();
  nanogui::ref<VisApp> app = new VisApp(argc, argv);
  // app

  Hale::init();
  Hale::Scene scene;
  
  /* then create viewer (in order to create the OpenGL context) */
  Hale::Viewer viewer(camsize[0], camsize[1], "Iso", &scene);
  app->pViewer = &viewer;
  viewer.lightDir(glm::vec3(-1.0f, 1.0f, 3.0f));
  viewer.camera.init(glm::vec3(camfr[0], camfr[1], camfr[2]),
                     glm::vec3(camat[0], camat[1], camat[2]),
                     glm::vec3(camup[0], camup[1], camup[2]),
                     camFOV, (float)camsize[0]/camsize[1],
                     camnc, camfc, camortho);

  viewer.refreshData(&viewer);
  sliso = isovalue;
  viewer.slider(&sliso, isomin, isomax);

  /* then create geometry, and add it to scene */
  Hale::Polydata hply(lpld, true,  // hply now owns lpld
                      Hale::ProgramLib(Hale::preprogramAmbDiff2SideSolid));

  scene.add(&hply);
  scene.drawInit();
  hply.rebuffer();
  viewer.current();
  
  try {
    app->drawAll();
    app->setVisible(true);
    if (hitandquit) {
      seekIsovalueSet(sctx, isovalue);
      seekUpdate(sctx);
      seekExtract(sctx, lpld);
      hply.rebuffer();

      app->drawAll();
      glfwWaitEvents();
      app->drawAll();
      viewer.snap();
      Hale::done();
      airMopOkay(mop);
      return 0;
    }
    nanogui::mainloop();
    nanogui::shutdown();
  }
  catch (const std::runtime_error &e) {
    std::string error_msg = std::string("Caught a fatal error: ") + std::string(e.what());
#if defined(_WIN32)
        MessageBoxA(nullptr, error_msg.c_str(), NULL, MB_ICONERROR | MB_OK);
#else
        std::cerr << error_msg << endl;
#endif
    return -1;
  }

  /* clean exit; all okay */
  Hale::done();
  airMopOkay(mop);
  return 0;
}