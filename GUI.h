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
// #include "FreeSpinner.h"



class HaleGUI;
template<class T>
class VariableBinding;
class GenericGUIElement;
template<class W, class T>
class GUIElement;

/* VariableWrapper provides a more OO method of wrapping
 * a container around a variable. This method does not
 * have to do with function pointers, and so is slightly
 * more versatile.
 */
template<class T>
class VariableWrapper{
public:
  VariableWrapper(T v);
  virtual T get() =0;
  virtual void set(T) =0;
};

/*
 * A wrapper over a particular variable. Allows a consistent means
 * of getting/setting data from the GUI and otherwise.
 */

class GenericBoundWidget {
public:
  virtual void updateFromBinding() = 0;  
};

template <typename T, typename S, typename sfinae = std::true_type> class BoundWidget { };

// We do this for each variable/widget pair we need.

template <> class BoundWidget<std::string, nanogui::TextBox, std::true_type> : public nanogui::TextBox, public GenericBoundWidget {
public:
  BoundWidget(nanogui::Widget *p, VariableBinding<std::string>* binding);
  void updateFromBinding();
  VariableBinding<std::string>* mBinding;
};

template <> class BoundWidget<bool, nanogui::CheckBox, std::true_type> : public nanogui::CheckBox, public GenericBoundWidget {
public:
  BoundWidget(nanogui::Widget *p, VariableBinding<bool>* binding);
  void updateFromBinding();
  VariableBinding<bool>* mBinding;
};

template <> class BoundWidget<nanogui::Color, nanogui::ColorPicker, std::true_type> : public nanogui::ColorPicker, public GenericBoundWidget {
public:
  BoundWidget(nanogui::Widget *p, VariableBinding<nanogui::Color>* binding);
  void updateFromBinding();
  VariableBinding<nanogui::Color>* mBinding;
};

template <typename T> class BoundWidget<T, nanogui::IntBox<T>, typename std::is_integral<T>::type> : public nanogui::IntBox<T>, public GenericBoundWidget {
public:
  BoundWidget(nanogui::Widget *p, VariableBinding<T>* binding);
  void updateFromBinding();
  VariableBinding<T>* mBinding;
};

template <typename T> class BoundWidget<T, nanogui::FloatBox<T>, typename std::is_floating_point<T>::type> : public nanogui::FloatBox<T>, public GenericBoundWidget {
public:
  BoundWidget(nanogui::Widget *p, VariableBinding<T>* binding);
  void updateFromBinding();
  VariableBinding<T>* mBinding;
};

template <> class BoundWidget<int, nanogui::ComboBox, std::true_type> : public nanogui::ComboBox, public GenericBoundWidget {
public:
  BoundWidget(nanogui::Widget *p, VariableBinding<int>* binding, std::vector<std::string> names);
  void updateFromBinding();
  VariableBinding<int>* mBinding;
};

template <typename T> class BoundWidget<T, nanogui::Slider, typename std::is_arithmetic<T>::type> : public nanogui::Slider, public GenericBoundWidget {
protected:
  T mMin;
  T mMax;
public:
  BoundWidget(nanogui::Widget *p, VariableBinding<T>* binding);
  void updateFromBinding();
  void setRange(T vmin, T vmax);
  VariableBinding<T>* mBinding;
};


class GenericVariableBinding{
public:
    GenericVariableBinding(const char* name);
    virtual ~GenericVariableBinding() =0;
    const char* const name;
    bool changed;
    virtual void updateBoundGUIElements() =0;
    virtual void bindGUIElement(GenericGUIElement* e) =0;
    // double getNumRep();
    // char* getStringRep();
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
//VariableWrapper* varWrapper;  // pointer to variable wrapper if neither value nor
                                // getter/setter are used.
  std::list<GenericGUIElement*> boundGUIElements;
  std::list<GenericBoundWidget*> boundWidgets;
  bool changed;
public:
  VariableBinding(const char* name, T init_value);
  VariableBinding(const char* name, t_getter get, t_setter set);
  VariableBinding(const char* name, std::function<T(void)> get, std::function<void(T)> set);
  VariableBinding(const char* name, T* t_ptr);
  virtual ~VariableBinding();
//VariableBinding(const char* name, VariableWrapper<T>* wrapper);
  void updateBoundGUIElements();
  void updateBoundWidgets();
  void bindGUIElement(GenericGUIElement* e);
  void bindWidget(GenericBoundWidget* e);
  void setValue(T val);             // set the real value of this variable.
  void setValue(T* val);            // set value, pass-by-reference
  T getValue();                     // get the real value of this variable.
  bool hasChanged();                // returns whether this value has been set since the
                                    // last call to this function.
};



/*
 * This class is the supertype of all GUIElements.
 * It is not templated, so that it itself be used as
 * a generic 1template type.
 */
class GenericGUIElement {
protected:
  // CEGUI::Window* const m_window;
  GenericVariableBinding* const m_varbinding;
  // GenericGUIElement(CEGUI::Window* window, GenericVariableBinding* binding);
public:
  virtual ~GenericGUIElement();
  // CEGUI::Window* getWindow();               // Return the window of this element

  const char* getWindowType();              // All CEGUI::Windows contain a member
                                            // type string. Just return that.
  bool hasChanged();
  const char* getVarName();

  virtual void updateGUIFromBinding() =0;
  // virtual bool handleEvent(const CEGUI::EventArgs& e) =0;

};

/********************************
 *
 * Template-Specializing VariableBinding would require a lot
 * of typing on the programmer's part. So we pass the
 * template-specialization on to the GUIElement class.
 *
 ********************************/

/*
 * A mapping between Variable Bindings and their corresponding GUI elements
 * (Ie. CEGUI::Windows). W should be a subclass of CEGUI::Window. T can be anything.
 */


// template<class W, class T>
// class GUIElement : public GenericGUIElement{  };


// template<>
// class GUIElement<CEGUI::Scrollbar, double> : public GenericGUIElement{
// protected:
//     VariableBinding<double>* binding;
//     CEGUI::Scrollbar* window;
//     double max,min,step;
// public:
//     GUIElement(CEGUI::Scrollbar* window, VariableBinding<double>* bind, double min, double max, double step);

//     void updateGUIFromBinding();
//     bool handleEvent(const CEGUI::EventArgs& e);
// };




/* 
 * Singleton class. Handle GUI stuff. Because of the way CEGUI
 * handles events, there can only ever be one HaleGUI instance.
 * The singleton pattern allows for easy inheritance.
 * Note: event-handling functions must be called from the same
 * thread.
 * 
 * 
 */

class HaleGUI{ 
public:
  static HaleGUI* inst;
protected:
  std::vector<GenericGUIElement*> guiElements;
  HaleGUI();
  ~HaleGUI();
private:
  static int eventHandledCount;
public:
  static HaleGUI* getInstance();
  // Set up this Window: register input handlers, etc.
  void addGUIElement(GenericGUIElement* in);

  // Clean up (but do not free memory for) this GUIElement: unregister input handlers, etc.
  GenericGUIElement* removeGUIElement(GenericGUIElement* in);

  // Remove and return a single GUIElement which is bound to the particular variable, or null.
  GenericGUIElement* removeGUIElement(char* varname);
  void renderAll();                      // Render all CEGUI elements.
  void init();                           // Initialize CEGUI.

  bool hasChanged(const char* name);     // returns whether the specified value has
                                         // been changed in a manner visible to its
                                         // VariableBinding.

  // returns whether any of the variables have changed, ie. if a redraw is necessary.
  bool hasChanged();
  void forceGUIUpdate();                 // update state of all gui elements from variable binding.
  void forceGUIUpdate(const char* name); //   - for a particular variable

  // static bool windowEventHandler(const CEGUI::EventArgs& e);

  // functions for setting up/managing window layouts.

  void layout();


  // a set of callback handlers which must be used manually by the calling code.
  static bool gui_charCallback(GLFWwindow* window, unsigned int char_pressed);
  static bool gui_cursorPosCallback(GLFWwindow* window, double x, double y);
  static bool gui_keyCallback(GLFWwindow* window, int key, int scan, int action, int mod);
  static bool gui_mouseButtonCallback(GLFWwindow* window, int button, int state, int mod);
  static bool gui_mouseWheelCallback(GLFWwindow* window, double x, double y);
  static void gui_windowResizedCallback(GLFWwindow* window, int width, int height);
  static void gui_errorCallback(int error, const char* message);
};
/////////////////////////////////////////////////////////////////
////
///                     Example Usage:
///
//
/*


double getIsoVal(){
    return isoval;
}
void setIsoVal(double in){
    isoval = in;
}

void setVerbose(bool in){
    seekVerboseSet( ... );
}
bool getVerbose(){
    return ... ;
}
void setEnum(char* in){
    if(!strcmp(in,enum_strings[0]))enum_val = 1;
    else if ...
}
char* getEnum(){
    return enum_strings[enum_val];
}

int main(){
  // iso slider.
  CEGUI::Scrollbar* isoSlider = (CEGUI::Scrollbar*) halegui->getWithID(15);
  isoval = new VariableBinding<double>("ISO", init_isoval);
  GUIElement<CEGUI::Scrollbar, double>* isoElm;
  isoElm = new GUIElement<CEGUI::Scrollbar, double>( isoSlider, isoval, isomax, isomin);

  // toy happiness slider.
  GUIElement<CEGUI::Scrollbar, double>* happiness;
  happiness = new GUIElement<CEGUI::Scrollbar, double>((CEGUI::Scrollbar*) halegui->getWithID(19), new VariableBinding<double>("Happy!",15), 100, 0);
  halegui->addGUIElement(happiness);

  //verbose checkbox
  CEGUI::ToggleButton* checkBox;
  checkBox = (CEGUI::ToggleButton*) halegui->getWithID(23);
  checkBox->setText("Verbose");
  halegui->addGUIElement(new GUIElement<CEGUI::ToggleButton,bool>(checkBox, new VariableBinding<bool>("Verbose", getVerbose, setVerbose)));

     .. etc ..
}


*/


#endif