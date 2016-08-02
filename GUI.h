#ifndef HALEGUI_INCLUDED
#define HALEGUI_INCLUDED

#include <teem/meet.h>
#include <nanogui/screen.h>
#include <nanogui/textbox.h>
#include <nanogui/checkbox.h>
#include <nanogui/colorpicker.h>
#include <nanogui/colorwheel.h>
#include <nanogui/combobox.h>
#include <nanogui/slider.h>
#include "vectorbox.h"
#include <glm/glm.hpp>
#include <vector>
#include <iostream>
#include <list>
#include <iterator>

template<class T>
class VariableBinding;
/*
 * A wrapper over a particular variable. Allows a consistent means
 * of getting/setting data from the GUI and otherwise.
 */

class GenericBoundWidget {
public:
  virtual void updateFromBinding() = 0;  
};

template <typename T, typename S, typename sfinae> class BoundWidget;



class GenericVariableBinding{
public:
    GenericVariableBinding(const char* myname) : name(myname), changed(false){

    }
    virtual ~GenericVariableBinding(){}
    const char* const name;
    bool changed;
};

template<typename T>
class VariableExposure{
public:
  VariableExposure(void* owner, char* name, std::function<T(void)> getter, std::function<void(T)> setter){
    this->getter = getter;
    this->setter = setter;
    this->owner = owner;
    this->name = name;
    binding = 0;
  }
  std::function<T(void)> getter;
  std::function<void(T)> setter;
  void* owner;
  char* name;
  static std::vector<VariableExposure<T>> exposedVariables;
  bool bind(VariableBinding<T>* vb){
    if(!binding){
      binding = vb;
      return true;
    }
    return false;
  }
  static void expose(void* owner, char* name, std::function<T(void)> getter, std::function<void(T)> setter){
    exposedVariables.push_back(VariableExposure(owner, name, getter, setter));
  }
  static void printExposedVariables(){
    // debug function. todo: remove.
    fprintf(stderr,"Exposed Variables:\n");
    for(VariableExposure<T> ve : exposedVariables){
      fprintf(stderr, "  %p: %s\n", ve.owner, ve.name);
    }
  }
  static std::vector<std::pair<void*,std::string>> getExposedVariables(){
    std::vector<std::pair<void*,std::string>> ret;
    for(VariableExposure<T> ve : exposedVariables){
      ret.push_back(std::pair<void*,std::string>(ve.owner,std::string(ve.name)));
    }
    return ret;
  }
  static VariableBinding<T>* getVariableBinding(void* owner, char* name){
    for(VariableExposure<T> ve : exposedVariables){
      if(ve.owner == owner && !strcmp(ve.name, name)){
        ve.binding = new VariableBinding<T>(name, ve.getter, ve.setter);
        return ve.binding;
      }
    }
    return NULL;
  }
private:
  VariableBinding<T> *binding;
};

template<typename T>
typename std::vector<VariableExposure<T>> VariableExposure<T>::exposedVariables;

template<class T>
class VariableBinding : public GenericVariableBinding{
public:
  typedef T (*t_getter)();
  typedef void (*t_setter)(T);
  typedef std::function<T(void)> t_fun_getter;
  typedef std::function<void(T)> t_fun_setter;
protected:
  T* value;               // pointer to real value, if a setter/getter are not used.
  const t_getter getter;  // the function which gets the value (wherever it may be).
  const t_setter setter;  // the function which sets the value.
  const t_fun_getter fGetter;
  const t_fun_setter fSetter; 
  bool deleteonexit;      // whether we should free (value) on exit.

  std::list<GenericBoundWidget*> boundWidgets;
  std::list<std::function<void(const T)>> varChangeListeners;
public:
  VariableBinding(const char* myname, T init_value) : GenericVariableBinding(myname), getter(0), setter(0), fGetter(0), fSetter(0), deleteonexit(false){
    this->value  = new T;
    *(this->value) = init_value;
    this->changed = true;
  }
  VariableBinding(const char* name, t_getter get, t_setter set) : GenericVariableBinding(name), getter(get), setter(set), fGetter(0), fSetter(0), deleteonexit(false){
    this->value  = 0;  
    this->changed = true;
  }
  VariableBinding(const char* name, VariableExposure<T>* exp) : GenericVariableBinding(name), getter(0), setter(0), fGetter(exp.getter), fSetter(exp.setter), deleteonexit(false){
    this->value  = 0;
    this->changed = true;
  }
  VariableBinding(const char* name, std::function<T(void)> get, std::function<void(T)> set) : GenericVariableBinding(name), getter(0), setter(0), fGetter(get), fSetter(set), deleteonexit(false){
    this->value  = 0;
    this->changed = true;
  }
  VariableBinding(const char* name, T* t_ptr) : GenericVariableBinding(name), getter(0), setter(0), fGetter(0), fSetter(0), deleteonexit(true){
    this->value  = t_ptr;
    this->changed = true;
  }
  virtual ~VariableBinding(){
    if(value && deleteonexit)delete value;
  }
  void updateBound(){
    for (std::list<GenericBoundWidget*>::const_iterator itr = boundWidgets.begin(), end = boundWidgets.end(); itr != end; ++itr) {
        (*itr)->updateFromBinding();
    }
    for (typename std::list<std::function<void(T)>>::const_iterator itr = varChangeListeners.begin(), end = varChangeListeners.end(); itr != end; ++itr) {
        (*itr)(*value);
    }
  }
  void bindWidget(GenericBoundWidget* e){
    boundWidgets.push_back(e);
    e->updateFromBinding();
  }
  void addListener(std::function<void(T)> listener){
    varChangeListeners.push_back(listener);
  }
  // set the real value of this variable.
  void setValue(T in){
    setValue(&in);
  }
  // set value, pass-by-reference
  void setValue(T* in){
    // fprintf(stderr,"%s: setting value with ", name);
    if(setter){
        // fprintf(stderr,"functor\n");
        setter(*in);
    }else if(value){
        // fprintf(stderr,"hidden pointer\n");
        *value = *in;
    }else{
        // fprintf(stderr,"std::function\n");
        fSetter(*in);
    }
    updateBound();
    this->changed = true;
  }
  
  // get the real value of this variable.
  T getValue(){
    if(getter){
        return getter();
    }else if(value){
        return *value;
    }else{
        return fGetter();
    }
  }
  // returns whether this value has been set since the
  // last call to this function.
  bool hasChanged(){
    if(changed){
        changed = false;
        return true;
    }
    return false;
  }
};



// http://stackoverflow.com/questions/8752837/undefined-reference-to-template-class-constructor

template <typename T, typename S, typename sfinae = std::true_type> class BoundWidget;
// We do this for each variable/widget pair we need.


template <> class BoundWidget<std::string, nanogui::TextBox, std::true_type> : public nanogui::TextBox, public GenericBoundWidget {
protected:
  VariableBinding<std::string>* mBinding;
public:
  BoundWidget(nanogui::Widget *p, VariableBinding<std::string>* binding) : nanogui::TextBox(p, "text"), mBinding(binding){
    binding->bindWidget(this);
    fprintf(stderr,"\ncreated bound widget.\n");
    setEditable(true);
    TextBox::setCallback([binding](const std::string &str) {
        binding->setValue(str);
        return true;
    });
  }
  void updateFromBinding(){
    // update in such a way that the callback does NOT get called.
    mValue = mBinding->getValue();
  }
};

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
// template<> class is_glm_mat_type<glm::mat4>{public:typedef std::true_type type; static const int M = 4; static const int N = 4;};
template<> class is_glm_mat_type<glm::mat3x4>{public:typedef std::true_type type; static const int M = 4; static const int N = 3;};
template<> class is_glm_mat_type<glm::mat3x3>{public:typedef std::true_type type; static const int M = 3; static const int N = 3;};
template<> class is_glm_mat_type<glm::mat3x2>{public:typedef std::true_type type; static const int M = 2; static const int N = 3;};
// template<> class is_glm_mat_type<glm::mat3>{public:typedef std::true_type type; static const int M = 3; static const int N = 3;};
template<> class is_glm_mat_type<glm::mat2x4>{public:typedef std::true_type type; static const int M = 4; static const int N = 2;};
template<> class is_glm_mat_type<glm::mat2x3>{public:typedef std::true_type type; static const int M = 3; static const int N = 2;};
template<> class is_glm_mat_type<glm::mat2x2>{public:typedef std::true_type type; static const int M = 2; static const int N = 2;};
// template<> class is_glm_mat_type<glm::mat2>{public:typedef std::true_type type; static const int M = 2; static const int N = 2;};

template<int Ni, int Mi> class is_glm_mat_type<Eigen::Matrix<double, Ni, Mi>>{
public:
  typedef std::true_type type;
  static const int M = Mi;
  static const int N = Ni;
};


// template <typename T> class BoundWidget<T, MatrixBox<T::N,T::M>, typename is_glm_mat_type<T>::type> : public MatrixBox<T::N,T::M>, public GenericBoundWidget {
template <typename T> class BoundWidget<T, MatrixBox<is_glm_mat_type<T>::N,is_glm_mat_type<T>::M,T>, typename is_glm_mat_type<T>::type> : public MatrixBox<is_glm_mat_type<T>::N,is_glm_mat_type<T>::M,T>, public GenericBoundWidget {
protected:
  static const int M = is_glm_mat_type<T>::M;
  static const int N = is_glm_mat_type<T>::N;
  VariableBinding<T>* mBinding;
public:
  BoundWidget(nanogui::Widget *p, VariableBinding<T>* binding) : MatrixBox<N,M,T>(p), mBinding(binding){
    binding->bindWidget(this);
    fprintf(stderr,"\ncreated bound widget!\n");
    MatrixBox<N, M,T>::setMatrix(binding->getValue());
    fprintf(stderr,"\nset matrix!\n");
    MatrixBox<N, M,T>::setCallback([binding](const T &val) {
        binding->setValue(val);
        fprintf(stderr,"setting value\n");
        // fprintf(stderr,"ortho v: %d\n", binding->getValue()?1:0);
        return true;
    });
  }
  void updateFromBinding(){
    // update in such a way that the callback does NOT get called.
    MatrixBox<N,M,T>::mMatrix = convert<T, Eigen::Matrix<double, N,M>>(mBinding->getValue());
  }
};

template <> class BoundWidget<bool, nanogui::CheckBox, std::true_type> : public nanogui::CheckBox, public GenericBoundWidget {
protected:
  VariableBinding<bool>* mBinding;
public:
  BoundWidget(nanogui::Widget *p, VariableBinding<bool>* binding) : nanogui::CheckBox(p), mBinding(binding){
    binding->bindWidget(this);
    fprintf(stderr,"\ncreated bound widget.\n");
    mCaption = binding->name;
    CheckBox::setCallback([binding](const bool &val) {
        binding->setValue(val);
        fprintf(stderr,"ortho v: %d\n", binding->getValue()?1:0);
        return true;
    });
  }
  void updateFromBinding(){
    // update in such a way that the callback does NOT get called.
    mChecked = mBinding->getValue();
  }
};

template <> class BoundWidget<nanogui::Color, nanogui::ColorPicker, std::true_type> : public nanogui::ColorPicker, public GenericBoundWidget {
protected:
  VariableBinding<nanogui::Color>* mBinding;
public:
  BoundWidget(nanogui::Widget *p, VariableBinding<nanogui::Color>* binding) : nanogui::ColorPicker(p), mBinding(binding){
    binding->bindWidget(this);
    fprintf(stderr,"\ncreated bound widget.\n");
    // mCaption = binding->name;
    nanogui::ColorPicker::setCallback([binding](const nanogui::Color &val) {
        binding->setValue(val);
        return true;
    });
  }
  void updateFromBinding(){
    // update in such a way that the callback does NOT get called.
    nanogui::Color color = mBinding->getValue();
    nanogui::Color fg = color.contrastingColor();
    nanogui::ColorPicker::setBackgroundColor(color);
    nanogui::ColorPicker::setTextColor(fg);
    mColorWheel->setColor(color);
    nanogui::ColorPicker::mPickButton->setBackgroundColor(color);
    nanogui::ColorPicker::mPickButton->setTextColor(fg);
  }
};
/*
template <> class BoundWidget<glm::vec3, nanogui::ColorPicker, std::true_type> : public nanogui::ColorPicker, public GenericBoundWidget {
protected:
  VariableBinding<nanogui::Color>* mBinding;
public:
  BoundWidget(nanogui::Widget *p, VariableBinding<nanogui::Color>* binding) : nanogui::ColorPicker(p), mBinding(binding){
    binding->bindWidget(this);
    fprintf(stderr,"\ncreated bound widget.\n");
    // mCaption = binding->name;
    nanogui::ColorPicker::setCallback([binding](const nanogui::Color &val) {
        binding->setValue(val);
        return true;
    });
  }
  void updateFromBinding(){
    // update in such a way that the callback does NOT get called.
    nanogui::Color color = mBinding->getValue();
    nanogui::Color fg = color.contrastingColor();
    nanogui::ColorPicker::setBackgroundColor(color);
    nanogui::ColorPicker::setTextColor(fg);
    mColorWheel->setColor(color);
    nanogui::ColorPicker::mPickButton->setBackgroundColor(color);
    nanogui::ColorPicker::mPickButton->setTextColor(fg);
  }
};
*/

template <typename T> class BoundWidget<T, nanogui::IntBox<T>, typename std::is_integral<T>::type> : public nanogui::IntBox<T>, public GenericBoundWidget {
protected:
  VariableBinding<T>* mBinding;
public:
  BoundWidget(nanogui::Widget *p, VariableBinding<T>* binding) : nanogui::IntBox<T>(p, 12), mBinding(binding){
    binding->bindWidget(this);
    fprintf(stderr,"\ncreated bound widget.\n");
    nanogui::IntBox<T>::setEditable(true);
    nanogui::IntBox<T>::setCallback([binding](const T val) {
        binding->setValue(val);
        return true;
    });
  }
  void updateFromBinding(){
    // update in such a way that the callback does NOT get called.
    nanogui::TextBox::mValue = std::to_string(mBinding->getValue());
  }
};

template <typename T> class BoundWidget<T, nanogui::FloatBox<T>, typename std::is_floating_point<T>::type> : public nanogui::FloatBox<T>, public GenericBoundWidget {
protected:
  VariableBinding<T>* mBinding;
public:
  BoundWidget(nanogui::Widget *p, VariableBinding<T>* binding) : nanogui::FloatBox<T>(p, 12), mBinding(binding){
    binding->bindWidget(this);
    fprintf(stderr,"\ncreated bound widget.\n");
    nanogui::FloatBox<T>::setEditable(true);
    nanogui::FloatBox<T>::setCallback([binding](const T val) {
        binding->setValue(val);
        return true;
    });
  }
  void updateFromBinding(){
    // update in such a way that the callback does NOT get called.
    nanogui::TextBox::mValue = std::to_string(mBinding->getValue());
  }
};
/*
** _airEnumIndex()
**
** given an enum "enm" and value "val", return the index into enm->str[]
** and enm->desc[] which correspond to that value.  To be safe, when
** given an invalid enum value, we return zero.
*/
static unsigned int
_airEnumIndex(const airEnum *enm, int val) {
  unsigned int ii, ret;

  ret = 0;
  if (enm->val) {
    for (ii=1; ii<=enm->M; ii++) {
      if (val == enm->val[ii]) {
        ret = ii;
        break;
      }
    }
  } else {
    unsigned int uval;
    uval = AIR_UINT(val);
    ret = (0 <= val && uval <= enm->M) ? uval : 0;
  }
  return ret;
}

template <> class BoundWidget<int, nanogui::ComboBox, std::true_type> : public nanogui::ComboBox, public GenericBoundWidget {
protected:
  VariableBinding<int>* mBinding;
  const airEnum *myAirEnum;
public:
  BoundWidget(nanogui::Widget *p, VariableBinding<int>* binding, std::vector<std::string> names) : myAirEnum(0), nanogui::ComboBox(p), mBinding(binding){
    binding->bindWidget(this);
    fprintf(stderr,"\ncreated bound widget.\n");
    auto self = this;
    setItems(names,names);
    nanogui::ComboBox::setCallback([binding, self](const int ind) {
        binding->setValue(ind);
        return true;
    });
  }
  BoundWidget(nanogui::Widget *p, VariableBinding<int>* binding, const airEnum* aenum) : myAirEnum(aenum), nanogui::ComboBox(p), mBinding(binding){
    binding->bindWidget(this);
    fprintf(stderr,"\ncreated bound widget.\n");
    auto self = this;

    std::vector<std::string> names;
    for(int i=1;i<aenum->M;++i){
      names.push_back(std::string(aenum->str[i]));
    }
    setItems(names,names);
    nanogui::ComboBox::setCallback([binding, self](const int ind) {
      fprintf(stderr, "set to %d\n", ind);
      if(self->myAirEnum){
        if(self->myAirEnum->val){
          binding->setValue(self->myAirEnum->val[ind+1]);
        }
        else{
          binding->setValue(ind+1);
          fprintf(stderr, "actually-> %d\n", ind+1);
        }
      }
      else{
        binding->setValue(ind);
      }
      return true;
    });
  }
  void updateFromBinding(){
    // update in such a way that the callback does NOT get called.
    if(myAirEnum){
      if(myAirEnum->val){
        nanogui::ComboBox::setSelectedIndex(_airEnumIndex(myAirEnum, mBinding->getValue())-1);
      }
      else{
        nanogui::ComboBox::setSelectedIndex(mBinding->getValue()-1);
      }
    }
    else{
      nanogui::ComboBox::setSelectedIndex(mBinding->getValue());
    }
  }
};


template <typename T> class BoundWidget<T, nanogui::Slider, typename std::is_arithmetic<T>::type> : public nanogui::Slider, public GenericBoundWidget {
protected:
  T mMin;
  T mMax;
  VariableBinding<T>* mBinding;
public:
  BoundWidget(nanogui::Widget *p, VariableBinding<T>* binding): nanogui::Slider(p), mBinding(binding){
    mMin = 0;
    mMax = 10;
    binding->bindWidget(this);
    fprintf(stderr,"\ncreated bound widget.\n");
    auto self = this;
    nanogui::Slider::setCallback([binding, self](const T val) {
        binding->setValue(self->mMin + val*(self->mMax - self->mMin));
        return true;
    });
  }
  void updateFromBinding(){
    // update in such a way that the callback does NOT get called.
    nanogui::Slider::mValue = (float)(mBinding->getValue()-mMin)/((float)mMax - mMin);
  }
  void setRange(T vmin, T vmax){
    mMin = vmin;
    mMax = vmax;
    updateFromBinding();
  }
};

template <typename T>
int getAirType();

template<typename T>
int getAirType(){
  return airTypeOther;
}

template<typename T>
nanogui::Widget *createWidget(nanogui::Widget* parent, VariableBinding<T>* binding, char* widgetType);


// template<typename T> typename std::enable_if<std::is_floating_point<T>::value, nanogui::Widget*>::type createWidget(nanogui::Widget* parent, VariableBinding<T>* binding, char* widgetType){
//   if(!strcmp(widgetType, "floatbox")){
//     return new BoundWidget<T, nanogui::FloatBox<T>>(parent, binding);
//   }
//   else{
//     fprintf(stderr,"error in GUI::createWidget: type mismatch for BoundWidget type %s\n", widgetType);
//     exit(0);
//   }
// }

class HCI{
protected:
  struct param{
    void* hestPtr;
    GenericVariableBinding* binding;
    char *flag;
    char *name;
    int type;
    int minargs;
    int maxargs;
    char* argDefault;
    char* info;
    void (*createBindingFun)(param*);
  };
public:
  static bool initialized;
  static std::vector<param*> parameters;

  /* boilerplate hest code */

  static hestOpt *hopt;
  static char *err;
  static hestParm *hparm;
  static airArray *mop;

  

  // handle input parameters:

  // Function that creates a variable binding for a
  // particular type inside a given param object.
  // Hest returns data that can be converted bitwise (C-style)
  // to type R. This is then c++-style casted to a
  // type T.
  template<typename T, typename R>
  static void createVariableBinding(param *pm){
    // reinterpret the data which hest gives us to construct a T.
    T v = T(*((R*)(pm->hestPtr)));
    // then, create a new VariableBinding using that T.
    if(!pm->binding)pm->binding = new VariableBinding<T>(pm->name, v);
    else{
      fprintf(stderr, "Set Value\n");
      ((VariableBinding<T>*)(pm->binding))->setValue(v);
    }
    // debug (and bad code):

    if(sizeof(T) <= sizeof(int)){
      fprintf(stderr, "%s: populated at %p with %d\n", pm->flag, (pm->hestPtr), *(int*)(pm->hestPtr));      
    }
    else{
      fprintf(stderr, "%s: populated at %p with %d %d\n", pm->flag, (pm->hestPtr), *(int*)(pm->hestPtr), *((int*)(pm->hestPtr)+1));      
    }

    //end debug.
  }
public:
  // returns a pointer to a VariableBinding pointer.
  // This is because VariableBinding doesn't have a meaningful NULL/unitialized state.
  // The return value will, at the end of the function, point to a pointer to NULL.
  // eg:
  //   VariableBinding<...> **binding = buildParameter(...);
  //   assert(*binding == NULL);
  //   // here, the variable binding is still unitialized.
  //   loadParameters();
  //   // here, the binding is now set to something.
  //   *binding->setValue(...);


  // R is the type that Hest outputs. Ie. The data that hest
  // returns can be C-style (bitwise) cast to a type R.
  // This R is then more intelligently cast (using a constructor)
  // to an object of type T, which is returned.
  template <typename T, typename R=T>
  static VariableBinding<T>** buildParameter(char *flag, char* name, int minNumArgs, int maxNumArgs, char* argDefault, unsigned int* sawP, airEnum* enm, hestCB* callback, void* exposureObj, char* exposureName, char* info, int type = 0){

    if(!initialized){
      mop = airMopNew();
      hparm = hestParmNew();
      hparm->respFileEnable = AIR_TRUE;
      airMopAdd(mop, hparm, (airMopper)hestParmFree, airMopAlways);
      hparm->respFileEnable = AIR_TRUE;

      initialized = true;
    }

    if(type == 0)type = getAirType<T>();
    
    param* p = new param;
    fprintf(stderr,"Retrieving %s from %p\n", exposureName, exposureObj);
    *p = {(T*)malloc(sizeof(T)), VariableExposure<T>::getVariableBinding(exposureObj, exposureName),flag,name,type,minNumArgs,maxNumArgs,argDefault,info,createVariableBinding<T,R>};
    fprintf(stderr,"Build param object.\n");
    if(sawP){
      hestOptAdd(&hopt, p->flag, p->name, p->type, p->minargs, p->maxargs, p->hestPtr, p->argDefault,
           p->info, sawP);
    }else if(enm){
      hestOptAdd(&hopt, p->flag, p->name, p->type, p->minargs, p->maxargs, p->hestPtr, p->argDefault,
           p->info, NULL, enm);
    }else if(callback){
      hestOptAdd(&hopt, p->flag, p->name, p->type, p->minargs, p->maxargs, p->hestPtr, p->argDefault,
           p->info, NULL, NULL, callback);
    }else{
      hestOptAdd(&hopt, p->flag, p->name, p->type, p->minargs, p->maxargs, p->hestPtr, p->argDefault, p->info);
    }
    
    fprintf(stderr,"%s: allocated %d at %p\n", name, sizeof(T), p->hestPtr);

    parameters.push_back(p);
    return (VariableBinding<T>**)&(p->binding);
  }
  static void loadParameters(int argc, const char** argv){
    const char* me = argv[0];
    hestParseOrDie(hopt, argc-1, argv+1, hparm,
                 me, "demo program", AIR_TRUE, AIR_TRUE, AIR_TRUE);
    airMopAdd(HCI::mop, HCI::hopt, (airMopper)hestOptFree, airMopAlways);
    airMopAdd(HCI::mop, HCI::hopt, (airMopper)hestParseFree, airMopAlways);
    for(param* m : parameters){
      fprintf(stderr, "param itr: %p\n", m);
      m->createBindingFun(m);
    }
  }
};



#endif