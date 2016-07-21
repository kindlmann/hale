#include "GUI.h"

bool HCI::initialized = false;

std::vector<HCI::param*> HCI::parameters;

  /* boilerplate hest code */

hestOpt *HCI::hopt =0;
char *HCI::err =0;
hestParm *HCI::hparm =0;
airArray *HCI::mop =0;
