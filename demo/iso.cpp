#include <iostream>
#include <Hale.h>
#include <teem/seek.h>
#include <glm/glm.hpp>

void
render(Hale::Viewer3 *viewer) {
  viewer->draw();
  viewer->bufferSwap();
}

int
main(int argc, const char **argv) {
  /* boilerplate hest code */
  const char *me = argv[0];
  airArray *mop = airMopNew();
  hestParm *hparm = hestParmNew();
  airMopAdd(mop, hparm, (airMopper)hestParmFree, airMopAlways);
  /* setting up the command-line options */
  hparm->responseFileEnable = AIR_TRUE;
  hestOpt *hopt = NULL;
  Nrrd *nin;
  hestOptAdd_1_Other(&hopt, "i", "volume", &nin, NULL, "input volume to isosurface",
                     nrrdHestNrrd);
  double isovalue;
  hestOptAdd_1_Double(&hopt, "v", "isovalue", &isovalue, "nan",
                      "isovalue at which to run Marching Cubes");
  float camfr[3];
  hestOptAdd_3_Float(&hopt, "fr", "x y z", camfr, "3 4 5", "look-from point");
  float camat[3];
  hestOptAdd_3_Float(&hopt, "at", "x y z", camat, "0 0 0", "look-at point");
  float camup[3];
  hestOptAdd_3_Float(&hopt, "up", "x y z", camup, "0 0 1", "up direction");
  float camnc;
  hestOptAdd_1_Float(&hopt, "nc", "dist", &(camnc), "-2",
                     "at-relative near clipping distance");
  float camfc;
  hestOptAdd_1_Float(&hopt, "fc", "dist", &(camfc), "2",
                     "at-relative far clipping distance");
  float camFOV;
  hestOptAdd_1_Float(&hopt, "fov", "angle", &(camFOV), "20",
                     "vertical field-of-view, in degrees. Full vertical "
                     "extent of image plane subtends this angle.");
  unsigned int camsize[2];
  hestOptAdd_2_UInt(&hopt, "sz", "s0 s1", camsize, "640 480",
                    "# samples (horz vert) of image plane. ");
  int camortho;
  hestOptAdd_Flag(&hopt, "ortho", &(camortho),
                  "use orthographic instead of (the default) "
                  "perspective projection ");
  int hitandquit;
  hestOptAdd_Flag(&hopt, "haq", &(hitandquit),
                  "save a screenshot rather than display the viewer");
  hestParseOrDie(hopt, argc - 1, argv + 1, hparm, me, "demo program");
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

  /* learn value range, and set initial isovalue if needed */
  NrrdRange *range = nrrdRangeNewSet(nin, AIR_FALSE);
  airMopAdd(mop, range, (airMopper)nrrdRangeNix, airMopAlways);
  double isomin = range->min;
  double isomax = range->max;
  if (!AIR_EXISTS(isovalue)) {
    isovalue = (isomin + isomax) / 2;
  }

  /* first, make sure we can isosurface ok */
  limnPolyData *lpld = limnPolyDataNew();
  seekContext *sctx = seekContextNew();
  airMopAdd(mop, sctx, (airMopper)seekContextNix, airMopAlways);
  sctx->pldArrIncr = nrrdElementNumber(nin);
  seekVerboseSet(sctx, 0);
  seekNormalsFindSet(sctx, AIR_TRUE);
  if (seekDataSet(sctx, nin, NULL, 0) || seekTypeSet(sctx, seekTypeIsocontour)
      || seekIsovalueSet(sctx, isovalue) || seekUpdate(sctx)
      || seekExtract(sctx, lpld)) {
    char *err;
    airMopAdd(mop, err = biffGetDone(SEEK), airFree, airMopAlways);
    fprintf(stderr, "trouble with isosurfacing:\n%s", err);
    airMopError(mop);
    return 1;
  }
  if (!lpld->xyzwNum) {
    fprintf(stderr, "%s: warning: No isocontour generated at isovalue %g\n", me,
            isovalue);
  }

  /* then create empty scene */
  Hale::init();
  Hale::Scene scene;
  /* then create viewer (in order to create the OpenGL context) */
  Hale::Viewer3 viewer(camsize[0], camsize[1], "Iso", &scene);
  viewer.lightDir(glm::vec3(-1.0f, 1.0f, 3.0f));
  viewer.camera.init(glm::vec3(camfr[0], camfr[1], camfr[2]),
                     glm::vec3(camat[0], camat[1], camat[2]),
                     glm::vec3(camup[0], camup[1], camup[2]), camFOV,
                     (float)camsize[0] / camsize[1], camnc, camfc, camortho);
  viewer.refreshCB((Hale::Viewer3Refresher)render);
  viewer.refreshData(&viewer);
  double sliso = isovalue;
  viewer.slider(&sliso, isomin, isomax);
  viewer.current();

  /* then create geometry, and add it to scene */
  Hale::Polydata hply(lpld,
                      true, // hply now owns lpld
                            //   Hale::ProgramLib(Hale::preprogramAmbDiff2SideSolid));
                      Hale::ProgramLib(Hale::preprogramAmbDiffSolid));
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
  while (!Hale::finishing) {
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
