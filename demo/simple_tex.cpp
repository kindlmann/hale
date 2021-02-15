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

  char *name;
  char *texname1, *texname2;

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
  hestOptAdd(&hopt, "tex1", "texname1", airTypeString, 1, 1, &texname1, "tex1.png", "name of first texture image");
  hestOptAdd(&hopt, "tex2", "texname2", airTypeString, 1, 1, &texname2, "tex2.png", "name of second texture image");
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

  Nrrd *nimg = nrrdNew();
  if (nrrdLoad(nimg, texname1, NULL)) {
//  if (nrrdLoad(nimg, "/home/trihuynh/Desktop/Research/softwares/hale/hale/tmp.png", NULL)) {
     char *err = biffGetDone(NRRD);
     printf("couldn't load image A:\n%s\n", err);
     nrrdNuke(nimg); return 0;
  }
  if (!(3 == nimg->dim && 3 == nimg->axis[0].size && nrrdTypeUChar == nimg->type)) {
     printf("did not get a 3D 3-by-X-by-Y unsigned char image "
            "(got %u-D %u-by-? %s array)\n", nimg->dim, (unsigned int)nimg->axis[0].size,
            airEnumStr(nrrdType, nimg->type));
     nrrdNuke(nimg); return 0;
  }  

  Nrrd *nimg2 = nrrdNew();
  if (nrrdLoad(nimg2, texname2, NULL)) {
//  if (nrrdLoad(nimg2, "/home/trihuynh/Desktop/Research/softwares/hale/hale/tmp2.png", NULL)) {
     char *err = biffGetDone(NRRD);
     printf("couldn't load image B:\n%s\n", err);
     nrrdNuke(nimg2); return 0;
  }
  if (!(3 == nimg2->dim && 3 == nimg2->axis[0].size && nrrdTypeUChar == nimg2->type)) {
     printf("did not get a 3D 3-by-X-by-Y unsigned char image "
            "(got %u-D %u-by-? %s array)\n", nimg2->dim, (unsigned int)nimg2->axis[0].size,
            airEnumStr(nrrdType, nimg2->type));
     nrrdNuke(nimg2); return 0;
  }  

  Hale::Program *newprog = new Hale::Program("texdemo-vert.glsl","texdemo-frag.glsl");
  newprog->compile();
  newprog->bindAttribute(Hale::vertAttrIdxXYZW, "positionVA");
  newprog->bindAttribute(Hale::vertAttrIdxRGBA, "colorVA");
  newprog->bindAttribute(Hale::vertAttrIdxNorm, "normalVA");
  newprog->bindAttribute(Hale::vertAttrIdxTex2, "tex2VA");
  newprog->link();  

  limnPolyData *lpld = limnPolyDataNew();

  limnPolyDataSquare(lpld, 1 << limnPolyDataInfoNorm | 1 << limnPolyDataInfoTex2);
  
  Hale::Polydata hpld(lpld, true,
                        NULL,
                       "square");
  hpld.program(newprog);
  hpld.setTexture((char*)"myTextureSampler",nimg);
  scene.add(&hpld);


  limnPolyData *lpld2 = limnPolyDataNew();
  limnPolyDataSquare(lpld2, 1 << limnPolyDataInfoNorm | 1 << limnPolyDataInfoTex2);
  Hale::Polydata hpld2(lpld2, true,
                        NULL,
                       "square");

  hpld2.program(newprog);  
  hpld2.setTexture((char*)"myTextureSampler",nimg2);
  glm::mat4 tmat = glm::mat4();
  tmat[3][2] = 1;
  hpld2.model(tmat);
  scene.add(&hpld2);

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
