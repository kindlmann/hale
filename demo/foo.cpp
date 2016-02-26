#include <iostream>
#include <Hale.h>
#include <glm/glm.hpp>

#include "unistd.h" // for sleep()
#include "/Users/meganrenshaw/Desktop/rokima/rkm.h"


/*
** DOCS: Does error checking on given nin to see if its really a
** 3D scalar volume with full orientation information, and
** then allocates an rkmVol to either wrap around it (if it
** was already nrrdTypeUShort), or to convert it to nrrdTypeUShort
** and contain the new ushort volume.  Also extracts orientation
** information as 4x4 matrix vol->i2w.
*/
rkmVol *
rkmVolNew(const Nrrd *nin) {
  static const char me[]="rkmVolNew";
  const Nrrd *ninUS; /* just for internal use */
  rkmVol *vol;

  if (!nin) {
    biffAddf(RKM, "%s: got NULL pointer", me);
    return NULL;
  }
  if (3 != nin->dim) {
    biffAddf(RKM, "%s: want a 3D volume (not %u-D)\n", me, nin->dim);
    return NULL;
  }
  if (3 != nin->spaceDim) {
    biffAddf(RKM, "%s: input not located in 3D worldspace\n", me);
    return NULL;
  }
  if (!( ELL_3V_EXISTS(nin->spaceOrigin)
         && ELL_3V_EXISTS(nin->axis[0].spaceDirection)
         && ELL_3V_EXISTS(nin->axis[1].spaceDirection)
         && ELL_3V_EXISTS(nin->axis[2].spaceDirection) )) {
    biffAddf(RKM, "%s: input doesn't have spaceOrigin and three "
             "(per-axis) spaceDirections completely set", me);
    return NULL;
  }

  /* input volume seems ok */
  vol = AIR_CALLOC(1, rkmVol);
  if (nrrdTypeUShort != nin->type) {
    /* if (verbose) {
      fprintf(stderr, "%s: NOTE: converting from %s to %s\n", me,
              airEnumStr(nrrdType, nin->type),
              airEnumStr(nrrdType, nrrdTypeUShort));
              } */
    ninUS = vol->ninUS = nrrdNew();
    if (nrrdConvert(vol->ninUS, nin, nrrdTypeUShort)) {
      biffMovef(RKM, NRRD, "%s: problem converting input from %s to %s", me,
                airEnumStr(nrrdType, nin->type),
                airEnumStr(nrrdType, nrrdTypeUShort));
      free(vol); nrrdNuke(vol->ninUS);
      return NULL;
    }
    vol->data = AIR_CAST(unsigned short *, vol->ninUS->data);
  } else {
    /* we got unsigned short type; no conversion needed */
    vol->data = AIR_CAST(unsigned short *, nin->data);
    vol->ninUS = NULL;
    ninUS = nin;
  }

  /* copy basic meta-data: axes sizes and index-to-world transform */
  vol->size[0] = AIR_CAST(unsigned short, ninUS->axis[0].size);
  vol->size[1] = AIR_CAST(unsigned short, ninUS->axis[1].size);
  vol->size[2] = AIR_CAST(unsigned short, ninUS->axis[2].size);
  {
    const double *d0, *d1, *d2, *oo;
    d0 = ninUS->axis[0].spaceDirection;
    d1 = ninUS->axis[1].spaceDirection;
    d2 = ninUS->axis[2].spaceDirection;
    oo = ninUS->spaceOrigin;
    ELL_4M_SET(vol->i2w,
               d0[0], d1[0], d2[0], oo[0],
               d0[1], d1[1], d2[1], oo[1],
               d0[2], d1[2], d2[2], oo[2],
               0.0,   0.0,   0.0,   1.0);
  }

  return vol;
}

rkmVol *
rkmVolNix(rkmVol *vol) {

  if (vol) {
    if (vol->ninUS) {
      /* if vol->ninUS is non-NULL, it was allocated by rkmVolNew,
         so here we have to clean it up */
      nrrdNuke(vol->ninUS);
    }
    free(vol);
  }
  return NULL;
}


int
findCov(double mean[3], const unsigned short *vv,
        const unsigned short size[3],
        const double i2w[16], double thr, double *cov) {
  static const char me[]="findCov";
  double pos[4], evec[9], eval[3];
  unsigned int iz, iy, ix, idx[4];
  unsigned int sx = size[0];
  unsigned int sy = size[1];
  unsigned int sz = size[2];
  unsigned int ii = 0;
  int above = 0;
  ELL_3M_ZERO_SET(cov);
  for (iz=0; iz<sz; iz++) {
    for (iy=0; iy<sy; iy++) {
      for (ix=0; ix<sx; ix++) {
        if (ii != ix + sx*(iy + sy*iz)) {
          fprintf(stderr, "%s: indexing assumptions broken %u != (%u,%u,%u)!\n",
                  me, ii, ix, iy, iz);
          return 1;
        }
        if (vv[ii] > thr) {
          double diff[3];
          above += 1;
          ELL_4V_SET(idx, ix, iy, iz, 1);
          ELL_4MV_MUL(pos, i2w, idx);
          ELL_3V_SUB(diff, pos, mean);
          ELL_3MV_OUTER_INCR(cov, diff, diff);

          if (ii%1000 == 0 ) {
            ell_3m_eigensolve_d(eval, evec, cov, AIR_TRUE);
            fprintf(stderr, "%f, %f, %f\n", pos[0], pos[1], pos[2]);
            fprintf(stderr, "%f, %f, %f\n", eval[0], eval[1], eval[2]);
            if (eval[0] == eval[1] || eval[0] == eval[2] || eval[2] == eval[1]) {
              printf("cov=%f %f %f %f %f %f\n", cov[0], cov[1], cov[2],
              cov[4], cov[5], cov[8]);
              fprintf(stderr, "WARNING FOUND SAME EIGEN VALUE\n");
            }
            fprintf(stderr, "-----------------------------------------\n");

          }
        }
        ii++;
      }
    }
  }

  if (!above) {
    fprintf(stderr, "%s: no voxels above threshold!\n", me);
    return 1;
  }

  ELL_3M_SCALE(cov, 1.0/above, cov);

  /* print upper triangular coefficients:
  **  0   1   2
  ** (3)  4   5
  ** (6) (7)  8
  */
  printf("cov=%f %f %f %f %f %f\n", cov[0], cov[1], cov[2],
         cov[4], cov[5], cov[8]);



  return 0;
}

int
findMeanAndCov(const unsigned short *vv, const unsigned short size[3],
               const double i2w[16], double thr, double *mean, double *cov) {
  static const char me[]="findMean";
  unsigned int ii, ix, iy, iz, sx, sy, sz, idx[4], above, below;
  double pos[4];

  above = below = 0;
  sx = size[0];
  sy = size[1];
  sz = size[2];
  ii = 0;
  ELL_3V_SET(mean, 0, 0, 0);
  for (iz=0; iz<sz; iz++) {
    for (iy=0; iy<sy; iy++) {
      for (ix=0; ix<sx; ix++) {
        /* ii = ix + sx*(iy + sy*iz) */
        if (vv[ii] > thr) {
          above += 1;
          ELL_4V_SET(idx, ix, iy, iz, 1);
          ELL_4MV_MUL(pos, i2w, idx);
          /* now (pos[0], pos[1], pos[2]) is the position in
             3D world-space of sample (ix,iy,iz).  pos[3] should be 1 */
          ELL_3V_INCR(mean, pos);
        } else {
          below += 1;
        }
        ii++;
      }
    }
  }
  if (above) {
    ELL_3V_SCALE(mean, 1.0/above, mean);
    if (findCov(mean, vv, size, i2w, thr, cov)) {
      fprintf(stderr, "%s: problem with covariance computation\n", me);
      return 1;
    }
  } else {
    ELL_3V_SET(mean, AIR_NAN, AIR_NAN, AIR_NAN);
  }

  printf("%s: %g%% of voxels above thresh %g\n", me,
         100.0*(above)/(above + below), thr);
  printf("mean=%f %f %f\n", mean[0], mean[1], mean[2]);

  return 0;
}

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
  double isovalue, sliso;

  double evec[9], eval[3];
  rkmVol *vol;
  double mean[3], cov[9];
  
  Nrrd *nout = nrrdNew();
  unsigned int bins = 2000;
  int type;

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
   hestOptAdd(&hopt, "t", "thresh", airTypeDouble, 1, 1, &isovalue, "nan",
             "threshold value for voxels to analyze");
  camortho = 0;

  hestParseOrDie(hopt, argc-1, argv+1, hparm,
                 me, "demo program", AIR_TRUE, AIR_TRUE, AIR_TRUE);
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

  /* Compute threshold (isovalue) */

  if (!AIR_EXISTS(isovalue)) {
    fprintf(stderr, "%s: finding threshold ... ", me);
    fflush(stderr);
    nrrdHisto(nout, nin, NULL, NULL, bins, nrrdTypeUInt);
    nrrdHistoThresholdOtsu(&isovalue, nout, 2.0);
    fprintf(stderr, "%g\n", isovalue);
  } else {
    fprintf(stderr, "%s: (using given threshold %g)\n", me, isovalue);
  }

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

  /* Compute covariance and mean data for plane rendering */

  if (!(vol = rkmVolNew(nin))) {
    airMopAdd(mop, err=biffGetDone(RKM), airFree, airMopAlways);
    fprintf(stderr, "%s: problem handling input:\n%s", me, err);
    airMopError(mop); return 1;
  }
  airMopAdd(mop, vol, (airMopper)rkmVolNix, airMopAlways);
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways); 

  /* Find mean and cov */
  if (findMeanAndCov(vol->data, vol->size, vol->i2w, isovalue, mean, cov)) {
    fprintf(stderr, "%s: computation problem\n", me);
    airMopError(mop); return 1;
  }

  /* Calculate the eigen values and eigen vectors */
  ell_3m_eigensolve_d(eval, evec, cov, AIR_TRUE);
  unsigned int ei;
  int index[3];
  if (eval[0] >= eval[1] && eval[0] >= eval[2] && eval[1] >= eval[2]) {
    index[0] = 0; 
    index[1] = 1;
    index[2] = 2;
  }
  else if (eval[0] >= eval[1] && eval[0] >= eval[2] && eval[2] >= eval[1]) {
    index[0] = 0; 
    index[1] = 2;
    index[2] = 1;
  }
  else if (eval[1] >= eval[2] && eval[1] >= eval[0] && eval[2] >= eval[0]) {
    index[0] = 1; 
    index[1] = 2;
    index[2] = 0;
  }
  else if (eval[1] >= eval[2] && eval[1] >= eval[0] && eval[0] >= eval[2]) {
    index[0] = 1; 
    index[1] = 0;
    index[2] = 2;
  }
  else if (eval[2] >= eval[1] && eval[2] >= eval[0] && eval[1] >= eval[0]) {
    index[0] = 2; 
    index[1] = 1;
    index[2] = 0;
  }
  else {
    index[0] = 2; 
    index[1] = 0;
    index[2] = 1;
  }

  for (ei=0; ei<3; ei++) {
    printf("eval[%u] = %g; evec = %g %g %g\n", ei, eval[ei],
           (evec + 3*ei)[0], (evec + 3*ei)[1], (evec + 3*ei)[2]);
  }


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
  glm::mat4 scalingMatrix = glm::mat4(100.0f, 0.0f, 0.0f, 0.0f, 
                                      0.0f, 100.0f, 0.0f, 0.0f, 
                                      0.0f, 0.0f, 1.0f, 0.0f,
                                      0.0f, 0.0f, 0.0f, 1.0f);
  glm::mat4 rotationTranslationMatrix = glm::mat4((evec + 3*index[0])[0], (evec + 3*index[1])[0], (evec + 3*index[2])[0], mean[0],
                                                  (evec + 3*index[0])[1], (evec + 3*index[1])[1], (evec + 3*index[2])[1], mean[1],
                                                  (evec + 3*index[0])[2], (evec + 3*index[1])[2], (evec + 3*index[2])[2], mean[2],
                                                  0.0f, 0.0f, 0.0f, 1.0f);
  hcube.model(glm::transpose(scalingMatrix* rotationTranslationMatrix));
  scene.add(&hiso);
  scene.add(&hcube);

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
