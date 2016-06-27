/***********************************************************************
    created:	3/2/2005
    author:		Paul D Turner
*************************************************************************/
/***************************************************************************
 *   Copyright (C) 2004 - 2006 Paul D Turner & The CEGUI Development Team
 *
 *   Permission is hereby granted, free of charge, to any person obtaining
 *   a copy of this software and associated documentation files (the
 *   "Software"), to deal in the Software without restriction, including
 *   without limitation the rights to use, copy, modify, merge, publish,
 *   distribute, sublicense, and/or sell copies of the Software, and to
 *   permit persons to whom the Software is furnished to do so, subject to
 *   the following conditions:
 *
 *   The above copyright notice and this permission notice shall be
 *   included in all copies or substantial portions of the Software.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *   IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 *   OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 *   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 *   OTHER DEALINGS IN THE SOFTWARE.
 ***************************************************************************/
#ifndef _CEGUIFreepinner_h_
#define _CEGUIFreeSpinner_h_

#include "CEGUI/Base.h"
#include "CEGUI/Window.h"

#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable : 4251)
#endif


// Start of CEGUI namespace section
namespace CEGUI
{
    /*!
    \brief
        Base class for the FreeSpinner widget.

        The FreeSpinner widget has a text area where numbers may be entered
        and two buttons which may be used to increase or decrease the
        value in the text area by a user specified amount.
    */
    class CEGUIEXPORT FreeSpinner : public Window
    {
    public:
        /*!
        \brief
            Enumerated type specifying possible input and/or display modes for the FreeSpinner.
        */
        enum TextInputMode
        {
            FloatingPoint,  //!< Floating point decimal.
            Integer,        //!< Integer decimal.
            Hexadecimal,    //!< Hexadecimal.
            Octal           //!< Octal
        };

        /*************************************************************************
            Events system constants
        *************************************************************************/
        static const String WidgetTypeName;                 //!< Window factory name
        static const String EventNamespace;                 //!< Namespace for global events
        /** Event fired when the FreeSpinner current value changes.
         * Handlers are passed a const WindowEventArgs reference with
         * WindowEventArgs::window set to the FreeSpinner whose current value has
         * changed.
         */
        static const String EventValueChanged;
        /** Event fired when the FreeSpinner step value is changed.
         * Handlers area passed a const WindowEventArgs reference with
         * WindowEventArgs::window set to the FreeSpinner whose step value has
         * changed.
         */
        static const String EventStepChanged;
        /** Event fired when the maximum FreeSpinner value is changed.
         * Handlers are passed a const WindowEventArgs reference with
         * WindowEventArgs::window set to the FreeSpinner whose maximum value has
         * been changed.
         */
        static const String EventMaximumValueChanged;
        /** Event fired when the minimum FreeSpinner value is changed.
         * Handlers are passed a const WindowEventArgs reference with
         * WindowEventArgs::windows set to the FreeSpinner whose minimum value has
         * been changed.
         */
        static const String EventMinimumValueChanged;
        /** Event fired when the FreeSpinner text input & display mode is changed.
         * Handlers are passed a const WindowEventArgs reference with
         * WindowEventArgs::window set to the FreeSpinner whose text mode has been
         * changed.
         */
        static const String EventTextInputModeChanged;

        /*************************************************************************
            Component widget name strings
        *************************************************************************/
        static const String EditboxName;          //!< Widget name for the editbox thumb component.
        static const String IncreaseButtonName;   //!< Widget name for the increase button component.
        static const String DecreaseButtonName;   //!< Widget name for the decrease button component.

        /*************************************************************************
            Object Construction and Destruction
        *************************************************************************/
        /*!
        \brief
            Constructor for FreeSpinner objects
        */
        FreeSpinner(const String& type, const String& name);

        /*!
        \brief
            Destructor for FreeSpinner objects
        */
        virtual ~FreeSpinner(void);

	    /*!
	    \brief
		    Initialises the Window based object ready for use.

	    \note
		    This must be called for every window created.  Normally this is handled automatically by the WindowFactory for each Window type.

	    \return
		    Nothing
	    */
        void initialiseComponents(void);


        /*************************************************************************
        	Accessors
        *************************************************************************/
        /*!
        \brief
            Return the current FreeSpinner value.

        \return
            current value of the FreeSpinner.
        */
        double getCurrentValue(void) const;

        /*!
        \brief
            Return the current step value.

        \return
            Step value.  This is the value added to the FreeSpinner vaue when the
            up / down buttons are clicked.
        */
        double getStepSize(void) const;

        /*!
        \brief
            Return the current maximum limit value for the FreeSpinner.

        \return
            Maximum value that is allowed for the FreeSpinner.
        */
        double getMaximumValue(void) const;

        /*!
        \brief
            Return the current minimum limit value for the FreeSpinner.

        \return
            Minimum value that is allowed for the FreeSpinner.
        */
        double getMinimumValue(void) const;

        /*!
        \brief
            Return the current text input / display mode setting.

        \return
            One of the TextInputMode enumerated values indicating the current
            text input and display mode.
        */
        TextInputMode getTextInputMode(void) const;

        /*************************************************************************
        	Manipulators
        *************************************************************************/
        /*!
        \brief
            Set the current FreeSpinner value.

        \param value
            value to be assigned to the FreeSpinner.

        \return
            Nothing.
        */
        void setCurrentValue(double value);

        /*!
        \brief
            Set the current step value.

        \param step
            The value added to be the FreeSpinner value when the
            up / down buttons are clicked.

        \return
            Nothing.
        */
        void setStepSize(double step);

        /*!
        \brief
            Set the FreeSpinner maximum value.

        \param maxValue
            The maximum value to be allowed by the FreeSpinner.

        \return
            Nothing.
        */
        void setMaximumValue(double maxValue);

        /*!
        \brief
            Set the FreeSpinner minimum value.

        \param minVaue
            The minimum value to be allowed by the FreeSpinner.

        \return
            Nothing.
        */
        void setMinimumValue(double minVaue);

        /*!
        \brief
            Set the FreeSpinner input / display mode.

        \param mode
            One of the TextInputMode enumerated values indicating the text
            input / display mode to be used by the FreeSpinner.

        \return
            Nothing.
        */
        void setTextInputMode(TextInputMode mode);

    protected:
        /*************************************************************************
        	Constants
        *************************************************************************/
        static const String FloatValidator;      //!< Validator regex used for floating point mode.
        static const String IntegerValidator;    //!< Validator regex used for decimal integer mode.
        static const String HexValidator;        //!< Validator regex used for hexadecimal mode.
        static const String OctalValidator;      //!< Validator regex used for octal mode.

        /*************************************************************************
        	Protected Implementation Methods
        *************************************************************************/
        /*!
        \brief
            Returns the numerical representation of the current editbox text.

        \return
            double value that is the numerical equivalent of the editbox text.

        \exception InvalidRequestException  thrown if the text can not be converted.
        */
        virtual double getValueFromText(void) const;

        /*!
        \brief
            Returns the textual representation of the current FreeSpinner value.

        \return
            String object that is equivalent to the the numerical value of the FreeSpinner.
        */
        virtual String getTextFromValue(void) const;

        /*!
        \brief
            Return a pointer to the 'increase' PushButtoncomponent widget for
            this FreeSpinner.

        \return
            Pointer to a PushButton object.

        \exception UnknownObjectException
            Thrown if the increase PushButton component does not exist.
        */
        PushButton* getIncreaseButton() const;

        /*!
        \brief
            Return a pointer to the 'decrease' PushButton component widget for
            this FreeSpinner.

        \return
            Pointer to a PushButton object.

        \exception UnknownObjectException
            Thrown if the 'decrease' PushButton component does not exist.
        */
        PushButton* getDecreaseButton() const;

        /*!
        \brief
            Return a pointer to the Editbox component widget for this FreeSpinner.

        \return
            Pointer to a Editbox object.

        \exception UnknownObjectException
            Thrown if the Editbox component does not exist.
        */
        Editbox* getEditbox() const;

        /*************************************************************************
        	Overrides for Event handler methods
        *************************************************************************/
        virtual	void onFontChanged(WindowEventArgs& e);
        virtual void onTextChanged(WindowEventArgs& e);
        virtual void onActivated(ActivationEventArgs& e);

        /*************************************************************************
        	New Event handler methods
        *************************************************************************/
        /*!
        \brief
            Method called when the FreeSpinner value changes.

        \param e
            WindowEventArgs object containing any relevant data.

        \return
            Nothing.
        */
        virtual void onValueChanged(WindowEventArgs& e);

        /*!
        \brief
            Method called when the step value changes.

        \param e
            WindowEventArgs object containing any relevant data.

        \return
            Nothing.
        */
        virtual void onStepChanged(WindowEventArgs& e);

        /*!
        \brief
            Method called when the maximum value setting changes.

        \param e
            WindowEventArgs object containing any relevant data.

        \return
            Nothing.
        */
        virtual void onMaximumValueChanged(WindowEventArgs& e);

        /*!
        \brief
            Method called when the minimum value setting changes.

        \param e
            WindowEventArgs object containing any relevant data.

        \return
            Nothing.
        */
        virtual void onMinimumValueChanged(WindowEventArgs& e);

        /*!
        \brief
            Method called when the text input/display mode is changed.

        \param e
            WindowEventArgs object containing any relevant data.

        \return
            Nothing.
        */
        virtual void onTextInputModeChanged(WindowEventArgs& e);

        /*************************************************************************
        	Internal event listener methods
        *************************************************************************/
        bool handleIncreaseButton(const EventArgs& e);
        bool handleDecreaseButton(const EventArgs& e);
        bool handleEditTextChange(const EventArgs& e);


        /*************************************************************************
        	Data Fields
        *************************************************************************/
        double   d_stepSize;     //!< Step size value used y the increase & decrease buttons.
        double   d_currentValue; //!< Numerical copy of the text in d_editbox.
        double   d_maxValue;     //!< Maximum value for FreeSpinner.
        double   d_minValue;     //!< Minimum value for FreeSpinner.
        TextInputMode   d_inputMode;    //!< Current text display/input mode.

    private:
        /*************************************************************************
        	Private Implementation Methods
        *************************************************************************/
        /*!
        \brief
            Adds properties supported by the FreeSpinner class.

        \return
            Nothing.
        */
        void    addFreeSpinnerProperties(void);
    };



template<>
class PropertyHelper<FreeSpinner::TextInputMode>
{
public:
    typedef FreeSpinner::TextInputMode return_type;
    typedef return_type safe_method_return_type;
    typedef FreeSpinner::TextInputMode pass_type;
    typedef String string_return_type;

    static const String& getDataTypeName()
    {
        static String type("TextInputMode");

        return type;
    }

    static return_type fromString(const String& str)
    {
        if (str == "FloatingPoint")
        {
            return FreeSpinner::FloatingPoint;
        }
        else if (str == "Hexadecimal")
        {
            return FreeSpinner::Hexadecimal;
        }
        else if (str == "Octal")
        {
            return FreeSpinner::Octal;
        }
        else
        {
            return FreeSpinner::Integer;
        }
    }

    static string_return_type toString(pass_type val)
    {
        if (val == FreeSpinner::Octal)
        {
            return "Octal";
        }
        else if (val == FreeSpinner::FloatingPoint)
        {
            return "FloatingPoint";
        }
        else if (val == FreeSpinner::Hexadecimal)
        {
            return "Hexadecimal";
        }
        else if (val == FreeSpinner::Integer)
        {
            return "Integer";
        }
        else
        {
            assert(false && "Invalid Text Input Mode");
            return "FloatingPoint";
        }
    }
};



} // End of  CEGUI namespace section

#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

#endif	// end of guard _CEGUIFreeSpinner_h_
