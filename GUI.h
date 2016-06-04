#include <GL/glew.h>
#include "Hale.h"
#include "privateHale.h"
#include <vector>
#include <CEGUI/CEGUI.h>




/*

    GUIHandlers may differ in three ways:
        1. type of value.
        2. GUI-mode of interaction.
        3. Manner of update.

*/

/*
 * Can display:
 *   - enum, int, double, string
 *
 */

// T is type of variable.
// W is type of window.


class HaleGUI;
template<class T>
class VariableBinding;




template<class T>
class VariableBinding{
public:
    typedef T (*t_getter)();
    typedef void (*t_setter)(T);
protected:
    T* value;         // pointer to real value, if a setter/getter are not used.
    t_getter getter;  // the method which gets the value (wherever it may be).
    t_setter setter;  // the method which sets the value.
    bool changed;
public:
    VariableBinding(const char* name, T init_value);
    VariableBinding(const char* name, t_getter getter, t_setter setter);
    VariableBinding(const char* name, T* t_ptr);
    void setValue(T val);             // function pointer to set the real value of this variable.
    T getValue();                     // function pointer to get the real value of this variable.
    void setValue(T* val);            // function pointer to set the real value of this variable.
    double getNumRep();               // get the numerical (double) representation of what this variable is bound to.
                                      // this function might just be a wrapper for (double) casting (which will fail
                                      // if T is of an incorrect type)
    char* getStringRep();             // get the string representation of what this variable is bound to.
};


/*
 * This class is the supertype of all GUIElements.
 * It is not a template, so that it can be used as
 * a type for lists.
 */
class GenericGUIElement {
    virtual void updateGUIFromBinding() =0;    // these two functions will have to be specialized for each supported
    virtual void updateBindingFromGUI() =0;    //   window type. 
    virtual CEGUI::Window* getWindow()  =0;    // Return the window of this class
    virtual const char* getWindowType() =0;    // All CEGUI::Windows contain a member type string. Just return that.
};

/*
 * A mapping between Variable Bindings and their corresponding GUI elements (Ie. CEGUI::Windows).
 * W should be a subclass of CEGUI::Window. T can be anything.
 */
template<class W, class T>
class GUIElement : public GenericGUIElement{
protected:
    VariableBinding<T>* binding;
    W* window;
public:
    GUIElement(W* window, VariableBinding<T>* bind);
};

/* 
 * Singleton class. Handle GUI stuff. There isn't really a reason why we
 * would need more than one to exist; but this pattern allows us to
 * easily edit the code to create more should the need arise.
 * 
 */

class HaleGUI{ 
public:
    static HaleGUI* inst;
    HaleGUI();
protected:
    std::vector<GenericGUIElement> guiElements;
public:

    ~HaleGUI();
    HaleGUI* getInstance();
    void addGUIElement(GenericGUIElement* in);         // Set up this Window: register input handlers, etc.
    GenericGUIElement* removeGUIElement(GenericGUIElement* in);      // Clean up (but do not free memory for) this window: unregister input handlers, etc.
    void removeGUIElement(char* varname);              // 
    void renderAll();                                  // Render all CEGUI elements.
    void init();                                       // Initialize CEGUI.

    // returns whether the specified value has been changed in a manner visible to its VariableBinding.
    bool hasChanged(const char* name);

    // returns whether any of the variables have changed.
    bool hasChanged();


    // a set of callback handlers which must be included manually.
    void gui_charCallback(GLFWwindow* window, unsigned int char_pressed);
    void gui_cursorPosCallback(GLFWwindow* window, double x, double y);
    void gui_keyCallback(GLFWwindow* window, int key, int scan, int action, int mod);
    void gui_mouseButtonCallback(GLFWwindow* window, int button, int state, int mod);
    void gui_mouseWheelCallback(GLFWwindow* window, double x, double y);
    void gui_windowResizedCallback(GLFWwindow* window, int width, int height);
    void gui_errorCallback(int error, const char* message);

};

/*
Example Usage:


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

int main(){

    // Iso value:
    VariableBinding<double> isoval(getIsoVal, setIsoVal);
    // VariableBinding<double> isoval(&isoval);             // alternatively.
    CEGUI::Scrollbar isoScrollbar = new CEGUI::Scrollbar(...);
    GUIElement<CEGUI::Scrollbar, double> isoElm(isoval, isoScrollbar);
    HaleGUI::getInstance()->addGUIElement(isoElm);

    // Verbose flag:
    VariableBinding<bool> verboseFlag(getVerbose, setVerbose);
    CEGUI::Checkbox* verboseBox = new CEGUI::Checkbox( ... );
    GUIElement<CEGUI::Checkbox, bool> verbElm(verboseFlag, verboseBox);
    HaleGUI::getInstance()->addGUIElement(verbElm);

     .. etc ..
}


*/
