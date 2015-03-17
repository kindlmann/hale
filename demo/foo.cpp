#include <iostream>
#include <Hale.h>
#include <glm/glm.hpp>

#include "unistd.h" // for sleep()

void render(Hale::Viewer *viewer){
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

  Hale::debugging = 0;

  /* boilerplate hest code */
  me = argv[0];
  mop = airMopNew();
  hparm = hestParmNew();
  airMopAdd(mop, hparm, (airMopper)hestParmFree, airMopAlways);
  /* setting up the command-line options */
  hparm->respFileEnable = AIR_TRUE;
  hparm->noArgsIsNoProblem = AIR_TRUE;
  int quit;
  hestOptAdd(&hopt, "quit", "bool", airTypeBool, 1, 1, &quit, "false",
             "quit as soon as one while-loop iteration is done");
  hestOptAdd(&hopt, "i", "volume", airTypeOther, 1, 1, &nin, "foo.nrrd",
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
  limnPolyData *liso = limnPolyDataNew();
  seekContext *sctx = seekContextNew();
  airMopAdd(mop, sctx, (airMopper)seekContextNix, airMopAlways);
  sctx->pldArrIncr = nrrdElementNumber(nin);
  seekVerboseSet(sctx, 0);
  seekNormalsFindSet(sctx, AIR_TRUE);
  if (seekDataSet(sctx, nin, NULL, 0)
      || seekTypeSet(sctx, seekTypeIsocontour)
      || seekIsovalueSet(sctx, isovalue)
      || seekUpdate(sctx)
      || seekExtract(sctx, liso)) {
    airMopAdd(mop, err=biffGetDone(SEEK), airFree, airMopAlways);
    fprintf(stderr, "trouble with isosurfacing:\n%s", err);
    airMopError(mop);
    return 1;
  }
  if (!liso->xyzwNum) {
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

  /* then add to scene */
  Hale::Polydata hiso(liso, true,  // hiso now owns liso
                      Hale::ProgramLib(Hale::preprogramAmbDiff2SideSolid),
                      "isosurface");


  limnPolyData *lcube = limnPolyDataNew();
  limnPolyDataCubeTriangles(lcube, 1 << limnPolyDataInfoNorm, AIR_TRUE);
  Hale::Polydata hcube(lcube, true,
                       Hale::ProgramLib(Hale::preprogramAmbDiffSolid),
                       "cube");
  hcube.colorSolid(1,0.5,0.5);
  hcube.model(glm::transpose(glm::mat4(30.0f, 0.0f, 0.0f, 0.0f,
                                       0.0f, 30.0f, 0.0f, 0.0f,
                                       0.0f, 0.0f, 30.0f, 0.0f,
                                       0.0f, 0.0f, 0.0f, 1.0f)));

  scene.add(&hiso);
  scene.add(&hcube);

  scene.drawInit();
  render(&viewer);
  /* GLK not sure why, but without second render() here,
     things don't show up on screen except after more GUI events */
  render(&viewer);
  while(!Hale::finishing){
    glfwWaitEvents();
    /*
    if (Hale::viewerModeNone == viewer.mode()) {
      continue;
    }
    */
    if (viewer.sliding() && sliso != isovalue) {
      // over-riding manually set isovalue for consistency of testing
      //isovalue = -0.01;
      isovalue = sliso;
      printf("!%s: isosurfacing at %g\n", me, isovalue);
      seekIsovalueSet(sctx, isovalue);
      seekUpdate(sctx);
      seekExtract(sctx, liso);
      hiso.rebuffer();
    }
    render(&viewer);
    if (quit) {
      printf("!%s: . . . . . . quitting;\n", me);
      sleep(1);
      break;
    }
  }

  /* clean exit; all okay */
  Hale::done();
  airMopOkay(mop);
  return 0;
}
