
#ifndef CONVERT_H
#define CONVERT_H

#include <glm/glm.hpp>
	
template<typename T, typename S, typename sfinae = std::true_type>
S convert(const T &in);

// convert<> function implementation

// template<typename T, typename S, typename sfinae = std::true_type>
// S convert(const T &in){
//     return S(in);
// }

template<typename T, typename S, typename std::is_same<T,S>::type>
S convert(const T &in){
    return in;
}
template<typename T, typename S, typename std::is_convertible<T,S>::type>
S convert(const T &in){
    return (S)in;
}

#endif