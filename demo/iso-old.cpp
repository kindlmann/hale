#include <iostream>

#include "GUI.h"
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

class MyScreen : public nanogui::Screen{
public:
  MyScreen(nanogui:: Vector2i s, char* name) : nanogui::Screen(s,name){

  }

};

template<typename T, int N>
struct array{
  T v[N];
};

int
main(int argc, const char **argv) {

  const char *me;
  char *err;

  /* variables learned via hest */
  Nrrd *nin =0;
  float camfr[3], camat[3], camup[3], camnc, camfc, camFOV;
  int camortho, hitandquit;
  unsigned int camsize[2];
  double isovalue, sliso, isomin, isomax;

  /* boilerplate hest code */
  // me = argv[0];
  // mop = airMopNew();
  // hparm = hestParmNew();
  // hparm->respFileEnable = AIR_TRUE;
  // airMopAdd(mop, hparm, (airMopper)hestParmFree, airMopAlways);
  
  // VariableBinding<Nrrd*> **ninbind =
  //   HCI::buildParameter<Nrrd*>("i", "volume", airTypeOther, 1, 1, NULL,
  //   "input volume to isosurface", NULL, NULL, nrrdHestNrrd);
  VariableBinding<double> **isovalbind = HCI::buildParameter<double>
  ("v", "isovalue", airTypeDouble, 1, 1, "nan",
             "isovalue at which to run Marching Cubes",0,0,0);
  HCI::hparm->respFileEnable = AIR_TRUE;
  hestOptAdd(&HCI::hopt, "i", "volume", airTypeOther, 1, 1, &nin, NULL,
           "input volume to isosurface", NULL, NULL, nrrdHestNrrd);

  VariableBinding<array<float, 3>> **camfrbind = HCI::buildParameter<array<float, 3>>("fr", "x y z", airTypeFloat, 3, 3, "3 4 5",
             "look-from point",0,0,0);
  VariableBinding<array<float, 3>> **camatbind = HCI::buildParameter<array<float, 3>>("at", "x y z", airTypeFloat, 3, 3, "0 0 0",
             "look-at point",0,0,0);
  VariableBinding<array<float, 3>> **camupbind = HCI::buildParameter<array<float, 3>>("up", "x y z", airTypeFloat, 3, 3, "0 0 1",
             "up direction",0,0,0);

  VariableBinding<double> **camncbind =
    HCI::buildParameter<double>("nc", "dist", airTypeFloat, 1, 1, "-2",
    "at-relative near clipping distance",0,0,0);
  VariableBinding<double> **camfcbind = 
    HCI::buildParameter<double>("fc", "dist", airTypeFloat, 1, 1, "2",
    "at-relative far clipping distance",0,0,0);
  VariableBinding<double> **camFOVbind = 
    HCI::buildParameter<double>("fov", "angle", airTypeFloat, 1, 1, "20",
    "vertical field-of-view, in degrees. Full vertical extent of image plane subtends this angle",0,0,0);
  VariableBinding<array<unsigned int, 2>> **camsizebind = 
    HCI::buildParameter<array<unsigned int, 2>>("sz", "s0 s1", airTypeUInt, 2, 2, "640 480",
    "# samples (horz vert) of image plane. ",0,0,0);
  VariableBinding<int> **camorthobind = 
    HCI::buildParameter<int>("ortho", NULL, airTypeInt, 0, 0, NULL,
    "use orthographic projection",0,0,0);
  VariableBinding<bool> **hitandquitbind = 
    HCI::buildParameter<bool>("haq", NULL, airTypeBool, 0, 0, NULL,
    "save a screenshot rather than display the viewer",0,0,0);
  HCI::loadParameters(argc, argv);
  airMopAdd(HCI::mop, HCI::hopt, (airMopper)hestOptFree, airMopAlways);
  airMopAdd(HCI::mop, HCI::hopt, (airMopper)hestParseFree, airMopAlways);

  fprintf(stderr,"1\n\n");
  // nin = (*ninbind)->getValue();
  isovalue = (*isovalbind)->getValue();
  fprintf(stderr,"  2\n\n");
  camfr[0] = (*camfrbind)->getValue().v[0];
  camfr[1] = (*camfrbind)->getValue().v[1];
  camfr[2] = (*camfrbind)->getValue().v[2];
  camat[0] = (*camatbind)->getValue().v[0];
  camat[1] = (*camatbind)->getValue().v[1];
  camat[2] = (*camatbind)->getValue().v[2];
  camup[0] = (*camupbind)->getValue().v[0];
  camup[1] = (*camupbind)->getValue().v[1];
  camup[2] = (*camupbind)->getValue().v[2];
  fprintf(stderr,"hi\n\n");
  camnc = (*camncbind)->getValue();
  camfc = (*camfcbind)->getValue();
  camFOV = (*camFOVbind)->getValue();
  camsize[0] = (*camsizebind)->getValue().v[0];
  camsize[1] = (*camsizebind)->getValue().v[1];
  camortho = (*camorthobind)->getValue();
  hitandquit = (*hitandquitbind)->getValue();

  fprintf(stderr,"iso: %f\n", isovalue);
  fprintf(stderr, "hit: %s\n", hitandquit?"true":"false");

  // fprintf(stderr, "isoval: %p, %p\n", isovalbind, (*isovalbind));
  // HCI::loadParameters(argc, argv);
  // fprintf(stderr, "parammed...\n");
  // fprintf(stderr, "isoval: %p, %p\n", isovalbind, (*isovalbind));
  // VariableBinding<double> *ivbd = *isovalbind;
  // fprintf(stderr, "isoval: %p, %s, %f\n", ivbd, ivbd->name, ivbd->getValue());
  // fprintf(stderr, "x y z: %f, %f, %f\n", (*camfrbind)->getValue().v[0], (*camfrbind)->getValue().v[1], (*camfrbind)->getValue().v[2]);
  // exit(0);
  // handle input parameters:
  /* learn value range, and set initial isovalue if needed */
  fprintf(stderr,"  3\n");
  NrrdRange *range = nrrdRangeNewSet(nin, AIR_FALSE);

  fprintf(stderr,"  3-%p, %p = %s\n",nin, nin->content, nin->content);
  fprintf(stderr,"  Nrrd: %s, dim %d\n",nin, nin->content, nin->type);
  airMopAdd(HCI::mop, range, (airMopper)nrrdRangeNix, airMopAlways);
  isomin = range->min;
  isomax = range->max;
  // if (!AIR_EXISTS(isovalue)) {
    isovalue = (isomin + isomax)/2;
  // }
  fprintf(stderr,"   4\n");

  /* first, make sure we can isosurface ok */
  lpld = limnPolyDataNew();
  sctx = seekContextNew();
  airMopAdd(HCI::mop, sctx, (airMopper)seekContextNix, airMopAlways);
  sctx->pldArrIncr = nrrdElementNumber(nin);
  seekVerboseSet(sctx, 0);
  seekNormalsFindSet(sctx, AIR_TRUE);
  if (seekDataSet(sctx, nin, NULL, 0)
      || seekTypeSet(sctx, seekTypeIsocontour)
      || seekIsovalueSet(sctx, isovalue)
      || seekUpdate(sctx)
      || seekExtract(sctx, lpld)) {
    airMopAdd(HCI::mop, err=biffGetDone(SEEK), airFree, airMopAlways);
    fprintf(stderr, "trouble with isosurfacing:\n%s", err);
    airMopError(HCI::mop);
    return 1;
  }
  if (!lpld->xyzwNum) {
    fprintf(stderr, "%s: warning: No isocontour generated at isovalue %g\n",
            me, isovalue);
  }


  /* initialize gui and scene */

  nanogui::init();
  Hale::init();
  Hale::Scene scene;



  /* then create viewer (in order to create the OpenGL context) */
  Hale::Viewer viewer(camsize[0], camsize[1], "Iso", &scene);
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
  phply = &hply;
  scene.add(&hply);
  scene.drawInit();
  hply.rebuffer();
  viewer.current();
  viewer.setUpdateFunction(update);

  /* now create gui elements */

  MyScreen *screen = new MyScreen(nanogui:: Vector2i(500, 700), "NanoGUI test");

  {
    using namespace nanogui;
    using std::cout;
    using std::cerr;
    using std::endl;
    using std::string;
    using std::to_string;


    FormHelper *gui = new FormHelper(screen);
    ref<Window> win = gui->addWindow(Eigen::Vector2i(31, 15), "Form helper example");
    gui->addGroup("Basic types");
    // bool* boolptr = new bool(true);
    bool boolptr = true;
    gui->addVariable<double>("double",
        [&](double v) { fprintf(stderr,"setting %f\n",v); },
        [&]() -> double { return 4.2; });


    VariableBinding<std::string> *binding =new VariableBinding<std::string>("textname", "something");
    fileBinding = new VariableBinding<std::string>("Filename", "~/~");
    VariableBinding<nanogui::Color> *colorBinding =new VariableBinding<nanogui::Color>("colorbox", 
        [&viewer, &scene](){
            glm::vec3 bgcol = scene.bgColor();
            return nanogui::Color(bgcol[0],bgcol[1],bgcol[2],1.f);
        },
        [&viewer, &scene](nanogui::Color in){
            scene.bgColor(in[0],in[1],in[2]);
        });
    timeBinding =new VariableBinding<int>("Elapsed Time", 0);
    VariableBinding<bool> *orthographic =new VariableBinding<bool>("Orthographic", 
        [&viewer](){
            return viewer.camera.orthographic();
        },
        [&viewer](bool in){
            viewer.camera.orthographic(in);
        });


    std::vector<std::string> vals = {"Apples", "Oranges", "Bananas", "Grapefruits"};
    isoBinding =new VariableBinding<double>("isoval", &isovalue);
    formatBinding =new VariableBinding<int>("format", 1);


    Window* window = new Window(&viewer, "Hale ISO");
    window->setPosition(Vector2i(400, 15));
    window->setLayout(new GroupLayout());

    new Label(window, "Controls", "sans-bold", 20);
    new BoundWidget<std::string, nanogui::TextBox>(window, binding);
    new Label(window, "Elapsed Time", "sans-bold", 16);
    new BoundWidget<int, nanogui::IntBox<int>>(window, timeBinding);
    new Label(window, "ISO Value", "sans-bold", 16);
    new BoundWidget<double, nanogui::FloatBox<double>>(window, isoBinding);
    auto *sliso = new BoundWidget<double, nanogui::Slider>(window, isoBinding);
    new BoundWidget<bool, nanogui::CheckBox>(window, orthographic);
    new Label(window, "Background Color", "sans-bold", 16);
    new BoundWidget<nanogui::Color, nanogui::ColorPicker>(window,colorBinding);

    sliso->setRange(isomin,isomax);
    binding->setValue("String entry");
 

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
        cout << "File dialog result: " << file_dialog(
                { {airEnumStr(nrrdFormatType, formatBinding->getValue()), airEnumDesc(nrrdFormatType, formatBinding->getValue())}, {"txt", "Text file"} }, true) << endl;
    });

    new Label(window, "File Format", "sans-bold", 16);

    new BoundWidget<int, nanogui::ComboBox>(window, formatBinding, nrrdFormatType);

    viewer.performLayout();
  }

  screen->setVisible(true);
  screen->performLayout();
  
  viewer.setUpdateFunction(update);

  try {
    viewer.drawAll();
    viewer.setVisible(true);

    if (hitandquit) {
      seekIsovalueSet(sctx, isovalue);
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
#if defined(_WIN32)
    MessageBoxA(nullptr, error_msg.c_str(), NULL, MB_ICONERROR | MB_OK);
#else
    std::cerr << error_msg << std::endl;
#endif
    return -1;
  }

  /* clean exit; all okay */
  Hale::done();
  airMopOkay(HCI::mop);
  return 0;
}