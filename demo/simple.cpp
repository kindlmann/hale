#include <iostream>
#include <Hale.h>
#include <glm/glm.hpp>

#include "unistd.h" // for sleep()

#include <fstream>

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
  float camfr[3], camat[3], camup[3], camnc, camfc, camFOV;
  int camortho;
  unsigned int camsize[2];
  double isovalue, sliso;

  double evec[9], eval[3];
  double mean[3], cov[9];

  Nrrd *nout = nrrdNew();
  unsigned int bins = 2000;
  int type;

  Hale::debugging = 1;

  /* boilerplate hest code */
  me = argv[0];
  mop = airMopNew();
  hparm = hestParmNew();
  airMopAdd(mop, hparm, (airMopper)hestParmFree, airMopAlways);
  /* setting up the command-line options */
  hparm->respFileEnable = AIR_TRUE;
  hparm->noArgsIsNoProblem = AIR_TRUE;

  hestOptAdd(&hopt, "fr", "x y z", airTypeFloat, 3, 3, camfr, "0 0 8",
             "look-from point");
  hestOptAdd(&hopt, "at", "x y z", airTypeFloat, 3, 3, camat, "0 0 0",
             "look-at point");
  hestOptAdd(&hopt, "up", "x y z", airTypeFloat, 3, 3, camup, "1 0 0",
             "up direction");
  hestOptAdd(&hopt, "nc", "dist", airTypeFloat, 1, 1, &(camnc), "-1",
             "at-relative near clipping distance");
  hestOptAdd(&hopt, "fc", "dist", airTypeFloat, 1, 1, &(camfc), "1",
             "at-relative far clipping distance");
  hestOptAdd(&hopt, "fov", "angle", airTypeFloat, 1, 1, &(camFOV), "20",
             "vertical field-of-view, in degrees. Full vertical "
             "extent of image plane subtends this angle.");
  hestOptAdd(&hopt, "sz", "s0 s1", airTypeUInt, 2, 2, &(camsize), "640 480",
             "# samples (horz vert) of image plane. ");
  hestOptAdd(&hopt, "t", "thresh", airTypeDouble, 1, 1, &isovalue, "nan",
             "threshold value for voxels to analyze");
  hestOptAdd(&hopt, "n", "name", airTypeString, 1, 1, &name, "foo", "name of data set");
  camortho = 0;

  hestParseOrDie(hopt, argc-1, argv+1, hparm,
                 me, "demo program", AIR_TRUE, AIR_TRUE, AIR_TRUE);
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

  /* Compute threshold (isovalue) */

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

  limnPolyData *lpld = limnPolyDataNew();
  limnPolyDataSquare(lpld, 1 << limnPolyDataInfoNorm);
  /* want:
    limnPolyDataSquare(lpld, 1 << limnPolyDataInfoNorm | 1 << limnPolyDataInfoTex2);
  */
  Hale::Polydata hpld(lpld, true,
                       Hale::ProgramLib(Hale::preprogramAmbDiffSolid),
                       "square");
  hpld.colorSolid(1,0.5,0.5);
  scene.add(&hpld);

  scene.drawInit();
  render(&viewer);
  while(!Hale::finishing){
    glfwWaitEvents();
    render(&viewer);
  }

  /* clean exit; all okay */
  Hale::done();
  airMopOkay(mop);

  return 0;
}
