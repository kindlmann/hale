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
    updateBoundGUIElements();
    this->changed = true;
}

template <class T>
void VariableBinding<T>::setValue(T* in) {
    if(setter){
        setter(*in);
    }else{
        *value = *in;
    }
    updateBoundGUIElements();
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
template <class T>
void VariableBinding<T>::updateBoundGUIElements(){
    for (std::list<GenericGUIElement*>::const_iterator itr = boundGUIElements.begin(), end = boundGUIElements.end(); itr != end; ++itr) {
        (*itr)->updateGUIFromBinding();
    }
}
template <class T>
void VariableBinding<T>::bindGUIElement(GenericGUIElement* e){
    boundGUIElements.push_back(e);
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
template class VariableBinding<int>;
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

GenericGUIElement::GenericGUIElement(CEGUI::Window* window, GenericVariableBinding* binding){
    this->m_window = window;
    this->m_varbinding = binding;
    binding->bindGUIElement(this);
}
CEGUI::Window* GenericGUIElement::getWindow(){
    return m_window;
}
const char* GenericGUIElement::getWindowType(){
    return m_window->getType().c_str();
}
bool GenericGUIElement::hasChanged(){
    if(m_varbinding->changed){
        m_varbinding->changed = false;
        return true;
    }
    return m_varbinding->changed;
}

// code for each type of GUIElement...

// Double scrollbar

bool GUIElement<CEGUI::Scrollbar, double>::handleEvent(const CEGUI::EventArgs& e){
  double val = window->getScrollPosition();
  val = min + val*(max-min);

  // enforce step size
  if(step!=0){
    int div = (int)(0.5 + val / step);
    val = div*step;
  }
  binding->setValue(val);
  fprintf(stderr,"%s: %.2f",binding->name,binding->getValue());
  return true;
};

GUIElement<CEGUI::Scrollbar, double>::GUIElement(CEGUI::Scrollbar* window, VariableBinding<double>* bind, double min, double max, double step) : GenericGUIElement(window, bind){
    this->window = window;
    this->binding = bind;
    this->max = max;
    this->min = min;
    this->step =step;
    window->subscribeEvent(CEGUI::Scrollbar::EventScrollPositionChanged, HaleGUI::windowEventHandler);
}
void GUIElement<CEGUI::Scrollbar, double>::updateGUIFromBinding(){
    double val = binding->getValue();
    window->setScrollPosition((val-min)/(max-min));
}

// Double editbox

bool GUIElement<CEGUI::Editbox, double>::handleEvent(const CEGUI::EventArgs& e){
  // CEGUI::WindowEventArgs* args =(CEGUI::WindowEventArgs*) (&e);
  double val = atof(window->getText().c_str());
  binding->setValue(val);
  printf("%s: %.2f",binding->name,binding->getValue());
  return true;
};

GUIElement<CEGUI::Editbox, double>::GUIElement(CEGUI::Editbox* window, VariableBinding<double>* bind) : GenericGUIElement(window, bind){
    this->window = window;
    this->binding = bind;
    window->subscribeEvent(CEGUI::Editbox::EventTextAccepted
, HaleGUI::windowEventHandler);
}
void GUIElement<CEGUI::Editbox, double>::updateGUIFromBinding(){
    double val = binding->getValue();
    window->setText(std::to_string(val).c_str());
}


// Boolean toggle-button

bool GUIElement<CEGUI::ToggleButton, bool>::handleEvent(const CEGUI::EventArgs& e){
  // CEGUI::WindowEventArgs* args =(CEGUI::WindowEventArgs*) (&e);
  bool val = window->isSelected();
  binding->setValue(val);
  printf("%s: %s\n",binding->name,binding->getValue()?"true":"false");
  return true;
};

GUIElement<CEGUI::ToggleButton, bool>::GUIElement(CEGUI::ToggleButton* window, VariableBinding<bool>* bind) : GenericGUIElement(window, bind){
    this->window = window;
    this->binding = bind;
    window->subscribeEvent(CEGUI::ToggleButton::EventSelectStateChanged, HaleGUI::windowEventHandler);
}
void GUIElement<CEGUI::ToggleButton, bool>::updateGUIFromBinding(){
    bool val = binding->getValue();
    window->setSelected(val);
}

// Integer combobox

bool GUIElement<CEGUI::Combobox, int>::handleEvent(const CEGUI::EventArgs& e){
  // CEGUI::WindowEventArgs* args =(CEGUI::WindowEventArgs*) (&e);
  int val = window->getItemIndex(window->getSelectedItem());
  binding->setValue(val);
  printf("%s: %d\n",binding->name,binding->getValue());
  return true;
};

GUIElement<CEGUI::Combobox, int>::GUIElement(CEGUI::Combobox* window, VariableBinding<int>* bind) : GenericGUIElement(window, bind){
    this->window = window;
    this->binding = bind;
    window->subscribeEvent(CEGUI::Combobox::EventListSelectionAccepted
, HaleGUI::windowEventHandler);
}
void GUIElement<CEGUI::Combobox, int>::updateGUIFromBinding(){
    int val = binding->getValue();
    if(window->getSelectedItem() != 0){
        window->setItemSelectState(window->getSelectedItem(),false);
    }
    window->setItemSelectState(val,true);
}

template class GUIElement<CEGUI::Editbox, double>;
template class GUIElement<CEGUI::Scrollbar, double>;
template class GUIElement<CEGUI::ToggleButton, bool>;
template class GUIElement<CEGUI::Combobox, int>;

// Helper function, create a combobox:

CEGUI::Combobox* HaleGUI::createComboboxFromEnum(CEGUI::Window* parent, const char* name, const char* values[], int numValues){
    using namespace CEGUI;
    Combobox* cbox = (Combobox*)parent->createChild( Combobox::WidgetTypeName, name);

    Editbox* edbox = (Editbox*) cbox->createChild("TaharezLook/Editbox", Combobox::EditboxName);
    ComboDropList* dlist = (ComboDropList*) cbox->createChild("TaharezLook/ComboDropList", Combobox::DropListName);
    PushButton* button = (PushButton*) cbox->createChild("TaharezLook/Button", Combobox::ButtonName);
    for(int i=0;i<numValues;++i){
        cbox->addItem(new ListboxTextItem(values[i],i));
    }

    edbox->setPosition(UVector2(UDim(0,0),UDim(0,0)));
    edbox->setSize(USize(UDim(0.8,0),UDim(0.3,0)));
    dlist->setPosition(UVector2(UDim(0,0),UDim(0.3f,0)));
    dlist->setSize(USize(UDim(1.0,0),UDim(0.7,0)));
    button->setPosition(UVector2(UDim(0.8,0),UDim(0,0)));
    button->setSize(USize(UDim(0.2f,0),UDim (0.3f,0)));

    cbox->initialiseComponents();


    return cbox;
}



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
void layoutHoriz(CEGUI::HorizontalLayoutContainer* container);
void layoutVert(CEGUI::VerticalLayoutContainer* container);
void layoutHoriz(CEGUI::HorizontalLayoutContainer* container){
    // preserve height.
    CEGUI::UDim oldHeight = container->getHeight();
    size_t index = 0;
    while (index < container->getChildCount()){
       CEGUI::Window* child = container->getChildAtIdx(index);
       // child->setHeight(CEGUI::UDim(0.95,0));
       child->setMargin(CEGUI::UBox(CEGUI::UDim(0.0075,0),CEGUI::UDim(0.05,0),CEGUI::UDim(0.0075,0),CEGUI::UDim(0.05,0)));
       ++index;
    }
    container->layout(); 
    container->setHeight(oldHeight);
}
void layoutVert(CEGUI::VerticalLayoutContainer* container){

    size_t index = 0;
    while (index < container->getChildCount()){
        CEGUI::Window* child = container->getChildAtIdx(index);
        if(!strcmp(child->getType().c_str(),"HorizontalLayoutContainer")){
            // printf("layout: %s\n",child->getType().c_str());
            // child->setWidth(CEGUI::UDim(0.95,0));
            // child->setMargin(CEGUI::UBox(CEGUI::UDim(0.0075,0),CEGUI::UDim(0.025,0),CEGUI::UDim(0.0075,0),CEGUI::UDim(0.025,0)));
            layoutHoriz((CEGUI::HorizontalLayoutContainer*)child);
        }
        ++index;
    }
    index = 0;
    while (index < container->getChildCount()){
        CEGUI::Window* child = container->getChildAtIdx(index);
        child->setWidth(CEGUI::UDim(0.95,0));
        child->setMargin(CEGUI::UBox(CEGUI::UDim(0.0075,0),CEGUI::UDim(0.025,0),CEGUI::UDim(0.0075,0),CEGUI::UDim(0.025,0)));
        ++index;
    }
    container->layout();

   // "HorizontalLayoutContainer"
}
void HaleGUI::layout(){
    layoutVert(leftPaneLayout);
}
CEGUI::Window* HaleGUI::createWindow(const char* type, const char* name){
    return leftPaneLayout->createChild(type,name);
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
            // uint id = wit.getCurrentValue()->getID();
//          printf("Type: %s: %d\n", wit.getCurrentValue()->getType().c_str(), wit.getCurrentValue()->getID());
            if(!strcmp("DefaultWindow",wit.getCurrentValue()->getType().c_str())){
                wit.getCurrentValue()->setMousePassThroughEnabled(true);
            }
            if(!strcmp("LeftPane",wit.getCurrentValue()->getName().c_str())){
                leftPane = wit.getCurrentValue();
                leftPaneLayout = (CEGUI::VerticalLayoutContainer*)(leftPane->createChild("VerticalLayoutContainer", "leftPaneLayout"));
            }
        }
        ++wit;
    }
}
CEGUI::Window* HaleGUI::getWithID(unsigned int id){
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
    inst->leftPaneLayout->layout();
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



