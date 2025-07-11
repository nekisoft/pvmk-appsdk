/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 OpenBOR Team
 */

#include "../../openbor.h"

#define MAX_MODELNAME_SIZE 1024

void createModelList(void);
void freeModelList(void);
void addModel(s_model *model);
void deleteModel(char *modelname);
s_model *findmodel(char *modelname);
s_model *getFirstModel(void);
s_model *getCurrentModel(void);
s_model *getNextModel(void);
int isLastModel(void);
