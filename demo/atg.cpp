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

  unsigned int nsamp;
  hestOptAdd(&hopt, "n", "num", airTypeUInt, 1, 1, &(nsamp), "10",
             "# samples along triangle edge");
  float gscale;
  hestOptAdd(&hopt, "scl", "scale", airTypeFloat, 1, 1, &(gscale), "1",
             "additional scaling of glyph");
  double phi;
  hestOptAdd(&hopt, "phi", "phi", airTypeDouble, 1, 1, &(phi), "0",
             "orientation of major eigenvector");

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

  limnPolyData *ldisc;
  Hale::Polydata *hdisc;
  float width = 20.0f;
  float ed = width/(nsamp-1);
  float edgei[2] = {ed, 0.0};
  float edgej[2] = {-ed/2, -sqrtf(3)*ed/2};
  float orig[2];
  {
    float foo = width/(2*sqrtf(3));
    float fjdx = (nsamp-1)/2.0f;
    float fidx = (nsamp-1)/4.0f;
    orig[0] = -fidx*edgei[0] - fjdx*edgej[0];
    orig[1] = -fidx*edgei[1] - fjdx*edgej[1];
  }
  for (unsigned int jj=0; jj<nsamp; jj++) {
    for (unsigned int ii=0; ii<nsamp; ii++) {
      if (ii > jj) {
        continue;
      }
      double bary[3] = {AIR_AFFINE(-0.5, jj, nsamp-0.5, 1, 0),
                        0,
                        AIR_AFFINE(-0.5, ii, nsamp-0.5, 0, 1)};
      bary[1] = 1 - bary[0] - bary[2];
      double DSR[3] = {bary[0], bary[1], bary[2]}, tmp;
      ELL_3V_NORM(DSR, DSR, tmp);

      double matE[4] = {DSR[0], 0, 0, DSR[0]};
      double matN[4] = {DSR[1]*cos(2*phi), DSR[1]*sin(2*phi), DSR[1]*sin(2*phi), -DSR[1]*cos(2*phi)};
      double matA[4] = {0, -DSR[2], DSR[2], 0};
      double ten[4];
      ELL_4V_ADD3(ten, matE, matN, matA);

      double mtmp[4], matM[4], recDSR[3];
      ELL_2M_TRANSPOSE(mtmp, ten);
      ELL_4V_SCALE_ADD2(matM, 0.5, ten, 0.5, mtmp);    /* M = (T + T^T)/2 */
      recDSR[0] = (ten[0] + ten[3])/2;                 /* D = tr(T)/2 */
      ELL_4V_SET(matE, recDSR[0], 0, 0, recDSR[0]);    /* E = D*{{1,0},{0,1}} */
      ELL_4V_SUB(matN, matM, matE);                    /* N = M - E */
      recDSR[1] = sqrt(AIR_MAX(0, -ELL_2M_DET(matN))); /* S = sqrt(-det(N)) */
      double recPhi = atan2(matN[1], matN[0])/2;       /* phi = atan(N_12 / N_11)/2 */
      ELL_4V_SET(matN, cos(2*recPhi), sin(2*recPhi), sin(2*recPhi), -cos(2*recPhi));
      ELL_4V_SCALE(matN, recDSR[1], matN);             /* N = S*{{cos(2*phi), sin(2*phi)},{sin(2*phi), cos(2*phi)}} */
      ELL_4V_SCALE_ADD2(matA, 0.5, ten, -0.5, mtmp);   /* A = (T - T^T)/2 */
      recDSR[2] = -matA[1];                            /* R = -A_12 */
      double DD = recDSR[0];
      double SS = recDSR[1];
      double RR = recDSR[2];
      double det = ELL_2M_DET(ten);

      double alpha = (RR > SS
                      ? 1
                      : (det < 0
                         ? AIR_AFFINE(-1, det, 0, 4, 2)
                         : pow(AIR_AFFINE(0, SS-RR, 1.0/sqrt(2), 1, 0), 1 /* "gamma" */)));

      ldisc = limnPolyDataNew();
      limnPolyDataSuperquadric2D(ldisc, 1 << limnPolyDataInfoNorm, alpha, 50);
      hdisc = new Hale::Polydata(ldisc, true,
                                 Hale::ProgramLib(Hale::preprogramAmbDiffSolid),
                                 "disc");
      //hdisc->colorSolid(bary[0], bary[1], bary[2]);
      hdisc->colorSolid(DSR[0], DSR[1], DSR[2]);
      hdisc->model(glm::transpose(glm::mat4(gscale*ed/2, 0.0f, 0.0f, ii*edgei[0] + jj*edgej[0] + orig[0],
                                            0.0f, gscale*ed/2, 0.0f, ii*edgei[1] + jj*edgej[1] + orig[1],
                                            0.0f, 0.0f, 1.0f, 0.0f,
                                            0.0f, 0.0f, 0.0f, 1.0f)));
      scene.add(hdisc);
    }
  }

  // scene.add(&hiso);

  scene.drawInit();
  render(&viewer);
  while(!Hale::finishing){
    glfwWaitEvents();
     if (viewer.sliding() && sliso != isovalue) {
      isovalue = sliso;
      printf("!%s: isosurfacing at %g\n", me, isovalue);
      seekIsovalueSet(sctx, isovalue);
      seekUpdate(sctx);
      seekExtract(sctx, liso);
      hiso.rebuffer();
    }
    render(&viewer);
  }

  /* clean exit; all okay */
  Hale::done();
  airMopOkay(mop);
  return 0;
}
