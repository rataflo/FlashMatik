#ifndef parameters_h
#define parameters_h

#include "constants.h"
#include <EEPROMex.h>
#include <EEPROMVar.h>

class ParametersHandler {
public:
    storage params;

    storage getParameters();
    void loadParameters();
    void updateParameters();
};

extern ParametersHandler parameters; // Declare the global instance of Parameters

#endif