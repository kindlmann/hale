#include <iostream>
#include <Hale.h>
#include <glm/glm.hpp>

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
  viewer.slider(&sliso, isomin, isomax);
  viewer.current();


  /* then create geometry, and add it to scene */
  Hale::Polydata hply(lpld, true,  // hply now owns lpld
                      Hale::ProgramLib(Hale::preprogramAmbDiff2SideSolid));
  scene.add(&hply);


  scene.drawInit();
  render(&viewer);
  if (hitandquit) {
    seekIsovalueSet(sctx, isovalue);
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
  while(!Hale::finishing){
    glfwWaitEvents();
    if (viewer.sliding() && sliso != isovalue) {
      isovalue = sliso;
      printf("%s: isosurfacing at %g\n", me, isovalue);
      seekIsovalueSet(sctx, isovalue);
      seekUpdate(sctx);
      seekExtract(sctx, lpld);
      hply.rebuffer();
    }
    render(&viewer);
  }

  /* clean exit; all okay */
  Hale::done();
  airMopOkay(mop);
  return 0;
}
