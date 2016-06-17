#ifndef HALEGUI_INCLUDED
#define HALEGUI_INCLUDED
#include <GL/glew.h>
#include "Hale.h"
#include <vector>
#include <CEGUI/CEGUI.h>




class HaleGUI;
template<class T>
class VariableBinding;
class GenericGUIElement;
template<class W, class T>
class GUIElement;


/*
 * A wrapper over a particular variable. Allows a consistent means
 * of getting/setting data from the GUI and otherwise.
 */

class GenericVariableBinding{
public:
    const char* name;
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
protected:
  T* value;         // pointer to real value, if a setter/getter are not used.
  t_getter getter;  // the function which gets the value (wherever it may be).
  t_setter setter;  // the function which sets the value.
  std::list<GenericGUIElement*> boundGUIElements;
public:
  VariableBinding(const char* name, T init_value);
  VariableBinding(const char* name, t_getter getter, t_setter setter);
  VariableBinding(const char* name, T* t_ptr);
  void updateBoundGUIElements();
  void bindGUIElement(GenericGUIElement* e);
  void setValue(T val);             // set the real value of this variable.
  void setValue(T* val);            // set the real value of this variable.
  T getValue();                     // get the real value of this variable.

  double getNumRep();               // get the numerical (double) representation
                                    // of what this variable is bound to. This
                                    // function may very well just be a wrapper
                                    // for (double) casting (which will fail if
                                    // T is of an incorrect type)
  char* getStringRep();             // get the string representation of what this
                                    // variable is bound to.
};


/*
 * This class is the supertype of all GUIElements.
 * It is not templated, so that it can be used as
 * a type for lists.
 */
class GenericGUIElement {
protected:
  CEGUI::Window* m_window;
  GenericVariableBinding* m_varbinding;
  GenericGUIElement(CEGUI::Window* window, GenericVariableBinding* binding);
public:
  virtual void updateGUIFromBinding() =0;   // these two functions will have to be
                                            // window type. 
  CEGUI::Window* getWindow();               // Return the window of this class

  const char* getWindowType();              // All CEGUI::Windows contain a member
                                            // type string. Just return that.
  virtual bool hasChanged();
  virtual bool handleEvent(const CEGUI::EventArgs& e) =0;

};

/********************************
 *
 * Template-Specializing VariableBinding would require a lot
 * of typing on the programmer's part (one specialization for
 * each of bool, double, char* and int). 
 *
 ********************************/

/*
 * A mapping between Variable Bindings and their corresponding GUI elements
 * (Ie. CEGUI::Windows). W should be a subclass of CEGUI::Window. T can be anything.
 */
template<class W, class T>
class GUIElement : public GenericGUIElement{  };


template<>
class GUIElement<CEGUI::Scrollbar, double> : public GenericGUIElement{
protected:
    VariableBinding<double>* binding;
    CEGUI::Scrollbar* window;
    double max,min,step;
public:
    GUIElement(CEGUI::Scrollbar* window, VariableBinding<double>* bind, double min, double max, double step);

    void updateGUIFromBinding();
    bool handleEvent(const CEGUI::EventArgs& e);
};


template<>
class GUIElement<CEGUI::ToggleButton, bool> : public GenericGUIElement{
protected:
    VariableBinding<bool>* binding;
    CEGUI::ToggleButton* window;
public:
    GUIElement(CEGUI::ToggleButton* window, VariableBinding<bool>* bind);

    void updateGUIFromBinding();
    bool handleEvent(const CEGUI::EventArgs& e);
};

template<>
class GUIElement<CEGUI::Editbox, double> : public GenericGUIElement{
protected:
    VariableBinding<double>* binding;
    CEGUI::Editbox* window;
public:
    GUIElement(CEGUI::Editbox* window, VariableBinding<double>* bind);

    void updateGUIFromBinding();
    bool handleEvent(const CEGUI::EventArgs& e);
};

template<>
class GUIElement<CEGUI::Combobox, int> : public GenericGUIElement{
protected:
    VariableBinding<int>* binding;
    CEGUI::Combobox* window;
public:
    GUIElement(CEGUI::Combobox* window, VariableBinding<int>* bind);

    void updateGUIFromBinding();
    bool handleEvent(const CEGUI::EventArgs& e);
};



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
  CEGUI::OpenGL3Renderer* cegui_renderer;
  CEGUI::Window* leftPane;
  CEGUI::VerticalLayoutContainer* leftPaneLayout;
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

  static bool windowEventHandler(const CEGUI::EventArgs& e);

  // functions for setting up/managing window layouts.

  // linear-time lookup of Window given ID.
  CEGUI::Window* getWithID(unsigned int id);

  // helper functions to create and lay out gui elements.
  CEGUI::Combobox* createComboboxFromEnum(CEGUI::Window* parent, const char* name, const char* values[], int numValues);
  CEGUI::Window* createWindow(const char* type, const char* name);
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