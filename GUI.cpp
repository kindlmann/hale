#include "GUI.h"
#include <glm/glm.hpp>

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






template<>
double convert<Eigen::Matrix<double, 1,1>, double, std::true_type>(const Eigen::Matrix<double, 1,1> &in){
	return in(0,0);
}

template<>
glm::vec2 convert<Eigen::Matrix<double, 2,1>, glm::vec2, std::true_type>(const Eigen::Matrix<double, 2,1> &v){
	return glm::vec2(v(0,0), v(1,0));
}
template<>
Eigen::Matrix<double, 2, 1> convert<glm::vec2, Eigen::Matrix<double, 2, 1>, std::true_type>(const glm::vec2 &v){
	return Eigen::Matrix<double, 2, 1>(v[0],v[1]);
}

template<>
glm::vec3 convert<Eigen::Matrix<double, 3,1>, glm::vec3, std::true_type>(const Eigen::Matrix<double, 3,1> &v){
	return glm::vec3(v(0,0), v(1,0), v(2,0));
}
template<>
Eigen::Matrix<double, 3, 1> convert<glm::vec3, Eigen::Matrix<double, 3, 1>, std::true_type>(const glm::vec3 &v){
	return Eigen::Matrix<double, 3, 1>(v[0],v[1],v[2]);
}

template<>
glm::vec4 convert<Eigen::Matrix<double, 4,1>, glm::vec4, std::true_type>(const Eigen::Matrix<double, 4,1> &v){
	return glm::vec4(v(0,0), v(1,0), v(2,0), v(3,0));
}
template<>
Eigen::Matrix<double, 4, 1> convert<glm::vec4, Eigen::Matrix<double, 4, 1>, std::true_type>(const glm::vec4 &v){
	return Eigen::Matrix<double, 4, 1>(v[0],v[1],v[2],v[3]);
}

/* Eigen::Matrix<double,2,2> -> glm::mat2x2 */
template<>
glm::mat2x2 convert<Eigen::Matrix<double,2,2>, glm::mat2x2, std::true_type>
  (const Eigen::Matrix<double,2,2> &v){
    return glm::mat2x2(v(0,0),v(1,0),v(0,1),v(1,1)); 
}
/* glm::mat2x2 -> Eigen::Matrix<double,2,2> */
template<>
Eigen::Matrix<double,2,2> convert<glm::mat2x2, Eigen::Matrix<double,2,2>, std::true_type>
  (const glm::mat2x2 &v){
  	Eigen::Matrix<double,2,2> m;
  	m << v[0][0], v[0][1],
  	     v[1][0], v[1][1];
    return m; 
}

/* Eigen::Matrix<double,2,3> -> glm::mat2x3 */
template<>
glm::mat2x3 convert<Eigen::Matrix<double,2,3>, glm::mat2x3, std::true_type>
  (const Eigen::Matrix<double,2,3> &v){
    return glm::mat2x3( v(0,0),v(1,0),v(0,1),v(1,1),v(0,2),v(1,2)); 
}
/* glm::mat2x3 -> Eigen::Matrix<double,2,3> */
template<>
Eigen::Matrix<double,2,3> convert<glm::mat2x3, Eigen::Matrix<double,2,3>, std::true_type>
  (const glm::mat2x3 &v){
    Eigen::Matrix<double,2,3> m;
    m << v[0][0], v[0][1], v[0][2],
         v[1][0], v[1][1], v[1][2];
    return m;
}

/* Eigen::Matrix<double,2,4> -> glm::mat2x4 */
template<>
glm::mat2x4 convert<Eigen::Matrix<double,2,4>, glm::mat2x4, std::true_type>
  (const Eigen::Matrix<double,2,4> &v){
    return glm::mat2x4( v(0,0),v(1,0),v(0,1),v(1,1),v(0,2),v(1,2),v(0,3),v(1,3)); 
}
/* glm::mat2x4 -> Eigen::Matrix<double,2,4> */
template<>
Eigen::Matrix<double,2,4> convert<glm::mat2x4, Eigen::Matrix<double,2,4>, std::true_type>
  (const glm::mat2x4 &v){
    Eigen::Matrix<double,2,4> m;
    m << v[0][0], v[0][1], v[0][2],
         v[1][0], v[1][1], v[1][2];
    return m;
}

/* Eigen::Matrix<double,3,3> -> glm::mat3x3 */
template<>
glm::mat3x3 convert<Eigen::Matrix<double,3,3>, glm::mat3x3, std::true_type>
  (const Eigen::Matrix<double,3,3> &v){
    return glm::mat3x3(v(0,0),v(1,0),v(2,0),
    				   v(0,1),v(1,1),v(2,1),
    				   v(0,2),v(1,2),v(2,2)); 
}
/* glm::mat3x3 -> Eigen::Matrix<double,3,3> */
template<>
Eigen::Matrix<double,3,3> convert<glm::mat3x3, Eigen::Matrix<double,3,3>, std::true_type>
  (const glm::mat3x3 &v){
    Eigen::Matrix<double,3,3> m;
    m << v[0][0], v[0][1], v[0][2],
         v[1][0], v[1][1], v[1][2],
         v[2][0], v[2][1], v[2][2];
    return m;
}

/* Eigen::Matrix<double,4,4> -> glm::mat4x4 */
template<>
glm::mat4x4 convert<Eigen::Matrix<double,4,4>, glm::mat4x4, std::true_type>
  (const Eigen::Matrix<double,4,4> &v){
    return glm::mat4x4(v(0,0),v(1,0),v(2,0),v(3,0),
    				   v(0,1),v(1,1),v(2,1),v(3,1),
    				   v(0,2),v(1,2),v(2,2),v(3,2),
    				   v(0,3),v(1,3),v(2,3),v(3,3)); 
}
/* glm::mat4x4 -> Eigen::Matrix<double,4,4> */
template<>
Eigen::Matrix<double,4,4> convert<glm::mat4x4, Eigen::Matrix<double,4,4>, std::true_type>
  (const glm::mat4x4 &v){
    Eigen::Matrix<double,4,4> m;
    m << v[0][0], v[0][1], v[0][2], v[0][3],
         v[1][0], v[1][1], v[1][2], v[1][3],
         v[2][0], v[2][1], v[2][2], v[2][3],
         v[3][0], v[3][1], v[3][2], v[3][3];
    return m;
}


// TODO implement:

/* Eigen::Matrix<double,4,3> -> glm::mat4x3 */
template<>
glm::mat4x3 convert<Eigen::Matrix<double,4,3>, glm::mat4x3, std::true_type>
  (const Eigen::Matrix<double,4,3> &v){
    return glm::mat4x3(v(0,0),v(1,0),v(2,0),
    				   v(0,1),v(1,1),v(2,1),
    				   v(0,2),v(1,2),v(2,2),
    				   v(0,3),v(1,3),v(2,3)); 
}
/* glm::mat4x3 -> Eigen::Matrix<double,4,3> */
template<>
Eigen::Matrix<double,4,3> convert<glm::mat4x3, Eigen::Matrix<double,4,3>, std::true_type>
  (const glm::mat4x3 &v){
    Eigen::Matrix<double,4,3> m;
    m << v[0][0], v[0][1], v[0][2],
         v[1][0], v[1][1], v[1][2],
         v[2][0], v[2][1], v[2][2],
         v[3][0], v[3][1], v[3][2];
    return m;
}

/* Eigen::Matrix<double,4,2> -> glm::mat4x2 */
template<>
glm::mat4x2 convert<Eigen::Matrix<double,4,2>, glm::mat4x2, std::true_type>
  (const Eigen::Matrix<double,4,2> &v){
    return glm::mat4x2(v(0,0),v(1,0),
    				   v(0,1),v(1,1),
    				   v(0,2),v(1,2),
    				   v(0,3),v(1,3)); 
}
/* glm::mat4x2 -> Eigen::Matrix<double,4,2> */
template<>
Eigen::Matrix<double,4,2> convert<glm::mat4x2, Eigen::Matrix<double,4,2>, std::true_type>
  (const glm::mat4x2 &v){
    Eigen::Matrix<double,4,2> m;
    m << v[0][0], v[0][1],
         v[1][0], v[1][1],
         v[2][0], v[2][1],
         v[3][0], v[3][1];
    return m;
}

/* Eigen::Matrix<double,3,4> -> glm::mat3x4 */
template<>
glm::mat3x4 convert<Eigen::Matrix<double,3,4>, glm::mat3x4, std::true_type>
  (const Eigen::Matrix<double,3,4> &v){
    return glm::mat3x4(v(0,0),v(1,0),v(2,0),v(3,0),
    				   v(0,1),v(1,1),v(2,1),v(3,1),
    				   v(0,2),v(1,2),v(2,2),v(3,2)); 
}
/* glm::mat3x4 -> Eigen::Matrix<double,3,4> */
template<>
Eigen::Matrix<double,3,4> convert<glm::mat3x4, Eigen::Matrix<double,3,4>, std::true_type>
  (const glm::mat3x4 &v){
    Eigen::Matrix<double,3,4> m;
    m << v[0][0], v[0][1], v[0][2], v[0][3],
         v[1][0], v[1][1], v[1][2], v[1][3],
         v[2][0], v[2][1], v[2][2], v[2][3];
    return m;
}

/* Eigen::Matrix<double,3,2> -> glm::mat3x2 */
template<>
glm::mat3x2 convert<Eigen::Matrix<double,3,2>, glm::mat3x2, std::true_type>
  (const Eigen::Matrix<double,3,2> &v){
    return glm::mat3x2(v(0,0),v(1,0),
    				   v(0,1),v(1,1),
    				   v(0,2),v(1,2)); 
}
/* glm::mat3x2 -> Eigen::Matrix<double,3,2> */
template<>
Eigen::Matrix<double,3,2> convert<glm::mat3x2, Eigen::Matrix<double,3,2>, std::true_type>
  (const glm::mat3x2 &v){
    Eigen::Matrix<double,3,2> m;
    m << v[0][0], v[0][1],
         v[1][0], v[1][1],
         v[2][0], v[2][1];
    return m;
}

