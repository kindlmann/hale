#ifndef HALEGUI_INCLUDED
#define HALEGUI_INCLUDED

#include <nanogui/screen.h>
#include <nanogui/textbox.h>
#include <nanogui/checkbox.h>
#include <nanogui/colorpicker.h>
#include <nanogui/colorwheel.h>
#include <nanogui/combobox.h>
#include <nanogui/slider.h>
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
  void updateBoundWidgets(){
    for (std::list<GenericBoundWidget*>::const_iterator itr = boundWidgets.begin(), end = boundWidgets.end(); itr != end; ++itr) {
        (*itr)->updateFromBinding();
    }
  }
  void bindWidget(GenericBoundWidget* e){
    boundWidgets.push_back(e);
    e->updateFromBinding();
  }
  // set the real value of this variable.
  void setValue(T in){
    setValue(&in);
  }
  // set value, pass-by-reference
  void setValue(T* in){
    if(setter){
        setter(*in);
    }else if(value){
        *value = *in;
    }else{
        fSetter(*in);
    }
    updateBoundWidgets();
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

template <typename T, typename S, typename sfinae = std::true_type> class BoundWidget { };

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
        return true;
    });
  }
  void updateFromBinding(){
    // update in such a way that the callback does NOT get called.
    mPushed = mBinding->getValue();
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


template <> class BoundWidget<int, nanogui::ComboBox, std::true_type> : public nanogui::ComboBox, public GenericBoundWidget {
protected:
  VariableBinding<int>* mBinding;
public:
  BoundWidget(nanogui::Widget *p, VariableBinding<int>* binding, std::vector<std::string> names) : nanogui::ComboBox(p), mBinding(binding){
    binding->bindWidget(this);
    fprintf(stderr,"\ncreated bound widget.\n");
    auto self = this;
    setItems(names,names);
    nanogui::ComboBox::setCallback([binding, self](const int ind) {
        binding->setValue(ind);
        return true;
    });
  }
  void updateFromBinding(){
    // update in such a way that the callback does NOT get called.
    nanogui::ComboBox::setSelectedIndex(mBinding->getValue());
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



#endif