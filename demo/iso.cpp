

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

seekContext *sctx;

void setVerbose(bool vb){
  seekVerboseSet(sctx,vb?1:0);
}
bool getVerbose(){
  return sctx->verbose==0?false:true;
}
void setFindNormals(bool in){
    seekNormalsFindSet(sctx, in?AIR_TRUE:AIR_FALSE);
}
bool getFindNormals(){
  return sctx->normalsFind==0?false:true;
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

  // iso label.
  halegui->getWithID(3)->setText("ISO");

  // iso slider.
  CEGUI::Scrollbar* isoSlider = (CEGUI::Scrollbar*) halegui->getWithID(15);
  isoval = new VariableBinding<double>("ISO", init_isoval);
  GUIElement<CEGUI::Scrollbar, double>* isoElm;
  isoElm = new GUIElement<CEGUI::Scrollbar, double>( isoSlider, isoval, isomax, isomin);
  halegui->addGUIElement(isoElm);

  // iso textbox.
  halegui->addGUIElement(
    new GUIElement<CEGUI::Editbox,double>(
      (CEGUI::Editbox*)halegui->getWithID(7),
      isoval));


  // toy happiness slider.
  GUIElement<CEGUI::Scrollbar, double>* happiness;
  happiness = new GUIElement<CEGUI::Scrollbar, double>((CEGUI::Scrollbar*) halegui->getWithID(19), new VariableBinding<double>("Happy!",15), 100, 0);
  halegui->addGUIElement(happiness);

  //verbose checkbox
  CEGUI::ToggleButton* checkBox;
  checkBox = (CEGUI::ToggleButton*) halegui->getWithID(23);
  checkBox->setText("Verbose");
  halegui->addGUIElement(new GUIElement<CEGUI::ToggleButton,bool>(checkBox, new VariableBinding<bool>("Verbose", getVerbose, setVerbose)));

    //seekNormalsFind checkbox
  halegui->addGUIElement(new GUIElement<CEGUI::ToggleButton,bool>((CEGUI::ToggleButton*) halegui->getWithID(29), new VariableBinding<bool>("Normals", getFindNormals, setFindNormals)));

  // shader type combobox. Windows can also be created without
  // using the xml file.


  // for setting an enum
  using namespace CEGUI;

  const char* fruitEnum[] = {"Apples", "Oranges", "Pears", "Nectarines"};
  Combobox* cbox = halegui->createComboboxFromEnum(halegui->leftPane, "ShaderType", fruitEnum, 4);
  cbox->setArea(UDim(0,5),UDim(0.210,0),UDim(1,-10),UDim(0.120,0));
  halegui->addGUIElement(new GUIElement<CEGUI::Combobox,int>(cbox, new VariableBinding<int>("Enum", 0)));


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

  while(!Hale::finishing){
    glfwWaitEvents();
    bool cg = halegui->hasChanged();

    // fprintf(stderr, "modd: %s\n",cg?"true":"false");
    if (cg) {
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


