

#include <iostream>

#include <CEGUI/CEGUI.h>
#include <CEGUI/RendererModules/OpenGL/GL3Renderer.h>
#include <CEGUI/System.h>
#include <CEGUI/WindowManager.h>


#include "../Hale.h"
#include "../GUI.h"
#include <glm/glm.hpp>


void render(Hale::Viewer *viewer){
    using namespace CEGUI;
    viewer->draw();

    // swap buffers   
    viewer->bufferSwap();
}

Hale::Viewer *p_viewer;
seekContext *sctx;

void setVerbose(bool vb){
  seekVerboseSet(sctx,vb?1:0);
}
bool getVerbose(){
  return sctx->verbose==0?false:true;
}
void setOrtho(bool o){
  p_viewer->camera.orthographic(o);
}
bool getOrtho(){
  return p_viewer->camera.orthographic();
}
void setFindNormals(bool in){
    seekNormalsFindSet(sctx, in?AIR_TRUE:AIR_FALSE);
}
bool getFindNormals(){
  return sctx->normalsFind==0?false:true;
}
void setClearR(double in){
  GLfloat c[4];
  glGetFloatv(GL_COLOR_CLEAR_VALUE, c);
  glClearColor(in,c[1],c[2],c[3]);
}
double getClearR(){
  GLfloat c[4];
  glGetFloatv(GL_COLOR_CLEAR_VALUE, c);
  return c[0];
}
void setClearG(double in){
  GLfloat c[4];
  glGetFloatv(GL_COLOR_CLEAR_VALUE, c);
  glClearColor(c[0],in,c[2],c[3]);
}
double getClearG(){
  GLfloat c[4];
  glGetFloatv(GL_COLOR_CLEAR_VALUE, c);
  return c[1];
}
void setClearB(double in){
  GLfloat c[4];
  glGetFloatv(GL_COLOR_CLEAR_VALUE, c);
  glClearColor(c[0],c[1],in,c[3]);
}
double getClearB(){
  GLfloat c[4];
  glGetFloatv(GL_COLOR_CLEAR_VALUE, c);
  return c[2];
}



int main(int argc, const char **argv) {
  const char *me;
  char *err;
  hestOpt *hopt=NULL;
  hestParm *hparm;
  airArray *mop;
  /* CEGUI stuff */

  /* variables learned via hest */
  Nrrd *nin;
  float camfr[3], camat[3], camup[3], camnc, camfc, camFOV;
  int camortho, hitandquit;
  unsigned int camsize[2];
  /* boilerplate hest code */
  me = argv[0];
  mop = airMopNew();
  hparm = hestParmNew();
  hparm->respFileEnable = AIR_TRUE;
  airMopAdd(mop, hparm, (airMopper)hestParmFree, airMopAlways);
  /* setting up the command-line options */
  hparm->respFileEnable = AIR_TRUE;
  double init_isoval=0;
  hestOptAdd(&hopt, "i", "volume", airTypeOther, 1, 1, &nin, NULL,
             "input volume to isosurface", NULL, NULL, nrrdHestNrrd);
  hestOptAdd(&hopt, "v", "isovalue", airTypeDouble, 1, 1, &init_isoval, "nan",
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


  /* learn value range, and set initial ProgramState::isovalue if needed */
  NrrdRange *range = nrrdRangeNewSet(nin, AIR_FALSE);
  airMopAdd(mop, range, (airMopper)nrrdRangeNix, airMopAlways);
  double isomin = range->min;
  double isomax = range->max;
  init_isoval = (range->max + range->min)/2.0;

  /* first, make sure we can isosurface ok */
  limnPolyData *lpld = limnPolyDataNew();
  sctx = seekContextNew();
  airMopAdd(mop, sctx, (airMopper)seekContextNix, airMopAlways);
  sctx->pldArrIncr = nrrdElementNumber(nin);
  seekVerboseSet(sctx, 0);
  seekNormalsFindSet(sctx, AIR_TRUE);
  if (seekDataSet(sctx, nin, NULL, 0)
      || seekTypeSet(sctx, seekTypeIsocontour)
      || seekIsovalueSet(sctx, init_isoval)
      || seekUpdate(sctx)
      || seekExtract(sctx, lpld)) {
    airMopAdd(mop, err=biffGetDone(SEEK), airFree, airMopAlways);
    fprintf(stderr, "trouble with isosurfacing:\n%s", err);
    airMopError(mop);
    return 1;
  }
  if (!lpld->xyzwNum) {
    fprintf(stderr, "%s: warning: No isocontour generated at isovalue %g\n",
            me, init_isoval);
  }


  /* then create empty scene */
  Hale::init();
  Hale::Scene scene;
  /* then create viewer (in order to create the OpenGL context) */
  Hale::Viewer viewer(camsize[0], camsize[1], "Iso", &scene);
  ::p_viewer = &viewer;
  viewer.lightDir(glm::vec3(-1.0f, 1.0f, 3.0f));
  viewer.camera.init(glm::vec3(camfr[0], camfr[1], camfr[2]),
                     glm::vec3(camat[0], camat[1], camat[2]),
                     glm::vec3(camup[0], camup[1], camup[2]),
                     camFOV, (float)camsize[0]/camsize[1],
                     camnc, camfc, camortho);
  viewer.refreshCB((Hale::ViewerRefresher)render);
  viewer.refreshData(&viewer);

  viewer.current();



  /* then create geometry, and add it to scene */
  Hale::Polydata hply(lpld, true,  // hply now owns lpld
                      Hale::ProgramLib(Hale::preprogramAmbDiff2SideSolid));
  scene.add(&hply);


  
  
  using namespace CEGUI;

  HaleGUI* halegui = HaleGUI::getInstance();
  halegui->init();

  VariableBinding<double> *isoval;
  const char* fruitEnum[] = {"Apples", "Oranges", "Pears", "Nectarines"};

  /* create all the gui windows we need */

  Window*       gui_isolabel    = (Window*) halegui->createChild("TaharezLook/Label","isoLabel2");
  Editbox*      gui_isobox      = (Editbox*) halegui->createChild("TaharezLook/Editbox","ebox2");
  Scrollbar*    gui_isoslider   = (Scrollbar*) halegui->createChild("TaharezLook/HorizontalScrollbar","scrbr2");
  Scrollbar*    gui_happyslider = (Scrollbar*) halegui->createChild("TaharezLook/HorizontalScrollbar","happy");
  ToggleButton* gui_verboseBox  = (ToggleButton*) halegui->createChild("TaharezLook/Checkbox","verbBox");
  ToggleButton* gui_normalsBox  = (ToggleButton*) halegui->createChild("TaharezLook/Checkbox","normalsBox");
  ToggleButton* gui_orthoBox  = (ToggleButton*) halegui->createChild("TaharezLook/Checkbox","orthoBox");

  Window*       gui_enumlabel    = (Window*) halegui->createChild("TaharezLook/Label","enumlabel");
  Combobox*     gui_cbox        = halegui->createComboboxFromEnum(halegui->leftPaneLayout, "ShaderType", fruitEnum, 4);

  Window*       gui_colorlabel    = (Window*) halegui->createChild("TaharezLook/Label","colorlabel");
  HorizontalLayoutContainer* gui_colorpane = (HorizontalLayoutContainer*)halegui->createChild("HorizontalLayoutContainer","colorpane");
  Scrollbar*    gui_sliderR   = (Scrollbar*) gui_colorpane->createChild("TaharezLook/VerticalScrollbar","colR");
  Scrollbar*    gui_sliderG   = (Scrollbar*) gui_colorpane->createChild("TaharezLook/VerticalScrollbar","colG");
  Scrollbar*    gui_sliderB   = (Scrollbar*) gui_colorpane->createChild("TaharezLook/VerticalScrollbar","colB");

  Window*       gui_modelabel    = (Window*) halegui->createChild("TaharezLook/Label","modelabel");
  Editbox*      gui_modebox      = (Editbox*) halegui->createChild("TaharezLook/Editbox","modebox");
  // Spinner*      gui_spinner      = (Spinner*) halegui->createSpinner(halegui->leftPaneLayout, "spinner", -10,10,1);
  

  gui_isolabel->setText("ISO");
  gui_modelabel->setText("String Entry");
  gui_enumlabel->setText("Enumerated Values");
  gui_colorlabel->setText("Clear Color (rgb)");

  /* scrollbar step sizes */
  gui_isoslider->setStepSize(0.01);
  gui_sliderR->setStepSize(0.01);
  gui_sliderG->setStepSize(0.01);
  gui_sliderB->setStepSize(0.01);

  /* elements may have varying heights */
  UDim unit(0.015,0);
  gui_isolabel->setHeight(unit*2);
  gui_modelabel->setHeight(unit*2);
  gui_colorlabel->setHeight(unit*2);
  gui_enumlabel->setHeight(unit*2);
  gui_isoslider->setHeight(unit);
  gui_happyslider->setHeight(unit);
  gui_isobox->setHeight(unit*2);
  gui_verboseBox->setHeight(unit);
  gui_normalsBox->setHeight(unit);
  gui_orthoBox->setHeight(unit);
  gui_cbox->setHeight(unit*10);
  gui_modebox->setHeight(unit*2);
  // gui_spinner->setHeight(unit*2);

  gui_colorpane->setHeight(unit*11);
  gui_sliderR->setSize(USize(UDim(0,12.5),unit*11));
  gui_sliderG->setSize(USize(UDim(0,12.5),unit*11));
  gui_sliderB->setSize(USize(UDim(0,12.5),unit*11));

  /* mode box. */
  halegui->addGUIElement(new GUIElement<CEGUI::Editbox,const char*>(gui_modebox, new VariableBinding<const char*>("Mode","Default")));

  /* iso slider. */
  isoval = new VariableBinding<double>("ISO", init_isoval);
  fprintf(stderr,"\n\nhi\n");
  halegui->addGUIElement(new GUIElement<CEGUI::Scrollbar, double>( gui_isoslider, isoval, isomin, isomax, 0));
  fprintf(stderr,"\n\nlove\n");

  /* iso textbox. */
  halegui->addGUIElement(new GUIElement<CEGUI::Editbox,double>(gui_isobox,isoval));

  /* clear colors */
  halegui->addGUIElement(new GUIElement<CEGUI::Scrollbar,double>(gui_sliderR, new VariableBinding<double>("colorR", getClearR, setClearR),0,1,0));
  halegui->addGUIElement(new GUIElement<CEGUI::Scrollbar,double>(gui_sliderG, new VariableBinding<double>("colorG", getClearG, setClearG),0,1,0));
  halegui->addGUIElement(new GUIElement<CEGUI::Scrollbar,double>(gui_sliderB, new VariableBinding<double>("colorB", getClearB, setClearB),0,1,0));

  /* toy happiness slider. */
  GUIElement<CEGUI::Scrollbar, double>* happiness;
  happiness = new GUIElement<CEGUI::Scrollbar, double>(gui_happyslider, new VariableBinding<double>("Happy!",15), 0, 100,15);
  halegui->addGUIElement(happiness);

  /* verbose checkbox */
  gui_verboseBox->setText("Verbose");
  halegui->addGUIElement(new GUIElement<CEGUI::ToggleButton,bool>(gui_verboseBox, new VariableBinding<bool>("Verbose", getVerbose, setVerbose)));

  gui_orthoBox->setText("Orthographic");
  halegui->addGUIElement(new GUIElement<CEGUI::ToggleButton,bool>(gui_orthoBox, new VariableBinding<bool>("Ortho", getOrtho, setOrtho)));

  /* seekNormalsFind checkbox */
  gui_normalsBox->setText("Find Normals?");
  halegui->addGUIElement(new GUIElement<CEGUI::ToggleButton,bool>(gui_normalsBox, new VariableBinding<bool>("Normals", getFindNormals, setFindNormals)));

  /* for setting an enum */
  halegui->addGUIElement(new GUIElement<CEGUI::Combobox,int>(gui_cbox, new VariableBinding<int>("Enum", 0)));

  scene.drawInit();

  if (hitandquit) {
    seekIsovalueSet(sctx, isoval->getValue());
    seekUpdate(sctx);
    seekExtract(sctx, lpld);
    hply.rebuffer();

    render(&viewer);
    glfwWaitEvents();
    render(&viewer);
    viewer.snap();
    Hale::done();
    airMopOkay(mop);
    return 0;
  }

  int loopn = 0;
  halegui->forceGUIUpdate();
  while(!Hale::finishing){
    glfwWaitEvents();

    if (halegui->hasChanged("ISO")) {
      printf("%s: isosurfacing at %g\n", me, isoval->getValue());
      seekIsovalueSet(sctx, isoval->getValue());
      seekUpdate(sctx);
      seekExtract(sctx, lpld);
      hply.rebuffer();
    }

    render(&viewer);



    // printf(" >> loop %d\n", ++loopn);
    // if(loopn>5)break;   // in effect: the program exits on mouse-over.
  }
  printf("\nProgram completed successfully.\n\n");
  /* clean exit; all okay */
  Hale::done();
  airMopOkay(mop);
  return 0;
}


