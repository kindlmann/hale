#include <iostream>
#include <Hale.h>
#include <glm/glm.hpp>

#include "unistd.h" // for sleep()

/*
 (done: generate fields, LIC)
 generate/render streamlines for each sample
 (done? eigenvalue normalization/scaling)
 (done? setting alpha)
 (done? eigenvector regularization)
rotation texture
overlaying with LIC result
checking that +-D and +-R is working right
*/

void
render(Hale::Viewer *viewer) {
    viewer->draw();
    viewer->bufferSwap();
}

int
pseigen(int *pseudo, double *eval, double *evec, const double DD, const double SS,
        const double matN[4], const double RR, const double tenOrig[4]) {
    static const char me[] = "pseigen";
    double ten[4], matR[4] = {0, -1, 1, 0};
    int ret;

    printf("   %s: %0.17g %s |%0.17g|=%0.17g\n", me, SS,
           SS > AIR_ABS(RR) ? ">" : "<=", RR, AIR_ABS(RR));
    if (SS > AIR_ABS(RR)) {
        *pseudo = 0; /* false */
        ELL_4V_SCALE_ADD2(ten, SS, matN, RR, matR);
    } else {
        *pseudo = 1; /* true */
        int rsgn = RR > 0 ? +1 : -1;
        printf("  %s: rsgn = %d\n", me, rsgn);
        /* HEY is is rsgn really needed? */
        ELL_4V_SCALE_ADD2(ten, RR / rsgn, matN, rsgn * SS, matR);
        // ELL_4V_SCALE_ADD2(ten, RR, matN, SS, matR);
    }
    printf("  %s: (pseudo %d) ten = {{%0.17g,%0.17g},{%0.17g,%0.17g}}\n", me, *pseudo,
           ten[0], ten[1], ten[2], ten[3]);

    ret = ell_2m_eigensolve_d(eval, evec, ten);
    printf("  %s: ell_2m_eigensolve_d returns %s:\n", me,
           airEnumStr(ell_quadratic_root, ret));
    printf("  %s: eval0: %0.17g (%0.17g,%0.17g)\n", me, eval[0], evec[0], evec[1]);
    printf("  %s: eval1: %0.17g (%0.17g,%0.17g)\n", me, eval[1], evec[2], evec[3]);
    if (*pseudo) {
        eval[0] = eval[1] = 1;
    } else {
        eval[0] += DD;
        eval[1] += DD;
    }
    double l2 = sqrt(eval[0] * eval[0] + eval[1] * eval[1]);
    if (l2) {
        eval[0] /= l2;
        eval[1] /= l2;
    }

    printf("  %s: new eval0: %0.17g\n", me, eval[0]);
    printf("  %s: new eval1: %0.17g (product = %g)\n", me, eval[1], eval[0] * eval[1]);

    double det = ELL_2M_DET(tenOrig); // 1 - 2*SS*SS;
    printf("  %s: det(tenOrig) = %g\n", me, det);
    if (det > 0) {
        det /= 2 * eval[0] * eval[1]; /* the 2* here was hard-won ... */
        printf("  %s: fixed0 det = %g\n", me, det);
        det = AIR_MIN(1, det); // just to be safe
        printf("  %s: fixed1 det = %g\n", me, det);
        double angle = asin(det);
        printf("  %s: angle = asin(det) = %g = (%g degrees)\n", me, angle,
               angle * 180 / AIR_PI);
        double tmp = ELL_2V_DOT(evec + 0, evec + 2);
        if (tmp < 0) {
            ELL_2V_SCALE(evec + 2, -1, evec + 2);
        }
        printf("    %s: det %g > 0 --------------\n", me, det);
        printf("    %s: evec0: (%0.17g,%0.17g)\n", me, evec[0], evec[1]);
        printf("    %s: evec1: (%0.17g,%0.17g)\n", me, evec[2], evec[3]);
        double evangle = ell_2v_angle_d(evec + 0, evec + 2);
        printf("    %s: evangle = %0.17g\n", me, evangle);
        double axis[2], perp[2];
        ELL_2V_ADD2(axis, evec + 0, evec + 2);
        ELL_2V_NORM(axis, axis, tmp);
        perp[0] = axis[1];
        perp[1] = -axis[0];
        printf("    %s: axis: (%0.17g,%0.17g)\n", me, axis[0], axis[1]);
        printf("    %s: perp: (%0.17g,%0.17g)\n", me, perp[0], perp[1]);
        if (evangle < angle) {
            /* the angle we want, based on determinant, is larger
               than the actual eigenvector angle */
            printf("    %s: %g < %g creating new evecs!!\n", me, evangle, angle);
            /* det = sin(angle) */
            double cc = cos(angle / 2);
            double ss = sin(angle / 2);
            double nevec[4];
            int esgn = ELL_2V_DOT(evec + 0, perp) > 0 ? 1 : -1;
            ELL_2V_SCALE_ADD2(nevec + 0, cc, axis, esgn * ss, perp);
            ELL_2V_SCALE_ADD2(nevec + 2, cc, axis, -esgn * ss, perp);
            ELL_4V_COPY(evec, nevec);
        }
    }

    printf("  %s leaving with:\n", me);
    printf("  %s: 0: %0.17g (%0.17g,%0.17g)\n", me, eval[0], evec[0], evec[1]);
    printf("  %s: 1: %0.17g (%0.17g,%0.17g)\n", me, eval[1], evec[2], evec[3]);
    return ret;
}

int
main(int argc, const char **argv) {
    const char *me;
    char *err;
    hestOpt *hopt = NULL;
    hestParm *hparm;
    airArray *mop;

    /* variables learned via hest */
    Nrrd *nin;
    float camfr[3], camat[3], camup[3], camnc, camfc, camFOV;
    int camortho, sgnDSR[3];
    unsigned int camsize[2];

    Hale::debugging = 0;

    /* boilerplate hest code */
    me = argv[0];
    mop = airMopNew();
    hparm = hestParmNew();
    airMopAdd(mop, hparm, (airMopper)hestParmFree, airMopAlways);
    /* setting up the command-line options */
    hparm->respFileEnable = AIR_TRUE;
    hparm->noArgsIsNoProblem = AIR_TRUE;
    hestOptAdd(&hopt, "fr", "x y z", airTypeFloat, 3, 3, camfr,
               "-673.394 42.9228 42.9228", "look-from point");
    hestOptAdd(&hopt, "at", "x y z", airTypeFloat, 3, 3, camat,
               "42.9228 42.9228 42.9228", "look-at point");
    hestOptAdd(&hopt, "up", "x y z", airTypeFloat, 3, 3, camup, "0 0 1", "up direction");
    hestOptAdd(&hopt, "nc", "dist", airTypeFloat, 1, 1, &(camnc), "-126.306",
               "at-relative near clipping distance");
    hestOptAdd(&hopt, "fc", "dist", airTypeFloat, 1, 1, &(camfc), "126.306",
               "at-relative far clipping distance");
    hestOptAdd(&hopt, "fov", "angle", airTypeFloat, 1, 1, &(camFOV), "20",
               "vertical field-of-view, in degrees. Full vertical "
               "extent of image plane subtends this angle.");
    hestOptAdd(&hopt, "sz", "s0 s1", airTypeUInt, 2, 2, &(camsize), "640 480",
               "# samples (horz vert) of image plane. ");
    hestOptAdd(&hopt, "dsr", "sgn sgn", airTypeInt, 3, 3, sgnDSR, "1 1 1",
               "sign of D, S, and R components");
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

    hestParseOrDie(hopt, argc - 1, argv + 1, hparm, me, "demo program", AIR_TRUE,
                   AIR_TRUE, AIR_TRUE);
    airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
    airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

    if (!(1 == AIR_ABS(sgnDSR[0]) && 1 == AIR_ABS(sgnDSR[1])
          && 1 == AIR_ABS(sgnDSR[2]))) {
        fprintf(stderr, "%s: DSR signs %d %d %d not all +/- 1\n", me, sgnDSR[0],
                sgnDSR[1], sgnDSR[2]);
        airMopError(mop);
        return 1;
    }

    /* then create empty scene */
    Hale::init();
    Hale::Scene scene;
    /* then create viewer (in order to create the OpenGL context) */
    Hale::Viewer viewer(camsize[0], camsize[1], "atg", &scene);
    viewer.lightDir(glm::vec3(-1.0f, 1.0f, 3.0f));
    viewer.camera.init(glm::vec3(camfr[0], camfr[1], camfr[2]),
                       glm::vec3(camat[0], camat[1], camat[2]),
                       glm::vec3(camup[0], camup[1], camup[2]), camFOV,
                       (float)camsize[0] / camsize[1], camnc, camfc, camortho);
    viewer.refreshCB((Hale::ViewerRefresher)render);
    viewer.refreshData(&viewer);
    NrrdRange *range = nrrdRangeNewSet(nin, AIR_FALSE);
    airMopAdd(mop, range, (airMopper)nrrdRangeNix, airMopAlways);
    viewer.current();

    /* then add to scene */

    limnPolyData *ldisc;
    Hale::Polydata *hdisc;
    float width = 20.0f;
    float ed = width / (nsamp - 1);
    float edgei[2] = {ed, 0.0};
    float edgej[2] = {-ed / 2, -sqrtf(3) * ed / 2};
    float orig[2];
    {
        float foo = width / (2 * sqrtf(3));
        float fjdx = (nsamp - 1) / 2.0f;
        float fidx = (nsamp - 1) / 4.0f;
        orig[0] = -fidx * edgei[0] - fjdx * edgej[0];
        orig[1] = -fidx * edgei[1] - fjdx * edgej[1];
    }
    if (-1 == sgnDSR[1]) {
        phi += AIR_PI / 2;
    }
    for (unsigned int jj = 0; jj < nsamp; jj++) {
        for (unsigned int ii = 0; ii < nsamp; ii++) {
            if (ii > jj) {
                continue;
            }

            double bary[3] = {AIR_AFFINE(0, jj, nsamp - 1, 1, 0), 0,
                              AIR_AFFINE(0, ii, nsamp - 1, 0, 1)};

            /* // cell-centered
            double bary[3] = {AIR_AFFINE(-0.5, jj, nsamp-0.5, 1, 0),
                              0,
                              AIR_AFFINE(-0.5, ii, nsamp-0.5, 0, 1)};
            */
            bary[1] = 1 - bary[0] - bary[2];
            double DSR[3] = {bary[0], bary[1], bary[2]}, tmp;
            DSR[0] *= sgnDSR[0];
            DSR[2] *= sgnDSR[2];
            ELL_3V_NORM(DSR, DSR, tmp);

            float pos[2] = {ii * edgei[0] + jj * edgej[0] + orig[0],
                            ii * edgei[1] + jj * edgej[1] + orig[1]};
            printf("[%u,%u] ============ bary(%g,%g,%g) @ XY(%g,%g)\n", ii, jj, bary[0],
                   bary[1], bary[2], pos[0], pos[1]);

            double matE[4] = {DSR[0], 0, 0, DSR[0]};
            double matN[4] = {cos(2 * phi), sin(2 * phi), sin(2 * phi), -cos(2 * phi)};
            double matA[4] = {0, -DSR[2], DSR[2], 0};
            double ten[4];
            ELL_4V_SCALE_ADD3(ten, 1.0, matE,           /* or DSR[0], ident */
                              DSR[1], matN, 1.0, matA); /* or DSR[2], {{0,-1},{1,0}} */
            printf("   ten = {{%.17g,%.17g},{%.17g,%.17g}}\n", ten[0], ten[1], ten[2],
                   ten[3]);

            double DD = (ten[0] + ten[3]) / 2; /* D = tr(T)/2 */
            double mtmp[4], matM[4];
            ELL_2M_TRANSPOSE(mtmp, ten);
            ELL_4V_SCALE_ADD2(matM, 0.5, ten, 0.5, mtmp);    /* M = (T + T^T)/2 */
            ELL_4V_SET(matE, DD, 0, 0, DD);                  /* E = D*{{1,0},{0,1}} */
            ELL_4V_SUB(matN, matM, matE);                    /* N = M - E */
            double SS = sqrt(AIR_MAX(0, -ELL_2M_DET(matN))); /* S = sqrt(-det(N)) */
            double recPhi = atan2(matN[1], matN[0]) / 2; /* phi = atan(N_12 / N_11)/2 */
            ELL_4V_SET(matN, cos(2 * recPhi), sin(2 * recPhi), sin(2 * recPhi),
                       -cos(2 * recPhi));
            ELL_4V_SCALE_ADD2(matA, 0.5, ten, -0.5, mtmp); /* A = (T - T^T)/2 */
            double RR = -matA[1];                          /* R = -A_12 */
            double recDSR[3] = {DD, SS, RR};
            ELL_3V_SUB(mtmp, DSR, recDSR);
            printf("   (%g) DSR = %g(%g)  %g(%g)  %g(%g)\n", ELL_3V_LEN(mtmp), DSR[0],
                   recDSR[0], DSR[1], recDSR[1], DSR[2], recDSR[2]);
            if (ELL_3V_LEN(mtmp) > 0.000001) {
                fprintf(stderr, "%s: BAD DSR!\n", me);
                exit(1);
            }
            double det = ELL_2M_DET(ten);
            double alpha
              = (AIR_ABS(RR) > SS
                   ? 1
                   : (det < 0
                        /* ? AIR_AFFINE(-1, det, 0, 4, 2) */
                        ? AIR_AFFINE(1 / sqrt(2), AIR_ABS(DD), 0, 2, 5)
                        : pow(AIR_AFFINE(0, SS - AIR_ABS(RR), 1.0 / sqrt(2), 1, 0),
                              1 /* "gamma" */)));

            int pseudo, eret;
            double eval[2], evec[4], rgb[3];
            eret = pseigen(&pseudo, eval, evec, DD, SS, matN, RR, ten);
            printf("   esolve: (%s) eval %g %g, evec (%g,%g)  (%g,%g)\n",
                   airEnumStr(ell_quadratic_root, eret), eval[0], eval[1], evec[0],
                   evec[1], evec[2], evec[3]);

            ldisc = limnPolyDataNew();
            limnPolyDataSuperquadric2D(ldisc, 1 << limnPolyDataInfoNorm,
                                       AIR_AFFINE(0, 0.0, 1, alpha, 1), 50);
            hdisc = new Hale::Polydata(ldisc, true,
                                       Hale::ProgramLib(Hale::preprogramAmbDiffSolid),
                                       "disc");
            // hdisc->colorSolid(bary[0], bary[1], bary[2]);
            /*
            hdisc->colorSolid(pseudo ? 1 - DSR[0] : DSR[0],
                              pseudo ? 1 - DSR[1] : DSR[1],
                              pseudo ? 1 - DSR[2] : DSR[2]);
            */

            switch (eret) {
            case ell_quadratic_root_two:
                ELL_3V_SET(rgb, 1, 0, 0);
                break;
            case ell_quadratic_root_double:
                ELL_3V_SET(rgb, 0, 1, 0);
                break;
            case ell_quadratic_root_complex:
                ELL_3V_SET(rgb, 0, 0, 1);
                break;
            }

            /*
            if (AIR_ABS(DD) > 1/sqrt(2)) {
              ELL_3V_SET(rgb, 0, 1, 0);
            } else {
              ELL_3V_SET(rgb, 1, 0, 0);
            }
            */
            hdisc->colorSolid(rgb[0], rgb[1], rgb[2]);
            float sc = gscale * ed / 2;
            float eps = 0.001;

            hdisc->model(glm::transpose(glm::mat4(
              sc * eval[0] * (eps + evec[0]), sc * eval[1] * evec[2], 0.0f, pos[0],
              sc * eval[0] * evec[1], sc * eval[1] * (eps + evec[3]), 0.0f, pos[1],

              /*
        hdisc->model(glm::transpose(glm::mat4(sc*(eps + evec[0]), sc*evec[2], 0.0f,
        pos[0], sc*evec[1],          sc*(eps + evec[3]), 0.0f, pos[1],
                                              */
              0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f)));
            scene.add(hdisc);
        }
    }

    scene.drawInit();
    render(&viewer);
    while (!Hale::finishing) {
        glfwWaitEvents();
        render(&viewer);
    }

    /* clean exit; all okay */
    Hale::done();
    airMopOkay(mop);
    return 0;
}
