#include "GUI.h"

bool HCI::initialized = false;

std::vector<HCI::param*> HCI::parameters;

  /* boilerplate hest code */

hestOpt *HCI::hopt =0;
char *HCI::err =0;
hestParm *HCI::hparm =0;
airArray *HCI::mop =0;



template<> int getAirType<bool>(){ return airTypeBool; }
template<> int getAirType<int>(){ return airTypeInt; }
template<> int getAirType<unsigned int>(){ return airTypeUInt; }
template<> int getAirType<long int>(){ return airTypeLongInt; }
template<> int getAirType<unsigned long int>(){ return airTypeULongInt; }
template<> int getAirType<float>(){ return airTypeFloat; }
template<> int getAirType<double>(){ return airTypeDouble; }
template<> int getAirType<char>(){ return airTypeChar; }
template<> int getAirType<char*>(){ return airTypeString; }
template<> int getAirType<airEnum>(){ return airTypeEnum; }

// enum {
//   airTypeUnknown,   /*  0 */
//   airTypeBool,      /*  1 */
//   airTypeInt,       /*  2 */
//   airTypeUInt,      /*  3 */
//   airTypeLongInt,   /*  4 */
//   airTypeULongInt,  /*  5 */
//   airTypeSize_t,    /*  6 */
//   airTypeFloat,     /*  7 */
//   airTypeDouble,    /*  8 */
//   airTypeChar,      /*  9 */
//   airTypeString,    /* 10 */
//   airTypeEnum,      /* 11 */
//   airTypeOther,     /* 12 */
//   airTypeLast
// };