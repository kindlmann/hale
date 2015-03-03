#include <iostream>
#include <Hale.h>
#include <glm/glm.hpp>

#include "unistd.h" // for sleep()

void render(Hale::Viewer *viewer){
  Hale::uniform("projectMat", viewer->camera.project());
  Hale::uniform("viewMat", viewer->camera.view());

  viewer->draw();
  viewer->bufferSwap();
}

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
  int camortho;
  unsigned int camsize[2];
  double isovalue=0.1, sliso;

  Hale::debugging = 1;

  /* boilerplate hest code */
  me = argv[0];
  mop = airMopNew();
  hparm = hestParmNew();
  airMopAdd(mop, hparm, (airMopper)hestParmFree, airMopAlways);
  /* setting up the command-line options */
  hparm->respFileEnable = AIR_TRUE;
  int showbug;
  hestOptAdd(&hopt, "bug", "bool", airTypeBool, 1, 1, &showbug, "true",
             "arrange things so that bug is evident");
  hestOptAdd(&hopt, "i", "volume", airTypeOther, 1, 1, &nin, NULL,
             "input volume to isosurface", NULL, NULL, nrrdHestNrrd);
  hestOptAdd(&hopt, "fr", "x y z", airTypeFloat, 3, 3, camfr, "-673.394 42.9228 42.9228",
             "look-from point");
  hestOptAdd(&hopt, "at", "x y z", airTypeFloat, 3, 3, camat, "42.9228 42.9228 42.9228",
             "look-at point");
  hestOptAdd(&hopt, "up", "x y z", airTypeFloat, 3, 3, camup, "0 0 1",
             "up direction");
  hestOptAdd(&hopt, "nc", "dist", airTypeFloat, 1, 1, &(camnc), "-126.306",
             "at-relative near clipping distance");
  hestOptAdd(&hopt, "fc", "dist", airTypeFloat, 1, 1, &(camfc), "126.306",
             "at-relative far clipping distance");
  hestOptAdd(&hopt, "fov", "angle", airTypeFloat, 1, 1, &(camFOV), "20",
             "vertical field-of-view, in degrees. Full vertical "
             "extent of image plane subtends this angle.");
  hestOptAdd(&hopt, "sz", "s0 s1", airTypeUInt, 2, 2, &(camsize), "640 480",
             "# samples (horz vert) of image plane. ");
  camortho = 0;

  hestParseOrDie(hopt, argc-1, argv+1, hparm,
                 me, "demo program", AIR_TRUE, AIR_TRUE, AIR_TRUE);
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

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
  sliso = isovalue;
  NrrdRange *range = nrrdRangeNewSet(nin, AIR_FALSE);
  airMopAdd(mop, range, (airMopper)nrrdRangeNix, airMopAlways);
  viewer.slider(&sliso, range->min, range->max);
  viewer.sliding(true);
  viewer.current();

  /* then add it to scene */
  limnPolyData *lcube = limnPolyDataNew();
  limnPolyDataCubeTriangles(lcube, 1 << limnPolyDataInfoNorm, AIR_TRUE);
  Hale::Polydata hcube(lcube, true,
                       /* BUG: should be able to use Hale::preprogramAmbDiffSolid */
                       Hale::ProgramLib(Hale::preprogramAmbDiff2SideSolid),
                       "cube");
  hcube.colorSolid(1,0.5,0.5);
  hcube.model(glm::transpose(glm::mat4(30.0f, 0.0f, 0.0f, 0.0f,
                                       0.0f, 30.0f, 0.0f, 0.0f,
                                       0.0f, 0.0f, 0.5f, 0.0f,
                                       0.0f, 0.0f, 0.0f, 1.0f)));
  if (!showbug) {
    scene.add(&hcube);
  }

  limnPolyData *lball = limnPolyDataNew();
  limnPolyDataSpiralSphere(lball, 1 << limnPolyDataInfoNorm, 10, 10);
  Hale::Polydata hball(lball, true,
                       Hale::ProgramLib(Hale::preprogramAmbDiff2SideSolid),
                       "ball");
  hball.colorSolid(0.5,1.0,0.5);
  hball.model(glm::transpose(glm::mat4(30.0f, 0.0f, 0.0f, 0.0f,
                                       0.0f, 30.0f, 0.0f, 60.0f,
                                       0.0f, 0.0f, 30.0f, 0.0f,
                                       0.0f, 0.0f, 0.0f, 1.0f)));
  scene.add(&hball);

  Hale::Polydata hply(lpld, true,  // hply now owns lpld
                      Hale::ProgramLib(Hale::preprogramAmbDiff2SideSolid),
                      "isosurface");
  scene.add(&hply);
  if (showbug) {
    scene.add(&hcube);
  }

  scene.drawInit();
  printf("!%s: ------------ initial render\n", me);
  render(&viewer);
  printf("!%s: ------------ initial glfwWaitEvents\n", me);
  glfwWaitEvents();
  printf("!%s: ------------ second render\n", me);
  render(&viewer);
  printf("!%s: ------------ entering render loop\n", me);
  unsigned int count = 0;
  while(!Hale::finishing){
    glfwWaitEvents();
    if (Hale::viewerModeNone == viewer.mode()) {
      continue;
    }
    printf("!%s: . . . . . . testing isovalue;\n", me);
    if (viewer.sliding() && sliso != isovalue) {
      // over-riding manually set isovalue for consistency of testing
      isovalue = -0.01;
      printf("!%s: isosurfacing at %g\n", me, isovalue);
      seekIsovalueSet(sctx, isovalue);
      seekUpdate(sctx);
      seekExtract(sctx, lpld);
      hply.rebuffer();
    }
    printf("!%s: . . . . . . rendering;\n", me);
    render(&viewer);
    printf("!%s: . . . . . . done rendering;\n", me);
    count++;
    if (count == 1) {
      /* after this many iterations the rendering bug has happened */
      printf("!%s: . . . . . . bug finished; quitting;\n", me);
      sleep(1);
      break;
    }
  }

  /* clean exit; all okay */
  Hale::done();
  airMopOkay(mop);
  return 0;
}
