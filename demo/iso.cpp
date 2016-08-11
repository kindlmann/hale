#include <iostream>

#include <Hale.h>
#include <glm/glm.hpp>

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

#define ANSI_RED     "\x1b[31m"
#define ANSI_GREEN   "\x1b[32m"
#define ANSI_YELLOW  "\x1b[33m"
#define ANSI_BLUE    "\x1b[34m"
#define ANSI_MAGENTA "\x1b[35m"
#define ANSI_CYAN    "\x1b[36m"
#define ANSI_RESET   "\x1b[0m"

#include "vectorbox.h"

#if defined(_WIN32)
#include <windows.h>
#endif
#include <nanogui/glutil.h>
#include <iostream>
#include <string>
#include <time.h>

  
VariableBinding<double> *isoBinding;
VariableBinding<int> *timeBinding;
VariableBinding<int> *formatBinding;
VariableBinding<std::string> *fileBinding;

seekContext *sctx;
limnPolyData *lpld;
Hale::Polydata *phply;

void update(){
    const char *me = "update()";
    static time_t start = time(0);
    if(isoBinding && isoBinding->hasChanged()){
        fprintf(stderr,"%s: isosurfacing at %g\n", me, isoBinding->getValue());
        seekIsovalueSet(sctx, isoBinding->getValue());
        seekUpdate(sctx);
        seekExtract(sctx, lpld);
        phply->rebuffer();
    }
    timeBinding->setValue((int)difftime( time(0), start));
}


int main(int argc, const char **argv) {

  const char *me;
  char *err;
  

  nanogui::init();
  Hale::init();
  Hale::Scene scene;
  Hale::Viewer viewer(600, 900, "Iso", &scene);
  viewer.lightDir(glm::vec3(-1.0f, 1.0f, 3.0f));
  
  // example: we can create our own exposures externally.
  VariableExposure<nanogui::Vector2i>::expose(&viewer, "size",
    [&viewer](){ return viewer.size(); }, // getter
    [&viewer](nanogui::Vector2i in){      // setter
      if(in[0] <= 0)in[0] = 1;
      if(in[1] <= 0)in[1] = 1;
      viewer.setSize(nanogui::Vector2i(in[0],in[1]));
      viewer.camera.aspect((float)in[0]/in[1]);
    });

  // we can get a list of exposed variables at runtime.
  // VariableExposure<double>::getExposedVariables();
  // VariableExposure<int>::getExposedVariables();

  // create variable bindings from the command line.
  // the type of each auto is: VariableBinding<T>**,
  // where T is the first template parameter passed to
  // buildParameter.

  // define functions to create glm::vec3 or
  // Eigen::Vector2i from raw hest bytes.

  std::function<glm::vec3(array<float, 3>)>
    makevec3=[](array<float, 3> in){
      return glm::vec3(in.v[0],in.v[1],in.v[2]);
    };
  std::function<nanogui::Vector2i(array<unsigned int, 2>)>
    makeVector2i= [](array<unsigned int, 2> in){
      return nanogui::Vector2i(in.v[0],in.v[1]);
    };

  // build parameters. bind to exposed variablebindings.
  auto ninbind = HCI::buildParameter<Nrrd*>(
    "i", "volume", 1, 1, NULL, NULL, NULL, nrrdHestNrrd,0,0,
    "input volume to isosurface", airTypeOther
  );
  auto camfrbind = HCI::buildParameter<glm::vec3, array<float, 3>>(
    "fr", "x y z", 3, 3, "3 4 5",0,0,0,&viewer.camera, "fromvec",
    "look-from point", airTypeFloat, &makevec3
  );
  auto camatbind = HCI::buildParameter<glm::vec3, array<float, 3>>(
    "at", "x y z", 3, 3, "0 0 0",0,0,0,&viewer.camera, "atvec",
    "look-at point", airTypeFloat, &makevec3
  );
  auto camupbind = HCI::buildParameter<glm::vec3, array<float, 3>>(
    "up", "x y z", 3, 3, "0 0 1",0,0,0,&viewer.camera, "upvec",
    "up direction", airTypeFloat, &makevec3
  );
  auto camncbind = HCI::buildParameter<double>(
    "nc", "dist", 1, 1, "-2",0,0,0,&viewer.camera, "nearclip",
    "at-relative near clipping distance"
  );
  auto camfcbind = HCI::buildParameter<double>(
    "fc", "dist", 1, 1, "2",0,0,0,&viewer.camera, "farclip",
    "at-relative far clipping distance"
  );
  auto camFOVbind = HCI::buildParameter<double>(
    "fov", "angle", 1, 1, "20",0,0,0,&(viewer.camera),"fov",
    "vertical field-of-view, in degrees. Full vertical extent of image plane subtends this angle"
  );
  auto camsizebind = HCI::buildParameter<nanogui::Vector2i, array<unsigned int, 2>>(
    "sz", "s0 s1", 2, 2, "640 480",0,0,0,&viewer,"size",
    "# samples (horz vert) of image plane. ", airTypeUInt, &makeVector2i
  );
  auto camorthobind = HCI::buildParameter<bool>(
    "ortho", NULL, 0, 0, NULL,0,0,0,&(viewer.camera),"ortho",
    "use orthographic projection"
  );
  auto hitandquitbind = HCI::buildParameter<bool>(
    "haq", NULL, 0, 0, NULL,0,0,0,0,0,
    "save a screenshot rather than display the viewer"
  );
  auto isovalbind = HCI::buildParameter<double>(
    "v", "isovalue", 1, 1, "nan",0,0,0,0,0,
    "isovalue at which to run Marching Cubes"
  );
  // load and initialize everything.
  HCI::loadParameters(argc, argv);

  Nrrd* nin = (*ninbind)->getValue();
  isoBinding = *isovalbind;
  NrrdRange *range = nrrdRangeNewSet(nin, AIR_FALSE);

  airMopAdd(HCI::mop, range, (airMopper)nrrdRangeNix, airMopAlways);
  double isomin = range->min;
  double isomax = range->max;
  if (!AIR_EXISTS(isoBinding->getValue())) {
    isoBinding->setValue((isomin + isomax)/2);
  }

  /* first, make sure we can isosurface ok */
  lpld = limnPolyDataNew();
  sctx = seekContextNew();
  airMopAdd(HCI::mop, sctx, (airMopper)seekContextNix, airMopAlways);
  sctx->pldArrIncr = nrrdElementNumber(nin);

  seekVerboseSet(sctx, 0);
  seekNormalsFindSet(sctx, AIR_TRUE);

  if (seekDataSet(sctx, nin, NULL, 0)
      || seekTypeSet(sctx, seekTypeIsocontour)
      || seekIsovalueSet(sctx, isoBinding->getValue())
      || seekUpdate(sctx)
      || seekExtract(sctx, lpld)) {
    airMopAdd(HCI::mop, err=biffGetDone(SEEK), airFree, airMopAlways);
    fprintf(stderr, "trouble with isosurfacing:\n%s", err);
    airMopError(HCI::mop);
    return 1;
  }
  if (!lpld->xyzwNum) {
    fprintf(stderr, "%s: warning: No isocontour generated at isovalue %g\n",
            me, isoBinding->getValue());
  }


  /* initialize gui and scene */

  viewer.refreshData(&viewer);

  /* then create geometry, and add it to scene */
  Hale::Polydata hply(lpld, true,  // hply now owns lpld
                      Hale::ProgramLib(Hale::preprogramAmbDiff2SideSolid));
  phply = &hply;
  scene.add(&hply);
  scene.drawInit();
  hply.rebuffer();
  viewer.current();
  viewer.setUpdateFunction(update);

  // sample variable bindings.

  VariableBinding<std::string> *binding =new VariableBinding<std::string>("textname", "string entry");
  fileBinding = new VariableBinding<std::string>("Filename", "~/~");
  timeBinding = new VariableBinding<int>("Elapsed Time", 0);
  formatBinding=new VariableBinding<int>("format", 1);
  VariableBinding<nanogui::Color> *colorBinding =new VariableBinding<nanogui::Color>("colorbox", 
      [&viewer, &scene](){
          glm::vec3 bgcol = scene.bgColor();
          return nanogui::Color(bgcol[0],bgcol[1],bgcol[2],1.f);
      },
      [&viewer, &scene](nanogui::Color in){
          scene.bgColor(in[0],in[1],in[2]);
      });

  /* now create gui elements */
  using Hale::ControllerScreen;
  ControllerScreen *screen;
  Window *window;

  {
    using namespace nanogui;

    // Each nanogui::Window is contained within the parent Viewer.
    window = new Window(&viewer, "Hale ISO");
    window->setPosition(Vector2i(400, 15));
    window->setLayout(new GroupLayout());

    new Label(window, "Controls", "sans-bold", 20);
    new BoundWidget<std::string, nanogui::TextBox>(window, binding);
    new Label(window, "ISO Value", "sans-bold", 16);
    new BoundWidget<double, nanogui::FloatBox<double>>(window, isoBinding);
    new BoundWidget<bool, nanogui::CheckBox>(window, *camorthobind);
    new BoundWidget<double, nanogui::FloatBox<double>>(window, *camFOVbind);
    new Label(window, "Background Color", "sans-bold", 16);
    new BoundWidget<nanogui::Color, nanogui::ColorPicker>(window,colorBinding);
    auto *sliso = new BoundWidget<double, nanogui::Slider>(window, isoBinding);
    sliso->setRange(isomin,isomax);
 
    window = new Window(&viewer, "File");
    window->setPosition(Vector2i(210, 15));
    window->setLayout(new GroupLayout());

    new Label(window, "Things With Files", "sans-bold", 20);
    new Label(window, "Filename", "sans-bold", 16);
    new BoundWidget<std::string, nanogui::TextBox>(window, fileBinding);

    Widget* tools = new Widget(window);
    tools->setLayout(new BoxLayout(Orientation::Horizontal,
                                   Alignment::Middle, 0, 6));
    nanogui::Button *b = new Button(tools, "Open");
    b->setCallback([&] {
        fileBinding->setValue(file_dialog(
                { {airEnumStr(nrrdFormatType, formatBinding->getValue()), airEnumDesc(nrrdFormatType, formatBinding->getValue())}, {"txt", "Text file"} }, false));
    });
    b = new Button(tools, "Save");
    b->setCallback([&] {
        fileBinding->setValue(file_dialog(
                { {airEnumStr(nrrdFormatType, formatBinding->getValue()), airEnumDesc(nrrdFormatType, formatBinding->getValue())}, {"txt", "Text file"} }, true));
    });

    new Label(window, "File Format", "sans-bold", 16);
    new BoundWidget<int, nanogui::ComboBox>(window, formatBinding, nrrdFormatType);

    auto matbind = new VariableBinding<glm::mat2x3>("matrix",glm::mat2x3());


    window = new Window(&viewer, "Vectors");
    window->setLayout(new GroupLayout());
    new Label(window, "Elapsed Time", "sans-bold", 16);
    new BoundWidget<int, nanogui::IntBox<int>>(window, timeBinding);
    new Label(window, "Up Vector", "sans-bold", 16);
    new BoundWidget<glm::vec3, MatrixBox<3,1, glm::vec3>>(window, *camupbind);
    new Label(window, "2x3 Matrix", "sans-bold", 16);
    new BoundWidget<glm::mat2x3, MatrixBox<2,3, glm::mat2x3>>(window, matbind);
    window->setVisible(false);

    screen = new ControllerScreen(nanogui::Vector2i(160, 400), "Controls");
    new Label(screen, "Elapsed Time", "sans-bold", 16);
    new BoundWidget<int, nanogui::IntBox<int>>(screen, timeBinding);
    new Label(screen, "Up Vector", "sans-bold", 16);
    new BoundWidget<glm::vec3, MatrixBox<3,1, glm::vec3>>(screen, *camupbind);
    new Label(screen, "2x3 Matrix", "sans-bold", 16);
    new BoundWidget<glm::mat2x3, MatrixBox<2,3, glm::mat2x3>>(screen, matbind);
    screen->addPopInButton(window);
    screen->performLayout();
    screen->setSize();
    screen->setVisible(true);



    viewer.performLayout();
    viewer.setUpdateFunction(update);
    viewer.drawAll();
    viewer.setVisible(true);

    
  }


  try {

    if ((*hitandquitbind)->getValue()) {
      seekIsovalueSet(sctx, isoBinding->getValue());
      seekUpdate(sctx);
      seekExtract(sctx, lpld);
      hply.rebuffer();

      viewer.drawAll();
      glfwWaitEvents();
      viewer.drawAll();
      viewer.snap();
      Hale::done();
      airMopOkay(HCI::mop);
      return 0;
    }
    nanogui::mainloop();
    nanogui::shutdown();
  }
  catch (const std::runtime_error &e) {
    std::string error_msg = std::string("Caught a fatal error: ") + std::string(e.what());
    std::cerr << error_msg << std::endl;
    return -1;
  }

  /* clean exit; all okay */
  Hale::done();
  airMopOkay(HCI::mop);
  return 0;
}