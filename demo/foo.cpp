#include <iostream>
#include <Hale.h>
#include <glm/glm.hpp>

#include "unistd.h" // for sleep()
#include "/Users/meganrenshaw/Desktop/rokima/rkm.h"

#include <fstream>

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
     // if (verbose) {
     //  fprintf(stderr, "%s: NOTE: converting from %s to %s\n", me,
     //          airEnumStr(nrrdType, nin->type),
     //          airEnumStr(nrrdType, nrrdTypeUShort));
     //          } 
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

  char * name;

  /* variables learned via hest */
  Nrrd *nin;
  float camfr[3], camat[3], camup[3], camnc, camfc, camFOV;
  int camortho;
  unsigned int camsize[2];
  double isovalue, sliso;

  double evec[9], eval[3];
  rkmVol *vol;
  double mean[3];
  
  Nrrd *nout = nrrdNew();
  unsigned int bins = 2000;
  int type;
  int hitandquit;

  Hale::debugging = 0;

  /* boilerplate hest code */
  me = argv[0];
  mop = airMopNew();
  hparm = hestParmNew();
  airMopAdd(mop, hparm, (airMopper)hestParmFree, airMopAlways);
  /* setting up the command-line options */
  hparm->respFileEnable = AIR_TRUE;
  hparm->noArgsIsNoProblem = AIR_TRUE;
  hestOptAdd(&hopt, "i", "volume", airTypeOther, 1, 1, &nin, NULL,
             "input volume to isosurface", NULL, NULL, nrrdHestNrrd);
  hestOptAdd(&hopt, "t", "thresh", airTypeDouble, 1, 1, &isovalue, NULL,
             "threshold value for voxels to analyze");
  hestOptAdd(&hopt, "m", "x y z", airTypeDouble, 3, 3, &mean, NULL,
             "mean position of volume");
  hestOptAdd(&hopt, "evec", "1x 1y 1z 2x 2y 2z 3x 3y 3z", airTypeDouble, 9, 9, &evec, NULL,
             "3 eigen vectors of volume, first 3 values represent first eigen vector, 2nd 3 correspond to second eigen vector, and 3rd 3 correspond to 3rd eigen vector");
  hestOptAdd(&hopt, "eval", "1 2 3", airTypeDouble, 3, 3, &eval, NULL,
             "eigen values corresponding to 1st, 2nd, and 3rd eigen vector respectively");
  
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
  
  
  hestOptAdd(&hopt, "n", "name", airTypeString, 1, 1, &name, "foo", "name of data set");
  camortho = 0;
  hestOptAdd(&hopt, "haq", NULL, airTypeBool, 0, 0, &(hitandquit), NULL,
             "save a screenshot rather than display the viewer");

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

  if (!(vol = rkmVolNew(nin))) {
    airMopAdd(mop, err=biffGetDone(RKM), airFree, airMopAlways);
    // fprintf(stderr, "%s: problem handling input:\n%s", me, err);
    airMopError(mop); return 1;
  }
  airMopAdd(mop, vol, (airMopper)rkmVolNix, airMopAlways);
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways); 

  /* Calculate the eigen values and eigen vectors */
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

  /* Print results to a file */
  // fprintf(stdout, "Printing data to file...\n");
  // std::ofstream outfile;
  // outfile.open("results.txt", std::ios_base::app);

  // char result [3000];


  // sprintf(result, "<tr>\n\t<td>%s</td>\n\t<td>\n\t\t<a href=\"full_img/%s.png\">\n\t\t\t<img src=\"small_img/%s.png\" width=\"100px\"></img>\n\t\t</a>\n\t</td>\n\t<td>%.0f %.0f %.0f<br/>%.0f %.0f %.0f<br/>%.0f %.0f %.0f</td>\n\t<td>%.0f %.0f %.0f</td>\n\t<td>%g</td>\n\t<td>\n\t\t<a href=\"hist/%s.png\">\n\t\t\t<img src=\"hist/%s.png\" width=\"100px\"></img>\n\t\t</a>\n\t</td>\n\t<td>eval[0] = %.0f; evec = %.3f %.3f %.3f<br/>eval[1] = %.0f; evec = %.3f %.3f %.3f<br/>eval[2] = %.0f; evec = %.3f %.3f %.3f</td>\n\t<td>\n\t\t<a href=\"result_img_1/%s.png\">\n\t\t\t<img src=\"result_img_1/%s.png\" width=\"100px\"></img>\n\t\t</a>\n\t\t<a href=\"result_img_2/%s.png\">\n\t\t\t<img src=\"result_img_2/%s.png\" width=\"100px\"></img>\n\t\t</a>\n\t\t<a href=\"result_img_3/%s.png\">\n\t\t\t<img src=\"result_img_3/%s.png\" width=\"100px\"></img>\n\t\t</a>\n\t</td>\n</tr>\n", 
  //         name, name, name, cov[0], cov[1], cov[2], cov[3], cov[4], cov[5], cov[6], cov[7], cov[8], mean[0], mean[1], mean[2], isovalue, name, name, eval[0], evec[0], evec[1], evec[2], eval[1], evec[3], evec[4], evec[5], eval[2], evec[6], evec[7], evec[8], name, name, name, name, name, name);


  // outfile << result; 


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
  glm::mat4 scalingMatrix = glm::mat4(1000.0f, 0.0f, 0.0f, 0.0f, 
                                      0.0f, 1000.0f, 0.0f, 0.0f, 
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
  if (hitandquit) {
    seekIsovalueSet(sctx, isovalue);
    seekUpdate(sctx);
    seekExtract(sctx, liso);
    hiso.rebuffer();

    render(&viewer);
    glfwWaitEvents();
    render(&viewer);
    viewer.snap(name);
    Hale::done();
    airMopOkay(mop);
    return 0;
  }
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
