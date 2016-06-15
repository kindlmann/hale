#include "GUI.h"



 
 

template <class T>
VariableBinding<T>::VariableBinding(const char* name, t_getter getter, t_setter setter){
    this->name = name;
    this->getter = getter;
    this->setter = setter;
    this->value  = 0;
    this->changed = false;
}
template <class T>
VariableBinding<T>::VariableBinding(const char* name, T val){
    this->name = name;
    this->getter = 0;
    this->setter = 0;
    this->value  = new T;
    *(this->value) = val;
    this->changed = false;
}
template <class T>
VariableBinding<T>::VariableBinding(const char* name, T* val_ptr){
    this->name = name;
    this->getter = 0;
    this->setter = 0;
    this->value  = val_ptr;
    this->changed = false;
}
template <class T>
void VariableBinding<T>::setValue(T in) {
    if(setter){
        setter(in);
    }else{
        *value = in;
    }
    this->changed = true;
}

template <class T>
void VariableBinding<T>::setValue(T* in) {
    if(setter){
        setter(*in);
    }else{
        *value = *in;
    }
    this->changed = true;
}
template <class T>
T VariableBinding<T>::getValue(){
    if(getter){
        return getter();
    }else{
        return *value;
    }
}
template <class T>
double VariableBinding<T>::getNumRep(){
    if(getter){
        return (double)(getter());
    }
    else{
        return (double)(*value);
    }
}

template <typename Type>
double getDoubleRep(Type in){
    return (double)in;
}


template<> double    getDoubleRep<double>(double in) { return in;        }
template<> double    getDoubleRep<int>   (int in)    { return double(in);   }
template<> double    getDoubleRep<float> (float in)  { return double(in); }
template<> double    getDoubleRep<short> (short in)  { return double(in); }
template<> double    getDoubleRep<char>  (char in)   { return double(in);  }
template<> double    getDoubleRep<long>  (long in)   { return double(in);  }


template <typename Type> const char* getStringRep(Type in){ return "[todo: string rep]";        }
template<> const char*   getStringRep<double>(double in) {  return "[todo: string rep<double>]";        }

// http://stackoverflow.com/questions/8752837/undefined-reference-to-template-class-constructor
template class VariableBinding<double>;
template class VariableBinding<bool>;

// CEGUI callbacks require a function pointer.
// this is the singular function which is called
// by all window events.

bool HaleGUI::windowEventHandler(const CEGUI::EventArgs& e){
    // figure out which window is being pointed to.
    CEGUI::WindowEventArgs* args =(CEGUI::WindowEventArgs*) (&e);
    CEGUI::Window* w = args->window;

    // search for the handlers to the matching window.
    std::vector<GenericGUIElement*>::iterator it;
    int i = 0;
    bool retval = false;
    for(it=inst->guiElements.begin() ; it < inst->guiElements.end(); it++,i++ ) {
        GenericGUIElement* curr = (*it);
        if(curr->getWindow()->getID() == w->getID()){
        // if(curr == w){
            // these are the same window.
            if(curr->handleEvent(e)){
                retval = true;
                ++eventHandledCount;
            }
            retval = retval || curr->handleEvent(e);
        }
    }
    return retval;
}

// code for each type of GUIElement...

bool GUIElement<CEGUI::Scrollbar, double>::handleEvent(const CEGUI::EventArgs& e){
  CEGUI::WindowEventArgs* args =(CEGUI::WindowEventArgs*) (&e);
  double val = window->getScrollPosition();
  binding->setValue(min + val*(max-min));
  printf("%s: %.2f",binding->name,binding->getValue());
  return true;
};

GUIElement<CEGUI::Scrollbar, double>::GUIElement(CEGUI::Scrollbar* window, VariableBinding<double>* bind, double max, double min){
    this->window = window;
    this->binding = bind;
    this->max = max;
    this->min = min;
    window->subscribeEvent(CEGUI::Scrollbar::EventScrollPositionChanged, HaleGUI::windowEventHandler);
}
void GUIElement<CEGUI::Scrollbar, double>::updateGUIFromBinding(){
    double val = binding->getValue();
    window->setScrollPosition((val-min)/(max-min));
}

const char* GUIElement<CEGUI::Scrollbar, double>::getWindowType(){
    return window->getType().c_str();
}
CEGUI::Window* GUIElement<CEGUI::Scrollbar, double>::getWindow(){
    return window;
}
bool GUIElement<CEGUI::Scrollbar, double>::hasChanged(){
    if(binding->changed){
        binding->changed = false;
        return true;
    }
    return binding->changed;
}

// Boolean toggle-button

bool GUIElement<CEGUI::ToggleButton, bool>::handleEvent(const CEGUI::EventArgs& e){
  CEGUI::WindowEventArgs* args =(CEGUI::WindowEventArgs*) (&e);
  bool val = window->isSelected();
  binding->setValue(val);
  printf("%s: %s\n",binding->name,binding->getValue()?"true":"false");
  return true;
};

GUIElement<CEGUI::ToggleButton, bool>::GUIElement(CEGUI::ToggleButton* window, VariableBinding<bool>* bind){
    this->window = window;
    this->binding = bind;
    window->subscribeEvent(CEGUI::ToggleButton::EventSelectStateChanged, HaleGUI::windowEventHandler);
}
void GUIElement<CEGUI::ToggleButton, bool>::updateGUIFromBinding(){
    bool val = binding->getValue();
    window->setSelected(val);
}
const char* GUIElement<CEGUI::ToggleButton, bool>::getWindowType(){
    return window->getType().c_str();
}
CEGUI::Window* GUIElement<CEGUI::ToggleButton, bool>::getWindow(){
    return window;
}
bool GUIElement<CEGUI::ToggleButton, bool>::hasChanged(){
    if(binding->changed){
        binding->changed = false;
        return true;
    }
    return binding->changed;
}

template class GUIElement<CEGUI::Scrollbar, double>;
template class GUIElement<CEGUI::ToggleButton, bool>;

// HaleGUI....



HaleGUI* HaleGUI::inst = 0;
int HaleGUI::eventHandledCount = 0;
// std::vector<GenericGUIElement*> guiElements;
HaleGUI::HaleGUI(){
    inst = 0;
}
HaleGUI::~HaleGUI(){

}
HaleGUI* HaleGUI::getInstance(){
    if(!HaleGUI::inst){
        HaleGUI::inst = new HaleGUI();
    }


    return HaleGUI::inst;
}
void HaleGUI::addGUIElement(GenericGUIElement* in){
    in->updateGUIFromBinding();
    guiElements.push_back(in);
}
void HaleGUI::renderAll(){
  using namespace CEGUI;
  if(cegui_renderer){
    // save state of program.
    int old_state_program;
    glGetIntegerv(GL_CURRENT_PROGRAM,&old_state_program);

    // render with CEGUI. Let CEGUI handle all of its own internals, without us interfering.
    glUseProgram(0);
    cegui_renderer->beginRendering();
    System::getSingleton().renderAllGUIContexts();
    cegui_renderer->endRendering();

    // reset state of program.
    glUseProgram(old_state_program);
    glEnable(GL_DEPTH_TEST);
  }
}
void HaleGUI::init(){
    using namespace CEGUI;

    // create renderer and enable extra states
    cegui_renderer = &(OpenGL3Renderer::create(Sizef(800.f, 600.f)));
    cegui_renderer->enableExtraStateSettings(true);

    // create CEGUI system object
    CEGUI::System::create(*cegui_renderer);

    // setup resource directories
    DefaultResourceProvider* rp = static_cast<DefaultResourceProvider*>(System::getSingleton().getResourceProvider());

    rp->setResourceGroupDirectory("schemes", "datafiles/schemes/");
    rp->setResourceGroupDirectory("imagesets", "datafiles/imagesets/");
    rp->setResourceGroupDirectory("fonts", "datafiles/fonts/");
    rp->setResourceGroupDirectory("layouts", "datafiles/layouts/");
    rp->setResourceGroupDirectory("looknfeels", "datafiles/looknfeel/");
    rp->setResourceGroupDirectory("lua_scripts", "datafiles/lua_scripts/");
    rp->setResourceGroupDirectory("schemas", "datafiles/xml_schemas/");

    // set default resource groups
    ImageManager::setImagesetDefaultResourceGroup("imagesets");
    Font::setDefaultResourceGroup("fonts");
    Scheme::setDefaultResourceGroup("schemes");
    WidgetLookManager::setDefaultResourceGroup("looknfeels");
    WindowManager::setDefaultResourceGroup("layouts");
    ScriptModule::setDefaultResourceGroup("lua_scripts");

    XMLParser* parser = System::getSingleton().getXMLParser();
    if (parser->isPropertyPresent("SchemaDefaultResourceGroup"))
        parser->setProperty("SchemaDefaultResourceGroup", "schemas");

    // load TaharezLook scheme and DejaVuSans-10 font
    SchemeManager::getSingleton().createFromFile("TaharezLook.scheme", "schemes");
    FontManager::getSingleton().createFromFile("DejaVuSans-10.font");

    // set default font and cursor image and tooltip type
    System::getSingleton().getDefaultGUIContext().setDefaultFont("DejaVuSans-10");
    // System::getSingleton().getDefaultGUIContext().getMouseCursor().setDefaultImage("TaharezLook/MouseArrow");
    System::getSingleton().getDefaultGUIContext().setDefaultTooltipType("TaharezLook/Tooltip");


    CEGUI::Window* root = WindowManager::getSingleton().loadLayoutFromFile("application_templates.layout");
    System::getSingleton().getDefaultGUIContext().setRootWindow(root);
    cegui_renderer = static_cast<CEGUI::OpenGL3Renderer*>(CEGUI::System::getSingleton().getRenderer());

    // Make the DefaultWindow transparent to mouse events.
    // If this is not done, then all mouse events will be captured by CEGUI.
    WindowManager::WindowIterator wit(WindowManager::getSingleton().getIterator());
    while (!wit.isAtEnd()){
        if( wit.getCurrentValue() ){
            uint id = wit.getCurrentValue()->getID();
//          printf("Type: %s: %d\n", wit.getCurrentValue()->getType().c_str(), wit.getCurrentValue()->getID());
            if(!strcmp("DefaultWindow",wit.getCurrentValue()->getType().c_str())){
                wit.getCurrentValue()->setMousePassThroughEnabled(true);
            }
            if(!strcmp("LeftPane",wit.getCurrentValue()->getName().c_str())){
                leftPane = wit.getCurrentValue();
            }
        }
        ++wit;
    }
}
CEGUI::Window* HaleGUI::getWithID(int id){
    CEGUI::WindowManager::WindowIterator wit(CEGUI::WindowManager::getSingleton().getIterator());
    while(!wit.isAtEnd()){
        if(wit.getCurrentValue()->getID() == id)
            return wit.getCurrentValue();
        ++wit;
    }
    return 0;
}
bool HaleGUI::hasChanged(){
    std::vector<GenericGUIElement*>::iterator it;
    for(it=guiElements.begin() ; it < guiElements.end(); it++) {

        if((*it)->hasChanged())return true;
    }
    return false;
}
// CEGUI input and callbacks
CEGUI::MouseButton cegui_toCEGUIButton(int button){
  switch (button){
  case GLFW_MOUSE_BUTTON_LEFT: 
    return CEGUI::LeftButton;

  case GLFW_MOUSE_BUTTON_MIDDLE:
    return CEGUI::MiddleButton;

  case GLFW_MOUSE_BUTTON_RIGHT:
    return CEGUI::RightButton;

  default:
    return CEGUI::MouseButtonCount;
  }
}
CEGUI::Key::Scan cegui_toCEGUIKey(int glfwKey){
  switch (glfwKey){
    case GLFW_KEY_ESCAPE: return CEGUI::Key::Escape;
    case GLFW_KEY_F1: return CEGUI::Key::F1;
    case GLFW_KEY_F2: return CEGUI::Key::F2;
    case GLFW_KEY_F3: return CEGUI::Key::F3;
    case GLFW_KEY_F4: return CEGUI::Key::F4;
    case GLFW_KEY_F5: return CEGUI::Key::F5;
    case GLFW_KEY_F6: return CEGUI::Key::F6;
    case GLFW_KEY_F7: return CEGUI::Key::F7;
    case GLFW_KEY_F8: return CEGUI::Key::F8;
    case GLFW_KEY_F9: return CEGUI::Key::F9;
    case GLFW_KEY_F10: return CEGUI::Key::F10;
    case GLFW_KEY_F11: return CEGUI::Key::F11;
    case GLFW_KEY_F12: return CEGUI::Key::F12;
    case GLFW_KEY_F13: return CEGUI::Key::F13;
    case GLFW_KEY_F14: return CEGUI::Key::F14;
    case GLFW_KEY_F15: return CEGUI::Key::F15;
    case GLFW_KEY_UP: return CEGUI::Key::ArrowUp;
    case GLFW_KEY_DOWN: return CEGUI::Key::ArrowDown;
    case GLFW_KEY_LEFT: return CEGUI::Key::ArrowLeft;
    case GLFW_KEY_RIGHT: return CEGUI::Key::ArrowRight;
    case GLFW_KEY_LEFT_SHIFT: return CEGUI::Key::LeftShift;
    case GLFW_KEY_RIGHT_SHIFT: return CEGUI::Key::RightShift;
    case GLFW_KEY_LEFT_CONTROL: return CEGUI::Key::LeftControl;
    case GLFW_KEY_RIGHT_CONTROL: return CEGUI::Key::RightControl;
    case GLFW_KEY_LEFT_ALT: return CEGUI::Key::LeftAlt;
    case GLFW_KEY_RIGHT_ALT: return CEGUI::Key::RightAlt;
    case GLFW_KEY_TAB: return CEGUI::Key::Tab;
    case GLFW_KEY_ENTER: return CEGUI::Key::Return;
    case GLFW_KEY_BACKSPACE: return CEGUI::Key::Backspace;
    case GLFW_KEY_INSERT: return CEGUI::Key::Insert;
    case GLFW_KEY_DELETE: return CEGUI::Key::Delete;
    case GLFW_KEY_PAGE_UP: return CEGUI::Key::PageUp;
    case GLFW_KEY_PAGE_DOWN: return CEGUI::Key::PageDown;
    case GLFW_KEY_HOME: return CEGUI::Key::Home;
    case GLFW_KEY_END: return CEGUI::Key::End;
    case GLFW_KEY_KP_ENTER: return CEGUI::Key::NumpadEnter;
    case GLFW_KEY_SPACE: return CEGUI::Key::Space;
    case 'A': return CEGUI::Key::A;
    case 'B': return CEGUI::Key::B;
    case 'C': return CEGUI::Key::C;
    case 'D': return CEGUI::Key::D;
    case 'E': return CEGUI::Key::E;
    case 'F': return CEGUI::Key::F;
    case 'G': return CEGUI::Key::G;
    case 'H': return CEGUI::Key::H;
    case 'I': return CEGUI::Key::I;
    case 'J': return CEGUI::Key::J;
    case 'K': return CEGUI::Key::K;
    case 'L': return CEGUI::Key::L;
    case 'M': return CEGUI::Key::M;
    case 'N': return CEGUI::Key::N;
    case 'O': return CEGUI::Key::O;
    case 'P': return CEGUI::Key::P;
    case 'Q': return CEGUI::Key::Q;
    case 'R': return CEGUI::Key::R;
    case 'S': return CEGUI::Key::S;
    case 'T': return CEGUI::Key::T;
    case 'U': return CEGUI::Key::U;
    case 'V': return CEGUI::Key::V;
    case 'W': return CEGUI::Key::W;
    case 'X': return CEGUI::Key::X;
    case 'Y': return CEGUI::Key::Y;
    case 'Z': return CEGUI::Key::Z;
    default: return CEGUI::Key::Unknown;
  }
}

bool HaleGUI::gui_charCallback(GLFWwindow* window, unsigned int char_pressed){
    return CEGUI::System::getSingleton().getDefaultGUIContext().injectChar(char_pressed);
}
bool HaleGUI::gui_cursorPosCallback(GLFWwindow* window, double x, double y){
    int old = eventHandledCount;
    bool r;
    r = CEGUI::System::getSingleton().getDefaultGUIContext().injectMousePosition(x, y);
    return r;
    // return (eventHandledCount > old);
}
bool HaleGUI::gui_keyCallback(GLFWwindow* window, int key, int scan, int action, int mod){
    CEGUI::Key::Scan cegui_key = cegui_toCEGUIKey(key);
    if (action == GLFW_PRESS){
        return CEGUI::System::getSingleton().getDefaultGUIContext().injectKeyDown(cegui_key);
    }
    else{
        return CEGUI::System::getSingleton().getDefaultGUIContext().injectKeyUp(cegui_key);
    }
}
bool HaleGUI::gui_mouseButtonCallback(GLFWwindow* window, int button, int state, int mod){
    int old = eventHandledCount;
    bool r;
    if (state == GLFW_PRESS){
        r= CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseButtonDown(cegui_toCEGUIButton(button));
    }
    else{
        r= CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseButtonUp(cegui_toCEGUIButton(button));
    }
    return r;
    // return (eventHandledCount > old);
}
bool HaleGUI::gui_mouseWheelCallback(GLFWwindow* window, double x, double y){
    if (y < 0.f)
        return CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseWheelChange(-1.f);
    else
        return CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseWheelChange(+1.f);
}
void HaleGUI::gui_windowResizedCallback(GLFWwindow* window, int width, int height){
    CEGUI::System::getSingleton().notifyDisplaySizeChanged(
        CEGUI::Sizef(static_cast<float>(width), static_cast<float>(height)));
    glViewport(0, 0, width, height);
}
void HaleGUI::gui_errorCallback(int error, const char* message){
    CEGUI::Logger::getSingleton().logEvent(message, CEGUI::Errors);
}


// public:
//   HaleGUI* getInstance();
//   // Set up this Window: register input handlers, etc.
//   void addGUIElement(GenericGUIElement* in);

//   // Clean up (but do not free memory for) this GUIElement: unregister input handlers, etc.
//   GenericGUIElement* removeGUIElement(GenericGUIElement* in);

//   // Remove and return a single GUIElement which is bound to the particular variable, or null.
//   GenericGUIElement* removeGUIElement(char* varname);
//   void renderAll();                      // Render all CEGUI elements.
//   void init();                           // Initialize CEGUI.

//   bool hasChanged(const char* name);     // returns whether the specified value has
//                                          // been changed in a manner visible to its
//                                          // VariableBinding.

//   // returns whether any of the variables have changed, ie. if a redraw is necessary.
//   bool hasChanged();


//   // a set of callback handlers which must be used manually by the calling code.
//   void gui_charCallback(GLFWwindow* window, unsigned int char_pressed);
//   void gui_cursorPosCallback(GLFWwindow* window, double x, double y);
//   void gui_keyCallback(GLFWwindow* window, int key, int scan, int action, int mod);
//   void gui_mouseButtonCallback(GLFWwindow* window, int button, int state, int mod);
//   void gui_mouseWheelCallback(GLFWwindow* window, double x, double y);
//   void gui_windowResizedCallback(GLFWwindow* window, int width, int height);
//   void gui_errorCallback(int error, const char* message);
// };

/*

template<>
class GUIElement<CEGUI::Scrollbar, double> : public GenericGUIElement{
protected:
    VariableBinding<double>* binding;
    CEGUI::Scrollbar* window;
public:
    GUIElement(CEGUI::Scrollbar* window, VariableBinding<double>* bind);

    void updateGUIFromBinding();
    void updateBindingFromGUI();
    CEGUI::Window* getWindow();
    const char* getWindowType();
};


*/




// template<double getNumRep(){

// }
// doublerep<



// template <class T>
// void VariableBinding<T>::setValue(T* in) {
//     if(setter){
//         setter(*in);
//     }else{
//         *value = *in;
//     }
//     this->changed = true;
// }
// template <class T>
// void VariableBinding<T>::setValueFromGUI() {
//     // TODO.
//     this->changed = true;
// }


// template<class T>
// void VariableBinding<T>::bindWindow(){
    
// }
// template<class T>
// bool VariableBinding<T>::valueChanged(){
//     if(this->changed){
//         this->changed = false;
//         return true;
//     }
//     return false;
// }1
