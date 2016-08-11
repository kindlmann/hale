
#ifndef CONVERT_H
#define CONVERT_H

#include <glm/glm.hpp>
	

/*
 * Helper function to convert between two types using
 * Hale-specific conversion strategies. Implementation
 * in GUI.cpp.
 */

template<typename T, typename S, typename sfinae = std::true_type>
S convert(const T &in);

template<typename T, typename S, typename std::is_same<T,S>::type>
S convert(const T &in){
    return in;
}
template<typename T, typename S, typename std::is_convertible<T,S>::type>
S convert(const T &in){
    return (S)in;
}


/*
 * Helper template to determine whether a particular type
 * can be represented as either a glm or an Eigen matrix of
 * doubles.
 *
 */
template<class T>
class is_glm_mat_type{
public: 
  typedef std::false_type type;
  static const int M = 0;
  static const int N = 0;
};
template<> class is_glm_mat_type<glm::vec2>{public:typedef std::true_type type; static const int M = 1; static const int N = 2;};
template<> class is_glm_mat_type<glm::vec3>{public:typedef std::true_type type; static const int M = 1; static const int N = 3;};
template<> class is_glm_mat_type<glm::vec4>{public:typedef std::true_type type; static const int M = 1; static const int N = 4;};
template<> class is_glm_mat_type<glm::mat4x4>{public:typedef std::true_type type; static const int M = 4; static const int N = 4;};
template<> class is_glm_mat_type<glm::mat4x3>{public:typedef std::true_type type; static const int M = 3; static const int N = 4;};
template<> class is_glm_mat_type<glm::mat4x2>{public:typedef std::true_type type; static const int M = 2; static const int N = 4;};
template<> class is_glm_mat_type<glm::mat3x4>{public:typedef std::true_type type; static const int M = 4; static const int N = 3;};
template<> class is_glm_mat_type<glm::mat3x3>{public:typedef std::true_type type; static const int M = 3; static const int N = 3;};
template<> class is_glm_mat_type<glm::mat3x2>{public:typedef std::true_type type; static const int M = 2; static const int N = 3;};
template<> class is_glm_mat_type<glm::mat2x4>{public:typedef std::true_type type; static const int M = 4; static const int N = 2;};
template<> class is_glm_mat_type<glm::mat2x3>{public:typedef std::true_type type; static const int M = 3; static const int N = 2;};
template<> class is_glm_mat_type<glm::mat2x2>{public:typedef std::true_type type; static const int M = 2; static const int N = 2;};

template<int Ni, int Mi> class is_glm_mat_type<Eigen::Matrix<double, Ni, Mi>>{
public:
  typedef std::true_type type;
  static const int M = Mi;
  static const int N = Ni;
};

#endif